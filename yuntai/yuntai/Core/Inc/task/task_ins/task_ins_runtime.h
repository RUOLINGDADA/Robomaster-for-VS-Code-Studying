/**
 * @file task_ins_runtime.h
 * @brief INS 硬件组合和事件循环；一个任务独占两总线。
 * 回调只发布事件，所有采集解析、标定、融合和打印在任务中执行。
 */
#ifndef TASK_INS_RUNTIME_H
#define TASK_INS_RUNTIME_H /* 防止重复包含。 */
#include <stdbool.h>
/** @brief 配置运行时、DWT 和应用参数。@return 成功 true；失败不启动传感器。
 * @note 仅 task_ins 调用一次；ISR 尚未绑定时忽略事件。 */
bool InsTask_RuntimeInit(void);
/** @brief 等待并处理最新事件。@return 无；传感器失败时有界退避。
 * @note 仅 task_ins 循环调用，可能阻塞到通知或超时，不可从 ISR 调用。 */
void InsTask_RuntimeRunCycle(void);
#endif
