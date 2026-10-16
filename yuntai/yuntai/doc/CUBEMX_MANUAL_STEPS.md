# CubeMX 重新生成后的人工步骤

`.ioc` 已保存 TIM1 PWM、PE9/PE11、计数频率和现有 FreeRTOS 配置。以下项目依赖硬件接线或 USER CODE，CubeMX 不能完整保存。每次重新生成后按顺序检查。

1. 在 Pinout 中确认 PE9 为 `TIM1_CH1`，PE11 为 `TIM1_CH2`。确认两根线分别接左、右 C615 信号输入。
2. 使用 DJI Assistant 2 或外部安全 PWM 工具校准 C615 的 PWM 行程并设置电机方向。协议允许 400~2200 us；当前正式配置停止值为 1000 us、活动值为 1400 us、运行上限为 1550 us。活动值与最大校准行程是两个参数，软件已删除 C615 校准模式。校准后同步修改 `task_feed_motor_config.h` 的脉宽宏。
3. 确认 `MX_TIM1_Init()` 在 `main()` 的 FreeRTOS 初始化之前执行。TIM1 不启用中断；PWM 由 `task_feed_motor` 在任务上下文启动和更新。
4. 确认 CAN2 为 PB5 RX/PB6 TX、CAN1 为 PD0 RX/PD1 TX，均为 1 Mbit/s、ABOM 使能；两个 `MX_CAN*_Init()` 在 USER CODE 启动前执行，对应 RX0 IRQ 调用各自 `HAL_CAN_IRQHandler()`。保留 `main.c` USER CODE 中 CAN1 bank0 的 16 位 ID 列表 `0x201~0x204` 与 CAN2 bank14 的全零掩码、双 CAN Start 和 FIFO0 pending 通知；两个分界值均为 14。双 CAN MSP 保留 CAN1 时钟依赖，接收回调只保留统一分发入口。CubeMX 只负责外设初始化，不能替代业务调用。
5. 保留 `stm32f4xx_it.c` 的 USART3 专用 DBUS IDLE、UART 错误和 DMA 双缓冲处理。不能恢复普通 `HAL_UART_IRQHandler(&huart3)`，否则会与 DBUS 双缓冲状态机争用。
6. 保留供弹任务中的 `HAL_TIM_PWM_Start()`、停止脉宽写入和 `C610_M2006_SendAll()` 调用。CubeMX 只能生成 TIM1 初始化，不能保存业务启停和安全归零逻辑。
7. 生成完成后检查 `Core/Inc/tim.h`、`Core/Src/tim.c`、TIM1 MSP 初始化和 `cmake/stm32cubemx/CMakeLists.txt` 的 `tim.c` 源文件项。
8. 检查 `.ioc` 的 `FREERTOS.Tasks01` 仍包含 `task_feed_motor`、`task_yaw`、`task_pitch`、`task_dbus`、`task_chassis`，并保持各任务优先级、栈大小和 `configTOTAL_HEAP_SIZE=40960`。
9. 执行 `git diff --check`，再执行 `cmake --preset Debug` 和 `cmake --build --preset Debug`。确认生成操作没有删除 USER CODE、`bsp/`、`task/` 或顶层 CMake 源项。

## 本轮只新增一个底盘任务

用户已在 FreeRTOS → Tasks and Queues 创建以下唯一底盘任务，`.ioc/freertos.c` 已生成对应属性、一次 `osThreadNew` 和弱入口。重新生成时保留本表及原四个任务属性和 40960 bytes 堆，不重复新增或添加电机/模式子任务：

| CubeMX 字段 | 填写值 |
|---|---|
| Task Name | `task_chassis` |
| Entry Function | `task_chassis_entry` |
| Priority | `osPriorityNormal2`（26） |
| Stack Size | `1024` words（4096 bytes） |
| Code Generation Option | `As weak` |
| Allocation | `Dynamic` |
| Parameter | `NULL` |

入口自行用 `vTaskDelayUntil()` 每 2 ms 调度，GUI 中不用增加定时器。生成后确认 `freertos.c` 的 `osThreadNew(task_chassis_entry,NULL,&task_chassis_attributes)`、`.stack_size=1024*4`、Normal2 和 `__weak task_chassis_entry`；业务强入口位于 `Core/Src/task/task_chassis/task_chassis.c`，不可再手写第二个创建调用。用 ELF `arm-none-eabi-nm` 检查 `task_chassis_entry` 为 `T` 而非 `W`，调试断点确认进入独立任务文件而不是弱兜底。

