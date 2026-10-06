/**
 * @file task_dbus_runtime.c
 * @brief DBUS 摇杆、鼠标和左键的任务适配。
 *
 * PC11/USART3 的 18 字节 DBUS 协议。
 * 仅 DBUS 任务调用。CH0→Yaw、CH1→Pitch；鼠标 X/Y 经虚拟摇杆模块累计为速度。
 * 合成速度限制为 ±1000‰。左键按住发布连续发射意图。
 * 三个命令均使用接收帧 HAL ms；重复发布不续期，离线使全部命令失效。
 * 本模块只发布命令和限频日志，不写 CAN 电流或 PWM。
 */
#include "task/task_dbus/task_dbus_runtime.h"
#include "task/task_dbus/task_dbus_config.h"
#include "bsp/dbus/dbus.h"
#include "algorithm/mouse_virtual_joystick/mouse_virtual_joystick.h"
#include "task/task_yaw/task_yaw_command.h"
#include "task/task_pitch/task_pitch_command.h"
#include "task/task_feed_motor/task_feed_motor_command.h"
#include "app/log/log.h"
#include <math.h>
#include "stm32f4xx_hal.h"

/* 先在编译期拒绝无意义死区、非摇杆索引和错误方向，防止开机后发布不可解释命令。 */
_Static_assert(DBUS_STICK_DEADBAND_RAW >= 0 && DBUS_STICK_DEADBAND_RAW < DBUS_CHANNEL_SPAN,
               "DBUS deadband must be smaller than full stick range");
_Static_assert(DBUS_YAW_CHANNEL < 4U && DBUS_PITCH_CHANNEL < 4U,
               "DBUS gimbal input must use a stick channel");
_Static_assert((DBUS_YAW_INPUT_SIGN == 1 || DBUS_YAW_INPUT_SIGN == -1) &&
               (DBUS_PITCH_INPUT_SIGN == 1 || DBUS_PITCH_INPUT_SIGN == -1),
               "DBUS input signs must be +1 or -1");

static MouseVirtualJoystick_HandleTypeDef g_mouse_virtual; /* DBUS 任务独占；X/Y 各自累计、保持和回中。 */
static bool g_mouse_virtual_ready; /* 配置初始化成功标志；失败不能发布有效鼠标速度。 */

/* 首次任务周期建立算法状态；失败时保持未就绪，下一周期重试并发布零鼠标量。 */
static void DbusTask_InitMouseVirtual(void) {
  const float yaw_gain = DBUS_YAW_MOUSE_VIRTUAL_GAIN_PERMILLE_PER_COUNT;
  const float pitch_gain = DBUS_PITCH_MOUSE_VIRTUAL_GAIN_PERMILLE_PER_COUNT;
  /* 浮点增益在初始化时检查。非法配置使两轴命令失效，不用整数断言截断小数。 */
  if (!isfinite(yaw_gain) || yaw_gain < 0.0f || !isfinite(pitch_gain) || pitch_gain < 0.0f) {
    g_mouse_virtual_ready = false;
    return;
  }
  const MouseVirtualJoystick_ConfigTypeDef config = {
      .x = {
          .gain_permille_per_count = yaw_gain * DBUS_YAW_MOUSE_VIRTUAL_SIGN,
          .hold_ms = DBUS_YAW_MOUSE_VIRTUAL_HOLD_MS,
          .decay_ms = DBUS_YAW_MOUSE_VIRTUAL_DECAY_MS,
      },
      .y = {
          .gain_permille_per_count = pitch_gain * DBUS_PITCH_MOUSE_VIRTUAL_SIGN,
          .hold_ms = DBUS_PITCH_MOUSE_VIRTUAL_HOLD_MS,
          .decay_ms = DBUS_PITCH_MOUSE_VIRTUAL_DECAY_MS,
      },
      .output_limit_permille = DBUS_MOUSE_VIRTUAL_OUTPUT_LIMIT_PERMILLE,
  };
  g_mouse_virtual_ready = MouseVirtualJoystick_Init(&g_mouse_virtual, &config);
}

#if LOG_GLOBAL_ENABLE && LOG_USART1_ENABLE && (LOG_DBUS_TEXT_ENABLE || LOG_DBUS_CHART_ENABLE)
_Static_assert(DBUS_TASK_LOG_PERIOD_MS > 0U && DBUS_TASK_LOG_PERIOD_MS <= INT32_MAX,
               "telemetry period must fit positive half-range of HAL tick");
_Static_assert(sizeof(LOG_CHART_PREFIX) > 1U && sizeof(LOG_CHART_PREFIX) <= 17U,
               "chart prefix must contain 1..16 bytes");
static uint32_t g_last_log_ms; /* 最近成功交给 UART DMA 的遥测时刻，单位 HAL ms（忙时不提前续期）。 */

