# 关键实现代码与调用说明

按输入→算法→云台→供弹→日志排列，以下为当前源码原样摘录，不是独立编译的另一套算法。初始化、类型和辅助函数沿源文件链接查阅。重复函数名取最后定义，避开未启用HAL的桩函数。

从工程根运行 powershell -NoProfile -File doc/tools/update_technical_reference.ps1 可重建本文件和宏索引。原理推导和实测状态仍需同步 [技术总文档](TECHNICAL_ARCHITECTURE.md)。

## Dbus_DecodeFrame

DBUS位解包、大端/小端区别和范围校验。

源文件：[Core/Src/bsp/dbus/dbus.c](../Core/Src/bsp/dbus/dbus.c)，定义始于第 56 行。

```c
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
```

## Dbus_CopyCompleted

DMA块复制到邮箱；环形数组保留一个空槽。

源文件：[Core/Src/bsp/dbus/dbus.c](../Core/Src/bsp/dbus/dbus.c)，定义始于第 123 行。

```c
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
```

## Dbus_Process

邮箱排空、epoch旧帧拒绝和鼠标位移累计。

源文件：[Core/Src/bsp/dbus/dbus.c](../Core/Src/bsp/dbus/dbus.c)，定义始于第 270 行。

```c
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
```

## Dbus_GetSnapshot

同一快照复制并消费累计鼠标位移。

源文件：[Core/Src/bsp/dbus/dbus.c](../Core/Src/bsp/dbus/dbus.c)，定义始于第 334 行。

```c
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
```

## DbusTask_MapChannel

摇杆死区和raw→千分比。

源文件：[Core/Src/task/task_dbus/task_dbus_runtime.c](../Core/Src/task/task_dbus/task_dbus_runtime.c)，定义始于第 84 行。

```c
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
```

## DbusTask_RuntimeRunCycle

同一DBUS快照生成三份命令，保留真实时间戳。

源文件：[Core/Src/task/task_dbus/task_dbus_runtime.c](../Core/Src/task/task_dbus/task_dbus_runtime.c)，定义始于第 113 行。

```c
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
```

## MouseVirtualJoystick_AdvanceAxis

固定锚点和绝对时间控制保持/回中。

源文件：[Core/Src/algorithm/mouse_virtual_joystick/mouse_virtual_joystick.c](../Core/Src/algorithm/mouse_virtual_joystick/mouse_virtual_joystick.c)，定义始于第 40 行。

```c
static void MouseVirtualJoystick_AdvanceAxis(
    MouseVirtualJoystick_AxisTypeDef *axis,
    const MouseVirtualJoystick_AxisConfigTypeDef *config,
    uint32_t now_ms) {
  if (!axis->has_event) {
    axis->state = MOUSE_VIRTUAL_JOYSTICK_STATE_IDLE;
    return;
  }
  const uint32_t age_ms = now_ms - axis->last_event_ms;
  if (age_ms < config->hold_ms) {
    axis->state = MOUSE_VIRTUAL_JOYSTICK_STATE_HOLD;
    return;
  }
  const uint32_t elapsed_ms = age_ms - config->hold_ms;
  if (elapsed_ms >= config->decay_ms) {
    *axis = (MouseVirtualJoystick_AxisTypeDef){0};
    return;
  }
  axis->value_permille = axis->anchor_permille *
      ((float)(config->decay_ms - elapsed_ms) / (float)config->decay_ms);
  axis->state = MOUSE_VIRTUAL_JOYSTICK_STATE_DECAY;
}
```

## MouseVirtualJoystick_Update

去重、时序校验、分轴累计和输出。

源文件：[Core/Src/algorithm/mouse_virtual_joystick/mouse_virtual_joystick.c](../Core/Src/algorithm/mouse_virtual_joystick/mouse_virtual_joystick.c)，定义始于第 144 行。

```c
bool MouseVirtualJoystick_Update(
    MouseVirtualJoystick_HandleTypeDef *handle,
    uint32_t now_ms,
    uint32_t frame_sequence,
    uint32_t frame_timestamp_ms,
    int32_t mouse_x,
    int32_t mouse_y,
    bool online,
    float *virtual_x,
    float *virtual_y) {
  /* 先准备失败输出，再检查输入时间。无效参数不能留下旧速度。（失效立即松开虚拟杆。） */
  if (virtual_x != NULL) {
    *virtual_x = 0.0f;
  }
  if (virtual_y != NULL) {
    *virtual_y = 0.0f;
  }
  if (handle == NULL) {
    return false;
  }
  if (virtual_x == NULL || virtual_y == NULL || !handle->initialized || !online ||
      !MouseVirtualJoystick_IsForward(now_ms, frame_timestamp_ms) ||
      (handle->has_update && !MouseVirtualJoystick_IsForward(now_ms, handle->last_update_ms)) ||
      (handle->has_frame && !MouseVirtualJoystick_IsForward(frame_sequence, handle->last_frame_sequence)) ||
      (handle->has_frame && !MouseVirtualJoystick_IsForward(frame_timestamp_ms, handle->last_frame_timestamp_ms)) ||
      (handle->has_frame && frame_sequence == handle->last_frame_sequence &&
       frame_timestamp_ms != handle->last_frame_timestamp_ms) ||
      (handle->x.has_event && !MouseVirtualJoystick_IsForward(now_ms, handle->x.last_event_ms)) ||
      (handle->y.has_event && !MouseVirtualJoystick_IsForward(now_ms, handle->y.last_event_ms))) {
    MouseVirtualJoystick_Reset(handle);
    return false;
  }

  const bool new_frame = !handle->has_frame || frame_sequence != handle->last_frame_sequence;
  if (!MouseVirtualJoystick_UpdateAxis(&handle->x, &handle->config.x, handle->config.output_limit_permille,
                                       now_ms, frame_timestamp_ms, new_frame ? mouse_x : 0) ||
      !MouseVirtualJoystick_UpdateAxis(&handle->y, &handle->config.y, handle->config.output_limit_permille,
                                       now_ms, frame_timestamp_ms, new_frame ? mouse_y : 0)) {
    MouseVirtualJoystick_Reset(handle);
    return false;
  }
  handle->last_frame_sequence = frame_sequence;
  handle->last_frame_timestamp_ms = frame_timestamp_ms;
  handle->last_update_ms = now_ms;
  handle->has_frame = true;
  handle->has_update = true;
  handle->state = handle->x.state > handle->y.state ? handle->x.state : handle->y.state;
  /* 状态已限幅且有限。保留小数，避免低增益输入在命令边界被吞掉。 */
  *virtual_x = handle->x.value_permille;
  *virtual_y = handle->y.value_permille;
  return true;
}
```

