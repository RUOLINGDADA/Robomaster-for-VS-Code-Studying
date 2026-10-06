/**
 * @file task_yaw_config.h
 * @brief Yaw 任务标定和控制参数。
 *
 * 不动时先检查标定许可和方向。过慢时先检查速度、电流限幅。
 * 摆动时逐项降低 P 或 I。静差时检查重力补偿和积分限幅。
 * 换向冲击时检查电流斜率。测速噪声大时减小滤波权重或 D。
 * 限位外无法向内移动时检查输入方向。修改后重新编译并烧录。
 */
#ifndef TASK_YAW_CONFIG_H
#define TASK_YAW_CONFIG_H /* 防止配置重复包含。 */

#include <stdint.h>

#define YAW_HARDWARE_TEST_MODE_OFF 0U /* 正式控制模式，0；使用摇杆和鼠标命令。 */
#define YAW_HARDWARE_TEST_MODE_CALIBRATION 1U /* 只读标定模式，1；手动移动并记录角度。 */
#define YAW_HARDWARE_TEST_MODE_ANGLE_LOOP 2U /* 固定角度测试模式，2；验证位置保持时使用。 */

#ifndef YAW_HARDWARE_TEST_MODE
#define YAW_HARDWARE_TEST_MODE YAW_HARDWARE_TEST_MODE_OFF /* 任务模式，0/1/2；正式运行选 OFF。 */
#endif

#define YAW_CALIBRATION_VALID 1U /* 标定许可，0/1；完成本轴标定后置 1。 */
#define YAW_TASK_PERIOD_MS 2U /* 控制周期，ms；改变后检查调度负载。 */
#define YAW_FEEDBACK_TIMEOUT_MS 100U /* 反馈期限，HAL ms；增大会延迟掉线清零。 */
#define YAW_COMMAND_TIMEOUT_MS 100U /* 输入期限，HAL ms；过期冻结目标并继续位置保持。 */
#define YAW_TASK_LOG_PERIOD_MS 500U /* 诊断周期，ms；串口刷屏时增大。 */
#define YAW_CALIBRATION_CENTER_ANGLE_RAW 2100 /* 中位连续角度，count；重新标定后填写。 */
#define YAW_CALIBRATION_MIN_ANGLE_RAW 115 /* 软件下界，连续 count；按实测边界设置并保留余量。 */
#define YAW_CALIBRATION_MAX_ANGLE_RAW 4085 /* 软件上界，连续 count；按实测边界设置并保留余量。 */
#define YAW_MOTOR_CURRENT_SIGN 1 /* 闭环电流方向，+1/-1；正误差驱动反向时核对。 */
#define YAW_FEEDBACK_SPEED_SIGN 1 /* 反馈速度方向，+1/-1；与连续角度增量同向。 */

#define YAW_GRAVITY_COMPENSATION_ENABLE 0U /* 重力补偿开关，0/1；水平轴关闭，负载下垂时检查。 */
#define YAW_GRAVITY_COMPENSATION_BIAS_CURRENT_RAW 0.0f /* 重力偏置，GM6020 raw；保持时下垂可增加，温升高时降低。 */
#define YAW_GRAVITY_COMPENSATION_AMPLITUDE_CURRENT_RAW 0.0f /* 角度补偿幅值，GM6020 raw；不同角度静差不同时调整。 */
#define YAW_GRAVITY_CURRENT_SIGN 1 /* 重力电流方向，+1/-1；独立于闭环电流方向。 */

#define YAW_MAX_COMMAND_SPEED_RPM 40.0f /* 满输入速度，rpm；整体动作过快时减小。 */

#define YAW_VELOCITY_FEEDFORWARD_GAIN 1.2f /* 速度前馈，无量纲且非负；跟随落后时增加，超前时降低。 */

#define YAW_MAX_SPEED_TARGET_RPM 95.0f /* 速度目标上限，rpm；过快时降低，饱和时先检查此项。 */

#define YAW_POSITION_KP_RPM_PER_RAW 0.01f /* 位置 P，rpm/count；响应慢时增加，过冲时降低。 */
#define YAW_POSITION_KI_RPM_PER_RAW_S 0.0f /* 位置 I，rpm/(count·s)；持续静差时增加，摆动时降低。 */
#define YAW_POSITION_INTEGRAL_LIMIT_RPM 5.0f /* 位置积分上限，rpm，0~MAX_SPEED_TARGET；过冲时降低。 */
#define YAW_POSITION_KD_RPM_S_PER_RAW 0.0f /* 位置 D，rpm·s/count；增加制动，过大会放大噪声。 */
#define YAW_SPEED_KP_CURRENT_PER_RPM 200.0f /* 速度 P，raw/rpm；跟随慢时增加，测速噪声大时降低。 */
#define YAW_SPEED_KI_CURRENT_PER_RPM_S 20.0f /* 速度 I，raw/(rpm·s)；持续速度静差时增加，摆动时降低。 */

#define YAW_MAX_CURRENT_RAW 7000.0f /* 总电流上限，GM6020 raw，(0,16384]；增大会提高力矩和温升。 */
#define YAW_SPEED_KD_CURRENT_S_PER_RPM 1.5f /* 速度 D，raw·s/rpm；测速跳变造成冲击时降低。 */
#define YAW_SPEED_INTEGRAL_LIMIT_RAW YAW_MAX_CURRENT_RAW /* 速度积分上限，raw，0~MAX_CURRENT；恢复冲击大时降低。 */
#define YAW_CURRENT_SLEW_RAW_PER_S 60000.0f /* 电流斜率，raw/s；冲击大时降低，制动滞后时增加。 */
#define YAW_SPEED_FILTER_ALPHA 0.20f /* 测速权重，(0,1]；噪声大时降低，制动滞后时增加。 */

#define YAW_ANGLE_LOOP_TARGET_ANGLE_DEG 30.0f /* 测试目标，相对中位 deg；保持测试时设置，必须在软件边界内。 */
#define YAW_ANGLE_LOOP_LOG_PERIOD_MS 200U /* 角度测试日志周期，ms；串口刷屏时增大。 */

#define YAW_CALIBRATION_LOG_PERIOD_MS 200U /* 标定日志周期，ms；漏看角度变化时减小。 */
#define YAW_CALIBRATION_PROMPT_PERIOD_MS 3000U /* 标定提示周期，ms；提示过密时增大。 */
#endif





