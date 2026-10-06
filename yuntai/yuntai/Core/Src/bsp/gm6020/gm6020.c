/**
 * @file gm6020.c
 * @brief GM6020 CAN 电流、连续角度与只读边界接口。
 *
 * 协议来自 GM6020 手册：反馈 0x205~0x20B，电流控制 0x1FE/0x2FE。
 * 0x1FF/0x2FF 是电压控制帧。CAN ISR 解码反馈，任务读一致快照并提交电流。
 * 8192 count/rev 的角度展开要求相邻帧位移小于半圈；丢帧后不能猜测整圈数。
 * 本模块不启动 CAN，不运行闭环，不过滤输入或锁存堵转。
 */

#include "bsp/gm6020/gm6020.h"

#include <string.h>

#if defined(HAL_CAN_MODULE_ENABLED)

/* Yaw 与 Pitch 共享同一组电流控制帧。不同反馈 ID 对应不同的两字节槽位。两个任务不能各发一帧覆盖对方。 */
static Gm6020_HandleTypeDef *g_gm6020_handles[GM6020_MAX_DEVICE_COUNT]; /* 任务注册、ISR 查找；句柄须持久存储，注册变更用 PRIMASK 保护。 */

/*
 * 单核 STM32 上用很短的 PRIMASK 临界区保护驱动状态及聚合发送。避免 ISR 与任务同时改槽位。
 * 保存旧 PRIMASK 后恢复原状态，不能无条件开中断。临界区内不等待、不打印。
 * CAN 发送仅提交最多两帧到邮箱，没有总线完成等待。
 */
/* 保存并屏蔽中断，保护注册表、反馈状态和聚合帧的短临界区。 */
static uint32_t Gm6020_EnterCritical(void) {
  const uint32_t previous = __get_PRIMASK();
  __disable_irq();
  __DMB();
  return previous;
}

/* 恢复进入临界区前的中断状态。无条件开中断会破坏调用者原本屏蔽 ISR 的上下文。 */
static void Gm6020_ExitCritical(uint32_t previous) {
  __DMB();
  __set_PRIMASK(previous);
}

/* 任务/发送路径共用的新鲜反馈门。即使上层漏调 Process，也不能继续发旧电流。 */
static bool Gm6020_FeedbackFresh(const Gm6020_HandleTypeDef *hmotor,
                                 uint32_t now_ms) {
  if (hmotor == NULL || !hmotor->angle_initialized ||
      hmotor->state == GM6020_STATE_OFFLINE ||
      hmotor->state == GM6020_STATE_UNINITIALIZED) {
    return false;
  }
  const uint32_t age_ms = now_ms - hmotor->feedback.last_feedback_tick;
  return age_ms > INT32_MAX || age_ms < hmotor->config.feedback_timeout_ms;
}


/* 判断句柄仍在静态注册表中。避免释放/未初始化句柄继续访问共享 CAN 槽位。 */
static bool Gm6020_IsRegistered(const Gm6020_HandleTypeDef *hmotor) {
  if (hmotor == NULL || !hmotor->initialized) {
    return false;
  }
  for (uint8_t i = 0U; i < GM6020_MAX_DEVICE_COUNT; i++) {
    if (g_gm6020_handles[i] == hmotor) {
      return true;
    }
  }
  return false;
}

/* 按 CAN 实例和反馈 ID 查找句柄。只接受注册表中的唯一设备，避免串到另一轴。 */
static Gm6020_HandleTypeDef *Gm6020_Find(CAN_HandleTypeDef *hcan,
                                         uint16_t feedback_id) {
  for (uint8_t i = 0U; i < GM6020_MAX_DEVICE_COUNT; i++) {
    Gm6020_HandleTypeDef *hmotor = g_gm6020_handles[i];
    if (hmotor != NULL && hmotor->config.hcan == hcan &&
        hmotor->config.feedback_id == feedback_id) {
      return hmotor;
    }
  }
  return NULL;
}

/* 按 GM6020 大端协议读取有符号 16 位字段。不能直接转指针，否则 STM32 小端会交换字节。 */
static int16_t Gm6020_ReadI16Be(const uint8_t data[2]) {
  /* CAN 协议是高字节在前。不能直接把地址转换成 int16_t*。STM32 小端会把字节顺序读反。 */
  return (int16_t)(((uint16_t)data[0] << 8U) | data[1]);
}

/* 按大端读取单圈编码器原始值，调用者随后再屏蔽到 13 位有效范围。 */
static uint16_t Gm6020_ReadU16Be(const uint8_t data[2]) {
  return (uint16_t)(((uint16_t)data[0] << 8U) | data[1]);
}

