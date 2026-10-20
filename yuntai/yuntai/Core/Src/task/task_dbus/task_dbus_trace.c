/**
 * @file task_dbus_trace.c
 * @brief DBUS 任务中的原始帧、命令变化采集和批量文本发送。
 *
 * R 行保留每个合法接收帧，C 行保留发布命令及鼠标累计输入。
 * 同一任务独占固定环形缓冲；不分配堆，不增加任务，不在 ISR 格式化。
 * DMA 接受前不移动读索引。溢出丢新记录并计数，不覆盖已经排队的时间线。
 */
#include "task/task_dbus/task_dbus_trace.h"
#include "task/task_dbus/task_dbus_config.h"
#include "app/log/log.h"
#include "stm32f4xx_hal.h"
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#if !(LOG_GLOBAL_ENABLE && LOG_USART1_ENABLE && LOG_DBUS_TRACE_ENABLE && !LOG_INS_MAG_CALIBRATION_ENABLE)

/* 编译关闭时不保留记录缓冲或格式化；控制调用点保持同一接口。 */
void DbusTrace_RecordFrame(const Dbus_ReceivedFrameTypeDef *frame) { (void)frame; }

/* 关闭采集不读取输入，也不改变已发布命令。 */
void DbusTrace_RecordCommand(const Dbus_SnapshotTypeDef *input, uint32_t control_ms,
    float virtual_x, float virtual_y, float yaw, float pitch, bool enabled,
    uint8_t x_state, uint8_t y_state) {
  (void)input; (void)control_ms; (void)virtual_x; (void)virtual_y;
  (void)yaw; (void)pitch; (void)enabled; (void)x_state; (void)y_state;
}

/* 关闭采集时没有待发送状态。 */
void DbusTrace_Flush(const Dbus_SnapshotTypeDef *input, uint32_t now_ms) {
  (void)input; (void)now_ms;
}

#else

#define DBUS_TRACE_INTERVAL_UNKNOWN_US UINT32_MAX /* 无前帧或间隔过长时的微秒间隔哨兵。 */
#define DBUS_TRACE_STATUS_MS 1000U /* 接收、排队与丢样统计周期，HAL ms。 */

_Static_assert(DBUS_TRACE_CAPACITY >= 16U && DBUS_TRACE_CAPACITY <= 512U,
               "trace capacity must be 16..512 records");
_Static_assert(DBUS_TRACE_BATCH_BYTES >= 256U && DBUS_TRACE_BATCH_BYTES <= 767U,
               "trace batch must fit the shared log buffer");
_Static_assert(DBUS_TRACE_HEARTBEAT_MS > 0U && DBUS_TRACE_HEARTBEAT_MS <= INT32_MAX,
               "trace heartbeat must fit positive HAL half-range");

/** 已量化的命令副本；单位均为 0.001‰，只用于诊断比较与输出。 */
typedef struct {
  int32_t virtual_x; /* 虚拟鼠标 X，0.001‰。 */
  int32_t virtual_y; /* 虚拟鼠标 Y，0.001‰。 */
  int32_t yaw; /* 合成 Yaw，0.001‰。 */
  int32_t pitch; /* 合成 Pitch，0.001‰。 */
  uint32_t flags; /* bit0接收在线、bit1命令许可、bit2虚拟鼠标开关；bit4/6起为X/Y两位阶段。 */
} DbusTrace_CommandTypeDef;

/** 环形缓冲的一条记录；kind 选择接收帧或命令联合体，所有内容均按值保存。 */
typedef struct {
  uint32_t id; /* 观察记录序号；被丢弃的记录同样消耗序号，可定位日志缺口。 */
  bool is_frame; /* true=R接收帧，false=C命令记录。 */
  union {
    struct {
      Dbus_ReceivedFrameTypeDef received; /* 原始帧及接收时刻。 */
      uint32_t interval_us; /* 与前一个合法观察帧的周期差换算微秒；未知用哨兵。 */
    } frame;
    struct {
      DbusTrace_CommandTypeDef value; /* 同周期已发布命令的量化副本。 */
      uint32_t sample_ms; /* 记录命令时的 HAL ms，区别于算法使用时刻。 */
      uint32_t cycles; /* 记录命令时的 DWT 周期计数。 */
      uint32_t control_ms; /* 本周期算法使用的 HAL ms。 */
      uint32_t receive_ms; /* 此命令引用的最新合法帧 HAL ms；不会因重发续期。 */
      uint32_t sequence; /* 此命令引用的最新合法帧序号。 */
      int32_t delta_x; /* 本周期实际交给算法的累计 X 位移，count。 */
      int32_t delta_y; /* 本周期实际交给算法的累计 Y 位移，count。 */
      uint32_t frames; /* 本周期累计包含的合法帧数；可大于1。 */
    } command;
  } data;
} DbusTrace_RecordTypeDef;

