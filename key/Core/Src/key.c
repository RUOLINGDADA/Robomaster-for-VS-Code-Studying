#include "key.h"
#include "SEGGER_RTT.h"
#include "stm32f4xx_hal.h"

#define true 1
#define false 0

#define SEGGER_DEBUG

// 安全计时(针对溢出情况)
#define is_time_A_before_B(a, b)                                               \
  ((int32_t)(a) - (int32_t)(b) < 0)                      // 当前时间a在时间b之前
#define is_time_A_after_B(a, b) is_time_A_before_B(b, a) // 当前时间a在时间b之后
#define is_time_A_before_B_eq(a, b)                                            \
  ((int32_t)(a) - (int32_t)(b) <= 0) // 当前时间a在或等于时间b之前
#define is_time_A_after_B_eq(a, b)                                             \
  is_time_A_before_B(b, a)     // 当前时间a或等于时间b之后
#define KEY_DEBOUNCE_MS 10     // 按键消抖时间
#define KEY_LONG_PRESS_MS 3000 // 长按触发时间

// 变量
// static 修饰 未初始化/0 位于.bss段 初始化值 位于.data段
// static volatile uint32_t HAL_GetTick() = 0; // 用于计时
// 用hal_gettick()替代

// 函数表
typedef KEY_StatusTypeDef (*Key_Destroy_Func)(KEY_HandleTypeDef *hkey);
typedef KEY_StatusTypeDef (*Key_Device_Process_Func)(KEY_HandleTypeDef *hkey);
typedef KEY_StatusTypeDef (*LED_Update_Loop_Func)(KEY_HandleTypeDef *hkey);

// 底层函数声明
static KEY_StatusTypeDef Key_Destroy_F407(KEY_HandleTypeDef *hkey);
static KEY_StatusTypeDef Key_Device_Process_Loop_F407(KEY_HandleTypeDef *hkey);
static KEY_StatusTypeDef LED_Update_Loop_F407(KEY_HandleTypeDef *hkey);

/*
const修饰位于.rodata段(flash)
[LED亮ms, LED灭ms]
*/
static const uint16_t device_mode[3][2] = {
    {200, 200}, // 模式A
    {400, 400}, // 模式B
    {600, 600}  // 模式C
};
#define MODE_COUNT (sizeof(device_mode) / sizeof(device_mode[0]))

struct KEY_HandleTypeDef {
  KEY_PlatformTypeDef platform;
  GPIO_TypeDef *key_pin_port;
  uint16_t key_pin;
  Key_Destroy_Func deinit;
  Key_Device_Process_Func process_func;
  struct {
    LED_Update_Loop_Func led_update_loop_func;
    bool power_state;
    uint8_t current_mode;
    GPIO_TypeDef *led_pin_port;
    uint16_t led_pin;
    struct {                     // 按键状态机
      GPIO_PinState last_state;  // 上一次稳定状态
      uint32_t press_start_time; // 按键按下起始时间
      bool long_press_triggered; // 长按已触发标志
      uint32_t debounce_time;    // 消抖计时起点
    } key;
  } device;
};

KEY_HandleTypeDef *Key_Create(GPIO_TypeDef *key_port, uint16_t key_pin,
                              KEY_PlatformTypeDef platform,
                              GPIO_TypeDef *led_pin_port, uint16_t led_pin) {
  if (key_port == NULL) {
    return NULL;
  }

  assert_param(IS_GPIO_ALL_INSTANCE(key_port));
  assert_param(IS_GPIO_PIN(key_pin));

  KEY_HandleTypeDef *hkey =
      (KEY_HandleTypeDef *)malloc(sizeof(KEY_HandleTypeDef));
  // 检查内存分配情况
  if (hkey == NULL) {
    return NULL;
  }

  hkey->key_pin_port = key_port;
  hkey->key_pin = key_pin;
  hkey->platform = platform;
  hkey->process_func = Key_Device_Process_Loop_F407;
  hkey->device.power_state = false;
  hkey->device.current_mode = 0;
  hkey->device.led_pin_port = led_pin_port;
  hkey->device.led_pin = led_pin;
  hkey->device.led_update_loop_func = LED_Update_Loop_F407;
  hkey->device.key.last_state = GPIO_PIN_SET;
  hkey->device.key.debounce_time = 0;
  hkey->device.key.press_start_time = 0;
  hkey->device.key.long_press_triggered = false;

  switch (platform) {
  case STM32F407:
    hkey->deinit = Key_Destroy_F407;
    break;
  default:
    hkey->deinit = Key_Destroy_F407;
    break;
  }
#ifdef SEGGER_DEBUG
  SEGGER_RTT_printf(0, "create succ");
#endif
  return hkey; // 返回对象地址
}

// 析构函数
KEY_StatusTypeDef Key_Destroy(KEY_HandleTypeDef *hkey) {
  if (hkey == NULL) {
    return KEY_NO_PARAMETER;
  }
  if (hkey->deinit(hkey) != KEY_OK) {
    return KEY_ERROR;
  }
  return KEY_OK;
}

