#include "c620.h"
#include <string.h>

#define C620_FRAME_DLC 8U

typedef struct {
  C620_HandleTypeDef handle;
  bool in_use;
} C620_InstanceSlotTypeDef;

// 全局电调实例数组（最多支持8个C620电调）
static C620_HandleTypeDef *g_c620_instances[C620_MAX_ESC_COUNT] = {NULL};
static C620_InstanceSlotTypeDef g_c620_static_pool[C620_MAX_ESC_COUNT];
static uint8_t g_c620_instance_count = 0;

// ===================== 命令表声明 =====================
static Device_StatusTypeDef C620_Cmd_SetCurrent(void *dev, void *arg);
static Device_StatusTypeDef C620_Cmd_GetAngle(void *dev, void *arg);
static Device_StatusTypeDef C620_Cmd_GetSpeed(void *dev, void *arg);
static Device_StatusTypeDef C620_Cmd_GetCurrent(void *dev, void *arg);
static Device_StatusTypeDef C620_Cmd_GetTemp(void *dev, void *arg);
static Device_StatusTypeDef C620_Cmd_ResetError(void *dev, void *arg);
static Device_StatusTypeDef C620_Cmd_DisableOutput(void *dev, void *arg);
static Device_StatusTypeDef C620_Cmd_EnableOutput(void *dev, void *arg);

// C620命令表（所有命令统一在这里注册）
static const Command_TableTypeDef g_c620_cmd_table[] = {
    {C620_CMD_SET_CURRENT, C620_Cmd_SetCurrent, sizeof(int16_t), "SetCurrent"},
    {C620_CMD_ENABLE_OUTPUT, C620_Cmd_EnableOutput, 0, "EnableOutput"},
    {C620_CMD_DISABLE_OUTPUT, C620_Cmd_DisableOutput, 0, "DisableOutput"},
    {C620_CMD_GET_ANGLE, C620_Cmd_GetAngle, sizeof(uint16_t), "GetAngle"},
    {C620_CMD_GET_SPEED, C620_Cmd_GetSpeed, sizeof(int16_t), "GetSpeed"},
    {C620_CMD_GET_CURRENT, C620_Cmd_GetCurrent, sizeof(int16_t), "GetCurrent"},
    {C620_CMD_GET_TEMP, C620_Cmd_GetTemp, sizeof(uint8_t), "GetTemp"},
    {C620_CMD_RESET_ERROR, C620_Cmd_ResetError, 0, "ResetError"},
};

static const Command_TableTypeDef *C620_FindCommand(C620_CmdIDTypeDef cmd_id);
static void C620_Init_DefaultCallbacks(C620_CallbackTypeDef *callbacks);
static int16_t C620_AbsI16(int16_t value);
static bool C620_IsFatalOfficialError(C620_OfficialErrorTypeDef error);
static void C620_FailSafeStop(C620_HandleTypeDef *hc620,
                              C620_StateTypeDef state);
static void C620_SetState(C620_HandleTypeDef *hc620,
                          C620_StateTypeDef new_state);
static Device_StatusTypeDef C620_SendCommandFrame(CAN_HandleTypeDef *hcan,
                                                  uint32_t std_id,
                                                  const uint8_t data[8]);

// 默认空回调函数（防止空指针调用）
static void C620_Default_StateChange(C620_HandleTypeDef *hc620,
                                     C620_StateTypeDef old_state,
                                     C620_StateTypeDef new_state) {
  (void)hc620;
  (void)old_state;
  (void)new_state;
}
static void C620_Default_OfficialError(C620_HandleTypeDef *hc620,
                                       C620_OfficialErrorTypeDef error) {
  (void)hc620;
  (void)error;
}
static void C620_Default_ExtError(C620_HandleTypeDef *hc620,
                                  C620_ExtErrorTypeDef error) {
  (void)hc620;
  (void)error;
}
static void C620_Default_Warning(C620_HandleTypeDef *hc620,
                                 C620_WarningTypeDef warning) {
  (void)hc620;
  (void)warning;
}
static void C620_Default_DataUpdate(C620_HandleTypeDef *hc620) {
  (void)hc620;
}
static void C620_Default_Stall(C620_HandleTypeDef *hc620) { (void)hc620; }
static void C620_Default_CommRecover(C620_HandleTypeDef *hc620) {
  (void)hc620;
}

