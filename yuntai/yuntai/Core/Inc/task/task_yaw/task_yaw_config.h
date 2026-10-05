/**
  ******************************************************************************
  * @file    task_yaw_config.h
  * @brief   Yaw 标定数据、硬件调参模式和控制参数入口。
  *
  * 先使用 GM6020 标定测试手动记录中心、最小和最大连续角度，再把记录值
  * 写入本文件并将 YAW_CALIBRATION_VALID 改为 1。Yaw 正式运行时和角度环
  * 硬件测试共同读取这些值，避免“测试一套范围、正式模式另一套范围”。
  * 本文件只保存编译期参数，不访问 CAN、FreeRTOS 或串口。
  * 通俗理解：调参时主要修改这里；代码运行时只读取这些固定值，不在这里做控制。
  ******************************************************************************
  */

#ifndef TASK_YAW_CONFIG_H
#define TASK_YAW_CONFIG_H /* 防止 Yaw 配置被同一编译单元重复包含（避免宏重复定义）。 */

#include <stdint.h>

#define YAW_HARDWARE_TEST_MODE_OFF 0U /* 关闭硬件测试，任务运行正式 Yaw 控制（使用真实命令）。 */
#define YAW_HARDWARE_TEST_MODE_CALIBRATION 1U /* 只读打印角度，禁止主动输出电流（人手移动记录标定值）。 */
#define YAW_HARDWARE_TEST_MODE_ANGLE_LOOP 2U /* 使用正式闭环保持用户指定角度（测试仍复用正式保护）。 */

#ifndef YAW_HARDWARE_TEST_MODE
#define YAW_HARDWARE_TEST_MODE YAW_HARDWARE_TEST_MODE_OFF /* 默认使用右摇杆正式控制；标定/固定角度模式按需显式切换（遥控掉线零输出）。 */
#endif

#define YAW_CALIBRATION_VALID 1U /* 1 表示下面三组角度已完成实机标定（未标定时闭环保持零输出）。 */
#define YAW_TASK_PERIOD_MS 2U /* 测试与正式共同使用的控制周期，单位 ms（PID/Ramp 的 dt 共用此值，不可小于一个 FreeRTOS Tick）。 */
#define YAW_FEEDBACK_TIMEOUT_MS 100U /* CAN 反馈离线阈值，单位 HAL ms（变大会延迟掉线停机，不能靠放宽它掩盖通信故障）。 */
#define YAW_COMMAND_TIMEOUT_MS 100U /* 正式命令有效时间，单位 HAL ms（固定角度测试没有外部命令，不使用这个超时）。 */
#define YAW_TASK_LOG_PERIOD_MS 500U /* 正式周期日志间隔，单位 HAL ms（堵转事件优先输出，不等待间隔）。 */
#define YAW_CALIBRATION_CENTER_ANGLE_RAW 2100 /* 物理中位连续角度，单位编码器计数（零点参考）。 */
#define YAW_CALIBRATION_MIN_ANGLE_RAW 115 /* 连续总角度中的较小实测边界，单位计数（用线性坐标便于比较，不能用回绕单圈值）。 */
#define YAW_CALIBRATION_MAX_ANGLE_RAW 4085 /* 连续总角度中的较大实测边界，单位计数（必须大于中位，单圈值跨 8191→0 会失去大小关系）。 */
#define YAW_MOTOR_CURRENT_SIGN 1 /* 实测正逻辑电流使连续角度增加（若上板观察到正误差反而变小，再改为 -1）。 */
#define YAW_FEEDBACK_SPEED_SIGN 1 /* Yaw 原始反馈转速与连续角度正方向一致；若实测角度/速度相反再单独改此宏。 */

#define YAW_MAX_COMMAND_SPEED_RPM 45.0f /* 正式遥控相对速度满杆上限，单位 rpm（摇杆满量程的最大转速）。 */
/*
 * 测试与正式模式读取同一份配置：切换模式不用再抄一次 PID、前馈或滤波。
 * 速度目标 = 位置 P × 连续角度误差 + 前馈系数 × 命令速度，随后统一限幅。
 * 运动跟随落后且未饱和时可小步增加前馈，超前/过冲则减小；固定角度命令速度为 0，改系数不改变到位响应。
 * （前馈是预先告诉速度环目标要怎么走，不是给静止目标凭空加力。）
 */
