/**
 * @file task_ins_runtime.c
 * @brief BMI088 DRDY→SPI DMA→任务融合，IST8310 分步恢复与低频日志。
 *
 * 一个任务拥有全部缓冲和采集状态。中断只记录整数事件、释放片选并通知。
 * 通知仅用于唤醒；实际待处理数据由递增序号决定，等待 DMA 时合并通知不会丢失事件。
 */
#include "task/task_ins/task_ins_runtime.h"
#include "task/task_ins/task_ins_config.h"
#include "app/ins/ins.h"
#include "app/log/log.h"
#include "bsp/bmi088/bmi088.h"
#include "bsp/ist8310/ist8310.h"
#include "spi.h"
#include "i2c.h"
#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include <limits.h>
#include <math.h>
#include <string.h>

#define INS_EVENT_SENSOR 1UL /* 任务通知 bit0：至少一个 DRDY，序号保存真实事件。 */
#define INS_EVENT_DMA 2UL /* 任务通知 bit1：DMA 完成/错误，状态由驱动保存。 */
#define INS_MAG_POLL_MS 5U /* 200Hz 磁力计状态兜底轮询间隔，HAL ms。 */
#define INS_MOTIONCAL_COUNTS_PER_LSB 3L /* PJRC UT_PER_COUNT=0.1μT；IST8310=0.3μT/LSB，芯片原始值乘3。 */

typedef struct {
  uint32_t sequence; /* DRDY 次数，可回绕；ISR 写，任务临界区复制。 */
  uint32_t cycles; /* 最新边沿 DWT 计数，32位，可回绕。 */
  uint32_t ms; /* 同一边沿的 HAL 时间，ms。 */
} InsTask_StampTypeDef;

/* 只定位任务中的失败出口，不据此推断接线、干扰或碰撞的物理根因。 */
typedef enum {
  INS_TASK_FAULT_NONE = 0, /* 尚未记录采集故障。 */
  INS_TASK_FAULT_INIT, /* 1：BMI初始化/ID/配置回读失败。 */
  INS_TASK_FAULT_GYRO_GAP, /* 2：连续陀螺边沿HAL间隔超时。 */
  INS_TASK_FAULT_GYRO_DMA, /* 3：陀螺DMA失败，结合DMA失败阶段。 */
  INS_TASK_FAULT_ACCEL_DMA, /* 4：加计DMA失败。 */
  INS_TASK_FAULT_TEMP_DMA, /* 5：温度DMA失败。 */
  INS_TASK_FAULT_DT, /* 6：DWT积分间隔非法。 */
  INS_TASK_FAULT_GYRO_AGE, /* 7：处理时陀螺样本过期。 */
  INS_TASK_FAULT_ACCEL_MISSING, /* 8：启动后一直未取得加计样本。 */
  INS_TASK_FAULT_ACCEL_AGE, /* 9：加计样本过期。 */
  INS_TASK_FAULT_GYRO_MISSING /* 10：等待陀螺新边沿超时。 */
} InsTask_FaultTypeDef;

typedef enum {
  INS_TASK_DMA_FAILURE_NONE = 0, /* 本次DMA成功或尚未读取。 */
  INS_TASK_DMA_FAILURE_START, /* 1：DMA启动被拒绝。 */
  INS_TASK_DMA_FAILURE_TIMEOUT, /* 2：等待完成超时。 */
  INS_TASK_DMA_FAILURE_FINISH /* 3：完成状态/解析失败。 */
} InsTask_DmaFailureTypeDef;

