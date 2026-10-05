/**
  ******************************************************************************
  * @file    c610_m2006.c
  * @brief   C610/M2006 RoboMaster CAN 电机驱动实现。
  *
  * 本文件使用固定大小的静态注册表，避免电机接收中断或控制任务调用
  * malloc。接收路径只做帧过滤、字段解析和时间戳更新；发送路径把同一
  * CAN 总线上的电流值聚合到 0x200/0x1FF 控制帧中。
  * 通俗理解：先收反馈确认电机在线，再把多个电机的目标电流放进对应控制帧槽位。
  ******************************************************************************
  */

#include "bsp/c610_m2006/c610_m2006.h"

#include <string.h>

#if !defined(HAL_CAN_MODULE_ENABLED)

/*
 * CubeMX 尚未启用 CAN 时保留接口而不引用 HAL CAN 字段和函数，保证工程
 * 可以先完成其它模块构建（此时所有驱动操作安全返回 false）。启用 HAL_CAN_MODULE_ENABLED 后自动编译下方完整实现。
 */
bool C610_M2006_Init(C610_M2006_HandleTypeDef *hmotor,
                     const C610_M2006_ConfigTypeDef *config) {
  (void)hmotor;
  (void)config;
  return false;
}

bool C610_M2006_DeInit(C610_M2006_HandleTypeDef *hmotor) {
  (void)hmotor;
  return false;
}

bool C610_M2006_SetCurrent(C610_M2006_HandleTypeDef *hmotor,
                           int16_t current_raw) {
  (void)hmotor;
  (void)current_raw;
  return false;
}

bool C610_M2006_SetOutputEnabled(C610_M2006_HandleTypeDef *hmotor,
                                 bool enabled) {
  (void)hmotor;
  (void)enabled;
  return false;
}

bool C610_M2006_HandleRxMessage(
    CAN_HandleTypeDef *hcan, const CAN_RxHeaderTypeDef *rx_header,
    const uint8_t data[C610_M2006_FRAME_DLC]) {
  (void)hcan;
  (void)rx_header;
  (void)data;
  return false;
}

bool C610_M2006_RxFifoCallback(CAN_HandleTypeDef *hcan, uint32_t rx_fifo) {
  (void)hcan;
  (void)rx_fifo;
  return false;
}

bool C610_M2006_SendAll(CAN_HandleTypeDef *hcan) {
  (void)hcan;
  return false;
}

bool C610_M2006_Process(C610_M2006_HandleTypeDef *hmotor,
                        uint32_t now_tick) {
  (void)hmotor;
  (void)now_tick;
  return false;
}

bool C610_M2006_GetFeedback(const C610_M2006_HandleTypeDef *hmotor,
                            C610_M2006_FeedbackTypeDef *feedback) {
  (void)hmotor;
  (void)feedback;
  return false;
}

bool C610_M2006_IsOnline(const C610_M2006_HandleTypeDef *hmotor) {
  (void)hmotor;
  return false;
}

#else

typedef struct {
  C610_M2006_HandleTypeDef *handle; /* 静态注册表中对应的设备句柄（每个槽位只放一个设备）。 */
  bool in_use;                      /* true 表示该槽位已被一个句柄占用（避免重复注册）。 */
} C610_M2006_SlotTypeDef;

static C610_M2006_SlotTypeDef
    g_c610_m2006_slots[C610_M2006_MAX_DEVICE_COUNT];

/* 聚合发送期间屏蔽 CAN ISR，保证所有槽位来自同一份状态并一起提交。 */
static uint32_t C610_M2006_EnterCritical(void) {
  const uint32_t previous = __get_PRIMASK();
  __disable_irq();
  __DMB();
  return previous;
}

/* 恢复调用者原来的中断状态；不能无条件开中断破坏外层临界区。 */
static void C610_M2006_ExitCritical(uint32_t previous) {
  __DMB();
  __set_PRIMASK(previous);
}

/* 发送安全门直接检查反馈新鲜度，避免漏调 Process 时继续发旧电流。 */
static bool C610_M2006_FeedbackFresh(const C610_M2006_HandleTypeDef *hmotor,
                                     uint32_t now_ms) {
  if (hmotor == NULL || !hmotor->feedback_received) {
    return false;
  }
  const uint32_t age_ms = now_ms - hmotor->feedback.last_feedback_tick;
  return age_ms > INT32_MAX || age_ms < hmotor->config.feedback_timeout_ms;
}