static DbusTrace_RecordTypeDef g_records[DBUS_TRACE_CAPACITY]; /* DBUS 任务独占，ISR 不访问。 */
static uint16_t g_head; /* 下次写入槽位。 */
static uint16_t g_count; /* 当前待发送记录数，不使用空槽区分满/空。 */
static uint16_t g_high_water; /* 历史最大待发记录数。 */
static uint32_t g_next_id; /* 包含丢样的观察记录累计序号，可回绕。 */
static uint32_t g_dropped; /* 日志环满时丢弃的新记录数，不等于 DBUS 接收丢帧。 */
static uint32_t g_rejected; /* 日志提交未被接受的次数，不等于丢样。 */
static uint32_t g_sent; /* 已交给日志 DMA 后端的记录数，不证明上位机已经接收。 */
static uint32_t g_previous_frame_ms; /* 前一个观察帧接收 HAL ms。 */
static uint32_t g_previous_frame_cycles; /* 前一个观察帧的 DWT 周期计数。 */
static bool g_have_frame; /* 是否有前一个观察帧，0时间戳不作哨兵。 */
static DbusTrace_CommandTypeDef g_previous_command; /* 最近一次被观察的命令，含阶段和许可。 */
static uint32_t g_previous_sequence; /* 最近一次被观察的命令引用的帧序号。 */
static uint32_t g_previous_command_ms; /* 最近命令采样 HAL ms，用于静止心跳。 */
static bool g_have_command; /* 前一条命令比较基准是否有效。 */
static uint8_t g_header; /* 字段说明发送进度，0~4；失败不前进。 */
static uint32_t g_last_status_ms; /* 最近成功提交统计的 HAL ms。 */
static char g_batch[DBUS_TRACE_BATCH_BYTES]; /* 批量格式化暂存；唯一日志后端复制后可复用。 */

/* 先为每个观察事件编号，再申请空槽；满时保留旧记录并留下显式序号缺口。 */
static DbusTrace_RecordTypeDef *DbusTrace_Reserve(void) {
  ++g_next_id;
  if (g_count == DBUS_TRACE_CAPACITY) {
    ++g_dropped;
    return NULL;
  }
  DbusTrace_RecordTypeDef *record = &g_records[g_head];
  *record = (DbusTrace_RecordTypeDef){0};
  record->id = g_next_id;
  g_head = (uint16_t)((g_head + 1U) % DBUS_TRACE_CAPACITY);
  ++g_count;
  if (g_count > g_high_water) { g_high_water = g_count; }
  return record;
}

/* 有限范围内四舍五入到 0.001‰；非有限值用 INT32_MIN 标记，不把它写回控制。 */
static int32_t DbusTrace_Quantize(float value) {
  if (!isfinite(value) || value < -1000.0f || value > 1000.0f) { return INT32_MIN; }
  return (int32_t)lroundf(value * 1000.0f);
}

/* 接收观察只复制值；长间隔不能使用一次模周期差伪装成准确帧间隔。 */
void DbusTrace_RecordFrame(const Dbus_ReceivedFrameTypeDef *frame) {
  if (frame == NULL) { return; }
  uint32_t interval_us = DBUS_TRACE_INTERVAL_UNKNOWN_US;
  if (g_have_frame && frame->timestamp_ms - g_previous_frame_ms < DBUS_TRACE_STATUS_MS &&
      SystemCoreClock >= 1000000U) {
    interval_us = (uint32_t)((uint64_t)(uint32_t)(frame->timestamp_cycles -
        g_previous_frame_cycles) * 1000000U / SystemCoreClock);
  }
  g_have_frame = true;
  g_previous_frame_ms = frame->timestamp_ms;
  g_previous_frame_cycles = frame->timestamp_cycles;
  DbusTrace_RecordTypeDef *record = DbusTrace_Reserve();
  if (record != NULL) {
    record->is_frame = true;
    record->data.frame.received = *frame;
    record->data.frame.interval_us = interval_us;
  }
}