typedef struct {
  Bmi088_HandleTypeDef bmi; /* 主 RAM 长期 DMA 缓冲，唯一所有者。 */
  Ist8310_HandleTypeDef mag; /* I2C 所有者句柄。 */
  Ins_InputTypeDef input; /* 最新已成功采集的数据，不用旧缓存刷新时间。 */
  uint32_t gyro_consumed; /* 已服务的陀螺序号。 */
  uint32_t accel_consumed; /* 已服务的比力序号。 */
  uint32_t mag_consumed; /* 已服务的磁序号。 */
  uint32_t previous_cycles; /* 上一成功采集陀螺边沿的 DWT 计数。 */
  uint32_t last_gyro_ms; /* 最新成功陀螺事务时间，HAL ms；无样本时从初始化完成计时。 */
  uint32_t imu_start_ms; /* 本次初始化完成时间，HAL ms；检测从未出现的加速度 DRDY。 */
  uint32_t last_mag_poll_ms; /* 磁兜底状态读取时间，HAL ms。 */
  uint32_t last_temp_attempt_ms; /* 温度读取调度时间，HAL ms。 */
  uint32_t last_log_ms; /* 状态日志调度时间，HAL ms。 */
  uint32_t last_mag_calibration_ms; /* MotionCal发送尝试调度时间，HAL ms。 */
  uint32_t mag_calibration_sample_ms; /* 最近成功提交的原始磁样本时间，HAL ms。 */
  TickType_t bmi_retry_tick; /* 最近 BMI 初始化尝试，FreeRTOS Tick。 */
  TickType_t mag_retry_tick; /* 最近磁初始化尝试，FreeRTOS Tick。 */
  bool have_cycles; /* DWT 前次采样有效，不能用计数0代替。 */
  bool have_mag_calibration_sample; /* 首次标定帧已提交；时间戳0也可能是有效样本。 */
  InsTask_FaultTypeDef last_fault; /* 上次失败出口；恢复后保留，不能当作当前状态。 */
  InsTask_DmaFailureTypeDef dma_failure; /* 本次DMA失败阶段，只在任务中访问。 */
  uint32_t fault_count; /* 采集故障累计次数，可回绕；不是丢帧数。 */
  uint32_t init_attempts; /* BMI初始化尝试次数，可回绕。 */
  uint32_t fault_ms; /* 上次故障HAL ms，0也可能有效。 */
  uint32_t fault_spi_error; /* Abort前捕获的HAL SPI错误位，不能用Abort后的值替代。 */
  uint32_t dma_spi_error; /* 本次DMA失败、Abort前的SPI错误位。 */
  InsTask_DmaFailureTypeDef fault_dma_failure; /* 上次故障的DMA失败阶段，重试不覆盖。 */
} InsTask_RuntimeTypeDef;

static InsTask_RuntimeTypeDef g_runtime; /* 仅 task_ins 访问，不能放 CCMRAM。 */
static TaskHandle_t g_ins_task; /* 先绑定任务再接受通知；ISR 中只读。 */
static volatile bool g_accept_imu; /* ISR 的初始化门，仅任务写。 */
static volatile InsTask_StampTypeDef g_gyro_stamp, g_accel_stamp, g_mag_stamp; /* 中断写，任务临界区复制。 */

_Static_assert(pdMS_TO_TICKS(INS_DMA_TIMEOUT_MS)>0U,"DMA timeout must cover a Tick");
_Static_assert(pdMS_TO_TICKS(INS_RETRY_MS)>0U,"retry must cover a Tick");
_Static_assert(pdMS_TO_TICKS(INS_GYRO_TIMEOUT_MS)>0U,"watchdog must cover a Tick");
_Static_assert(INS_TEMPERATURE_PERIOD_MS>0U && INS_LOG_PERIOD_MS>0U,
               "INS periods must be positive milliseconds");
_Static_assert(INS_MAG_CALIBRATION_PERIOD_MS>=100U,"MotionCal output must not exceed 10 Hz");
_Static_assert(INS_MAG_ENABLE!=0U || LOG_INS_MAG_CALIBRATION_ENABLE==0U,
               "MotionCal requires magnetic acquisition");

/* 初始化延时向上覆盖所需毫秒；不得在任务运行中用它等待磁复位。 */
static void InsTask_DelayMs(uint32_t ms) {
  TickType_t ticks=pdMS_TO_TICKS(ms);
  if (ticks==0U) { ticks=1U; }
  vTaskDelay(ticks+1U); /* 覆盖调用发生在 Tick 边界前的不足一周期情况。 */
}

