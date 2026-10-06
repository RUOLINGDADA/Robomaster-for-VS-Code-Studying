/**
 * @file c610_m2006.c
 * @brief C610/M2006 CAN 反馈与电流接口。
 *
 * 协议来自 C610 手册：反馈 0x201~0x208，控制 0x200/0x1FF，标准帧 8 字节。
 * CAN ISR 解码大端字段并展开连续角度；所属任务缓存电流、检查 HAL ms 年龄。
 * 任务在短临界区构建并提交聚合帧。过期槽位为零，不等待总线完成。
 * 本模块不启动 CAN，不运行角度闭环，不检测卡弹。
 */

#include "bsp/c610_m2006/c610_m2006.h"

#include <string.h>

#if !defined(HAL_CAN_MODULE_ENABLED)

/**
 * @brief 保留未启用 CAN 时的移植接口。
 * @param hmotor 保留参数；未启用 CAN 时不使用。
 * @param config 保留参数；未启用 CAN 时不使用。
 * @retval false；不修改句柄、输出参数或硬件。
 * @note 仅在 HAL_CAN_MODULE_ENABLED 未定义时编译；不代表硬件已停止。
 */
bool C610_M2006_Init(C610_M2006_HandleTypeDef *hmotor,
                     const C610_M2006_ConfigTypeDef *config) {
  (void)hmotor;
  (void)config;
  return false;
}

/**
 * @brief 保留未启用 CAN 时的移植接口。
 * @param hmotor 保留参数；未启用 CAN 时不使用。
 * @retval false；不修改句柄、输出参数或硬件。
 * @note 仅在 HAL_CAN_MODULE_ENABLED 未定义时编译；不代表硬件已停止。
 */
bool C610_M2006_DeInit(C610_M2006_HandleTypeDef *hmotor) {
  (void)hmotor;
  return false;
}

/**
 * @brief 保留未启用 CAN 时的移植接口。
 * @param hmotor 保留参数；未启用 CAN 时不使用。
 * @param current_raw 保留参数；未启用 CAN 时不使用。
 * @retval false；不修改句柄、输出参数或硬件。
 * @note 仅在 HAL_CAN_MODULE_ENABLED 未定义时编译；不代表硬件已停止。
 */
bool C610_M2006_SetCurrent(C610_M2006_HandleTypeDef *hmotor,
                           int16_t current_raw) {
  (void)hmotor;
  (void)current_raw;
  return false;
}

/**
 * @brief 保留未启用 CAN 时的移植接口。
 * @param hmotor 保留参数；未启用 CAN 时不使用。
 * @param enabled 保留参数；未启用 CAN 时不使用。
 * @retval false；不修改句柄、输出参数或硬件。
 * @note 仅在 HAL_CAN_MODULE_ENABLED 未定义时编译；不代表硬件已停止。
 */
bool C610_M2006_SetOutputEnabled(C610_M2006_HandleTypeDef *hmotor,
                                 bool enabled) {
  (void)hmotor;
  (void)enabled;
  return false;
}

/**
 * @brief 保留未启用 CAN 时的移植接口。
 * @param hcan 保留参数；未启用 CAN 时不使用。
 * @param rx_header 保留参数；未启用 CAN 时不使用。
 * @param data 保留参数；未启用 CAN 时不使用。
 * @retval false；不修改句柄、输出参数或硬件。
 * @note 仅在 HAL_CAN_MODULE_ENABLED 未定义时编译；不代表硬件已停止。
 */
bool C610_M2006_HandleRxMessage(
    CAN_HandleTypeDef *hcan, const CAN_RxHeaderTypeDef *rx_header,
    const uint8_t data[C610_M2006_FRAME_DLC]) {
  (void)hcan;
  (void)rx_header;
  (void)data;
  return false;
}