#if LOG_DBUS_CHART_ENABLE
/* 将拨杆和鼠标按钮压缩为图表第 8 通道；每个字段占用两位或一位，便于上位机按位解析。 */
static uint32_t DbusTask_PackChartStatus(const Dbus_DataTypeDef *data) {
  if (data == NULL) {
    return 0U;
  }
  return ((uint32_t)(data->switch_left & 0x03U)) |
         ((uint32_t)(data->switch_right & 0x03U) << 2U) |
         ((uint32_t)(data->mouse_left & 0x01U) << 4U) |
         ((uint32_t)(data->mouse_right & 0x01U) << 5U);
}

#endif


#endif

/* 原始摇杆量换千分比；先死区再缩放、最后限幅（噪声不能持续积分目标，越界不能放大转速）。 */
static float DbusTask_MapChannel(int16_t channel, int32_t sign) {
  if (channel >= -DBUS_STICK_DEADBAND_RAW && channel <= DBUS_STICK_DEADBAND_RAW) {
    return 0.0f;
  }
  /* 死区外按官方 ±660 raw 满量程线性缩放；不减去死区宽度。
   * int32_t 承接乘法，避免 int16_t 中间结果溢出改变方向；±1000‰ 只是速度意图。 */
  float velocity = (float)channel * 1000.0f * (float)sign /
      (float)DBUS_CHANNEL_SPAN;
  if (velocity > 1000.0f) {
    velocity = 1000.0f;
  } else if (velocity < -1000.0f) {
    velocity = -1000.0f;
  }
  return velocity;
}

/* 合成摇杆和鼠标速度，再限制到命令范围。浮点保留鼠标小增量。 */
static float DbusTask_Combine(float stick, float mouse) {
  const float velocity = stick + mouse;
  return velocity > 1000.0f ? 1000.0f :
      velocity < -1000.0f ? -1000.0f : velocity;
}

/**
 * @brief 处理当前 DBUS 快照并向两轴和供弹发布同源命令（不靠转发旧帧刷新有效期）。
 * @param now_ms HAL_GetTick 的当前 ms，不能传 FreeRTOS Tick。
 * @retval None 更新命令邮箱与日志时刻。
 * @note 仅 DBUS 任务调用，作为两轴与供弹命令的唯一发布者；UART 忙时不等待。
 */