/* 组装参数，而不是让驱动/算法包含任务配置。 */
static Ins_ConfigTypeDef InsTask_MakeConfig(void) {
  /* 标定模式只采集原始磁场；即使已有参数有效，也暂时关闭磁反馈以便重新标定。 */
  Ins_ConfigTypeDef c={
    .ahrs={.kp=INS_AHRS_KP,.ki=INS_AHRS_KI,.integral_limit_rad_s=INS_AHRS_INTEGRAL_LIMIT_RAD_S,
      .gravity_mps2=INS_GRAVITY_MPS2,.accel_full_fraction=INS_ACCEL_FULL_FRACTION,
      .accel_reject_fraction=INS_ACCEL_REJECT_FRACTION,.mag_min_ut=INS_MAG_MIN_UT,.mag_max_ut=INS_MAG_MAX_UT,
      .mag_relative_tolerance=INS_MAG_RELATIVE_TOLERANCE,.mag_innovation_rad=INS_MAG_INNOVATION_RAD,
      .mag_rate_limit_rad_s=INS_MAG_RATE_LIMIT_RAD_S,.mag_recovery_s=INS_MAG_RECOVERY_S,.max_dt_s=INS_MAX_DT_S},
    .bmi_rotation=INS_BMI_ROTATION,.mag_rotation=INS_MAG_ROTATION,.mag_bias_ut=INS_MAG_BIAS_UT,
    .mag_softiron=INS_MAG_SOFTIRON,.mount_confirmed=INS_MOUNT_CONFIRMED!=0U,
    .mag_calibrated=INS_MAG_ENABLE!=0U && INS_MAG_CALIBRATED!=0U && LOG_INS_MAG_CALIBRATION_ENABLE==0U,
    .still_ms=INS_STILL_MS,.still_gyro_rad_s=INS_STILL_GYRO_RAD_S,.still_accel_fraction=INS_STILL_ACCEL_FRACTION,
    .still_gyro_std_rad_s=INS_STILL_GYRO_STD_RAD_S,.gyro_timeout_ms=INS_GYRO_TIMEOUT_MS,
    .accel_timeout_ms=INS_ACCEL_TIMEOUT_MS,.mag_timeout_ms=INS_MAG_TIMEOUT_MS};
  return c;
}

/* CubeMX 尚未按文档生成时保持无效；不在运行时偷偷改分频、中断边沿或优先级。 */
static bool InsTask_HardwareReady(void) {
  const IRQn_Type irqs[]={EXTI3_IRQn,EXTI4_IRQn,DMA2_Stream0_IRQn,DMA2_Stream3_IRQn,SPI1_IRQn};
  if (hspi1.Init.BaudRatePrescaler!=SPI_BAUDRATEPRESCALER_16 || hspi1.Init.CLKPolarity!=SPI_POLARITY_HIGH ||
      hspi1.Init.CLKPhase!=SPI_PHASE_2EDGE ||
      (EXTI->FTSR & (GPIO_PIN_3|GPIO_PIN_4))!=(GPIO_PIN_3|GPIO_PIN_4) ||
      (EXTI->RTSR & (GPIO_PIN_3|GPIO_PIN_4))!=0U || SystemCoreClock==0U) { return false; }
  for (unsigned i=0U;i<sizeof(irqs)/sizeof(irqs[0]);++i) {
    if (NVIC_GetEnableIRQ(irqs[i])==0U || NVIC_GetPriority(irqs[i])!=5U) { return false; }
  }
  /* 六轴不依赖磁接口；只在明确开启磁采集时检查 I2C 和 IST 的 EXTI。 */
  if (INS_MAG_ENABLE!=0U && (hi2c3.Init.ClockSpeed!=400000U ||
      (EXTI->FTSR & GPIO_PIN_5)==0U || (EXTI->RTSR & GPIO_PIN_5)!=0U ||
      NVIC_GetEnableIRQ(EXTI9_5_IRQn)==0U || NVIC_GetPriority(EXTI9_5_IRQn)!=5U)) { return false; }
  return true;
}

/* 先发布时间和序号，再通知；所有相关 IRQ 同优先级5，不相互抢占。 */
static void InsTask_RecordFromIsr(volatile InsTask_StampTypeDef *stamp) {
  stamp->cycles=DWT->CYCCNT;
  stamp->ms=HAL_GetTick();
  ++stamp->sequence;
  BaseType_t higher_priority_task_woken=pdFALSE;
  (void)xTaskNotifyFromISR(g_ins_task,INS_EVENT_SENSOR,eSetBits,&higher_priority_task_woken);
  portYIELD_FROM_ISR(higher_priority_task_woken);
}