/* 返回两个单圈角度的最短有符号差。相邻反馈不得跨越半圈。 */
static int32_t Gm6020_SignedAngleDelta(uint16_t current_raw,
                                      uint16_t reference_raw) {
  /* 把环形单圈差值折叠到最短路径。这里只适用于相邻帧真实运动不足半圈。 */
  int32_t delta = (int32_t)current_raw - (int32_t)reference_raw;
  if (delta > (int32_t)(GM6020_ENCODER_COUNTS_PER_REV / 2U)) {
    delta -= (int32_t)GM6020_ENCODER_COUNTS_PER_REV;
  } else if (delta < -(int32_t)(GM6020_ENCODER_COUNTS_PER_REV / 2U)) {
    delta += (int32_t)GM6020_ENCODER_COUNTS_PER_REV;
  }
  return delta;
}

/* 将有符号电流按 CAN 大端写入一个两字节槽位。先转无符号只为保留补码位型。 */
static void Gm6020_WriteI16Be(uint8_t data[2], int16_t value) {
  const uint16_t raw = (uint16_t)value;
  data[0] = (uint8_t)(raw >> 8U); /* 电流控制帧要求高字节在前。先放数值的高 8 位。 */
  data[1] = (uint8_t)raw;
}

/* 检查是否配置了有效机械范围。未标定范围用哨兵值表示，不能参与比较。 */
static bool Gm6020_LimitsEnabled(const Gm6020_HandleTypeDef *hmotor) {
  return hmotor->config.mechanical_min_raw != GM6020_ANGLE_LIMIT_DISABLED_MIN ||
         hmotor->config.mechanical_max_raw != GM6020_ANGLE_LIMIT_DISABLED_MAX;
}

/* 用相邻帧最短差展开单圈计数。连续坐标才能直接做限位和位移判断。 */
static void Gm6020_UpdateContinuousAngle(Gm6020_HandleTypeDef *hmotor,
                                         uint16_t angle_raw) {
  if (!hmotor->angle_initialized) {
    if (hmotor->config.angle_reference_enabled) {
      /*
       * 标定中心是连续坐标，反馈角度却是单圈值。取相对中心的最短回绕
       * 差值，可以在 8191->0 时仍得到正确的连续角度。启动位置与中心的
       * 差值必须小于半圈。这不要求机构的总机械行程小于半圈。只限制首帧对齐。
       */
       /* 以标定中心作为连续坐标起点。不用首帧单圈值，才能让标定边界仍保持同一条线。 */
      const int32_t delta = Gm6020_SignedAngleDelta(
          angle_raw, hmotor->config.angle_reference_single_raw);
      hmotor->feedback.angle_total_raw =
          hmotor->config.angle_reference_total_raw + delta;
    } else {
      /* 未完成标定时保留旧行为：首帧单圈值作为临时连续角度起点。只适合观察，不代表真实机械零点。 */
      hmotor->feedback.angle_total_raw = (int32_t)angle_raw;
    }
    hmotor->angle_initialized = true;
    return;
  }

  /* 后续帧只累加相邻单圈的最短差，得到连续总角度。单圈角度在 8191→0 回绕，直接比较会把正常跨圈当成大跳变。 */
  const int32_t delta =
      Gm6020_SignedAngleDelta(angle_raw, hmotor->feedback.angle_raw);
  hmotor->feedback.angle_total_raw += delta;
}

/* 只报告当前连续角度所在的几何边界。输出方向由上层命令过滤决定。 */
static void Gm6020_CheckAngleLimit(Gm6020_HandleTypeDef *hmotor) {
  if (!Gm6020_LimitsEnabled(hmotor) || !hmotor->angle_initialized) {
    hmotor->limit = GM6020_LIMIT_NONE;
    return;
  }
  if (hmotor->feedback.angle_total_raw <= hmotor->config.mechanical_min_raw) {
    hmotor->limit = GM6020_LIMIT_MIN;
  } else if (hmotor->feedback.angle_total_raw >= hmotor->config.mechanical_max_raw) {
    hmotor->limit = GM6020_LIMIT_MAX;
  } else {
    hmotor->limit = GM6020_LIMIT_NONE;
  }
}

