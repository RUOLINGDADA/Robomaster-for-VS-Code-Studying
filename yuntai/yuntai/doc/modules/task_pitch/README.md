# Pitch 发射云台任务

Pitch 是纵向轴，默认 GM6020 反馈 ID `0x206`。使用独立 `PitchTask_RuntimeTypeDef` 与
`task_pitch_config.h`，共用 `app/gimbal` 控制、保护和驱动同级硬件测试，不复制 Yaw 算法。
任务入口仍是 CubeMX 已创建的 `task_pitch_entry`；每 2 ms 只调用一个模式。

## 用户操作

- 默认 `PITCH_HARDWARE_TEST_MODE_CALIBRATION`：烧录后零电流，串口提示中位、下限位、上限位记录连续角度。
- 把三组实测值写入 `PITCH_CALIBRATION_CENTER/MIN/MAX_ANGLE_RAW`；min/max 按计数排序，留安全余量；确认后 `PITCH_CALIBRATION_VALID=1`。
- 切换 `PITCH_HARDWARE_TEST_MODE_ANGLE_LOOP`，设 `PITCH_ANGLE_LOOP_TARGET_ANGLE_DEG`，默认 0°，运行正式控制链验证到位、保持和拨离恢复。
- 调整 Pitch 自己的 PID、限流、Ramp 和保护参数；确认后共用运行时继续使用同一配置。
- `PITCH_HARDWARE_TEST_MODE_OFF`：正式输入尚未接入，仍注册本轴并持续零电流，不自动施加保持或重力补偿力矩。

## 恢复与日志

固定目标堵转后，手动向触发方向反向移开 `PITCH_RELEASE_POSITION_DELTA_RAW`（默认 114 计数，
约 5°），范围内且反馈电流下降，连续确认 `PITCH_RELEASE_CONFIRM_MS`（默认 100 ms）才解除。
掉线不清除堵转，恢复及解除先零输出；参数仅为台架起点，Pitch 负载需单独验证。

标定、角度测试和正式参数均来自本轴配置；反馈与控制日志使用同周期快照。
同一 CAN 帧聚合时保留彼此电流槽位，UART DMA 忙时不会阻塞任务。
电机 ID、上下方向、边界、负载重力影响和长期温升仍待实测。
