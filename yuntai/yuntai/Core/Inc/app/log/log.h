/**
 * @file log.h
 * @brief 所有任务和硬件测试共用的 USART1 DMA 日志接口。
 *
 * 打印点使用 LOG_TRY_PRINTF。宏直接调用唯一发送函数，不经过分类包装函数。
 * 禁用分类时不计算格式参数。DMA 完成后释放共享缓冲区。
 * 本模块不等待、不排队，不支持 ISR 打印，也不访问 USART3。
 */
#ifndef APP_LOG_H
#define APP_LOG_H /* 防止日志接口重复包含。 */

#include "app/log/log_config.h"
#include <stdbool.h>
#include <stdint.h>

/** 日志分类位；共享测试由所属任务传入本轴分类，禁止通过名称猜测轴。 */
typedef enum {
  LOG_CATEGORY_NONE = 0U, /* 不输出；未指定分类的测试保持静默。 */
  LOG_CATEGORY_DBUS_TEXT = 1U << 0, /* DBUS 文本。 */
  LOG_CATEGORY_DBUS_CHART = 1U << 1, /* DBUS 图表。 */
  LOG_CATEGORY_DBUS_TELEMETRY = (1U << 0) | (1U << 1), /* 两种 DBUS 帧合并为一次 DMA。 */
  LOG_CATEGORY_YAW = 1U << 2, /* Yaw 正式诊断和初始化提示。 */
  LOG_CATEGORY_PITCH = 1U << 3, /* Pitch 正式诊断和初始化提示。 */
  LOG_CATEGORY_FEED_MOTOR = 1U << 4, /* 供弹正式诊断和初始化提示。 */
  LOG_CATEGORY_YAW_TEST = 1U << 5, /* Yaw 标定和固定目标测试。 */
  LOG_CATEGORY_PITCH_TEST = 1U << 6, /* Pitch 标定和固定目标测试。 */
  LOG_CATEGORY_FEED_MOTOR_TEST = 1U << 7 /* C610 供弹测试。 */
} Log_CategoryTypeDef;

/* 掩码只使用编译期参数。共享测试的运行时分类也经过同一个开关。 */
#define LOG_ENABLED_CATEGORY_MASK ( \
    (LOG_DBUS_TEXT_ENABLE ? LOG_CATEGORY_DBUS_TEXT : 0U) | \
    (LOG_DBUS_CHART_ENABLE ? LOG_CATEGORY_DBUS_CHART : 0U) | \
    (LOG_TASK_ENABLE && LOG_YAW_ENABLE ? LOG_CATEGORY_YAW : 0U) | \
    (LOG_TASK_ENABLE && LOG_PITCH_ENABLE ? LOG_CATEGORY_PITCH : 0U) | \
    (LOG_TASK_ENABLE && LOG_FEED_MOTOR_ENABLE ? LOG_CATEGORY_FEED_MOTOR : 0U) | \
    (LOG_TEST_ENABLE && LOG_YAW_ENABLE ? LOG_CATEGORY_YAW_TEST : 0U) | \
    (LOG_TEST_ENABLE && LOG_PITCH_ENABLE ? LOG_CATEGORY_PITCH_TEST : 0U) | \
    (LOG_TEST_ENABLE && LOG_FEED_MOTOR_ENABLE ? LOG_CATEGORY_FEED_MOTOR_TEST : 0U)) /* 分类位，无单位。 */

#define LOG_CATEGORY_ENABLED(category) (LOG_GLOBAL_ENABLE && LOG_USART1_ENABLE && \
    (((uint32_t)(category) & (uint32_t)LOG_ENABLED_CATEGORY_MASK) != 0U)) /* 查询分类；无格式化或硬件操作。 */

#if LOG_GLOBAL_ENABLE && LOG_USART1_ENABLE
#define LOG_TRY_PRINTF(category, ...) (LOG_CATEGORY_ENABLED(category) ? \
    Log_TryPrintf(__VA_ARGS__) : false) /* 直接提交日志；禁用分支不计算格式参数。 */
#else
#define LOG_TRY_PRINTF(category, ...) (false) /* 总开关关闭，不计算分类和格式参数。 */
#endif

/**
 * @brief 格式化文本并尝试交给 USART1 DMA。
 * @param fmt printf 格式串；业务通过 LOG_TRY_PRINTF 传入。
 * @param ... 格式参数，仅在分类启用时计算。
 * @retval true DMA 已接受；false 忙、ISR、空指针、空文本、超长或 HAL 失败。
 * @note 仅任务调用。固定缓冲区为 768 字节，整条拒绝超长文本，保留 UTF-8 边界。
 * @note 本函数是唯一发送后端。成功后缓冲区保持不变，直到 USART1 完成回调。
 */
bool Log_TryPrintf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

#endif /* APP_LOG_H */