/* 在临界区内校验配置并注册句柄。注册失败时清零，避免留下看似初始化成功的对象。 */
static bool Gm6020_InitLocked(Gm6020_HandleTypeDef *hmotor,
                 const Gm6020_ConfigTypeDef *config) {
  if (hmotor == NULL || config == NULL || config->hcan == NULL ||
      (config->current_sign != 1 && config->current_sign != -1) ||
      (config->speed_sign != 1 && config->speed_sign != -1) ||
      config->feedback_id < GM6020_FEEDBACK_ID_MIN ||
      config->feedback_id > GM6020_FEEDBACK_ID_MAX ||
      Gm6020_Find(config->hcan, config->feedback_id) != NULL ||
      config->mechanical_min_raw >= config->mechanical_max_raw ||
      (config->angle_reference_enabled &&
       config->angle_reference_single_raw >= GM6020_ENCODER_COUNTS_PER_REV)) {
    return false;
  }

  memset(hmotor, 0, sizeof(*hmotor));
  hmotor->config = *config;
  if (hmotor->config.feedback_timeout_ms == 0U) {
    hmotor->config.feedback_timeout_ms = GM6020_FEEDBACK_TIMEOUT_MS;
  }
  hmotor->initialized = true;
  for (uint8_t i = 0U; i < GM6020_MAX_DEVICE_COUNT; i++) {
    if (g_gm6020_handles[i] == NULL) {
      g_gm6020_handles[i] = hmotor;
      break;
    }
  }
  if (!Gm6020_IsRegistered(hmotor)) {
    memset(hmotor, 0, sizeof(*hmotor));
    return false;
  }
  hmotor->state = GM6020_STATE_OFFLINE;
  return true;
}

/* 在已进入临界区时移除注册表项并清空句柄，防止 ISR 再找到半释放对象。 */
static bool Gm6020_DeInitLocked(Gm6020_HandleTypeDef *hmotor) {
  if (!Gm6020_IsRegistered(hmotor)) {
    return false;
  }
  for (uint8_t i = 0U; i < GM6020_MAX_DEVICE_COUNT; i++) {
    if (g_gm6020_handles[i] == hmotor) {
      g_gm6020_handles[i] = NULL;
      break;
    }
  }
  memset(hmotor, 0, sizeof(*hmotor));
  return true;
}

/**
 * @brief  设置目标电流缓存。
 * @param  hmotor 电机句柄。
 * @param  current_raw GM6020 电流给定原始值，自动钳位到 -16384~16384。
 * @retval true 已缓存。false 句柄无效。
 * @note   这里只改 RAM，边界命令过滤由 app/gimbal 完成。
 * @note 仅所属轴任务调用。ISR 不设置目标。
 */
bool Gm6020_SetCurrent(Gm6020_HandleTypeDef *hmotor, int16_t current_raw) {
  if (!Gm6020_IsRegistered(hmotor)) return false;
  if (current_raw < GM6020_CURRENT_RAW_MIN) current_raw = GM6020_CURRENT_RAW_MIN;
  if (current_raw > GM6020_CURRENT_RAW_MAX) current_raw = GM6020_CURRENT_RAW_MAX;
  const uint32_t previous = Gm6020_EnterCritical();
  hmotor->target_current_raw = current_raw;
  Gm6020_ExitCritical(previous);
  return true;
}

/**
 * @brief  开关输出。
 * @param  hmotor 电机句柄。
 * @param  enabled false 会立即把目标电流清零并保持关闭。
 * @retval true 许可已设置。false 句柄无效或反馈离线。
 * @note   每轴由自己的任务写。驱动内部短临界区与 CAN ISR 同步状态。
 * @note 仅所属轴任务调用。ISR 不改变输出许可。
 */
bool Gm6020_SetOutputEnabled(Gm6020_HandleTypeDef *hmotor, bool enabled) {
  if (!Gm6020_IsRegistered(hmotor)) return false;
  const uint32_t previous = Gm6020_EnterCritical();
  hmotor->output_enabled = enabled && Gm6020_FeedbackFresh(hmotor, HAL_GetTick());
  if (!hmotor->output_enabled) {
    hmotor->target_current_raw = 0;
    if (hmotor->state != GM6020_STATE_OFFLINE) hmotor->state = GM6020_STATE_DISABLED;
  } else if (hmotor->state == GM6020_STATE_ONLINE) {
    hmotor->state = GM6020_STATE_RUNNING;
  }
  const bool accepted = hmotor->output_enabled == enabled;
  Gm6020_ExitCritical(previous);
  return accepted;
}

/**
 * @brief  重新设置软件角度边界。
 * @param  hmotor 电机句柄。
 * @param  min_angle_raw 连续角度下限，单位编码器计数。
 * @param  max_angle_raw 连续角度上限，必须大于下限。
 * @retval true 设置成功。false 参数非法。
 * @note   这不是读取真实机械挡块位置。首次上电应先标定零点和边界。
 * @note 仅所属轴任务调用。更新后清零目标并关闭输出。
 */