static const Command_TableTypeDef *C620_FindCommand(C620_CmdIDTypeDef cmd_id) {
  for (uint32_t i = 0U;
       i < (sizeof(g_c620_cmd_table) / sizeof(g_c620_cmd_table[0])); i++) {
    if (g_c620_cmd_table[i].cmd_id == (uint8_t)cmd_id) {
      return &g_c620_cmd_table[i];
    }
  }

  return NULL;
}

static void C620_Init_DefaultCallbacks(C620_CallbackTypeDef *callbacks) {
  callbacks->on_state_change = C620_Default_StateChange;
  callbacks->on_official_error = C620_Default_OfficialError;
  callbacks->on_ext_error = C620_Default_ExtError;
  callbacks->on_warning = C620_Default_Warning;
  callbacks->on_data_update = C620_Default_DataUpdate;
  callbacks->on_stall = C620_Default_Stall;
  callbacks->on_comm_recover = C620_Default_CommRecover;
}

static int16_t C620_AbsI16(int16_t value) {
  if (value == INT16_MIN) {
    return INT16_MAX;
  }
  return (value < 0) ? (int16_t)-value : value;
}

static bool C620_IsFatalOfficialError(C620_OfficialErrorTypeDef error) {
  switch (error) {
  case C620_ERROR_MOTOR_EEPROM:
  case C620_ERROR_OVER_VOLTAGE:
  case C620_ERROR_PHASE_MISSING:
  case C620_ERROR_ENCODER_LOST:
  case C620_ERROR_OVER_TEMP_CRIT:
  case C620_ERROR_CALIBRATION_FAIL:
    return true;
  case C620_ERROR_NONE:
  case C620_ERROR_OVER_TEMP_WARN:
  default:
    return false;
  }
}

static void C620_FailSafeStop(C620_HandleTypeDef *hc620,
                              C620_StateTypeDef state) {
  hc620->target_current = 0;
  hc620->enable_output = false;
  hc620->state = state;
}

static void C620_SetState(C620_HandleTypeDef *hc620,
                          C620_StateTypeDef new_state) {
  if (hc620->state != new_state) {
    const C620_StateTypeDef old_state = hc620->state;
    hc620->state = new_state;
    hc620->callbacks.on_state_change(hc620, old_state, new_state);
  }
}

static Device_StatusTypeDef C620_SendCommandFrame(CAN_HandleTypeDef *hcan,
                                                  uint32_t std_id,
                                                  const uint8_t data[8]) {
  CAN_TxHeaderTypeDef tx_header = {0};
  uint32_t tx_mailbox;

  if (HAL_CAN_GetTxMailboxesFreeLevel(hcan) == 0U) {
    return DEVICE_BUSY;
  }

  tx_header.StdId = std_id;
  tx_header.RTR = CAN_RTR_DATA;
  tx_header.IDE = CAN_ID_STD;
  tx_header.DLC = C620_FRAME_DLC;

  if (HAL_CAN_AddTxMessage(hcan, &tx_header, (uint8_t *)data, &tx_mailbox) !=
      HAL_OK) {
    return DEVICE_ERROR;
  }

  return DEVICE_OK;
}