## Pid_Update

候选积分、微分首帧、限幅和抗积分饱和。

源文件：[Core/Src/algorithm/pid/pid.c](../Core/Src/algorithm/pid/pid.c)，定义始于第 82 行。

```c
float Pid_Update(Pid_ControllerTypeDef *controller, float error, float dt_s) {
  if (controller == NULL || dt_s <= 0.0f) {
    return 0.0f;
  }

  if (!controller->initialized) {
    /* 首次没有真实的上一帧误差；若拿默认 0 做差分，启动瞬间会制造假的微分尖峰。 */
    controller->previous_error = error;
    controller->initialized = true;
  }
  /* 微分项看误差变化率；dt_s 必须是真实周期，否则任务抖动会被当成快速运动。 */
  const float derivative = (error - controller->previous_error) / dt_s;
  /* 先计算候选积分，再限幅（避免历史误差超过允许范围）。 */
  const float next_integral = Pid_Clamp(
      controller->integral + controller->ki * error * dt_s,
      controller->integral_min, controller->integral_max);
  const float unsaturated = controller->kp * error + next_integral +
                            controller->kd * derivative;
  const float output =
      Pid_Clamp(unsaturated, controller->output_min, controller->output_max);

  /*
   * 积分饱和：电机被挡住时误差长期存在，积分会越堆越大，解除阻挡后就会过冲。
   * 这里记录输出是否已经顶到上限或下限（控制器已经没有更多“力气”可用）。
   */
  const bool saturated_high = unsaturated > controller->output_max;
  const bool saturated_low = unsaturated < controller->output_min;
  /*
   * 没有饱和时正常保存积分；已经顶到上限时只接受负误差，顶到下限时只接受正误差
   * （通俗理解：只允许积分把输出从“顶住的方向”拉回来，不允许继续往墙上加力）。
   */
  if ((!saturated_high && !saturated_low) ||
      (saturated_high && error < 0.0f) ||
      (saturated_low && error > 0.0f)) {
    controller->integral = next_integral;
  }
  controller->previous_error = error;
  return output;
}
```

## Ramp_Update

rate×dt_s限制单周期变化。

源文件：[Core/Src/algorithm/ramp/ramp.c](../Core/Src/algorithm/ramp/ramp.c)，定义始于第 49 行。

```c
float Ramp_Update(Ramp_HandleTypeDef *ramp,
                  float target_value,
                  float max_rate_per_s,
                  float dt_s) {
  if (ramp == NULL || dt_s <= 0.0f || max_rate_per_s < 0.0f) {
    return 0.0f;
  }
  if (!ramp->initialized) {
    Ramp_Init(ramp, target_value);
  }

  /* 变化率乘以真实 dt 才是本周期允许的步长；直接把“每秒”当“每周期”会让周期变化改变电流斜率。 */
  const float maximum_step = max_rate_per_s * dt_s;
  const float difference = target_value - ramp->value; /* 目标和当前值之差（正数要增加，负数要减少）。 */
  if (difference > maximum_step) {
    ramp->value += maximum_step; /* 需要增加很多，本次只增加上限（避免输出突然跳变）。 */
  } else if (difference < -maximum_step) {
    ramp->value -= maximum_step; /* 需要减少很多，本次只减少上限（避免反向冲击）。 */
  } else {
    ramp->value = target_value; /* 差距已经很小，直接到达目标（避免长期留下微小误差）。 */
  }
  return ramp->value;
}
```

## LowPassFilter_Update

速度反馈低通。

源文件：[Core/Src/algorithm/filter/low_pass_filter.c](../Core/Src/algorithm/filter/low_pass_filter.c)，定义始于第 66 行。

```c
float LowPassFilter_Update(LowPassFilter_HandleTypeDef *filter, float input) {
  if (filter == NULL) {
    return 0.0f;
  }
  if (!filter->initialized) {
    LowPassFilter_Init(filter, input, filter->alpha);
  }
  
  /*
   * y[n]=y[n-1]+alpha*(x[n]-y[n-1])：只走差值的一部分；若直接赋值，单帧
   * CAN 毛刺会原样进入控制环，alpha 越小越稳但滞后越明显（不能两者同时为零）。
   */
  filter->value += filter->alpha * (input - filter->value);
  return filter->value;
}
```

## GravityCompensation_Update

int64角差、周期取模和余弦补偿。

