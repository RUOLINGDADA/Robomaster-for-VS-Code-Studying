# 重力补偿算法

文件位于 `Core/Inc/algorithm/gravity_compensation/` 和
`Core/Src/algorithm/gravity_compensation/`。模块只接收连续编码器角度，返回
GM6020 电流 raw；不访问 CAN、FreeRTOS、UART 或目标位置。

`GravityCompensation_Init()` 复制并校验每轴配置。`GravityCompensation_Update()` 使用：

```text
angle_offset = (angle_total_raw - center_angle_raw) * 2π / 8192
current = current_sign * (bias_current_raw + amplitude_current_raw * cos(angle_offset))
```

角差先用 int64 求差，再按 8192 count 周期取余后换算弧度；余弦值不变，避免大连续计数转 float 后丢失小角差。模块检查结果有限性并按 `±max_current_raw` 限幅。云台控制器随后把该结果与位置/速度闭环电流相加，再按轴总电流上限限幅并经过 Ramp。反馈超时路径不会调用该模块，直接发送零电流。

Yaw 和 Pitch 共享同一模块接口。Yaw 配置为禁用并输出 0；Pitch 默认启用，当前起点为偏置 `4500 raw`、幅值 `1800 raw`、补偿方向 `+1`。方向与 `PITCH_MOTOR_CURRENT_SIGN` 分开，必须在急停台架确认。

该算法不移动目标位置。遥控速度只在 `gimbal_control` 中积分为最后目标；命令超时后目标冻结，闭环和补偿仍按当前反馈周期计算。