// ===================== 对外接口实现 =====================
C620_HandleTypeDef *C620_Create(const char *name, uint8_t esc_id,
                                CAN_HandleTypeDef *hcan) {
  C620_ConfigTypeDef config = {
      .name = name,
      .esc_id = esc_id,
      .hcan = hcan,
  };

  for (uint32_t i = 0U; i < C620_MAX_ESC_COUNT; i++) {
    if (!g_c620_static_pool[i].in_use) {
      if (C620_Init(&g_c620_static_pool[i].handle, &config) != DEVICE_OK) {
        return NULL;
      }
      g_c620_static_pool[i].in_use = true;
      return &g_c620_static_pool[i].handle;
    }
  }

  return NULL;
}

Device_StatusTypeDef C620_Init(C620_HandleTypeDef *hc620,
                               const C620_ConfigTypeDef *config) {
  if (hc620 == NULL || config == NULL || config->name == NULL ||
      config->hcan == NULL) {
    return DEVICE_NO_PARAMETER;
  }

  if (config->esc_id < 1U || config->esc_id > C620_MAX_ESC_COUNT) {
    return DEVICE_INVALID_PARAM;
  }

  if (g_c620_instance_count >= C620_MAX_ESC_COUNT ||
      g_c620_instances[config->esc_id - 1U] != NULL) {
    return DEVICE_BUSY;
  }

  // 清零句柄
  memset(hc620, 0, sizeof(C620_HandleTypeDef));

  // 初始化基类
  hc620->base.name = config->name;
  hc620->base.device_id = config->esc_id;
  hc620->base.is_initialized = true;
  hc620->base.deinit = (Device_StatusTypeDef (*)(DeviceBase *))C620_Destroy;

  // 初始化硬件相关
  hc620->hcan = config->hcan;
  hc620->esc_id = config->esc_id;

  // 初始化状态
  hc620->state = C620_STATE_STOP;
  hc620->error.official = C620_ERROR_NONE;
  hc620->error.ext = C620_EXT_ERROR_NONE;
  hc620->warning = C620_WARNING_NONE;
  hc620->mechanical_angle = 0U;
  hc620->last_mechanical_angle = 0U;
  hc620->feedback_timeout_enable = true;
  hc620->data_updated = false;
  hc620->comm_recovered = false;
  hc620->last_rx_time = HAL_GetTick();

  // 初始化回调函数为默认空函数
  C620_Init_DefaultCallbacks(&hc620->callbacks);

  // 初始化内部函数指针
  hc620->process_loop = C620_Process_Loop;

  // 加入全局实例数组
  g_c620_instances[config->esc_id - 1U] = hc620;
  g_c620_instance_count++;

  return DEVICE_OK;
}

Device_StatusTypeDef C620_SetFeedbackTimeoutEnable(C620_HandleTypeDef *hc620,
                                                   bool enable) {
  if (hc620 == NULL) {
    return DEVICE_NO_PARAMETER;
  }

  hc620->feedback_timeout_enable = enable;
  if (!enable) {
    hc620->error.ext &= ~C620_EXT_ERROR_COMM_TIMEOUT;
    hc620->last_rx_time = HAL_GetTick();
  }

  return DEVICE_OK;
}

Device_StatusTypeDef C620_Destroy(C620_HandleTypeDef *hc620) {
  if (hc620 == NULL) {
    return DEVICE_NO_PARAMETER;
  }

  // 从全局实例数组中移除
  if (hc620->esc_id >= 1U && hc620->esc_id <= C620_MAX_ESC_COUNT &&
      g_c620_instances[hc620->esc_id - 1U] == hc620) {
    g_c620_instances[hc620->esc_id - 1U] = NULL;
    if (g_c620_instance_count > 0U) {
      g_c620_instance_count--;
    }
  }

  for (uint32_t i = 0U; i < C620_MAX_ESC_COUNT; i++) {
    if (&g_c620_static_pool[i].handle == hc620) {
      g_c620_static_pool[i].in_use = false;
      break;
    }
  }

  memset(hc620, 0, sizeof(*hc620));
  return DEVICE_OK;
}

