/**
 * @file m3508_c620.c
 * @brief C620/M3508 的协议解析、注册、一致快照和电流发送。
 *
 * 从 MyChassis1 的帧布局和电流槽位迁移；按当前工程收紧使能及发送新鲜度检查。
 * ISR 只写反馈。任务短临界区复制及提交，不做轮序、PID 或自动任务创建。
 */
#include "bsp/m3508_c620/m3508_c620.h"
#include <stddef.h>
#include <string.h>

static C620_M3508_HandleTypeDef *g_motors[C620_M3508_MAX_DEVICES]; /* 静态指针表；任务改表时屏蔽 ISR。 */

/* 调用者已持有临界区，按总线和 ID 查找，不能把 CAN2 的同 ID 当底盘。 */
static C620_M3508_HandleTypeDef *C620_M3508_Find(CAN_HandleTypeDef *hcan, uint8_t id) {
  for (uint32_t i = 0; i < C620_M3508_MAX_DEVICES; ++i) {
    if (g_motors[i] != NULL && g_motors[i]->config.hcan == hcan && g_motors[i]->config.motor_id == id) {
      return g_motors[i];
    }
  }
  return NULL;
}

/* 不能只信 initialized，防止传入伪造或已注销的地址；在临界区内调用。 */
static bool C620_M3508_Registered(const C620_M3508_HandleTypeDef *motor) {
  for (uint32_t i = 0; i < C620_M3508_MAX_DEVICES; ++i) {
    if (motor != NULL && g_motors[i] == motor) { return true; }
  }
  return false;
}

/* now 在复制反馈后取值；无符号差支持 HAL ms 回绕，时间戳 0 不代表没反馈。 */
static bool C620_M3508_Fresh(const C620_M3508_HandleTypeDef *motor, uint32_t now_ms) {
  return motor->feedback.received && now_ms - motor->feedback.timestamp_ms <= motor->config.feedback_timeout_ms;
}

/* 接收数据为大端，逐字节拼接避免 STM32 小端和非对齐读取问题。 */
static uint16_t C620_M3508_ReadBe(const uint8_t *data) {
  return (uint16_t)(((uint16_t)data[0] << 8U) | data[1]);
}

/* 转到 int32_t 再减去 65536，避免超范围 uint16_t 转 int16_t 的实现差异。 */
static int16_t C620_M3508_ReadSignedBe(const uint8_t *data) {
  const uint16_t raw = C620_M3508_ReadBe(data);
  return (int16_t)(raw <= INT16_MAX ? (int32_t)raw : (int32_t)raw - 65536);
}

/* 参数检查和空槽查找先完成，再改句柄；失败不会留下半注册对象。 */
bool C620_M3508_Init(C620_M3508_HandleTypeDef *motor, const C620_M3508_ConfigTypeDef *config) {
  if (motor == NULL || config == NULL || config->hcan == NULL || config->motor_id < 1U ||
      config->motor_id > 8U || config->feedback_timeout_ms == 0U || config->feedback_timeout_ms > INT32_MAX) {
    return false;
  }
  const uint32_t mask = __get_PRIMASK();
  __disable_irq();
  bool ok = false;
  if (!C620_M3508_Registered(motor) && C620_M3508_Find(config->hcan, config->motor_id) == NULL) {
    for (uint32_t i = 0; i < C620_M3508_MAX_DEVICES; ++i) {
      if (g_motors[i] == NULL) {
        *motor = (C620_M3508_HandleTypeDef){.config = *config, .initialized = true};
        g_motors[i] = motor;
        ok = true;
        break;
      }
    }
  }
  __set_PRIMASK(mask);
  return ok;
}

/* 清表与清句柄必须对 ISR 原子化，否则 ISR 可能访问被清空的配置。 */
void C620_M3508_DeInit(C620_M3508_HandleTypeDef *motor) {
  const uint32_t mask = __get_PRIMASK();
  __disable_irq();
  for (uint32_t i = 0; i < C620_M3508_MAX_DEVICES; ++i) {
    if (motor != NULL && g_motors[i] == motor) {
      g_motors[i] = NULL;
      memset(motor, 0, sizeof(*motor));
      break;
    }
  }
  __set_PRIMASK(mask);
}

/* 所有反馈字段只复制一次；online 和 age 基于这份副本，不再拼另一帧。 */
bool C620_M3508_GetSnapshot(const C620_M3508_HandleTypeDef *motor, C620_M3508_SnapshotTypeDef *snapshot) {
  if (snapshot == NULL) { return false; }
  const uint32_t mask = __get_PRIMASK();
  __disable_irq();
  const bool ok = C620_M3508_Registered(motor);
  if (ok) {
    *snapshot = motor->feedback;
    const uint32_t now_ms = HAL_GetTick();
    snapshot->age_ms = now_ms - snapshot->timestamp_ms;
    snapshot->online = C620_M3508_Fresh(motor, now_ms);
  }
  __set_PRIMASK(mask);
  return ok;
}

/* 使能不能伪造反馈在线；拒绝使能也清旧电流，避免恢复瞬间直接复用旧值。 */
bool C620_M3508_SetOutputEnabled(C620_M3508_HandleTypeDef *motor, bool enabled) {
  const uint32_t mask = __get_PRIMASK();
  __disable_irq();
  bool ok = C620_M3508_Registered(motor);
  if (ok) {
    motor->output_enabled = enabled && C620_M3508_Fresh(motor, HAL_GetTick());
    if (!motor->output_enabled) { motor->target_current_raw = 0; }
    ok = !enabled || motor->output_enabled;
  }
  __set_PRIMASK(mask);
  return ok;
}

