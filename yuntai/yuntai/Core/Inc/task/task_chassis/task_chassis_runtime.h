/**
 * @file task_chassis_runtime.h
 * @brief 单底盘任务的模式、电机快照、控制及 CAN1 提交组合。
 *
 * Init/RunCycle 只由 task_chassis 调用；CAN ISR 仅通过 C620 驱动更新反馈。
 * 不创建任务、USART 接收或第二套 PID；调试器可观察静态运行时，不跨任务写字段。
 */
#ifndef TASK_CHASSIS_RUNTIME_H
#define TASK_CHASSIS_RUNTIME_H /* 防止重复包含。 */
#include "app/chassis/chassis_control.h"
#include "bsp/m3508_c620/m3508_c620.h"

typedef struct {
  C620_M3508_HandleTypeDef motors[CHASSIS_WHEEL_COUNT]; /* 唯一四个 CAN1 电机句柄，ISR 保存其长期地址。 */
  ChassisControl_HandleTypeDef control; /* 底盘任务独占的纯控制状态。 */
  C620_M3508_SnapshotTypeDef feedback[CHASSIS_WHEEL_COUNT]; /* 每周期每轮只取一次的反馈。 */
  ChassisControl_OutputTypeDef output; /* 本周期逻辑目标与电流，日志复用。 */
  Chassis_ModeTypeDef mode; /* Ctrl 循环模式；上电/命令失效回 STOP。 */
  uint32_t last_presses; /* 已消费的本代 Ctrl 次数，无符号差处理回绕。 */
  uint32_t link_generation; /* 最近输入通信代数，改变时先清模式。 */
  uint32_t previous_ms; /* 上一周期 HAL ms，不能用 FreeRTOS Tick。 */
  uint32_t last_log_ms; /* 最近成功提交日志的 HAL ms；忙时不续期。 */
  uint32_t send_failures; /* HAL 未接受或 Bus-Off 的提交累计数，诊断用。 */
  bool wheel_online[CHASSIS_WHEEL_COUNT]; /* 上周期各轮安全门，用于恢复时清该轮 PID。 */
  bool output_allowed[CHASSIS_WHEEL_COUNT]; /* 本周期各轮使能设置结果，和 HAL 提交分别记录。 */
  bool ready; /* 四轮注册和控制配置均完成，失败回滚后 false。 */
  bool clock_valid; /* 已建立 previous_ms，首周期只零输出。 */
  bool zero_pending; /* 模式/故障停机帧尚未被 HAL 接受，禁止抢先非零输出。 */
  bool submitted; /* 本周期 HAL 是否接受发送；不代表电调已经执行。 */
  bool was_bus_off; /* 上周期 Bus-Off，用于退出时再清历史并等待新帧。 */
} ChassisTask_RuntimeTypeDef;

/**
 * @brief 组装配置并注册 CAN1 ID1~4，失败回滚，不影响其它总线。
 * @param runtime 静态或任务长期存活的运行时，初始应全零。
 * @retval true 已就绪；false 配置或注册失败，所有本轮注册已撤销。
 * @note task_chassis 初始化调用；main 必须先启动双 CAN。
 */
bool ChassisTask_RuntimeInit(ChassisTask_RuntimeTypeDef *runtime);
/**
 * @brief 推进一次模式、安全门、控制、发送和可选日志。
 * @param runtime 已就绪的唯一底盘运行时。
 * @param now_ms HAL_GetTick 毫秒时间，计算实际 dt_s。
 * @retval None；异常清零且不等待，下一周期重试。
 * @note 只在底盘任务中周期调用，禁止 ISR/多任务并发。
 */
void ChassisTask_RuntimeRunCycle(ChassisTask_RuntimeTypeDef *runtime, uint32_t now_ms);
#endif /* TASK_CHASSIS_RUNTIME_H */