Device_StatusTypeDef
C620_RegisterCallback(C620_HandleTypeDef *hc620,
                      const C620_CallbackTypeDef *callbacks) {
  if (hc620 == NULL || callbacks == NULL) {
    return DEVICE_NO_PARAMETER;
  }

  C620_Init_DefaultCallbacks(&hc620->callbacks);
  if (callbacks->on_state_change != NULL) {
    hc620->callbacks.on_state_change = callbacks->on_state_change;
  }
  if (callbacks->on_official_error != NULL) {
    hc620->callbacks.on_official_error = callbacks->on_official_error;
  }
  if (callbacks->on_ext_error != NULL) {
    hc620->callbacks.on_ext_error = callbacks->on_ext_error;
  }
  if (callbacks->on_warning != NULL) {
    hc620->callbacks.on_warning = callbacks->on_warning;
  }
  if (callbacks->on_data_update != NULL) {
    hc620->callbacks.on_data_update = callbacks->on_data_update;
  }
  if (callbacks->on_stall != NULL) {
    hc620->callbacks.on_stall = callbacks->on_stall;
  }
  if (callbacks->on_comm_recover != NULL) {
    hc620->callbacks.on_comm_recover = callbacks->on_comm_recover;
  }
  return DEVICE_OK;
}

// ===================== 统一命令执行接口 =====================
Device_StatusTypeDef C620_ExecuteCommand(C620_HandleTypeDef *hc620,
                                         C620_CmdIDTypeDef cmd_id, void *arg) {
  if (hc620 == NULL) {
    return DEVICE_NO_PARAMETER;
  }

  const Command_TableTypeDef *cmd = C620_FindCommand(cmd_id);
  if (cmd == NULL) {
    return DEVICE_INVALID_PARAM;
  }

  // 检查参数大小
  if (cmd->arg_size > 0 && arg == NULL) {
    return DEVICE_INVALID_PARAM;
  }

  // 执行命令处理函数
  return cmd->handler(hc620, arg);
}

// ===================== 命令处理函数实现 =====================
static Device_StatusTypeDef C620_Cmd_SetCurrent(void *dev, void *arg) {
  C620_HandleTypeDef *hc620 = (C620_HandleTypeDef *)dev;
  int16_t current = *(int16_t *)arg;

  if (current > C620_MAX_CURRENT_RAW)
    current = C620_MAX_CURRENT_RAW;
  if (current < C620_MIN_CURRENT_RAW)
    current = C620_MIN_CURRENT_RAW;

  hc620->target_current = current;
  return DEVICE_OK;
}

static Device_StatusTypeDef C620_Cmd_GetAngle(void *dev, void *arg) {
  C620_HandleTypeDef *hc620 = (C620_HandleTypeDef *)dev;
  *(uint16_t *)arg = hc620->mechanical_angle;
  return DEVICE_OK;
}

static Device_StatusTypeDef C620_Cmd_GetSpeed(void *dev, void *arg) {
  C620_HandleTypeDef *hc620 = (C620_HandleTypeDef *)dev;
  *(int16_t *)arg = hc620->actual_speed;
  return DEVICE_OK;
}

static Device_StatusTypeDef C620_Cmd_GetCurrent(void *dev, void *arg) {
  C620_HandleTypeDef *hc620 = (C620_HandleTypeDef *)dev;
  *(int16_t *)arg = hc620->actual_current;
  return DEVICE_OK;
}

static Device_StatusTypeDef C620_Cmd_GetTemp(void *dev, void *arg) {
  C620_HandleTypeDef *hc620 = (C620_HandleTypeDef *)dev;
  *(uint8_t *)arg = hc620->actual_temp;
  return DEVICE_OK;
}

static Device_StatusTypeDef C620_Cmd_ResetError(void *dev, void *arg) {
  C620_HandleTypeDef *hc620 = (C620_HandleTypeDef *)dev;
  (void)arg;
  hc620->error.official = C620_ERROR_NONE;
  hc620->error.ext = C620_EXT_ERROR_NONE;
  hc620->warning = C620_WARNING_NONE;
  hc620->target_current = 0;
  hc620->enable_output = false;
  hc620->stall_start_time = 0;
  C620_SetState(hc620, C620_STATE_STOP);
  return DEVICE_OK;
}

