/**
 * @file task_yaw_runtime.c
 * @brief Yaw配置、命令与共用轴运行时适配。
 *
 * CAN1 反馈 0x205 的独立 GM6020。
 * 仅所属轴任务调用；Yaw/Pitch 不共享句柄、控制历史或测试状态。
 * HAL ms 用于命令、反馈和日志。dt_ms 用于控制周期，不传 FreeRTOS Tick。
 * 命令失效按零速度保持。未标定、参数无效或 CAN 掉线时清零电流。
 * 控制与日志共用 GimbalAxis 的周期快照。本模块不复制 PID 或 CAN 协议。
 */
#include "task/task_yaw/task_yaw_runtime.h"
#include "task/task_yaw/task_yaw_config.h"
#include "can.h"
#include "app/log/gimbal_log.h"
#include <stddef.h>
#include "task/task_yaw/task_yaw_command.h"

/**
 * @brief  准备本轴配置和测试参数并注册 GM6020（只在任务入口做一次）。
 * @param  runtime 任务静态运行时对象（保存本轴全部状态）。
 * @retval true 成功；false 指针无效、CAN/ID 非法、ID 重复或注册表已满；入口不进入控制，不保证零帧已送达。
 * @note   只在任务入口初始化一次，CAN 由 main/CubeMX 初始化（这里不启动外设）。
 */
bool YawTask_RuntimeInit(YawTask_RuntimeTypeDef *runtime) {
  if (runtime == NULL) {
    return false;
  }
  /* 只在任务启动时清空运行时。注册表随后持有内部 motor 地址；运行中重复清空会破坏注册状态。
   * 局部 config 会被 GimbalAxis_Init 复制，不保存本函数栈上的配置地址。 */
  *runtime = (YawTask_RuntimeTypeDef){0};
  const GimbalAxis_ConfigTypeDef config = {
      /* 硬件与逻辑方向分开配置：电流符号控制施力，速度符号只转换反馈。
       * 两者不能当作一个开关一起改，否则闭环方向错误可能被掩盖。 */
      .motor = {
          .hcan = &hcan1,
          .feedback_id = GM6020_FEEDBACK_ID_YAW,
          .current_sign = YAW_MOTOR_CURRENT_SIGN,
          .speed_sign = YAW_FEEDBACK_SPEED_SIGN,
          .feedback_timeout_ms = YAW_FEEDBACK_TIMEOUT_MS,
      },
      /* 三个值均为连续 count，min<center<max 且 valid=true 才允许闭环。
       * 中心还用于首帧单圈坐标对齐；直接使用 0~8191 单圈值会在回绕处误判边界。 */
      .calibration = {
          .valid = YAW_CALIBRATION_VALID != 0U,
          .center_angle_raw = YAW_CALIBRATION_CENTER_ANGLE_RAW,
          .min_angle_raw = YAW_CALIBRATION_MIN_ANGLE_RAW,
          .max_angle_raw = YAW_CALIBRATION_MAX_ANGLE_RAW,
      },
      /* 从本轴配置复制位置环、速度环、限幅、Ramp 和滤波参数，正式/测试共用。
       * 输入满量程速度、位置环速度上限与总电流上限是三个不同约束，不能相互代替。 */
      .control = {
          .max_command_speed_rpm = YAW_MAX_COMMAND_SPEED_RPM, /* ±1000‰ 输入到命令 rpm；决定目标角度移动速度。 */
          .max_speed_target_rpm = YAW_MAX_SPEED_TARGET_RPM, /* 位置控制产生的 rpm 上限，不等于输入满量程。 */
          .position_kp_rpm_per_raw = YAW_POSITION_KP_RPM_PER_RAW, /* 连续 count 误差到 rpm 的比例，不用角度 deg 增益。 */
          .position_ki_rpm_per_raw_s = YAW_POSITION_KI_RPM_PER_RAW_S, /* 连续 count·s 积分到 rpm；控制周期负责秒换算。 */
          .position_integral_limit_rpm = YAW_POSITION_INTEGRAL_LIMIT_RPM, /* 位置积分独立限幅，不能超过总目标速度。 */
          .position_kd_rpm_s_per_raw = YAW_POSITION_KD_RPM_S_PER_RAW, /* 速度推导的误差变化率阻尼，单位 rpm·s/count。 */
          .speed_kp_current_per_rpm = YAW_SPEED_KP_CURRENT_PER_RPM, /* 速度误差 rpm 到 GM6020 raw。 */
          .speed_ki_current_per_rpm_s = YAW_SPEED_KI_CURRENT_PER_RPM_S, /* 速度误差 rpm·s 到 GM6020 raw。 */
          .speed_kd_current_s_per_rpm = YAW_SPEED_KD_CURRENT_S_PER_RPM, /* 测速跳变会产生差分尖峰，单位 raw·s/rpm。 */
          .speed_integral_limit_raw = YAW_SPEED_INTEGRAL_LIMIT_RAW, /* 只限制积分历史，总输出仍受 MAX_CURRENT 限制。 */
          .velocity_feedforward_gain = YAW_VELOCITY_FEEDFORWARD_GAIN, /* 直接叠加运动速度请求；固定目标命令速度为零。 */
          .max_current_raw = YAW_MAX_CURRENT_RAW, /* 控制器 raw 上限，驱动另做 GM6020 协议钳位。 */
          .current_slew_raw_per_s = YAW_CURRENT_SLEW_RAW_PER_S, /* raw/s；反馈恢复时从清零历史重新爬坡。 */
          .speed_filter_alpha = YAW_SPEED_FILTER_ALPHA, /* 无量纲采样权重；增大会降低滞后但减少平滑。 */
          /* Yaw 仍经过统一算法模块；水平轴无重力模型，禁用后输出恒为 0。 */
          .gravity_compensation = {
              .enabled = YAW_GRAVITY_COMPENSATION_ENABLE != 0U,
              .center_angle_raw = YAW_CALIBRATION_CENTER_ANGLE_RAW,
              .bias_current_raw = YAW_GRAVITY_COMPENSATION_BIAS_CURRENT_RAW,
              .amplitude_current_raw = YAW_GRAVITY_COMPENSATION_AMPLITUDE_CURRENT_RAW,
              .current_sign = YAW_GRAVITY_CURRENT_SIGN,
              .max_current_raw = YAW_MAX_CURRENT_RAW,
          },
      },
  };
  /* 测试状态与正式轴属于同一任务，但提示时间和固定目标日志各自独立。
   * 这里只准备参数；不执行标定、不写入实测结果，也不启动固定角度运动。 */
  runtime->calibration_test.log_category = LOG_CATEGORY_YAW_TEST;
  runtime->calibration_test.axis_name = "水平轴";
  runtime->calibration_test.first_limit_name = "左限位";
  runtime->calibration_test.second_limit_name = "右限位";
  runtime->calibration_test.log_period_ms = YAW_CALIBRATION_LOG_PERIOD_MS;
  runtime->calibration_test.prompt_period_ms = YAW_CALIBRATION_PROMPT_PERIOD_MS;
  runtime->angle_test.log_category = LOG_CATEGORY_YAW_TEST;
  runtime->angle_test.axis_name = "水平轴角度测试";
  runtime->angle_test.target_angle_deg = YAW_ANGLE_LOOP_TARGET_ANGLE_DEG;
  runtime->angle_test.log_period_ms = YAW_ANGLE_LOOP_LOG_PERIOD_MS;
  /* 注册成功仅表示句柄可接收 CAN。参数许可和首帧反馈由共用运行时逐周期检查。
   * 返回 false 由任务入口等待处理，不能回退到未经标定的范围驱动。 */
  return GimbalAxis_Init(&runtime->axis, &config);
}

