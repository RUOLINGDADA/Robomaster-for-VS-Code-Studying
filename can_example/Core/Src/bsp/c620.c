#include "c620.h"
#include <string.h>

#define C620_FRAME_DLC 8U
#define C620_LOW_GROUP_MAX_ID 4U
#define C620_HIGH_GROUP_MIN_ID 5U

// 静态内存池的一个槽位。
// 嵌入式项目常避免 malloc/free：堆碎片、失败时机、实时性都不可控。
// 这里预留最多 8 个 C620 对象，创建时从池里拿，销毁时还回池里。
// 这种全局 static 对象通常位于 .bss 或 .data：
//   - 初值全 0 的进 .bss，启动文件在进入 main 前统一清零；
//   - 非 0 初值的进 .data，启动文件从 Flash 拷贝初值到 RAM。
// 所以静态池的分配时间和内存占用在链接阶段就确定了，实时性可预测。
typedef struct {
  C620_HandleTypeDef handle;
  bool in_use;
} C620_InstanceSlotTypeDef;

// CAN接收路径和主循环之间的事件快照。
// volatile 标志在中断里置位，主循环一次性“取出并清空”，
// 这样回调不会在中断里执行，也不会漏掉通信恢复这类边沿事件。
typedef struct {
  bool data_updated;
  bool comm_recovered;
} C620_EventSnapshotTypeDef;

// 全局电调实例表（最多支持8个C620电调）
// 注意这里存的是指针，不是对象本体；对象可能来自静态池，也可能来自调用方静态分配。
// 查找时使用 hcan + esc_id，而不是只用 esc_id，给多CAN总线扩展留下空间。
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
// static 限制符号只在本 c 文件可见，避免和其它模块重名。
// const 表示表内容运行期不改，编译器通常会把它放进 Flash/.rodata。
// “static const Command_TableTypeDef *”读法：函数返回一个指针，
// 指向 const 的命令表项；调用者可以换指针，但不能改表项内容。
// 注意：表里存了函数地址。链接器最终会把 C620_Cmd_xxx 的入口地址填进这里。
// MCU 执行 handler 时，本质是通过函数指针跳转到对应 Flash 代码地址。
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
static bool C620_IsValidId(uint8_t esc_id);
static bool C620_IsInitialized(const C620_HandleTypeDef *hc620);
static bool C620_IsFatalOfficialError(C620_OfficialErrorTypeDef error);
static bool C620_IsHealthyForOutput(const C620_HandleTypeDef *hc620);
static void C620_FailSafeStop(C620_HandleTypeDef *hc620,
                              C620_StateTypeDef state);
static void C620_SetState(C620_HandleTypeDef *hc620,
                          C620_StateTypeDef new_state);
static Device_StatusTypeDef C620_DeInitFromBase(DeviceBase *dev);
static uint8_t C620_GetControlSlot(uint8_t esc_id);
static uint16_t C620_ReadU16Be(const uint8_t *data);
static int16_t C620_ReadI16Be(const uint8_t *data);
static void C620_WriteI16Be(uint8_t *data, int16_t value);
static uint32_t C620_EnterCritical(void);
static void C620_ExitCritical(uint32_t primask);
static bool C620_IsRegistered(const C620_HandleTypeDef *hc620);
static void C620_SetExtError(C620_HandleTypeDef *hc620,
                             C620_ExtErrorTypeDef error);
static void C620_ClearExtError(C620_HandleTypeDef *hc620,
                               C620_ExtErrorTypeDef error);
static bool C620_HasExtError(const C620_HandleTypeDef *hc620,
                             C620_ExtErrorTypeDef error);
static void C620_SetWarning(C620_HandleTypeDef *hc620,
                            C620_WarningTypeDef warning);
static void C620_ClearWarning(C620_HandleTypeDef *hc620,
                              C620_WarningTypeDef warning);
static bool C620_HasWarning(const C620_HandleTypeDef *hc620,
                            C620_WarningTypeDef warning);
static C620_HandleTypeDef *C620_FindInstance(CAN_HandleTypeDef *hcan,
                                             uint8_t esc_id);
static bool C620_RegisterInstance(C620_HandleTypeDef *hc620);
static void C620_UnregisterInstance(C620_HandleTypeDef *hc620);
static void C620_PopEvents(C620_HandleTypeDef *hc620,
                           C620_EventSnapshotTypeDef *events);