static Device_StatusTypeDef C620_Cmd_DisableOutput(void *dev, void *arg) {
  C620_HandleTypeDef *hc620 = (C620_HandleTypeDef *)dev;
  (void)arg;
  hc620->target_current = 0;
  hc620->enable_output = false;
  C620_SetState(hc620, C620_STATE_STOP);
  return DEVICE_OK;
}

static Device_StatusTypeDef C620_Cmd_EnableOutput(void *dev, void *arg) {
  C620_HandleTypeDef *hc620 = (C620_HandleTypeDef *)dev;
  (void)arg;
  if (hc620->error.official == C620_ERROR_NONE &&
      hc620->error.ext == C620_EXT_ERROR_NONE &&
      hc620->warning == C620_WARNING_NONE) {
    hc620->enable_output = true;
    C620_SetState(hc620, C620_STATE_RUNNING);
  }
  return DEVICE_OK;
}

// ===================== CAN接收回调函数 =====================
bool C620_CAN_Rx_Callback(CAN_HandleTypeDef *hcan, uint32_t RxFifo) {
  CAN_RxHeaderTypeDef rx_header;
  uint8_t rx_data[8];

  if (HAL_CAN_GetRxMessage(hcan, RxFifo, &rx_header, rx_data) != HAL_OK) {
    return false;
  }

  return C620_HandleCanRxMessage(hcan, &rx_header, rx_data);
}

bool C620_HandleCanRxMessage(CAN_HandleTypeDef *hcan,
                             const CAN_RxHeaderTypeDef *rx_header,
                             const uint8_t data[8]) {
  if (hcan == NULL || rx_header == NULL || data == NULL) {
    return false;
  }

  if (rx_header->IDE != CAN_ID_STD || rx_header->RTR != CAN_RTR_DATA ||
      rx_header->DLC != C620_FRAME_DLC) {
    return false;
  }

  if (rx_header->StdId <= C620_CAN_FEEDBACK_ID_BASE ||
      rx_header->StdId > (C620_CAN_FEEDBACK_ID_BASE + C620_MAX_ESC_COUNT)) {
    return false;
  }

  // 解析C620反馈数据（反馈帧ID = 0x200 + ESC_ID, 即0x201~0x208）
  uint8_t esc_id = rx_header->StdId - C620_CAN_FEEDBACK_ID_BASE;
  if (esc_id < 1 || esc_id > C620_MAX_ESC_COUNT ||
      g_c620_instances[esc_id - 1] == NULL) {
    return false;
  }

  C620_HandleTypeDef *hc620 = g_c620_instances[esc_id - 1];
  if (hc620->hcan != hcan) {
    return false;
  }

  // 更新反馈数据（C620 CAN协议：字节0-1=机械角度, 2-3=转速, 4-5=转矩电流, 6=温度, 7=错误码）
  hc620->last_mechanical_angle = hc620->mechanical_angle;
  hc620->mechanical_angle = (uint16_t)(data[0] << 8 | data[1]);
  hc620->actual_speed = (int16_t)(data[2] << 8 | data[3]);
  hc620->actual_current = (int16_t)(data[4] << 8 | data[5]);
  hc620->actual_temp = data[6];
  hc620->error.official = (C620_OfficialErrorTypeDef)data[7];
  hc620->last_rx_time = HAL_GetTick();
  hc620->data_updated = true;

  // 检查通信超时恢复
  if (hc620->error.ext & C620_EXT_ERROR_COMM_TIMEOUT) {
    hc620->error.ext &= ~C620_EXT_ERROR_COMM_TIMEOUT;
    hc620->comm_recovered = true;
  }

  return true;
}

