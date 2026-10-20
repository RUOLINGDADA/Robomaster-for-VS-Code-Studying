/**
 * @file ins.h
 * @brief 云台 INS 的标定、安装转换和一致姿态快照。
 * 一个任务拥有处理状态；其他任务只能读取快照。接口不创建任务、不控制电机。
 */
#ifndef APP_INS_H
#define APP_INS_H /* 防止重复包含。 */
#include "algorithm/ahrs/ahrs.h"
#include <stdint.h>

typedef enum {
  INS_STATE_WAIT_STILL, /* 正在等待连续静止标定。 */
  INS_STATE_READY, /* 算法已建立姿态；有效性还取决于安装确认和新鲜度。 */
  INS_STATE_FAULT /* IMU 失效，等待重新初始化。 */
} Ins_StateTypeDef;

typedef enum {
  INS_MODE_INVALID, /* 无可使用姿态。 */
  INS_MODE_GYRO, /* 比力不可用，暂时仅陀螺传播。 */
  INS_MODE_SIX_AXIS, /* 陀螺仪和加速度融合，航向无磁约束。 */
  INS_MODE_NINE_AXIS /* 当前通过磁门控并使用航向反馈。 */
} Ins_ModeTypeDef;

typedef struct {
  Ahrs_ConfigTypeDef ahrs; /* 纯算法配置，参数不反向依赖任务头。 */
  float bmi_rotation[3][3]; /* 芯片到前右下云台系，必须正交且 det=+1。 */
  float mag_rotation[3][3]; /* 磁芯片到同一云台系，独立于 BMI088。 */
  float mag_bias_ut[3]; /* 硬铁偏置，原始磁芯片坐标 μT。 */
  float mag_softiron[3][3]; /* 原始磁芯片坐标，无量纲对称正定校正。 */
  bool mount_confirmed; /* 两个安装旋转经过实测确认。 */
  bool mag_calibrated; /* 安装后的完整磁标定参数已经验证。 */
  uint32_t still_ms; /* 连续静止时间，ms。 */
  float still_gyro_rad_s; /* 静止判定角速度模长上限，rad/s。 */
  float still_accel_fraction; /* 静止比力模长相对 g 容差。 */
  float still_gyro_std_rad_s; /* 标定窗口每轴最大标准差，rad/s。 */
  uint32_t gyro_timeout_ms; /* HAL 毫秒基准的陀螺新鲜度上限。 */
  uint32_t accel_timeout_ms; /* HAL 毫秒基准的比力新鲜度上限。 */
  uint32_t mag_timeout_ms; /* HAL 毫秒基准的磁场新鲜度上限。 */
} Ins_ConfigTypeDef;

typedef struct {
  float accel_mps2[3]; /* 未安装转换的 BMI088 芯片比力。 */
  float gyro_rad_s[3]; /* 未减零偏的 BMI088 芯片角速度。 */
  float mag_ut[3]; /* 未校正的 IST8310 芯片磁场。 */
  int16_t accel_raw[3]; /* 对应比力的芯片 LSB。 */
  int16_t gyro_raw[3]; /* 对应角速度的芯片 LSB。 */
  int16_t mag_raw[3]; /* 对应磁场的芯片 LSB。 */
  float temperature_c; /* BMI088 摄氏度。 */
  float dt_s; /* 两次已采集陀螺边沿的 DWT 时间差，s。 */
  uint32_t now_ms; /* 所有年龄比较使用 HAL_GetTick 毫秒。 */
  uint32_t gyro_ms; /* 本帧陀螺边沿 HAL 时间。 */
  uint32_t accel_ms; /* 最近真实加速度样本 HAL 时间。 */
  uint32_t mag_ms; /* 最近真实磁样本 HAL 时间。 */
  uint32_t temperature_ms; /* 最近成功温度事务 HAL 时间，不随缓存复用刷新。 */
  bool accel_valid; /* 曾成功采集比力，时间戳 0 也可能有效。 */
  bool mag_valid; /* 曾成功采集磁场。 */
  bool temperature_valid; /* 温度读数有效。 */
  uint32_t dropped_gyro; /* 累计合并/丢弃的陀螺事件数。 */
  uint32_t mag_overruns; /* 累计磁场旧数据覆盖次数。 */
} Ins_InputTypeDef;