bool Gm6020_SetMechanicalLimit(Gm6020_HandleTypeDef *hmotor,
                               int32_t min_angle_raw,
                               int32_t max_angle_raw) {
  if (!Gm6020_IsRegistered(hmotor) || min_angle_raw >= max_angle_raw) {
    return false;
  }
  const uint32_t previous = Gm6020_EnterCritical();
  hmotor->config.mechanical_min_raw = min_angle_raw;
  hmotor->config.mechanical_max_raw = max_angle_raw;
  hmotor->limit = GM6020_LIMIT_NONE;
  hmotor->output_enabled = false; /* 重新标定后仍需显式重新使能，避免突然转动。清除边界不等于恢复输出。 */
  hmotor->target_current_raw = 0;
  hmotor->state = GM6020_STATE_DISABLED;
  Gm6020_ExitCritical(previous);
  return true;
}


/**
 * @brief  处理一帧从 HAL FIFO 取出的反馈。
 * @param  hcan CAN 外设句柄。
 * @param  rx_header HAL 标准帧头。
 * @param  data 8 字节数据区。
 * @retval true 帧属于本驱动并已更新。false 帧不匹配。
 * @note   可在 CAN ISR 调用。只做解析和状态记录，不调用串口或阻塞 API。
 * @note 仅由串行化的 CAN 接收路径调用；通常位于 CAN ISR。
 */
bool Gm6020_HandleRxMessage(CAN_HandleTypeDef *hcan,
                            const CAN_RxHeaderTypeDef *rx_header,
                            const uint8_t data[GM6020_FRAME_DLC]) {
  if (hcan == NULL || rx_header == NULL || data == NULL ||
      rx_header->IDE != CAN_ID_STD ||
      rx_header->RTR != CAN_RTR_DATA || rx_header->DLC != GM6020_FRAME_DLC ||
      rx_header->StdId < GM6020_FEEDBACK_ID_MIN ||
      rx_header->StdId > GM6020_FEEDBACK_ID_MAX) {
    return false;
  }

  Gm6020_HandleTypeDef *hmotor = Gm6020_Find(hcan, (uint16_t)rx_header->StdId);
  if (hmotor == NULL) {
    return false;
  }
  /* 序列号为奇数表示 ISR 正在写快照。任务读者会避开这一时段。只接受前后相同的偶数序号。 */
  hmotor->feedback_sequence++;
  __DMB();

  const uint16_t angle_raw =
      Gm6020_ReadU16Be(&data[0]) & (GM6020_ENCODER_COUNTS_PER_REV - 1U);
  Gm6020_UpdateContinuousAngle(hmotor, angle_raw);
  hmotor->feedback.angle_raw = angle_raw;
  hmotor->feedback.speed_rpm = Gm6020_ReadI16Be(&data[2]);
  hmotor->feedback.current_raw = Gm6020_ReadI16Be(&data[4]);
  hmotor->feedback.temperature_c = data[6];
  hmotor->feedback.reserved_raw = data[7]; /* 手册未定义该保留字节，原样保存供诊断。不参与控制。 */
  hmotor->feedback.last_feedback_tick = HAL_GetTick();
  hmotor->state = hmotor->output_enabled ? GM6020_STATE_RUNNING
                                         : GM6020_STATE_ONLINE;
  Gm6020_CheckAngleLimit(hmotor);
  __DMB();
  hmotor->feedback_sequence++;
  return true;
}

/**
 * @brief  从指定 CAN FIFO 取一帧并解析。
 * @param  hcan CAN 外设句柄。
 * @param  rx_fifo HAL_CAN_RX_FIFO0 或 HAL_CAN_RX_FIFO1。
 * @retval true 取到并处理了本驱动帧。false HAL 失败或 ID 不匹配。
 * @note 仅 CAN ISR 调用。多个驱动共享 FIFO 时使用统一分发入口。
 */
bool Gm6020_RxFifoCallback(CAN_HandleTypeDef *hcan, uint32_t rx_fifo) {
  CAN_RxHeaderTypeDef rx_header = {0};
  uint8_t data[GM6020_FRAME_DLC] = {0};
  if (hcan == NULL || HAL_CAN_GetRxMessage(hcan, rx_fifo, &rx_header, data) !=
                          HAL_OK) {
    return false;
  }
  return Gm6020_HandleRxMessage(hcan, &rx_header, data);
}