void DbusTask_RuntimeRunCycle(uint32_t now_ms) {
  /* 先消费 ISR 邮箱并处理 DMA 恢复，再一次取得数据和 online。
   * 本周期所有输入用同一帧，不能为 Yaw/Pitch/左键分别取快照。（三份命令来自同一张纸条。） */
  Dbus_Process(now_ms);
  Dbus_SnapshotTypeDef input = {0};
  const bool snapshot_ok = Dbus_GetSnapshot(now_ms, &input);
  /* DMA 可能在入口取 now_ms 后才完成新帧。只在此竞态重读 HAL 时刻，
   * 不把真实的稍新帧误判成时间回退；原接收时间戳保持不变。（先拿纸条，再看钟。） */
  if (snapshot_ok && input.valid && now_ms - input.timestamp_ms > INT32_MAX) {
    now_ms = HAL_GetTick();
  }
  const bool available = snapshot_ok && input.valid && input.online;
  if (!g_mouse_virtual_ready) {
    DbusTask_InitMouseVirtual();
  }
  float virtual_x = 0.0f;
  float virtual_y = 0.0f;
  bool gimbal_available = available;
  if (available && g_mouse_virtual_ready && DBUS_MOUSE_VIRTUAL_ENABLE) {
    gimbal_available = MouseVirtualJoystick_Update(&g_mouse_virtual, now_ms, input.valid_frames,
                                                   input.timestamp_ms, input.mouse_delta_x,
                                                   input.mouse_delta_y, true, &virtual_x, &virtual_y);
  } else if (g_mouse_virtual_ready) {
    MouseVirtualJoystick_Reset(&g_mouse_virtual);
  } else if (DBUS_MOUSE_VIRTUAL_ENABLE) {
    gimbal_available = false; /* 初始化失败时不能绕过输入算法继续发布两轴有效速度。 */
  }
  /* 摇杆先做死区，再叠加虚拟鼠标速度。虚拟模块用 valid_frames 去重，
   * 因此任务重复读取同一快照不会重复放大相对位移。 */
  const Yaw_CommandTypeDef yaw = {
      .velocity_permille = gimbal_available ? DbusTask_Combine(
          DbusTask_MapChannel(input.data.channels[DBUS_YAW_CHANNEL], DBUS_YAW_INPUT_SIGN),
          virtual_x) : 0.0f,
      .enabled = gimbal_available,
      .timestamp_ms = input.timestamp_ms,
  };
  /* Pitch 独立使用 CH1 与 mouse_y，不能把 Yaw 电流极性当作输入符号。
   * 输入或算法失效同时禁用并清零速度，最后合法帧只保留为诊断信息。 */
  const Pitch_CommandTypeDef pitch = {
      .velocity_permille = gimbal_available ? DbusTask_Combine(
          DbusTask_MapChannel(input.data.channels[DBUS_PITCH_CHANNEL], DBUS_PITCH_INPUT_SIGN),
          virtual_y) : 0.0f,
      .enabled = gimbal_available,
      .timestamp_ms = input.timestamp_ms,
  };
  /* 两轴都用接收时刻；即使本周期没有新帧也不把 now_ms 写入命令（否则断链后旧速度永远不过期）。 */
  (void)YawCommand_Submit(&yaw);
  (void)PitchCommand_Submit(&pitch);
  /* 左键按住是一份持续请求，不是事件队列；松开或离线会覆盖上一份请求。
   * 三个邮箱分别发布，各消费者只保证自身快照一致；不能宣称三任务同时执行新命令。 */
  const FeedMotor_CommandTypeDef feed = {
    .enabled = available,
    .fire_requested = available && input.data.mouse_left == 1U,
    .timestamp_ms = input.timestamp_ms,
  };
  (void)FeedMotorCommand_Submit(&feed);
  /* 合法命令已发布后才尝试遥测。文本和图表共用一次 DMA 提交；忙或过长时下一周期重试。 */
#if LOG_GLOBAL_ENABLE && LOG_USART1_ENABLE && (LOG_DBUS_TEXT_ENABLE || LOG_DBUS_CHART_ENABLE)
  if (now_ms - g_last_log_ms >= DBUS_TASK_LOG_PERIOD_MS) {
#if LOG_DBUS_CHART_ENABLE
    const uint32_t status = DbusTask_PackChartStatus(&input.data);
#endif
    /* 两类格式各自按宏拼接，全部字段只写一份；关闭分类也删除相应格式化参数。 */
    const bool accepted = LOG_TRY_PRINTF(LOG_CATEGORY_DBUS_TELEMETRY,
#if LOG_DBUS_TEXT_ENABLE
        "[DBUS] online=%u age_ms=%lu ch0=%d ch1=%d ch2=%d ch3=%d ch4=%d "
        "sw_left=%u sw_right=%u mouse_x=%d mouse_y=%d mouse_z=%d "
        "mouse_left=%u mouse_right=%u keyboard=0x%04X yaw_x100=%ld pitch_x100=%ld fire=%u "
        "virtual_mouse_x_x100=%ld virtual_mouse_y_x100=%ld mouse_virtual_state=%u "
        "mouse_virtual_x_state=%u mouse_virtual_y_state=%u mouse_frame_sequence=%lu "
        "mouse_delta_x=%ld mouse_delta_y=%ld mouse_delta_frames=%u frame_overruns=%lu "
        "valid=%lu invalid=%lu resync=%lu uart_err=%lu dma_err=%lu start_err=%lu\r\n"
#endif
#if LOG_DBUS_CHART_ENABLE
        LOG_CHART_PREFIX "%d,%d,%d,%d,%d,%d,%d,%d,%lu,%u\n"
#endif
#if LOG_DBUS_TEXT_ENABLE
        , input.online ? 1U : 0U, (unsigned long)input.age_ms,
        (int)input.data.channels[0], (int)input.data.channels[1],
        (int)input.data.channels[2], (int)input.data.channels[3],
        (int)input.data.channels[4], (unsigned)input.data.switch_left,
        (unsigned)input.data.switch_right, (int)input.data.mouse_x,
        (int)input.data.mouse_y, (int)input.data.mouse_z,
        (unsigned)input.data.mouse_left, (unsigned)input.data.mouse_right,
        (unsigned)input.data.keyboard, (long)(yaw.velocity_permille * 100.0f),
        (long)(pitch.velocity_permille * 100.0f), feed.fire_requested ? 1U : 0U,
        (long)(virtual_x * 100.0f), (long)(virtual_y * 100.0f),
      (unsigned)g_mouse_virtual.state, (unsigned)g_mouse_virtual.x.state,
      (unsigned)g_mouse_virtual.y.state, (unsigned long)input.valid_frames,
      (long)input.mouse_delta_x, (long)input.mouse_delta_y,
      (unsigned)input.mouse_delta_frames, (unsigned long)input.frame_overruns,
      (unsigned long)input.valid_frames, (unsigned long)input.invalid_frames,
        (unsigned long)input.resync_count,
        (unsigned long)input.uart_errors, (unsigned long)input.dma_errors,
        (unsigned long)input.start_errors
#endif
#if LOG_DBUS_CHART_ENABLE
        , (int)input.data.channels[0], (int)input.data.channels[1],
        (int)input.data.channels[2], (int)input.data.channels[3],
        (int)input.data.channels[4], (int)lroundf(virtual_x),
        (int)lroundf(virtual_y), (int)input.data.mouse_z,
        (unsigned long)status, (unsigned)input.data.keyboard
#endif
        );
    if (accepted) {
      g_last_log_ms = now_ms;
    }
  }
#endif
}
