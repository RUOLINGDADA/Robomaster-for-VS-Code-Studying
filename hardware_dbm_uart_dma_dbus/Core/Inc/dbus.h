/**
 ******************************************************************************
 * @file    dbus.h
 * @brief   大疆 DT7/DR16 遥控器 DBUS 接收与解码接口
 *
 * @note    本模块负责 DBUS 原始帧接收、DMA 双缓冲管理和遥控器数据解码。
 *          应用层通常只需要调用 DBUS_Init()，然后读取 g_remote_control。
 ******************************************************************************
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __DBUS_H__
#define __DBUS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/**
 * @brief DBUS 一帧固定为 18 字节
 * @note  DMA 双缓冲每收满 18 字节就认为得到一帧完整遥控器数据。
 */
#define DBUS_FRAME_LEN 18u

/**
 * @brief DBUS 解码后的遥控器数据
 * @note  原始 DBUS 数据是按 bit 压缩的，应用层不直接解析原始字节，
 *        而是读取该结构体中已经解码好的摇杆、拨杆、鼠标和键盘数据。
 */
typedef struct {
  /**
   * @brief 遥控器本体数据：摇杆、拨轮/保留通道、拨杆
   */
  struct {
    /**
     * @brief 摇杆通道和 CH4 通道
     *
     * @note  解码时已经减去 1024，因此摇杆中位附近为 0。
     *        常见范围约为 -660 ~ +660，异常或未连接时需要结合实际值判断。
     *
     *        channels[0]: 右摇杆左右，右为正，左为负
     *        channels[1]: 右摇杆上下，上为正，下为负
     *        channels[2]: 左摇杆上下，上为正，下为负
     *        channels[3]: 左摇杆左右，右为正，左为负
     *        channels[4]: 鼠标/拨轮扩展通道，按接收机配置可能不同
     */
    int16_t channels[5];

    /**
     * @brief 两个三档拨杆状态
     *
     * @note  switches[0]: 左侧拨杆 S1
     *        switches[1]: 右侧拨杆 S2
     *
     *        取值含义：
     *        1: 上档
     *        2: 中档
     *        3: 下档
     *        0: 异常值，通常表示数据未正确接收或未初始化
     */
    uint8_t switches[2];
  } controller;

  /**
   * @brief 鼠标数据
   * @note  当遥控器接收机连接鼠标时有效；未连接时通常保持 0。
   */
  struct {
    int16_t x; /*!< 鼠标 X 轴移动量，右移为正，左移为负 */
    int16_t y; /*!< 鼠标 Y 轴移动量，上移为正，下移为负 */
    int16_t z; /*!< 鼠标滚轮移动量，方向取决于接收机协议定义 */

    uint8_t press_left;  /*!< 鼠标左键状态，0: 松开，1: 按下 */
    uint8_t press_right; /*!< 鼠标右键状态，0: 松开，1: 按下 */
  } mouse;

  /**
   * @brief 键盘数据
   * @note  每一位代表一个键是否按下，具体按键位定义由 DBUS 协议决定。
   */
  struct {
    uint16_t keys; /*!< 键盘按键位图，bit=1 表示对应按键按下 */
  } keyboard;
} SBUS_RemoteControl_t;

/**
 * @brief 最新一次成功解码的遥控器数据
 * @note  DMA 收到完整 18 字节帧后会更新该变量，应用层可直接读取。
 */
extern SBUS_RemoteControl_t g_remote_control;

/**
 * @brief DBUS 原始接收双缓冲
 * @note  主要用于调试观察原始帧；正常业务逻辑应读取 g_remote_control。
 */
extern uint8_t sbus_rx_buf[2][DBUS_FRAME_LEN];

/**
 * @brief 初始化 DBUS 接收
 * @note  在 MX_DMA_Init() 和 MX_USART3_UART_Init() 之后调用一次。
 */
void DBUS_Init(void);

/**
 * @brief USART3 空闲中断处理入口
 * @note  由 USART3_IRQHandler() 调用，仅用于帧边界重新同步，不负责解码。
 */
void DBUS_USART3Idle_IRQHandler(void);

/**
 * @brief 打印并清除 DBUS 接收错误
 * @note  建议在主循环中调用；中断中只记录错误，不直接打印。
 */
void DBUS_Check_Error(void);

/**
 * @brief 打印最近一次保存的 DBUS 原始帧
 * @note  只有 dbus.c 中 DEBUG_RAW_ENABLE 打开时才有输出。
 */
void DBUS_Debug_PrintRaw(void);

#ifdef __cplusplus
}
#endif

#endif /* __DBUS_H__ */
