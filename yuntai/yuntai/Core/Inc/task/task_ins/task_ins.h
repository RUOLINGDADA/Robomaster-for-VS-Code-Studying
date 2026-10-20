/**
 * @file task_ins.h
 * @brief CubeMX 创建的唯一 INS 强入口；只负责初始化和事件调度。
 * 不手写 osThreadNew，不改动现有电机任务。需要 SPI1/I2C3/EXTI/DMA 已生成。
 */
#ifndef TASK_INS_H
#define TASK_INS_H /* 防止重复包含。 */
/** @brief INS 任务强入口。@param argument 未使用，CubeMX 填 NULL。
 * @return 永不返回；参数错误时受控等待，不影响其他任务。 */
void task_ins_entry(void *argument);
#endif
