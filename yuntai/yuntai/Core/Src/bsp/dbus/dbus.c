/**
 * @file dbus.c
 * @brief USART3 DJI DBUS 双缓冲接收与一致快照。
 *
 * 协议参考 20.standard_robot/application/remote_control.c；每帧 18 字节。
 * PC11/USART3 使用 8 位数据加偶校验。DMA M0/M1 完成 ISR 只复制帧。
 * DBUS 任务解码副本并恢复 DMA；其它任务在短临界区取得数据与 HAL ms 年龄。
 * 接收代数使重启前旧帧失效。DBUS 无 CRC；字段校验不能发现全部传输错误。
 * 本模块不写电机，不把接收在线等同于机械安全。
 */
#include "bsp/dbus/dbus.h"
#include "FreeRTOS.h"
#include "task.h"
#include "usart.h"
#include <stddef.h>
#include <string.h>

typedef struct {
  uint8_t bytes[DBUS_FRAME_LENGTH]; /* 已完成 DMA 块的副本。任务不持有 DMA 内存。 */
  uint32_t timestamp_ms; /* ISR 完整帧时刻，单位 HAL ms。任务转发不刷新接收时刻。 */
  uint32_t epoch; /* 复制完成帧时的接收代数。失同步会改变，旧帧不能复活输入。 */
} Dbus_FrameTypeDef;

static uint8_t g_rx_buffers[2][DBUS_FRAME_LENGTH]; /* DMA 专有的两块内存。不能拿它们直接给任务解码。 */
#define DBUS_FRAME_MAILBOX_LENGTH 16U /* 固定邮箱槽位；任务短暂延迟时保留连续鼠标帧，溢出只记录次数。 */
static Dbus_FrameTypeDef g_frame_mailbox[DBUS_FRAME_MAILBOX_LENGTH]; /* ISR 写入、任务按先进先出读取。 */
static volatile uint8_t g_frame_mailbox_head; /* ISR 下一写入槽位；仅 ISR 修改。 */
static volatile uint8_t g_frame_mailbox_tail; /* 任务下一读取槽位；仅任务修改。 */
static volatile uint32_t g_frame_mailbox_overruns; /* 邮箱满时递增；不能伪造未收到的鼠标位移。 */
static Dbus_SnapshotTypeDef g_snapshot; /* 任务写入、任务快照读取，均在短临界区复制。 */
static uint32_t g_snapshot_epoch; /* 最近合法帧所属代数。与 ISR 的 epoch 比较。 */
static volatile uint32_t g_epoch; /* ISR 故障/失同步时递增。不是数据一致性保护的替代品。 */
static volatile uint32_t g_resync_count; /* ISR 记录边界重新建立次数。 */
static volatile uint32_t g_uart_errors; /* ISR 记录 UART 故障次数。 */
static volatile uint32_t g_dma_errors; /* ISR 记录 DMA 故障次数。 */
static volatile bool g_restart_requested; /* ISR 提交恢复请求。等待工作留给任务。 */
static volatile bool g_boundary_seen; /* 已看到帧间 IDLE。不能把半帧重启点当帧头。 */
static volatile bool g_synchronized; /* 收帧从已确认边界开始。启动前 false。 */
static volatile bool g_active; /* DMA 启动成功标志。不代表已有合法帧。 */
static volatile bool g_boundary_wait_armed; /* 已开启仅等待 IDLE 的同步窗口；此时不读取 DMA 帧。 */
static uint32_t g_last_retry_ms; /* 任务恢复重试时刻，单位 HAL ms。 */
static bool g_retry_wait; /* 启动失败后的限频标志。时间戳 0 也有效。 */

/* 拼接小端 16 位字段。逐字节移位不依赖 STM32 内存布局。不能把任意字节指针强转成 int16_t*。 */
static uint16_t Dbus_ReadU16(const uint8_t *bytes) {
  return (uint16_t)((uint16_t)bytes[0] | ((uint16_t)bytes[1] << 8));
}

/**
 * @brief 解码并校验一份 DBUS 帧。失败不改输出，不能靠坏数据刷新在线状态。
 * @param frame 至少 18 字节的只读帧。通道按 11 位打包，鼠标/键盘按小端解析。
 * @param data 成功时写入解析结果。
 * @retval true 字段合法。false 空指针或字段非法。
 * @note 纯计算，无 HAL/RTOS 操作。无 CRC，范围校验不能发现所有损坏。
 */
