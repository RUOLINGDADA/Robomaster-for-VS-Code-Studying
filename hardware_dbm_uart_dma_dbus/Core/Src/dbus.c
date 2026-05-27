#include "dbus.h"
#include "usart.h"

#define DEBUG_RAW_ENABLE 0

#if DEBUG_RAW_ENABLE
static volatile uint8_t debug_raw_buf[DBUS_FRAME_LEN];
static volatile uint16_t debug_raw_size = 0;
static volatile uint8_t debug_raw_ready = 0;
#endif

uint8_t sbus_rx_buf[2][DBUS_FRAME_LEN];
SBUS_RemoteControl_t g_remote_control;

static volatile uint8_t uart3_error_flag = 0;
static volatile uint32_t uart3_err_code = 0;
static volatile uint32_t dma3_err_code = 0;

static void DBUS_DmaStart(void);
static void DBUS_DmaRestart(void);
static void DBUS_DecodeBuffer(uint8_t index);
static void DBUS_DmaM0CpltCallback(DMA_HandleTypeDef *hdma);
static void DBUS_DmaM1CpltCallback(DMA_HandleTypeDef *hdma);
static void DBUS_DmaErrorCallback(DMA_HandleTypeDef *hdma);
static void DBUS_DecodeToRemoteData(const volatile uint8_t *sbus_raw_data,
                                    SBUS_RemoteControl_t *remote_data);

void DBUS_Init(void) {
  DBUS_DmaStart();
}

static void DBUS_DmaStart(void) {
  DMA_HandleTypeDef *hdma = huart3.hdmarx;

  /*
   * DBUS帧长固定为18字节。用ReceiveToIdle_DMA时，每帧结束都需要CPU
   * 重新启动DMA；回调被半传输事件打断或重启不及时都可能错位。
   *
   * 这里改用DMA硬件双缓冲：一次把buf0/buf1交给DMA，DMA收满18字节
   * 后自动切到另一块，CPU只在整帧完成回调里解析刚写满的buffer。
   */
  hdma->XferCpltCallback = DBUS_DmaM0CpltCallback;
  hdma->XferM1CpltCallback = DBUS_DmaM1CpltCallback;
  hdma->XferErrorCallback = DBUS_DmaErrorCallback;

  /*
   * 不处理半传输。9字节不是完整DBUS帧；原先36字节能跑，是因为
   * HAL半传输回调的36/2刚好等于18。
   */
  hdma->XferHalfCpltCallback = NULL;
  hdma->XferM1HalfCpltCallback = NULL;
  hdma->XferAbortCallback = NULL;

  huart3.ErrorCode = HAL_UART_ERROR_NONE;

  __HAL_UART_CLEAR_OREFLAG(&huart3);
  __HAL_UART_CLEAR_IDLEFLAG(&huart3);
  CLEAR_BIT(hdma->Instance->CR, DMA_SxCR_CT);

  if (HAL_DMAEx_MultiBufferStart_IT(
          hdma, (uint32_t)&huart3.Instance->DR, (uint32_t)sbus_rx_buf[0],
          (uint32_t)sbus_rx_buf[1], DBUS_FRAME_LEN) != HAL_OK) {
    Error_Handler();
  }

  /*
   * HAL没有提供UART ReceiveToIdle + DBM的一键接口，所以DBM由本模块
   * 手动启动。USART3中断只用于IDLE帧同步，不走HAL_UART_IRQHandler，
   * 避免HAL按单缓冲接收流程abort这条DBM DMA。
   */
  SET_BIT(huart3.Instance->CR3, USART_CR3_DMAR);
  __HAL_UART_ENABLE_IT(&huart3, UART_IT_IDLE);
}

static void DBUS_DmaRestart(void) {
  /*
   * 只有半帧IDLE或DMA错误才重启。先关DMAR再Abort，让HAL DMA handle
   * 回到READY，随后才能重新写入M0/M1地址并等待下一帧边界。
   */
  CLEAR_BIT(huart3.Instance->CR3, USART_CR3_DMAR);
  (void)HAL_DMA_Abort(huart3.hdmarx);

  huart3.ErrorCode = HAL_UART_ERROR_NONE;
  DBUS_DmaStart();
}

static void DBUS_DecodeBuffer(uint8_t index) {
#if DEBUG_RAW_ENABLE
  for (uint8_t i = 0; i < DBUS_FRAME_LEN; i++) {
    debug_raw_buf[i] = sbus_rx_buf[index][i];
  }
  debug_raw_size = DBUS_FRAME_LEN;
  debug_raw_ready = 1;
#endif

  DBUS_DecodeToRemoteData(sbus_rx_buf[index], &g_remote_control);
}