/* 检查 M2006/C610 反馈 ID 是否落在 1~8；越界 ID 不能映射到控制帧槽位。 */
static bool C610_M2006_IsValidId(uint8_t motor_id) {
  return motor_id >= C610_M2006_MIN_DEVICE_ID &&
         motor_id <= C610_M2006_MAX_DEVICE_ID;
}

/* 判断句柄是否仍在静态表中，防止未初始化对象参与 CAN 聚合。 */
static bool C610_M2006_IsRegistered(
    const C610_M2006_HandleTypeDef *hmotor) {
  for (uint8_t i = 0U; i < C610_M2006_MAX_DEVICE_COUNT; i++) {
    if (g_c610_m2006_slots[i].in_use &&
        g_c610_m2006_slots[i].handle == hmotor) {
      return true;
    }
  }
  return false;
}

/* 按 CAN 实例和电机 ID 找到唯一句柄，避免同一总线的反馈串轴。 */
static C610_M2006_HandleTypeDef *C610_M2006_Find(
    CAN_HandleTypeDef *hcan, uint8_t motor_id) {
  for (uint8_t i = 0U; i < C610_M2006_MAX_DEVICE_COUNT; i++) {
    C610_M2006_HandleTypeDef *hmotor = g_c610_m2006_slots[i].handle;
    if (g_c610_m2006_slots[i].in_use && hmotor != NULL &&
        hmotor->config.hcan == hcan && hmotor->config.motor_id == motor_id) {
      return hmotor;
    }
  }
  return NULL;
}

/* 解析大端有符号字段；直接指针读取会因 STM32 小端布局交换 DATA[0]/DATA[1]。 */
static int16_t C610_M2006_ReadI16Be(const uint8_t data[2]) {
  /* 电调协议先发送高字节；不能把 data 地址强转成 int16_t*（STM32 小端会读反字节）。 */
  uint16_t raw = ((uint16_t)data[0] << 8U) | data[1];
  return (int16_t)raw;
}

/* 解析大端无符号编码器字段，供诊断或范围检查使用。 */
static uint16_t C610_M2006_ReadU16Be(const uint8_t data[2]) {
  return ((uint16_t)data[0] << 8U) | data[1];
}

/* 将电流补码按大端放入一个两字节槽位，保持与 C610 控制帧协议一致。 */
static void C610_M2006_WriteI16Be(uint8_t data[2], int16_t value) {
  const uint16_t raw = (uint16_t)value;
  data[0] = (uint8_t)(raw >> 8U); /* CAN DATA[0] 放高字节（DATA[1] 放低字节）。 */
  data[1] = (uint8_t)raw;
}

/* 将 ID 1~4/5~8 映射到各自帧内的 0~3 槽位；两组不能直接用 motor_id 做数组下标。 */
static uint8_t C610_M2006_ControlSlot(uint8_t motor_id) {
  return motor_id <= 4U ? (uint8_t)(motor_id - 1U)
                         : (uint8_t)(motor_id - 5U);
}

/* 为发送入口找到该 CAN 总线上的任一已注册句柄，以取得 HAL 句柄。 */
static C610_M2006_HandleTypeDef *C610_M2006_FindFirstOnBus(
    CAN_HandleTypeDef *hcan) {
  for (uint8_t i = 0U; i < C610_M2006_MAX_DEVICE_COUNT; i++) {
    C610_M2006_HandleTypeDef *hmotor = g_c610_m2006_slots[i].handle;
    if (g_c610_m2006_slots[i].in_use && hmotor != NULL &&
        hmotor->config.hcan == hcan) {
      return hmotor;
    }
  }
  return NULL;
}