源文件：[Core/Src/algorithm/gravity_compensation/gravity_compensation.c](../Core/Src/algorithm/gravity_compensation/gravity_compensation.c)，定义始于第 73 行。

```c
float GravityCompensation_Update(
    GravityCompensation_HandleTypeDef *handle,
    int32_t angle_total_raw) {
  if (handle == NULL || !handle->initialized || !handle->config.enabled) {
    return 0.0f;
  }

  /* 先用 int64 求差，再按一圈取余。余弦周期不变；大连续计数转 float 后相减会丢失小角差。
   * 取余只用于重力模型，不修改传入角度或控制目标。（跨圈仍计算同一机械角的补偿。） */
  const int64_t offset_raw = (int64_t)angle_total_raw - handle->config.center_angle_raw;
  const float angle_offset_rad = (float)(offset_raw % GRAVITY_COMPENSATION_COUNTS_PER_REV) *
      GRAVITY_COMPENSATION_TWO_PI / (float)GRAVITY_COMPENSATION_COUNTS_PER_REV;
  const float magnitude = handle->config.bias_current_raw +
      handle->config.amplitude_current_raw * cosf(angle_offset_rad);
  const float current_raw = magnitude * (float)handle->config.current_sign;
  if (!isfinite(current_raw)) {
    return 0.0f;
  }
  return GravityCompensation_Clamp(current_raw, handle->config.max_current_raw);
}
```

## GimbalControl_Update

目标积分、边界过滤、级联环、补偿和Ramp。

源文件：[Core/Src/app/gimbal/gimbal_control.c](../Core/Src/app/gimbal/gimbal_control.c)，定义始于第 123 行。

```c
void GimbalControl_Update(GimbalControl_HandleTypeDef *controller,
                          const Gm6020_FeedbackTypeDef *feedback,
                          const Gimbal_CommandTypeDef *command,
                          float dt_s,
                          GimbalControl_OutputTypeDef *output) {
  if (output != NULL) {
    *output = (GimbalControl_OutputTypeDef){0};
  }
  if (controller == NULL || feedback == NULL || command == NULL ||
      output == NULL || !controller->initialized || !isfinite(dt_s) || dt_s <= 0.0f) {
    return;
  }

  const float filtered_speed =
      LowPassFilter_Update(&controller->speed_filter,
                           (float)feedback->speed_rpm);
  const float requested_command_speed = command->enabled
      ? ((float)command->velocity_permille / 1000.0f) *
            controller->config.max_command_speed_rpm
      : 0.0f;
  const bool at_min = feedback->angle_total_raw <= controller->config.min_angle_raw;
  const bool at_max = feedback->angle_total_raw >= controller->config.max_angle_raw;
  const bool remote_command = !command->fixed_target;
  const bool outward_at_min = remote_command && at_min &&
                              requested_command_speed < 0.0f;
  const bool outward_at_max = remote_command && at_max &&
                              requested_command_speed > 0.0f;
  /* 软件边界必须同时屏蔽目标积分和速度前馈。只拦截目标积分会留下正的
   * 前馈速度，位置误差虽然为零，速度环仍会向挡块输出电流。（边界像红灯，
   * 不能只禁止加速目标，却继续把油门信号送进下一环。） */
  const bool block_outward = outward_at_min || outward_at_max;
  const float command_speed = block_outward ? 0.0f : requested_command_speed;
  const bool at_or_beyond = at_min || at_max;

  /* 只有非零有效输入更新目标。边界钳位也只作用于本次积分；零输入不能改写保存目标。
   * 越界反馈不触发重定位，向内命令从最后目标连续积分。（松手只停止目标移动。） */
  if (command_speed != 0.0f) {
    controller->target_angle_raw +=
        command_speed * ((float)GM6020_ENCODER_COUNTS_PER_REV / 60.0f) * dt_s;
    if (!at_or_beyond) {
      controller->target_angle_raw = GimbalControl_Clamp(
          controller->target_angle_raw, (float)controller->config.min_angle_raw,
          (float)controller->config.max_angle_raw);
    }
  }

  const float angle_error = controller->target_angle_raw -
                            (float)feedback->angle_total_raw;
  const float previous_integral = controller->position_integral_rpm;
  const float integral_candidate = previous_integral +
      controller->config.position_ki_rpm_per_raw_s * angle_error * dt_s;
  controller->position_integral_rpm = GimbalControl_Clamp(
      integral_candidate, -controller->config.position_integral_limit_rpm,
      controller->config.position_integral_limit_rpm);
  const float angle_error_rate_raw_per_s =
      (command_speed - filtered_speed) *
      ((float)GM6020_ENCODER_COUNTS_PER_REV / 60.0f);
  const float target_speed_unclamped =
      controller->config.position_kp_rpm_per_raw * angle_error +
      controller->position_integral_rpm +
      controller->config.position_kd_rpm_s_per_raw * angle_error_rate_raw_per_s +
      controller->config.velocity_feedforward_gain * command_speed;
  if ((target_speed_unclamped > controller->config.max_speed_target_rpm &&
       angle_error > 0.0f) ||
      (target_speed_unclamped < -controller->config.max_speed_target_rpm &&
       angle_error < 0.0f)) {
    /* 恢复上一周期的限幅积分。不能从已钳位值减完整增量，否则大误差会造成反向越限。 */
    controller->position_integral_rpm = previous_integral;
  }
  float target_speed = controller->config.position_kp_rpm_per_raw * angle_error +
      controller->position_integral_rpm +
      controller->config.position_kd_rpm_s_per_raw * angle_error_rate_raw_per_s +
      controller->config.velocity_feedforward_gain * command_speed;
  target_speed = GimbalControl_Clamp(target_speed,
                                     -controller->config.max_speed_target_rpm,
                                     controller->config.max_speed_target_rpm);
  controller->target_speed_rpm = target_speed;

  const float speed_error = target_speed - filtered_speed;
  const float speed_loop_current =
      Pid_Update(&controller->speed_pid, speed_error, dt_s);
  const float gravity_current = GravityCompensation_Update(
      &controller->gravity_compensation, feedback->angle_total_raw);
  const float combined_current = GimbalControl_Clamp(
      speed_loop_current + gravity_current, -controller->config.max_current_raw,
      controller->config.max_current_raw);
  if (!isfinite(combined_current)) {
    GimbalControl_ResetHistory(controller);
    return; /* 非有限计算结果不转换为 CAN 整数；输出保持本周期初始零值。 */
  }
  const float ramped_current = Ramp_Update(
      &controller->current_ramp, combined_current,
      controller->config.current_slew_raw_per_s, dt_s);

  output->target_angle_raw = (int32_t)controller->target_angle_raw;
  output->angle_error_raw = (int32_t)angle_error;
  output->target_speed_rpm = target_speed;
  output->speed_loop_current_raw = GimbalControl_ToCurrent(speed_loop_current);
  output->gravity_compensation_current_raw =
      GimbalControl_ToCurrent(gravity_current);
  output->target_current_raw = GimbalControl_ToCurrent(ramped_current);
  output->at_min_limit = at_min;
  output->at_max_limit = at_max;
}
```

