/**
 ******************************************************************************
 * @copyright (c) 2023 - ~, Singularity
 * @file   : PID.h
 * @author : SY7_yellow
 * @brief  : PID控制器库
 * @date   : 2023-7-24
 * @par Change Log：
 * <table>
 * <tr><th>Date           <th>Version     <th>Author      <th>Description
 * <tr><td>2022-07-24     <td>1.0         <td>SY7_yellow  <td>创建初始版本
 * </table>
 *@details :
 * ============================================================================
 *                       How to use this driver
 * ============================================================================
 *
 * 
 ******************************************************************************
 * @attention:
 *
 * 文件编码：UTF-8,出现乱码请勿上传! \n
 * 修改后测试没问题记得修改版本号,未经过测试的请使用加上后缀alpha,beta...并且请
 * 勿合并到master. \n
 * 防御性编程,对输入参数做有效性检查,并返回错误号. \n
 * 不要包含太多不相关的头文件. \n
 * 若发现bug请提交issue,详细描述现象(必须)和复现条件(选填),以便对应的负责人能
 * 够准确定位修复. \n
 * File encoding:UTF-8,Do not upload garbled code!\n
 * Please remember to change the version number. If you have not been tested,
 * please use the suffix alpha,beta... And do not merge to master. \n
 * Defensive programming, where input arguments are checked for validity and
 * an error number is returned. \n
 * Don't include too many irrelevant headers. \n
 * If you find a bug, file an issue with a detailed description of the
 * phenomenon (required) and conditions for reoccurrence (optional) so that
 * the appropriate owner can locate the correct fix. \n
 *
 ******************************************************************************
 */
#ifndef __PID_H_
#define __PID_H_
#include "SEML_common.h"
#include "math_common.h"

/// @brief PID元素类型
typedef float PIDElem_t;
/// @brief PID限幅函数
#define Constrain_PID(amt, high, low) constrain(amt, high, low)
/**
 * @brief 积分抗饱和方式
 */
typedef enum
{
  anti_windup_none,               /**< 不限幅 */
  anti_windup_clamping,           /**< 积分遇限消弱法 */
  anti_windup_back_calculkation,  /**< 反馈抑制抗饱和 */
}PID_anti_windup_t;
/// @brief 断言是否为积分抗饱和方式
#define IS_PID_ANTI_WINDUP(x) ((x) == anti_windup_none ||(x) == anti_windup_clamping ||(x) == anti_windup_back_calculkation)

/**
 * @brief PID优化回调函数
 * @param PIDElem_t PID配置结构体指针
 * @param E_value 期望值
 * @param C_value 实际值
 */
typedef void (*PID_Callback_Fun_t)(void *, PIDElem_t E_value, PIDElem_t C_value);
/**
 * @brief PID参数
 */
typedef struct
{
  PIDElem_t Kp; /**< 比例因子 */
  PIDElem_t Ki; /**< 积分因子 */
  PIDElem_t Kd; /**< 微分因子 */
} PID_Param_t;
/**
 * @brief PID配置结构体
 */
typedef struct
{
  PID_Param_t Param; /**< PID参数 */

  float SampTime; /**< 采样时间 */

  PIDElem_t PIDMax;                 /**< PID最大值 */
  PIDElem_t PIDMin;                 /**< PID最大值 */
  PIDElem_t ErrConstrain;           /**< 误差限幅*/
  PIDElem_t ErrValue;               /**< 误差 */
  PIDElem_t lest_ErrValue;          /**< 上一次误差 */
  PIDElem_t ITerm;                  /**< 积分累加值 */
  PIDElem_t DiffValue;              /**< 误差的导数 */
  PID_anti_windup_t anti_windup;    /**< 积分抗饱和方式 */
  PIDElem_t Ki_Clamp;               /**< 反馈抑制抗饱和系数 */
  PIDElem_t Kd_lowpass;             /**< 微分低通滤波器系数 */
  PID_Callback_Fun_t Callback_Fun;  /**< PID回调函数 */

  PIDElem_t PIDout; /**< PID输出值 */
} PIDConfig_t;

/**
 * @brief PID配置初始化
 *     - 只配置最基础的pid参数
 *     - 抗积分饱和方式：钳位
 *     - 微分低通滤波:不滤波
 * @param[out] PID_Config PID配置结构体指针
 * @param[in] Kp 比例因子
 * @param[in] Ki 积分因子
 * @param[in] Kd 微分因子
 * @param[in] PID_Max 输出最大限幅值
 * @param[in] PID_Min 输出最小限幅值
 * @param[in] sample_time 采样时间(秒)
 */
void PID_Init(PIDConfig_t *PID_Config, PIDElem_t Kp, PIDElem_t Ki, PIDElem_t Kd, PIDElem_t PID_Max, PIDElem_t PID_Min, PIDElem_t ErrConstrain, float sample_time);

/**
 * @brief PID优化配置
 * @param[out] PID_Config PID配置结构体指针
 * @param[in] Ki_Clamp 抗积分饱和系数
 * @param[in] Kd_lowpass 微分低通滤波系数
 * @param[in] PID_Callback_Fun pid参数处理回调函数
 */
void PID_Config(PIDConfig_t *PID_Config, PIDElem_t Ki_Clamp, const PIDElem_t Kd_lowpass, const PID_Callback_Fun_t PID_Callback_Fun, const PID_anti_windup_t anti_windup);

/**
 * @brief 基础位置式PID控制器
 * @param[in,out] PIDConfig PID配置
 * @param[in] E_value 期望值
 * @param[in] C_value 实际值
 * @return 当前PID输出
 */
_FAST PIDElem_t Basic_PID_Controller(PIDConfig_t *PIDConfig, const PIDElem_t E_value, const PIDElem_t C_value);

/**
 * @brief 基础位置式PI控制器
 * @param[in,out] PIDConfig PID配置
 * @param[in] E_value 期望值
 * @param[in] C_value 实际值
 * @return 当前PID输出
 */
_FAST PIDElem_t Basic_PI_Controller(PIDConfig_t *PIDConfig, const PIDElem_t E_value, const PIDElem_t C_value);

/**
 * @brief PID重置
 * @param[out] PID_Config PID配置结构体指针
 * @param[in] ITerm 积分值(乘以ki之后的)
 * @param[in] lest_ErrValue 上一次误差值
 */
void PID_Reset(PIDConfig_t *PID_Config, const PIDElem_t ITerm, const PIDElem_t lest_ErrValue);
#endif
