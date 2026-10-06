# USART1 日志与遥控器图表遥测

日志配置入口是 `Core/Inc/app/log/log_config.h`。改宏后重新编译和烧录。
USART1 使用 `.ioc` 中已有的 115200 baud、8N1、TX DMA。USART3 继续接收 DBUS。
统一接口位于 `Core/Inc/app/log/log.h`，唯一格式化和 DMA 后端位于 `Core/Src/app/log/log.c`。
不新增任务、队列、RTOS 内存或 UART 外设。

## 开关与调用

| 宏 | 默认 | 作用 |
| --- | --- | --- |
| `LOG_GLOBAL_ENABLE` | 1 | 工程业务日志总门 |
| `LOG_USART1_ENABLE` | 1 | USART1 日志输出总门 |
| `LOG_DBUS_TEXT_ENABLE` | 1 | 完整遥控数据与接收状态文本 |
| `LOG_DBUS_CHART_ENABLE` | 1 | 遥控器 10 通道数值帧 |
| `LOG_YAW_ENABLE` | 1 | Yaw 初始化、正式诊断、标定和固定目标测试 |
| `LOG_PITCH_ENABLE` | 1 | Pitch 初始化、正式诊断、标定和固定目标测试 |
| `LOG_FEED_MOTOR_ENABLE` | 1 | 供弹初始化、正式诊断和 C610 自循环测试 |
| `LOG_TASK_ENABLE` | 1 | 三个电机正式日志总门，仍须开启对应电机开关 |
| `LOG_TEST_ENABLE` | 1 | 硬件测试日志总门，仍须开启对应电机开关 |
| `LOG_CHART_PREFIX` | `"ch:"` | 图表解析前缀，1~16 字节，不含逗号、换行或 printf 占位符 |

所有打印点包含 `app/log/log.h`，使用同一个 `LOG_TRY_PRINTF(category, ...)`。
宏直接展开到 `Log_TryPrintf()`，后端一次完成格式化和 DMA 提交；没有分类包装函数或 va_list 转发层。
不需要重试的提示使用 `(void)LOG_TRY_PRINTF(...)`；需要重试时仅在返回 true 后更新日志时刻。

```c
(void)LOG_TRY_PRINTF(LOG_CATEGORY_YAW, "[水平轴] 初始化失败，保持零输出\r\n");
if (LOG_TRY_PRINTF(LOG_CATEGORY_FEED_MOTOR, "[供弹] 电流=%d\r\n", current_raw)) {
  last_log_ms = now_ms;
}
```

正式分类为 `LOG_CATEGORY_YAW/PITCH/FEED_MOTOR`，对应测试分类增加 `_TEST` 后缀。
共用 GM6020 标定和固定目标测试由各轴任务填写 `test.log_category`，不通过中文名称判断轴；零值不输出。
云台诊断只共享 `gimbal_log.h` 的格式和快照参数，任务与测试直接调用统一日志宏。
DBUS 按文本/图表开关拼接内容，用 `LOG_CATEGORY_DBUS_TELEMETRY` 合并为一次 DMA 提交。
关闭 Yaw 日志不关闭 DBUS 文本中的 yaw 输入字段；该字段属于遥控器诊断。

禁用宏不求值参数，try 返回 false，表示没有 DMA 提交。不要将控制、CAN 发送或状态推进放入
日志宏参数；禁用日志不能删除硬件操作。旧 `usart_printf/usart_try_printf` 和云台日志转发函数已删除。
业务只通过统一宏输出；外设初始化文件不保存日志缓冲区或格式化实现。

## DBUS 字段与图表

完整文本的固定字段为 `online/age_ms`、`ch0~ch4`、`sw_left/sw_right`、`mouse_x/y/z`、
`mouse_left/right`、`keyboard`、`yaw/pitch/fire`、`valid/invalid/resync/uart_err/dma_err/start_err`。
虚拟鼠标另有 `virtual_mouse_x/y`、`mouse_virtual_state`、`mouse_frame_sequence`；虚拟值为 ‰，
状态为 IDLE=0、HOLD=1、DECAY=2、INPUT=3，序号等于本快照的 `valid_frames`。
其中 `valid/invalid` 是累计帧数，keyboard 为 16 位十六进制位图；Yaw/Pitch 单位为 ‰。
CH0~CH4 已减去中心 1024，鼠标为有符号 count。图表所有值均为十进制整数。

```text
ch:-660,-123,0,123,660,-1000,1000,-42,55,65535
```

