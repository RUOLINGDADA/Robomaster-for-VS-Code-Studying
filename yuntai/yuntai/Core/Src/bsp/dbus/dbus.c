/**
 * @file dbus.c
 * @brief DBUS 硬件双缓冲、帧边界恢复与任务解码（DMA 连续接收，CPU 不逐字节搬运）。
 *
 * M0/M1 各 18 字节；完成 ISR 立即复制到固定单帧邮箱，任务只接触副本。
 * 接收代数 epoch 隔离恢复前后的帧（重启后即使队列有旧帧，也不能把它当成新连接）。
 */
#include "bsp/dbus/dbus.h"
#include "FreeRTOS.h"
#include "task.h"
#include "usart.h"
#include <stddef.h>
#include <string.h>

typedef struct {
  uint8_t bytes[DBUS_FRAME_LENGTH]; /* 已完成 DMA 块的副本（任务不持有 DMA 内存）。 */
  uint32_t timestamp_ms; /* ISR 完整帧时刻，单位 HAL ms（排队时间不续命）。 */
  uint32_t epoch; /* 入队时接收代数（失同步会改变，旧帧不能复活输入）。 */
} Dbus_FrameTypeDef;

static uint8_t g_rx_buffers[2][DBUS_FRAME_LENGTH]; /* DMA 专有的两块内存（不能拿它们直接给任务解码）。 */
static Dbus_FrameTypeDef g_pending_frame; /* 固定单帧邮箱；DBUS 只需要最新状态，不需要 ISR 动态队列。 */
static volatile bool g_pending_frame_valid; /* ISR 写入完整帧后置位，任务取走后清零。 */
static Dbus_SnapshotTypeDef g_snapshot; /* 任务写入、任务快照读取，均在短临界区复制。 */
static uint32_t g_snapshot_epoch; /* 最近合法帧所属代数（与 ISR 的 epoch 比较）。 */
static volatile uint32_t g_epoch; /* ISR 故障/失同步时递增（不是数据一致性保护的替代品）。 */
static volatile uint32_t g_resync_count; /* ISR 记录边界重新建立次数。 */
static volatile uint32_t g_uart_errors; /* ISR 记录 UART 故障次数。 */
static volatile uint32_t g_dma_errors; /* ISR 记录 DMA 故障次数。 */
static volatile bool g_restart_requested; /* ISR 提交恢复请求（等待工作留给任务）。 */
static volatile bool g_boundary_seen; /* 已看到帧间 IDLE（不能把半帧重启点当帧头）。 */
static volatile bool g_synchronized; /* 收帧从已确认边界开始（启动前 false）。 */
static volatile bool g_active; /* DMA 启动成功标志（不代表已有合法帧）。 */
static uint32_t g_last_retry_ms; /* 任务恢复重试时刻，单位 HAL ms。 */
static bool g_retry_wait; /* 启动失败后的限频标志（时间戳 0 也有效）。 */

/* 拼接小端 16 位字段；逐字节移位不依赖 STM32 内存布局（不能把任意字节指针强转成 int16_t*）。 */
static uint16_t Dbus_ReadU16(const uint8_t *bytes) {
  return (uint16_t)((uint16_t)bytes[0] | ((uint16_t)bytes[1] << 8));
}