/* 在临界区内重建并提交完整聚合帧。跨任务分别写槽位会把另一轴的新值和旧值拼在一起。 */
static bool Gm6020_SendLocked(Gm6020_HandleTypeDef *hmotor) {
  if (!Gm6020_IsRegistered(hmotor) ||
      HAL_CAN_GetTxMailboxesFreeLevel(hmotor->config.hcan) == 0U) {
    return false;
  }

  uint8_t low_data[GM6020_FRAME_DLC] = {0};
  uint8_t high_data[GM6020_FRAME_DLC] = {0};
  bool low_used = false;
  bool high_used = false;
  const uint32_t now_ms = HAL_GetTick();
  /*
   * GM6020 手册把电流控制分成两条标准帧：0x1FE 承载 ID 1~4，
   * 0x2FE 承载 ID 5~7。每次发送都重新聚合所有已注册句柄，避免
   * Yaw 和 Pitch 两个任务分别写帧时把另一个电机的槽位误清零。每次按注册表重新聚合。
   */
  for (uint8_t i = 0U; i < GM6020_MAX_DEVICE_COUNT; i++) {
    Gm6020_HandleTypeDef *registered = g_gm6020_handles[i];
    if (registered == NULL || registered->config.hcan != hmotor->config.hcan) {
      continue;
    }
    const int16_t current = registered->output_enabled &&
                                    Gm6020_FeedbackFresh(registered, now_ms)
                                ? registered->target_current_raw
                                : 0;
    const uint8_t motor_index = (uint8_t)(registered->config.feedback_id -
                                          GM6020_FEEDBACK_ID_MIN);
    if (motor_index < 4U) {
      /* ID 1~4 在 0x1FE 中依次占 DATA[0:1]、[2:3]、[4:5]、[6:7]。每个 ID 两字节。 */
      Gm6020_WriteI16Be(&low_data[2U * motor_index], current);
      low_used = true;
    } else {
      /* ID 5~7 在 0x2FE 中从 DATA[0:1] 重新编号，不能写到数组越界处。高帧只有 3 个槽位。 */
      const uint8_t high_slot = (uint8_t)(motor_index - 4U);
      Gm6020_WriteI16Be(&high_data[2U * high_slot], current);
      high_used = true;
    }
  }

  if (low_used) {
    CAN_TxHeaderTypeDef header = {0};
    uint32_t mailbox = 0U;
    header.StdId = GM6020_CURRENT_CONTROL_ID_LOW;
    header.IDE = CAN_ID_STD;
    header.RTR = CAN_RTR_DATA;
    header.DLC = GM6020_FRAME_DLC;
    if (HAL_CAN_AddTxMessage(hmotor->config.hcan, &header, low_data, &mailbox) !=
        HAL_OK) {
      return false;
    }
  }

  if (high_used) {
    CAN_TxHeaderTypeDef header = {0};
    uint32_t mailbox = 0U;
    header.StdId = GM6020_CURRENT_CONTROL_ID_HIGH;
    header.IDE = CAN_ID_STD;
    header.RTR = CAN_RTR_DATA;
    header.DLC = GM6020_FRAME_DLC;
    if (HAL_CAN_AddTxMessage(hmotor->config.hcan, &header, high_data, &mailbox) !=
        HAL_OK) {
      return false;
    }
  }

  return low_used || high_used;
}

/**
 * @brief  发送本 CAN 总线上已注册句柄的电流控制帧。
 * @param  hmotor 已初始化句柄。
 * @retval true HAL 发送成功。false 邮箱不足、参数非法或设备未注册。
 * @note   ID 1~4 聚合到 0x1FE，ID 5~7 聚合到 0x2FE。每个任务使用自己的句柄。
 * 构建与提交在一个很短的 PRIMASK 临界区完成，不等待邮箱或总线完成。
 * 返回 true 只说明 HAL 接受，不能解释为电调已经执行。
 * @note 仅任务调用。ISR 只记录反馈，不发送任务电流。
 */
/* 聚合帧构建与提交必须处于同一临界区，防止任务切换提交旧帧。避免 Yaw/Pitch 槽位不一致。 */
bool Gm6020_Send(Gm6020_HandleTypeDef *hmotor) {
  const uint32_t previous = Gm6020_EnterCritical();
  const bool submitted = Gm6020_SendLocked(hmotor);
  Gm6020_ExitCritical(previous);
  return submitted;
}

