# 通用控制算法

## 文件

```text
Core/Inc/algorithm/pid/pid.h
Core/Src/algorithm/pid/pid.c
Core/Inc/algorithm/ramp/ramp.h
Core/Src/algorithm/ramp/ramp.c
Core/Inc/algorithm/filter/low_pass_filter.h
Core/Src/algorithm/filter/low_pass_filter.c
Core/Inc/algorithm/gravity_compensation/gravity_compensation.h
Core/Src/algorithm/gravity_compensation/gravity_compensation.c
Core/Inc/algorithm/mouse_virtual_joystick/mouse_virtual_joystick.h
Core/Src/algorithm/mouse_virtual_joystick/mouse_virtual_joystick.c
```

这些文件只处理数值，不依赖 GM6020、CAN、FreeRTOS 或串口。模块使用者必须在接口外保证单位一致、控制周期真实且对象只在一个控制上下文中更新。

## PID

`Pid_Update()` 接收当前误差和秒单位 `dt_s`，计算比例、积分、微分并分别执行输出限幅和积分限幅。输出达到上/下限且误差仍推动它继续饱和时，积分暂停；误差反向、有助于离开饱和时允许积分释放。两轴 D 项由独立任务配置选择；对量化速度再次微分会放大噪声。

## Ramp

`Ramp_Update()` 以“每秒最大变化量”为限制，逐步追踪目标。Yaw 使用它限制电流换向斜率，避免遥控器一帧内改变方向时电流瞬时跳变；保护释放后从零建立输出也因此更平滑。`dt_s` 为零或变化率非法时返回安全值，不会修改未定义状态。

## 一阶低通

`LowPassFilter_Update()` 使用 `y += alpha * (x-y)`。`alpha=1` 等于不滤波，接近 0 表示响应慢。两轴分别维护实测速度滤波对象；滤波不能替代反馈新鲜性判断，也不能单独作为堵转结论。

## 验证边界

`mouse_virtual_joystick` 将鼠标位移按帧序列去重并累加为速度，X/Y 分别保持和线性回中。
调用者传入毫秒时刻及接收帧序号，算法没有外设依赖。接口、默认值与失效规则见
`doc/modules/mouse_virtual_joystick/README.md`。

`gravity_compensation` 是连续角度到电流的独立算法模块。它使用
`bias + amplitude*cos(angle_offset)`，执行独立电流方向和模块限幅，不改变云台目标位置。
Pitch 默认启用，Yaw 使用同一 interface 但禁用；云台控制器只负责把补偿电流与位置/速度闭环结果合成。
详细参数、单位和验证边界见 `doc/modules/algorithm/gravity_compensation.md`。

算法可在主机上用合成序列测试：零误差、正负阶跃、输出饱和、dt 变化、Ramp 反向和滤波阶跃。算法通过不代表实际电流环稳定；PID 增益、Ramp 速率和保护阈值仍需结合电机、减速箱和负载台架标定。