/* 提交一条 8 字节标准 CAN 控制帧；邮箱满时立即失败，不能在任务里忙等。 */
static bool C610_M2006_SendFrame(CAN_HandleTypeDef *hcan, uint32_t std_id,
                                 const uint8_t data[8]) {
  if (hcan == NULL || data == NULL ||
      HAL_CAN_GetTxMailboxesFreeLevel(hcan) == 0U) {
    return false;
  }

  CAN_TxHeaderTypeDef header = {0};
  uint32_t mailbox = 0U;
  header.StdId = std_id;
  header.IDE = CAN_ID_STD;
  header.RTR = CAN_RTR_DATA;
  header.DLC = C610_M2006_FRAME_DLC;
  return HAL_CAN_AddTxMessage(hcan, &header, (uint8_t *)data, &mailbox) ==
         HAL_OK;
}

bool C610_M2006_Init(C610_M2006_HandleTypeDef *hmotor,
                     const C610_M2006_ConfigTypeDef *config) {
  if (hmotor == NULL || config == NULL || config->hcan == NULL ||
      !C610_M2006_IsValidId(config->motor_id) ||
      C610_M2006_Find(config->hcan, config->motor_id) != NULL) {
    return false;
  }

  memset(hmotor, 0, sizeof(*hmotor));  /* 清零整个句柄（丢掉上一次使用留下的状态，重新从干净起点初始化）。 */
  /*
    如果写 hmotor->config = config，只是复制指针；外部 config 如果是局部栈变量，函数结束栈销毁，hmotor->config 变成野指针，后续访问直接 HardFault。
    hmotor->config = *config 做结构体值拷贝，句柄内部拥有一份独立副本，不受外部 config 生命周期影响（所以配置可来自局部变量）。
  */
  hmotor->config = *config;
  if (hmotor->config.feedback_timeout_ms == 0U) {
    hmotor->config.feedback_timeout_ms = C610_M2006_FEEDBACK_TIMEOUT_MS;
  }

  /* 放入静态注册表（驱动用固定槽位管理电机，不在运行时 malloc，适合 ISR/任务并发）。 */
  for (uint8_t i = 0U; i < C610_M2006_MAX_DEVICE_COUNT; i++) {
    if (!g_c610_m2006_slots[i].in_use) {
      g_c610_m2006_slots[i].handle = hmotor;
      g_c610_m2006_slots[i].in_use = true;
      hmotor->state = C610_M2006_STATE_OFFLINE;
      hmotor->initialized = true;
      return true;
    }
  }

  return false;
}

bool C610_M2006_DeInit(C610_M2006_HandleTypeDef *hmotor) {
  if (hmotor == NULL || !C610_M2006_IsRegistered(hmotor)) {
    return false;
  }

  for (uint8_t i = 0U; i < C610_M2006_MAX_DEVICE_COUNT; i++) {
    if (g_c610_m2006_slots[i].handle == hmotor) {
      g_c610_m2006_slots[i].handle = NULL;
      g_c610_m2006_slots[i].in_use = false;
      break;
    }
  }
  memset(hmotor, 0, sizeof(*hmotor));
  return true;
}

bool C610_M2006_SetCurrent(C610_M2006_HandleTypeDef *hmotor,
                           int16_t current_raw) {
  if (hmotor == NULL || !hmotor->initialized ||
      !C610_M2006_IsRegistered(hmotor)) {
    return false;
  }

  if (current_raw < C610_M2006_CURRENT_RAW_MIN) {
    current_raw = C610_M2006_CURRENT_RAW_MIN;
  }
  if (current_raw > C610_M2006_CURRENT_RAW_MAX) {
    current_raw = C610_M2006_CURRENT_RAW_MAX;
  }
  hmotor->target_current_raw = current_raw;
  return true;
}

bool C610_M2006_SetOutputEnabled(C610_M2006_HandleTypeDef *hmotor,
                                 bool enabled) {
  if (hmotor == NULL || !hmotor->initialized ||
      !C610_M2006_IsRegistered(hmotor)) {
    return false;
  }

  /* 未收到新鲜反馈时拒绝打开输出；只改目标缓存而不设安全门会让首帧前的旧命令直接上总线。 */
  if (enabled && !C610_M2006_IsOnline(hmotor)) {
    return false;
  }

  hmotor->output_enabled = enabled;
  if (!enabled) {
    hmotor->target_current_raw = 0;
    hmotor->state = C610_M2006_STATE_DISABLED;
  } else if (hmotor->state == C610_M2006_STATE_ONLINE) {
    hmotor->state = C610_M2006_STATE_RUNNING;
  }
  return true;
}