## GimbalAxis_Run

一致快照、安全门、恢复、符号和CAN提交。

源文件：[Core/Src/app/gimbal/gimbal_axis.c](../Core/Src/app/gimbal/gimbal_axis.c)，定义始于第 179 行。

```c
static void GimbalAxis_Run(GimbalAxis_HandleTypeDef *axis,
                           const Gimbal_CommandTypeDef *command,
                           bool fixed_target, int32_t target_angle_raw,
                           uint32_t now_ms, uint32_t dt_ms) {
  if (axis == NULL || !axis->initialized) {
    return;
  }
  axis->cycle = (GimbalAxis_OutputTypeDef){0};
  (void)Gm6020_Process(&axis->motor, now_ms);
  axis->cycle.snapshot_valid =
      Gm6020_GetSnapshot(&axis->motor, &axis->cycle.snapshot);
  if (axis->cycle.snapshot_valid) {
    axis->cycle.snapshot.feedback.speed_rpm = GimbalAxis_ToLogicalSpeed(
        axis->cycle.snapshot.feedback.speed_rpm, axis->config.motor.speed_sign);
    axis->cycle.limit = axis->cycle.snapshot.limit;
    axis->cycle.feedback_age_ms = GimbalAxis_FeedbackAge(
        now_ms, axis->cycle.snapshot.feedback.last_feedback_tick);
  }

  if (!axis->cycle.snapshot_valid || !axis->cycle.snapshot.online ||
      axis->cycle.feedback_age_ms >= axis->config.motor.feedback_timeout_ms) {
    axis->feedback_lost = axis->controller_initialized;
    axis->cycle.phase = axis->controller_initialized
        ? GIMBAL_AXIS_PHASE_FEEDBACK_LOST : GIMBAL_AXIS_PHASE_WAIT_FEEDBACK;
    axis->cycle.reason = GIMBAL_AXIS_REASON_FEEDBACK_LOST;
    (void)GimbalAxis_SendZero(axis);
    return;
  }
  if (!axis->calibration_valid || !axis->parameters_valid || dt_ms == 0U) {
    axis->cycle.phase = GIMBAL_AXIS_PHASE_WAIT_FEEDBACK;
    axis->cycle.reason = !axis->calibration_valid
        ? GIMBAL_AXIS_REASON_CALIBRATION_INVALID
        : GIMBAL_AXIS_REASON_CONTROL_INVALID;
    (void)GimbalAxis_SendZero(axis);
    return;
  }
  if (command == NULL || !isfinite(command->velocity_permille) ||
      command->velocity_permille < -1000.0f ||
      command->velocity_permille > 1000.0f) {
    axis->cycle.phase = GIMBAL_AXIS_PHASE_HOLD_POSITION;
    axis->cycle.reason = GIMBAL_AXIS_REASON_CONTROL_INVALID;
    (void)GimbalAxis_SendZero(axis);
    return;
  }
  if (fixed_target && (target_angle_raw < axis->config.calibration.min_angle_raw ||
                       target_angle_raw > axis->config.calibration.max_angle_raw)) {
    axis->cycle.phase = GIMBAL_AXIS_PHASE_WAIT_FEEDBACK;
    axis->cycle.reason = GIMBAL_AXIS_REASON_TARGET_INVALID;
    (void)GimbalAxis_SendZero(axis);
    return;
  }

  if (!axis->controller_initialized) {
    GimbalControl_Init(&axis->controller, &axis->config.control,
                       axis->cycle.snapshot.feedback.angle_total_raw);
    axis->controller_initialized = axis->controller.initialized;
    if (!axis->controller_initialized) {
      axis->cycle.reason = GIMBAL_AXIS_REASON_CONTROL_INVALID;
      (void)GimbalAxis_SendZero(axis);
      return;
    }
    axis->cycle.phase = GIMBAL_AXIS_PHASE_HOLD_POSITION;
    (void)GimbalAxis_SendZero(axis);
    return;
  }
  if (axis->feedback_lost) {
    axis->feedback_lost = false;
    GimbalAxis_ResetController(axis);
    axis->cycle.phase = GIMBAL_AXIS_PHASE_HOLD_POSITION;
    (void)GimbalAxis_SendZero(axis);
    return;
  }

  if (fixed_target) {
    (void)GimbalControl_SetTargetAngle(&axis->controller, target_angle_raw);
  }
  GimbalControl_Update(&axis->controller, &axis->cycle.snapshot.feedback,
                       command, (float)dt_ms / 1000.0f, &axis->cycle.control);
  if (axis->cycle.limit == GM6020_LIMIT_MIN || axis->cycle.control.at_min_limit) {
    axis->cycle.limit = GM6020_LIMIT_MIN;
    axis->cycle.reason = GIMBAL_AXIS_REASON_SOFT_MIN;
  } else if (axis->cycle.limit == GM6020_LIMIT_MAX || axis->cycle.control.at_max_limit) {
    axis->cycle.limit = GM6020_LIMIT_MAX;
    axis->cycle.reason = GIMBAL_AXIS_REASON_SOFT_MAX;
  } else if (!command->enabled || command->velocity_permille == 0) {
    axis->cycle.reason = GIMBAL_AXIS_REASON_INPUT_HOLD;
  }

  const int32_t physical_current =
      (int32_t)axis->cycle.control.target_current_raw * axis->config.motor.current_sign;
  const int16_t physical_current_i16 = physical_current > INT16_MAX ? INT16_MAX :
      (physical_current < INT16_MIN ? INT16_MIN : (int16_t)physical_current);
  const bool current_set = Gm6020_SetCurrent(&axis->motor, physical_current_i16);
  const bool enabled = current_set && Gm6020_SetOutputEnabled(&axis->motor, true);
  if (enabled) {
    axis->cycle.output_enabled = true;
    axis->cycle.applied_current_raw = physical_current_i16;
    axis->cycle.can_submitted = Gm6020_Send(&axis->motor);
  } else {
    (void)GimbalAxis_SendZero(axis);
  }

  if (axis->cycle.limit != GM6020_LIMIT_NONE) {
    axis->cycle.phase = GIMBAL_AXIS_PHASE_LIMIT_HOLD;
  } else if (command->velocity_permille != 0) {
    axis->cycle.phase = GIMBAL_AXIS_PHASE_ACTIVE;
  } else {
    axis->cycle.phase = GIMBAL_AXIS_PHASE_HOLD_POSITION;
  }
}
```