/** @brief DRDY 回调。@param pin EXTI GPIO 位掩码。@return 无；ISR 不访问总线。 */
void HAL_GPIO_EXTI_Callback(uint16_t pin) {
  if (g_ins_task==NULL) { return; }
  if (INS_MAG_ENABLE!=0U && pin==DRDY_IST8310_Pin) { InsTask_RecordFromIsr(&g_mag_stamp); }
  else if (g_accept_imu && pin==INT1_Gyro_Pin) { InsTask_RecordFromIsr(&g_gyro_stamp); }
  else if (g_accept_imu && pin==INT1_Accel_Pin) { InsTask_RecordFromIsr(&g_accel_stamp); }
}

/* DMA 回调不解析数据；只结束当前传输并唤醒所有者。其他 SPI 不由本模块处理。 */
static void InsTask_DmaFromIsr(SPI_HandleTypeDef *spi, bool error) {
  if (spi!=&hspi1 || g_ins_task==NULL || g_runtime.bmi.dma_state!=BMI088_DMA_BUSY) { return; }
  Bmi088_DmaCompleteFromIsr(&g_runtime.bmi,error);
  BaseType_t higher_priority_task_woken=pdFALSE;
  (void)xTaskNotifyFromISR(g_ins_task,INS_EVENT_DMA,eSetBits,&higher_priority_task_woken);
  portYIELD_FROM_ISR(higher_priority_task_woken);
}

/** @brief HAL 全双工 DMA 完成。@param spi SPI 实例。@return 无；ISR 调用。 */
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *spi) { InsTask_DmaFromIsr(spi,false); }
/** @brief HAL SPI/DMA 错误。@param spi SPI 实例。@return 无；ISR 调用。 */
void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *spi) { InsTask_DmaFromIsr(spi,true); }

/* 等待有界 DMA；DRDY 通知可提前唤醒，但不会被误当成传输完成。 */
static bool InsTask_ReadDma(Bmi088_ChannelTypeDef channel) {
  uint32_t events;
  g_runtime.dma_failure=INS_TASK_DMA_FAILURE_NONE;
  (void)xTaskNotifyWait(0U,UINT32_MAX,&events,0U);
  if (!Bmi088_StartRead(&g_runtime.bmi,channel)) {
    g_runtime.dma_failure=INS_TASK_DMA_FAILURE_START;
    g_runtime.dma_spi_error=hspi1.ErrorCode;
    return false;
  }
  TickType_t start=xTaskGetTickCount(), timeout=pdMS_TO_TICKS(INS_DMA_TIMEOUT_MS);
  while (g_runtime.bmi.dma_state==BMI088_DMA_BUSY) {
    TickType_t elapsed=xTaskGetTickCount()-start;
    if (elapsed>=timeout) {
      g_runtime.dma_failure=INS_TASK_DMA_FAILURE_TIMEOUT;
      g_runtime.dma_spi_error=hspi1.ErrorCode;
      Bmi088_Abort(&g_runtime.bmi); return false;
    }
    (void)xTaskNotifyWait(0U,UINT32_MAX,&events,timeout-elapsed);
  }
  if (!Bmi088_FinishRead(&g_runtime.bmi)) {
    g_runtime.dma_failure=INS_TASK_DMA_FAILURE_FINISH;
    g_runtime.dma_spi_error=hspi1.ErrorCode;
    return false;
  }
  return true;
}

/* 中断可持续发生；一次临界区取得一致的最新边沿快照。 */
static InsTask_StampTypeDef InsTask_GetStamp(volatile InsTask_StampTypeDef *source) {
  taskENTER_CRITICAL();
  InsTask_StampTypeDef stamp=*source;
  taskEXIT_CRITICAL();
  return stamp;
}

/* 失败不退出任务；清采集有效性，重试后必须重做连续静止标定。 */
static void InsTask_ImuFault(InsTask_FaultTypeDef reason) {
  /* 先保留失败证据再Abort；HAL清理可能清错误位，重试不能抹去上次原因。 */
  g_runtime.last_fault=reason;
  ++g_runtime.fault_count;
  g_runtime.fault_ms=HAL_GetTick();
  g_runtime.fault_dma_failure=g_runtime.dma_failure;
  g_runtime.fault_spi_error=g_runtime.dma_failure!=INS_TASK_DMA_FAILURE_NONE
      ? g_runtime.dma_spi_error : hspi1.ErrorCode;
  g_accept_imu=false;
  Bmi088_Abort(&g_runtime.bmi);
  g_runtime.input.accel_valid=false;
  g_runtime.input.temperature_valid=false;
  g_runtime.have_cycles=false;
  g_runtime.bmi_retry_tick=xTaskGetTickCount();
  Ins_SetFault();
}

