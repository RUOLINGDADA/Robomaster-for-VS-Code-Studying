#include "app_c620_demo.h"
#include "SEGGER_RTT.h"
#include "bsp/c620.h"
#include "bsp/device_base.h"

#define APP_C620_DEMO_ESC_ID 1U
#define APP_C620_DEMO_PERIOD_MS 2U
#define APP_C620_DEMO_PHASE_MS 1500U
#define APP_C620_DEMO_OPEN_LOOP 1
#define APP_C620_DEMO_FEEDBACK_LOG_MS 200U
#define APP_C620_DEMO_RAW_LOG_ENABLE 0

typedef struct {
  const char *label;
  int16_t current;
} App_C620DemoPhaseTypeDef;

static C620_HandleTypeDef *g_esc;
static CAN_HandleTypeDef *g_can;
static uint32_t g_last_process_tick;
static uint32_t g_phase_start_tick;
static uint32_t g_last_feedback_log_tick;
#if APP_C620_DEMO_RAW_LOG_ENABLE
static uint32_t g_last_raw_rx_log_tick;
#endif
static uint8_t g_phase_index;
static bool g_feedback_received;

static const App_C620DemoPhaseTypeDef g_phase_table[] = {
    {"forward raw +1000", 1000},  {"forward raw +2000", 2000},
    {"forward raw +4000", 4000},  {"stop raw 0", 0},
    {"reverse raw -1000", -1000}, {"reverse raw -2000", -2000},
    {"reverse raw -4000", -4000}, {"stop raw 0", 0},
};

static bool App_TimeReached(uint32_t now, uint32_t deadline) {
  return (int32_t)(now - deadline) >= 0;
}

static void App_C620Demo_SendCurrent(uint8_t esc_id, int16_t current) {
  CAN_TxHeaderTypeDef tx_header = {0};
  uint8_t tx_data[8] = {0};
  uint32_t tx_mailbox;
  uint8_t group_index;

  if (g_can == NULL || esc_id < 1U || esc_id > C620_MAX_ESC_COUNT ||
      HAL_CAN_GetTxMailboxesFreeLevel(g_can) == 0U) {
    return;
  }

  if (esc_id <= 4U) {
    tx_header.StdId = C620_CAN_CTRL_ID_BASE;
    group_index = (uint8_t)(esc_id - 1U);
  } else {
    tx_header.StdId = C620_CAN_CTRL_ID_EXT;
    group_index = (uint8_t)(esc_id - 5U);
  }

  tx_header.IDE = CAN_ID_STD;
  tx_header.RTR = CAN_RTR_DATA;
  tx_header.DLC = 8U;

  tx_data[2U * group_index] = (uint8_t)(current >> 8);
  tx_data[2U * group_index + 1U] = (uint8_t)current;

  (void)HAL_CAN_AddTxMessage(g_can, &tx_header, tx_data, &tx_mailbox);
}

static void App_C620Demo_OnDataUpdate(C620_HandleTypeDef *hc620) {
  const uint32_t now = HAL_GetTick();

  g_feedback_received = true;
  if (!App_TimeReached(now, g_last_feedback_log_tick +
                                APP_C620_DEMO_FEEDBACK_LOG_MS)) {
    return;
  }
  g_last_feedback_log_tick = now;

  SEGGER_RTT_printf(
      0,
      "[ESC%d] Angle:%5u Last:%5u Speed:%5d rpm Current:%5d Temp:%3u C "
      "Err:%u Ext:0x%02X State:%d\r\n",
      hc620->esc_id, hc620->mechanical_angle, hc620->last_mechanical_angle,
      hc620->actual_speed, hc620->actual_current, hc620->actual_temp,
      hc620->error.official, hc620->error.ext, hc620->state);
}

static void App_C620Demo_OnStateChange(C620_HandleTypeDef *hc620,
                                       C620_StateTypeDef old_state,
                                       C620_StateTypeDef new_state) {
  SEGGER_RTT_printf(0, "[ESC%d] State: %d -> %d\r\n", hc620->esc_id, old_state,
                    new_state);
}

static void App_C620Demo_OnOfficialError(C620_HandleTypeDef *hc620,
                                         C620_OfficialErrorTypeDef error) {
  SEGGER_RTT_printf(0, "[ESC%d] Official Error: 0x%02X\r\n", hc620->esc_id,
                    error);
}

static void App_C620Demo_OnExtError(C620_HandleTypeDef *hc620,
                                    C620_ExtErrorTypeDef error) {
  SEGGER_RTT_printf(0, "[ESC%d] Ext Error: 0x%02X\r\n", hc620->esc_id, error);
}

static void App_C620Demo_OnWarning(C620_HandleTypeDef *hc620,
                                   C620_WarningTypeDef warning) {
  SEGGER_RTT_printf(0, "[ESC%d] Warning: 0x%02X (temp=%u C)\r\n", hc620->esc_id,
                    warning, hc620->actual_temp);
}

static void App_C620Demo_OnStall(C620_HandleTypeDef *hc620) {
  SEGGER_RTT_printf(0, "[ESC%d] STALL detected!\r\n", hc620->esc_id);
}

static void App_C620Demo_OnCommRecover(C620_HandleTypeDef *hc620) {
  SEGGER_RTT_printf(0, "[ESC%d] Communication recovered\r\n", hc620->esc_id);
}

