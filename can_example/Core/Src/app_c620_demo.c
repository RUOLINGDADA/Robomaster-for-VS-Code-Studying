#include "app_c620_demo.h"
#include "SEGGER_RTT.h"
#include "bsp/c620.h"
#include "bsp/device_base.h"

#define APP_C620_ESC_ID 1U
#define APP_C620_TX_PERIOD_MS 2U
#define APP_C620_PHASE_PERIOD_MS 1500U
#define APP_C620_FEEDBACK_TIMEOUT_MS 500U
#define APP_C620_FEEDBACK_LOG_MS 250U
#define APP_C620_OPEN_LOOP_TEST 1

typedef struct {
  const char *name;
  int16_t current;
} App_C620PhaseTypeDef;

typedef struct {
  CAN_HandleTypeDef *can;
  C620_HandleTypeDef *esc;
  uint32_t last_tx_tick;
  uint32_t last_phase_tick;
  uint32_t last_feedback_tick;
  uint32_t last_log_tick;
  uint8_t phase;
} App_C620ContextTypeDef;

static App_C620ContextTypeDef g_app;

static const App_C620PhaseTypeDef g_phases[] = {
    {"forward +1000", 1000},  {"forward +2000", 2000},
    {"forward +4000", 4000},  {"stop", 0},
    {"reverse -1000", -1000}, {"reverse -2000", -2000},
    {"reverse -4000", -4000}, {"stop", 0},
};

static bool App_TimeReached(uint32_t now, uint32_t deadline) {
  return (int32_t)(now - deadline) >= 0;
}

static uint32_t App_PhaseCount(void) {
  return (uint32_t)(sizeof(g_phases) / sizeof(g_phases[0]));
}

static bool App_C620_SendCurrent(uint8_t esc_id, int16_t current) {
  CAN_TxHeaderTypeDef header = {0};
  uint8_t data[8] = {0};
  uint32_t mailbox;
  uint8_t slot;

  if (g_app.can == NULL || esc_id < 1U || esc_id > C620_MAX_ESC_COUNT) {
    return false;
  }
  if (HAL_CAN_GetTxMailboxesFreeLevel(g_app.can) == 0U) {
    return false;
  }

  if (esc_id <= 4U) {
    header.StdId = C620_CAN_CTRL_ID_BASE;
    slot = (uint8_t)(esc_id - 1U);
  } else {
    header.StdId = C620_CAN_CTRL_ID_EXT;
    slot = (uint8_t)(esc_id - 5U);
  }

  header.IDE = CAN_ID_STD;
  header.RTR = CAN_RTR_DATA;
  header.DLC = 8U;
  data[2U * slot] = (uint8_t)(current >> 8);
  data[2U * slot + 1U] = (uint8_t)current;

  return HAL_CAN_AddTxMessage(g_app.can, &header, data, &mailbox) == HAL_OK;
}

static void App_C620_LogPhase(void) {
  const App_C620PhaseTypeDef *phase = &g_phases[g_app.phase];

  SEGGER_RTT_printf(0, "\r\n[TEST] ESC%u %s raw=%d\r\n", APP_C620_ESC_ID,
                    phase->name, phase->current);
}

static void App_C620_OnDataUpdate(C620_HandleTypeDef *esc) {
  const uint32_t now = HAL_GetTick();

  g_app.last_feedback_tick = now;
  if (!App_TimeReached(now, g_app.last_log_tick + APP_C620_FEEDBACK_LOG_MS)) {
    return;
  }
  g_app.last_log_tick = now;

  SEGGER_RTT_printf(
      0,
      "[ESC%u] angle=%u speed=%d current=%d temp=%u err=%u ext=0x%02X\r\n",
      esc->esc_id, esc->mechanical_angle, esc->actual_speed,
      esc->actual_current, esc->actual_temp, esc->error.official,
      esc->error.ext);
}

static void App_C620_OnStateChange(C620_HandleTypeDef *esc,
                                   C620_StateTypeDef old_state,
                                   C620_StateTypeDef new_state) {
  SEGGER_RTT_printf(0, "[ESC%u] state %d -> %d\r\n", esc->esc_id, old_state,
                    new_state);
}

