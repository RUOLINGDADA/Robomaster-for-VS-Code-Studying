# yuntai 工程文档

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

- `TECHNICAL_ARCHITECTURE.md`：全工程架构、启动与任务调用链、CAN/DBUS/PWM 协议、时间基准、每个纯算法的公式、供弹状态机、主要配置宏的直接作用、故障安全门、验证方法和文档完成规则。实现或修改功能时，先读本文件的相关章节，再进入模块 README。
- `MACRO_REFERENCE.md`：由 `tools/update_technical_reference.ps1` 从 `Core` 生成的逐个 `#define` 原文、作用解释和引用导航；条件分支全部保留，不能把未选中的分支值当成当前生效值。
- `IMPLEMENTATION_GUIDE.md`：24 个关键解码、快照、PID、Ramp、云台、供弹、PWM 和日志函数的当前源码原样摘录与链接；算法说明仍以总文档为准。
- `tools/update_technical_reference.ps1`：修改宏或关键函数后重建上述两个附录。脚本只扫描 `Core`，生成文件不应手工编辑；生成后必须人工检查宏的单位/边界/副作用和摘录是否对应正式分支。

## 当前已实现模块

当前已实现模块：

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
- DBUS 鼠标 X/Y 累计为可保持、可自动回中的虚拟速度，再叠加到 Yaw/Pitch 右摇杆速度；鼠标左键命令由 `task_dbus` 发布，`task_feed_motor` 先预旋双 C615，再驱动 C610 上弹，停滞确认后进入单发保持；长按首发完成后连发。

协议和硬件文档优先记录来源、版本、单位、字节序、时序和验证日期。无法确认的内容标为“待确认”，不要用猜测替代。

## 新模块文档最小模板

每个新增模块至少说明：用途、文件位置、公开 API、依赖、运行上下文（任务/ISR/普通调用）、周期或超时、线程安全约束、失败处理、构建验证命令和已知限制。


## 参数集中配置

- 鼠标浮点灵敏度、每轴保持与回中：`Core/Inc/task/task_dbus/task_dbus_config.h`；过快时可将增益改为 `0.05f` 或 `0.01f`。
- Yaw/Pitch 标定、PID、Ramp 和重力补偿：对应轴的 `task_*_config.h`。
- C615 脉宽与 Ramp、C610 角度步长测试及正式供弹：`task_feed_motor_config.h`。角度步长测试宏只用于测试构建，测试完成后必须人工把结果写入正式供弹宏并关闭测试开关。
- 中文受控注释、配置归属和技能使用规则：`AGENTS.md` 与 `doc/CODING_STYLE.md`。