/**
 * @brief  注册一个 GM6020 句柄并清零运行状态。
 * @param  hmotor 调用方持有的句柄，不能为 NULL。
 * @param  config CAN 句柄、反馈 ID、超时、角度边界和可选标定参考。函数会复制配置内容。
 * @retval true 参数合法且注册成功。
 * @retval false 参数非法、同一 CAN 总线上的 ID 重复或静态注册表已满。
 * @note   本函数不启动 CAN，也不配置过滤器。初始化属于板级代码。
 * @note 仅任务初始化。短临界区串行化注册表与 CAN ISR。
 */
bool Gm6020_Init(Gm6020_HandleTypeDef *hmotor,
                 const Gm6020_ConfigTypeDef *config) {
  const uint32_t previous = Gm6020_EnterCritical();
  const bool registered = Gm6020_InitLocked(hmotor, config);
  Gm6020_ExitCritical(previous);
  return registered;
}

/**
 * @brief  注销 GM6020 句柄。
 * @param  hmotor 已初始化句柄。
 * @retval true 注销成功。false 参数为空或句柄未注册。
 * @note 仅任务调用。先提交零电流，再注销句柄；注销不等于硬件已停止。
 */
bool Gm6020_DeInit(Gm6020_HandleTypeDef *hmotor) {
  const uint32_t previous = Gm6020_EnterCritical();
  const bool removed = Gm6020_DeInitLocked(hmotor);
  Gm6020_ExitCritical(previous);
  return removed;
}

/**
 * @brief  在任务上下文更新在线和软件限位状态。
 * @param  hmotor 电机句柄。
 * @param  now_tick 与反馈时间戳相同的毫秒时基。当前实现应传入 HAL_GetTick()，使用无符号差值处理回绕。
 * @retval true 处理完成。false 句柄无效。
 * @note 仅所属轴任务调用。
 */
bool Gm6020_Process(Gm6020_HandleTypeDef *hmotor, uint32_t now_tick) {
  if (!Gm6020_IsRegistered(hmotor)) {
    return false;
  }
  const uint32_t previous = Gm6020_EnterCritical();
  const uint32_t elapsed = now_tick - hmotor->feedback.last_feedback_tick;
  /* ISR 可能在取样后写入更新的时间戳。无符号下溢必须视为未超时。 */
  const bool timed_out = elapsed <= INT32_MAX &&
                        elapsed >= hmotor->config.feedback_timeout_ms;
  if (!hmotor->angle_initialized || timed_out) {
    hmotor->state = GM6020_STATE_OFFLINE;
    hmotor->target_current_raw = 0;
    hmotor->output_enabled = false;
  } else {
    Gm6020_CheckAngleLimit(hmotor);
  }
  Gm6020_ExitCritical(previous);
  return true;
}

/**
 * @brief  读取一组来自同一次 CAN 反馈更新的状态快照。
 * @param  hmotor 已初始化的 GM6020 句柄。
 * @param  snapshot 输出快照，不保存内部指针。
 * @retval true  成功取得一致快照。
 * @retval false 句柄无效或 ISR 更新过于频繁导致本次复制失败。
 * @note   CAN ISR 使用序列号包住反馈写入，任务不应直接读取句柄中的
 * feedback/state 字段，否则一行日志可能混合两帧数据。
 * @note 仅任务读取。每个控制周期使用同一份快照。
 */
bool Gm6020_GetSnapshot(const Gm6020_HandleTypeDef *hmotor,
                        Gm6020_SnapshotTypeDef *snapshot) {
  if (!Gm6020_IsRegistered(hmotor) || snapshot == NULL) {
    return false;
  }

  /*
   * CAN ISR 写入反馈期间序列号为奇数。复制前后各读一次序列号，只有两次
   * 相同且为偶数才接受结果。这样不会把角度来自新帧、速度来自旧帧的内容
   * 交给位置环。失败时调用者应保持零输出或沿用上一份安全状态。不能使用半帧数据。
   */
  for (uint8_t retry = 0U; retry < 4U; retry++) {
    const uint32_t sequence_before = hmotor->feedback_sequence;
    if ((sequence_before & 1U) != 0U) {
      continue;
    }
    __DMB();
    snapshot->feedback = hmotor->feedback;
    snapshot->feedback_received = hmotor->angle_initialized;
    snapshot->state = hmotor->state;
    snapshot->limit = hmotor->limit;
    snapshot->target_current_raw = hmotor->target_current_raw;
    snapshot->output_enabled = hmotor->output_enabled;
    const uint32_t age_ms = HAL_GetTick() - snapshot->feedback.last_feedback_tick;
    snapshot->online = snapshot->feedback_received &&
                       snapshot->state != GM6020_STATE_OFFLINE &&
                       snapshot->state != GM6020_STATE_UNINITIALIZED &&
                       (age_ms > INT32_MAX ||
                        age_ms < hmotor->config.feedback_timeout_ms);
    __DMB();
    /* 读取后再次检查序列号，确认复制期间没有被 ISR 改写。 */
    const uint32_t sequence_after = hmotor->feedback_sequence;
    /*
     * 1. 前后序列号相等：复制期间没有 CAN 反馈更新。
     * 2. 序列号为偶数：反馈字段已完整写入。
     */
    if (sequence_before == sequence_after &&
        (sequence_after & 1U) == 0U) {
      return true;
    }
  }
  return false;
}

