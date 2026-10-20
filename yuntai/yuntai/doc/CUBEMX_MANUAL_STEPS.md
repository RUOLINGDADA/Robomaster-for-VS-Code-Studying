# CubeMX 重新生成后的人工步骤

## MotionCal 标定时临时修改 USART1

2026-10-18新增编译期磁标定模式；当前用户已将USART1生成115200/8N1，`.ioc`与 `usart.c`一致，标定开关为1。当天修复了帧内空格导致官方解析器拒收的问题，只需重新编译烧录，无需再次生成。以下步骤用于以后更改或恢复串口配置。

1. 打开实际工程 `yuntai.ioc`，进入 **Connectivity → USART1 → Parameter Settings**，只将 **Baud Rate** 改为 `115200`；保持8 Bits、No Parity、1 Stop Bit、现有引脚和TX DMA。
2. 保持Keep User Code，保存并Generate Code。不要新建任务、队列、UART或第二次DMA发送后端；USART3 DBUS、SPI/I2C和六任务保持原配置。
3. 在 `Core/Inc/app/log/log_config.h` 设置 `LOG_INS_MAG_CALIBRATION_ENABLE=1U`，总门 `LOG_GLOBAL_ENABLE/LOG_USART1_ENABLE`保持1。模式自动屏蔽其他日志，磁反馈暂禁用，原电机任务仍运行。重新配置/构建并烧录。
4. 检查 `.ioc`和 `usart.c`中的USART1均为115200，USER CODE及独立任务/回调保留；用串口助手115200/8N1确认 `Raw:0,0,0,0,0,0,mx,my,mz`，帧内无空格、结尾CRLF。关闭助手释放COM，再用MotionCal打开同一端口；前六项固定零可作磁场标定。
5. 完成采样后按 [INS说明](modules/ins/README.md#motioncal-磁场标定模式)手动回填芯片系偏置/矩阵，将模式宏恢复0，再编译烧录。恢复原日志9600时，在CubeMX中改回9600、保存并重新生成；也可保留115200并同步所有终端。

正常模式下，仅修改标定开关、帧格式或参数不需要重新生成；改变USART1波特率需要保存到 `.ioc`并生成。本次代码实现保留用户115200串口生成配置；以下原有清单中的9600为此前正常配置背景。

## INS 任务手动创建与生成

2026-10-17 用户第二次生成后，**EXTI9_5遗漏已补齐，生成配置及正式Debug构建/链接检查通过**。INS任务、堆49152、SPI /16、三个下降沿和六项IRQ优先级5均已保存；六任务入口及EXTI9_5在ELF中均为T。下一步上板检查任务创建返回值、采样和堆/栈余量；[INS说明](modules/ins/README.md)包含安装、标定和接口。

EXTI9_5现已在 `.ioc` → System Core → NVIC中勾选，优先级5/0；`gpio.c`含 `HAL_NVIC_EnableIRQ(EXTI9_5_IRQn)`，`stm32f4xx_it.c`含 `EXTI9_5_IRQHandler()`并调用 `HAL_GPIO_EXTI_IRQHandler(INT1_Gyro_Pin)`，正式ELF该IRQ为T。后续生成保留此配置，不重复添加任务。下表作为完整配置核对清单。

### 打开工程、设置引脚与中断

打开实际工程（本工作区内第二层 `yuntai/`）的 `yuntai.ioc`。保留原五任务、时钟、双CAN、USART1 9600、DBUS和PWM。

| CubeMX页面/字段 | 填写/核对值 |
|---|---|
| SPI1 Mode | Full-Duplex Master |
| SPI1 Data Size / First Bit / NSS | 8 Bits / MSB First / Software |
| SPI1 Clock Polarity / Phase | High / 2 Edge，即Mode3 |
| SPI1 Baud Rate Prescaler | **16**，当前84MHz下为5.25MHz；/8会得到超过手册10MHz的10.5MHz |
| SPI1引脚 | PB3 SCK、PB4 MISO、PA7 MOSI；SYS调试保持Serial Wire释放JTAG引脚 |
| PA4 / PB0 | GPIO Output Push Pull、初始High，加速度/陀螺片选 |
| I2C3 | Fast Mode 400000Hz，7-bit；PA8=SCL、PC9=SDA |
| PG6 | GPIO Output Push Pull、初始High，IST8310复位 |
| PC4 / PC5 / PG3 | External Interrupt、**Falling Edge**，保留上拉 |

保留标签 `CS1_Accel/CS1_Gyro/INT1_Accel/INT1_Gyro/DRDY_IST8310/RSTN_IST8310`。当前三个DRDY为Rising，必须改成Falling。修改保存在 `.ioc`，不要只改生成的 `spi.c/gpio.c`。

SPI1 → DMA Settings保留/添加：

| Request | Stream / Channel | Direction | Mode | Increment / Alignment | 硬件优先级 |
|---|---|---|---|---|---|
| SPI1_RX | DMA2 Stream0 / Channel3 | Peripheral to Memory | Normal | Memory Enable，Peripheral Disable，Byte | Very High |
| SPI1_TX | DMA2 Stream3 / Channel3 | Memory to Peripheral | Normal | Memory Enable，Peripheral Disable，Byte | High |

NVIC开启 **SPI1 global、DMA2 Stream0/3、EXTI line3、line4、line[9:5]**，六项均设置Preemption Priority=5、Sub Priority=0。保持FreeRTOS可调用ISR优先级上限数值5，不可改成0。其他串口DMA、DBUS和CAN中断保持原配置。

### 创建唯一任务并设置堆

进入 **Middleware and Software Packs → FREERTOS → Tasks and Queues → Add**（部分版本为Middleware → FREERTOS），保留原五行，只新增：

| 字段 | 值 |
|---|---|
| Task Name | `task_ins` |
| Entry Function | `task_ins_entry` |
| Priority | `osPriorityAboveNormal`，32 |
| Stack Size | `1024` words，即4096 bytes |
| Code Generation Option | `As weak` |
| Allocation / Parameter | `Dynamic` / `NULL` |

不用新增队列、信号量、定时器或单独的磁任务。入口使用任务通知阻塞调度，不是每周期 `osDelay(1)`。

在 FREERTOS → Config Parameters → Memory Management，将 `configTOTAL_HEAP_SIZE` 设为 **49152 bytes**。本轮已同步 `FreeRTOSConfig.h` USER CODE旧40960覆盖值为49152；生成后确认 `.ioc`、生成默认宏和最终覆盖值一致。六任务栈合计36864 bytes，还要计入TCB、Idle/Timer等并实测余量。

### 保存与重新生成

Project Manager → Code Generator勾选 **Keep User Code when re-generating**，保留CMake工具链，保存 `.ioc`，点击Generate Code。不要把业务代码粘贴进弱入口，不手写第二次 `osThreadNew()`。

GPIO和SPI回调已经在独立 `task_ins_runtime.c` 中提供强定义。不要在main/gpio另写第二套 `HAL_GPIO_EXTI_Callback/HAL_SPI_TxRxCpltCallback/HAL_SPI_ErrorCallback`。以下IRQ应由CubeMX生成，不能另外复制同名实现：

```c
void EXTI3_IRQHandler(void) { HAL_GPIO_EXTI_IRQHandler(DRDY_IST8310_Pin); }
void EXTI4_IRQHandler(void) { HAL_GPIO_EXTI_IRQHandler(INT1_Accel_Pin); }
void EXTI9_5_IRQHandler(void) { HAL_GPIO_EXTI_IRQHandler(INT1_Gyro_Pin); }
void SPI1_IRQHandler(void) { HAL_SPI_IRQHandler(&hspi1); }
/* DMA2 Stream0/3分别调用HAL_DMA_IRQHandler，传入hdma_spi1_rx/tx。 */
```

确认GPIO、DMA、SPI1和I2C3初始化均在FreeRTOS创建前，DMA初始化先于SPI。保留CAN启动和DBUS专用IRQ的USER CODE。

### 生成后检查

1. `freertos.c` 中有唯一 `osThreadNew(task_ins_entry,NULL,&task_ins_attributes)`，栈为1024×4，优先级AboveNormal，存在同名 `__weak` 空循环；原五任务也各创建一次。
2. 独立强入口仍在 `Core/Src/task/task_ins/task_ins.c`；顶层CMake六个新源文件、生成子列表SPI/I2C/HAL均保留。用git diff确认USER CODE与现有业务没有被覆盖。
3. 在实际工程目录运行：

   ```powershell
   cmake --preset Debug
   cmake --build --preset Debug
   arm-none-eabi-nm build/Debug/yuntai.elf | Select-String 'task_ins_entry|Ins_Process|Ahrs_Update'
   git diff --check -- .
   ```

   `task_ins_entry` 应为 **T**，不是W或缺失。生成前默认ELF可以删除未引用的INS代码；临时强保留链接不代表任务已运行。
4. 调试确认 `task_insHandle != NULL`，断点进入独立强入口；若停在受控等待，核对/16、下降沿和六IRQ。检查六个创建返回值、`xPortGetFreeHeapSize()`、`xPortGetMinimumEverFreeHeapSize()`、`uxTaskGetStackHighWaterMark(task_insHandle)`（单位words）。
5. 保持默认确认开关0，先核对轴向、连续静止2s，再按INS说明做磁标定。现有电机任务不会被INS自动停机，静止由操作员保证。

保存归属：`.ioc`由本次GUI操作保存任务、堆、SPI/IRQ/引脚；USER CODE保存49152 bytes堆覆盖；独立BSP/算法/app/task保存采集回调、标定、融合和快照，顶层CMake保存源项。本轮没有只修改生成区的分频或边沿，也没有手写IRQ占位。

`.ioc` 已保存 TIM1 PWM、PE9/PE11、计数频率和现有 FreeRTOS 配置。以下项目依赖硬件接线或 USER CODE，CubeMX 不能完整保存。每次重新生成后按顺序检查。

1. 在 Pinout 中确认 PE9 为 `TIM1_CH1`，PE11 为 `TIM1_CH2`。确认两根线分别接左、右 C615 信号输入。
2. 使用 DJI Assistant 2 或外部安全 PWM 工具校准 C615 的 PWM 行程并设置电机方向。协议允许 400~2200 us；当前正式配置停止值为 1000 us、活动值为 1400 us、运行上限为 1550 us。活动值与最大校准行程是两个参数，软件已删除 C615 校准模式。校准后同步修改 `task_feed_motor_config.h` 的脉宽宏。
3. 确认 `MX_TIM1_Init()` 在 `main()` 的 FreeRTOS 初始化之前执行。TIM1 不启用中断；PWM 由 `task_feed_motor` 在任务上下文启动和更新。
4. 确认 CAN2 为 PB5 RX/PB6 TX、CAN1 为 PD0 RX/PD1 TX，均为 1 Mbit/s、ABOM 使能；两个 `MX_CAN*_Init()` 在 USER CODE 启动前执行，对应 RX0 IRQ 调用各自 `HAL_CAN_IRQHandler()`。保留 `main.c` USER CODE 中 CAN1 bank0 的 16 位 ID 列表 `0x201~0x204` 与 CAN2 bank14 的全零掩码、双 CAN Start 和 FIFO0 pending 通知；两个分界值均为 14。双 CAN MSP 保留 CAN1 时钟依赖，接收回调只保留统一分发入口。CubeMX 只负责外设初始化，不能替代业务调用。
5. 保留 `stm32f4xx_it.c` 的 USART3 专用 DBUS IDLE、UART 错误和 DMA 双缓冲处理。不能恢复普通 `HAL_UART_IRQHandler(&huart3)`，否则会与 DBUS 双缓冲状态机争用。
6. 保留供弹任务中的 `HAL_TIM_PWM_Start()`、停止脉宽写入和 `C610_M2006_SendAll()` 调用。CubeMX 只能生成 TIM1 初始化，不能保存业务启停和安全归零逻辑。
7. 生成完成后检查 `Core/Inc/tim.h`、`Core/Src/tim.c`、TIM1 MSP 初始化和 `cmake/stm32cubemx/CMakeLists.txt` 的 `tim.c` 源文件项。
8. 检查 `.ioc` 保留原五任务及其属性；完成上方INS步骤后另有唯一 `task_ins`，总堆同步49152 bytes。
9. 执行 `git diff --check`，再执行 `cmake --preset Debug` 和 `cmake --build --preset Debug`。确认生成操作没有删除 USER CODE、`bsp/`、`task/` 或顶层 CMake 源项。

## 已存在的底盘任务（重新生成时保留）

用户已在 FreeRTOS → Tasks and Queues 创建以下唯一底盘任务，`.ioc/freertos.c` 已生成对应属性、一次 `osThreadNew` 和弱入口。重新生成时保留本表及原四个任务属性；本次增加 INS 后总堆按上节同步为 **49152 bytes**。不重复新增底盘任务或添加电机/模式子任务：

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

原五个业务任务栈合计32768 bytes；增加 INS 后六任务栈合计36864 bytes，49152 bytes动态堆还须承担TCB、Idle/Timer、队列等。检查底盘及INS任务句柄非NULL、`xPortGetFreeHeapSize()`、`xPortGetMinimumEverFreeHeapSize()` 与 `uxTaskGetStackHighWaterMark()`，高水位单位为words；链接RAM占用不代表动态堆余量已验证。测量实际周期、最坏执行时间和栈余量后再调整。

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

- `.ioc` 保存引脚、TIM1 PWM、时钟、中断和当前 FreeRTOS 六任务配置；底盘和INS均由用户通过CubeMX创建，INS所需EXTI9_5已生成并核对。
- `main.c` 的 USER CODE 保存 CAN 过滤器、启动和 FIFO0 通知；`stm32f4xx_it.c` 的
  USER CODE 保存 USART3 专用 DBUS IRQ。双缓冲及 IDLE 业务接收机制不能由 GUI 完整配置。
- 独立 BSP/task 文件保存 PWM 启动、Ramp、鼠标输入、命令超时和供弹状态机。顶层
  `CMakeLists.txt` 保存这些非生成源文件，生成子列表由 CubeMX 保存 `tim.c`。

## 重新生成验证记录

2026-10-17 第二次生成已补齐EXTI9_5：`.ioc`、`gpio.c`、IRQ声明和HAL转发一致，优先级5/0、使能且ELF为T。六任务各创建一次、全部入口为T，堆49152和其余外设保持；92个既有USER CODE区域保留（堆为此前批准调整）。普通Debug配置/构建通过（RAM59416 B、Flash102968 B），无强保留参数。生成配置问题已关闭；实际任务创建、采样、堆/栈余量和安装标定仍待上板。

2026-10-17 第一次生成（遗漏已由上条关闭）：用户创建 `task_ins` 后，`.ioc/freertos.c`共六任务且各一次创建，INS为AboveNormal/32、1024 words、As weak、Dynamic、NULL；堆生成值与USER CODE覆盖均49152。SPI1 Mode3 /16、I2C3 400kHz、三个下降沿、DMA2 Stream0/3 Channel3和五项已启用IRQ均正确，当时缺少EXTI9_5的NVIC使能、声明及处理函数，ELF为指向Default_Handler的W。普通Debug配置/构建通过（RAM59416 B、Flash102936 B），六任务入口及INS回调均为T，无强保留链接参数；92个既有USER CODE区域核对保留（堆为已批准变更）。该次记录不代表任务实际运行。

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