bool C610_M2006_HandleRxMessage(
    CAN_HandleTypeDef *hcan, const CAN_RxHeaderTypeDef *rx_header,
    const uint8_t data[C610_M2006_FRAME_DLC]) {
  if (hcan == NULL || rx_header == NULL || data == NULL ||
      rx_header->IDE != CAN_ID_STD || rx_header->RTR != CAN_RTR_DATA ||
      rx_header->DLC != C610_M2006_FRAME_DLC ||
      rx_header->StdId <= C610_M2006_FEEDBACK_ID_BASE ||
      rx_header->StdId > C610_M2006_FEEDBACK_ID_BASE +
                             C610_M2006_MAX_DEVICE_ID) {
    return false;
  }

  const uint8_t motor_id =
      (uint8_t)(rx_header->StdId - C610_M2006_FEEDBACK_ID_BASE);
  C610_M2006_HandleTypeDef *hmotor = C610_M2006_Find(hcan, motor_id);
  if (hmotor == NULL) {
    return false;
  }

  /*
   * 反馈字段是协议定义的 8 字节快照（角度、转速、电流均为大端有符号/无符号
   * 整数，DATA[6] 为空、DATA[7] 为错误码。接收中断只完成解码和时间戳更新，
   * 把控制决策留给任务；通俗理解：ISR 只翻译 CAN 帧，不在中断里决定电机怎么转）。
   */
  /* 手册规定角度为 13 位 0~8191；屏蔽未定义高位，避免把保留位当角度（只保留低 13 位）。 */
  hmotor->feedback_sequence++;
  __DMB();
  hmotor->feedback.angle_raw =
      C610_M2006_ReadU16Be(&data[0]) & (C610_M2006_ENCODER_COUNTS_PER_REV - 1U);
  hmotor->feedback.speed_rpm = C610_M2006_ReadI16Be(&data[2]);
  hmotor->feedback.current_raw = C610_M2006_ReadI16Be(&data[4]);
  hmotor->feedback.reserved_raw = data[6]; /* C610 手册明确 DATA[6] 为空（原样留给诊断）。 */
  hmotor->feedback.error_code = data[7];
  hmotor->feedback.last_feedback_tick = HAL_GetTick();
  hmotor->feedback_received = true;
  hmotor->state = hmotor->output_enabled ? C610_M2006_STATE_RUNNING
                                         : C610_M2006_STATE_ONLINE;
  __DMB();
  hmotor->feedback_sequence++;
  return true;
}

bool C610_M2006_RxFifoCallback(CAN_HandleTypeDef *hcan, uint32_t rx_fifo) {
  CAN_RxHeaderTypeDef rx_header = {0};
  uint8_t data[C610_M2006_FRAME_DLC] = {0};
  if (hcan == NULL || HAL_CAN_GetRxMessage(hcan, rx_fifo, &rx_header, data) !=
                          HAL_OK) {
    return false;
  }
  return C610_M2006_HandleRxMessage(hcan, &rx_header, data);
}