/* BMI 启动可以延时，因为此时姿态已无效；绝不在正常融合中等待磁复位100ms。 */
static void InsTask_TryImuInit(void) {
  Bmi088_ConfigTypeDef c={.spi=&hspi1,.accel_port=CS1_Accel_GPIO_Port,.accel_pin=CS1_Accel_Pin,
    .gyro_port=CS1_Gyro_GPIO_Port,.gyro_pin=CS1_Gyro_Pin,.delay_ms=InsTask_DelayMs,.timeout_ms=INS_SPI_TIMEOUT_MS,
    .accel_range=INS_ACCEL_RANGE_REG,.accel_conf=INS_ACCEL_CONF_REG,.gyro_range=INS_GYRO_RANGE_REG,
    .gyro_bandwidth=INS_GYRO_BANDWIDTH_REG};
  g_accept_imu=false;
  ++g_runtime.init_attempts;
  g_runtime.dma_failure=INS_TASK_DMA_FAILURE_NONE;
  if (!Bmi088_Init(&g_runtime.bmi,&c)) { InsTask_ImuFault(INS_TASK_FAULT_INIT); return; }
  taskENTER_CRITICAL();
  g_runtime.gyro_consumed=g_gyro_stamp.sequence;
  g_runtime.accel_consumed=g_accel_stamp.sequence;
  g_accept_imu=true;
  taskEXIT_CRITICAL();
  g_runtime.last_gyro_ms=HAL_GetTick();
  g_runtime.imu_start_ms=g_runtime.last_gyro_ms;
  g_runtime.have_cycles=false;
  g_runtime.input.accel_valid=false;
}

/* 每次最多一个磁初始化事务或一次数据读取，等待阶段立即返回。 */
static void InsTask_ServiceMag(uint32_t now) {
  if (INS_MAG_ENABLE==0U) { g_runtime.input.mag_valid=false; return; }
  Ist8310_HandleTypeDef *mag=&g_runtime.mag;
  if (!mag->initialized) {
    if (mag->config.i2c==NULL || mag->init_state==IST8310_INIT_ERROR) {
      TickType_t tick=xTaskGetTickCount();
      if (tick-g_runtime.mag_retry_tick<pdMS_TO_TICKS(INS_RETRY_MS)) { return; }
      Ist8310_ConfigTypeDef config={.i2c=&hi2c3,.reset_port=RSTN_IST8310_GPIO_Port,
        .reset_pin=RSTN_IST8310_Pin,.delay_ms=InsTask_DelayMs,.timeout_ms=INS_I2C_TIMEOUT_MS};
      g_runtime.mag_retry_tick=tick;
      (void)Ist8310_BeginInit(mag,&config,now);
    } else { (void)Ist8310_PollInit(mag,now); }
    return;
  }
  InsTask_StampTypeDef stamp=InsTask_GetStamp(&g_mag_stamp);
  if (stamp.sequence==g_runtime.mag_consumed && (uint32_t)(now-g_runtime.last_mag_poll_ms)<INS_MAG_POLL_MS) { return; }
  g_runtime.mag_consumed=stamp.sequence;
  g_runtime.last_mag_poll_ms=now;
  Ist8310_ReadResultTypeDef result=Ist8310_Read(mag);
  if (result==IST8310_READ_ERROR) {
    mag->initialized=false; mag->init_state=IST8310_INIT_ERROR;
    g_runtime.input.mag_valid=false; g_runtime.mag_retry_tick=xTaskGetTickCount();
  } else if (result==IST8310_READ_OK || result==IST8310_READ_OVERRUN) {
    memcpy(g_runtime.input.mag_ut,mag->mag_ut,sizeof(mag->mag_ut));
    memcpy(g_runtime.input.mag_raw,mag->raw,sizeof(mag->raw));
    g_runtime.input.mag_ms=HAL_GetTick(); /* 仅实际新数据成功读取才刷新；兜底轮询也可使用。 */
    g_runtime.input.mag_valid=true;
    if (result==IST8310_READ_OVERRUN) { ++g_runtime.input.mag_overruns; }
  }
}

