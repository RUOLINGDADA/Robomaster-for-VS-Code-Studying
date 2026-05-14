#include "c620.h"

// 全局电调实例数组（最多8个，对应ID1-8）
static C620_HandleTypeDef *g_c620_instances[C620_MAX_ESC_COUNT] = {NULL};
static uint8_t g_c620_instance_count = 0;

// ===================== 命令处理函数声明 =====================
static Device_StatusTypeDef C620_Cmd_SetCurrent(void *dev, void *arg);
static Device_StatusTypeDef C620_Cmd_EnableOutput(void *dev, void *arg);
static Device_StatusTypeDef C620_Cmd_DisableOutput(void *dev, void *arg);
static Device_StatusTypeDef C620_Cmd_GetAngle(void *dev, void *arg);
static Device_StatusTypeDef C620_Cmd_GetSpeed(void *dev, void *arg);
static Device_StatusTypeDef C620_Cmd_GetCurrent(void *dev, void *arg);
static Device_StatusTypeDef C620_Cmd_GetTemp(void *dev, void *arg);
static Device_StatusTypeDef C620_Cmd_ResetError(void *dev, void *arg);

// ===================== 命令表（所有命令统一注册） =====================
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

// 默认空回调函数（防止空指针调用崩溃）
static void C620_Default_Callback(void) { return; }

// ===================== 对外接口实现 =====================
C620_HandleTypeDef *C620_Create(const char *name, uint8_t esc_id,
                                CAN_HandleTypeDef *hcan) {
  DEVICE_ASSERT(name != NULL);
  DEVICE_ASSERT(esc_id >= 1 && esc_id <= C620_MAX_ESC_COUNT);
  DEVICE_ASSERT(hcan != NULL);

  // 检查ID是否已被占用
  if (g_c620_instances[esc_id - 1] != NULL) {
    return NULL;
  }

  // 分配内存
  C620_HandleTypeDef *hc620 =
      (C620_HandleTypeDef *)malloc(sizeof(C620_HandleTypeDef));
  if (hc620 == NULL) {
    return NULL;
  }

  // 清零句柄
  memset(hc620, 0, sizeof(C620_HandleTypeDef));

  // 初始化设备基类
  hc620->base.name = name;
  hc620->base.device_id = esc_id;
  hc620->base.is_initialized = true;
  hc620->base.deinit = (Device_StatusTypeDef (*)(DeviceBase *))C620_Destroy;

  // 初始化硬件配置
  hc620->hcan = hcan;
  hc620->esc_id = esc_id;

  // 初始化状态
  hc620->state = C620_STATE_INIT;
  hc620->error.official = C620_ERROR_NONE;
  hc620->error.ext = C620_EXT_ERROR_NONE;
  hc620->warning = C620_WARNING_NONE;
  hc620->enable_output = false;
  hc620->target_current = 0;

  // 初始化回调函数为默认空函数
  hc620->callbacks.on_state_change = (void *)C620_Default_Callback;
  hc620->callbacks.on_official_error = (void *)C620_Default_Callback;
  hc620->callbacks.on_ext_error = (void *)C620_Default_Callback;
  hc620->callbacks.on_warning = (void *)C620_Default_Callback;
  hc620->callbacks.on_data_update = (void *)C620_Default_Callback;
  hc620->callbacks.on_stall = (void *)C620_Default_Callback;
  hc620->callbacks.on_comm_recover = (void *)C620_Default_Callback;

  // 初始化内部函数指针
  hc620->process_loop = C620_Process_Loop;

  // 加入全局实例数组
  g_c620_instances[esc_id - 1] = hc620;
  g_c620_instance_count++;

  return hc620;
}

Device_StatusTypeDef C620_Destroy(C620_HandleTypeDef *hc620) {
  if (hc620 == NULL) {
    return DEVICE_NO_PARAMETER;
  }

  // 禁用输出
  hc620->enable_output = false;
  hc620->target_current = 0;

  // 从全局实例数组中移除
  g_c620_instances[hc620->esc_id - 1] = NULL;
  g_c620_instance_count--;

  free(hc620);
  return DEVICE_OK;
}

Device_StatusTypeDef
C620_RegisterCallback(C620_HandleTypeDef *hc620,
                      const C620_CallbackTypeDef *callbacks) {
  DEVICE_ASSERT(hc620 != NULL);
  DEVICE_ASSERT(callbacks != NULL);

  memcpy(&hc620->callbacks, callbacks, sizeof(C620_CallbackTypeDef));
  return DEVICE_OK;
}