bool C610_M2006_SendAll(CAN_HandleTypeDef *hcan) {
  if (hcan == NULL) {
    return false;
  }
  if (C610_M2006_FindFirstOnBus(hcan) == NULL) {
    return true;
  }

  const uint32_t previous = C610_M2006_EnterCritical();
  const uint32_t now_ms = HAL_GetTick();

  uint8_t low_data[C610_M2006_FRAME_DLC] = {0}; /* 0x200 控制帧数据（电机 ID 1~4 各占两个字节）。 */
  uint8_t high_data[C610_M2006_FRAME_DLC] = {0}; /* 0x1FF 控制帧数据（电机 ID 5~8 各占两个字节）。 */
  bool low_used = false;
  bool high_used = false;

  /* 遍历这条 CAN 总线上的所有注册电机（不同总线不能混装在同一帧）。 */
  for (uint8_t i = 0U; i < C610_M2006_MAX_DEVICE_COUNT; i++) {
    C610_M2006_HandleTypeDef *hmotor = g_c610_m2006_slots[i].handle;
    if (!g_c610_m2006_slots[i].in_use || hmotor == NULL ||
        hmotor->config.hcan != hcan) {
      continue;
    }

    const uint8_t slot = C610_M2006_ControlSlot(hmotor->config.motor_id);
    const int16_t current = hmotor->output_enabled &&
                                    hmotor->state != C610_M2006_STATE_OFFLINE &&
                                    hmotor->state != C610_M2006_STATE_UNINITIALIZED &&
                                    C610_M2006_FeedbackFresh(hmotor, now_ms)
                                ? hmotor->target_current_raw
                                : (int16_t)0;
    if (hmotor->config.motor_id <= 4U) {
      /*
        slot=0：从下标 0 开始写 2 字节 → ID1 电流
        slot=1：从下标 2 开始写 2 字节 → ID2 电流
        slot=2：从下标 4 开始写 2 字节 → ID3 电流
        slot=3：从下标 6 开始写 2 字节 → ID4 电流（槽位 = ID - 1）。
      */
      C610_M2006_WriteI16Be(&low_data[2U * slot], current);
      low_used = true; /* 标记 0x200 帧已使用（至少有一个低编号电机需要发送）。 */
    } else {
      C610_M2006_WriteI16Be(&high_data[2U * slot], current);
      high_used = true; /* 标记 0x1FF 帧已使用（至少有一个高编号电机需要发送）。 */
    }
  }

  bool sent = true;
  if (low_used && !C610_M2006_SendFrame(hcan, C610_M2006_CONTROL_ID_LOW,
                                        low_data)) {
    sent = false;
  }
  if (sent && high_used &&
      !C610_M2006_SendFrame(hcan, C610_M2006_CONTROL_ID_HIGH, high_data)) {
    sent = false;
  }
  C610_M2006_ExitCritical(previous);
  return sent;
}

bool C610_M2006_Process(C610_M2006_HandleTypeDef *hmotor,
                        uint32_t now_tick) {
  if (hmotor == NULL || !hmotor->initialized ||
      !C610_M2006_IsRegistered(hmotor)) {
    return false;
  }

  /* 没有收到过反馈时，last_feedback_tick 默认为 0，不能把启动早期的小时间差误认为在线；
   * 必须先确认真实 CAN 帧到达（feedback_received 是首帧门槛）。 */
  if (!C610_M2006_FeedbackFresh(hmotor, now_tick)) {
    hmotor->state = C610_M2006_STATE_OFFLINE;
    hmotor->target_current_raw = 0;
    hmotor->output_enabled = false;
    return true;
  }
  if (hmotor->state == C610_M2006_STATE_OFFLINE) {
    hmotor->state = hmotor->output_enabled ? C610_M2006_STATE_RUNNING
                                           : C610_M2006_STATE_ONLINE;
  }
  return true;
}

bool C610_M2006_GetFeedback(const C610_M2006_HandleTypeDef *hmotor,
                            C610_M2006_FeedbackTypeDef *feedback) {
  if (hmotor == NULL || feedback == NULL || !hmotor->initialized ||
      !C610_M2006_IsRegistered(hmotor)) {
    return false;
  }
  for (uint8_t retry = 0U; retry < 4U; retry++) {
    const uint32_t sequence_before = hmotor->feedback_sequence;
    if ((sequence_before & 1U) != 0U) {
      continue;
    }
    __DMB();
    *feedback = hmotor->feedback;
    __DMB();
    const uint32_t sequence_after = hmotor->feedback_sequence;
    if (sequence_before == sequence_after &&
        (sequence_after & 1U) == 0U) {
      return true;
    }
  }
  return false;
}

bool C610_M2006_IsOnline(const C610_M2006_HandleTypeDef *hmotor) {
  if (hmotor == NULL || !hmotor->initialized || !hmotor->feedback_received ||
      hmotor->state == C610_M2006_STATE_OFFLINE ||
      hmotor->state == C610_M2006_STATE_UNINITIALIZED) {
    return false;
  }
  /* 即使调用者漏掉本周期 Process，也不能把旧反馈永久当成在线（在线判断本身使用 HAL ms）。 */
  return C610_M2006_FeedbackFresh(hmotor, HAL_GetTick());
}

#endif /* HAL_CAN_MODULE_ENABLED */