#if LOG_GLOBAL_ENABLE && LOG_USART1_ENABLE && LOG_INS_ENABLE && !LOG_INS_MAG_CALIBRATION_ENABLE
/* newlib-nano 默认不保证 %f；用带单位整数输出，极端异常数值不会触发转换溢出。 */
static long InsTask_ScaledLog(float value, float scale) {
  float scaled=value*scale;
  return isfinite(scaled) && scaled>-2147483520.0f && scaled<2147483520.0f ? (long)scaled : 0L;
}
#endif

/* 日志忙即丢弃本次输出；从不等待 DMA，也不改变姿态计算。 */
static void InsTask_Log(uint32_t now) {
#if LOG_GLOBAL_ENABLE && LOG_USART1_ENABLE && LOG_INS_MAG_CALIBRATION_ENABLE
  /* 直接输出未标定/未旋转的磁芯片原始值。前六项占位为0，不代表真实惯性数据。
   * PJRC ascii_parse不接受冒号/逗号后的空格；必须紧凑输出，并以CR结束帧，保留CRLF。
   * MotionCal接收int16_t，乘3必须先扩展；越界整帧拒绝，不能钳位或回绕成假磁场。
   * 单次DMA提交整行，忙时丢帧；只有接受后记录样本时间，不重复提交旧样本。 */
  if (g_runtime.input.mag_valid && (uint32_t)(now-g_runtime.input.mag_ms)<=INS_MAG_TIMEOUT_MS &&
      (uint32_t)(now-g_runtime.last_mag_calibration_ms)>=INS_MAG_CALIBRATION_PERIOD_MS &&
      (!g_runtime.have_mag_calibration_sample || g_runtime.mag_calibration_sample_ms!=g_runtime.input.mag_ms)) {
    g_runtime.last_mag_calibration_ms=now;
    long counts[3];
    for (unsigned i=0U;i<3U;++i) {
      counts[i]=(long)g_runtime.input.mag_raw[i]*INS_MOTIONCAL_COUNTS_PER_LSB;
      if (counts[i]<INT16_MIN || counts[i]>INT16_MAX) { return; }
    }
    if (LOG_TRY_PRINTF(LOG_CATEGORY_INS_MAG_CALIBRATION,"Raw:0,0,0,0,0,0,%ld,%ld,%ld\r\n",
        counts[0],counts[1],counts[2])) {
      g_runtime.mag_calibration_sample_ms=g_runtime.input.mag_ms;
      g_runtime.have_mag_calibration_sample=true;
    }
  }
#elif LOG_GLOBAL_ENABLE && LOG_USART1_ENABLE && LOG_INS_ENABLE
  if ((uint32_t)(now-g_runtime.last_log_ms)>=INS_LOG_PERIOD_MS) {
    g_runtime.last_log_ms=now;
    Ins_SnapshotTypeDef s;
    if (Ins_GetSnapshot(&s)) {
      /* 采集诊断与姿态状态共用一次 DMA 提交，避免第二次提交必因首条仍在发送而丢弃。
       * 原码为未映射/未去偏置的芯片LSB，比力为转换后的机体系mm/s²；同快照对照可发现重复取反。 */
      (void)LOG_TRY_PRINTF(LOG_CATEGORY_INS,"姿态,状态=%u,有效=%u,安装=%u,磁融合=%u,航向/俯仰/横滚毫度=%ld/%ld/%ld,重置=%lu,丢帧=%lu"
#if !LOG_INS_MAG_DIAGNOSTIC_ENABLE
        ",时间ms=%lu,序号=%lu,上次采集故障=%u,故障次数=%lu,故障时刻ms=%lu,初始化次数=%lu,DMA失败阶段=%u,SPI错误=%lu"
#endif
        "\r\n"
#if LOG_INS_MAG_DIAGNOSTIC_ENABLE
        "[DEBUG-INS-MAG-1018],时间ms=%lu,序号=%lu,磁有效=%u,磁龄ms=%lu,磁芯片nT=%ld/%ld/%ld,磁机体nT=%ld/%ld/%ld,角速度urad_s=%ld/%ld/%ld,四元数百万分=%ld/%ld/%ld/%ld,温度毫摄氏度=%ld,加计原码=%d/%d/%d,陀螺原码=%d/%d/%d,比力mm_s2=%ld/%ld/%ld\r\n"
#endif
        ,
        (unsigned)s.state,(unsigned)s.attitude_valid,(unsigned)s.mount_confirmed,(unsigned)s.mag_used,
        InsTask_ScaledLog(s.yaw_deg,1000.0f),InsTask_ScaledLog(s.pitch_deg,1000.0f),InsTask_ScaledLog(s.roll_deg,1000.0f),
        (unsigned long)s.reset_sequence,(unsigned long)s.dropped_gyro
#if !LOG_INS_MAG_DIAGNOSTIC_ENABLE
        ,(unsigned long)now,(unsigned long)s.sequence,(unsigned)g_runtime.last_fault,
        (unsigned long)g_runtime.fault_count,(unsigned long)g_runtime.fault_ms,
        (unsigned long)g_runtime.init_attempts,(unsigned)g_runtime.fault_dma_failure,
        (unsigned long)g_runtime.fault_spi_error
#endif
#if LOG_INS_MAG_DIAGNOSTIC_ENABLE
        ,(unsigned long)now,(unsigned long)s.sequence,(unsigned)s.mag_valid,(unsigned long)(now-s.mag_ms),
        InsTask_ScaledLog(s.mag_chip_ut[0],1000.0f),InsTask_ScaledLog(s.mag_chip_ut[1],1000.0f),InsTask_ScaledLog(s.mag_chip_ut[2],1000.0f),
        InsTask_ScaledLog(s.mag_ut[0],1000.0f),InsTask_ScaledLog(s.mag_ut[1],1000.0f),InsTask_ScaledLog(s.mag_ut[2],1000.0f),
        InsTask_ScaledLog(s.gyro_rad_s[0],1000000.0f),InsTask_ScaledLog(s.gyro_rad_s[1],1000000.0f),InsTask_ScaledLog(s.gyro_rad_s[2],1000000.0f),
        InsTask_ScaledLog(s.q[0],1000000.0f),InsTask_ScaledLog(s.q[1],1000000.0f),InsTask_ScaledLog(s.q[2],1000000.0f),InsTask_ScaledLog(s.q[3],1000000.0f),
        InsTask_ScaledLog(s.temperature_c,1000.0f),
        (int)s.accel_raw[0],(int)s.accel_raw[1],(int)s.accel_raw[2],
        (int)s.gyro_raw[0],(int)s.gyro_raw[1],(int)s.gyro_raw[2],
        InsTask_ScaledLog(s.accel_mps2[0],1000.0f),InsTask_ScaledLog(s.accel_mps2[1],1000.0f),InsTask_ScaledLog(s.accel_mps2[2],1000.0f)
#endif
        );
    }
  }
#else
  (void)now;
#endif
}