| 通道 | 含义 |
| --- | --- |
| 0、1 | 右摇杆左右、上下，中心为 0 |
| 2、3 | 左摇杆左右、上下，中心为 0 |
| 4 | 遥控器滚轮；不同于鼠标 Z |
| 5、6 | 虚拟鼠标 X、Y，单位 ‰，默认范围 -1000~1000 |
| 7 | 原始鼠标 Z，有符号 count |
| 8 | `左拨杆 | (右拨杆<<2) | (鼠标左键<<4) | (鼠标右键<<5)`，0~63 |
| 9 | keyboard，0~65535 |

拨杆编码为上=1、中=3、下=2。键盘 bit0~15 依次是
W、S、A、D、Shift、Ctrl、Q、E、R、F、G、Z、X、C、V、B。
图表通道没有在线标志；离线时通道 5/6 清零，其它原始通道显示最后合法输入，
请结合文本 `online/age_ms` 判断数据是否新鲜。原始 mouse X/Y 仍在文本遥测中，
不能把虚拟速度与 count 直接比较；上位机把通道 5/6 命名为“虚拟鼠标 X/Y”。

数据参考 `C:\Users\zhoujinyuan\Desktop\hehe.html` 的 10.1.2/10.2 节：文本 CSV、`ch:` 前缀、
逗号分隔、最多 10 通道、单帧数据不超过 200 字符、换行帧尾。附件中的其他产品功能不是本工程需求。

上位机操作：打开 USART1 对应 USB 串口，设 115200/8N1；接收分帧选择换行，图表解析选择
文本 CSV，前缀设 `ch:`；按上表命名通道。文本和图表同时开启时，中文任务日志及 `[DBUS]`
行不是数值帧；若上位机提示无效帧头，可关闭解析错误提示或仅开启图表分类。
图表模式建议关闭 `LOG_TASK_ENABLE/LOG_TEST_ENABLE/LOG_DBUS_TEXT_ENABLE`，得到纯 `ch:` 数据流。

## DMA、实时性与验证

同一份 DBUS 快照先发布控制命令，再输出遥测；日志不刷新帧接收时间戳。离线时文本显示
`online=0` 和原始帧年龄，两轴停止目标移动并保持位置，供弹请求失效。命令/反馈安全门不依赖日志开关。

保留原始 768 字节唯一 DMA 缓冲区。串口忙、ISR 调用、空文本、超长文本或 HAL 失败立即返回；
发送成功后只有 `log.c` 中的 USART1 完成回调释放缓冲区。无队列、无等待，不保证每个样本必达；繁忙时
下一 2 ms 调度周期尝试最新快照，采样间隔可能大于 100 ms。不要按固定频率假定丢样不存在。

115200/8N1 的容量为 11520 字节/s。本轮实际任务的极值离线测试包为 454 字节，100 ms 周期占
4540 字节/s，约 39.4% UART 带宽，单次发送约 39.4 ms。保守按原始字段、32 位计数/年龄和
虚拟值的最大字符宽度估计，组合包小于 500 字节、图表帧小于 70 字节，均小于缓冲区和 200 字符图表限制。
正式云台/供弹及测试日志另外占用带宽。需要更稳定图表时关闭其他分类，不修改 DBUS 周期或超时。

验证使用临时 HAL 替身：所有文本字段、图表 10 通道/状态打包、保持接收时间戳、离线状态、
限频/忙时重试、HAL 失败、767 字节边界、超长拒绝、DMA 缓冲区不覆盖和 HAL ms 回绕均通过。
8 组可执行开关组合通过；18 组 ARM 严格编译组合及启用的硬件测试分支通过。
Debug 配置/构建通过；临时测试文件不纳入工程。尚未上板确认 USB 串口与多多盒子绘图效果。
虚拟鼠标本轮补测 32 组日志/鼠标开关运行、16 组 ARM 严格编译和关闭遥测后的未引用符号，均通过。
统一后端改造后，临时 HAL 验证通过全部 512 组日志开关组合；单独关闭电机时同时屏蔽正式和测试分类。
242 次 ARM 严格编译覆盖独立开关和硬件测试模式；关闭分类的正式任务对象没有日志后端引用，
总关闭的后端没有格式化、DMA 或发送缓冲区符号。实际 DBUS 任务的 32 组回归保持 10 通道格式。

## CubeMX 持久化

本轮保留 `.ioc`、FreeRTOS、USART3 IRQ、UART/DMA 初始化和 CubeMX 生成区。
`usart.c/.h` 的日志 USER CODE 已迁入独立日志模块，顶层 CMake 新增 `Core/Src/app/log/log.c`。
现有 `Core/Inc` include 路径已覆盖日志头文件。
CubeMX 重生成后按 `doc/CUBEMX_MANUAL_STEPS.md` 检查 USER CODE 和独立任务源列表。

DBUS 遥测周期只由 `task_dbus_config.h` 的 `DBUS_TASK_LOG_PERIOD_MS` 配置，默认 100 ms。
日志配置只控制启停和图表前缀，不再提供重复的遥测周期宏。
