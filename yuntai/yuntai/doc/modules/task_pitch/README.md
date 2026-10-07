# Pitch 发射云台任务

Pitch 是纵向轴，默认 GM6020 反馈 ID `0x206`。使用独立 `PitchTask_RuntimeTypeDef` 与
`task_pitch_config.h`，共用 `app/gimbal` 控制和驱动同级硬件测试，不复制 Yaw 算法。
任务入口仍是 CubeMX 已创建的 `task_pitch_entry`；每 2 ms 只调用一个模式。

## 用户操作

- 当前配置默认 `PITCH_HARDWARE_TEST_MODE_OFF`：摇杆与鼠标正式控制；首次标定或需要安全停机时应显式切换到 `CALIBRATION`（只读零电流）。
- 把三组实测值写入 `PITCH_CALIBRATION_CENTER/MIN/MAX_ANGLE_RAW`；min/max 按计数排序，留安全余量；确认后 `PITCH_CALIBRATION_VALID=1`。
- 切换 `PITCH_HARDWARE_TEST_MODE_ANGLE_LOOP`，设 `PITCH_ANGLE_LOOP_TARGET_ANGLE_DEG`，默认 0°，运行正式控制链验证到位、保持和拨离恢复。
- 调整 Pitch 自己的 PID、限流和 Ramp 参数；确认后共用运行时继续使用同一配置。
- 当前可调参起点（以 `task_pitch_config.h` 为准）：位置 `KP=0.05 rpm/count`、`KI=2.0 rpm/(count·s)`、`KD=0`、积分上限 `5 rpm`，速度 `KP=200 raw/rpm`、`KI=20 raw/(rpm·s)`、`KD=0`，正式输入满量程 `40 rpm`、最大目标速度 `70 rpm`，最大电流 `7000` 原始值，Ramp `60000 raw/s`，速度滤波权重 `0.15`。固定目标的速度前馈为 0；正式模式叠加 DBUS 右摇杆上下与鼠标 Y。
- `PITCH_HARDWARE_TEST_MODE_OFF`：使用 DBUS 正式输入；未完成标定或 CAN 反馈超时持续零电流；遥控命令过期和合法回中按零速度保持最后目标位置，并继续计算独立重力补偿。

## 恢复与日志

Pitch 不再有动态堵转候选、锁存或固定限位电流。达到或越过边界时只忽略继续向外的摇杆或鼠标速度，现有闭环继续保持最后目标位置；反向输入同周期积分目标。反馈掉线仍清零，恢复后只清控制历史，保留最后目标。

标定、角度测试和正式参数均来自本轴配置；反馈与控制日志使用同周期快照。
同一 CAN 帧聚合时保留彼此电流槽位，UART DMA 忙时不会阻塞任务。
电机 ID、上下方向、边界、负载重力影响和长期温升仍待实测。

抗重力参数位于 `task_pitch_config.h`，`PITCH_GRAVITY_COMPENSATION_ENABLE` 默认是 1；开启后
`gravity_current = current_sign*(bias + amplitude*cos(angle_offset))`，`angle_offset` 是相对 8753 的连续计数并换算为弧度。
当前偏置为 `4500`、幅值为 `1500`，单位都是 GM6020 raw；补偿方向由独立的 `PITCH_GRAVITY_CURRENT_SIGN` 控制。

推荐调参顺序：先确认 `PITCH_MOTOR_CURRENT_SIGN=+1` 和 `PITCH_FEEDBACK_SPEED_SIGN=+1` 的方向关系；再用较小位置 P 调整响应，随后调整速度 P；确认目标速度和电流不频繁触顶后，再逐步调整 Ramp、滤波、位置/速度 I、位置 D 和重力幅值。每轮只改一组参数。
判断稳定性的依据是目标附近误差收敛、目标速度和合成电流不持续饱和、请求电流、保护后电流和 CAN 提交结果可解释；HAL 接受不能证明电调实际执行；若出现来回摆动，先降低位置 P、速度 P 或目标速度上限，不要先增大抗重力电流。
当前没有动态卡死检测或锁存。反馈超时只检测通信中断；卡住的电机仍可能持续上报反馈。软件边界过滤和 `PITCH_MAX_CURRENT_RAW` 限幅不能保证卡死时停机。边界处只忽略继续向外的输入和前馈，最后目标不改写；反向输入同周期生效。越界反馈仍按普通位置/速度误差计算，物理限位必须由急停台架验证。