bool InsTask_RuntimeInit(void) {
  Ins_ConfigTypeDef c=InsTask_MakeConfig();
  if (!Ins_Init(&c)) { return false; }
  if (!InsTask_HardwareReady()) { Ins_SetFault(); return false; }
  memset(&g_runtime,0,sizeof(g_runtime));
  CoreDebug->DEMCR|=CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CTRL|=DWT_CTRL_CYCCNTENA_Msk; /* 保留已有周期计数；不重置其他使用者的时基。 */
  __DSB(); __ISB();
  g_ins_task=xTaskGetCurrentTaskHandle();
  g_runtime.bmi_retry_tick=xTaskGetTickCount()-pdMS_TO_TICKS(INS_RETRY_MS);
  g_runtime.mag_retry_tick=g_runtime.bmi_retry_tick;
  return true;
}

void InsTask_RuntimeRunCycle(void) {
  uint32_t events;
  TickType_t tick=xTaskGetTickCount();
  if (!g_runtime.bmi.initialized && tick-g_runtime.bmi_retry_tick>=pdMS_TO_TICKS(INS_RETRY_MS)) {
    g_runtime.bmi_retry_tick=tick; InsTask_TryImuInit();
  }
  (void)xTaskNotifyWait(0U,UINT32_MAX,&events,pdMS_TO_TICKS(INS_DMA_TIMEOUT_MS));
  uint32_t now=HAL_GetTick();
  if (!g_runtime.bmi.initialized) { InsTask_ServiceMag(now); InsTask_Log(now); return; }
  InsTask_StampTypeDef gyro=InsTask_GetStamp(&g_gyro_stamp);
  if (gyro.sequence!=g_runtime.gyro_consumed) {
    /* DWT 约25.6s回绕；再用同一HAL时基检查跨帧间隔，不能让长暂停被模运算伪装成短dt。 */
    if (g_runtime.have_cycles && (uint32_t)(gyro.ms-g_runtime.last_gyro_ms)>INS_GYRO_TIMEOUT_MS) {
      InsTask_ImuFault(INS_TASK_FAULT_GYRO_GAP); return;
    }
    uint32_t count=gyro.sequence-g_runtime.gyro_consumed;
    g_runtime.input.dropped_gyro+=count-1U;
    g_runtime.gyro_consumed=gyro.sequence;
    if (!InsTask_ReadDma(BMI088_CHANNEL_GYRO)) { InsTask_ImuFault(INS_TASK_FAULT_GYRO_DMA); return; }
    InsTask_StampTypeDef accel=InsTask_GetStamp(&g_accel_stamp);
    if (accel.sequence!=g_runtime.accel_consumed) {
      g_runtime.accel_consumed=accel.sequence;
      if (!InsTask_ReadDma(BMI088_CHANNEL_ACCEL)) { InsTask_ImuFault(INS_TASK_FAULT_ACCEL_DMA); return; }
      memcpy(g_runtime.input.accel_mps2,g_runtime.bmi.sample.accel_mps2,sizeof(g_runtime.input.accel_mps2));
      memcpy(g_runtime.input.accel_raw,g_runtime.bmi.sample.accel_raw,sizeof(g_runtime.input.accel_raw));
      g_runtime.input.accel_ms=accel.ms; g_runtime.input.accel_valid=true;
    }
    now=HAL_GetTick();
    if ((uint32_t)(now-g_runtime.last_temp_attempt_ms)>=INS_TEMPERATURE_PERIOD_MS) {
      g_runtime.last_temp_attempt_ms=now;
      if (!InsTask_ReadDma(BMI088_CHANNEL_TEMP)) { InsTask_ImuFault(INS_TASK_FAULT_TEMP_DMA); return; }
      g_runtime.input.temperature_c=g_runtime.bmi.sample.temperature_c;
      g_runtime.input.temperature_valid=g_runtime.bmi.sample.temperature_valid;
      g_runtime.input.temperature_ms=HAL_GetTick();
    }
    memcpy(g_runtime.input.gyro_rad_s,g_runtime.bmi.sample.gyro_rad_s,sizeof(g_runtime.input.gyro_rad_s));
    memcpy(g_runtime.input.gyro_raw,g_runtime.bmi.sample.gyro_raw,sizeof(g_runtime.input.gyro_raw));
    g_runtime.input.dt_s=g_runtime.have_cycles ? (float)(uint32_t)(gyro.cycles-g_runtime.previous_cycles)/(float)SystemCoreClock : 0.001f;
    g_runtime.previous_cycles=gyro.cycles; g_runtime.have_cycles=true;
    g_runtime.input.gyro_ms=gyro.ms; g_runtime.input.now_ms=HAL_GetTick();
    g_runtime.last_gyro_ms=gyro.ms;
    if (g_runtime.input.dt_s<=0.0f || g_runtime.input.dt_s>INS_MAX_DT_S) {
      InsTask_ImuFault(INS_TASK_FAULT_DT); return;
    }
    if ((uint32_t)(g_runtime.input.now_ms-gyro.ms)>INS_GYRO_TIMEOUT_MS) {
      InsTask_ImuFault(INS_TASK_FAULT_GYRO_AGE); return;
    }
    if (!g_runtime.input.accel_valid && (uint32_t)(g_runtime.input.now_ms-g_runtime.imu_start_ms)>INS_ACCEL_TIMEOUT_MS) {
      InsTask_ImuFault(INS_TASK_FAULT_ACCEL_MISSING); return;
    }
    if (g_runtime.input.accel_valid && (uint32_t)(g_runtime.input.now_ms-g_runtime.input.accel_ms)>INS_ACCEL_TIMEOUT_MS) {
      InsTask_ImuFault(INS_TASK_FAULT_ACCEL_AGE); return;
    }
    Ins_Process(&g_runtime.input);
  } else if ((uint32_t)(now-g_runtime.last_gyro_ms)>INS_GYRO_TIMEOUT_MS) {
    InsTask_ImuFault(INS_TASK_FAULT_GYRO_MISSING); return;
  }
  InsTask_ServiceMag(HAL_GetTick());
  InsTask_Log(HAL_GetTick());
}
