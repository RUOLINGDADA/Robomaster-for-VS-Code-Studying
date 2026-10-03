/**
  * @file gimbal_command.h
  * @brief 与具体任务无关的云台相对速度命令数据结构。
  */
#ifndef GIMBAL_COMMAND_H
#define GIMBAL_COMMAND_H /* 防止通用云台命令结构重复包含。 */

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  int16_t velocity_permille; /* -1000~1000，负/正表示相反方向。 */
  bool enabled; /* true 表示允许命令改变目标角度。 */
  uint32_t timestamp_ms; /* 发布命令的 HAL 毫秒时间戳。 */
} Gimbal_CommandTypeDef;

#endif /* GIMBAL_COMMAND_H */
