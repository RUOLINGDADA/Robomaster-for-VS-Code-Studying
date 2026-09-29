/**
  ******************************************************************************
  * @copyright (c) 2023 - ~, Singularity
  * @file   : DCDC.h
  * @author : SY7_yellow
  * @brief  : 直流变换器控制库
  * @date   : 2023-7-15
  * @par Change Log：
  * <table>
  * <tr><th>Date           <th>Version     <th>Author      <th>Description
  * <tr><td>2022-07-15     <td>0.1-alpha   <td>SY7_yellow  <td>从超电代码移植过来
  * <tr><td>2022-07-25     <td>0.1         <td>SY7_yellow  <td>调试通过,代码能跑
  * <tr><td>2022-07-26     <td>0.2         <td>SY7_yellow  <td>移植了mppt算法
  * </table>
  * @todo    : 使用q16格式，尽量摆脱使用浮点型
  * @details :
  * ============================================================================
  *                       How to use this driver  
  * ============================================================================
  * 1. 在初始化的时候调用BuckBoost_Init，并且配置好参数
  * 2. 使用BuckBoost_Config设定输出电压和输出电流
  * 3. 在控制开关管的PWM定时器中断中调用BuckBoost_Loop_Control,若运算频率不够可以
  *    适当降低控制频率.注意控制频率不得高于adc的采样频率.
  * 4. 在较低频率的定时器(如100Hz定时器)中调用BuckBoost_Status_Control，通过
  *    Power_switch来控制环路的开启和关闭。
  * 5. 本库适用于buck变换器,boost变换器,四开关buck-boost变换器。
  * 6. 有些定时器(如hrtim)可能会出现占空比为0时候变成上管导通，此时需要对其进行限
  *    幅，使得cmp值>=1.
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
  ******************************************************************************
  */
#ifndef _DCDC_H_
#define _DCDC_H_
#include "SEML_common.h"
#include "math_common.h"
#include "PID.h"
/**
 * @addtogroup 解算库
 * @{
 */
/**
 * @addtogroup 电源控制库
 * @{
 */
/**
 * @addtogroup 直流变换器控制
 * @{
 */
/**
 * @addtogroup 四开关buck-boost变换器控制器
 * @{
 */

/**
 * @brief 四开关控制器dcdc参数结构体
 */
typedef struct
{
  enum
  {
    Buck = 1,
    Boost,
    Buck_Boost
  } power_mode;             /**< 电源环路工作模式 */

  enum
  {
    Wait = 0,
    Init,
    Start,
    Run,
    Error,
  } running_status;         /**< 电源运行状态 */

  PIDConfig_t Iloop_PID;    /**< 电流环的PID配置 */
  PIDConfig_t Vloop_PID;    /**< 电压环的PID配置 */

  struct
  {
    float expect_out_V;     /**< 目标输出电压 */
    float buck_max_duty;    /**< buck最大占空比 */
    float boost_min_duty;   /**< boost最小占空比 */
  } data;

  struct
  {
    float conduction_duty;  /**< 导通占空比 */
    float boost_min_duty;   /**< 最小boost占空比 */
    float control_frec;      /**< 采样频率 */
  } config_data;

  struct
  {
    float setting_max_I;    /**< 设定最大电流 */
    float setting_out_V;    /**< 设定输出电压 */
  } setting_data;

} BuckBoost_Config_t;

/**
 * @brief 四开关buck-boost电源环路初始化
 * @param[out] Power_Config 电源环路配置指针
 * @param[in] max_transform_ratio 最大变换比
 * @param[in] conduction_duty 导通占空比
 * @param[in] control_frec 控制频率(Hz,赫兹)
 * @param[in] ILoop_Param 电流环pid参数
 * @param[in] VLoop_Param 电压环pid参数
 */
void BuckBoost_Init(BuckBoost_Config_t *Power_Config, float max_transform_ratio,float conduction_duty, float control_frec, PID_Param_t ILoop_Param, PID_Param_t VLoop_Param);

/**
 * @brief 四开关buck-boost参数配置
 * @param[out] Power_Config 电源环路配置指针
 * @param[in] Vset 设定电压(V,伏)
 * @param[in] Imax 最大电流(A,安)
 */
void BuckBoost_Config(BuckBoost_Config_t *Power_Config, float Vset, float Imax);

/**
 * @brief 四开关buck-boost电源环路控制
 * @param[in,out] Power_Config 电源环路配置指针
 * @param[in] Vout 输出电压(V,伏)
 * @param[in] Iout 输出电流(A,安)
 * @param[out] buck_duty buck占空比(D,上管占空比)
 * @param[out] boost_duty boost占空比(1-D,上管占空比)
 * @attention 传入的输出电流需要以输出方向为正方向
 */
_FAST void BuckBoost_Loop_Control(BuckBoost_Config_t *Power_Config,const float Vout,const float Iout, float *buck_duty, float *boost_duty);

/**
 * @brief 四开关buck-boost状态控制
 * @param[in,out] Power_Config 电源环路配置
 * @param[in] Vin 输入电压(V,伏)
 * @param[in] Vout 输出电压(V,伏)
 * @param[in] Power_switch 电源开关状态
*/
_FAST void BuckBoost_Status_Control(BuckBoost_Config_t *Power_Config, float Vin, float Vout, FunctionalState Power_switch);
/// @} 四开关buck-boost算法结束
/**
 * @addtogroup MPPT算法
 * @{
*/

typedef struct 
{
  float Pin;              /**< 输入功率 */
  float lest_Pin;         /**< 上一次输入功率 */
  float step_longth;      /**< 步长 */
  float max_duty;         /**< 最大占空比 */
  float min_duty;         /**< 最小占空比 */
  float mppt_duty;        /**< mppt占空比 */
  enum
  {
    mppt_add = 0,  
    mppt_sub
  } mppt_direction;       /**< 占空比移动方向 */
} mppt_config_t;

/**
 * @brief 最大功率点追踪初始化
 * @param[out] mppt_config 最大功率点配置结构体
 * @param[in] max_duty 最大占空比
 * @param[in] min_duty 最小占空比
 * @param[in] step_longth 每次迭代步长
 */
void mppt_Init(mppt_config_t *mppt_config,float max_duty,float min_duty, float step_longth);

/**
 * @brief 最大功率点追踪
 * @param[in,out] mppt_config 最大功率点配置结构体
 * @param[in] Vin 输入电压
 * @param[in] Iin 输入电流
 * @return mppt占空比(上管占空比)
 */
float mppt(mppt_config_t *mppt_config,float Vin,float Iin);

/// @} MPPT算法结束
/// @} 直流变换器控制结束
/// @} 电源控制库结束
/// @} 解算库结束
#endif