static void App_C620_OnOfficialError(C620_HandleTypeDef *esc,
                                     C620_OfficialErrorTypeDef error) {
  SEGGER_RTT_printf(0, "[ESC%u] official error=%u\r\n", esc->esc_id, error);
}

static void App_C620_OnExtError(C620_HandleTypeDef *esc,
                                C620_ExtErrorTypeDef error) {
  SEGGER_RTT_printf(0, "[ESC%u] ext error=0x%02X\r\n", esc->esc_id, error);
}

static bool App_C620_RegisterCallbacks(C620_HandleTypeDef *esc) {
  C620_CallbackTypeDef callbacks = {0};

  callbacks.on_data_update = App_C620_OnDataUpdate;
  callbacks.on_state_change = App_C620_OnStateChange;
  callbacks.on_official_error = App_C620_OnOfficialError;
  callbacks.on_ext_error = App_C620_OnExtError;

  return C620_RegisterCallback(esc, &callbacks) == DEVICE_OK;
}

bool App_C620Demo_Init(CAN_HandleTypeDef *hcan) {
  if (hcan == NULL) {
    return false;
  }

  g_app.can = hcan;
  g_app.esc = C620_Create("C620", APP_C620_ESC_ID, hcan);
  if (g_app.esc == NULL || !App_C620_RegisterCallbacks(g_app.esc)) {
    return false;
  }

#if APP_C620_OPEN_LOOP_TEST
  (void)C620_SetFeedbackTimeoutEnable(g_app.esc, false);
#endif
  (void)C620_ExecuteCommand(g_app.esc, C620_CMD_ENABLE_OUTPUT, NULL);

  const uint32_t now = HAL_GetTick();
  g_app.last_tx_tick = now;
  g_app.last_phase_tick = now;
  g_app.last_feedback_tick = now;
  g_app.last_log_tick = now;
  g_app.phase = 0U;

  (void)App_C620_SendCurrent(APP_C620_ESC_ID, g_phases[g_app.phase].current);
  App_C620_LogPhase();

  SEGGER_RTT_printf(0, "C620 test: ESC ID=%u, tx=%ums, phase=%ums\r\n",
                    APP_C620_ESC_ID, APP_C620_TX_PERIOD_MS,
                    APP_C620_PHASE_PERIOD_MS);
  return true;
}

void App_C620Demo_Process(void) {
  const uint32_t now = HAL_GetTick();

  if (g_app.esc == NULL) {
    return;
  }

  if (App_TimeReached(now, g_app.last_tx_tick + APP_C620_TX_PERIOD_MS)) {
    g_app.last_tx_tick = now;
    (void)App_C620_SendCurrent(APP_C620_ESC_ID, g_phases[g_app.phase].current);
    (void)C620_Process_Loop(g_app.esc);
  }

  if (App_TimeReached(now,
                      g_app.last_feedback_tick + APP_C620_FEEDBACK_TIMEOUT_MS)) {
    g_app.last_feedback_tick = now;
    SEGGER_RTT_printf(0, "[ESC%u] no feedback for %ums\r\n", APP_C620_ESC_ID,
                      APP_C620_FEEDBACK_TIMEOUT_MS);
  }

  if (App_TimeReached(now, g_app.last_phase_tick + APP_C620_PHASE_PERIOD_MS)) {
    g_app.last_phase_tick = now;
    g_app.phase++;
    if (g_app.phase >= App_PhaseCount()) {
      g_app.phase = 0U;
    }
    App_C620_LogPhase();
  }
}

void App_C620Demo_OnCanRxMessage(CAN_HandleTypeDef *hcan,
                                 const CAN_RxHeaderTypeDef *rx_header,
                                 const uint8_t data[8]) {
  if (hcan == NULL || rx_header == NULL || data == NULL) {
    return;
  }

  (void)C620_HandleCanRxMessage(hcan, rx_header, data);
}