static bool C620_BusAlreadySent(CAN_HandleTypeDef *hcan, uint8_t end_index);
static Device_StatusTypeDef C620_SendCommandsOnBus(CAN_HandleTypeDef *hcan);
static Device_StatusTypeDef C620_SendCommandFrame(CAN_HandleTypeDef *hcan,
                                                  uint32_t std_id,
                                                  const uint8_t data[8]);

// 默认空回调函数（防止空指针调用）
// 这是驱动里常用的 Null Object 思路：即使用户没注册回调，
// 驱动也能直接调用函数指针，不需要每次 if (callback != NULL)。
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
  // sizeof(array) / sizeof(array[0]) 在编译期求出元素个数。
  // 这比手写命令数量安全：表增删时循环边界自动同步。
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
  // int16_t 的范围是 -32768~32767，直接对 INT16_MIN 取负会溢出。
  // 嵌入式里整数溢出经常是隐蔽 bug，所以先单独夹住边界值。
  // C 里有符号溢出是未定义行为，不只是“回绕一下”；
  // 优化器可能基于“不会溢出”的假设重排代码，所以边界保护必须显式写。
  if (value == INT16_MIN) {
    return INT16_MAX;
  }
  return (value < 0) ? (int16_t)-value : value;
}

static bool C620_IsValidId(uint8_t esc_id) {
  return esc_id >= 1U && esc_id <= C620_MAX_ESC_COUNT;
}