## Gm6020_UpdateContinuousAngle

首帧中心对齐；后续反馈回绕展开。

源文件：[Core/Src/bsp/gm6020/gm6020.c](../Core/Src/bsp/gm6020/gm6020.c)，定义始于第 116 行。

```c
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
```

## Gm6020_SendLocked

完整聚合帧保留另一轴槽位；不保证两轴计算同步。

源文件：[Core/Src/bsp/gm6020/gm6020.c](../Core/Src/bsp/gm6020/gm6020.c)，定义始于第 340 行。

```c
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
```

## C610_M2006_HandleRxMessage

C610协议解码与连续角度。

源文件：[Core/Src/bsp/c610_m2006/c610_m2006.c](../Core/Src/bsp/c610_m2006/c610_m2006.c)，定义始于第 399 行。

```c
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
```

## C610_M2006_SendAll

发送前安全门、槽位大端和PRIMASK临界区。

源文件：[Core/Src/bsp/c610_m2006/c610_m2006.c](../Core/Src/bsp/c610_m2006/c610_m2006.c)，定义始于第 482 行。

```c
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
```

## C610_M2006_AngleStep_RunCycle

只读手动测量：首帧基准、连续角度增量、零电流和参数隔离。

源文件：[Core/Src/bsp/c610_m2006/test_c610_m2006_angle_step.c](../Core/Src/bsp/c610_m2006/test_c610_m2006_angle_step.c)，定义始于第 140 行。

```c
void C610_M2006_AngleStep_RunCycle(
    C610_M2006_HandleTypeDef *motor,
    C610_M2006_AngleStepStateTypeDef *state,
    const C610_M2006_AngleStepConfigTypeDef *config,
    uint32_t now_ms,
    uint32_t dt_ms) {
  (void)dt_ms;
  if (motor == NULL || state == NULL || !C610_AngleStepConfigValid(config)) {
    if (state != NULL) {
      C610_AngleStep_Reset(state);
    }
    (void)C610_AngleStep_SendZero(motor);
    return;
  }

  /* 先关输出并提交零电流，再复制一份快照供角度测量和日志共用。
   * 反馈失效时清除基准；离线期间可能漏过多圈，不能把恢复后的差值当成完整位移。 */
  (void)C610_M2006_Process(motor, now_ms);
  const bool submitted = C610_AngleStep_SendZero(motor);
  (void)submitted; /* 日志裁剪仍须提交零电流。 */

  C610_M2006_FeedbackTypeDef feedback = {0};
  if (!C610_M2006_IsOnline(motor) ||
      !C610_M2006_GetFeedback(motor, &feedback)) {
    C610_AngleStep_Reset(state);
    return;
  }

  const int64_t logical_angle = feedback.angle_total_raw * config->feedback_sign;
  const int32_t logical_speed = (int32_t)feedback.speed_rpm * config->feedback_sign;
  if (!state->initialized) {
    state->phase = C610_M2006_ANGLE_STEP_MONITOR;
    state->reference_count = logical_angle;
    state->previous_count = logical_angle;
    state->delta_from_reference = 0;
    state->delta_since_previous = 0;
    state->initialized = true;
  } else {
    state->delta_since_previous = logical_angle - state->previous_count;
    state->previous_count = logical_angle;
    state->delta_from_reference = logical_angle - state->reference_count;
  }

#if LOG_GLOBAL_ENABLE && LOG_USART1_ENABLE && LOG_TEST_ENABLE && LOG_FEED_MOTOR_ENABLE
  (void)C610_AngleStep_Log(state, config, &feedback, logical_angle,
                           logical_speed, submitted, now_ms);
#else
  (void)feedback;
  (void)logical_speed;
#endif
}
```

