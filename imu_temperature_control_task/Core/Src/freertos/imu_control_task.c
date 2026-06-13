#include "bmi088.h"
#include "cmsis_os.h"
#include "imu.h"
#include "main.h"
#include "pid.h"
#include "portmacro.h"
#include "projdefs.h"
#include "spi.h"
#include "tim.h"
#include <stdint.h>

#define IMU_KP 1600.0
#define IMU_KI 0.2
#define IMU_KD 0.0
#define IMU_SET_TEMP 40.0

BMI088_HandleTypeDef bmi088 = {.hspi = &hspi1,
                               .ACC_CS_GPIO = GPIOA,
                               .ACC_CS_PIN = CS1_Accel_Pin,
                               .GYRO_CS_GPIO = GPIOB,
                               .GYRO_CS_PIN = CS1_Gyro_Pin};

BMI088_RawDataTypeDef raw;
BMI088_PhysDataTypeDef phys;

volatile uint32_t bmi088_read_count = 0;
volatile uint32_t bmi088_read_error_count = 0;
static uint32_t temp_div = 0;
static TaskHandle_t IMUControlTaskToNotify = NULL;

void StartIMUControlTask(void *argument) {
  // 先保存任务句柄，避免初始化过程中 DRDY 中断提前到来时通知 NULL 句柄
  IMUControlTaskToNotify = xTaskGetCurrentTaskHandle();

  // 初始化BMI088
  BMI088_Init(&bmi088, BMI088_ACC_RANGE_24G, BMI088_ACC_ODR_1600HZ,
              BMI088_GYRO_RANGE_2000DPS, BMI088_GYRO_BW_532HZ_2000HZ);
  // 开启加热器定时器
  HAL_TIM_Base_Start(&htim10);
  HAL_TIM_PWM_Start(&htim10, TIM_CHANNEL_1);
  // 初始化pid
  PID_HandleTypeDef *hpid1 = PID_Create();
  if (hpid1 != NULL) {
    PID_Init(hpid1, IMU_KP, IMU_KI, IMU_KD);
  }
  float output = 0.0f;
  for (;;) {
    // 中断唤醒任务
    if (ulTaskNotifyTake(pdTRUE, portMAX_DELAY)) {
      if (BMI088_ReadRaw6Axis(&bmi088, &raw) == BMI088_OK) {
        if (++temp_div >= 40) // 400Hz 下约 10Hz 读温度
        {
          temp_div = 0;
          BMI088_ReadTemperature(&bmi088, &raw);
        }
        BMI088_ConvertRawToPhys(&bmi088, &raw, &phys);
        output = PID_Calc(hpid1, PID_MODE_DELTA, IMU_SET_TEMP, phys.Temp);
        Set_IMU_PWM((uint32_t)output);
        bmi088_read_count++;
      } else {
        bmi088_read_error_count++;
      }
    }
  }
}

/**
GYRO DRDY INT3 作为唯一采样中断
    ↓
EXTI 回调只置位 flag，不读 SPI
    ↓
main while 中看到 flag
    ↓
一次性 BMI088_ReadRaw() 读取 ACC + GYRO
    ↓
BMI088_ConvertRawToPhys()
    ↓
温度每 100 次或 200 次再读一次
*/
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
  if (GPIO_Pin == INT1_Gyro_Pin) {
    if (IMUControlTaskToNotify != NULL &&
        xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
      // FreeRTOS中断通知任务标准操作
      BaseType_t xHigherPriorityTaskWoken = pdFALSE;
      vTaskNotifyGiveFromISR(IMUControlTaskToNotify, &xHigherPriorityTaskWoken);
      portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
  }
}