bool Dbus_DecodeFrame(const uint8_t *b, Dbus_DataTypeDef *data) {
  if (b == NULL || data == NULL) {
    return false;
  }
  Dbus_DataTypeDef decoded = {0};
  /* 四路 11 位通道跨字节排列。必须保留每字节 bit7。USART 7 位接收会在这里悄悄破坏摇杆值。 */
  decoded.channels[0] = (int16_t)(((b[0] | (b[1] << 8)) & 0x07FF) - DBUS_CHANNEL_CENTER);
  decoded.channels[1] = (int16_t)((((b[1] >> 3) | (b[2] << 5)) & 0x07FF) - DBUS_CHANNEL_CENTER);
  decoded.channels[2] = (int16_t)((((b[2] >> 6) | (b[3] << 2) | (b[4] << 10)) & 0x07FF) - DBUS_CHANNEL_CENTER);
  decoded.channels[3] = (int16_t)((((b[4] >> 1) | (b[5] << 7)) & 0x07FF) - DBUS_CHANNEL_CENTER);
  /* CH4 也是 11 位字段。不屏蔽高 5 位会把保留位带进滚轮值，进而溢出或误判帧。 */
  decoded.channels[4] = (int16_t)((Dbus_ReadU16(&b[16]) & 0x07FFU) -
                                  DBUS_CHANNEL_CENTER);
  /*
   * S1 左拨杆使用 DATA[5] bit6~7，S2 右拨杆使用 bit4~5。
   * 不能按变量名的自然顺序猜位序，否则日志中的左右拨杆会对调。
   */
  const uint8_t switch_bits = (uint8_t)((b[5] >> 4) & 0x0FU);
  decoded.switch_left = (switch_bits >> 2) & 0x03U; /* S1，DATA[5] bit6~7。 */
  decoded.switch_right = switch_bits & 0x03U; /* S2，DATA[5] bit4~5。 */
  decoded.mouse_x = (int16_t)Dbus_ReadU16(&b[6]);
  decoded.mouse_y = (int16_t)Dbus_ReadU16(&b[8]);
  decoded.mouse_z = (int16_t)Dbus_ReadU16(&b[10]);
  decoded.mouse_left = b[12];
  decoded.mouse_right = b[13];
  decoded.keyboard = Dbus_ReadU16(&b[14]);
  /* 协议没有帧头和 CRC，范围校验只能筛掉明显错位。坏帧不能覆盖最后合法输入或刷新在线时间。 */
  /* 仅四路摇杆使用 364~1684 的官方范围。滚轮 CH4 已按 11 位掩码，允许其独立物理范围。 */
  for (uint8_t i = 0U; i < 4U; ++i) {
    if (decoded.channels[i] < -DBUS_CHANNEL_SPAN || decoded.channels[i] > DBUS_CHANNEL_SPAN) {
      return false;
    }
  }
  /*
   * 拨杆保留值 0 不控制本轮输出许可，接收器切换瞬间仍可保留摇杆输入。
   * 鼠标按键只接受协议 0/1；其它值拒绝整帧，防止错位数据触发发射。
   */
  if (decoded.mouse_left > 1U || decoded.mouse_right > 1U) {
    return false;
  }
  *data = decoded;
  return true;
}

/* USART/DMA ISR 关闭接收请求并作废邮箱，递增接收代数；任务随后 Abort 和重启。
 * ISR 只写寄存器和标志，不等待 DMA 停稳；低优先级 HAL Tick 不能推进时，阻塞 Abort 会卡死。 */
static void Dbus_RequestRecovery(void) {
  if (huart3.Instance == NULL || huart3.hdmarx == NULL ||
      huart3.hdmarx->Instance == NULL) {
    return;
  }
  CLEAR_BIT(huart3.Instance->CR3, USART_CR3_DMAR | USART_CR3_EIE);
  CLEAR_BIT(huart3.Instance->CR1, USART_CR1_PEIE);
  __HAL_DMA_DISABLE(huart3.hdmarx);
  g_synchronized = false;
  g_active = false;
  g_boundary_seen = false;
  g_boundary_wait_armed = false;
  g_frame_mailbox_head = 0U;
  g_frame_mailbox_tail = 0U;
  g_restart_requested = true;
  ++g_epoch;
}