Device_StatusTypeDef C620_ExecuteCommand(C620_HandleTypeDef *hc620,
                                         C620_CmdIDTypeDef cmd_id, void *arg) {
  DEVICE_ASSERT(hc620 != NULL);
  DEVICE_ASSERT(cmd_id < C620_CMD_COUNT);

  const Command_TableTypeDef *cmd = &g_c620_cmd_table[cmd_id];

  // 检查参数
  if (cmd->arg_size > 0 && arg == NULL) {
    return DEVICE_INVALID_PARAM;
  }

  // 执行命令
  return cmd->handler(hc620, arg);
}

// ===================== 命令处理函数实现 =====================
static Device_StatusTypeDef C620_Cmd_SetCurrent(void *dev, void *arg) {
  C620_HandleTypeDef *hc620 = (C620_HandleTypeDef *)dev;
  int16_t current = *(int16_t *)arg;

  // 官方限幅：-16384~16384
  if (current > C620_MAX_CURRENT_RAW)
    current = C620_MAX_CURRENT_RAW;
  if (current < C620_MIN_CURRENT_RAW)
    current = C620_MIN_CURRENT_RAW;

  hc620->target_current = current;
  return DEVICE_OK;
}

static Device_StatusTypeDef C620_Cmd_EnableOutput(void *dev, void *arg) {
  C620_HandleTypeDef *hc620 = (C620_HandleTypeDef *)dev;
  // 只有待机或警告状态才能使能输出
  if (hc620->state == C620_STATE_STANDBY ||
      hc620->state == C620_STATE_WARNING) {
    hc620->enable_output = true;
  }
  return DEVICE_OK;
}

static Device_StatusTypeDef C620_Cmd_DisableOutput(void *dev, void *arg) {
  C620_HandleTypeDef *hc620 = (C620_HandleTypeDef *)dev;
  hc620->enable_output = false;
  hc620->target_current = 0;
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
  hc620->error.official = C620_ERROR_NONE;
  hc620->error.ext = C620_EXT_ERROR_NONE;
  hc620->warning = C620_WARNING_NONE;
  hc620->stall_start_time = 0;
  return DEVICE_OK;
}

// ===================== CAN接收回调函数 =====================
void C620_CAN_Rx_Callback(CAN_HandleTypeDef *hcan, uint32_t RxFifo) {
  CAN_RxHeaderTypeDef rx_header;
  uint8_t rx_data[8];

  if (HAL_CAN_GetRxMessage(hcan, RxFifo, &rx_header, rx_data) != HAL_OK) {
    return;
  }

  // 解析C620反馈ID（0x201~0x208对应ID1~8）
  uint8_t esc_id = rx_header.StdId - 0x200;
  if (esc_id < 1 || esc_id > C620_MAX_ESC_COUNT ||
      g_c620_instances[esc_id - 1] == NULL) {
    return;
  }

  C620_HandleTypeDef *hc620 = g_c620_instances[esc_id - 1];

  // 100%对齐官方协议第9页
  hc620->mechanical_angle = (uint16_t)(rx_data[0] << 8 | rx_data[1]);
  hc620->actual_speed = (int16_t)(rx_data[2] << 8 | rx_data[3]);
  hc620->actual_current = (int16_t)(rx_data[4] << 8 | rx_data[5]);
  hc620->actual_temp = rx_data[6];
  hc620->error.official = (C620_OfficialErrorTypeDef)rx_data[7];
  hc620->last_rx_time = HAL_GetTick();

  // 触发数据更新回调
  hc620->callbacks.on_data_update(hc620);

  // 通信恢复处理
  if (hc620->error.ext & C620_EXT_ERROR_COMM_TIMEOUT) {
    hc620->error.ext &= ~C620_EXT_ERROR_COMM_TIMEOUT;
    hc620->callbacks.on_comm_recover(hc620);
  }
}

