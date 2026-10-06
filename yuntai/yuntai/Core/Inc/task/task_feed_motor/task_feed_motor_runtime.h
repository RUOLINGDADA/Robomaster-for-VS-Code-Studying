/**
 * @file task_feed_motor_runtime.h
 * @brief 双 C615 PWM 与唯一 M2006 自动供弹运行时。
 *
 * CAN1 ID1 M2006 与 PE9/PE11 双 C615。
 * 仅供弹任务调用。两个 PWM 句柄由该任务独占，CAN ISR 只更新反馈。
 * 部分初始化失败时保留已注册 CAN 句柄，重试未启动 PWM 并保持停止值。
 * 每周期在短临界区复制首帧标志和反馈；控制和日志共用这一份快照。
 * 命令/反馈年龄使用 HAL ms；阶段使用 FreeRTOS Tick；Ramp 周期换成 ms。
 * 故障立即写停止脉宽。在线释放时摩擦轮 Ramp 停止，M2006 当周期清零。
 * HAL 接受 CAN 帧不等于电调执行。本模块不检测卡弹或计算实发弹数。
 */
/* 调用顺序：main 外设初始化→所属任务 RuntimeInit→成功后周期 RunCycle。
 * 对象须保持固定地址；不得由其它任务再次初始化或复用。日志只能读本周期副本。 */
#ifndef TASK_FEED_MOTOR_RUNTIME_H
#define TASK_FEED_MOTOR_RUNTIME_H /* 防止重复包含。 */
#include "bsp/c610_m2006/c610_m2006.h"
#include "bsp/snail_2305/snail_2305.h"
#include "task/task_feed_motor/task_feed_motor_command.h"
#include "task/task_feed_motor/task_feed_motor_control.h"

typedef struct {
  C610_M2006_HandleTypeDef motor; /* 唯一 M2006 句柄；CAN ISR 更新反馈。 */
  Snail2305_HandleTypeDef left; /* PE9/TIM1_CH1；任务独占输出。 */
  Snail2305_HandleTypeDef right; /* PE11/TIM1_CH2；任务独占输出。 */
  FeedMotor_ControlTypeDef control; /* 供弹阶段与目标；任务独占。 */
  TickType_t last_log_tick; /* UART 接受整条日志后的 FreeRTOS Tick；Init=0，失败不更新，周期控制不依赖它。 */
  TickType_t last_cycle_tick; /* 上一周期的 FreeRTOS Tick；仅 cycle_started=true 时有效，用于换算实际 dt_ms。 */
  bool configured; /* 首次清空/控制初始化已完成；重试保留 true，防止清掉 CAN 注册表仍引用的句柄。 */
  bool initialized; /* CAN 句柄和两 PWM 均成功才置 true；失败不执行 RunCycle，不代表电调机械已就绪。 */
  bool cycle_started; /* Init=false；首周期用 TASK_PERIOD_MS 后置 true，时间戳 0 也可作为上一周期。 */
} FeedMotor_RuntimeTypeDef;

/**
 * @brief 初始化三个驱动并保持停止；允许失败后重试。
 * @param runtime 静态零初始化的任务对象；不能复制到别的任务。
 * @retval true 三个驱动就绪；false 部分失败，已启动的输出仍停止。
 * @note 仅供弹任务调用；重试保留已注册的 C610 句柄。
 */
bool FeedMotor_RuntimeInit(FeedMotor_RuntimeTypeDef *runtime);
/**
 * @brief 执行一个反馈、控制、PWM、CAN 和日志周期。
 * @param runtime 已初始化任务对象。
 * @param now_ms HAL_GetTick 时间，ms；用于命令/反馈新鲜度。
 * @param now_tick FreeRTOS Tick；用于阶段和日志。
 * @retval None；许可失效时输出停止值，不等待硬件。
 * @note 仅供弹任务调用；日志繁忙不阻塞控制。
 */
void FeedMotor_RuntimeRunCycle(FeedMotor_RuntimeTypeDef *runtime, uint32_t now_ms, TickType_t now_tick);
#endif /* TASK_FEED_MOTOR_RUNTIME_H */