/* DMA ISR 将已完成的 M0/M1 复制到环形帧邮箱，并最后发布写索引；邮箱满时保留旧帧。
 * HAL 按切换后的 CT 选择回调，DMA 此时写另一块。任务只取副本，不直接读 DMA 活动内存。
 * 未同步、待恢复或索引越界时退出；否则半帧可能被误发布为鼠标发射命令。 */
static void Dbus_CopyCompleted(uint8_t index) {
  if (!g_active || !g_synchronized || g_restart_requested ||
      index >= 2U) {
    return;
  }
  /*
   * DMA 已切换到另一块后才复制当前完成块。不调用 FreeRTOS API，避免
   * DMA/USART 中断路径受到队列句柄和优先级配置影响。
   */
  const uint8_t next_head = (uint8_t)((g_frame_mailbox_head + 1U) %
                                      DBUS_FRAME_MAILBOX_LENGTH);
  if (next_head == g_frame_mailbox_tail) {
    ++g_frame_mailbox_overruns;
    return; /* 邮箱满时保留已排队帧；丢帧次数进入快照，禁止假造鼠标位移。 */
  }
  Dbus_FrameTypeDef *slot = &g_frame_mailbox[g_frame_mailbox_head];
  memcpy(slot->bytes, g_rx_buffers[index], DBUS_FRAME_LENGTH);
  slot->timestamp_ms = HAL_GetTick();
  slot->epoch = g_epoch;
  __DMB();
  g_frame_mailbox_head = next_head;
}

/* DMA ISR 的 M0 完成回调；仅处理 USART3 RX，DMA 已切换到 M1。 */
static void Dbus_DmaM0Complete(DMA_HandleTypeDef *hdma) {
  if (hdma == huart3.hdmarx) {
    Dbus_CopyCompleted(0U);
  }
}

/* DMA ISR 的 M1 完成回调；仅处理 USART3 RX，DMA 已切换到 M0。 */
static void Dbus_DmaM1Complete(DMA_HandleTypeDef *hdma) {
  if (hdma == huart3.hdmarx) {
    Dbus_CopyCompleted(1U);
  }
}

/* DMA 错误只记录并请求恢复。ISR 不 Abort、不打印、不进入 Error_Handler。 */
static void Dbus_DmaError(DMA_HandleTypeDef *hdma) {
  if (hdma == huart3.hdmarx) {
    ++g_dma_errors;
    Dbus_RequestRecovery();
  }
}

/* DMA 已停稳后一次配置双缓冲。调用方短临界区屏蔽优先级 5 IRQ。不能让中断看到半初始化状态。 */
static bool Dbus_Start(bool synchronized) {
  DMA_HandleTypeDef *hdma = huart3.hdmarx;
  if (huart3.Instance == NULL || hdma == NULL || hdma->Instance == NULL) {
    return false;
  }
  hdma->XferCpltCallback = Dbus_DmaM0Complete;
  hdma->XferM1CpltCallback = Dbus_DmaM1Complete;
  hdma->XferErrorCallback = Dbus_DmaError;
  hdma->XferHalfCpltCallback = NULL;
  hdma->XferM1HalfCpltCallback = NULL;
  hdma->XferAbortCallback = NULL;
  CLEAR_BIT(hdma->Instance->CR, DMA_SxCR_CT | DMA_IT_HT);
  __HAL_UART_CLEAR_IDLEFLAG(&huart3); /* SR→DR 清 IDLE 与接收错误。此时 DMA 未运行，不能与 DMA 抢读 DR。 */
  if (HAL_DMAEx_MultiBufferStart_IT(hdma, (uint32_t)&huart3.Instance->DR,
      (uint32_t)g_rx_buffers[0], (uint32_t)g_rx_buffers[1], DBUS_FRAME_LENGTH) != HAL_OK) {
    ++g_snapshot.start_errors;
    return false;
  }
  g_synchronized = synchronized;
  g_restart_requested = false;
  g_boundary_seen = false;
  g_active = true;
  g_boundary_wait_armed = false;
  SET_BIT(huart3.Instance->CR3, USART_CR3_DMAR | USART_CR3_EIE);
  SET_BIT(huart3.Instance->CR1, USART_CR1_IDLEIE | USART_CR1_PEIE);
  return true;
}

