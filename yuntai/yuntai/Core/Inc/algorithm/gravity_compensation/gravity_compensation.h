/**
 * @file gravity_compensation.h
 * @brief 连续编码器角度到重力补偿电流的纯算法模块。
 *
 * 输入使用连续编码器计数，输出使用 GM6020 电流原始值。
 * 模块只保存配置和初始化状态，不访问 CAN、FreeRTOS、UART 或硬件。
 * 调用者必须在反馈在线时调用 Update；反馈失效时由上层直接输出零电流。
 */

#ifndef ALGORITHM_GRAVITY_COMPENSATION_H
#define ALGORITHM_GRAVITY_COMPENSATION_H /* 防止重力补偿接口被重复包含。 */

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  bool enabled; /* true 时计算补偿；false 时 Update 始终返回 0 raw。 */
  int32_t center_angle_raw; /* 补偿模型中心，单位连续编码器计数。 */
  float bias_current_raw; /* 补偿偏置，单位 GM6020 电流 raw，必须非负。 */
  float amplitude_current_raw; /* 余弦补偿幅值，单位 GM6020 电流 raw，必须非负。 */
  int8_t current_sign; /* 补偿方向，+1/-1；与闭环电流方向独立。 */
  float max_current_raw; /* 补偿绝对限幅，单位 GM6020 电流 raw，必须为正。 */
} GravityCompensation_ConfigTypeDef;

typedef struct {
  GravityCompensation_ConfigTypeDef config; /* 初始化时复制的参数，不保存外部指针。 */
  bool initialized; /* true 表示参数通过校验，可以执行 Update。 */
} GravityCompensation_HandleTypeDef;

/**
 * @brief 复制并校验重力补偿配置。
 * @param handle 调用者独占的算法对象；不能与另一云台轴共用。
 * @param config 补偿开关、中心、偏置、幅值、方向和限幅参数。
 * @retval true 配置有效并完成初始化。
 * @retval false 指针为空、方向非法、浮点参数非有限或幅值超过限幅；对象保持未初始化。
 * @note 仅普通任务初始化上下文调用；函数不访问硬件，不创建 RTOS 对象。
 */
bool GravityCompensation_Init(
    GravityCompensation_HandleTypeDef *handle,
    const GravityCompensation_ConfigTypeDef *config);

/**
 * @brief 按连续角度计算一周期重力补偿电流。
 * @param handle 已初始化的算法对象。
 * @param angle_total_raw 当前连续编码器角度，单位 count；不能传单圈角度。
 * @retval 带方向和限幅的 GM6020 电流 raw；模块禁用或对象无效时返回 0。
 * @note 仅任务上下文调用。公式为 sign*(bias+amplitude*cos(angle_offset))，不产生位置目标。
 */
float GravityCompensation_Update(
    GravityCompensation_HandleTypeDef *handle,
    int32_t angle_total_raw);

#endif /* ALGORITHM_GRAVITY_COMPENSATION_H */