/* 新帧保留一次消费现场；无新帧时仍捕获保持、回中、清零和许可变化。 */
void DbusTrace_RecordCommand(const Dbus_SnapshotTypeDef *input, uint32_t control_ms,
    float virtual_x, float virtual_y, float yaw, float pitch, bool enabled,
    uint8_t x_state, uint8_t y_state) {
  if (input == NULL) { return; }
  const DbusTrace_CommandTypeDef value = {
      .virtual_x = DbusTrace_Quantize(virtual_x), .virtual_y = DbusTrace_Quantize(virtual_y),
      .yaw = DbusTrace_Quantize(yaw), .pitch = DbusTrace_Quantize(pitch),
      .flags = (input->online ? 1U : 0U) | (enabled ? 2U : 0U) |
          (DBUS_MOUSE_VIRTUAL_ENABLE ? 4U : 0U) |
          ((uint32_t)(x_state & 3U) << 4U) | ((uint32_t)(y_state & 3U) << 6U),
  };
  const uint32_t sample_ms = HAL_GetTick();
  const uint32_t cycles = DWT->CYCCNT;
  if (g_have_command && input->valid_frames == g_previous_sequence &&
      memcmp(&value, &g_previous_command, sizeof(value)) == 0 &&
      sample_ms - g_previous_command_ms < DBUS_TRACE_HEARTBEAT_MS) { return; }
  g_have_command = true;
  g_previous_command = value;
  g_previous_sequence = input->valid_frames;
  g_previous_command_ms = sample_ms;
  DbusTrace_RecordTypeDef *record = DbusTrace_Reserve();
  if (record != NULL) {
    record->data.command.value = value;
    record->data.command.sample_ms = sample_ms;
    record->data.command.cycles = cycles;
    record->data.command.control_ms = control_ms;
    record->data.command.receive_ms = input->timestamp_ms;
    record->data.command.sequence = input->valid_frames;
    record->data.command.delta_x = input->mouse_delta_x;
    record->data.command.delta_y = input->mouse_delta_y;
    record->data.command.frames = input->mouse_delta_frames;
  }
}

/* 紧凑 CSV 行用固定字段顺序保留精度，中文说明单独发送，避免每帧重复字段名。 */
static int DbusTrace_Format(char *buffer, size_t capacity, const DbusTrace_RecordTypeDef *record) {
  if (record->is_frame) {
    const Dbus_ReceivedFrameTypeDef *frame = &record->data.frame.received;
    const Dbus_DataTypeDef *data = &frame->data;
    const uint32_t flags = data->switch_left | ((uint32_t)data->switch_right << 2U) |
        ((uint32_t)data->mouse_left << 4U) | ((uint32_t)data->mouse_right << 5U);
    return snprintf(buffer, capacity,
        "R,%lu,%lu,%lu,%lu,%lu,%d,%d,%d,%d,%d,%d,%d,%d,%u,%lu\r\n",
        (unsigned long)record->id, (unsigned long)frame->sequence,
        (unsigned long)frame->timestamp_ms, (unsigned long)frame->timestamp_cycles,
        (unsigned long)record->data.frame.interval_us,
        (int)data->channels[0], (int)data->channels[1], (int)data->channels[2],
        (int)data->channels[3], (int)data->channels[4],
        (int)data->mouse_x, (int)data->mouse_y, (int)data->mouse_z,
        (unsigned)data->keyboard, (unsigned long)flags);
  }
  return snprintf(buffer, capacity,
      "C,%lu,%lu,%lu,%lu,%lu,%lu,%ld,%ld,%lu,%ld,%ld,%ld,%ld,%lu\r\n",
      (unsigned long)record->id, (unsigned long)record->data.command.sequence,
      (unsigned long)record->data.command.sample_ms, (unsigned long)record->data.command.cycles,
      (unsigned long)record->data.command.control_ms, (unsigned long)record->data.command.receive_ms,
      (long)record->data.command.delta_x, (long)record->data.command.delta_y,
      (unsigned long)record->data.command.frames,
      (long)record->data.command.value.virtual_x, (long)record->data.command.value.virtual_y,
      (long)record->data.command.value.yaw, (long)record->data.command.value.pitch,
      (unsigned long)record->data.command.value.flags);
}