/**
 * @brief  从一致状态快照复制反馈。
 * @param  hmotor 电机句柄。
 * @param  feedback 输出反馈结构体。
 * @retval true 复制成功。false 参数非法。
 * @note 仅任务读取；内部通过 GetSnapshot 复制，不返回内部指针。
 */
bool Gm6020_GetFeedback(const Gm6020_HandleTypeDef *hmotor,
                        Gm6020_FeedbackTypeDef *feedback) {
  Gm6020_SnapshotTypeDef snapshot = {0};
  if (feedback == NULL || !Gm6020_GetSnapshot(hmotor, &snapshot)) {
    return false;
  }
  *feedback = snapshot.feedback;
  return true;
}

/**
 * @brief  查询电机是否在线。
 * @param  hmotor 电机句柄。
 * @retval true 最近收到反馈。false 未初始化或已超时。
 * @note 仅任务查询；返回值不表示电机正在转动。
 */
bool Gm6020_IsOnline(const Gm6020_HandleTypeDef *hmotor) {
  Gm6020_SnapshotTypeDef snapshot = {0};
  return Gm6020_GetSnapshot(hmotor, &snapshot) && snapshot.online;
}

/**
 * @brief  获取当前限位原因。
 * @param  hmotor 电机句柄。
 * @retval GM6020_LIMIT_NONE、MIN 或 MAX。该值只表示当前连续角度是否越过
 * 配置边界，不锁存限位，也不代表机械堵转。
 * @note 仅任务诊断单一边界字段；控制与日志使用 GetSnapshot。
 */
Gm6020_LimitTypeDef Gm6020_GetLimit(const Gm6020_HandleTypeDef *hmotor) {
  return Gm6020_IsRegistered(hmotor) ? hmotor->limit : GM6020_LIMIT_NONE;
}

#else

/**
 * @brief 保留未启用 CAN 时的移植接口。
 * @param h 保留参数；未启用 CAN 时不使用。
 * @param c 保留参数；未启用 CAN 时不使用。
 * @retval false；不修改句柄、输出参数或硬件。
 * @note 仅在 HAL_CAN_MODULE_ENABLED 未定义时编译；不代表硬件已停止。
 */
bool Gm6020_Init(Gm6020_HandleTypeDef *h, const Gm6020_ConfigTypeDef *c) { (void)h; (void)c; return false; }
/**
 * @brief 保留未启用 CAN 时的移植接口。
 * @param h 保留参数；未启用 CAN 时不使用。
 * @retval false；不修改句柄、输出参数或硬件。
 * @note 仅在 HAL_CAN_MODULE_ENABLED 未定义时编译；不代表硬件已停止。
 */
bool Gm6020_DeInit(Gm6020_HandleTypeDef *h) { (void)h; return false; }
/**
 * @brief 保留未启用 CAN 时的移植接口。
 * @param h 保留参数；未启用 CAN 时不使用。
 * @param c 保留参数；未启用 CAN 时不使用。
 * @retval false；不修改句柄、输出参数或硬件。
 * @note 仅在 HAL_CAN_MODULE_ENABLED 未定义时编译；不代表硬件已停止。
 */
bool Gm6020_SetCurrent(Gm6020_HandleTypeDef *h, int16_t c) { (void)h; (void)c; return false; }
/**
 * @brief 保留未启用 CAN 时的移植接口。
 * @param h 保留参数；未启用 CAN 时不使用。
 * @param e 保留参数；未启用 CAN 时不使用。
 * @retval false；不修改句柄、输出参数或硬件。
 * @note 仅在 HAL_CAN_MODULE_ENABLED 未定义时编译；不代表硬件已停止。
 */
bool Gm6020_SetOutputEnabled(Gm6020_HandleTypeDef *h, bool e) { (void)h; (void)e; return false; }
/**
 * @brief 保留未启用 CAN 时的移植接口。
 * @param h 保留参数；未启用 CAN 时不使用。
 * @param min 保留参数；未启用 CAN 时不使用。
 * @param max 保留参数；未启用 CAN 时不使用。
 * @retval false；不修改句柄、输出参数或硬件。
 * @note 仅在 HAL_CAN_MODULE_ENABLED 未定义时编译；不代表硬件已停止。
 */