## FeedMotorControl_Update

单发先预旋、上弹期间持续PWM、停滞确认结束上弹、短按/长按和连发状态机。

源文件：[Core/Src/task/task_feed_motor/task_feed_motor_control.c](../Core/Src/task/task_feed_motor/task_feed_motor_control.c)，定义始于第 136 行。

```c
int16_t FeedMotorControl_Update(FeedMotor_ControlTypeDef *control,
    bool command_enabled, bool button_pressed, bool wheels_ready,
    int64_t angle_count, TickType_t now_tick, uint32_t dt_ms) {
  if (control == NULL || dt_ms == 0U || !command_enabled) {
    if (control != NULL) {
      FeedMotorControl_Stop(control);
    }
    return 0;
  }

  const bool press_rising = button_pressed && !control->press_active;
  FeedMotorControl_UpdatePress(control, button_pressed, dt_ms);

  /* STOP 只接受新的按下沿。释放后的单发在其它阶段继续，不会重新触发。 */
  if (control->phase == FEED_MOTOR_PHASE_STOP) {
    if (press_rising) {
      control->continuous_requested = false;
      FeedMotorControl_StartSpinup(control, angle_count);
    }
    return 0;
  }

  /* 单发阶段不因释放中断；长按只锁存请求，不能跳过预旋、上弹和停滞确认。 */
  if (control->phase == FEED_MOTOR_PHASE_FEED ||
      control->phase == FEED_MOTOR_PHASE_FEED_SETTLE) {
    if (FeedMotorControl_PositionStalled(control, angle_count, dt_ms)) {
      /* 确认完成的同周期停止 C610；C615 已预旋并持续活动，直接开始发射保持。 */
      control->phase = FEED_MOTOR_PHASE_SINGLE_FIRE;
      control->phase_start_tick = now_tick;
      return 0;
    }
    if (control->stall_elapsed_ms == 0U) {
      control->phase = FEED_MOTOR_PHASE_FEED;
    } else {
      control->phase = FEED_MOTOR_PHASE_FEED_SETTLE;
    }
    return (int16_t)(FEED_MOTOR_FEED_CURRENT_RAW * FEED_MOTOR_CURRENT_SIGN);
  }

  if (control->phase == FEED_MOTOR_PHASE_FRIC_SPINUP) {
    if (wheels_ready) {
      /* 两路活动 PWM 已写入才允许上弹；从当前位置重新建立停滞检测基准。 */
      control->phase = FEED_MOTOR_PHASE_FEED;
      control->last_position_count = angle_count;
      control->stall_elapsed_ms = 0U;
      return (int16_t)(FEED_MOTOR_FEED_CURRENT_RAW * FEED_MOTOR_CURRENT_SIGN);
    }
    return 0;
  }

  if (control->phase == FEED_MOTOR_PHASE_SINGLE_FIRE) {
    if ((TickType_t)(now_tick - control->phase_start_tick) >=
        pdMS_TO_TICKS(FEED_MOTOR_SINGLE_FIRE_HOLD_MS)) {
      if (control->continuous_requested && button_pressed) {
        control->phase = FEED_MOTOR_PHASE_CONTINUOUS_FEED;
        return (int16_t)(FEED_MOTOR_FEED_CURRENT_RAW * FEED_MOTOR_CURRENT_SIGN);
      } else {
        control->phase = FEED_MOTOR_PHASE_WAIT_RELEASE;
      }
    }
    return 0;
  }

  if (control->phase == FEED_MOTOR_PHASE_WAIT_RELEASE) {
    if (control->continuous_requested && button_pressed) {
      control->phase = FEED_MOTOR_PHASE_CONTINUOUS_FEED;
      return (int16_t)(FEED_MOTOR_FEED_CURRENT_RAW * FEED_MOTOR_CURRENT_SIGN);
    }
    if (!button_pressed) {
      FeedMotorControl_Stop(control);
    }
    return 0;
  }

  /* 连发不调用停滞判定；只要合法左键仍按住，就同时驱动 C610 和 C615。 */
  if (control->phase == FEED_MOTOR_PHASE_CONTINUOUS_FEED) {
    if (!button_pressed) {
      FeedMotorControl_Stop(control);
      return 0;
    }
    return (int16_t)(FEED_MOTOR_FEED_CURRENT_RAW * FEED_MOTOR_CURRENT_SIGN);
  }

  FeedMotorControl_Stop(control);
  return 0;
}
```

## FeedMotor_RuntimeRunCycle

实际dt、双PWM、许可和M2006输出。

源文件：[Core/Src/task/task_feed_motor/task_feed_motor_runtime.c](../Core/Src/task/task_feed_motor/task_feed_motor_runtime.c)，定义始于第 135 行。