static bool App_C620Demo_RegisterCallbacks(C620_HandleTypeDef *hc620) {
  C620_CallbackTypeDef callbacks = {0};

  callbacks.on_data_update = App_C620Demo_OnDataUpdate;
  callbacks.on_state_change = App_C620Demo_OnStateChange;
  callbacks.on_official_error = App_C620Demo_OnOfficialError;
  callbacks.on_ext_error = App_C620Demo_OnExtError;
  callbacks.on_warning = App_C620Demo_OnWarning;
  callbacks.on_stall = App_C620Demo_OnStall;
  callbacks.on_comm_recover = App_C620Demo_OnCommRecover;

  return C620_RegisterCallback(hc620, &callbacks) == DEVICE_OK;
}

static void App_C620Demo_ApplyPhase(uint8_t phase_index) {
  const App_C620DemoPhaseTypeDef *phase = &g_phase_table[phase_index];

  App_C620Demo_SendCurrent(APP_C620_DEMO_ESC_ID, phase->current);

  SEGGER_RTT_printf(0, "\r\n[TEST] ESC%u Phase %u: %s (raw=%d)\r\n",
                    APP_C620_DEMO_ESC_ID, phase_index, phase->label,
                    phase->current);
}

bool App_C620Demo_Init(CAN_HandleTypeDef *hcan) {
  if (hcan == NULL) {
    return false;
  }

  g_can = hcan;
  g_esc = C620_Create("ESC", APP_C620_DEMO_ESC_ID, hcan);
  if (g_esc == NULL) {
    return false;
  }

  if (!App_C620Demo_RegisterCallbacks(g_esc)) {
    return false;
  }

#if APP_C620_DEMO_OPEN_LOOP
  (void)C620_SetFeedbackTimeoutEnable(g_esc, false);
#endif
  (void)C620_ExecuteCommand(g_esc, C620_CMD_ENABLE_OUTPUT, NULL);

  g_last_process_tick = HAL_GetTick();
  g_phase_start_tick = HAL_GetTick();
  g_last_feedback_log_tick = HAL_GetTick();
#if APP_C620_DEMO_RAW_LOG_ENABLE
  g_last_raw_rx_log_tick = HAL_GetTick();
#endif
  g_feedback_received = false;
  g_phase_index = 0U;
  App_C620Demo_ApplyPhase(g_phase_index);

  SEGGER_RTT_printf(0, "\r\n===== C620 ESC Current Test =====\r\n");
  SEGGER_RTT_printf(0, "ESC ID=%u, direct CAN current frames, 2ms period.\r\n",
                    APP_C620_DEMO_ESC_ID);
  SEGGER_RTT_printf(0, "C620 green LED blink count should match this ESC ID.\r\n");
  SEGGER_RTT_printf(0, "Test sequence: +1000/+2000/+4000/0/-1000/-2000/"
                       "-4000/0\r\n");
#if APP_C620_DEMO_OPEN_LOOP
  SEGGER_RTT_printf(0, "Feedback timeout protection is disabled for open-loop "
                       "bench testing.\r\n\r\n");
#endif

  return true;
}

void App_C620Demo_Process(void) {
  const uint32_t now = HAL_GetTick();

  if (App_TimeReached(now, g_last_process_tick + APP_C620_DEMO_PERIOD_MS)) {
    g_last_process_tick = now;

    App_C620Demo_SendCurrent(APP_C620_DEMO_ESC_ID,
                             g_phase_table[g_phase_index].current);
    (void)C620_Process_Loop(g_esc);

    if (!g_feedback_received &&
        App_TimeReached(now, g_last_feedback_log_tick +
                                 APP_C620_DEMO_FEEDBACK_LOG_MS)) {
      g_last_feedback_log_tick = now;
      SEGGER_RTT_printf(0, "[DIAG] No C620 feedback received yet. Check CAN "
                           "wiring, CAN port, ESC power, and ESC ID.\r\n");
    }
  }

  if (App_TimeReached(now, g_phase_start_tick + APP_C620_DEMO_PHASE_MS)) {
    g_phase_start_tick = now;
    g_phase_index++;
    if (g_phase_index >= (sizeof(g_phase_table) / sizeof(g_phase_table[0]))) {
      g_phase_index = 0U;
    }

    App_C620Demo_ApplyPhase(g_phase_index);
  }
}

void App_C620Demo_OnCanRxMessage(CAN_HandleTypeDef *hcan,
                                 const CAN_RxHeaderTypeDef *rx_header,
                                 const uint8_t data[8]) {
  if (hcan == NULL || rx_header == NULL || data == NULL) {
    return;
  }

#if APP_C620_DEMO_RAW_LOG_ENABLE
  if (rx_header->IDE == CAN_ID_STD && rx_header->RTR == CAN_RTR_DATA &&
      rx_header->DLC == 8U &&
      rx_header->StdId > C620_CAN_FEEDBACK_ID_BASE &&
      rx_header->StdId <= (C620_CAN_FEEDBACK_ID_BASE + C620_MAX_ESC_COUNT)) {
    const uint32_t now = HAL_GetTick();
    if (App_TimeReached(now, g_last_raw_rx_log_tick +
                                 APP_C620_DEMO_FEEDBACK_LOG_MS)) {
      g_last_raw_rx_log_tick = now;
      SEGGER_RTT_printf(0,
                        "[RAW] StdId=0x%03lX -> protocol_id=%lu angle=%u "
                        "speed=%d current=%d temp=%u err=%u\r\n",
                        rx_header->StdId,
                        rx_header->StdId - C620_CAN_FEEDBACK_ID_BASE,
                        (uint16_t)(data[0] << 8 | data[1]),
                        (int16_t)(data[2] << 8 | data[3]),
                        (int16_t)(data[4] << 8 | data[5]), data[6], data[7]);
    }
  }
#endif

  (void)C620_HandleCanRxMessage(hcan, rx_header, data);
}