/**
 * @brief 保留未启用 CAN 时的移植接口。
 * @param hcan 保留参数；未启用 CAN 时不使用。
 * @param rx_fifo 保留参数；未启用 CAN 时不使用。
 * @retval false；不修改句柄、输出参数或硬件。
 * @note 仅在 HAL_CAN_MODULE_ENABLED 未定义时编译；不代表硬件已停止。
 */
bool C610_M2006_RxFifoCallback(CAN_HandleTypeDef *hcan, uint32_t rx_fifo) {
  (void)hcan;
  (void)rx_fifo;
  return false;
}

/**
 * @brief 保留未启用 CAN 时的移植接口。
 * @param hcan 保留参数；未启用 CAN 时不使用。
 * @retval false；不修改句柄、输出参数或硬件。
 * @note 仅在 HAL_CAN_MODULE_ENABLED 未定义时编译；不代表硬件已停止。
 */
bool C610_M2006_SendAll(CAN_HandleTypeDef *hcan) {
  (void)hcan;
  return false;
}

/**
 * @brief 保留未启用 CAN 时的移植接口。
 * @param hmotor 保留参数；未启用 CAN 时不使用。
 * @param now_tick 保留参数；未启用 CAN 时不使用。
 * @retval false；不修改句柄、输出参数或硬件。
 * @note 仅在 HAL_CAN_MODULE_ENABLED 未定义时编译；不代表硬件已停止。
 */
bool C610_M2006_Process(C610_M2006_HandleTypeDef *hmotor,
                        uint32_t now_tick) {
  (void)hmotor;
  (void)now_tick;
  return false;
}

/**
 * @brief 保留未启用 CAN 时的移植接口。
 * @param hmotor 保留参数；未启用 CAN 时不使用。
 * @param feedback 保留参数；未启用 CAN 时不使用。
 * @retval false；不修改句柄、输出参数或硬件。
 * @note 仅在 HAL_CAN_MODULE_ENABLED 未定义时编译；不代表硬件已停止。
 */
bool C610_M2006_GetFeedback(const C610_M2006_HandleTypeDef *hmotor,
                            C610_M2006_FeedbackTypeDef *feedback) {
  (void)hmotor;
  (void)feedback;
  return false;
}

/**
 * @brief 保留未启用 CAN 时的移植接口。
 * @param hmotor 保留参数；未启用 CAN 时不使用。
 * @retval false；不修改句柄、输出参数或硬件。
 * @note 仅在 HAL_CAN_MODULE_ENABLED 未定义时编译；不代表硬件已停止。
 */
bool C610_M2006_IsOnline(const C610_M2006_HandleTypeDef *hmotor) {
  (void)hmotor;
  return false;
}

#else

typedef struct {
  C610_M2006_HandleTypeDef *handle; /* 静态注册表中对应的设备句柄。每个槽位只放一个设备。 */
  bool in_use;                      /* true 表示该槽位已被一个句柄占用。避免重复注册。 */
} C610_M2006_SlotTypeDef;

static C610_M2006_SlotTypeDef
    g_c610_m2006_slots[C610_M2006_MAX_DEVICE_COUNT]; /* 任务注册、CAN ISR 查找；句柄必须持久，注册/注销须串行化。 */

/* 聚合发送期间屏蔽 CAN ISR，保证所有槽位来自同一份状态并一起提交。 */
static uint32_t C610_M2006_EnterCritical(void) {
  const uint32_t previous = __get_PRIMASK();
  __disable_irq();
  __DMB();
  return previous;
}

/* 恢复调用者原来的中断状态。不能无条件开中断破坏外层临界区。 */
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

/* 检查 M2006/C610 反馈 ID 是否落在 1~8。越界 ID 不能映射到控制帧槽位。 */
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

/* 解析大端有符号字段。直接指针读取会因 STM32 小端布局交换 DATA[0]/DATA[1]。 */
static int16_t C610_M2006_ReadI16Be(const uint8_t data[2]) {
  /* 电调协议先发送高字节。不能把 data 地址强转成 int16_t*。STM32 小端会读反字节。 */
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
  data[0] = (uint8_t)(raw >> 8U); /* CAN DATA[0] 放高字节。DATA[1] 放低字节。 */
  data[1] = (uint8_t)raw;
}

