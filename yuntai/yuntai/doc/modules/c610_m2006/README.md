# C610 + M2006 CAN 驱动

## 模块职责

该模块面向 RoboMaster C610 无刷电机调速器和 M2006 电机组合，提供：

- 一个 CAN 总线上最多 8 个设备的静态注册；
- 标准 CAN 反馈帧过滤和字段解析；
- 目标电流缓存及 `0x200`/`0x1FF` 聚合控制帧发送；
- 反馈超时后的离线状态；
- `feedback_received` 首帧标志，避免初始化零时间戳造成假在线；
- 面向上层控制任务的反馈快照读取。

它不负责 CubeMX 的 CAN 外设初始化、GPIO 复用、过滤器配置、FreeRTOS 任务创建、PID 或云台业务控制。

当前 `.ioc` 已启用 CAN1，CubeMX 生成的 HAL 配置定义 `HAL_CAN_MODULE_ENABLED`，并将
`stm32f4xx_hal_can.c` 加入 CMake。驱动仍保留未启用 CAN 时的失败桩，便于独立阅读和
移植；失败桩不会伪装成设备可用。

## 文件

```text
Core/Inc/bsp/c610_m2006/c610_m2006.h  # 公共类型、协议常量和 API
Core/Src/bsp/c610_m2006/c610_m2006.c  # 静态注册表、解析、发送和状态处理
```

## 协议依据

协议字段依据 `参考文档/markdown/C610_-----------.md`、M2006 说明书、RoboMaster CAN
教程和现有 `can_example` C620 驱动整理。C610 官方手册明确规定控制转矩电流值范围为
`[-10000, 10000]`，对应电调输出约 `[-10 A, 10 A]`；该范围不能与 C620/3508 或
GM6020 的电流给定范围混用。
标准反馈帧格式如下：

| 字节 | 含义 | 类型 | 说明 |
|---|---|---|---|
| `DATA[0:1]` | 机械角度 | `uint16_t` | 大端，驱动取低 13 位，`0~8191` 对应一圈 |
| `DATA[2:3]` | 转速 | `int16_t` | 大端，单位通常为 rpm |
| `DATA[4:5]` | 实际电流/转矩电流 | `int16_t` | 大端，电调原始量 |
| `DATA[6]` | 保留/空 | `uint8_t` | 官方手册标为空，驱动只保存原始字节 |
| `DATA[7]` | 电调错误码 | `uint8_t` | `0` 无异常；`2` 过压、`3` 三相线未接、`4` 位置传感器信号丢失、`6` 堵转、`7` 校准失败 |

反馈标准帧 ID 为 `0x201` 到 `0x208`，计算方式是 `0x200 + motor_id`。发送时：

- ID 1 至 4 使用控制帧 `0x200`，每个电机占两个字节；
- ID 5 至 8 使用控制帧 `0x1FF`，每个电机占两个字节；
- 每个电流值按高字节在前、低字节在后的大端顺序发送。

驱动不把 `DATA[6]` 当温度；C610 手册给出了 `DATA[7]` 的错误码定义，但为兼容不同固件，仍保存原始字节，不在底层强行转换为枚举。

## 供弹控制约束

正式 `task_feed_motor` 使用 CAN1 ID 1（反馈 `0x201`，控制 `0x200`）。反馈在每个 CAN
帧到达时展开为 64 位连续角度，回绕阈值为半圈 4096 count；任务通过快照读取角度、速度、
反馈时间和保留字节。命令或反馈超过 100 ms 时，驱动清除输出许可和目标电流，聚合帧对应
槽位发送零。恢复后由供弹控制层以当前位置重建下一发目标，避免追踪掉线期间的旧角度。
相邻已收到的帧之间真实转动必须小于半圈；丢帧或掉线期间超过半圈的转动无法只靠 13 位
单圈编码器唯一恢复，连续角度可能少计或多计整圈。正式步进在恢复后按当前位置重新起步。

`DATA[6]` 只保存为 `reserved_raw`，`DATA[7]` 只保存为 `error_code`。驱动不将保留字节
当作温度，也不实现堵转判断。

## 接入顺序

1. 在 CubeMX 中启用目标 CAN 实例和对应 TX/RX GPIO。
2. 配置 CAN 位时序。参考工程使用 `Prescaler=3`、`BS1=10TQ`、`BS2=3TQ`、`SJW=1TQ`，实际波特率必须按当前 APB1 时钟和硬件总线确认。
3. 配置过滤器接收目标标准 ID，并启动 CAN 和 FIFO0 pending 中断。
4. 在普通初始化代码中注册句柄：