bool Gm6020_SetMechanicalLimit(Gm6020_HandleTypeDef *h, int32_t min, int32_t max) { (void)h; (void)min; (void)max; return false; }
/**
 * @brief 保留未启用 CAN 时的移植接口。
 * @param h 保留参数；未启用 CAN 时不使用。
 * @param r 保留参数；未启用 CAN 时不使用。
 * @param d 保留参数；未启用 CAN 时不使用。
 * @retval false；不修改句柄、输出参数或硬件。
 * @note 仅在 HAL_CAN_MODULE_ENABLED 未定义时编译；不代表硬件已停止。
 */
bool Gm6020_HandleRxMessage(CAN_HandleTypeDef *h, const CAN_RxHeaderTypeDef *r, const uint8_t d[GM6020_FRAME_DLC]) { (void)h; (void)r; (void)d; return false; }
/**
 * @brief 保留未启用 CAN 时的移植接口。
 * @param h 保留参数；未启用 CAN 时不使用。
 * @param f 保留参数；未启用 CAN 时不使用。
 * @retval false；不修改句柄、输出参数或硬件。
 * @note 仅在 HAL_CAN_MODULE_ENABLED 未定义时编译；不代表硬件已停止。
 */
bool Gm6020_RxFifoCallback(CAN_HandleTypeDef *h, uint32_t f) { (void)h; (void)f; return false; }
/**
 * @brief 保留未启用 CAN 时的移植接口。
 * @param h 保留参数；未启用 CAN 时不使用。
 * @retval false；不修改句柄、输出参数或硬件。
 * @note 仅在 HAL_CAN_MODULE_ENABLED 未定义时编译；不代表硬件已停止。
 */
bool Gm6020_Send(Gm6020_HandleTypeDef *h) { (void)h; return false; }
/**
 * @brief 保留未启用 CAN 时的移植接口。
 * @param h 保留参数；未启用 CAN 时不使用。
 * @param t 保留参数；未启用 CAN 时不使用。
 * @retval false；不修改句柄、输出参数或硬件。
 * @note 仅在 HAL_CAN_MODULE_ENABLED 未定义时编译；不代表硬件已停止。
 */
bool Gm6020_Process(Gm6020_HandleTypeDef *h, uint32_t t) { (void)h; (void)t; return false; }
/**
 * @brief 保留未启用 CAN 时的移植接口。
 * @param h 保留参数；未启用 CAN 时不使用。
 * @param s 保留参数；未启用 CAN 时不使用。
 * @retval false；不修改句柄、输出参数或硬件。
 * @note 仅在 HAL_CAN_MODULE_ENABLED 未定义时编译；不代表硬件已停止。
 */
bool Gm6020_GetSnapshot(const Gm6020_HandleTypeDef *h, Gm6020_SnapshotTypeDef *s) { (void)h; (void)s; return false; }
/**
 * @brief 保留未启用 CAN 时的移植接口。
 * @param h 保留参数；未启用 CAN 时不使用。
 * @param f 保留参数；未启用 CAN 时不使用。
 * @retval false；不修改句柄、输出参数或硬件。
 * @note 仅在 HAL_CAN_MODULE_ENABLED 未定义时编译；不代表硬件已停止。
 */
bool Gm6020_GetFeedback(const Gm6020_HandleTypeDef *h, Gm6020_FeedbackTypeDef *f) { (void)h; (void)f; return false; }
/**
 * @brief 保留未启用 CAN 时的移植接口。
 * @param h 保留参数；未启用 CAN 时不使用。
 * @retval false；不修改句柄、输出参数或硬件。
 * @note 仅在 HAL_CAN_MODULE_ENABLED 未定义时编译；不代表硬件已停止。
 */
bool Gm6020_IsOnline(const Gm6020_HandleTypeDef *h) { (void)h; return false; }
/**
 * @brief 保留未启用 CAN 时的移植接口。
 * @param h 保留参数；未启用 CAN 时不使用。
 * @retval GM6020_LIMIT_NONE；不读取硬件。
 * @note 仅在 HAL_CAN_MODULE_ENABLED 未定义时编译；不代表硬件已停止。
 */
Gm6020_LimitTypeDef Gm6020_GetLimit(const Gm6020_HandleTypeDef *h) { (void)h; return GM6020_LIMIT_NONE; }

#endif