/* 字段说明只在 DMA 接受后推进；格式全部使用整数，兼容 newlib-nano。 */
static void DbusTrace_Header(void) {
  bool accepted = false;
  switch (g_header) {
  case 0:
    accepted = LOG_TRY_PRINTF(LOG_CATEGORY_DBUS_TRACE,
        "[输入采集] 版本=1,周期频率Hz=%lu,任务周期ms=%u,命令单位=0.001千分比,周期计数32位回绕\r\n",
        (unsigned long)SystemCoreClock, (unsigned)DBUS_TASK_PERIOD_MS);
    break;
  case 1:
    accepted = LOG_TRY_PRINTF(LOG_CATEGORY_DBUS_TRACE,
        "[输入采集] R,记录序号,接收帧序号,接收ms,接收周期计数,帧间隔us,CH0,CH1,CH2,CH3,CH4,鼠标X,鼠标Y,鼠标Z,键盘位图,按键拨杆位图\r\n"
        "[输入采集] R位图:位0~1左拨杆,位2~3右拨杆,位4左键,位5右键;未知间隔=4294967295\r\n");
    break;
  case 2:
    accepted = LOG_TRY_PRINTF(LOG_CATEGORY_DBUS_TRACE,
        "[输入采集] C,记录序号,引用帧序号,记录ms,记录周期计数,算法ms,引用接收ms,累计鼠标X,累计鼠标Y,累计帧数,虚拟X,虚拟Y,Yaw命令,Pitch命令,状态位图\r\n"
        "[输入采集] C位图:位0在线,位1命令有效,位2鼠标启用,位4~5为X阶段,位6~7为Y阶段;阶段0空闲1保持2回中3输入\r\n");
    break;
  default:
    accepted = LOG_TRY_PRINTF(LOG_CATEGORY_DBUS_TRACE,
        "[输入采集] S,统计ms,合法帧,非法帧,接收溢出,重同步,串口错误,DMA错误,启动错误,日志丢样,待发,最大待发,提交拒绝,已提交记录\r\n"
        "[输入采集] 鼠标=位移测速,参考周期ms=%lu,增益乘1000=%ld/%ld,缺帧保持ms=%lu/%lu,缺帧回中ms=%lu/%lu,方向=%d/%d,上限千分比=%d\r\n"
        "[输入采集] 摇杆死区count=%d,Yaw/Pitch通道=%u/%u,摇杆方向=%d/%d\r\n",
        (unsigned long)DBUS_MOUSE_VIRTUAL_REFERENCE_PERIOD_MS,
        (long)(DBUS_YAW_MOUSE_VIRTUAL_GAIN_PERMILLE_PER_COUNT * 1000.0f),
        (long)(DBUS_PITCH_MOUSE_VIRTUAL_GAIN_PERMILLE_PER_COUNT * 1000.0f),
        (unsigned long)DBUS_YAW_MOUSE_VIRTUAL_HOLD_MS, (unsigned long)DBUS_PITCH_MOUSE_VIRTUAL_HOLD_MS,
        (unsigned long)DBUS_YAW_MOUSE_VIRTUAL_DECAY_MS, (unsigned long)DBUS_PITCH_MOUSE_VIRTUAL_DECAY_MS,
        (int)DBUS_YAW_MOUSE_VIRTUAL_SIGN, (int)DBUS_PITCH_MOUSE_VIRTUAL_SIGN,
        (int)DBUS_MOUSE_VIRTUAL_OUTPUT_LIMIT_PERMILLE, (int)DBUS_STICK_DEADBAND_RAW,
        (unsigned)DBUS_YAW_CHANNEL, (unsigned)DBUS_PITCH_CHANNEL,
        (int)DBUS_YAW_INPUT_SIGN, (int)DBUS_PITCH_INPUT_SIGN);
    break;
  }
  if (accepted) { ++g_header; } else { ++g_rejected; }
}

/* 一次只提交一个批次，先核对完整行长度；不把 snprintf 的截断内容交给串口。 */
void DbusTrace_Flush(const Dbus_SnapshotTypeDef *input, uint32_t now_ms) {
  if (input == NULL) { return; }
  if (g_header < 4U) { DbusTrace_Header(); return; }
  if (now_ms - g_last_status_ms >= DBUS_TRACE_STATUS_MS) {
    if (LOG_TRY_PRINTF(LOG_CATEGORY_DBUS_TRACE,
        "S,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%u,%u,%lu,%lu\r\n",
        (unsigned long)now_ms, (unsigned long)input->valid_frames,
        (unsigned long)input->invalid_frames, (unsigned long)input->frame_overruns,
        (unsigned long)input->resync_count, (unsigned long)input->uart_errors,
        (unsigned long)input->dma_errors, (unsigned long)input->start_errors,
        (unsigned long)g_dropped, (unsigned)g_count, (unsigned)g_high_water,
        (unsigned long)g_rejected, (unsigned long)g_sent)) {
      g_last_status_ms = now_ms;
    } else { ++g_rejected; }
    return;
  }
  size_t used = 0U;
  uint16_t count = 0U;
  const uint16_t tail = (uint16_t)((g_head + DBUS_TRACE_CAPACITY - g_count) % DBUS_TRACE_CAPACITY);
  while (count < g_count) {
    const uint16_t index = (uint16_t)((tail + count) % DBUS_TRACE_CAPACITY);
    const int length = DbusTrace_Format(g_batch + used, sizeof(g_batch) - used, &g_records[index]);
    if (length <= 0 || (size_t)length >= sizeof(g_batch) - used) { break; }
    used += (size_t)length;
    ++count;
  }
  g_batch[used] = '\0';
  if (count != 0U) {
    if (LOG_TRY_PRINTF(LOG_CATEGORY_DBUS_TRACE, "%s", g_batch)) {
      g_count = (uint16_t)(g_count - count);
      g_sent += count;
    } else { ++g_rejected; }
  }
}

#endif
