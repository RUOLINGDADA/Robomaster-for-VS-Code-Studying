# CAN1 单任务底盘

底盘入口、配置、命令邮箱和运行时位于 `Core/Inc|Src/task/task_chassis/`。入口只初始化并以 `vTaskDelayUntil()` 每 2 ms 调度；运行时独占四轮电机、模式与控制历史。没有模式管理、电机工作或小陀螺子任务。四轮控制调用 [纯控制层](../chassis/README.md)，协议调用 [C620 驱动](../m3508_c620/README.md)，日志复用现有 USART1 DMA。

**用户已通过 CubeMX 创建唯一底盘任务。** `.ioc/freertos.c` 为 Normal2/26、1024 words、As weak、Dynamic、NULL，只有一次创建调用；正式 Debug ELF 已确认独立 `task_chassis_entry` 强入口覆盖生成弱入口，无额外手写 `osThreadNew`。重新生成检查见 [CubeMX 清单](../../CUBEMX_MANUAL_STEPS.md)。实际调度与任务创建成功仍需上板观察。

## 模式与输入

Ctrl 每个按下沿循环 `STOP → STICK → KEYBOARD → STOP`。长按只切一次。上电或 DBUS 失效回 STOP；重连先看到释放 Ctrl，再接受新按下。两个拨杆无功能。右摇杆/鼠标继续用于云台，Shift 继续控制摩擦轮。

| 模式 | 输入 | 输出行为 |
|---|---|---|
| STOP | 忽略运动键/摇杆 | 直接四轮零电流，清 PID 和曲线 |
| STICK | CH2 左右、CH3 前后 | 无旋转输入；零速仍用速度环制动 |
| KEYBOARD | W/S、A/D、Q/E | 忽略摇杆；三轴独立曲线，零速仍制动 |

切换当周期先清历史并提交零，下一周期运行新模式。零帧若 HAL 未接受，则保持 `zero_pending`，继续零帧直至成功；不忙等。已在硬件邮箱中的旧帧不作抢占撤销，HAL 接受也不代表机械已停止。

## API、时间和所有权

- `ChassisCommand_Submit()`：现有 DBUS 任务是唯一发布者，从同一份 `Dbus_GetSnapshot()` 复制 CH2/CH3、keyboard、可用性和真实接收 HAL ms。底盘不再次读取会消费鼠标增量的 DBUS 快照。整个命令在短 PRIMASK 临界区发布。
- `ChassisCommand_GetSnapshot()`：底盘任务取得一致值副本；超过 100 ms 清许可/运动键，不刷新原时间戳。邮箱用按下次数与通信代数保存发布事件；重复读不切模式，两次读之间掉线再上线仍能重置。Ctrl 只能看到 DBUS 任务已观测到的状态；两次采样间完成的极短按放不会被凭空重建。
- `ChassisTask_RuntimeInit()`：组装配置、注册 CAN1 ID1~4；任一失败回滚本轮已注册对象，100 ms 后重试。传入对象初始全零、生命周期贯穿运行。
- `ChassisTask_RuntimeRunCycle()`：消费模式事件，每轮只读一次反馈，计算实际 `HAL ms` 间隔，调用控制，变换方向并每周期提交一帧 `0x200`。反馈、保护与日志均引用本周期副本。

FreeRTOS Tick 只用于调度/初始化重试，所有宏在使用处经 `pdMS_TO_TICKS()`。PID/曲线使用 `elapsed_ms*0.001f`。HAL 无符号差支持回绕；首周期、间隔为 0 或超过 100 ms 均清历史发零。

CAN1：PD0 RX/PD1 TX，1 Mbit/s，bank0 16 位列表只接收标准数据帧 `0x201~0x204`；CAN2 bank14 保持全零掩码，两个 `SlaveStartFilterBank` 均为 14。唯一 FIFO0 回调只取一次帧，按总线分发。CAN1 的 ID1 与 CAN2 供弹 ID1 不共享状态或输出。

DBUS 过期四轮零电流且回 STOP。单轮反馈过期只停该轮并清 PID，其余轮继续；发送入口再次检查真实反馈新鲜度。Bus-Off 四轮失效、清历史，依赖已有 ABOM；恢复首周期再次清旧反馈，等待此后新帧再使能。不调用 HAL Stop/Init，不重置 CAN2。发送失败记录累计数，下一周期发送最新帧。

