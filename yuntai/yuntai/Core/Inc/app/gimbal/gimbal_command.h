/**
  * @file gimbal_command.h
  * @brief 与具体任务无关的云台相对速度命令数据结构。
  *
  * 通俗理解：输入层只告诉云台“想向哪边、以多快转”，
  * 具体如何保持角度由后面的控制器负责。
  */
#ifndef GIMBAL_COMMAND_H
#define GIMBAL_COMMAND_H /* 防止通用云台命令结构重复包含（避免结构体定义重复）。 */

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  int16_t velocity_permille; /* -1000~1000，负/正表示相反方向（把摇杆量化成千分比）。 */
  bool enabled; /* 正式相对输入 true 表示有效；false 走禁用零输出周期（有效回中是 true 且速度为零）。 */
  bool fixed_target; /* true 表示固定目标测试意图；零速度不应被边界过滤当成遥控器回中。 */
  uint32_t timestamp_ms; /* 发布命令的 HAL 毫秒时间戳（用来判断命令是否过期）。 */
} Gimbal_CommandTypeDef;

#endif /* GIMBAL_COMMAND_H（防止命令结构被重复包含） */
