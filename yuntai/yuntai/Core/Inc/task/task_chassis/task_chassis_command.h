/**
 * @file task_chassis_command.h
 * @brief DBUS 单发布者与底盘任务之间的值副本邮箱。
 *
 * 仅任务调用；短临界区保护整个命令。Ctrl 计数由发布者捕获，底盘重复读不重复切模式。
 * 接收时间为 HAL ms，拨杆无功能；邮箱不访问 CAN 或 USART。
 */
#ifndef TASK_CHASSIS_COMMAND_H
#define TASK_CHASSIS_COMMAND_H /* 防止重复包含。 */
#include <stdbool.h>
#include <stdint.h>

typedef struct {
  int16_t vx_raw; /* DBUS CH2 中心化 raw，Submit 按 ±660 限幅。 */
  int16_t vy_raw; /* DBUS CH3 中心化 raw，Submit 按 ±660 限幅。 */
  uint16_t keyboard; /* DJI 按键位图，bit5 为 Ctrl；拨杆不进入此接口。 */
  uint32_t timestamp_ms; /* 原合法帧接收 HAL ms，不是本次转发时间。 */
  bool enabled; /* DBUS 快照合法且在线；false 同时清运动输入。 */
  uint32_t mode_presses; /* GetSnapshot 输出：本通信代内已捕获的 Ctrl 按下次数，Submit 忽略调用者此字段。 */
  uint32_t link_generation; /* GetSnapshot 输出：掉线/长间隔递增，防止遗漏短暂失效；Submit 内部维护。 */
} Chassis_CommandTypeDef;

/**
 * @brief 发布命令，捕获 Ctrl 边沿并保存值副本。
 * @param command 输入字段，内部不保存外部地址；生成字段由邮箱覆盖。
 * @retval true 已发布；false NULL，不修改旧命令。
 * @note 仅现有 DBUS 任务单发布；上电及重连须先见到释放 Ctrl，再允许切模式。
 */
bool ChassisCommand_Submit(const Chassis_CommandTypeDef *command);
/**
 * @brief 原子复制命令并执行 HAL ms 有效期检查。
 * @param now_ms HAL_GetTick 当前毫秒，未来帧竞态会重取 HAL 时刻。
 * @param command 输出完整命令；超时清 enabled、按键和运动量，保留代数供复位。
 * @retval true 复制完成，不代表命令有效；false 输出指针 NULL。
 * @note 底盘任务调用，不消费鼠标增量、不改变原时间戳。
 */
bool ChassisCommand_GetSnapshot(uint32_t now_ms, Chassis_CommandTypeDef *command);
#endif /* TASK_CHASSIS_COMMAND_H */