/* 将 ID 1~4/5~8 映射到各自帧内的 0~3 槽位。两组不能直接用 motor_id 做数组下标。 */
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

/* 提交一条 8 字节标准 CAN 控制帧。邮箱满时立即失败，不能在任务里忙等。 */
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

/**
 * @brief  注册并初始化一个 C610/M2006 设备句柄。
 * @param  hmotor 由调用方提供的句柄存储，不能为 NULL。
 * @param  config  CAN 句柄、电调 ID 和超时参数。
 * @retval true   初始化成功并完成静态注册。
 * @retval false  参数非法、ID 重复、总线为空或注册表已满。
 * @note   本函数不启动 CAN 外设，也不配置过滤器。这属于 CubeMX/板级层。
 * @note 仅所属任务初始化。调用者串行化注册操作，注册时保持输出关闭。
 */
bool C610_M2006_Init(C610_M2006_HandleTypeDef *hmotor,
                     const C610_M2006_ConfigTypeDef *config) {
  if (hmotor == NULL || config == NULL || config->hcan == NULL ||
      !C610_M2006_IsValidId(config->motor_id) ||
      C610_M2006_Find(config->hcan, config->motor_id) != NULL) {
    return false;
  }

  memset(hmotor, 0, sizeof(*hmotor));  /* 清除旧句柄状态。从零状态重新初始化。 */
  /* 复制配置值。句柄不能保存调用者的局部配置指针。 */
  hmotor->config = *config;
  if (hmotor->config.feedback_timeout_ms == 0U) {
    hmotor->config.feedback_timeout_ms = C610_M2006_FEEDBACK_TIMEOUT_MS;
  }

  /* 放入静态注册表。驱动用固定槽位管理电机，不在运行时 malloc，适合 ISR/任务并发。 */
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

/**
 * @brief  注销一个已注册设备并清零其运行状态。
 * @param  hmotor 已初始化的设备句柄。
 * @retval true   注销成功。
 * @retval false  参数为空或句柄未注册。
 * @note 仅所属任务调用。先提交零电流，并屏蔽 CAN 接收中断后注销。
 */
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

/**
 * @brief  设置目标电流原始值。
 * @param  hmotor 电机句柄。
 * @param  current_raw C610 CAN 协议中的有符号电流原始值。
 * @retval true   值已写入目标缓存。
 * @retval false  参数非法或设备未初始化。
 * @note   本函数只更新 RAM 中的目标值。真正发送由
 * C610_M2006_SendAll() 完成，因此适合在控制任务中调用。
 * @note 仅所属控制任务调用；ISR 不设置目标。
 */
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

/**
 * @brief  设置输出使能状态。
 * @param  hmotor 电机句柄。
 * @param  enabled true 允许发送目标电流，false 强制本设备电流为零。
 * @retval true   状态已更新。
 * @retval false  参数非法或设备未初始化。
 * @note 仅所属控制任务调用；ISR 不改变输出许可。
 */
bool C610_M2006_SetOutputEnabled(C610_M2006_HandleTypeDef *hmotor,
                                 bool enabled) {
  if (hmotor == NULL || !hmotor->initialized ||
      !C610_M2006_IsRegistered(hmotor)) {
    return false;
  }

  /* 未收到新鲜反馈时拒绝打开输出。只改目标缓存而不设安全门会让首帧前的旧命令直接上总线。 */
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

/**
 * @brief  处理一帧已经从 HAL CAN FIFO 取出的接收数据。
 * @param  hcan CAN 外设句柄。
 * @param  rx_header HAL 接收帧头。
 * @param  data 8 字节 CAN 数据区。
 * @retval true   该帧属于已注册设备并已更新反馈。
 * @retval false  该帧不是本驱动支持的反馈帧。
 * @note   可从 `HAL_CAN_RxFifo0MsgPendingCallback` 调用。不要在这里阻塞。
 * @note 仅由串行化的 CAN 接收路径调用；通常位于 CAN ISR。
 */
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
   * 反馈字段是协议定义的 8 字节快照。角度、转速、电流均为大端有符号/无符号
   * 整数，DATA[6] 为空、DATA[7] 为错误码。接收中断只完成解码和时间戳更新，
   * 把控制决策留给任务。ISR 只翻译 CAN 帧，不执行控制算法。
   */
  /* 手册规定角度为 13 位 0~8191。屏蔽未定义高位，避免把保留位当角度。只保留低 13 位。 */
  hmotor->feedback_sequence++;
  __DMB();
  const uint16_t angle_raw =
      C610_M2006_ReadU16Be(&data[0]) & (C610_M2006_ENCODER_COUNTS_PER_REV - 1U);
  /* 每个 CAN 帧展开角度。若只在 2 ms 任务中展开，高速时可能漏过半圈。不能跳帧计圈。 */
  if (!hmotor->feedback_received) {
    hmotor->feedback.angle_total_raw = angle_raw;
  } else {
    int32_t delta = (int32_t)angle_raw - hmotor->feedback.angle_raw;
    if (delta > 4096) {
      delta -= 8192;
    } else if (delta < -4096) {
      delta += 8192;
    }
    hmotor->feedback.angle_total_raw += delta;
  }
  hmotor->feedback.angle_raw = angle_raw;
  hmotor->feedback.speed_rpm = C610_M2006_ReadI16Be(&data[2]);
  hmotor->feedback.current_raw = C610_M2006_ReadI16Be(&data[4]);
  hmotor->feedback.reserved_raw = data[6]; /* C610 手册明确 DATA[6] 为空。原样留给诊断。 */
  hmotor->feedback.error_code = data[7];
  hmotor->feedback.last_feedback_tick = HAL_GetTick();
  hmotor->feedback_received = true;
  hmotor->state = hmotor->output_enabled ? C610_M2006_STATE_RUNNING
                                         : C610_M2006_STATE_ONLINE;
  __DMB();
  hmotor->feedback_sequence++;
  return true;
}

/**
 * @brief  从硬件 CAN FIFO 取帧并交给本驱动解析。
 * @param  hcan CAN 外设句柄。
 * @param  rx_fifo HAL CAN FIFO 编号，例如 CAN_RX_FIFO0。
 * @retval true   取帧成功且属于本驱动设备。
 * @retval false  HAL 取帧失败或帧不匹配。
 * @note 仅 CAN ISR 调用。多个驱动共享 FIFO 时使用统一分发入口。
 */
bool C610_M2006_RxFifoCallback(CAN_HandleTypeDef *hcan, uint32_t rx_fifo) {
  CAN_RxHeaderTypeDef rx_header = {0};
  uint8_t data[C610_M2006_FRAME_DLC] = {0};
  if (hcan == NULL || HAL_CAN_GetRxMessage(hcan, rx_fifo, &rx_header, data) !=
                          HAL_OK) {
    return false;
  }
  return C610_M2006_HandleRxMessage(hcan, &rx_header, data);
}

/**
 * @brief  发送所有已注册设备的聚合控制帧。
 * @param  hcan 指定 CAN 总线句柄。
 * @retval true   没有发送错误，或该总线上没有设备。
 * @retval false  CAN 邮箱不足、句柄非法或 HAL 发送失败。
 * @note   建议由固定周期任务调用。每条总线最多发送 0x200 和 0x1FF
 * 两帧，帧中的每两个字节对应一个电机 ID。函数在提交前检查反馈
 * 新鲜度，并在短 PRIMASK 临界区内完成槽位读取、构建和提交。不等待邮箱。
 * @note 仅任务调用。每条总线的槽位构建与提交使用同一短临界区。
 */
bool C610_M2006_SendAll(CAN_HandleTypeDef *hcan) {
  if (hcan == NULL) {
    return false;
  }
  if (C610_M2006_FindFirstOnBus(hcan) == NULL) {
    return true;
  }

  const uint32_t previous = C610_M2006_EnterCritical();
  const uint32_t now_ms = HAL_GetTick();

  uint8_t low_data[C610_M2006_FRAME_DLC] = {0}; /* 0x200 控制帧数据。电机 ID 1~4 各占两个字节。 */
  uint8_t high_data[C610_M2006_FRAME_DLC] = {0}; /* 0x1FF 控制帧数据。电机 ID 5~8 各占两个字节。 */
  bool low_used = false;
  bool high_used = false;

  /* 遍历这条 CAN 总线上的所有注册电机。不同总线不能混装在同一帧。 */
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
       * slot=0：从下标 0 开始写 2 字节 → ID1 电流
       * slot=1：从下标 2 开始写 2 字节 → ID2 电流
       * slot=2：从下标 4 开始写 2 字节 → ID3 电流
       * slot=3：从下标 6 开始写 2 字节 → ID4 电流。槽位 = ID - 1。
       */
      C610_M2006_WriteI16Be(&low_data[2U * slot], current);
      low_used = true; /* 标记 0x200 帧已使用。至少有一个低编号电机需要发送。 */
    } else {
      C610_M2006_WriteI16Be(&high_data[2U * slot], current);
      high_used = true; /* 标记 0x1FF 帧已使用。至少有一个高编号电机需要发送。 */
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

/**
 * @brief  按 HAL ms 更新在线状态。
 * @param  hmotor 电机句柄。
 * @param  now_tick HAL_GetTick() 的当前 ms。不能传 FreeRTOS Tick。
 * @retval true   状态检查完成。
 * @retval false  参数非法或设备未初始化。
 * @note 仅所属任务调用；时间必须与 CAN ISR 的 HAL ms 同源。
 */
bool C610_M2006_Process(C610_M2006_HandleTypeDef *hmotor,
                        uint32_t now_tick) {
  if (hmotor == NULL || !hmotor->initialized ||
      !C610_M2006_IsRegistered(hmotor)) {
    return false;
  }

  /*
   * 没有收到过反馈时，last_feedback_tick 默认为 0，不能把启动早期的小时间差误认为在线。
   * 必须先确认真实 CAN 帧到达。feedback_received 是首帧门槛。
   */
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

/**
 * @brief  复制一份反馈快照。
 * @param  hmotor 电机句柄。
 * @param  feedback 输出结构体。
 * @retval true   复制成功。
 * @retval false  参数非法、未初始化或复制期间连续冲突。
 * @note   接收中断可能同时更新句柄。实现使用短序列号重试取得同一帧，
 * 连续冲突时返回 false，调用者不能把失败输出当成真实反馈。
 * @note 仅任务读取。失败输出不可用于控制；快照本身不保证反馈新鲜。
 */
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

/**
 * @brief  根据 HAL 毫秒时间判断反馈是否仍在线。只报告新鲜度，不保证多字段原子快照。
 * @param  hmotor 已初始化的 C610/M2006 句柄。
 * @retval true 最近收到有效反馈。false 未初始化、从未收到反馈或超过超时。
 * @note 仅任务查询新鲜度；需要多个反馈字段时读取 GetFeedback 快照。
 */
bool C610_M2006_IsOnline(const C610_M2006_HandleTypeDef *hmotor) {
  if (hmotor == NULL || !hmotor->initialized || !hmotor->feedback_received ||
      hmotor->state == C610_M2006_STATE_OFFLINE ||
      hmotor->state == C610_M2006_STATE_UNINITIALIZED) {
    return false;
  }
  /* 即使调用者漏掉本周期 Process，也不能把旧反馈永久当成在线。在线判断本身使用 HAL ms。 */
  return C610_M2006_FeedbackFresh(hmotor, HAL_GetTick());
}

#endif /* HAL_CAN_MODULE_ENABLED。 */
