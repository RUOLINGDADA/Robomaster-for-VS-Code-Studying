#ifndef __POWER_H_
#define __POWER_H_
#include "main.h"
#include "hrtim.h"
/*
                       _oo0oo_
                      o8888888o
                      88" . "88
                      (| -_- |)
                      0\  =  /0
                    ___/`---'\___
                  .' \\|     |// '.
                 / \\|||  :  |||// \
                / _||||| -:- |||||- \
               |   | \\\  -  /// |   |
               | \_|  ''\---/''  |_/ |
               \  .-\__  '-'  ___/-. /
             ___'. .'  /--.--\  `. .'___
          ."" '<  `.___\_<|>_/___.' >' "".
         | | :  `- \`.;`\ _ /`;.`/ - ` : | |
         \  \ `_.   \_ __\ /__ _/   .-` /  /
     =====`-.____`.___ \_____/___.-`___.-'=====
                       `=---='
     ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

               佛祖保佑         永不炸管
*/
#define SINGLE_CUP_V_MAX 2.60f            // 单节电容最大电压

#define CUP_V_CLOSE 4.5f                  // 电容关闭电压
#define CUP_V_MAX   SINGLE_CUP_V_MAX * 9  // 电容最大电压
#define OUT_V_VALUE 26.0f                 // 输出电压
#define EXPECT_POWER_BUFFER 50.0f         // 能量缓冲设定值

#define CHARGE_I_MAX 7.0f
#define COMPENSATE_I_MAX 12.5f

// 导通占空比为最大占空比 buck和boost最大占空比不能大于它
// 导通占空比极限为98%(示波器观察到下管没有驱动波形)，但自举电容可能举不上去
#define CONDUCTION_DUTY 0.97f
#define BUCK_DUTY_MAX 0.97f
#define BUCK_DUTY_MIN 0.01f
#define BOOST_DUTY_MAX 0.90f
#define BOOST_DUTY_MIN 0.03f

// 电压外环积分限幅值须保证在*Ki后依然能够达到PID输出限幅值
// 电流内环积分限幅值须保证在*Ki后依然能够>1
// 占空比定义为上管占空比 ，boost使用1-PID输出，buck使用正反馈
// 电感温升电流为12.5A，饱和电流(衰减20%)为23A

// PID参数
#define BOOST_I_P 0.0158f
#define BOOST_I_I 0.0032f
#define BOOST_I_D 0
#define BOOST_I_PMAX 12.5f
#define BOOST_I_DMAX 0
#define BOOST_I_IMAX (1/BOOST_I_I)

#define BOOST_V_P 11.8479f
#define BOOST_V_I 0.1136f
#define BOOST_V_D 0
#define BOOST_V_PIDMAX 12.5f
#define BOOST_V_PMAX 10.0f
#define BOOST_V_DMAX 0
#define BOOST_V_IMAX (BOOST_V_PIDMAX/BOOST_V_I)

#define BUCK_I_P 0.0133f
#define BUCK_I_I 0.0038f
#define BUCK_I_D 0
#define BUCK_I_PMAX 1.0f
#define BUCK_I_DMAX 0
#define BUCK_I_IMAX (1/BUCK_I_I)

#define BUCK_V_P 3.5615f
#define BUCK_V_I 0.0499f
#define BUCK_V_D 0
#define BUCK_V_PIDMAX 12.5f
#define BUCK_V_PMAX 0.1f
#define BUCK_V_DMAX 0
#define BUCK_V_IMAX (BUCK_V_PIDMAX/BUCK_V_I)

// 功率环 正反馈
#define POWER_P 0.2f
#define POWER_I 0.05f
#define POWER_D 0.0f
#define POWER_PIDMAX 15.0f
#define POWER_PMAX 20.0f
#define POWER_IMAX (POWER_PIDMAX/POWER_I)
#define POWER_DMAX 10.0f
// 能量缓冲环 负反馈
#define POWER_BUFFER_P -2.0f
#define POWER_BUFFER_I 0
#define POWER_BUFFER_D 0
#define POWER_BUFFER_PMAX 60.0f
#define POWER_BUFFER_IMAX 0.0f
#define POWER_BUFFER_DMAX 0.0f