bool Dbus_DecodeFrame(const uint8_t *b, Dbus_DataTypeDef *data) {
  if (b == NULL || data == NULL) {
    return false;
  }
  Dbus_DataTypeDef decoded = {0};
  /* 四路 11 位通道跨字节排列；必须保留每字节 bit7（USART 7 位接收会在这里悄悄破坏摇杆值）。 */
  decoded.channels[0] = (int16_t)(((b[0] | (b[1] << 8)) & 0x07FF) - DBUS_CHANNEL_CENTER);
  decoded.channels[1] = (int16_t)((((b[1] >> 3) | (b[2] << 5)) & 0x07FF) - DBUS_CHANNEL_CENTER);
  decoded.channels[2] = (int16_t)((((b[2] >> 6) | (b[3] << 2) | (b[4] << 10)) & 0x07FF) - DBUS_CHANNEL_CENTER);
  decoded.channels[3] = (int16_t)((((b[4] >> 1) | (b[5] << 7)) & 0x07FF) - DBUS_CHANNEL_CENTER);
  /* CH4 也是 11 位字段；不屏蔽高 5 位会把保留位带进滚轮值，进而溢出或误判帧。 */
  decoded.channels[4] = (int16_t)((Dbus_ReadU16(&b[16]) & 0x07FFU) -
                                  DBUS_CHANNEL_CENTER);
  /*
   * 参考工程的定义是 switches[0]=S1 左拨杆（高两位）、
   * switches[1]=S2 右拨杆（低两位）；先取出连续的四位再按该定义拆分。
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
  /* 协议没有帧头和 CRC，范围校验只能筛掉明显错位（坏帧不能覆盖最后合法输入或刷新在线时间）。 */
  /* 仅四路摇杆使用 364~1684 的官方范围；滚轮 CH4 已按 11 位掩码，允许其独立物理范围。 */
  for (uint8_t i = 0U; i < 4U; ++i) {
    if (decoded.channels[i] < -DBUS_CHANNEL_SPAN || decoded.channels[i] > DBUS_CHANNEL_SPAN) {
      return false;
    }
  }
  /*
   * 拨杆/鼠标不是云台输入的有效性门槛。某些接收器在拨杆切换或链路
   * 恢复的瞬间会给出保留值 0；若这里拒绝整帧，右摇杆也会同时失效。
   * 通道范围仍保留，用来过滤明显错位的 18 字节块；鼠标按键仍只接受
   * 协议定义的 0/1，避免把明显错位数据误当作有效输入。
   */
  if (decoded.mouse_left > 1U || decoded.mouse_right > 1U) {
    return false;
  }
  *data = decoded;
  return true;
}

/* 立刻停住接收并使旧代数失效；只写寄存器，不能在 ISR 等待 HAL Abort（低优先级 HAL Tick 无法推进会卡住）。 */
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
  g_pending_frame_valid = false;
  g_restart_requested = true;
  ++g_epoch;
}

/* 复制刚完成的块；HAL 根据 CT 已切到另一块来选择 M0/M1 回调（不能复制当前正在写入的块）。 */
static void Dbus_CopyCompleted(uint8_t index) {
  if (!g_active || !g_synchronized || g_restart_requested ||
      index >= 2U) {
    return;
  }
  /* DMA 已切换到另一块后才复制当前完成块；不调用 FreeRTOS API，避免
   * DMA/USART 中断路径受到队列句柄和优先级配置影响。 */
  memcpy(g_pending_frame.bytes, g_rx_buffers[index], DBUS_FRAME_LENGTH);
  g_pending_frame.timestamp_ms = HAL_GetTick();
  g_pending_frame.epoch = g_epoch;
  __DMB();
  g_pending_frame_valid = true;
}

/* HAL 的 M0 完成回调只复制 M0（DMA 已改写 M1）。 */
static void Dbus_DmaM0Complete(DMA_HandleTypeDef *hdma) {
  if (hdma == huart3.hdmarx) {
    Dbus_CopyCompleted(0U);
  }
}

/* HAL 的 M1 完成回调只复制 M1（与 M0 共享同一入队规则）。 */
static void Dbus_DmaM1Complete(DMA_HandleTypeDef *hdma) {
  if (hdma == huart3.hdmarx) {
    Dbus_CopyCompleted(1U);
  }
}

/* DMA 错误只记录并请求恢复（ISR 不 Abort、不打印、不进入 Error_Handler）。 */
static void Dbus_DmaError(DMA_HandleTypeDef *hdma) {
  if (hdma == huart3.hdmarx) {
    ++g_dma_errors;
    Dbus_RequestRecovery();
  }
}

/* DMA 已停稳后一次配置双缓冲；调用方短临界区屏蔽优先级 5 IRQ（不能让中断看到半初始化状态）。 */
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
  __HAL_UART_CLEAR_IDLEFLAG(&huart3); /* SR→DR 清 IDLE 与接收错误（此时 DMA 未运行，不能与 DMA 抢读 DR）。 */
  if (HAL_DMAEx_MultiBufferStart_IT(hdma, (uint32_t)&huart3.Instance->DR,
      (uint32_t)g_rx_buffers[0], (uint32_t)g_rx_buffers[1], DBUS_FRAME_LENGTH) != HAL_OK) {
    ++g_snapshot.start_errors;
    return false;
  }
  g_synchronized = synchronized;
  g_restart_requested = false;
  g_boundary_seen = false;
  g_active = true;
  SET_BIT(huart3.Instance->CR3, USART_CR3_DMAR | USART_CR3_EIE);
  SET_BIT(huart3.Instance->CR1, USART_CR1_IDLEIE | USART_CR1_PEIE);
  return true;
}