static bool C620_IsInitialized(const C620_HandleTypeDef *hc620) {
  return hc620 != NULL && hc620->base.is_initialized;
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

static bool C620_IsHealthyForOutput(const C620_HandleTypeDef *hc620) {
  return hc620->error.official == C620_ERROR_NONE &&
         hc620->error.ext == C620_EXT_ERROR_NONE &&
         hc620->warning == C620_WARNING_NONE;
}

static void C620_FailSafeStop(C620_HandleTypeDef *hc620,
                              C620_StateTypeDef state) {
  // Fail-safe 原则：一旦进入危险状态，先把输出命令清零并关闭使能。
  // 即使后续某次发送函数继续运行，也只会发出 0 电流。
  hc620->target_current = 0;
  hc620->enable_output = false;
  hc620->state = state;
}

static Device_StatusTypeDef C620_DeInitFromBase(DeviceBase *dev) {
  if (dev == NULL) {
    return DEVICE_NO_PARAMETER;
  }

  // DeviceBase 是 C620_HandleTypeDef 的第一个成员，所以两个地址相同。
  // 这就是 C 里模拟“父类指针调用子类析构”的关键约定。
  return C620_Destroy((C620_HandleTypeDef *)dev);
}

static uint8_t C620_GetControlSlot(uint8_t esc_id) {
  // 官方控制帧每个电调用 2 字节：
  // ID1~4 在 0x200 的 slot0~3，ID5~8 在 0x1FF 的 slot0~3。
  // 你之前遇到“宏写2却像ID1”的问题，本质就是对象ID和帧slot映射必须分清。
  if (esc_id <= C620_LOW_GROUP_MAX_ID) {
    return (uint8_t)(esc_id - 1U);
  }

  return (uint8_t)(esc_id - C620_HIGH_GROUP_MIN_ID);
}

static uint16_t C620_ReadU16Be(const uint8_t *data) {
  // C620 CAN协议采用高字节在前(big-endian)。
  // STM32本身是 little-endian，但总线协议字节序和CPU字节序是两回事，
  // 所以不要强转 uint16_t*，要显式按协议拼字节。
  // 这样也避开两个底层坑：
  // 1. 未对齐访问：data 只是字节数组，不保证能按 uint16_t 对齐；
  // 2. 严格别名：用 uint16_t* 读取 uint8_t[] 可能违反 C aliasing 规则。
  // 先把 data[0] 转 uint16_t 再左移，是为了避免整数提升后符号/宽度不清晰。
  return (uint16_t)(((uint16_t)data[0] << 8) | data[1]);
}

static int16_t C620_ReadI16Be(const uint8_t *data) {
  return (int16_t)C620_ReadU16Be(data);
}

static void C620_WriteI16Be(uint8_t *data, int16_t value) {
  // 发送电流也是高字节在前。先转成 uint16_t 是为了明确保留二进制补码位型，
  // 例如 -1000 在总线上要按 16bit 原始值发送。
  // C 标准层面 int16_t 到 uint16_t 的转换按 2^16 取模；
  // 在实际 ARM GCC/Clang 上就是保留补码位模式，适合这种“协议原始值”发送。
  const uint16_t raw = (uint16_t)value;
  data[0] = (uint8_t)(raw >> 8);
  data[1] = (uint8_t)raw;
}

static uint32_t C620_EnterCritical(void) {
  // PRIMASK 是 Cortex-M 的全局中断屏蔽位。
  // 进入临界区前先保存原状态，退出时只在原来允许中断的情况下重新打开。
  // 这样函数嵌套在别的临界区里时，不会错误地把外层关掉的中断打开。
  // __disable_irq() 最终对应 CPSID i 指令，屏蔽可屏蔽中断；
  // 它不是互斥锁，也不适合包住耗时操作，只适合保护几个内存标志位。
  const uint32_t primask = __get_PRIMASK();
  __disable_irq();
  return primask;
}

static void C620_ExitCritical(uint32_t primask) {
  if (primask == 0U) {
    __enable_irq();
  }
}

static bool C620_IsRegistered(const C620_HandleTypeDef *hc620) {
  for (uint8_t i = 0U; i < C620_MAX_ESC_COUNT; i++) {
    if (g_c620_instances[i] == hc620) {
      return true;
    }
  }

  return false;
}

static void C620_SetExtError(C620_HandleTypeDef *hc620,
                             C620_ExtErrorTypeDef error) {
  // ext 是 bit mask：用 | 置位，不会影响其它已经存在的错误。
  // enum 在 C 里不是强类型位集，所以这里显式 cast 回枚举类型，
  // 让“这是位组合后的 C620_ExtErrorTypeDef”表达更清楚。
  hc620->error.ext = (C620_ExtErrorTypeDef)(hc620->error.ext | error);
}

static void C620_ClearExtError(C620_HandleTypeDef *hc620,
                               C620_ExtErrorTypeDef error) {
  // 用 &~ 清除指定 bit，不要直接赋 0，否则会误清其它错误来源。
  hc620->error.ext = (C620_ExtErrorTypeDef)(hc620->error.ext & ~error);
}

static bool C620_HasExtError(const C620_HandleTypeDef *hc620,
                             C620_ExtErrorTypeDef error) {
  return (hc620->error.ext & error) != 0U;
}

static void C620_SetWarning(C620_HandleTypeDef *hc620,
                            C620_WarningTypeDef warning) {
  // warning 也是 bit mask。首次出现时才回调，避免主循环每周期刷屏。
  if ((hc620->warning & warning) == 0U) {
    hc620->warning = (C620_WarningTypeDef)(hc620->warning | warning);
    hc620->callbacks.on_warning(hc620, warning);
  }
}

static void C620_ClearWarning(C620_HandleTypeDef *hc620,
                              C620_WarningTypeDef warning) {
  hc620->warning = (C620_WarningTypeDef)(hc620->warning & ~warning);
}

static bool C620_HasWarning(const C620_HandleTypeDef *hc620,
                            C620_WarningTypeDef warning) {
  return (hc620->warning & warning) != 0U;
}

static C620_HandleTypeDef *C620_FindInstance(CAN_HandleTypeDef *hcan,
                                             uint8_t esc_id) {
  // 反馈帧只告诉我们“这个 CAN 句柄上收到了某个 ESC ID”。
  // 所以实例查找必须同时匹配总线和ID，不能只看ID。
  for (uint8_t i = 0U; i < C620_MAX_ESC_COUNT; i++) {
    C620_HandleTypeDef *hc620 = g_c620_instances[i];
    if (hc620 != NULL && hc620->hcan == hcan && hc620->esc_id == esc_id) {
      return hc620;
    }
  }

  return NULL;
}

static bool C620_RegisterInstance(C620_HandleTypeDef *hc620) {
  // 注册表负责保证同一条 CAN 总线上不会注册两个相同 ESC ID。
  // 如果两个对象抢同一个反馈帧，状态机会变得不可预测。
  if (C620_FindInstance(hc620->hcan, hc620->esc_id) != NULL) {
    return false;
  }

  for (uint8_t i = 0U; i < C620_MAX_ESC_COUNT; i++) {
    if (g_c620_instances[i] == NULL) {
      g_c620_instances[i] = hc620;
      g_c620_instance_count++;
      return true;
    }
  }

  return false;
}

static void C620_UnregisterInstance(C620_HandleTypeDef *hc620) {
  for (uint8_t i = 0U; i < C620_MAX_ESC_COUNT; i++) {
    if (g_c620_instances[i] == hc620) {
      g_c620_instances[i] = NULL;
      if (g_c620_instance_count > 0U) {
        g_c620_instance_count--;
      }
      return;
    }
  }
}

static void C620_PopEvents(C620_HandleTypeDef *hc620,
                           C620_EventSnapshotTypeDef *events) {
  // “取出并清空”必须是原子操作：
  // 如果主循环刚读到 data_updated=false，中断立刻置 true，
  // 然后主循环又清 false，就会丢事件。短临界区可以避免这个竞态。
  const uint32_t primask = C620_EnterCritical();
  events->data_updated = hc620->data_updated;
  events->comm_recovered = hc620->comm_recovered;
  hc620->data_updated = false;
  hc620->comm_recovered = false;
  C620_ExitCritical(primask);
}

static bool C620_BusAlreadySent(CAN_HandleTypeDef *hcan, uint8_t end_index) {
  for (uint8_t i = 0U; i < end_index; i++) {
    if (g_c620_instances[i] != NULL && g_c620_instances[i]->hcan == hcan) {
      return true;
    }
  }

  return false;
}

static Device_StatusTypeDef C620_SendCommandsOnBus(CAN_HandleTypeDef *hcan) {
  // C620 不是“一个电调发一帧”，而是官方规定：
  // ID1~4 合并到 StdId 0x200，ID5~8 合并到 StdId 0x1FF。
  // 未启用或故障的电调槽位保持 0，相当于发送停止电流。
  // 8字节布局是纯协议层概念：
  //   byte0-1 -> ID1/5, byte2-3 -> ID2/6,
  //   byte4-5 -> ID3/7, byte6-7 -> ID4/8。
  // 所以 data 数组每次从 0 初始化，能保证未配置槽位不会残留上一次电流。
  uint8_t low_data[8] = {0};
  uint8_t high_data[8] = {0};
  bool has_low = false;
  bool has_high = false;

  for (uint8_t i = 0U; i < C620_MAX_ESC_COUNT; i++) {
    C620_HandleTypeDef *hc620 = g_c620_instances[i];
    if (hc620 == NULL || hc620->hcan != hcan) {
      continue;
    }

    const bool can_output = hc620->enable_output &&
                            hc620->state != C620_STATE_ERROR &&
                            hc620->state != C620_STATE_STOP &&
                            hc620->state != C620_STATE_STALL;
    // slot 决定写入 data[0:1]、data[2:3]、data[4:5] 还是 data[6:7]。
    // 这里不要把“反馈ID 0x201”和“控制帧槽位0”混为一谈。
    const uint8_t slot = C620_GetControlSlot(hc620->esc_id);
    uint8_t *data = (hc620->esc_id <= C620_LOW_GROUP_MAX_ID) ? low_data
                                                             : high_data;
    if (hc620->esc_id <= C620_LOW_GROUP_MAX_ID) {
      has_low = true;
    } else {
      has_high = true;
    }

    if (can_output) {
      C620_WriteI16Be(&data[2U * slot], hc620->target_current);
    }
  }

  if (has_low) {
    Device_StatusTypeDef status =
        C620_SendCommandFrame(hcan, C620_CAN_CTRL_ID_BASE, low_data);
    if (status != DEVICE_OK) {
      return status;
    }
  }

  if (has_high) {
    Device_StatusTypeDef status =
        C620_SendCommandFrame(hcan, C620_CAN_CTRL_ID_EXT, high_data);
    if (status != DEVICE_OK) {
      return status;
    }
  }

  return DEVICE_OK;
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

  // STM32 bxCAN 有发送邮箱。邮箱满时不要阻塞死等，返回 BUSY 让上层下周期重试。
  // HAL_CAN_AddTxMessage 最终会配置 CAN 外设的 Tx mailbox 寄存器：
  // StdId/IDE/RTR/DLC 写入帧头，data[0..7] 写入数据寄存器，然后请求发送。
  // 在控制周期里死等邮箱会拉长主循环抖动，所以这里选择显式返回 BUSY。
  if (HAL_CAN_GetTxMailboxesFreeLevel(hcan) == 0U) {
    return DEVICE_BUSY;
  }

  tx_header.StdId = std_id;
  tx_header.RTR = CAN_RTR_DATA;
  tx_header.IDE = CAN_ID_STD;
  tx_header.DLC = C620_FRAME_DLC;
  // CAN 标准帧 ID 是 11 bit。0x200/0x1FF 都在标准帧范围内；
  // IDE=CAN_ID_STD 必须和滤波器/电调协议一致，否则电调不会识别。

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
      // 从静态池拿对象，再走统一 Init 流程。
      // 这样 Create 和用户自备静态句柄的 C620_Init 共享同一套校验逻辑。
      // in_use 是池管理元数据，不属于 C620 协议状态；
      // handle 本体会在 C620_Init 里 memset，保证每次创建都是干净对象。
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

  if (!C620_IsValidId(config->esc_id)) {
    return DEVICE_INVALID_PARAM;
  }

  if (C620_IsRegistered(hc620)) {
    return DEVICE_BUSY;
  }

  if (g_c620_instance_count >= C620_MAX_ESC_COUNT ||
      C620_FindInstance(config->hcan, config->esc_id) != NULL) {
    return DEVICE_BUSY;
  }

  // 清零句柄：把上一次使用留下的状态、指针、标志全部归零。
  // 注意这只能在确认该 handle 未注册时做，否则会破坏正在运行的对象。
  // memset 对普通 POD 风格结构体是可接受的；本结构没有 C++ 对象、锁或动态资源。
  // 对含硬件句柄指针的对象，清零后必须重新填入 hcan/esc_id 等不变配置。
  memset(hc620, 0, sizeof(C620_HandleTypeDef));

  // 初始化基类：这些字段是所有设备都共有的“父类属性”。
  hc620->base.name = config->name;
  hc620->base.device_id = config->esc_id;
  hc620->base.is_initialized = true;
  hc620->base.deinit = C620_DeInitFromBase;

  // 初始化硬件相关：这里保存的是 HAL CAN 句柄指针，不复制外设对象。
  // 外设生命周期由 CubeMX/HAL 管理，驱动只引用它。
  // CAN_HandleTypeDef 内部包含外设寄存器基地址、状态、锁、错误码等 HAL 上下文；
  // 驱动保存指针，是为了所有模块都操作同一个 HAL 上下文。
  hc620->hcan = config->hcan;
  hc620->esc_id = config->esc_id;

  // 初始化状态：上电后默认 STOP，不主动输出电流。
  // 工业/比赛机器人代码都应默认安全，必须显式 Enable 后才运行。
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

  if (!C620_RegisterInstance(hc620)) {
    // 理论上前面已经检查过，这里是防御式编程：
    // 如果注册失败，回滚 handle，避免半初始化对象被误用。
    memset(hc620, 0, sizeof(C620_HandleTypeDef));
    return DEVICE_BUSY;
  }

  return DEVICE_OK;
}

Device_StatusTypeDef C620_SetFeedbackTimeoutEnable(C620_HandleTypeDef *hc620,
                                                   bool enable) {
  if (hc620 == NULL) {
    return DEVICE_NO_PARAMETER;
  }
  if (!C620_IsInitialized(hc620)) {
    return DEVICE_NOT_INITIALIZED;
  }

  hc620->feedback_timeout_enable = enable;
  if (!enable) {
    C620_ClearExtError(hc620, C620_EXT_ERROR_COMM_TIMEOUT);
    hc620->last_rx_time = HAL_GetTick();
  }

  return DEVICE_OK;
}

Device_StatusTypeDef C620_Destroy(C620_HandleTypeDef *hc620) {
  if (hc620 == NULL) {
    return DEVICE_NO_PARAMETER;
  }

  C620_UnregisterInstance(hc620);

  for (uint32_t i = 0U; i < C620_MAX_ESC_COUNT; i++) {
    if (&g_c620_static_pool[i].handle == hc620) {
      // 如果对象来自内部静态池，释放的是“池槽位占用标志”，不是 free 内存。
      g_c620_static_pool[i].in_use = false;
      break;
    }
  }

  // 清零后 base.is_initialized 也会变 false，后续 API 会返回 NOT_INITIALIZED。
  memset(hc620, 0, sizeof(*hc620));
  return DEVICE_OK;
}

Device_StatusTypeDef
C620_RegisterCallback(C620_HandleTypeDef *hc620,
                      const C620_CallbackTypeDef *callbacks) {
  if (hc620 == NULL || callbacks == NULL) {
    return DEVICE_NO_PARAMETER;
  }
  if (!C620_IsInitialized(hc620)) {
    return DEVICE_NOT_INITIALIZED;
  }

  C620_Init_DefaultCallbacks(&hc620->callbacks);
  // 用户只需要填自己关心的回调，没填的继续使用默认空函数。
  // 这种写法比要求用户完整填满函数表更不容易出错。
  // 函数指针赋值只保存代码入口地址，不复制函数本体；
  // 所以回调函数必须是静态/全局函数或生命周期足够长的函数入口。
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
  if (!C620_IsInitialized(hc620)) {
    return DEVICE_NOT_INITIALIZED;
  }

  const Command_TableTypeDef *cmd = C620_FindCommand(cmd_id);
  if (cmd == NULL) {
    return DEVICE_INVALID_PARAM;
  }

  // 检查参数大小：命令表里 arg_size > 0 表示该命令必须带参数。
  // 这里不检查具体类型，类型约定由命令ID决定，例如 SET_CURRENT 要 int16_t*。
  // void * 带来灵活性，也牺牲编译期类型检查。
  // 工业项目里通常用文档、单元测试或更强类型的 wrapper API 来约束调用。
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

  // 指令限幅：上层即使传错过大的值，也不会让驱动发出超说明书范围的电流。
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
  hc620->over_current_start_time = 0;
  C620_SetState(hc620, C620_STATE_STOP);
  return DEVICE_OK;
}

static Device_StatusTypeDef C620_Cmd_DisableOutput(void *dev, void *arg) {
  C620_HandleTypeDef *hc620 = (C620_HandleTypeDef *)dev;
  (void)arg;
  hc620->target_current = 0;
  hc620->enable_output = false;
  hc620->stall_start_time = 0;
  hc620->over_current_start_time = 0;
  C620_SetState(hc620, C620_STATE_STOP);
  return DEVICE_OK;
}

static Device_StatusTypeDef C620_Cmd_EnableOutput(void *dev, void *arg) {
  C620_HandleTypeDef *hc620 = (C620_HandleTypeDef *)dev;
  (void)arg;
  if (!C620_IsHealthyForOutput(hc620)) {
    return DEVICE_ERROR;
  }

  hc620->enable_output = true;
  C620_SetState(hc620, C620_STATE_RUNNING);
  return DEVICE_OK;
}

// ===================== CAN接收回调函数 =====================
bool C620_CAN_Rx_Callback(CAN_HandleTypeDef *hcan, uint32_t RxFifo) {
  CAN_RxHeaderTypeDef rx_header;
  uint8_t rx_data[8];

  // HAL_CAN_GetRxMessage 会从 CAN FIFO 寄存器取出一帧。
  // 读取动作通常会释放 FIFO 中该邮箱，所以必须一次把 header/data 都取走。
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
    // 只接受标准帧、数据帧、8字节数据。协议入口越早过滤，后面越安全。
    return false;
  }

  if (rx_header->StdId <= C620_CAN_FEEDBACK_ID_BASE ||
      rx_header->StdId > (C620_CAN_FEEDBACK_ID_BASE + C620_MAX_ESC_COUNT)) {
    return false;
  }

  // 解析C620反馈数据（反馈帧ID = 0x200 + ESC_ID, 即0x201~0x208）
  uint8_t esc_id = (uint8_t)(rx_header->StdId - C620_CAN_FEEDBACK_ID_BASE);
  if (!C620_IsValidId(esc_id)) {
    return false;
  }

  C620_HandleTypeDef *hc620 = C620_FindInstance(hcan, esc_id);
  if (hc620 == NULL) {
    return false;
  }

  // 更新反馈数据（C620 CAN协议：字节0-1=机械角度, 2-3=转速, 4-5=转矩电流, 6=温度, 7=错误码）
  // 这些字段是最新一次反馈，不在 ISR 里做复杂控制，保持接收路径短而确定。
  // 这里先更新数据字段，再置 data_updated 标志；主循环看到标志时，
  // 至少能读到一组已经写入过的最新字段。
  hc620->last_mechanical_angle = hc620->mechanical_angle;
  hc620->mechanical_angle = C620_ReadU16Be(&data[0]);
  hc620->actual_speed = C620_ReadI16Be(&data[2]);
  hc620->actual_current = C620_ReadI16Be(&data[4]);
  hc620->actual_temp = data[6];
  hc620->error.official = (C620_OfficialErrorTypeDef)data[7];
  hc620->last_rx_time = HAL_GetTick();

  const uint32_t primask = C620_EnterCritical();
  // 标志位和 error.ext 可能同时被主循环读改写，所以放进临界区。
  // 反馈数据字段本身没有全部放临界区，是为了缩短中断关闭时间；
  // 如果后续做高精度闭环，可增加 C620_GetFeedbackSnapshot() 统一拷贝。
  hc620->data_updated = true;
  if (C620_HasExtError(hc620, C620_EXT_ERROR_COMM_TIMEOUT)) {
    // 收到有效反馈说明通信恢复。这里只置恢复事件，真正回调放到主循环。
    C620_ClearExtError(hc620, C620_EXT_ERROR_COMM_TIMEOUT);
    hc620->comm_recovered = true;
  }
  C620_ExitCritical(primask);

  return true;
}

