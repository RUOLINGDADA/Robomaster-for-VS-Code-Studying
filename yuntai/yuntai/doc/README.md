# yuntai 工程文档

## 当前 Yaw 边界让步

正式 Yaw 到达/越过 115～4085 count 边界时，丢弃不可达的向外目标误差，清向外积分并在 Ramp 后禁止向外电流。IMU 继续真实累计；底盘反转后保持让步形成的新航向，不再抵消旧转角。Pitch 与固定机械角测试保留原行为。源码、主机验证及新增日志见 [Yaw 边界让步](modules/gimbal/yaw_boundary_yield.md)。

## 输入采集入口

USART1 日志使用 **921600、8N1**。`LOG_DBUS_TRACE_ENABLE=1` 时独占日志分类，逐帧记录原始摇杆/鼠标，并记录 2 ms 周期发布的命令变化；设备控制继续运行。详细字段、时间含义、丢样判断和采集动作见 [DBUS 输入采集](modules/dbus/README.md#摇杆与鼠标输入采集)。两份回传未见区间内丢样；按用户选择改为鼠标跟手测速，取消速度累计，零帧清零、反向立即更新，见[分析与代码回放](modules/mouse_virtual_joystick/input_trace_analysis_20261019.md)。关闭此开关后恢复其他分类日志。仅修改源码，由用户在 VS Code 构建并烧录。

## 当前源码与交付方式

直接维护当前工程源码，由用户在 VS Code 构建并烧录最新代码。agent 默认只进行静态检查和必要的临时主机验证；不自行生成固件或另建诊断构建。旧诊断构建目录已清理，历史报告中的构建结果仅作为验证记录。

两轴方向已确认正常，惯性反馈映射为 Yaw=-1、Pitch=+1。Pitch 人工下界为8250 count，物理下挡块记录为8184～8187 count；Pitch 软件限位仍只限制人工命令，Yaw 使用上方边界让步策略。鼠标和控制参数以上板源码为准，本次保留全部现有调参值。实际触底抖动改善、动态余量和手感仍待上板验证，历史记录见[诊断与验证记录](modules/gimbal/pitch_stop_mouse_speed_20261019.md)。

## 2026-10-18 双轴自稳基线（Yaw 边界策略已更新）

按最新要求暂停磁力计采集/融合/诊断，保留磁校正参数；正式 Yaw/Pitch 锁定进入时当前惯性姿态，以原2 ms周期持续纠偏。软件机械限位只过滤人工目标增量及对应前馈，范围外同样允许捕获和保持，零输入不改目标。IMU失效转编码器保持，连续有效更新100 ms后重新锁姿态；CAN/标定/参数失败仍零电流。当前实现、单位、恢复规则、验证与 Pitch→Yaw→双轴上板步骤见[共用云台模块](modules/gimbal/README.md)，INS硬件状态见[INS模块](modules/ins/README.md)。下方磁测试记录保留为历史，不代表当前开关。

临时主机控制/真实GM6020验证2520项、真实INS/AHRS验证42065项及10个ARM条件分支通过；尚未烧录验证自稳精度。所有本次业务改动位于独立非生成模块，无需重新CubeMX生成。

本目录是 `yuntai/yuntai` 工程的长期对接入口。任何 agent 或新成员开始修改前，先阅读：

1. 工程根目录的 `AGENTS.md`：目录、代码、FreeRTOS、构建和交付规则。
2. `PROJECT_CONTEXT.md`：当前硬件、软件入口、边界和已知状态。
3. `OPEN_QUESTIONS.md`：尚未确认的需求；实现前应优先读取并在完成后关闭相关问题。
4. `CODING_STYLE.md`：命名、注释层次、底层解释和常见坑点的写法。
5. 对应模块目录中的 `README.md`：模块 API、任务模型、通信协议和验证方法。
6. `CHANGELOG.md`：近期变更及其验证结果。

修复 Bug 或修改超时、保护、CubeMX 任务配置前，另阅读 `BUG_FIXES.md` 中的相关案例，按 `AGENTS.md` 的防复发规则检查。

## 文档目录约定

```text
doc/
├── README.md              # 文档入口和阅读顺序
├── PROJECT_CONTEXT.md     # 工程现状、边界、工具链
├── OPEN_QUESTIONS.md      # 需要用户确认的需求
├── CODING_STYLE.md        # 代码、注释和底层解释规范
├── CHANGELOG.md           # 面向 agent 的变更记录
├── BUG_FIXES.md           # Bug 现象、根因、修复和防复发检查
├── TECHNICAL_ARCHITECTURE.md # 全工程架构、公式、状态机和板级附录
├── MACRO_REFERENCE.md      # Core 宏逐项自动索引
├── IMPLEMENTATION_GUIDE.md # 关键函数源码摘录
├── tools/                  # 技术附录生成脚本
├── protocol/              # CAN、串口、传感器等协议说明
├── hardware/              # 接线、引脚、板卡和器件资料
├── modules/               # 各驱动、算法和任务的对接说明
├── decisions/             # 重要架构决策记录
└── REFERENCES.md          # 本轮 PDF 来源、转换限制和参考代码
```

## 总体技术文档

- `REFERENCE_STANDARD_ROBOT_COMPARISON.md`：用户指定的本地“官方”与“整车”两个standard_robot目录的逐文件对比、磁力计实际执行路径、安装矩阵和编译/摩擦轮差异。

- `TECHNICAL_ARCHITECTURE.md`：全工程架构、启动与任务调用链、CAN/DBUS/PWM 协议、时间基准、每个纯算法的公式、供弹状态机、主要配置宏的直接作用、故障安全门、验证方法和文档完成规则。实现或修改功能时，先读本文件的相关章节，再进入模块 README。
- `MACRO_REFERENCE.md`：由 `tools/update_technical_reference.ps1` 从 `Core` 生成的逐个 `#define` 原文、作用解释和引用导航；条件分支全部保留，不能把未选中的分支值当成当前生效值。
- `IMPLEMENTATION_GUIDE.md`：关键解码、快照、PID、Ramp、云台、供弹、PWM 和日志函数的当前源码原样摘录与链接；算法说明仍以总文档为准。
- `tools/update_technical_reference.ps1`：修改宏或关键函数后重建上述两个附录。脚本只扫描 `Core`，生成文件不应手工编辑；生成后必须人工检查宏的单位/边界/副作用和摘录是否对应正式分支。

## 当前已实现模块

- INS最新97秒静止通断电对照：用户确认30秒通电/92秒断电且机构无运动，Yaw均值0.855→2.904°，断电后97秒回到0.849°；芯片磁向量变化1.382μT而模长均值仅变0.36%，正式AHRS磁输入对照可产生约1.60°偏移。支持通电相关磁观测干扰，不唯一定位器件；保留参数，下一步通电状态按原左右标记运动并比较重复性。[最新证据与限制](modules/ins/mag_calibration_20261018.md)；本次无新固件，下方待通电对照为历史。

- INS最新153秒左右物理标记测试已确认两次按同一标记回位，末10秒相对起点Yaw差约+0.71/+0.98°、动作后保持极差0.057～0.093°；151个READY点全有效/磁融合1，无丢帧/重置增长。右转早期仍约5.3°回调，纯Pitch耦合未关闭，不作全局精度验收。保持现有固件，下一步同起点断电机→通电保持对照；[最新数据及边界](modules/ins/mag_calibration_20261018.md)已记录，下方待做物理标记为此前阶段。

- INS修正后的152秒回传已确认新校正生效，150个READY点均有效/磁融合1，无重置或丢帧增长；用户确认分轴动作但角度/回位目测。纯Pitch仍有约2.47°Yaw耦合，右转后早期仍约6°回调、后期稳定，不能当作完整九轴验收。保留参数，下一步用左右/起点物理标记做重复回位；无需新固件。[窗口、受控验证与当前步骤](modules/ins/mag_calibration_20261018.md)已更新，下方待首次上板为历史状态。

- INS按用户要求完成受限日志磁标定候选修正：仅更新偏置/软铁，BMI/IST映射及全部异常门保持。三组11窗口联合估计；未拟合的63秒倾斜磁航向变化约9.11°→0.20°，独立108秒RMS约8.13°→3.11°，回位仍约2.3°。正式AHRS四项受控检查与Debug构建通过，未烧录/未完成全姿态九轴验收；烧录后先断电机做分轴保持及回位。[参数、方法、局限与回退](modules/ins/mag_calibration_20261018.md)是当前入口，下方“不改参数”等为此前阶段。

- INS最新63秒整车倾斜：两关节固定/电机断电，用户进一步目视确认实际左右朝向未变；支持记录约+9.16°Yaw变化具有明显俯仰耦合，不能只归于关节相对运动。正式AHRS同Yaw受控检查实测磁约+9.11°、NULL/理想场近0°；目视信息不作精密角度真值，平移/外部磁场仍待区分。源码/参数保持，不再仅因手提要求重复同一轮；下一步改变外部磁环境对照，并处理最终装配充分三维标定/安装问题。数据和边界见[INS当前记录](modules/ins/README.md)。

- INS最新102秒复测确认独立IST候选[+X,-Y,-Z]生效，BMI[-X,+Y,-Z]保留，100组READY磁融合均1。用户确认纯抬头时Yaw仍变化约11.6°，独立磁航向变化约12.4°、校正模长增加27.2%；九轴尚未验收。本轮不改固件/磁参数，转为整体锁定倾斜与单Pitch的磁源对照；目测回位不评回零误差。数据、限制及动作见[INS当前记录](modules/ins/README.md)，下方107秒等待烧录为历史状态。

- INS当前107秒整圈诊断：用户确认没有精确90°角标；原码平面/独立重力及历史坐标变更共同支持IST候选[+X,-Y,-Z]，BMI保持[-X,+Y,-Z]，已单独修改磁矩阵用于九轴复测。原标定场强27.7～46.2μT仍未通过；换轴不等于完成标定。[INS当前记录](modules/ins/README.md)为数据、限制和下一步唯一详细入口；下方同映射和待做整圈为历史阶段。


- INS最新91秒整机对照仍出现静止Yaw20.6→9.8°回拉，用户确认固定关节并整机转动；全READY磁融合1，观测一致性未通过。正式AHRS真实磁输入能复现，NULL/理想场对照正常；不再只归于相对底座运动，也不单改硬铁偏置。现有源码不变，下一步整机物理角标0/90/180/270/360°各20s检查原码，平面检查不代替完整标定；见[最新INS分析](modules/ins/README.md)。

- INS最新92秒定点测试已确认不动时Yaw44.8→18.8°的回拉，全READY磁融合1；正式AHRS真实磁输入可复现，NULL磁输入和理想45°输入没有主要回拉。计算链通过不代表磁观测准确；当前源码不变，下一步固定云台相对底盘后整机旋转对照，区分相对磁源与标定/安装/场地问题。详情见[最新INS实测与对照](modules/ins/README.md)。下方177秒动作不确定仅适用于旧附件。

- INS最新177秒九轴回传：采集/计算链通过，3s起READY有效、无重置/丢帧增长；41～68s磁融合退出，校正场强27.09～48.44μT，九轴磁一致性未通过。用户更正右转后是否静止为“记不清”，不据该段判定静止Yaw回拉。现有固件不变，下一轮固定Pitch做有物理标记的起点/右转/回位各30s并记时间；详情见[最新分析与复测](modules/ins/README.md)。下方待烧录/回传为历史阶段。

- INS当前按用户要求重开九轴测试：BMI/IST均diag(-1,+1,-1)，磁标定反馈门1，原MotionCal偏置/软铁及全部异常门不变，普通/磁诊断1、MotionCal0。等待烧录后按INS模块顶部执行静止、单Pitch、左右Yaw/回位，先电机断电；开启测试不等于磁标定或九轴验收。以下磁反馈0/IST旧矩阵为历史记录。

- INS当前BMI映射已改为[-X,+Y,-Z]：用户确认新日志11～17s抬头、38～40s低头，原码与旧映射确实反号。正式AHRS同样样本旧映射断言失败、新映射通过；实板Roll/Yaw和约6°水平基准差仍待复测。IST继续[+X,-Y,-Z]、磁反馈0；本次直到46s才READY，复测必须等状态1再动作。详见INS模块顶部，下方历史同映射记录不代表当前配置。

- INS最新108秒原码日志：原码→单位→安装变换的计算通过，但用户确认起始枪口实际低头约31°，输出却为Pitch+31.4°，物理安装方向验收重新打开。当前磁校正模长约27～52μT，单改正交矩阵不能消除；保持现有固件、磁反馈0，先按INS模块顶部做带物理标记的水平/抬头/低头静止测试。下方此前“BMI方向通过”等记录不能覆盖此新证据。

- INS当前方向复核：用户再次确认两芯片同用[+X,-Y,-Z]，已恢复磁映射、撤下依赖另一映射拟合的硬铁候选，恢复最新MotionCal测量值并暂停磁反馈。正式AHRS三轴方向/倾角/同映射九轴合成测试通过，未发现BMI符号问题；原码日志用于实板交叉核对，当前状态以INS模块顶部为准。

- INS当前已按用户要求修正磁标定：独立六轴日志拟合硬铁偏置[31.534,14.879,41.146]μT，单位软铁基线，磁轴[-X,+Y,-Z]，恢复九轴复测。留出Yaw保持段优于两组原软铁模型；完整软铁与实机九轴仍待验收，配置/方法及动作见INS模块顶部，以下暂停磁反馈记录为历史阶段。

- INS已分析126秒独立六轴/磁日志：24种右手轴排列中新旧标定均优先支持磁轴[-X,+Y,-Z]，已更新磁旋转配置，BMI轴保持原值。现有参数仍留下约24°磁水平朝向误差，磁反馈继续关闭；结果及窗口/单位见INS模块顶部，不代表九轴通过。

- INS当前为独立磁轴辨识：退出MotionCal、暂停磁反馈，保留1Hz芯片磁场与六轴四元数诊断。烧录后按INS模块顶部表格完成左右Yaw及Pitch/回位采样，联合比较24种右手轴排列；待硬件数据，尚未确定新矩阵。

- INS最新受限采样：新MotionCal候选仍未达质量参考；对已装回外壳日志回算，磁模长改善但当前矩阵下倾斜补偿磁航向差约79°，未替换固件参数。下一步结合固定俯仰的独立Yaw日志核对磁轴，详见INS模块最新候选记录。

- INS当前排查抬头保持后的Yaw偏移：磁融合1时出现约3°/s航向拉动，电机断电/动作分轴已确认。矩阵和融合参数保留，启用临时1Hz磁向量/角速度/四元数诊断；待烧录回传，见 [当前航向排查](modules/ins/README.md#2026-10-18-当前排查抬头保持后yaw被拉动)。未确定实机根因，不作为已修复Bug。

- 当前INS已按用户确认的IST方向开启九轴测试：安装/磁标定门均1、正常状态日志1、MotionCal模式0，沿用现有矩阵与标定参数；异常磁场仍回退六轴。需重新构建烧录，不需CubeMX生成，上板流程见 [当前九轴测试](modules/ins/README.md#2026-10-18-当前配置开启九轴上板测试)。下方门0的记录为此前阶段。

- INS最新轴向确认：用户已核对抬头/右转/左侧抬高的Pitch/Yaw/Roll均增加，BMI当前矩阵符号通过；IST方向、最终安装磁标定及九轴仍待单独验证。具体六轴→磁向量→九轴测试步骤见 [下一步测试顺序](modules/ins/README.md#下一步测试顺序)。

- INS固定安装后两份日志已确认倾角保留及运动响应；此前板未固定导致的“不跟随”现象关闭。当前源码已移除临时DEBUG及轮询诊断，保留正常1Hz状态、DMA采集和原参数；安装/磁标定确认门仍0。详情及后续方向/九轴验证见 [最新验证记录](modules/ins/README.md#2026-10-18-固定安装后的验证结果与当前代码)。下方为此前排查记录。

- INS最新原因已确认：此前板放在地上、未固定到云台，枪口运动未传到传感器；38组整板倾斜已看到采集和姿态响应。用户确认PCB与枪管平行，正使用现有固件固定到Pitch后重新测试；完整轴向和九轴融合仍待验证，详见INS模块最新说明。

- INS第二级日志已收到：直接SPI与DMA均近水平，31组ID/配置正常且芯片计数推进。下一步用现有固件直接倾斜整块PCB60°～90°检查重力换轴，用户已确认可执行、结果待返回；详见INS模块当前故障排查。

- INS最新日志已回传：31组原始比力仍近水平，用户确认已抬头且串口/J-Link接同一板。现新增1Hz `[DEBUG-INS-SPI]` 独立单寄存器轮询，对照DMA原始值、ID、配置、传感器计数；根因仍待上板确定，最新操作见INS模块排查章节。

- 2026-10-18当前INS排查：用户确认板随枪口转动，手动运动却几乎不改变姿态，原因待定位。当前源码已退出MotionCal模式（0）、开启状态日志（1）；正常1Hz输出附临时 `[DEBUG-INS-1018]` 采集/比力倾角对照。主机真实驱动/INS/AHRS运动测试通过，尚未复现硬件问题；烧录与动作采集方法见 [INS排查说明](modules/ins/README.md#2026-10-18-当前故障排查转动后角度几乎不变)。

- 2026-10-18最新磁标定状态：用户已确认MotionCal COM8点云，候选硬铁/软铁参数已回填；截图质量尚未达参考条件，继续采样，九轴及安装确认保持关闭。数值、来源与补采说明见 [本次采样记录](modules/ins/README.md#2026-10-18-本次采样记录候选参数待补采)。

- `bsp/bmi088`、`bsp/ist8310`、`algorithm/ahrs`、`app/ins`、`task/task_ins`：云台惯性/磁场采集、静止零偏、融合和快照。用户已生成第六任务、SPI /16、堆49152并补齐EXTI9_5，正式Debug构建、六任务强入口及采集IRQ链接检查通过。BMI与IST分别配置为diag(1,-1,-1)；上板方向复核及磁标定待完成。2026-10-18移除Python拟合工具，改为MotionCal串口磁标定，芯片raw×3、参数手动回填。当前用户开启模式并生成USART1 115200/8N1；已修复帧内空格拒收，官方解析/磁缓冲测试通过，六零占位保留，实际MotionCal显示待烧录确认。见 [INS模块](modules/ins/README.md#motioncal-磁场标定模式) 和 [手动步骤](CUBEMX_MANUAL_STEPS.md#motioncal-标定时临时修改-usart1)。

云台 Yaw/Pitch 与供弹 M2006 使用 CAN2（PB5 RX、PB6 TX，1 Mbit/s）；四轮 C620/M3508 底盘使用 CAN1（PD0 RX、PD1 TX，1 Mbit/s）。双 CAN 已配置启动；用户已通过 CubeMX 创建唯一 `task_chassis`，正式 Debug ELF 已确认链接独立强入口，实际调度及硬件效果待上板。过滤器分区和任务清单见 `TECHNICAL_ARCHITECTURE.md` §13.1 与 `CUBEMX_MANUAL_STEPS.md`。

当前已实现模块：

- `bsp/m3508_c620`、`app/chassis`、`task/task_chassis`：CAN1 四轮驱动、纯混控/曲线及单任务运行时。Ctrl 循环停机→左摇杆平移→键盘，拨杆无作用；复用 DBUS/PID/日志。见 `doc/modules/m3508_c620/README.md`、`doc/modules/chassis/README.md`、`doc/modules/task_chassis/README.md`。
- `Core/Inc/app/log/` 与 `Core/Src/app/log/`：独立 Yaw/Pitch/供弹开关、统一日志宏和唯一 USART1 DMA 后端，支持 DBUS 全量/10 通道图表输出；配置和上位机操作见 `doc/modules/log/README.md`。
- `Core/Inc/bsp/c610_m2006/` 与 `Core/Src/bsp/c610_m2006/`：C610 电调 + M2006 电机 CAN 驱动。接口和协议边界见 `doc/modules/c610_m2006/README.md`。
- `Core/Inc/bsp/snail_2305/` 与 `Core/Src/bsp/snail_2305/`：C615 + Snail 2305 TIM1 PWM 驱动，接口和安全脉宽见 `doc/modules/snail_2305/README.md`。
- `Core/Inc/task/task_feed_motor/` 与 `Core/Src/task/task_feed_motor/`：唯一 M2006 供弹电机的目录化任务，分为入口、命令、控制和运行时；C610 驱动保留反馈新鲜度清零门，并可通过任务配置宏开启只读手动角度步长测量。运行参数和在线判定见 `doc/modules/task_feed_motor/README.md`。
- `Core/Inc/bsp/gm6020/` 与 `Core/Src/bsp/gm6020/`：GM6020 协议、连续角度、快照和只读软件边界状态；云台命令过滤与闭环保持由 `app/gimbal` 完成，见 `doc/modules/gm6020/README.md`。
- `Core/Inc/algorithm/` 与 `Core/Src/algorithm/`：与硬件无关的 PID、Ramp、一阶低通和连续角度重力补偿，见 `doc/modules/algorithm/README.md`。
- `Core/Inc/algorithm/mouse_virtual_joystick/` 与 `Core/Src/algorithm/mouse_virtual_joystick/`：DBUS 鼠标帧去重、虚拟速度累计、保持和自动回中，见 `doc/modules/mouse_virtual_joystick/README.md`。
- `Core/Inc/task/task_yaw/` 与 `Core/Src/task/task_yaw/`：Yaw 配置、命令适配和 FreeRTOS 入口，控制和边界过滤由 app/gimbal 共用，见 `doc/modules/task_yaw/README.md`。
- `Core/Inc/bsp/dbus/`、`Core/Src/bsp/dbus/` 与 `task/task_dbus/`：DJI DBUS USART3 DMA 双缓冲接收、快照和右摇杆双轴命令适配，见 `doc/modules/dbus/README.md`。
- `doc/CUBEMX_MANUAL_STEPS.md`：重新生成后的 USER CODE、TIM1 PWM、DBUS IRQ 和硬件校准检查清单。
- `doc/REFERENCES.md`：本轮相关 PDF 的 Markdown 转换来源、版本、限制和参考代码入口。
- `Core/Src/task/task_pitch/task_pitch.c`：Pitch 独立句柄和硬件调参入口，见 `doc/modules/task_pitch/README.md`。
- `Core/Inc/app/gimbal/` 与 `Core/Src/app/gimbal/`：Yaw/Pitch 共用的 GM6020 轴运行时；负责快照驱动的正式角度环、Ramp、边界命令过滤和基础安全门，不绑定任务命令，见 `doc/modules/gimbal/README.md`。
- `Core/Inc/bsp/gm6020/` 与 `Core/Src/bsp/gm6020/`：GM6020 驱动以及手动角度标定、固定目标角度闭环硬件调参代码；`Core/Inc/bsp/c610_m2006/` 与 `Core/Src/bsp/c610_m2006/`：C610/M2006 驱动和只读手动角度步长测量代码。C610 自循环、C615 行程/转向校准测试已删除；纯协议/算法验证不建立永久测试目录。
- DBUS 鼠标 X/Y 累计为可保持、可自动回中的虚拟速度，再叠加到 Yaw/Pitch 右摇杆速度；左键短按完成当前单发步长，长按 400 ms 后按“单发步长 → 零电流停顿 200 ms → 下一步”重复；Shift 按住启动双 C615，松开当周期写停止脉宽。参数入口见 `task_feed_motor_config.h`，具体行为及限制见供弹模块 README。

协议和硬件文档优先记录来源、版本、单位、字节序、时序和验证日期。无法确认的内容标为“待确认”，不要用猜测替代。

## 新模块文档最小模板

每个新增模块至少说明：用途、文件位置、公开 API、依赖、运行上下文（任务/ISR/普通调用）、周期或超时、线程安全约束、失败处理、构建验证命令和已知限制。


## 参数集中配置

- 鼠标浮点灵敏度、每轴保持与回中：`Core/Inc/task/task_dbus/task_dbus_config.h`；过快时可将增益改为 `0.05f` 或 `0.01f`。
- 四轮方向、速度 PID、键盘曲线与通信门：`Core/Inc/task/task_chassis/task_chassis_config.h`；源工程参数待本机实测。
- Yaw/Pitch 标定、PID、Ramp 和重力补偿：对应轴的 `task_*_config.h`。
- C615 脉宽与 Ramp、C610 角度步长测试及正式供弹：`task_feed_motor_config.h`。角度步长测试宏只用于测试构建，测试完成后必须人工把结果写入正式供弹宏并关闭测试开关。
- 中文受控注释、配置归属和技能使用规则：`AGENTS.md` 与 `doc/CODING_STYLE.md`。
