# DBUS 鼠标虚拟摇杆

本模块把 DBUS 鼠标相对位移转换为可保持、可自动回中的 X/Y 虚拟速度。模块按合法帧序列去重，乘以浮点增益后累加；两轴分别按任务配置保持和线性回中。当前参数以 `task_dbus_config.h` 为准。它只改变输入速度，不更改目标角度、PID、限位、电流 Ramp 或重力补偿。

接口位于 `Core/Inc/algorithm/mouse_virtual_joystick/`，实现位于 `Core/Src/algorithm/mouse_virtual_joystick/`。模块不访问 HAL、FreeRTOS、CAN 或 UART，只由 `task_dbus` 任务调用；同一实例不能并发访问。

## 接口与调用

`MouseVirtualJoystick_Init(handle, config)` 复制并校验配置。失败时句柄失效并清零。
`MouseVirtualJoystick_Reset(handle)` 清除速度、事件与序号历史，保留已校验配置。
`MouseVirtualJoystick_Update(handle, now_ms, frame_sequence, frame_timestamp_ms, mouse_x, mouse_y, online, &x, &y)`
返回 bool，两个输出单位为 ‰；失败时非空输出写零并清除运行状态。所有实例均由一个任务独占。

`task_dbus` 使用一份 `Dbus_SnapshotTypeDef`，将 `valid_frames` 作为帧序号。相同序号只推进时钟；
新序号才消费鼠标增量，鼠标原值保持 int16_t；增益、累计速度、回中起点和回中计算使用 float。向前跳号允许，
不会猜测丢失的位移；DBUS 环形邮箱会累计当前批次的合法帧，邮箱溢出只通过
`frame_overruns` 报告，不能伪造平滑输入。

参数集中在 `task/task_dbus/task_dbus_config.h`，改动后重新编译并烧录。
保留 `#ifndef`，可用编译器 `-D宏=值` 临时覆盖。CMake 验证覆盖应通过 C 编译选项传入。

| 参数后缀 | 单位与范围 | 何时调整 |
| --- | --- | --- |
| `GAIN_PERMILLE_PER_COUNT` | ‰/count，有限非负浮点数 | 过快时减小，可用 `0.1f`、`0.05f`、`0.01f` |
| `HOLD_MS` | ms，可为 0 | 停手后拖尾过长时减小 |
| `DECAY_MS` | ms，必须大于 0 | 回中冲击或摆动时增大，会增加停手后位移 |
| `SIGN` | +1/-1 | 方向反时修改 |

两轴分别使用 `DBUS_YAW_MOUSE_VIRTUAL_*` 和 `DBUS_PITCH_MOUSE_VIRTUAL_*`。
后缀 `GAIN_PERMILLE_PER_COUNT` 调灵敏度；`HOLD_MS` 调停手后的保持；
`DECAY_MS` 调回中速度；`SIGN` 调方向。共享开关为 `DBUS_MOUSE_VIRTUAL_ENABLE=1`；
共享安全上限为 `DBUS_MOUSE_VIRTUAL_OUTPUT_LIMIT_PERMILLE=1000`。

增益允许小数，0 会禁用对应轴的鼠标增量；NaN、Inf 和负的任务增益使初始化失败。保持时间可为 0；回中时间必须大于 0；
每轴保持加回中不超过 INT32_MAX ms。方向为 +1/-1，上限为 1~1000‰。
响应太快时先减增益；拖尾过长时减保持；停止冲击大时增加回中时间。
当前增益为 `0.1f`。小动作在内部累计，不会因为单帧增量低于 1‰ 而丢失；进一步降低最大速度可减公共输出上限。

算法配置的 X/Y 增益是带方向的 float，任务用非负增益乘 ±1 符号转换。各轴累计结果限幅后，
Update 直接发布浮点千分比；任务只在图表协议需要时取整，取整结果不回写累计状态。
例如增益 `0.1f`、连续五帧各 1 count，内部累计为 0.5‰，命令保持 0.5‰，不会在任务接口处变为零。
任务再执行 `clamp(stick + virtual_mouse, -1000, 1000)`，避免两种输入合成后越界；命令保留浮点，图表按整数协议输出。