// ===================== 全局CAN发送函数（1ms调用一次） =====================
Device_StatusTypeDef C620_Send_All_Commands(void) {
  if (g_c620_instance_count == 0) {
    return DEVICE_OK;
  }

  CAN_TxHeaderTypeDef tx_header;
  uint8_t tx_data[8] = {0};
  uint32_t tx_mailbox;

  // 配置CAN发送头（0x200控制ID1~4）
  tx_header.StdId = 0x200;
  tx_header.ExtId = 0;
  tx_header.RTR = CAN_RTR_DATA;
  tx_header.IDE = CAN_ID_STD;
  tx_header.DLC = 8;

  // 填充ID1~4的电流数据
  for (int i = 0; i < 4; i++) {
    if (g_c620_instances[i] == NULL) {
      tx_data[2 * i] = 0;
      tx_data[2 * i + 1] = 0;
      continue;
    }

    C620_HandleTypeDef *hc620 = g_c620_instances[i];

    // 错误/待机状态发送0电流
    if (hc620->state == C620_STATE_ERROR || hc620->state == C620_STATE_INIT ||
        !hc620->enable_output) {
      tx_data[2 * i] = 0;
      tx_data[2 * i + 1] = 0;
    } else {
      tx_data[2 * i] = (hc620->target_current >> 8) & 0xFF;
      tx_data[2 * i + 1] = hc620->target_current & 0xFF;
    }
  }

  // 发送ID1~4控制帧
  if (HAL_CAN_AddTxMessage(g_c620_instances[0]->hcan, &tx_header, tx_data,
                           &tx_mailbox) != HAL_OK) {
    return DEVICE_ERROR;
  }

  // 配置CAN发送头（0x1FF控制ID5~8）
  tx_header.StdId = 0x1FF;
  memset(tx_data, 0, 8);

  // 填充ID5~8的电流数据
  for (int i = 4; i < 8; i++) {
    if (g_c620_instances[i] == NULL) {
      tx_data[2 * (i - 4)] = 0;
      tx_data[2 * (i - 4) + 1] = 0;
      continue;
    }

    C620_HandleTypeDef *hc620 = g_c620_instances[i];

    if (hc620->state == C620_STATE_ERROR || hc620->state == C620_STATE_INIT ||
        !hc620->enable_output) {
      tx_data[2 * (i - 4)] = 0;
      tx_data[2 * (i - 4) + 1] = 0;
    } else {
      tx_data[2 * (i - 4)] = (hc620->target_current >> 8) & 0xFF;
      tx_data[2 * (i - 4) + 1] = hc620->target_current & 0xFF;
    }
  }

  // 发送ID5~8控制帧
  if (HAL_CAN_AddTxMessage(g_c620_instances[0]->hcan, &tx_header, tx_data,
                           &tx_mailbox) != HAL_OK) {
    return DEVICE_ERROR;
  }

  return DEVICE_OK;
}