// ===================== 全局CAN发送函数（1ms调用一次） =====================
Device_StatusTypeDef C620_Send_All_Commands(void) {
  CAN_HandleTypeDef *hcan = NULL;
  int i;

  // 查找第一个有效实例获取CAN句柄
  for (i = 0; i < C620_MAX_ESC_COUNT; i++) {
    if (g_c620_instances[i] != NULL) {
      hcan = g_c620_instances[i]->hcan;
      break;
    }
  }
  if (hcan == NULL) {
    return DEVICE_OK;
  }

  // 填充ESC 1-4电流数据 → CAN ID 0x200
  uint8_t tx_data_low[8] = {0};
  uint8_t tx_data_high[8] = {0};
  bool has_low = false;
  bool has_high = false;

  for (i = 0; i < 4; i++) {
    C620_HandleTypeDef *hc620 = g_c620_instances[i];
    if (hc620 == NULL) {
      continue;
    }
    has_low = true;
    if (hc620->enable_output &&
        hc620->state != C620_STATE_ERROR &&
        hc620->state != C620_STATE_STOP) {
      tx_data_low[2 * i] = (hc620->target_current >> 8) & 0xFF;
      tx_data_low[2 * i + 1] = hc620->target_current & 0xFF;
    }
  }

  // 填充ESC 5-8电流数据 → CAN ID 0x1FF
  for (i = 0; i < 4; i++) {
    C620_HandleTypeDef *hc620 = g_c620_instances[i + 4];
    if (hc620 == NULL) {
      continue;
    }
    has_high = true;
    if (hc620->enable_output &&
        hc620->state != C620_STATE_ERROR &&
        hc620->state != C620_STATE_STOP) {
      tx_data_high[2 * i] = (hc620->target_current >> 8) & 0xFF;
      tx_data_high[2 * i + 1] = hc620->target_current & 0xFF;
    }
  }

  // 发送第一帧: CAN ID 0x200 (ESC 1-4)
  if (has_low) {
    Device_StatusTypeDef status =
        C620_SendCommandFrame(hcan, C620_CAN_CTRL_ID_BASE, tx_data_low);
    if (status != DEVICE_OK) {
      return status;
    }
  }

  // 发送第二帧: CAN ID 0x1FF (ESC 5-8)
  if (has_high) {
    Device_StatusTypeDef status =
        C620_SendCommandFrame(hcan, C620_CAN_CTRL_ID_EXT, tx_data_high);
    if (status != DEVICE_OK) {
      return status;
    }
  }

  return DEVICE_OK;
}