```c
/* 以下为调用示例；产品参数统一从任务配置组装。 */
MouseVirtualJoystick_HandleTypeDef mouse;
const MouseVirtualJoystick_ConfigTypeDef config = {
    .x = {.gain_permille_per_count = -0.1f, .hold_ms = 50U, .decay_ms = 50U},
    .y = {.gain_permille_per_count = 0.1f, .hold_ms = 50U, .decay_ms = 50U},
    .output_limit_permille = 1000,
};
(void)MouseVirtualJoystick_Init(&mouse, &config);
/* 任务周期：input 是本周期唯一快照，now_ms 在取得快照之后读取。 */
float virtual_x = 0.0f;
float virtual_y = 0.0f;
(void)MouseVirtualJoystick_Update(&mouse, now_ms, input.valid_frames,
    input.timestamp_ms, input.mouse_delta_x, input.mouse_delta_y, input.online,
    &virtual_x, &virtual_y);
```

## 时序与失效

`MouseVirtualJoystick_AxisConfigTypeDef` 保存单轴有符号增益、保持和回中时间；
`MouseVirtualJoystick_ConfigTypeDef.x/y` 分别配置两轴，共用输出上限。
X/Y 分别保存速度、回中起点和最后非零事件时间。X 的新事件不会续期 Y；零位移帧也不会续期。
新事件到达时先计算当前回中速度，再累计该帧增量；反向位移立即抵消或反转虚拟杆。
回中从固定起点按绝对经过时间线性计算，不用逐周期乘衰减系数，稀疏/密集调度具有相同回中曲线。

按上述示例，Yaw 在 1000 ms 收到 `mouse_x=100`：输出为 -10‰；1050 ms 开始回中，1075 ms 为 -5‰，
1100 ms 为零。这 100 ms 内云台仍可能继续移动，回中完成后按既有控制器保持最后目标和重力补偿。
接收器必须持续发送合法帧；若 100 ms 没有合法 DBUS 帧，离线安全门优先，立即清虚拟速度。

每轴阶段 IDLE=0、HOLD=1、DECAY=2、INPUT=3；INPUT 仅表示当前调用消费了该轴非零增量。
句柄 `state` 汇总两轴，优先级 INPUT > DECAY > HOLD > IDLE，不表示两个轴的阶段完全相同。
日志按 `DBUS_TASK_LOG_PERIOD_MS` 采样，默认 100 ms，可能没有采到短暂的 INPUT；图表 5/6 是实时虚拟速度，原始 X/Y 在文本日志中。
`mouse_virtual_x_state/y_state` 分别记录两轴阶段；`mouse_virtual_state` 保留汇总阶段。

0 ms 时间戳和序号 0 都有效。uint32_t 无符号差值支持时间/序号正常回绕；回退、半圈以上差值、
未来帧、同一序号却改变时间戳均清零。非有限计算结果也清零，禁止将 NaN/Inf 转换为整数。
任务处理“DMA 新帧晚于入口取时刻”的竞态时重读 HAL_GetTick，
继续保留接收时刻发布命令。快照无效、DBUS 离线、初始化失败或算法失败时发布两轴零速度，
不会把 now_ms 当接收时间续期。此失效表示停止目标积分，电机反馈在线时仍保持最后目标。

## 调参与验证

鼠标不动先查开关和符号；过慢增加增益，代价是更早累计到上限；停手后运动太久先减保持时间，
再减回中时间；停止冲击可增加回中时间，但会增加停手后位移。静差、反馈噪声和机械摆动仍需
结合已有轴日志检查，不能仅凭新输入算法推断 PID 或实际机构已稳定。

临时原生 C 测试覆盖去重、0 ms、独立两轴保持、线性终点、新输入打断回中、反向和饱和、
int16_t 极值、小数累计、NaN/Inf/浮点溢出、无效参数、掉线、未来时间及时间/序号回绕；实际 DBUS 任务替身覆盖
同快照发布、时间戳不续期、DMA 忙、快照失败、真实 100 ms 超时及 10 通道格式。
分轴配置整理后，32 组日志/虚拟开关运行、16 组 ARM 严格编译和 8 组错误配置拒绝通过。
实际 DBUS 任务在不同轴浮点增益/时序下运行通过；32 组日志/虚拟开关、非法浮点增益拒绝及 ARM 严格编译通过。关闭日志后的任务对象没有 printf 引用。
临时验证文件位于系统临时目录，不加入工程。

执行 `cmake --preset Debug`、`cmake --build --preset Debug` 和工程目录 `git diff --check -- .`。
新增源文件只接入顶层 CMake，现有 Core/Inc 覆盖 include 路径；`.ioc`、FreeRTOS 任务、USART3 IRQ
和 CubeMX 生成区无需改变。本轮未进行 CubeMX 重生成或实机验证，鼠标手感及停止摆动须上板确认。