typedef enum
{
  NormalMode,
  OnlyChangeMode,
  OnlyCompensateMode
} Power_Limit_mode_t;

typedef struct
{
  enum
  {
    Standby,
    Buck,
    Boost,
  } power_mode; // 电源环路工作模式
  enum
  {
    Wait,
    Init,
    Start,
    Run,
    ModeSwitch,
    Error,
  } running_status; // 电源运行状态
  union
  {
    uint8_t value;
    struct
    {
      FunctionalState CupEnable : 1;              // 电容使能
      FunctionalState CupUndervoltage : 1;        // 电容欠压
      FunctionalState PowerCompensationLimit : 1; // 补偿达到上限
      FunctionalState RESERVER : 3;               // 保留位
      FunctionalState CanLossOfConnection : 1;    // Can通讯失联
      FunctionalState PowerLoopError : 1;         // 电源环路异常
    } flag;
  } status;
  union Power_Status_t
  {
    uint8_t value;
    struct Power_Status_Flag_t
    {
      FunctionalState OutputOvervoltage : 1;       // 输出过压
      FunctionalState CupOvervoltage : 1;          // 超级电容过压
      FunctionalState InsufficientPowerBuffer : 1; // 能量缓冲不足
      FunctionalState LoopOvercurrent : 1;         // 环路过流
      FunctionalState RESERVER : 5;                // 保留位
    } flag;
  } error_status;
  PIDConfig_t boost_I_PID; // Boost模式电流环的PID配置
  PIDConfig_t buck_I_PID;  // Buck模式电流环的PID配置
  PIDConfig_t boost_V_PID; // Boost模式电压环的PID配置
  PIDConfig_t buck_V_PID;  // Buck模式电压环的PID配置
  struct
  {
    float setting_cup_V;  // 设定电容电压
    float setting_out_V;  // 设定输出电压
    float expect_cup_V;   // 目标电容电压
    float expect_out_V;   // 目标输出电压
    float setting_max_I;  // 设定最大电流
    float buck_max_duty;  // buck最大占空比
    float buck_min_duty;  // buck最小占空比
    float boost_max_duty; // boost最大占空比
    float boost_min_duty; // buck最小占空比
    float* power_V_out;   // 输出电压指针
  } data;
} Power_Loop_t;

typedef struct
{
  enum
  {
    cup_charge,               // 充电模式
    cup_compensate,             // 补偿模式
  } mode;
  PIDConfig_t Power_PID;        // 功率环PID配置
  PIDConfig_t Power_Buffer_PID; // 功率缓冲环PID配置
  struct
  {
    float Pset;                // 设置功率
    float Pcup;                // 电容功率
    float Pin;                 // 输入功率
    float Pout;                // 输出功率
    float power_buffer;        // 能量缓冲
  } data;
  union
  {
    uint8_t value;
    struct
    {
      Power_Limit_mode_t power_limit_mode : 2;
      FunctionalState RESERVER : 6;              // 保留位

    } flag;
  } status;
  struct
  {
    float Icompensate_max;    // 最大补偿电流
    float Icharge_max;        // 最大充电电流
    float expect_i;           // 期望电流
  } config_data;
  struct 
  {
    Timestamp_t rx_timestamp; // 最近接收时间戳
    Timestamp_t compensate_timestamp; //功率补偿时间戳
  }timestamp;
  
  
} Power_Limit_Loop_t;


extern Power_Loop_t Power;
extern Power_Limit_Loop_t Power_limit;
extern uint16_t BuckPWM, BoostPWM, SampPWM;

void Init_Power(void);
void Status_Control(void);
void Power_Loop(void);
void Power_Limit_Loop(void);
void Power_Error_Taip(void);
void Power_Loop_Mode(void);
void Power_Mode_Switch(uint32_t power_mode);
#endif
/*********************End of File**********************************/
