/**
 * @file task_ins_config.h
 * @brief INS 唯一参数入口；当前使用 BMI088 六轴，磁场参数保留供后续验证。
 *
 * 静止无法完成时先排除振动；六轴航向会长期漂移，不能靠增加重力反馈增益消除。
 * BMI 使用 [-X,+Y,-Z] 映射；磁采集与反馈由 INS_MAG_ENABLE 控制。
 */
#ifndef TASK_INS_CONFIG_H
#define TASK_INS_CONFIG_H /* 防止重复包含。 */

#ifndef INS_MAG_ENABLE
#define INS_MAG_ENABLE 0U /* 磁采集总开关，0/1；0跳过IST事务/中断通知/硬件检查并禁止磁融合。 */
#endif
#ifndef INS_MOUNT_CONFIRMED
#define INS_MOUNT_CONFIRMED 1U /* 0/1；1允许发布READY姿态供复测，0强制无效；本开关不代表物理三轴已验收。 */
#endif
#ifndef INS_BMI_ROTATION
#define INS_BMI_ROTATION {{-1.0f,0.0f,0.0f},{0.0f,1.0f,0.0f},{0.0f,0.0f,-1.0f}} /* 芯片→前右下，无量纲正交矩阵，det=+1；同步转换比力和角速度。 */
#endif
#ifndef INS_MAG_ROTATION
#define INS_MAG_ROTATION {{1.0f,0.0f,0.0f},{0.0f,-1.0f,0.0f},{0.0f,0.0f,-1.0f}} /* 磁芯片→前右下，无量纲正交矩阵，det=+1；决定磁航向反馈方向。 */
#endif
#ifndef INS_MAG_CALIBRATED
#define INS_MAG_CALIBRATED 1U /* 保留的磁校正确认门，0/1；仅INS_MAG_ENABLE=1且非MotionCal模式时允许磁融合，不代表质量已验收。 */
#endif
#ifndef INS_MAG_BIAS_UT
#define INS_MAG_BIAS_UT {29.3624f,16.6561f,38.2947f} /* 芯片系硬铁偏置，μT；先减偏置，再应用软铁矩阵。 */
#endif
#ifndef INS_MAG_SOFTIRON
#define INS_MAG_SOFTIRON {{0.984267f,0.031144f,-0.071657f},{0.031144f,1.086981f,0.040221f},{-0.071657f,0.040221f,0.928752f}} /* 软铁校正矩阵，无量纲、对称正定；在芯片系减偏置后左乘。 */
#endif
#ifndef INS_STILL_MS
#define INS_STILL_MS 2000U /* 连续静止窗口，ms，1~60000；运动或过期会重开。 */
#endif
#ifndef INS_STILL_GYRO_RAD_S
#define INS_STILL_GYRO_RAD_S 0.05f /* 静止角速度模长上限，rad/s，>0；不能识别阈值内恒速慢转。 */
#endif
#ifndef INS_STILL_ACCEL_FRACTION
#define INS_STILL_ACCEL_FRACTION 0.05f /* 静止比力相对 g 容差，比例，>0；振动大时应先处理安装。 */
#endif
#ifndef INS_STILL_GYRO_STD_RAD_S
#define INS_STILL_GYRO_STD_RAD_S 0.005f /* 窗口每轴标准差上限，rad/s，>0；越小越难完成。 */
#endif
#ifndef INS_AHRS_KP
#define INS_AHRS_KP 0.5f /* Mahony 比例增益，1/s，>=0；越大重力/航向收敛越快。 */
#endif
#ifndef INS_AHRS_KI
#define INS_AHRS_KI 0.0f /* Mahony 重力反馈积分增益，1/s²，>=0；0 禁用，不替代启动零偏。 */
#endif
#ifndef INS_AHRS_INTEGRAL_LIMIT_RAD_S
#define INS_AHRS_INTEGRAL_LIMIT_RAD_S 0.05f /* 每轴重力积分补偿上限，rad/s，>=0。 */
#endif
#ifndef INS_GRAVITY_MPS2
#define INS_GRAVITY_MPS2 9.80665f /* 标准重力，m/s²，>0；水平静止机体系应为 [0,0,-g]。 */
#endif
#ifndef INS_ACCEL_FULL_FRACTION
#define INS_ACCEL_FULL_FRACTION 0.05f /* 模长偏离 g 不超过此比例时全权重，>=0。 */
#endif
#ifndef INS_ACCEL_REJECT_FRACTION
#define INS_ACCEL_REJECT_FRACTION 0.20f /* 模长偏差达到此比例时拒绝重力校正，须大于全权重阈值。 */
#endif
#ifndef INS_MAG_MIN_UT
#define INS_MAG_MIN_UT 10.0f /* 校正磁场模长下限，μT，>0；过小不参与融合。 */
#endif
#ifndef INS_MAG_MAX_UT
#define INS_MAG_MAX_UT 100.0f /* 校正磁场模长上限，μT，须大于下限；不能证明范围内没有干扰。 */
#endif
#ifndef INS_MAG_RELATIVE_TOLERANCE
#define INS_MAG_RELATIVE_TOLERANCE 0.25f /* 相对初始场强容差，比例 (0,1)，超出回退六轴。 */
#endif
#ifndef INS_MAG_INNOVATION_RAD
#define INS_MAG_INNOVATION_RAD 0.7853981634f /* 最大航向创新，rad，默认45°；更大差异被当作异常。 */
#endif
#ifndef INS_MAG_RATE_LIMIT_RAD_S
#define INS_MAG_RATE_LIMIT_RAD_S 0.0523598776f /* 最大磁航向修正速度，rad/s，默认3°/s；>0。 */
#endif
#ifndef INS_MAG_RECOVERY_S
#define INS_MAG_RECOVERY_S 2.0f /* 磁观测恢复到全权重时间，s，>0；期间不重置四元数。 */
#endif
#ifndef INS_MAX_DT_S
#define INS_MAX_DT_S 0.01f /* 最大积分间隔，s，(0,0.1]；超过重启静止标定，不补积分。 */
#endif
#ifndef INS_GYRO_TIMEOUT_MS
#define INS_GYRO_TIMEOUT_MS 10U /* 陀螺新鲜度和等待上限，HAL ms，>0；超时使姿态无效。 */
#endif
#ifndef INS_ACCEL_TIMEOUT_MS
#define INS_ACCEL_TIMEOUT_MS 10U /* 加速度新鲜度上限，HAL ms，>0；超时姿态无效并重新初始化。 */
#endif
#ifndef INS_MAG_TIMEOUT_MS
#define INS_MAG_TIMEOUT_MS 50U /* 磁场新鲜度上限，HAL ms，>0；超时回退六轴，不刷新旧样本时间。 */
#endif
#ifndef INS_DMA_TIMEOUT_MS
#define INS_DMA_TIMEOUT_MS 2U /* 单次 SPI DMA 最大等待，ms，>0；超时中止 SPI 并重启。 */
#endif
#ifndef INS_SPI_TIMEOUT_MS
#define INS_SPI_TIMEOUT_MS 5U /* 初始化单次 SPI HAL 超时，ms，>0；正常采集不使用轮询。 */
#endif
#ifndef INS_I2C_TIMEOUT_MS
#define INS_I2C_TIMEOUT_MS 1U /* 单次 I2C HAL 超时，ms，>0；失败后退避，避免长期占用 CPU。 */
#endif
#ifndef INS_RETRY_MS
#define INS_RETRY_MS 1000U /* 初始化/掉线重试间隔，ms，>0；用 FreeRTOS Tick 调度。 */
#endif
#ifndef INS_TEMPERATURE_PERIOD_MS
#define INS_TEMPERATURE_PERIOD_MS 1000U /* 温度读取间隔，HAL ms，>0；传感器本身约1.28s更新。 */
#endif
#ifndef INS_LOG_PERIOD_MS
#define INS_LOG_PERIOD_MS 1000U /* 姿态状态日志间隔，ms，>0；9600 baud 下禁止逐帧打印。 */
#endif
#ifndef INS_MAG_CALIBRATION_PERIOD_MS
#define INS_MAG_CALIBRATION_PERIOD_MS 100U /* MotionCal原始磁场输出间隔，HAL ms，>=100；最多10Hz，仅标定模式使用。 */
#endif
#ifndef INS_ACCEL_RANGE_REG
#define INS_ACCEL_RANGE_REG 1U /* BMI088 ACC_RANGE 寄存器编码0~3，默认1=±6g。 */
#endif
#ifndef INS_ACCEL_CONF_REG
#define INS_ACCEL_CONF_REG 0xABU /* ACC_CONF，默认normal/800Hz；高半字节8~10、低半字节5~12。 */
#endif
#ifndef INS_GYRO_RANGE_REG
#define INS_GYRO_RANGE_REG 0U /* GYRO_RANGE 编码0~4，默认0=±2000°/s。 */
#endif
#ifndef INS_GYRO_BANDWIDTH_REG
#define INS_GYRO_BANDWIDTH_REG 2U /* GYRO_BANDWIDTH 编码0~7，默认2=1000Hz/116Hz。 */
#endif

_Static_assert(INS_MAG_ENABLE<=1U && INS_MOUNT_CONFIRMED<=1U && INS_MAG_CALIBRATED<=1U,
               "INS switches must be 0 or 1");
#endif
