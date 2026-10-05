# DBUS 遥控输入

`Core/Inc|Src/bsp/dbus` 使用 USART3 PC11、100000 baud、9B+偶校验、DMA1 Stream1
双缓冲接收 18 字节 DJI DBUS 帧。9B 包含 8 位数据和 1 位校验；配置成 8B+偶校验会
把数据最高位当成校验位，摇杆高位出现时会产生错误控制量。

DMA 中断只复制完成块到长度为 1 的覆盖队列，`task_dbus` 每 2 ms 解码并发布一次
Yaw CH0、Pitch CH1 命令。覆盖队列只保留最新状态，避免积压旧摇杆动作；命令时间戳
使用接收时刻，100 ms 没有合法帧即掉线。掉线或快照失效走零电流，合法回中仍保持
当前位置。拨杆、鼠标和键盘目前只解码和诊断，不改变使能。

中断优先级为 5，满足 FreeRTOS `FromISR` 规则；USART3 不调用 `HAL_UART_IRQHandler`，
因为 HAL 单缓冲接收会 Abort 自定义 DBM。半帧 IDLE、UART/DMA 错误只请求任务恢复，
不会在 ISR 中打印或阻塞。驱动没有 CRC，字段范围校验只能过滤明显错位，不能替代实机
链路检查。

任务配置位于 `Core/Inc/task/task_dbus/task_dbus_config.h`：摇杆反向调
`DBUS_*_INPUT_SIGN`，回中漂移调 `DBUS_STICK_DEADBAND_RAW`，满杆速度调对应轴
`*_MAX_COMMAND_SPEED_RPM`；输入方向和 GM6020 电流方向是两个坐标系，不能混改。

验证：临时主机测试覆盖 11 位通道高位、鼠标有符号值、拨杆编码、M0/M1 所有权、IDLE
顺序、错误恢复、时间回绕、覆盖队列和双轴命令超时；`cmake --preset Debug` 与构建通过。