static void DBUS_DmaM0CpltCallback(DMA_HandleTypeDef *hdma) {
  if (hdma == huart3.hdmarx) {
    DBUS_DecodeBuffer(0);
  }
}

static void DBUS_DmaM1CpltCallback(DMA_HandleTypeDef *hdma) {
  if (hdma == huart3.hdmarx) {
    DBUS_DecodeBuffer(1);
  }
}

static void DBUS_DmaErrorCallback(DMA_HandleTypeDef *hdma) {
  if (hdma == huart3.hdmarx) {
    dma3_err_code = hdma->ErrorCode;
    uart3_error_flag = 1;
    DBUS_DmaRestart();
  }
}

void DBUS_USART3Idle_IRQHandler(void) {
  if (__HAL_UART_GET_FLAG(&huart3, UART_FLAG_IDLE) != RESET &&
      __HAL_UART_GET_IT_SOURCE(&huart3, UART_IT_IDLE) != RESET) {
    __HAL_UART_CLEAR_IDLEFLAG(&huart3);

    /*
     * IDLE不负责解码，只负责修正帧边界。
     * remaining=18说明DMA刚切到空buffer；remaining=0说明刚收满一帧但
     * TC中断可能还没执行。只有1~17字节才表示当前buffer从半帧开始收，
     * 需要静默重启等待下一帧。
     */
    uint16_t remaining = __HAL_DMA_GET_COUNTER(huart3.hdmarx);
    if (remaining > 0u && remaining < DBUS_FRAME_LEN) {
      DBUS_DmaRestart();
    }
  }
}

void DBUS_Debug_PrintRaw(void) {
#if DEBUG_RAW_ENABLE
  if (debug_raw_ready) {
    usart_printf("Raw[%d]: ", debug_raw_size);
    for (int i = 0; i < debug_raw_size && i < DBUS_FRAME_LEN; i++) {
      usart_printf("%02X ", debug_raw_buf[i]);
    }
    usart_printf("\r\n");
    debug_raw_ready = 0;
  }
#endif
}

void DBUS_Check_Error(void) {
  if (uart3_error_flag) {
    usart_printf("[FATAL] UART3 ERR: 0x%08lX | DMA ERR: 0x%08lX\r\n",
                 uart3_err_code, dma3_err_code);
    uart3_error_flag = 0;
  }
}

static void DBUS_DecodeToRemoteData(const volatile uint8_t *sbus_raw_data,
                                    SBUS_RemoteControl_t *remote_data) {
  if (sbus_raw_data == NULL || remote_data == NULL) {
    return;
  }

  remote_data->controller.channels[0] =
      (sbus_raw_data[0] | (sbus_raw_data[1] << 8)) & 0x07FF;
  remote_data->controller.channels[1] =
      ((sbus_raw_data[1] >> 3) | (sbus_raw_data[2] << 5)) & 0x07FF;
  remote_data->controller.channels[2] =
      ((sbus_raw_data[2] >> 6) | (sbus_raw_data[3] << 2) |
       (sbus_raw_data[4] << 10)) &
      0x07FF;
  remote_data->controller.channels[3] =
      ((sbus_raw_data[4] >> 1) | (sbus_raw_data[5] << 7)) & 0x07FF;

  uint8_t sw = (sbus_raw_data[5] >> 4) & 0x0F;
  remote_data->controller.switches[0] = (sw >> 2) & 0x03;
  remote_data->controller.switches[1] = sw & 0x03;

  remote_data->mouse.x = sbus_raw_data[6] | (sbus_raw_data[7] << 8);
  remote_data->mouse.y = sbus_raw_data[8] | (sbus_raw_data[9] << 8);
  remote_data->mouse.z = sbus_raw_data[10] | (sbus_raw_data[11] << 8);
  remote_data->mouse.press_left = sbus_raw_data[12];
  remote_data->mouse.press_right = sbus_raw_data[13];
  remote_data->keyboard.keys = sbus_raw_data[14] | (sbus_raw_data[15] << 8);
  remote_data->controller.channels[4] =
      sbus_raw_data[16] | (sbus_raw_data[17] << 8);

  remote_data->controller.channels[0] -= 1024;
  remote_data->controller.channels[1] -= 1024;
  remote_data->controller.channels[2] -= 1024;
  remote_data->controller.channels[3] -= 1024;
  remote_data->controller.channels[4] -= 1024;
}
