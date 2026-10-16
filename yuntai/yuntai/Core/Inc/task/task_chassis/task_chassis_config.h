/**
 * @file task_chassis_config.h
 * @brief 底盘上板参数的唯一默认来源。
 *
 * 轮序/方向不对先核对 ID 与 SIGN；起步太快调速度或键盘时间，力度太大调限流。
 * PID 与方向沿用 MyChassis1，待本机架空实测。参数使用者在任务中组装配置，不反向包含本头。
 */
#ifndef TASK_CHASSIS_CONFIG_H
#define TASK_CHASSIS_CONFIG_H /* 防止重复包含。 */

#ifndef CHASSIS_TASK_PERIOD_MS
#define CHASSIS_TASK_PERIOD_MS 2U /* 调度周期，ms；必须换算为至少一个 Tick。 */
#endif
#ifndef CHASSIS_INIT_RETRY_MS
#define CHASSIS_INIT_RETRY_MS 100U /* 初始化失败重试间隔，ms；避免忙等。 */
#endif
#ifndef CHASSIS_COMMAND_TIMEOUT_MS
#define CHASSIS_COMMAND_TIMEOUT_MS 100U /* 命令期限，HAL ms，原接收时间不被转发续期。 */
#endif
#ifndef CHASSIS_FEEDBACK_TIMEOUT_MS
#define CHASSIS_FEEDBACK_TIMEOUT_MS 100U /* 单轮反馈期限，HAL ms；超时只停止该轮。 */
#endif
#ifndef CHASSIS_CONTROL_GAP_MS
#define CHASSIS_CONTROL_GAP_MS 100U /* 允许的最大控制间隔，HAL ms，超过后清历史并零输出。 */
#endif
#ifndef CHASSIS_LOG_PERIOD_MS
#define CHASSIS_LOG_PERIOD_MS 1000U /* 底盘遥测周期，HAL ms；9600 baud 下建议不小于此值。 */
#endif
#ifndef CHASSIS_STICK_DEADBAND_RAW
#define CHASSIS_STICK_DEADBAND_RAW 30 /* 左摇杆中心死区，raw，[0,660)，越大越不敏感。 */
#endif
#ifndef CHASSIS_MAX_SPEED_RPM
#define CHASSIS_MAX_SPEED_RPM 6000.0f /* 转子目标限速，rpm，>0；不是轮轴 rpm，待整定。 */
#endif
#ifndef CHASSIS_SPEED_KP
#define CHASSIS_SPEED_KP 4.0f /* 比例增益，raw/rpm，>=0；误差约 1229 rpm 时单 P 饱和。 */
#endif
#ifndef CHASSIS_SPEED_KI
#define CHASSIS_SPEED_KI 2.0f /* 积分增益，raw/(rpm*s)，>=0；本轮不重新整定。 */
#endif
#ifndef CHASSIS_SPEED_KD
#define CHASSIS_SPEED_KD 0.0f /* 微分增益，raw*s/rpm，>=0；默认关闭，尚未验证噪声。 */
#endif
#ifndef CHASSIS_INTEGRAL_LIMIT_RAW
#define CHASSIS_INTEGRAL_LIMIT_RAW 1000.0f /* 积分项上限，raw，[0,实际限流]，防止历史累加。 */
#endif
#ifndef CHASSIS_INTEGRAL_ERROR_SCALE
#define CHASSIS_INTEGRAL_ERROR_SCALE 0.01f /* 变速积分 a，1/rpm，>=0；权重 1/(1+a*abs(e))。 */
#endif
#ifndef CHASSIS_CURRENT_LIMIT_RAW
#define CHASSIS_CURRENT_LIMIT_RAW 4915.0f /* 实际电流限幅，C620 raw，(0,16384]，约 6 A，待实测。 */
#endif
#ifndef CHASSIS_KEYBOARD_T1_MS
#define CHASSIS_KEYBOARD_T1_MS 200U /* 键盘直线段结束时间，ms，>0；值越大起步越慢。 */
#endif
#ifndef CHASSIS_KEYBOARD_T3_MS
#define CHASSIS_KEYBOARD_T3_MS 1000U /* 键盘加速完成时间，ms，>T1；反向零速段同样用此时长。 */
#endif
#ifndef CHASSIS_KEYBOARD_LINEAR_RATIO
#define CHASSIS_KEYBOARD_LINEAR_RATIO (1.0f / 3.0f) /* T1 时达到目标的比例，(0,1)，影响起步斜率。 */
#endif
#ifndef CHASSIS_KEYBOARD_LOG_SHAPE
#define CHASSIS_KEYBOARD_LOG_SHAPE 4.0f /* 对数曲线形状系数，>0，无量纲；修改后复核单调性。 */
#endif
#ifndef CHASSIS_ID1_SIGN
#define CHASSIS_ID1_SIGN (-1) /* CAN1 ID1 右前轮安装方向，±1，同时转换反馈与电流。 */
#endif
#ifndef CHASSIS_ID2_SIGN
#define CHASSIS_ID2_SIGN 1 /* CAN1 ID2 左前轮安装方向，±1。 */
#endif
#ifndef CHASSIS_ID3_SIGN
#define CHASSIS_ID3_SIGN 1 /* CAN1 ID3 左后轮安装方向，±1。 */
#endif
#ifndef CHASSIS_ID4_SIGN
#define CHASSIS_ID4_SIGN (-1) /* CAN1 ID4 右后轮安装方向，±1。 */
#endif
#endif /* TASK_CHASSIS_CONFIG_H */