// ===================== 全局CAN聚合发送函数 =====================
Device_StatusTypeDef C620_Send_All_Commands(void) {
  // 这个函数适合放在 1~2ms 周期任务里调用。
  // 它会按 CAN 总线聚合发送，避免同一条总线重复发 0x200/0x1FF。
  // 周期发送而不是“设置一次发一次”，是因为 C620 电流控制更接近实时控制流：
  // 上层每周期刷新目标值，驱动每周期把当前目标值投递到 CAN 总线。
  for (uint8_t i = 0U; i < C620_MAX_ESC_COUNT; i++) {
    C620_HandleTypeDef *hc620 = g_c620_instances[i];
    if (hc620 == NULL || C620_BusAlreadySent(hc620->hcan, i)) {
      continue;
    }

    Device_StatusTypeDef status = C620_SendCommandsOnBus(hc620->hcan);
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
  if (!C620_IsInitialized(hc620)) {
    return DEVICE_NOT_INITIALIZED;
  }

  const uint32_t now = HAL_GetTick();
  // 本轮状态机只读取一次 now，保证所有超时判断基于同一个时间截面。
  // 这能减少边界抖动，也方便调试复现。
  C620_StateTypeDef old_state = hc620->state;
  C620_EventSnapshotTypeDef events;
  C620_PopEvents(hc620, &events);

  // 主循环消费事件：回调里可以打印、上报、改变应用状态。
  // 这些动作不适合放在 CAN 接收中断里。
  if (events.data_updated) {
    hc620->callbacks.on_data_update(hc620);
  }

  if (events.comm_recovered) {
    hc620->callbacks.on_comm_recover(hc620);
    if (hc620->enable_output &&
        !C620_IsFatalOfficialError(hc620->error.official) &&
        hc620->error.ext == C620_EXT_ERROR_NONE) {
      hc620->state = C620_STATE_RUNNING;
    }
  }

  // 1. 检查通信超时
  if (hc620->feedback_timeout_enable &&
      is_time_A_after_B(now, hc620->last_rx_time + C620_COMM_TIMEOUT_MS)) {
    // 超时是 MCU 侧扩展错误，不是电调官方 DATA[7] 错误。
    // 但安全动作一样：清目标电流、关闭输出、进入 ERROR。
    C620_SetExtError(hc620, C620_EXT_ERROR_COMM_TIMEOUT);
    C620_FailSafeStop(hc620, C620_STATE_ERROR);
  }

  // 2. 检查电调官方错误码
  if (hc620->error.official == C620_ERROR_OVER_TEMP_WARN) {
    // DATA[7] 是电调官方错误码。官方过温警告可按配置选择降级或直接关断。
    C620_SetWarning(hc620, C620_WARNING_OVER_TEMP);
#if C620_TEMP_WARN_SHUTDOWN_ENABLE
    // 这里把“官方警告”升级成 MCU 侧错误，是一种保守安全策略。
    // 如果比赛策略允许降额运行，可以把 C620_TEMP_WARN_SHUTDOWN_ENABLE 设为 0。
    C620_SetExtError(hc620, C620_EXT_ERROR_OVER_TEMP);
    C620_FailSafeStop(hc620, C620_STATE_ERROR);
#else
    if (hc620->state == C620_STATE_RUNNING) {
      hc620->state = C620_STATE_WARNING;
    }
#endif
  } else if (C620_IsFatalOfficialError(hc620->error.official)) {
    C620_FailSafeStop(hc620, C620_STATE_ERROR);
  } else if (C620_HasWarning(hc620, C620_WARNING_OVER_TEMP) &&
             hc620->actual_temp < C620_OVER_TEMP_WARN) {
    C620_ClearWarning(hc620, C620_WARNING_OVER_TEMP);
    C620_ClearExtError(hc620, C620_EXT_ERROR_OVER_TEMP);
    if (hc620->enable_output && hc620->error.ext == C620_EXT_ERROR_NONE) {
      hc620->state = C620_STATE_RUNNING;
    }
  }

  // 3. 检查过流
  if (C620_AbsI16(hc620->actual_current) >= C620_MAX_CURRENT_RAW) {
    // 过流用“持续时间”判定：单次采样可能是瞬态、噪声或制动尖峰。
    if (hc620->over_current_start_time == 0U) {
      hc620->over_current_start_time = now;
    } else if (is_time_A_after_B(
                   now, hc620->over_current_start_time +
                            C620_OVER_CURRENT_TIME_MS)) {
      C620_SetExtError(hc620, C620_EXT_ERROR_OVER_CURRENT);
      C620_FailSafeStop(hc620, C620_STATE_ERROR);
    }
  } else {
    hc620->over_current_start_time = 0U;
  }

  // 4. 检查过热
  if (hc620->actual_temp >= C620_OVER_TEMP_WARN) {
    // 温度也由 MCU 再检查一遍：即使 DATA[7] 没及时置错误码，
    // 实际温度字段达到阈值也会触发保护。这是冗余安全判断。
    C620_SetWarning(hc620, C620_WARNING_OVER_TEMP);
#if C620_TEMP_WARN_SHUTDOWN_ENABLE
    C620_SetExtError(hc620, C620_EXT_ERROR_OVER_TEMP);
    C620_FailSafeStop(hc620, C620_STATE_ERROR);
#else
    if (hc620->state == C620_STATE_RUNNING) {
      hc620->state = C620_STATE_WARNING;
    }
#endif
  }
  if (hc620->actual_temp >= C620_OVER_TEMP_CRIT) {
    hc620->error.official = C620_ERROR_OVER_TEMP_CRIT;
    C620_SetExtError(hc620, C620_EXT_ERROR_OVER_TEMP);
    C620_FailSafeStop(hc620, C620_STATE_ERROR);
  }

  // 5. 检查堵转
  if (hc620->state == C620_STATE_RUNNING) {
    // 堵转的典型特征：电流很大但速度接近 0，并且持续一段时间。
    // 只在 RUNNING 状态检查，停止状态下速度为0是正常现象。
    if (C620_AbsI16(hc620->actual_current) > C620_STALL_CURRENT_RAW &&
        C620_AbsI16(hc620->actual_speed) < 10) {

      if (hc620->stall_start_time == 0U) {
        hc620->stall_start_time = now;
      } else if (is_time_A_after_B(now, hc620->stall_start_time +
                                            C620_STALL_TIME_MS)) {
        C620_SetExtError(hc620, C620_EXT_ERROR_STALL);
        C620_FailSafeStop(hc620, C620_STATE_STALL);
        hc620->callbacks.on_stall(hc620);
      }
    } else {
      hc620->stall_start_time = 0;
    }
  }

  if (hc620->state == C620_STATE_ERROR || hc620->state == C620_STATE_STALL) {
    // 最后一层保险：只要状态机落入 ERROR/STALL，输出使能必定关闭。
    // 这样即使上层忘记调用 DisableOutput，发送聚合帧也会把该槽位保持 0。
    hc620->enable_output = false;
  }

  // 6. 状态变化处理
  if (hc620->state != old_state) {
    // 状态变化统一在函数末尾回调，避免同一次 Process_Loop 里多次通知。
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
