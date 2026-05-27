#include "key.h"
#include "stm32f4xx_hal.h"
#include <stdlib.h>

#define KEY_DEBOUNCE_MS 20
#define KEY_LONG_PRESS_MS 1000
#define KEY_TRIGGER_LEVEL GPIO_PIN_RESET

struct KEY_HandleTypeDef {
  GPIO_TypeDef *port;
  uint16_t pin;
  GPIO_PinState stable_state;
  GPIO_PinState last_sample_state;
  uint32_t debounce_start_time;
  uint32_t press_start_time;
  bool long_press_reported;
};

static bool Key_IsDebounceFinished(uint32_t now, uint32_t start_time) {
  return (uint32_t)(now - start_time) >= KEY_DEBOUNCE_MS;
}

KEY_HandleTypeDef *Key_Create(GPIO_TypeDef *key_port, uint16_t key_pin) {
  if (key_port == NULL) {
    return NULL;
  }

  assert_param(IS_GPIO_ALL_INSTANCE(key_port));
  assert_param(IS_GPIO_PIN(key_pin));

  KEY_HandleTypeDef *hkey =
      (KEY_HandleTypeDef *)malloc(sizeof(KEY_HandleTypeDef));
  if (hkey == NULL) {
    return NULL;
  }

  hkey->port = key_port;
  hkey->pin = key_pin;
  hkey->stable_state = HAL_GPIO_ReadPin(key_port, key_pin);
  hkey->last_sample_state = hkey->stable_state;
  hkey->debounce_start_time = HAL_GetTick();
  hkey->press_start_time =
      hkey->stable_state == KEY_TRIGGER_LEVEL ? hkey->debounce_start_time : 0;
  hkey->long_press_reported = hkey->stable_state == KEY_TRIGGER_LEVEL;

  return hkey;
}

KEY_StatusTypeDef Key_Destroy(KEY_HandleTypeDef *hkey) {
  if (hkey == NULL) {
    return KEY_NO_PARAMETER;
  }

  HAL_GPIO_DeInit(hkey->port, hkey->pin);
  free(hkey);
  return KEY_OK;
}

KEY_EventTypeDef Key_GetEvent(KEY_HandleTypeDef *hkey) {
  if (hkey == NULL) {
    return KEY_EVENT_NONE;
  }

  uint32_t now = HAL_GetTick();
  GPIO_PinState sample_state = HAL_GPIO_ReadPin(hkey->port, hkey->pin);

  if (sample_state != hkey->last_sample_state) {
    hkey->last_sample_state = sample_state;
    hkey->debounce_start_time = now;
    return KEY_EVENT_NONE;
  }

  if (!Key_IsDebounceFinished(now, hkey->debounce_start_time)) {
    return KEY_EVENT_NONE;
  }

  if (sample_state == hkey->stable_state) {
    if (hkey->stable_state == KEY_TRIGGER_LEVEL && hkey->press_start_time != 0 &&
        !hkey->long_press_reported &&
        (uint32_t)(now - hkey->press_start_time) >= KEY_LONG_PRESS_MS) {
      hkey->long_press_reported = true;
      return KEY_EVENT_LONG_PRESS;
    }

    return KEY_EVENT_NONE;
  }

  hkey->stable_state = sample_state;
  if (hkey->stable_state == KEY_TRIGGER_LEVEL) {
    hkey->press_start_time = now;
    hkey->long_press_reported = false;
  } else {
    bool is_short_press = hkey->press_start_time != 0 && !hkey->long_press_reported;
    hkey->press_start_time = 0;
    hkey->long_press_reported = false;

    if (is_short_press) {
      return KEY_EVENT_SHORT_PRESS;
    }
  }

  return KEY_EVENT_NONE;
}

bool Key_IsTriggered(KEY_HandleTypeDef *hkey) {
  return Key_GetEvent(hkey) != KEY_EVENT_NONE;
}
