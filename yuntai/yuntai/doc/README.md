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
├── protocol/              # CAN、串口、传感器等协议说明
├── hardware/              # 接线、引脚、板卡和器件资料
├── modules/               # 各驱动、算法和任务的对接说明
└── decisions/             # 重要架构决策记录
```

当前已实现模块：

- `Core/Inc/bsp/c610_m2006/` 与 `Core/Src/bsp/c610_m2006/`：C610 电调 + M2006 电机 CAN 驱动。接口和协议边界见 `doc/modules/c610_m2006/README.md`。
- `Core/Inc/task/task_feed_motor/` 与 `Core/Src/task/task_feed_motor/`：唯一 M2006 供弹电机的目录化任务，分为入口、命令、控制、保护和运行时。运行参数和在线判定见 `doc/modules/task_feed_motor/README.md`。
- `Core/Inc/bsp/gm6020/` 与 `Core/Src/bsp/gm6020/`：GM6020 协议、连续角度、快照和软件硬限位；动态堵转属于通用云台保护层，见 `doc/modules/gm6020/README.md`。
- `Core/Inc/algorithm/` 与 `Core/Src/algorithm/`：与硬件无关的 PID、Ramp 和一阶低通，见 `doc/modules/algorithm/README.md`。
- `Core/Inc/task/task_yaw/` 与 `Core/Src/task/task_yaw/`：Yaw 配置、命令适配和 FreeRTOS 入口，控制/保护由 app/gimbal 共用，见 `doc/modules/task_yaw/README.md`。
- `Core/Src/task/task_pitch/task_pitch.c`：Pitch 独立句柄和硬件调参入口，见 `doc/modules/task_pitch/README.md`。
- `Core/Inc/app/gimbal/` 与 `Core/Src/app/gimbal/`：Yaw/Pitch 共用的 GM6020 轴运行时；负责快照驱动的正式角度环、Ramp 和保护组合，不绑定任务命令，见 `doc/modules/gimbal/README.md`。
- `Core/Inc/bsp/gm6020/` 与 `Core/Src/bsp/gm6020/`：GM6020 驱动以及手动角度标定、固定目标角度闭环硬件调参代码；`Core/Inc/bsp/c610_m2006/` 与 `Core/Src/bsp/c610_m2006/`：C610/M2006 驱动和供弹自循环硬件调参代码。纯协议/算法验证不建立永久测试目录。

协议和硬件文档优先记录来源、版本、单位、字节序、时序和验证日期。无法确认的内容标为“待确认”，不要用猜测替代。

## 新模块文档最小模板

每个新增模块至少说明：用途、文件位置、公开 API、依赖、运行上下文（任务/ISR/普通调用）、周期或超时、线程安全约束、失败处理、构建验证命令和已知限制。