bool Dbus_Init(void) {
  if (huart3.Instance == NULL || huart3.hdmarx == NULL ||
      huart3.hdmarx->Instance == NULL) {
    return false;
  }
  if (g_active || g_restart_requested) {
    return true;
  }
  /* 首次启动不知道线上的帧位置，先收但不发布，第一次 IDLE 后重新建立起点（任意 18 字节不一定是一帧）。 */
  taskENTER_CRITICAL();
  const bool started = Dbus_Start(false);
  taskEXIT_CRITICAL();
  return started;
}

void Dbus_USART3_IRQHandler(void) {
  if (huart3.Instance == NULL || huart3.hdmarx == NULL ||
      huart3.hdmarx->Instance == NULL) {
    return;
  }
  const uint32_t status = huart3.Instance->SR;
  const uint32_t errors = status & (USART_SR_PE | USART_SR_FE | USART_SR_NE | USART_SR_ORE);
  const bool idle = (status & USART_SR_IDLE) != 0U;
  /* 同一次 SR→DR 读取统一清除错误和 IDLE；错误优先，不能把带错误的一帧发布为正常输入。 */
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
    if (g_restart_requested) {
      g_boundary_seen = true;
    } else if (!g_synchronized || (remaining > 0U && remaining < DBUS_FRAME_LENGTH)) {
      ++g_resync_count;
      Dbus_RequestRecovery();
      g_boundary_seen = true;
    }
    /* NDTR=18 是刚切换的空块，0 是刚完成但 TC 可能未处理（两种都不能当成半帧丢弃）。 */
  }
}

void Dbus_Process(uint32_t now_ms) {
  if (huart3.Instance == NULL || huart3.hdmarx == NULL ||
      huart3.hdmarx->Instance == NULL) {
    return;
  }
  if ((!g_active && (!g_restart_requested || g_boundary_seen)) &&
      (!g_retry_wait || now_ms - g_last_retry_ms >= DBUS_RETRY_PERIOD_MS)) {
    /* 先关请求，任务有界 Abort 把 HAL 句柄解锁，再短临界区重启（不能在关中断期间等待 stream 停止）。 */
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
  Dbus_FrameTypeDef frame;
  bool frame_available = false;
  taskENTER_CRITICAL();
  if (g_pending_frame_valid) {
    frame = g_pending_frame;
    g_pending_frame_valid = false;
    frame_available = true;
  }
  taskEXIT_CRITICAL();
  if (frame_available) {
    Dbus_DataTypeDef decoded;
    const bool valid = Dbus_DecodeFrame(frame.bytes, &decoded);
    taskENTER_CRITICAL();
    /* 解码期间 ISR 也可能故障；提交前再核对代数（不能用已断开的旧帧重新宣告在线）。 */
    if (frame.epoch == g_epoch && g_active && g_synchronized && !g_restart_requested) {
      if (valid) {
        g_snapshot.data = decoded;
        g_snapshot.timestamp_ms = frame.timestamp_ms;
        g_snapshot.valid = true;
        ++g_snapshot.valid_frames;
        g_snapshot_epoch = frame.epoch;
      } else {
        ++g_snapshot.invalid_frames;
      }
    }
    taskEXIT_CRITICAL();
  }
}

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
  /* ISR 可能在任务取 now_ms 之后才收完帧；略新的时间戳按 0 ms，不能让无符号下溢制造假掉线。 */
  if (snapshot->age_ms > INT32_MAX) {
    snapshot->age_ms = 0U;
  }
  snapshot->online = snapshot->valid && g_snapshot_epoch == g_epoch &&
      g_active && g_synchronized && !g_restart_requested &&
      snapshot->age_ms < DBUS_OFFLINE_TIMEOUT_MS;
  taskEXIT_CRITICAL();
  return true;
}