/* 对外接口*/
KEY_StatusTypeDef Key_Device_Process_Loop(KEY_HandleTypeDef *hkey) {
  if (hkey == NULL) {
    return KEY_NO_PARAMETER;
  }
  if (hkey->process_func(hkey) != KEY_DEVICE_NORMAL_LOOP) {
    return KEY_ERROR;
  }
  return KEY_OK;
}

KEY_StatusTypeDef LED_Update_Loop(KEY_HandleTypeDef *hkey) {
  if (hkey == NULL) {
    return KEY_NO_PARAMETER;
  }
  if (hkey->device.led_update_loop_func(hkey) != KEY_DEVICE_NORMAL_LOOP) {
    return KEY_ERROR;
  }
  return KEY_OK;
}

uint8_t Get_Device_PowerState(KEY_HandleTypeDef *hkey) {
  if (hkey == NULL) {
    return 0;
  }
  return hkey->device.power_state;
}

// 底层函数实现

static KEY_StatusTypeDef Key_Destroy_F407(KEY_HandleTypeDef *hkey) {
  if (hkey == NULL) {
    return KEY_NO_PARAMETER;
  }
  HAL_GPIO_DeInit(hkey->key_pin_port, hkey->key_pin);
  HAL_GPIO_DeInit(hkey->device.led_pin_port, hkey->device.led_pin);
  free(hkey);
  return KEY_OK;
}

static KEY_StatusTypeDef Key_Device_Process_Loop_F407(KEY_HandleTypeDef *hkey) {
  GPIO_PinState current_state =
      HAL_GPIO_ReadPin(hkey->key_pin_port, hkey->key_pin);
  // 1.状态发生了变化
  if (current_state != hkey->device.key.last_state) {
    hkey->device.key.last_state = current_state;
    hkey->device.key.debounce_time = HAL_GetTick();
    return KEY_DEVICE_NORMAL_LOOP; // 立即返回 等待消抖
  }

  // 2.消抖
  if (!is_time_A_after_B_eq(HAL_GetTick(),
                            hkey->device.key.debounce_time + KEY_DEBOUNCE_MS)) {
    return KEY_DEVICE_NORMAL_LOOP; // 消抖未完成，直接返回
  }

  GPIO_PinState stable_state = current_state;
  // 状态1：按键按下（稳定状态从SET变为RESET）
  if (hkey->device.key.press_start_time == 0 &&
      stable_state == GPIO_PIN_RESET) {
    hkey->device.key.press_start_time = HAL_GetTick();
    hkey->device.key.long_press_triggered = false;
  }
  // 状态2: 按键持续不放 触发长按 避免长按中持续触发短按
  else if (hkey->device.key.long_press_triggered == false &&
           stable_state == GPIO_PIN_RESET &&
           hkey->device.key.press_start_time != 0) {
    if (is_time_A_after_B_eq(HAL_GetTick(), hkey->device.key.press_start_time +
                                                KEY_LONG_PRESS_MS)) {
      hkey->device.key.long_press_triggered = true;         // 长按标志位置反
      hkey->device.power_state = !hkey->device.power_state; // 电源标志位置反
#ifdef SEGGER_DEBUG
      SEGGER_RTT_printf(0, "POWER:%d\r\n", hkey->device.power_state);
#endif
      if (!hkey->device.power_state) {
        // 执行关闭操作
        HAL_GPIO_WritePin(hkey->device.led_pin_port, hkey->device.led_pin,
                          RESET);
      }
    }
  }
  // 状态3: 按键松开 只要没触发长按 认定为短按
  else if (stable_state == GPIO_PIN_SET &&
           hkey->device.key.press_start_time != 0) {
    if (!hkey->device.key.long_press_triggered &&
        hkey->device.power_state == true) {
      hkey->device.current_mode = (hkey->device.current_mode + 1) % MODE_COUNT;
    }
    // 重置按键状态
    hkey->device.key.press_start_time = 0;
    hkey->device.key.long_press_triggered = false;
  }
  return KEY_DEVICE_NORMAL_LOOP;
}

KEY_StatusTypeDef LED_Update_Loop_F407(KEY_HandleTypeDef *hkey) {
  if (hkey->device.power_state == false) {
    HAL_GPIO_WritePin(hkey->device.led_pin_port, hkey->device.led_pin, RESET);
    return KEY_DEVICE_NORMAL_LOOP;
  }

  uint16_t bright_time = device_mode[hkey->device.current_mode][0];
  uint16_t extinguish_time = device_mode[hkey->device.current_mode][1];
  uint16_t phase =
      HAL_GetTick() %
      (bright_time +
       extinguish_time); // 总周期维持在bright_time - extinguish_time内
  HAL_GPIO_WritePin(hkey->device.led_pin_port, hkey->device.led_pin,
                    phase < bright_time ? SET : RESET);
#ifdef SEGGER_DEBUG
  if (phase < bright_time) {
    SEGGER_RTT_printf(0, "Phase:%d SET\r\n", phase);
  } else {
    SEGGER_RTT_printf(0, "Phase:%d RESET\r\n", phase);
  }
#endif
  return KEY_DEVICE_NORMAL_LOOP;
}