```c
void FeedMotor_RuntimeRunCycle(FeedMotor_RuntimeTypeDef *runtime, uint32_t now_ms, TickType_t now_tick) {
  /* 初始化许可先于任何硬件访问；失败对象不能执行周期。
   * 这里直接返回，停止输出由初始化路径负责，并不再次向未启动通道写寄存器。 */
  if (runtime == NULL || !runtime->initialized) {
    return;
  }
  /* Tick 差值只算周期，再按 configTICK_RATE_HZ 换成 ms。
   * 命令与反馈年龄另用 HAL ms；即使当前都是 1 kHz 也不能直接相减。 */
  const TickType_t elapsed_tick = now_tick - runtime->last_cycle_tick;
  uint32_t dt_ms = runtime->cycle_started ?
      (uint32_t)((uint64_t)elapsed_tick * 1000U / configTICK_RATE_HZ) : FEED_MOTOR_TASK_PERIOD_MS;
  runtime->cycle_started = true;
  runtime->last_cycle_tick = now_tick;
  /* 首周期用配置时长。整 Tick 换算得到 0 ms 时也用配置时长，避免 PID/Ramp 拒绝周期。
   * 64 位乘法保存 elapsed_tick×1000 的中间值，不能先用 32 位乘法再强转。 */
  if (dt_ms == 0U) {
    dt_ms = FEED_MOTOR_TASK_PERIOD_MS;
  }
  /* 命令先零初始化；从未收到命令时保持禁用。反馈先 Process 再复制，
   * 快照成功也必须有首帧标志，不能把全零启动内存当成电机当前位置。 */
  FeedMotor_CommandTypeDef command = {0};
  (void)FeedMotorCommand_GetSnapshot(now_ms, &command);
  (void)C610_M2006_Process(&runtime->motor, now_ms);
  C610_M2006_FeedbackTypeDef feedback = {0};
  /* 在同一短临界区复制首帧标志与全部反馈。64 位计数不能逐字段直接读取（只用这一份）。 */
  taskENTER_CRITICAL();
  const bool valid = C610_M2006_GetFeedback(&runtime->motor, &feedback) && runtime->motor.feedback_received;
  taskEXIT_CRITICAL();
  /* HAL ms 无符号差允许计数回绕。ISR 可能在取 now_ms 后更新出略新的反馈，
   * 大于半周期的下溢按 0 ms 处理；valid=false 仍不能在线，0 ms 时间戳本身不表示离线。 */
  uint32_t age_ms = now_ms - feedback.last_feedback_tick;
  if (age_ms > INT32_MAX) {
    age_ms = 0U;
  }
  const bool online = valid && age_ms < C610_M2006_FEEDBACK_TIMEOUT_MS;
  /* 命令许可与反馈新鲜度共同决定输出。首发先在 FRIC_SPINUP 预旋；进入
   * FEED/FEED_SETTLE 后继续保持两路活动 PWM，避免 C610 上弹时摩擦轮带载起动。
   * 按钮释放不能中断进行中的单发；连发释放当周期清零 C610，并让 C615 目标转为停止值。 */
  const bool permitted = command.enabled && online;
  bool wheels_active = permitted && FeedMotorControl_WheelsActive(&runtime->control);
  if (runtime->control.phase == FEED_MOTOR_PHASE_CONTINUOUS_FEED &&
      !command.fire_requested) {
    wheels_active = false;
  }
  if (!permitted) {
    FeedMotorRuntime_StopWheels(runtime);
  } else {
    const uint16_t target = wheels_active ? FEED_MOTOR_SNAIL_ACTIVE_PULSE_US :
        FEED_MOTOR_SNAIL_STOP_PULSE_US;
    (void)Snail2305_SetTargetPulse(&runtime->left, target);
    (void)Snail2305_SetTargetPulse(&runtime->right, target);
    (void)Snail2305_Process(&runtime->left, dt_ms);
    (void)Snail2305_Process(&runtime->right, dt_ms);
  }
  /* PWM 到达活动脉宽只表示 CCR 已写入目标，不是 C615 实际转速反馈。 */
  const bool wheels_ready = runtime->left.pulse_us == FEED_MOTOR_SNAIL_ACTIVE_PULSE_US &&
      runtime->right.pulse_us == FEED_MOTOR_SNAIL_ACTIVE_PULSE_US;
  const int64_t angle_count = feedback.angle_total_raw * FEED_MOTOR_FEEDBACK_SIGN;
  const int16_t current_raw = FeedMotorControl_Update(&runtime->control, permitted,
      command.fire_requested, wheels_ready, angle_count, now_tick, dt_ms);
  /* 每周期都更新许可和电流，再提交整组 CAN 槽位；反馈失效或阶段停止仍发零。
   * 当前电流非零才允许 C610 输出，单发释放后的 FEED 阶段因此仍可继续完成。 */
  (void)C610_M2006_SetOutputEnabled(&runtime->motor, current_raw != 0);
  (void)C610_M2006_SetCurrent(&runtime->motor, current_raw);
  const bool submitted = C610_M2006_SendAll(&hcan1);
  (void)submitted; /* 关闭日志时仍须提交电流；不能把有硬件副作用的调用放入日志宏。 */
  /* 只打印本周期快照；DMA 忙时保留提交时刻，下周期重试，不缓存旧日志。 */
#if LOG_GLOBAL_ENABLE && LOG_USART1_ENABLE && LOG_TASK_ENABLE && LOG_FEED_MOTOR_ENABLE
  if ((TickType_t)(now_tick - runtime->last_log_tick) >= pdMS_TO_TICKS(FEED_MOTOR_LOG_PERIOD_MS)) {
    char angle_count_text[FEED_MOTOR_COUNT_TEXT_SIZE];
    FeedMotorRuntime_FormatCount(angle_count, angle_count_text);
    if (LOG_TRY_PRINTF(LOG_CATEGORY_FEED_MOTOR, "[供弹] 阶段=%s 左键=%u DBUS许可=%u 反馈有效=%u 在线=%u 年龄(ms)=%lu 角度=%s 转速(rpm)=%d 计算电流=%d 输出许可=%u CAN提交=%u 左PWM(us)=%u 右PWM(us)=%u 停滞窗口(count)=%u 停滞确认(ms)=%lu\r\n",
      FeedMotorControl_PhaseName(runtime->control.phase), command.fire_requested ? 1U : 0U,
      command.enabled ? 1U : 0U, valid ? 1U : 0U, online ? 1U : 0U,
      (unsigned long)age_ms, angle_count_text,
      (int)(feedback.speed_rpm * FEED_MOTOR_FEEDBACK_SIGN), (int)current_raw,
      current_raw != 0 ? 1U : 0U,
      submitted ? 1U : 0U, (unsigned)runtime->left.pulse_us,
      (unsigned)runtime->right.pulse_us, (unsigned)FEED_MOTOR_STALL_COUNTS,
      (unsigned long)runtime->control.stall_elapsed_ms)) {
      runtime->last_log_tick = now_tick;
    }
  }
#endif
}
```