/* 指令只存 RAM；协议边界与任务实际限流分开，C610 的 ±10000 不能用于 C620。 */
bool C620_M3508_SetCurrent(C620_M3508_HandleTypeDef *motor, int16_t current_raw) {
  const uint32_t mask = __get_PRIMASK();
  __disable_irq();
  const bool ok = C620_M3508_Registered(motor);
  if (ok) {
    if (current_raw > C620_M3508_CURRENT_LIMIT_RAW) { current_raw = C620_M3508_CURRENT_LIMIT_RAW; }
    if (current_raw < -C620_M3508_CURRENT_LIMIT_RAW) { current_raw = -C620_M3508_CURRENT_LIMIT_RAW; }
    motor->target_current_raw = current_raw;
  }
  __set_PRIMASK(mask);
  return ok;
}

/* Bus-Off 期间每周期清 received，恢复时只有新帧才能打开该轮安全门。 */
void C620_M3508_InvalidateFeedback(C620_M3508_HandleTypeDef *motor) {
  const uint32_t mask = __get_PRIMASK();
  __disable_irq();
  if (C620_M3508_Registered(motor)) {
    motor->feedback.received = false;
    motor->output_enabled = false;
    motor->target_current_raw = 0;
  }
  __set_PRIMASK(mask);
}

/* ISR 不再读 FIFO，不改变输出许可；合法反馈只证明通信，不等于允许运动。 */
bool C620_M3508_HandleRxMessage(CAN_HandleTypeDef *hcan, const CAN_RxHeaderTypeDef *header, const uint8_t *data) {
  if (hcan == NULL || header == NULL || data == NULL || header->IDE != CAN_ID_STD ||
      header->RTR != CAN_RTR_DATA || header->DLC != C620_M3508_FRAME_BYTES ||
      header->StdId <= C620_M3508_FEEDBACK_BASE_ID || header->StdId > C620_M3508_FEEDBACK_BASE_ID + 8U ||
      C620_M3508_ReadBe(data) > 8191U) { return false; }
  C620_M3508_HandleTypeDef *motor = C620_M3508_Find(hcan, (uint8_t)(header->StdId - C620_M3508_FEEDBACK_BASE_ID));
  if (motor == NULL) { return false; }
  motor->feedback.angle_raw = C620_M3508_ReadBe(data);
  motor->feedback.speed_rpm = C620_M3508_ReadSignedBe(&data[2]);
  motor->feedback.current_raw = C620_M3508_ReadSignedBe(&data[4]);
  motor->feedback.temperature_c = data[6];
  motor->feedback.status_raw = data[7];
  motor->feedback.timestamp_ms = HAL_GetTick();
  motor->feedback.received = true;
  return true;
}

/* 临界区内只提交一次，不等待邮箱；载荷始终是标准 8 字节大端电流。 */
static bool C620_M3508_SendFrame(CAN_HandleTypeDef *hcan, uint32_t id, uint8_t *data) {
  if (HAL_CAN_GetTxMailboxesFreeLevel(hcan) == 0U) { return false; }
  CAN_TxHeaderTypeDef header = {.StdId = id, .IDE = CAN_ID_STD, .RTR = CAN_RTR_DATA,
                              .DLC = C620_M3508_FRAME_BYTES, .TransmitGlobalTime = DISABLE};
  uint32_t mailbox;
  return HAL_CAN_AddTxMessage(hcan, &header, data, &mailbox) == HAL_OK;
}

/* 从读取所有目标槽位到提交属于同一事务，否则切换任务可能提交已经过期的旧帧。 */
bool C620_M3508_SendAll(CAN_HandleTypeDef *hcan) {
  if (hcan == NULL || hcan->Instance == NULL) { return false; }
  const uint32_t mask = __get_PRIMASK();
  __disable_irq();
  uint8_t data[2][C620_M3508_FRAME_BYTES] = {{0}}; /* 低组和高组负载；未注册槽位必须为零。 */
  bool used[2] = {false, false};
  const uint32_t now_ms = HAL_GetTick();
  const bool bus_ok = (hcan->Instance->ESR & CAN_ESR_BOFF) == 0U;
  for (uint32_t i = 0; i < C620_M3508_MAX_DEVICES; ++i) {
    C620_M3508_HandleTypeDef *motor = g_motors[i];
    if (motor == NULL || motor->config.hcan != hcan) { continue; }
    if (!bus_ok || !C620_M3508_Fresh(motor, now_ms)) {
      motor->output_enabled = false;
      motor->target_current_raw = 0;
    }
    const uint32_t group = (motor->config.motor_id - 1U) / 4U;
    const uint32_t offset = ((motor->config.motor_id - 1U) % 4U) * 2U;
    const uint16_t raw = (uint16_t)(motor->output_enabled ? motor->target_current_raw : 0);
    data[group][offset] = (uint8_t)(raw >> 8U);
    data[group][offset + 1U] = (uint8_t)raw;
    used[group] = true;
  }
  const bool sent = bus_ok && (!used[0] || C620_M3508_SendFrame(hcan, C620_M3508_CONTROL_LOW_ID, data[0])) &&
                    (!used[1] || C620_M3508_SendFrame(hcan, C620_M3508_CONTROL_HIGH_ID, data[1]));
  __set_PRIMASK(mask);
  return sent;
}