/**
 * @brief  正式模式执行一个周期（读取命令后交给通用轴运行时）。
 * @param  runtime 本轴任务独占对象（不能与另一任务共享）。
 * @param  now_ms HAL_GetTick() 当前 ms（用于命令和反馈超时）。
 * @param  dt_ms 控制周期 ms（用于控制器换算）。
 * @note   Yaw 命令超时检查后交给正式共用运行时（过期命令冻结最后目标位置）。
 * @retval None 运行结果保存在本轴对象；只在本轴任务调用。
 */
void YawTask_RuntimeRunCycle(YawTask_RuntimeTypeDef *runtime,
                             uint32_t now_ms, uint32_t dt_ms) {
  if (runtime == NULL) {
    return;
  }
  /* 先取本轴命令副本并检查原始接收时刻，再运行共用轴控制，最后尝试日志。
   * 快照失败或过期按零速度停止目标移动；CAN 反馈失效由轴运行时发零电流。 */
  Yaw_CommandTypeDef command = {0};
  (void)YawCommand_GetSnapshot(now_ms, &command);
  GimbalAxis_RunCycle(&runtime->axis, &command, now_ms, dt_ms);
  /* 日志只读 axis.cycle，不能重新取 CAN 反馈拼接下一帧。
   * 只有 UART 接受整条文本才更新时间；串口忙时下一周期再试，不延迟电流计算。 */
#if LOG_GLOBAL_ENABLE && LOG_USART1_ENABLE && LOG_TASK_ENABLE && LOG_YAW_ENABLE
  if (now_ms - runtime->last_log_ms >= YAW_TASK_LOG_PERIOD_MS) {
    if (LOG_TRY_PRINTF(LOG_CATEGORY_YAW, GIMBAL_LOG_FORMAT,
        GIMBAL_LOG_ARGS(&runtime->axis, "水平轴", command.velocity_permille))) {
      runtime->last_log_ms = now_ms;
    }
  }
#endif
}
