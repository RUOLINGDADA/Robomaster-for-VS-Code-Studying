/**
 * @file task_dbus_runtime.c
 * @brief 一帧遥控数据适配两轴命令（拨杆只记录，CH0/CH1 右摇杆控制云台）。
 */
#include "task/task_dbus/task_dbus_runtime.h"
#include "task/task_dbus/task_dbus_config.h"
#include "bsp/dbus/dbus.h"
#include "task/task_yaw/task_yaw_command.h"
#include "task/task_pitch/task_pitch_command.h"
#include "usart.h"

_Static_assert(DBUS_STICK_DEADBAND_RAW >= 0 && DBUS_STICK_DEADBAND_RAW < DBUS_CHANNEL_SPAN,
               "DBUS deadband must be smaller than full stick range");
_Static_assert(DBUS_YAW_CHANNEL < 4U && DBUS_PITCH_CHANNEL < 4U,
               "DBUS gimbal input must use a stick channel");
_Static_assert((DBUS_YAW_INPUT_SIGN == 1 || DBUS_YAW_INPUT_SIGN == -1) &&
               (DBUS_PITCH_INPUT_SIGN == 1 || DBUS_PITCH_INPUT_SIGN == -1),
               "DBUS input signs must be +1 or -1");

static uint32_t g_last_log_ms; /* 最近成功交给 UART DMA 的日志时刻，单位 HAL ms（忙时不提前续期）。 */

/* 原始摇杆量换千分比；先死区再缩放、最后限幅（噪声不能持续积分目标，越界不能放大转速）。 */
static int16_t DbusTask_MapChannel(int16_t channel, int32_t sign) {
  if (channel >= -DBUS_STICK_DEADBAND_RAW && channel <= DBUS_STICK_DEADBAND_RAW) {
    return 0;
  }
  int32_t velocity = (int32_t)channel * 1000 * sign / DBUS_CHANNEL_SPAN;
  if (velocity > 1000) {
    velocity = 1000;
  } else if (velocity < -1000) {
    velocity = -1000;
  }
  return (int16_t)velocity;
}

void DbusTask_RuntimeRunCycle(uint32_t now_ms) {
  Dbus_Process(now_ms);
  Dbus_SnapshotTypeDef input = {0};
  const bool available = Dbus_GetSnapshot(now_ms, &input) && input.online;
  const Yaw_CommandTypeDef yaw = {
      .velocity_permille = available ? DbusTask_MapChannel(input.data.channels[DBUS_YAW_CHANNEL], DBUS_YAW_INPUT_SIGN) : 0,
      .enabled = available,
      .timestamp_ms = input.timestamp_ms,
  };
  const Pitch_CommandTypeDef pitch = {
      .velocity_permille = available ? DbusTask_MapChannel(input.data.channels[DBUS_PITCH_CHANNEL], DBUS_PITCH_INPUT_SIGN) : 0,
      .enabled = available,
      .timestamp_ms = input.timestamp_ms,
  };
  /* 两轴都用接收时刻；即使本周期没有新帧也不把 now_ms 写入命令（否则断链后旧速度永远不过期）。 */
  (void)YawCommand_Submit(&yaw);
  (void)PitchCommand_Submit(&pitch);
  if (now_ms - g_last_log_ms >= DBUS_TASK_LOG_PERIOD_MS &&
      usart_try_printf("[DBUS遥控] 在线=%u 右左右=%d 右上下=%d 左左右=%d 左上下=%d 滚轮=%d 左拨杆=%u 右拨杆=%u Yaw命令=%d Pitch命令=%d 反馈年龄(ms)=%lu 合法帧=%lu 坏帧=%lu 重同步=%lu UART错误=%lu DMA错误=%lu 启动错误=%lu\r\n",
          input.online ? 1U : 0U, (int)input.data.channels[0], (int)input.data.channels[1],
          (int)input.data.channels[2], (int)input.data.channels[3], (int)input.data.channels[4],
          (unsigned)input.data.switch_left, (unsigned)input.data.switch_right,
          (int)yaw.velocity_permille, (int)pitch.velocity_permille,
          (unsigned long)input.age_ms, (unsigned long)input.valid_frames,
          (unsigned long)input.invalid_frames, (unsigned long)input.resync_count,
          (unsigned long)input.uart_errors, (unsigned long)input.dma_errors,
          (unsigned long)input.start_errors)) {
    g_last_log_ms = now_ms;
  }
}
