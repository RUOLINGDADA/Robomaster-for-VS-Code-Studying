/**
 * @file ahrs.h
 * @brief 无 HAL/RTOS 依赖的 Mahony 姿态与坐标变换。
 *
 * 使用前右下右手坐标、机体到参考系主动四元数 [w,x,y,z]。
 * 加速度输入是比力，水平静止为 [0,0,-g]；角速度为机体系 rad/s。
 */
#ifndef ALGORITHM_AHRS_H
#define ALGORITHM_AHRS_H /* 防止重复包含。 */
#include <stdbool.h>

typedef struct {
  float kp; /* Mahony 比例增益，1/s，非负；对单位向量叉积生效。 */
  float ki; /* Mahony 积分增益，1/s²，非负；0 禁用积分。 */
  float integral_limit_rad_s; /* 三轴积分补偿各自上限，rad/s。 */
  float gravity_mps2; /* 静止比力模长，m/s²，正数。 */
  float accel_full_fraction; /* 与 g 的相对偏差小于此值时全权重。 */
  float accel_reject_fraction; /* 相对偏差达到此值时拒绝重力校正。 */
  float mag_min_ut; /* 磁场模长下限，μT。 */
  float mag_max_ut; /* 磁场模长上限，μT。 */
  float mag_relative_tolerance; /* 相对初始磁场模长容差，比例 (0,1)。 */
  float mag_innovation_rad; /* 最大航向创新绝对值，rad，(0,π]。 */
  float mag_rate_limit_rad_s; /* 磁校正最大角速度，rad/s。 */
  float mag_recovery_s; /* 从失效恢复到完整磁权重的时间，s，正数。 */
  float max_dt_s; /* 单次允许最大积分间隔，s；超出不修改状态。 */
} Ahrs_ConfigTypeDef;

typedef struct {
  Ahrs_ConfigTypeDef config; /* 句柄私有参数。 */
  float q[4]; /* 单位四元数，机体到初始水平参考系。 */
  float integral_rad_s[3]; /* 重力反馈积分，rad/s；受限且可禁用。 */
  float mag_reference[3]; /* 初始化磁场在参考系中的单位方向。 */
  float mag_reference_ut; /* 初始校正磁场模长，μT。 */
  float mag_weight; /* 磁场恢复权重 [0,1]。 */
  float accel_weight; /* 本次重力校正权重 [0,1]。 */
  bool mag_reference_valid; /* 水平分量足够时建立参考。 */
  bool mag_used; /* 本次通过全部门控并使用磁反馈。 */
  bool initialized; /* 配置与初始倾角有效。 */
} Ahrs_HandleTypeDef;

/** @brief 验证右手安装旋转。@param rotation 行为机体系、列为芯片系的矩阵。
 * @return 有限、正交且 det=+1 时 true；拒绝镜像。普通调用，无副作用。 */
bool Ahrs_IsRotation(const float rotation[3][3]);
/** @brief 旋转或标定向量。@param matrix 3×3 矩阵。@param input 输入。@param output 输出。
 * @return 无；允许 input 与 output 相同。普通调用。 */
void Ahrs_Transform(const float matrix[3][3], const float input[3], float output[3]);
/** @brief 建立真实倾角和零航向。@param handle 独立句柄。@param config 参数。
 * @param accel_mps2 机体系比力。@param mag_ut 可为 NULL 的已校正机体系磁场。
 * @return 成功为 true，失败句柄未初始化；调用者串行使用同一实例。 */
bool Ahrs_Init(Ahrs_HandleTypeDef *handle, const Ahrs_ConfigTypeDef *config,
               const float accel_mps2[3], const float mag_ut[3]);
/** @brief 根据实际间隔更新姿态。@param handle 独立句柄。@param gyro_rad_s 已减零偏的角速度。
 * @param accel_mps2 比力，NULL 表示过期。@param mag_ut 校正磁场，NULL 表示不可使用。
 * @param dt_s 秒，必须 (0,max_dt_s]。@return 成功为 true；失败保留原状态。 */
bool Ahrs_Update(Ahrs_HandleTypeDef *handle, const float gyro_rad_s[3],
                 const float accel_mps2[3], const float mag_ut[3], float dt_s);
/** @brief 四元数转标准 ZYX 欧拉角。@param handle 已初始化句柄。
 * @param angles_rad 输出 [yaw,pitch,roll]，rad。@return 无；俯仰±90°时欧拉角存在奇异性。 */
void Ahrs_GetEuler(const Ahrs_HandleTypeDef *handle, float angles_rad[3]);
#endif