// ===================== 核心状态机处理函数 =====================
Device_StatusTypeDef C620_Process_Loop(C620_HandleTypeDef *hc620) {
  DEVICE_ASSERT(hc620 != NULL);
  DEVICE_ASSERT(hc620->base.is_initialized);

  C620_StateTypeDef old_state = hc620->state;

  // ===================== 第一步：更新错误和警告状态 =====================
  // 1. 检查CAN通信超时
  if (is_time_A_after_B(HAL_GetTick(),
                        hc620->last_rx_time + C620_COMM_TIMEOUT_MS)) {
    hc620->error.ext |= C620_EXT_ERROR_COMM_TIMEOUT;
  }

  // 2. 检查过流（超过20A持续）
  if (abs(hc620->actual_current) > C620_MAX_CURRENT_RAW) {
    hc620->error.ext |= C620_EXT_ERROR_OVER_CURRENT;
  } else {
    hc620->error.ext &= ~C620_EXT_ERROR_OVER_CURRENT;
  }

  // 3. 检查堵转
  if (hc620->state == C620_STATE_RUNNING && hc620->enable_output) {
    if (abs(hc620->actual_current) > C620_STALL_CURRENT_RAW &&
        abs(hc620->actual_speed) < 10) {
      if (hc620->stall_start_time == 0) {
        hc620->stall_start_time = HAL_GetTick();
      } else if (is_time_A_after_B(HAL_GetTick(), hc620->stall_start_time +
                                                      C620_STALL_TIME_MS)) {
        hc620->error.ext |= C620_EXT_ERROR_STALL;
        hc620->callbacks.on_stall(hc620);
      }
    } else {
      hc620->stall_start_time = 0;
    }
  }

  // 4. 更新警告状态
  hc620->warning = C620_WARNING_NONE;
  if (hc620->error.official == C620_ERROR_OVER_TEMP_WARN) {
    hc620->warning |= C620_WARNING_OVER_TEMP;
  }

  // ===================== 第二步：状态机转换 =====================
  switch (hc620->state) {
  case C620_STATE_INIT:
    // 收到第一帧CAN反馈且无错误 → 进入待机
    if (hc620->last_rx_time != 0 && hc620->error.official == C620_ERROR_NONE &&
        hc620->error.ext == C620_EXT_ERROR_NONE) {
      hc620->state = C620_STATE_STANDBY;
    }
    // 初始化有官方错误 → 进入错误状态
    else if (hc620->error.official != C620_ERROR_NONE &&
             hc620->error.official != C620_ERROR_OVER_TEMP_WARN) {
      hc620->state = C620_STATE_ERROR;
      hc620->callbacks.on_official_error(hc620, hc620->error.official);
    }
    break;

  case C620_STATE_STANDBY:
    // 有错误 → 进入错误状态
    if ((hc620->error.official != C620_ERROR_NONE &&
         hc620->error.official != C620_ERROR_OVER_TEMP_WARN) ||
        hc620->error.ext != C620_EXT_ERROR_NONE) {
      hc620->state = C620_STATE_ERROR;
      hc620->enable_output = false;
      if (hc620->error.official != C620_ERROR_NONE) {
        hc620->callbacks.on_official_error(hc620, hc620->error.official);
      }
      if (hc620->error.ext != C620_EXT_ERROR_NONE) {
        hc620->callbacks.on_ext_error(hc620, hc620->error.ext);
      }
    }
    // 输出使能 → 进入运行状态
    else if (hc620->enable_output) {
      hc620->state = C620_STATE_RUNNING;
    }
    // 有警告 → 进入警告状态
    else if (hc620->warning != C620_WARNING_NONE) {
      hc620->state = C620_STATE_WARNING;
      hc620->callbacks.on_warning(hc620, hc620->warning);
    }
    break;

  case C620_STATE_RUNNING:
    // 有错误 → 进入错误状态
    if ((hc620->error.official != C620_ERROR_NONE &&
         hc620->error.official != C620_ERROR_OVER_TEMP_WARN) ||
        hc620->error.ext != C620_EXT_ERROR_NONE) {
      hc620->state = C620_STATE_ERROR;
      hc620->enable_output = false;
      if (hc620->error.official != C620_ERROR_NONE) {
        hc620->callbacks.on_official_error(hc620, hc620->error.official);
      }
      if (hc620->error.ext != C620_EXT_ERROR_NONE) {
        hc620->callbacks.on_ext_error(hc620, hc620->error.ext);
      }
    }
    // 输出禁用 → 回到待机
    else if (!hc620->enable_output) {
      hc620->state = C620_STATE_STANDBY;
    }
    // 有警告 → 进入警告状态（仍保持输出）
    else if (hc620->warning != C620_WARNING_NONE) {
      hc620->state = C620_STATE_WARNING;
      hc620->callbacks.on_warning(hc620, hc620->warning);
    }
    break;

  case C620_STATE_WARNING:
    // 有错误 → 进入错误状态
    if ((hc620->error.official != C620_ERROR_NONE &&
         hc620->error.official != C620_ERROR_OVER_TEMP_WARN) ||
        hc620->error.ext != C620_EXT_ERROR_NONE) {
      hc620->state = C620_STATE_ERROR;
      hc620->enable_output = false;
      if (hc620->error.official != C620_ERROR_NONE) {
        hc620->callbacks.on_official_error(hc620, hc620->error.official);
      }
      if (hc620->error.ext != C620_EXT_ERROR_NONE) {
        hc620->callbacks.on_ext_error(hc620, hc620->error.ext);
      }
    }
    // 警告消失 → 回到运行/待机
    else if (hc620->warning == C620_WARNING_NONE) {
      hc620->state =
          hc620->enable_output ? C620_STATE_RUNNING : C620_STATE_STANDBY;
    }
    break;

  case C620_STATE_ERROR:
    // 所有错误清除 → 回到待机
    if ((hc620->error.official == C620_ERROR_NONE ||
         hc620->error.official == C620_ERROR_OVER_TEMP_WARN) &&
        hc620->error.ext == C620_EXT_ERROR_NONE) {
      hc620->state = C620_STATE_STANDBY;
      hc620->callbacks.on_comm_recover(hc620);
    }
    break;

  case C620_STATE_CALIBRATING:
  case C620_STATE_ID_SETTING:
    // 校准和ID设置由电调硬件自动处理，MCU仅监控状态
    break;
  }

  // ===================== 第三步：状态变化回调 =====================
  if (hc620->state != old_state) {
    hc620->callbacks.on_state_change(hc620, old_state, hc620->state);
  }

  return DEVICE_OK;
}