生成后五个业务任务栈合计 32768 bytes，动态堆 40960 bytes 还须承担 TCB、Idle/Timer、队列等。检查底盘任务句柄非 NULL、`xPortGetFreeHeapSize()`、`xPortGetMinimumEverFreeHeapSize()` 与 `uxTaskGetStackHighWaterMark()`，高水位单位为 words；链接 RAM 占用不代表动态堆余量已验证。测量实际周期、最坏执行时间和栈余量后再调整，先保留当前堆配置。

USART1 保留用户已有 9600 baud，重新生成后检查 `.ioc` 与 `usart.c` 一致；底盘日志默认关闭。先架空核对 ID1右前/ID2左前/ID3左后/ID4右后及 -/+/+/-，验证 Ctrl 循环、长按去重、重连释放、两种运动输入、单轮掉线与 CAN1 Bus-Off，再验证云台/供弹并行。上述调度和硬件项目尚未实测。

## C615 行程与方向设置

依据 C615 V1.0（2019.10）说明书第 4 页，先在可急停台架固定电机，使用 DJI Assistant 2
或独立 PWM 校准工具。正式鼠标供弹程序只执行运行脉宽，不执行校准时序。

1. 使用 DJI Assistant 2 时，按手册将电调 PWM/RX、TX 和 GND 接到 USB 转串口工具，
   为电调供电。在 ESC 的“设置”页设置 PWM 行程和电机转向。保持通信和供电直至设置完成。
2. 使用 PWM 校准时，先让独立工具输出最大行程 2000 us，再为已连接电机的电调上电。
   电机会交替发出 BB 和 BBB，间隔 2 秒。在 BB 声后的 2 秒内改为最小行程 1000 us，
   等待约 1 秒 B 声确认完成。实际行程必须与软件宏一致。
3. 左右轮的转向通过 Assistant 设置或交换电机任意两根相线。使用 PWM 切换转向时，
   先在 Assistant 开启“快速设置转向”，在 BBB 声后的 2 秒内改为最小行程并等待确认声。
4. 重新连接 PE9/PE11 运行信号与共同 GND，先确认 1000 us 停止，再验证正式 1400 us 活动和 1550 us 上限
   脉宽和左右轮方向。本项目不提供转速反馈，仍需实测温升和供弹条件。

## 配置保存边界

- `.ioc` 保存引脚、TIM1 PWM、时钟、中断和当前 FreeRTOS 五任务配置，其中底盘任务由用户通过 CubeMX 创建。
- `main.c` 的 USER CODE 保存 CAN 过滤器、启动和 FIFO0 通知；`stm32f4xx_it.c` 的
  USER CODE 保存 USART3 专用 DBUS IRQ。双缓冲及 IDLE 业务接收机制不能由 GUI 完整配置。
- 独立 BSP/task 文件保存 PWM 启动、Ramp、鼠标输入、命令超时和供弹状态机。顶层
  `CMakeLists.txt` 保存这些非生成源文件，生成子列表由 CubeMX 保存 `tim.c`。

## 重新生成验证记录

2026-10-16 用户创建 `task_chassis` 后，检查 `.ioc/freertos.c`：Normal2/26、1024 words、As weak、Dynamic、NULL，只有一次创建调用；原四任务、40960 bytes 堆、双 CAN USER CODE 与 USART1 9600 baud 保留。普通 Debug 配置/构建通过（RAM 49800 B，Flash 75180 B），无强保留链接参数；ELF 中 `task_chassis_entry` 为 T，独立底盘运行时及控制器均保留。此记录验证配置和链接，不代表 `osThreadNew` 运行成功、2 ms 实际调度、堆/栈高水位或轮子已上板验证。

2026-10-06 使用 STM32CubeMX 6.18.1、F4 V1.28.3 在临时副本实际执行 load/save/generate，
生成成功。采用该次输出的 `.ioc`、`tim.c/h`、MSP、`main.c` 和生成 CMake 子列表。
TIM1 MSP 位于 `tim.c`，无需再手动向 `stm32f4xx_hal_msp.c` 添加重复实现。
`freertos.c`、`FreeRTOSConfig.h`、CAN/USART3 参数和 DBUS 中断 USER CODE 保持一致。

## CubeMX 无法确认的硬件项目

- PE9/PE11 的实际 C615 接线和左右轮对应关系。
- C615 方向、校准行程、正式 1400 us/测试 1520 us/上限 1550 us 的温升和持续运行时间。
- M2006 的 `feedback_sign`、`current_sign`、当前 +4000 count 每发角度、200 ms 零电流停顿和机械急停范围。
- 到目标后零电流的惯性超调、卡弹风险和连续供弹能力。

这些项目必须在可急停台架上完成。C615 没有本驱动使用的 CAN 反馈；PWM 启动成功不能证明摩擦轮已达到目标转速。