typedef struct {
  float q[4]; /* 云台到初始水平参考系，[w,x,y,z]。 */
  float yaw_deg; /* 右转为正，[-180,180]，不是磁北航向。 */
  float pitch_deg; /* 抬头为正，[-90,90]。 */
  float roll_deg; /* 右倾为正，[-180,180]。 */
  float yaw_total_deg; /* 解回绕后的累计 Yaw，重置时归零，浮点长期精度有限。 */
  float accel_mps2[3]; /* 云台机体系比力，m/s²。 */
  float gyro_rad_s[3]; /* 云台机体系、已减零偏，rad/s。 */
  float gyro_bias_rad_s[3]; /* 云台机体系启动零偏，rad/s。 */
  float mag_ut[3]; /* 校正并安装转换后的磁场，μT。 */
  float mag_chip_ut[3]; /* 未校正磁芯片数据，μT，用于离线标定。 */
  int16_t accel_raw[3]; /* 芯片 LSB，不参与姿态坐标解释。 */
  int16_t gyro_raw[3]; /* 芯片 LSB。 */
  int16_t mag_raw[3]; /* 芯片 LSB。 */
  float temperature_c; /* 摄氏度，结合有效标志读取。 */
  float dt_s; /* 当前真实积分间隔，s。 */
  uint32_t gyro_ms; /* HAL 时间，ms。 */
  uint32_t accel_ms; /* HAL 时间，ms。 */
  uint32_t mag_ms; /* HAL 时间，ms。 */
  uint32_t temperature_ms; /* 最近成功温度读数的 HAL 时间，ms。 */
  uint32_t sequence; /* 每次发布样本递增，可回绕。 */
  uint32_t reset_sequence; /* 每次重新建立姿态递增；初次成功为 1。 */
  uint32_t dropped_gyro; /* 事件丢帧诊断计数，可回绕。 */
  uint32_t mag_overruns; /* 磁覆盖诊断计数，可回绕。 */
  Ins_StateTypeDef state; /* 启动、正常或故障。 */
  Ins_ModeTypeDef mode; /* 本次融合方式。 */
  bool attitude_valid; /* 安装已确认、已完成标定且 IMU 新鲜。 */
  bool mount_confirmed; /* 安装确认开关的快照。 */
  bool mag_calibrated; /* 磁标定确认开关的快照。 */
  bool accel_valid; /* 最近比力新鲜。 */
  bool mag_valid; /* 最近磁数据新鲜，不等于已经通过融合门控。 */
  bool mag_used; /* 当前真正使用磁反馈。 */
  bool temperature_valid; /* 最近温度非无效码；年龄由 temperature_ms 判断。 */
} Ins_SnapshotTypeDef;

/** @brief 配置唯一 INS 实例。@param config 参数。@return 参数有效为 true。
 * @note 仅所有者任务在开始采集前调用，不允许与 Process 并发。 */
bool Ins_Init(const Ins_ConfigTypeDef *config);
/** @brief 消费一个新陀螺样本并发布快照。@param input 同一采集周期的输入。
 * @return 无；非法样本标为故障，静止未满足时只发布原始观测。仅所有者任务调用。 */
void Ins_Process(const Ins_InputTypeDef *input);
/** @brief 记录 BMI088 故障并清启动窗口。@return 无。
 * @note 仅所有者任务调用，旧姿态只供诊断，恢复必须重新静止校准。 */
void Ins_SetFault(void);
/** @brief 复制一致快照并重新检查年龄。@param output 输出，非 NULL。
 * @return 至少发布过一份快照为 true；不表示姿态有效，须检查 attitude_valid。
 * @note 仅任务调用，短临界区复制，不能直接访问内部状态。 */
bool Ins_GetSnapshot(Ins_SnapshotTypeDef *output);
#endif