/**
 * @brief 初始化固定环形邮箱并启动 USART3 双缓冲。先有复制落点，再允许 ISR 写入。
 * @param None。
 * @retval true 接收已启动或等待恢复。false USART3 或 DMA 未初始化或启动失败。
 * @note USART3/DMA 必须先初始化。仅 DBUS 任务调用。ISR 不调用普通 RTOS API。
 */
bool Dbus_Init(void) {
  if (huart3.Instance == NULL || huart3.hdmarx == NULL ||
      huart3.hdmarx->Instance == NULL) {
    return false;
  }
  if (g_active || g_restart_requested) {
    return true;
  }
  /* 首次启动不知道线上的帧位置。先只等一段帧间空闲，不启动 DMA，避免把
   * 未对齐的 18 字节块当成输入；确认边界后再从下一帧的第一个字节开始计数。 */
  taskENTER_CRITICAL();
  __HAL_UART_CLEAR_IDLEFLAG(&huart3);
  CLEAR_BIT(huart3.Instance->CR3, USART_CR3_DMAR | USART_CR3_EIE);
  SET_BIT(huart3.Instance->CR1, USART_CR1_IDLEIE | USART_CR1_PEIE);
  g_synchronized = false;
  g_boundary_seen = false;
  g_boundary_wait_armed = true;
  const bool started = true;
  taskEXIT_CRITICAL();
  return started;
}

/**
 * @brief USART3 专用 IRQ 钩子。只记录故障或 IDLE 边界，不进入 HAL 单缓冲接收状态机。
 * @param None。
 * @retval None。错误只请求任务恢复。
 * @note 仅 USART3 ISR 调用，优先级须为 5 或更低，不允许打印或普通 RTOS API。
 */
void Dbus_USART3_IRQHandler(void) {
  if (huart3.Instance == NULL || huart3.hdmarx == NULL ||
      huart3.hdmarx->Instance == NULL) {
    return;
  }
  const uint32_t status = huart3.Instance->SR;
  const uint32_t errors = status & (USART_SR_PE | USART_SR_FE | USART_SR_NE | USART_SR_ORE);
  const bool idle = (status & USART_SR_IDLE) != 0U;
  /* 同一次 SR→DR 读取统一清除错误和 IDLE。错误优先，不能把带错误的一帧发布为正常输入。 */
  if (errors != 0U || idle) {
    const volatile uint32_t discarded = huart3.Instance->DR;
    (void)discarded;
  }
  if (errors != 0U) {
    ++g_uart_errors;
    Dbus_RequestRecovery();
  }
  if (idle) {
    const uint32_t remaining = __HAL_DMA_GET_COUNTER(huart3.hdmarx);
    if (!g_active && !g_restart_requested && g_boundary_wait_armed && errors == 0U) {
      /* 启动同步窗口只消费 IDLE 边界，不读取任何半帧数据；任务随后从下一帧启动 DMA。 */
      g_boundary_seen = true;
    } else if (g_restart_requested) {
      g_boundary_seen = true;
    } else if (!g_synchronized || (remaining > 0U && remaining < DBUS_FRAME_LENGTH)) {
      ++g_resync_count;
      Dbus_RequestRecovery();
      g_boundary_seen = true;
    }
    /* NDTR=18 是刚切换的空块，0 是刚完成但 TC 可能未处理。两种都不能当成半帧丢弃。 */
  }
}

/**
 * @brief 非阻塞排空当前邮箱并累计合法帧。ISR 不做解码、打印或等待。
 * @param now_ms HAL_GetTick 的当前 ms，恢复重试使用同一时基。
 * @retval None 更新内部数据与接收状态。
 * @note 只由 DBUS 任务调用。HAL_DMA_Abort 的有界等待只发生在任务中。
 */