```c
C610_M2006_HandleTypeDef motor;
C610_M2006_ConfigTypeDef config = {
    .hcan = &hcan1,
    .motor_id = 1U,
    .feedback_timeout_ms = 100U,
};

(void)C610_M2006_Init(&motor, &config);
```

5. 在 `HAL_CAN_RxFifo0MsgPendingCallback` 中只取帧并交给驱动：

```c
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan) {
  (void)C610_M2006_RxFifoCallback(hcan, CAN_RX_FIFO0);
}
```

6. 在固定周期的 FreeRTOS 控制任务中设置目标、检查超时并发送：

```c
C610_M2006_SetOutputEnabled(&motor, true);
C610_M2006_SetCurrent(&motor, target_current_raw);
C610_M2006_Process(&motor, HAL_GetTick());
C610_M2006_SendAll(&hcan1);
```

发送周期应由上层控制需求决定。对于电流控制，通常需要稳定的毫秒级周期；驱动本身不创建任务，也不保证调用者的周期。

## 上下文和并发

- `C610_M2006_RxFifoCallback` 可在 HAL CAN FIFO 中断回调中调用，必须保持短小，不打印、不阻塞、不调用普通 FreeRTOS API。
- `C610_M2006_SetCurrent`、`C610_M2006_Process`、`C610_M2006_SendAll` 应在普通任务或主循环调用。
- `feedback_received` 由 CAN ISR 写入、任务读取；`feedback_sequence` 用奇偶序列号包住反馈字段更新，任务通过 `C610_M2006_GetFeedback` 只接受前后相同的偶数序列号，避免角度/速度/电流来自不同帧。
- `C610_M2006_GetFeedback` 连续多次遇到 ISR 更新会返回 `false`，调用者应保留安全默认值或下一周期重试，不能使用不完整结构体。
- `C610_M2006_SendAll` 只聚合同一个 `CAN_HandleTypeDef` 的设备，避免把 CAN1 和 CAN2 的目标电流放到同一帧。

## 安全行为

- 输出未使能时，聚合帧中该设备对应的两个字节强制为零。
- 未收到过合法反馈，或反馈超时后，设备进入 `C610_M2006_STATE_OFFLINE`，目标电流清零并关闭输出；只有 `feedback_received=true` 且未超时才报告在线。
- 电流输入超出 `[-10000, 10000]` 时饱和到边界，避免超过 C610 官方手册给出的
  M2006/C610 供弹电流范围；不能把 C620/3508 的 `[-16384, 16384]` 限值套到 C610。
- 发送邮箱不足或 HAL 发送失败时返回 `false`，不伪造“发送成功”状态。

## 已知限制和待确认项

- 当前 CAN1 已启用并在 USER CODE 中启动。总线终端电阻、供电、ID 和方向仍待台架确认。
- C610 的实际额定电流、保护阈值、`DATA[7]` 错误码和电机方向约定需根据使用的 C610 固件、接线和测试结果确认。这里的 `[-10000, 10000]` 是官方协议输入范围，不等同于允许长期输出的机械安全电流。
- 当前驱动不含 PID、堵转判定、温度保护回调、日志和 FreeRTOS 任务，这些属于上层功能模块。
- HAL CAN 接收过滤器应覆盖 `0x201` 至 `0x208`，具体 16/32 位掩码配置需与使用 CAN 实例和其它设备共享过滤器的方案一起确定。

## 硬件调参测试

永久测试代码位于驱动同级的
`Core/Inc/bsp/c610_m2006/test_c610_m2006_self_cycle.h` 和
`Core/Src/bsp/c610_m2006/test_c610_m2006_self_cycle.c`。

它只保留上板真正需要的上弹、停止、下弹、停止循环。用户修改头文件中的阶段时间和
`FEED_MOTOR_TEST_*_CURRENT_RAW` 参数，任务入口直接调用
`C610_M2006_TestSelfCycle_Run()`。反馈无效时先强制零输出；函数每次只执行一个非阻塞
控制步。协议字段、ID 过滤和电流钳位由 agent 使用临时脚本或静态检查验证，不建立永久
合成测试目录。

正式鼠标供弹使用 `task_feed_motor_config.h` 的角度步长、到位和电流配置。自循环宏仅在
`FEED_MOTOR_TEST_ENABLE=1` 时生效，不能把往返测试时间当作每发供弹角度。

自循环参数的唯一默认来源为 `Core/Inc/task/task_feed_motor/task_feed_motor_config.h`。
任务通过 `C610_M2006_TestConfigTypeDef` 传入时序、电流、日志周期；BSP 不反向包含任务头。