// ===================== 主循环处理函数 =====================
Device_StatusTypeDef C620_Process_Loop(C620_HandleTypeDef *hc620) {
  if (hc620 == NULL) {
    return DEVICE_NO_PARAMETER;
  }

  C620_StateTypeDef old_state = hc620->state;
  const bool data_updated = hc620->data_updated;
  const bool comm_recovered = hc620->comm_recovered;
  hc620->data_updated = false;
  hc620->comm_recovered = false;

  if (data_updated) {
    hc620->callbacks.on_data_update(hc620);
  }

  if (comm_recovered) {
    hc620->callbacks.on_comm_recover(hc620);
    if (hc620->enable_output &&
        !C620_IsFatalOfficialError(hc620->error.official) &&
        hc620->error.ext == C620_EXT_ERROR_NONE) {
      C620_SetState(hc620, C620_STATE_RUNNING);
      old_state = hc620->state;
    }
  }

  // 1. 检查通信超时
  if (hc620->feedback_timeout_enable &&
      is_time_A_after_B(HAL_GetTick(),
                        hc620->last_rx_time + C620_COMM_TIMEOUT_MS)) {
    hc620->error.ext |= C620_EXT_ERROR_COMM_TIMEOUT;
    C620_FailSafeStop(hc620, C620_STATE_ERROR);
  }

  // 2. 检查电调官方错误码
  if (hc620->error.official == C620_ERROR_OVER_TEMP_WARN) {
    hc620->warning |= C620_WARNING_OVER_TEMP;
#if C620_TEMP_WARN_SHUTDOWN_ENABLE
    hc620->error.ext |= C620_EXT_ERROR_OVER_TEMP;
    C620_FailSafeStop(hc620, C620_STATE_ERROR);
#else
    if (hc620->state == C620_STATE_RUNNING) {
      hc620->state = C620_STATE_WARNING;
    }
#endif
  } else if (C620_IsFatalOfficialError(hc620->error.official)) {
    C620_FailSafeStop(hc620, C620_STATE_ERROR);
  } else if (hc620->warning != C620_WARNING_NONE &&
             hc620->actual_temp < C620_OVER_TEMP_WARN) {
    hc620->warning = C620_WARNING_NONE;
    hc620->error.ext &= ~C620_EXT_ERROR_OVER_TEMP;
    if (hc620->enable_output && hc620->error.ext == C620_EXT_ERROR_NONE) {
      hc620->state = C620_STATE_RUNNING;
    }
  }

  // 3. 检查过流
  if (C620_AbsI16(hc620->actual_current) >= C620_MAX_CURRENT_RAW) {
    hc620->error.ext |= C620_EXT_ERROR_OVER_CURRENT;
    C620_FailSafeStop(hc620, C620_STATE_ERROR);
  }

  // 4. 检查过热
  if (hc620->actual_temp >= C620_OVER_TEMP_WARN) {
    if ((hc620->warning & C620_WARNING_OVER_TEMP) == 0U) {
      hc620->warning |= C620_WARNING_OVER_TEMP;
      hc620->callbacks.on_warning(hc620, C620_WARNING_OVER_TEMP);
    }
#if C620_TEMP_WARN_SHUTDOWN_ENABLE
    hc620->error.ext |= C620_EXT_ERROR_OVER_TEMP;
    C620_FailSafeStop(hc620, C620_STATE_ERROR);
#elif hc620->state == C620_STATE_RUNNING
    hc620->state = C620_STATE_WARNING;
#endif
  }
  if (hc620->actual_temp >= C620_OVER_TEMP_CRIT) {
    hc620->error.official = C620_ERROR_OVER_TEMP_CRIT;
    hc620->error.ext |= C620_EXT_ERROR_OVER_TEMP;
    C620_FailSafeStop(hc620, C620_STATE_ERROR);
  }

  // 5. 检查堵转
  if (hc620->state == C620_STATE_RUNNING) {
    if (C620_AbsI16(hc620->actual_current) > C620_STALL_CURRENT_RAW &&
        C620_AbsI16(hc620->actual_speed) < 10) {

      if (hc620->stall_start_time == 0) {
        hc620->stall_start_time = HAL_GetTick();
      } else if (is_time_A_after_B(HAL_GetTick(), hc620->stall_start_time +
                                                      C620_STALL_TIME_MS)) {
        hc620->error.ext |= C620_EXT_ERROR_STALL;
        C620_FailSafeStop(hc620, C620_STATE_STALL);
        hc620->callbacks.on_stall(hc620);
      }
    } else {
      hc620->stall_start_time = 0;
    }
  }

  if (hc620->state == C620_STATE_ERROR || hc620->state == C620_STATE_STALL) {
    hc620->enable_output = false;
  }

  // 5. 状态变化处理
  if (hc620->state != old_state) {
    hc620->callbacks.on_state_change(hc620, old_state, hc620->state);

    // 官方硬件错误回调
    if (C620_IsFatalOfficialError(hc620->error.official)) {
      hc620->callbacks.on_official_error(hc620, hc620->error.official);
    }

    // MCU扩展错误回调
    if (hc620->error.ext != C620_EXT_ERROR_NONE) {
      hc620->callbacks.on_ext_error(hc620, hc620->error.ext);
    }
  }

  return DEVICE_OK;
}