#define YAW_VELOCITY_FEEDFORWARD_GAIN 1.2f /* 命令速度前馈，无量纲、非负；0 关闭、1 保留原行为（只影响跟随，不改变目标角度积分）。 */

#define YAW_MAX_SPEED_TARGET_RPM 95.0f /* 30°调参起点限制到 50 rpm（提高响应但仍保留提前刹车余量）。 */
/*
 * 固定目标时：目标速度 = 限幅(位置 P × 角度误差)，8192 计数 = 360°。
 * P=0.10 时，差 1°请求约 2.28 rpm；上限 50 rpm 时，差约 22°才饱和。
 * P=10 时，同一上限下差 5 计数（约 0.22°）就满速，目标附近容易反复换向。
 * （位置 P 决定离目标多近时开始减速；增大它会缩短减速区，饱和后不会继续加快。）
 */
#define YAW_POSITION_KP_RPM_PER_RAW 0.01f /* 角度 P，单位 rpm/计数；按 0.01 小步试调（不能套用无量纲 PID 增益）。 */
#define YAW_POSITION_KI_RPM_PER_RAW_S 0.0f /* 角度 I，单位 rpm/(计数·s)；Yaw 默认关闭，先独立确认位置 P/D 和速度环。 */
#define YAW_POSITION_INTEGRAL_LIMIT_RPM 5.0f /* 角度 I 上限，单位 rpm；I=0 时保持无积分行为。 */
#define YAW_POSITION_KD_RPM_S_PER_RAW 0.0f /* 角度 D，单位 rpm·s/计数；Yaw 默认关闭，启用前先确认速度反馈噪声和制动裕量。 */
#define YAW_SPEED_KP_CURRENT_PER_RPM 200.0f /* 速度 P，单位原始电流/rpm；增大可加强驱动与制动，也会放大测速噪声（是否改善须逐步实测）。 */
#define YAW_SPEED_KI_CURRENT_PER_RPM_S 20.0f /* 速度积分，单位原始值/(rpm·s)；用于稳定后的持续静差（振荡时先单独关闭以区分积分影响）。 */

#define YAW_MAX_CURRENT_RAW 7000.0f /* 30°首次调参的电流上限（先限制最大力矩，避免失控时快速冲到机械挡块）。 */
#define YAW_SPEED_KD_CURRENT_S_PER_RPM 1.5f /* 速度 D，单位原始值·s/rpm；默认关闭（误差差分会对测速跳变和目标突变产生尖峰，不能盲目加大）。 */
#define YAW_SPEED_INTEGRAL_LIMIT_RAW YAW_MAX_CURRENT_RAW /* 积分项绝对值上限，单位原始值，范围 0~MAX_CURRENT（默认保留旧限幅；补偿过大可降低，静差时先看积分是否已到上限）。 */
#define YAW_CURRENT_SLEW_RAW_PER_S 60000.0f /* 电流变化率上限，单位原始值/秒（必须能在几十毫秒内反向刹车，不能让旧方向电流拖到限位）。 */
#define YAW_SPEED_FILTER_ALPHA 0.20f /* 速度低通本次采样权重，范围 (0, 1]（提高权重以减少测速滞后，否则刹车指令来得太晚）。 */


#define YAW_ANGLE_LOOP_TARGET_ANGLE_DEG 30.0f /* 角度环测试目标，正负相对中心，单位度（必须在标定范围内）。 */
#define YAW_ANGLE_LOOP_LOG_PERIOD_MS 200U /* 角度环测试日志最短间隔，单位 ms（避免串口刷屏）。 */

#define YAW_CALIBRATION_LOG_PERIOD_MS 200U /* 标定数据日志间隔，单位 ms（观察连续角度）。 */
#define YAW_CALIBRATION_PROMPT_PERIOD_MS 3000U /* 操作说明重提示间隔，单位 ms（提醒操作者记录边界）。 */
#endif /* TASK_YAW_CONFIG_H */





