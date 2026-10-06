# CubeMX 重新生成后的人工步骤

`.ioc` 已保存 TIM1 PWM、PE9/PE11、计数频率和现有 FreeRTOS 配置。以下项目依赖硬件接线或 USER CODE，CubeMX 不能完整保存。每次重新生成后按顺序检查。

1. 在 Pinout 中确认 PE9 为 `TIM1_CH1`，PE11 为 `TIM1_CH2`。确认两根线分别接左、右 C615 信号输入。
2. 校准 C615 的 PWM 行程并设置电机方向，按下节步骤操作。软件配置行程为 1000~2000 us，发射活动值为 1800 us；活动值与最大校准行程是两个参数。校准后同步修改 `snail_2305.h` 的脉宽宏。
3. 确认 `MX_TIM1_Init()` 在 `main()` 的 FreeRTOS 初始化之前执行。TIM1 不启用中断；PWM 由 `task_feed_motor` 在任务上下文启动和更新。
4. 保留 `main.c` 的 CAN 过滤器、`HAL_CAN_Start()` 和 FIFO0 pending 通知 USER CODE。CubeMX 只负责 CAN 外设初始化，不能替代这些业务调用。
5. 保留 `stm32f4xx_it.c` 的 USART3 专用 DBUS IDLE、UART 错误和 DMA 双缓冲处理。不能恢复普通 `HAL_UART_IRQHandler(&huart3)`，否则会与 DBUS 双缓冲状态机争用。
6. 保留供弹任务中的 `HAL_TIM_PWM_Start()`、停止脉宽写入和 `C610_M2006_SendAll()` 调用。CubeMX 只能生成 TIM1 初始化，不能保存业务启停和安全归零逻辑。
7. 生成完成后检查 `Core/Inc/tim.h`、`Core/Src/tim.c`、TIM1 MSP 初始化和 `cmake/stm32cubemx/CMakeLists.txt` 的 `tim.c` 源文件项。
8. 检查 `.ioc` 的 `FREERTOS.Tasks01` 仍包含 `task_feed_motor`、`task_yaw`、`task_pitch`、`task_dbus`，并保持原优先级、栈大小和 `configTOTAL_HEAP_SIZE=40960`。
9. 执行 `git diff --check`，再执行 `cmake --preset Debug` 和 `cmake --build --preset Debug`。确认生成操作没有删除 USER CODE、`bsp/`、`task/` 或顶层 CMake 源项。

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
4. 重新连接 PE9/PE11 运行信号与共同 GND，先确认 1000 us 停止，再验证 1800 us 活动
   脉宽和左右轮方向。本项目不提供转速反馈，仍需实测温升和供弹条件。

## 配置保存边界

- `.ioc` 保存引脚、TIM1 PWM、时钟、中断和 FreeRTOS 四任务配置。
- `main.c` 的 USER CODE 保存 CAN 过滤器、启动和 FIFO0 通知；`stm32f4xx_it.c` 的
  USER CODE 保存 USART3 专用 DBUS IRQ。双缓冲及 IDLE 业务接收机制不能由 GUI 完整配置。
- 独立 BSP/task 文件保存 PWM 启动、Ramp、鼠标输入、命令超时和供弹状态机。顶层
  `CMakeLists.txt` 保存这些非生成源文件，生成子列表由 CubeMX 保存 `tim.c`。

## 重新生成验证记录

2026-10-06 使用 STM32CubeMX 6.18.1、F4 V1.28.3 在临时副本实际执行 load/save/generate，
生成成功。采用该次输出的 `.ioc`、`tim.c/h`、MSP、`main.c` 和生成 CMake 子列表。
TIM1 MSP 位于 `tim.c`，无需再手动向 `stm32f4xx_hal_msp.c` 添加重复实现。
`freertos.c`、`FreeRTOSConfig.h`、CAN/USART3 参数和 DBUS 中断 USER CODE 保持一致。

## CubeMX 无法确认的硬件项目

- PE9/PE11 的实际 C615 接线和左右轮对应关系。
- C615 方向、校准行程、1800 us 温升和持续运行时间。
- M2006 的 `feedback_sign`、`current_sign`、409 count 步长和机械急停范围。
- 拨弹盘每发是否对应 409 个 M2006 电机轴编码器 count。

这些项目必须在可急停台架上完成。C615 没有本驱动使用的 CAN 反馈；PWM 启动成功不能证明摩擦轮已达到目标转速。