void Dbus_Process(uint32_t now_ms) {
  if (huart3.Instance == NULL || huart3.hdmarx == NULL ||
      huart3.hdmarx->Instance == NULL) {
    return;
  }
  if (!g_active && g_boundary_seen &&
      (!g_retry_wait || now_ms - g_last_retry_ms >= DBUS_RETRY_PERIOD_MS)) {
    /* 先关请求，任务有界 Abort 把 HAL 句柄解锁，再短临界区重启。不能在关中断期间等待 stream 停止。 */
    const HAL_StatusTypeDef stopped = huart3.hdmarx->State == HAL_DMA_STATE_READY
        ? HAL_OK : HAL_DMA_Abort(huart3.hdmarx);
    taskENTER_CRITICAL();
    const bool started = stopped == HAL_OK && Dbus_Start(g_boundary_seen);
    if (stopped != HAL_OK) {
      ++g_snapshot.start_errors;
    }
    taskEXIT_CRITICAL();
    g_retry_wait = !started;
    g_last_retry_ms = now_ms;
  }
  /* 一次排空当前邮箱。鼠标是相对位移，必须累计所有合法帧，不能只保留最后一帧。
   * 快照时间使用最后一帧时间戳，避免积压帧加入后立刻按旧年龄进入回中。 */
  for (;;) {
    Dbus_FrameTypeDef frame;
    bool frame_available = false;
    taskENTER_CRITICAL();
    if (g_frame_mailbox_tail != g_frame_mailbox_head) {
      frame = g_frame_mailbox[g_frame_mailbox_tail];
      g_frame_mailbox_tail = (uint8_t)((g_frame_mailbox_tail + 1U) %
                                        DBUS_FRAME_MAILBOX_LENGTH);
      frame_available = true;
    }
    taskEXIT_CRITICAL();
    if (!frame_available) {
      break;
    }
    Dbus_DataTypeDef decoded;
    const bool valid = Dbus_DecodeFrame(frame.bytes, &decoded);
    taskENTER_CRITICAL();
    /* 解码期间 ISR 也可能故障。提交前再核对代数，旧帧不能重新宣告在线。 */
    if (frame.epoch == g_epoch && g_active && g_synchronized && !g_restart_requested) {
      if (valid) {
        g_snapshot.data = decoded;
        g_snapshot.timestamp_ms = frame.timestamp_ms;
        g_snapshot.valid = true;
        ++g_snapshot.valid_frames;
        g_snapshot.mouse_delta_x += decoded.mouse_x;
        g_snapshot.mouse_delta_y += decoded.mouse_y;
        ++g_snapshot.mouse_delta_frames;
        g_snapshot_epoch = frame.epoch;
      } else {
        ++g_snapshot.invalid_frames;
      }
    }
    taskEXIT_CRITICAL();
  }
}

/**
 * @brief 获取一份完整数据与接收状态。短临界区保证字段来自同一状态。
 * @param now_ms HAL 毫秒时间，不能传入 FreeRTOS Tick。
 * @param snapshot 输出快照，离线时 online=false。
 * @retval true 已复制。false 指针为空，输出不变。
 * @note 仅任务上下文。不返回内部缓冲区指针，不阻塞等待新帧。
 */
bool Dbus_GetSnapshot(uint32_t now_ms, Dbus_SnapshotTypeDef *snapshot) {
  if (snapshot == NULL) {
    return false;
  }
  taskENTER_CRITICAL();
  *snapshot = g_snapshot;
  snapshot->resync_count = g_resync_count;
  snapshot->uart_errors = g_uart_errors;
  snapshot->dma_errors = g_dma_errors;
  snapshot->age_ms = snapshot->valid ? now_ms - snapshot->timestamp_ms : 0U;
  /* ISR 可能在任务取 now_ms 之后才收完帧。略新的时间戳按 0 ms，不能让无符号下溢制造假掉线。 */
  if (snapshot->age_ms > INT32_MAX) {
    snapshot->age_ms = 0U;
  }
  snapshot->online = snapshot->valid && g_snapshot_epoch == g_epoch &&
      g_active && g_synchronized && !g_restart_requested &&
      snapshot->age_ms < DBUS_OFFLINE_TIMEOUT_MS;
  /* 鼠标量属于当前已解码帧。复制后清零，重复任务周期不会重复消费。 */
  g_snapshot.mouse_delta_x = 0;
  g_snapshot.mouse_delta_y = 0;
  g_snapshot.mouse_delta_frames = 0U;
  snapshot->frame_overruns = g_frame_mailbox_overruns;
  taskEXIT_CRITICAL();
  return true;
}