## 配置宏

唯一默认来源是 `task_chassis_config.h`，全部允许 `#ifndef` 编译覆盖。非法数值配置使初始化失败，保持零输出；不能把初始化重试当作已运动。

| 宏（省略 CHASSIS_） | 默认/单位 | 有效范围与直接作用 |
|---|---|---|
| TASK_PERIOD_MS | 2 ms | 转换后至少 1 Tick，改变调度周期 |
| INIT_RETRY_MS | 100 ms | 转换后至少 1 Tick，改变初始化失败重试间隔 |
| COMMAND_TIMEOUT_MS | 100 HAL ms | 1~INT32_MAX，超出原帧年龄则 STOP |
| FEEDBACK_TIMEOUT_MS | 100 HAL ms | 1~INT32_MAX，超出只停止对应轮 |
| CONTROL_GAP_MS | 100 HAL ms | 1~INT32_MAX，超过或 dt=0 清历史发零 |
| LOG_PERIOD_MS | 1000 HAL ms | 1~INT32_MAX，成功提交日志后计间隔 |
| STICK_DEADBAND_RAW | 30 raw | [0,660)，严格死区内目标为零，外部不扣宽度 |
| MAX_SPEED_RPM | 6000 rotor rpm | 有限 >0，限制三轴与各轮目标，待整定 |
| SPEED_KP/KI/KD | 4/2/0 | 有限 ≥0；单位 raw/rpm、raw/(rpm*s)、raw*s/rpm |
| INTEGRAL_LIMIT_RAW | 1000 raw | [0,CURRENT_LIMIT_RAW]，限制积分项 |
| INTEGRAL_ERROR_SCALE | 0.01 1/rpm | 有限 ≥0，积分权重 1/(1+a*abs(e))；0 关闭权重 |
| CURRENT_LIMIT_RAW | 4915 raw | 有限 (0,16384]，实际限流约 6 A，待实测 |
| KEYBOARD_T1_MS | 200 ms | >0 且 <T3，直线段结束时刻 |
| KEYBOARD_T3_MS | 1000 ms | >T1，加速/反向零速段结束时刻 |
| KEYBOARD_LINEAR_RATIO | 1/3 | 有限 (0,1)，T1 的幅度比例与接入斜率 |
| KEYBOARD_LOG_SHAPE | 4 | 有限 >0，无量纲，改变对数段形状，须复核单调性 |
| ID1/2/3/4_SIGN | -1/+1/+1/-1 | 只能 ±1，同时变换反馈 rpm 与电流，轮序须实测 |

`LOG_CHASSIS_ENABLE=0` 默认关闭；与 `LOG_TASK_ENABLE` 及总门同时开才输出，0/1。日志包含模式、在线/许可位图、反馈年龄、目标、计算电流、HAL 提交、BOFF、零帧待交和失败计数；不能把计算电流当作电调实际执行。保留用户 USART1 9600 baud，底盘默认 1 s 遥测，繁忙直接拒绝。

## 验证和上板

临时 MSVC C11 `/W4 /WX` 测试实际 PID、控制、邮箱、运行时、CAN 分发与 DBUS 适配，HAL/DBUS 硬件由替身提供；114420 项通过。包含旧 PID 默认路径逐次对照、积分选项/限幅/释放、轮序/方向/槽位、曲线、Ctrl/长按/重连、拨杆无作用、命令期限、单轮失效、BOFF、邮箱失败和时间回绕。测试未加入固件，临时文件验证后删除。Debug 与强制保留底盘入口且开启日志的链接验证通过；强制保留符号仅检验可链接，不会创建/运行任务。

用户生成后普通 Debug 构建通过（RAM 49800 B、Flash 75180 B），不需要强保留链接参数，ELF 中底盘入口为 T。仍须上板检查 `osThreadNew` 返回非空、实际 2 ms 调度、剩余堆与栈高水位；先架空核对 ID1右前/ID2左前/ID3左后/ID4右后及方向，再低速验证两种运动模式、轮失效和云台/供弹并行。尚未烧录；实际负载稳定性与多轮故障时运动姿态待实测。