## Snail2305_ConfigValid

TIM1时钟、PSC/ARR和脉宽合法性。

源文件：[Core/Src/bsp/snail_2305/snail_2305.c](../Core/Src/bsp/snail_2305/snail_2305.c)，定义始于第 17 行。

```c
static bool Snail2305_ConfigValid(const Snail2305_ConfigTypeDef *config) {
  if (config == NULL || config->htim == NULL || config->htim->Instance != TIM1 ||
      (config->channel != TIM_CHANNEL_1 && config->channel != TIM_CHANNEL_2) ||
      (config->direction_sign != 1 && config->direction_sign != -1) ||
      config->stop_pulse_us < SNAIL_2305_MIN_PROTOCOL_PULSE_US ||
      config->max_pulse_us > SNAIL_2305_MAX_PROTOCOL_PULSE_US ||
      config->stop_pulse_us >= config->max_pulse_us ||
      !isfinite(config->ramp_us_per_s) || config->ramp_us_per_s <= 0.0f) {
    return false;
  }
  const uint32_t pclk_hz = HAL_RCC_GetPCLK2Freq();
  const uint32_t timer_hz = (RCC->CFGR & RCC_CFGR_PPRE2) == 0U ? pclk_hz : 2U * pclk_hz;
  return timer_hz == 1000000U * (config->htim->Instance->PSC + 1U) &&
      config->htim->Instance->ARR == 19999U;
}
```

## Snail2305_Process

浮点Ramp转整数CCR；不是实际转速。

源文件：[Core/Src/bsp/snail_2305/snail_2305.c](../Core/Src/bsp/snail_2305/snail_2305.c)，定义始于第 142 行。

```c
bool Snail2305_Process(Snail2305_HandleTypeDef *motor, uint32_t dt_ms) {
  if (!Snail2305_IsInitialized(motor) || dt_ms == 0U) {
    return false;
  }
  /* 将 ms 换成 s 后按 us/s 推进 Ramp。CCR 预装载到下一次 PWM 更新才生效，最迟 20 ms。
   * pulse_us 只记录最近写值；不能据此断言电调转速已到位。 */
  const float pulse = Ramp_Update(&motor->ramp, (float)motor->target_pulse_us,
                                 motor->config.ramp_us_per_s, (float)dt_ms * 0.001f);
  motor->pulse_us = (uint16_t)lroundf(pulse);
  __HAL_TIM_SET_COMPARE(motor->config.htim, motor->config.channel, motor->pulse_us);
  return true;
}
```

## Log_TryPrintf

原子所有权、上下文、格式化长度和DMA失败清理。

源文件：[Core/Src/app/log/log.c](../Core/Src/app/log/log.c)，定义始于第 29 行。

```c
bool Log_TryPrintf(const char *fmt, ...) {
#if LOG_GLOBAL_ENABLE && LOG_USART1_ENABLE
  /* 先拒绝 ISR，再取得所有权。格式化不占临界区，也不覆盖仍在发送的数据。 */
  if (fmt == NULL || __get_IPSR() != 0U ||
      __atomic_exchange_n(&g_log_tx_busy, true, __ATOMIC_ACQUIRE)) {
    return false;
  }
  if (huart1.gState != HAL_UART_STATE_READY) {
    __atomic_store_n(&g_log_tx_busy, false, __ATOMIC_RELEASE);
    return false;
  }
  va_list args;
  va_start(args, fmt);
  const int length = vsnprintf((char *)g_log_tx_buffer, sizeof(g_log_tx_buffer), fmt, args);
  va_end(args);
  /* 使用 int 检查格式化结果。超长文本整条拒绝，不发送被截断的中文。 */
  if (length <= 0 || length >= (int)sizeof(g_log_tx_buffer) ||
      HAL_UART_Transmit_DMA(&huart1, g_log_tx_buffer, (uint16_t)length) != HAL_OK) {
    __atomic_store_n(&g_log_tx_busy, false, __ATOMIC_RELEASE);
    return false;
  }
  return true;
#else
  (void)fmt;
  return false;
#endif
}
```
