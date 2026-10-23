# 宏逐项参考与源码消费点

由 [生成脚本](tools/update_technical_reference.ps1) 从 Core 源码生成。原理见 [技术总文档](TECHNICAL_ARCHITECTURE.md)，源码摘录见 [实现说明](IMPLEMENTATION_GUIDE.md)。

范围包含 Core 内业务宏、私有宏、包含保护和平台配置。Drivers/Middlewares 内部宏使用厂商文档。保留条件编译每个定义位置：值是源表达式，不是编译器覆盖后的生效值。已注释的伪定义不计入；Core引用扫描只作为导航，第三方实际消费以预处理和源代码为准。

## Core/Inc/algorithm/ahrs/ahrs.h

### ALGORITHM_AHRS_H（第 9 行定义）

```c
#define ALGORITHM_AHRS_H /* 防止重复包含。 */
```

包含保护：避免同一翻译单元重复声明，没有控制算法。

源文件：[Core/Inc/algorithm/ahrs/ahrs.h](../Core/Inc/algorithm/ahrs/ahrs.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

## Core/Inc/algorithm/filter/low_pass_filter.h

### ALGORITHM_LOW_PASS_FILTER_H（第 11 行定义）

```c
#define ALGORITHM_LOW_PASS_FILTER_H /* 防止低通滤波接口被重复包含（避免类型和函数声明重复）。 */
```

包含保护：避免同一翻译单元重复声明，没有控制算法。

源文件：[Core/Inc/algorithm/filter/low_pass_filter.h](../Core/Inc/algorithm/filter/low_pass_filter.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

## Core/Inc/algorithm/gravity_compensation/gravity_compensation.h

### ALGORITHM_GRAVITY_COMPENSATION_H（第 11 行定义）

```c
#define ALGORITHM_GRAVITY_COMPENSATION_H /* 防止重力补偿接口被重复包含。 */
```

包含保护：避免同一翻译单元重复声明，没有控制算法。

源文件：[Core/Inc/algorithm/gravity_compensation/gravity_compensation.h](../Core/Inc/algorithm/gravity_compensation/gravity_compensation.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

## Core/Inc/algorithm/mouse_virtual_joystick/mouse_virtual_joystick.h

### ALGORITHM_MOUSE_VIRTUAL_JOYSTICK_H（第 10 行定义）

```c
#define ALGORITHM_MOUSE_VIRTUAL_JOYSTICK_H /* 防止虚拟鼠标接口重复包含。 */
```

包含保护：避免同一翻译单元重复声明，没有控制算法。

源文件：[Core/Inc/algorithm/mouse_virtual_joystick/mouse_virtual_joystick.h](../Core/Inc/algorithm/mouse_virtual_joystick/mouse_virtual_joystick.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

## Core/Inc/algorithm/pid/pid.h

### ALGORITHM_PID_H（第 11 行定义）

```c
#define ALGORITHM_PID_H /* 防止 PID 接口被重复包含（避免类型和函数声明重复）。 */
```

包含保护：避免同一翻译单元重复声明，没有控制算法。

源文件：[Core/Inc/algorithm/pid/pid.h](../Core/Inc/algorithm/pid/pid.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

## Core/Inc/algorithm/ramp/ramp.h

### ALGORITHM_RAMP_H（第 11 行定义）

```c
#define ALGORITHM_RAMP_H /* 防止 Ramp 接口被重复包含（避免类型和函数声明重复）。 */
```

包含保护：避免同一翻译单元重复声明，没有控制算法。

源文件：[Core/Inc/algorithm/ramp/ramp.h](../Core/Inc/algorithm/ramp/ramp.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

## Core/Inc/app/chassis/chassis_control.h

### APP_CHASSIS_CONTROL_H（第 9 行定义）

```c
#define APP_CHASSIS_CONTROL_H /* 防止重复包含。 */
```

包含保护：避免同一翻译单元重复声明，没有控制算法。

源文件：[Core/Inc/app/chassis/chassis_control.h](../Core/Inc/app/chassis/chassis_control.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### CHASSIS_WHEEL_COUNT（第 13 行定义）

```c
#define CHASSIS_WHEEL_COUNT 4U /* 底盘轮数，个；数组顺序为电调 ID1~4。 */
```

四轮数组容量，顺序为CAN1 ID1右前、ID2左前、ID3左后、ID4右后。

源文件：[Core/Inc/app/chassis/chassis_control.h](../Core/Inc/app/chassis/chassis_control.h)。

Core引用：[Core/Inc/app/chassis/chassis_control.h](../Core/Inc/app/chassis/chassis_control.h) 第 50 行；[Core/Inc/task/task_chassis/task_chassis_runtime.h](../Core/Inc/task/task_chassis/task_chassis_runtime.h) 第 14 行；[Core/Src/app/chassis/chassis_control.c](../Core/Src/app/chassis/chassis_control.c) 第 103 行；[Core/Src/task/task_chassis/task_chassis_runtime.c](../Core/Src/task/task_chassis/task_chassis_runtime.c) 第 15 行。

## Core/Inc/app/gimbal/gimbal_axis.h

### GIMBAL_AXIS_H（第 11 行定义）

```c
#define GIMBAL_AXIS_H /* 防止通用云台轴接口被重复包含。 */
```

包含保护：避免同一翻译单元重复声明，没有控制算法。

源文件：[Core/Inc/app/gimbal/gimbal_axis.h](../Core/Inc/app/gimbal/gimbal_axis.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

## Core/Inc/app/gimbal/gimbal_command.h

### GIMBAL_COMMAND_H（第 10 行定义）

```c
#define GIMBAL_COMMAND_H /* 防止通用云台命令结构重复包含（避免结构体定义重复）。 */
```

包含保护：避免同一翻译单元重复声明，没有控制算法。

源文件：[Core/Inc/app/gimbal/gimbal_command.h](../Core/Inc/app/gimbal/gimbal_command.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

## Core/Inc/app/gimbal/gimbal_control.h

### GIMBAL_CONTROL_H（第 12 行定义）

```c
#define GIMBAL_CONTROL_H /* 防止通用云台控制接口被重复包含（避免控制结构和函数声明重复）。 */
```

包含保护：避免同一翻译单元重复声明，没有控制算法。

源文件：[Core/Inc/app/gimbal/gimbal_control.h](../Core/Inc/app/gimbal/gimbal_control.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

## Core/Inc/app/ins/ins.h

### APP_INS_H（第 7 行定义）

```c
#define APP_INS_H /* 防止重复包含。 */
```

包含保护：避免同一翻译单元重复声明，没有控制算法。

源文件：[Core/Inc/app/ins/ins.h](../Core/Inc/app/ins/ins.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

## Core/Inc/app/log/gimbal_log.h

### APP_GIMBAL_LOG_H（第 10 行定义）

```c
#define APP_GIMBAL_LOG_H /* 防止云台日志格式重复包含。 */
```

包含保护：避免同一翻译单元重复声明，没有控制算法。

源文件：[Core/Inc/app/log/gimbal_log.h](../Core/Inc/app/log/gimbal_log.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### GIMBAL_LOG_FORMAT（第 31 行定义）

```c
#define GIMBAL_LOG_FORMAT \
  "[%s] 反馈=%s 阶段=%s 原因=%s 命令百分之一‰=%ld 人工受限=%u 目标让步=%u 向外电流受限=%u 输入暂停=%u " \
  "目标/实际控制角毫度=%ld/%ld 误差等效count=%ld 编码器count=%ld " \
  "目标/控制反馈速度百分之一rpm=%ld/%ld 编码器rpm=%d " \
  "速度环/补偿/目标/提交/反馈电流=%d/%d/%d/%d/%d 输出许可=%u CAN提交=%u " \
  "机械边界=%s CAN龄ms=%lu 在线=%u 快照=%u IMU快照/有效=%u/%u 陀螺/加计龄ms=%lu/%lu 重置=%lu\r\n"
```

统一日志格式/同周期快照字段；轴参数多次展开，须无副作用。

源文件：[Core/Inc/app/log/gimbal_log.h](../Core/Inc/app/log/gimbal_log.h)。

Core引用：[Core/Src/bsp/gm6020/test_gm6020_angle_loop.c](../Core/Src/bsp/gm6020/test_gm6020_angle_loop.c) 第 86 行；[Core/Src/task/task_pitch/task_pitch_runtime.c](../Core/Src/task/task_pitch/task_pitch_runtime.c) 第 127 行；[Core/Src/task/task_yaw/task_yaw_runtime.c](../Core/Src/task/task_yaw/task_yaw_runtime.c) 第 126 行。

### GIMBAL_LOG_ARGS（第 39 行定义）

```c
#define GIMBAL_LOG_ARGS(axis, label, command_permille) \
  label, GimbalAxis_FeedbackName((axis)->cycle.feedback_source), \
  GimbalAxis_PhaseName((axis)->cycle.phase), GimbalAxis_ReasonName((axis)->cycle.reason), \
  (long)GimbalLog_Scaled((command_permille), 100.0f), \
  (axis)->cycle.control.command_limited ? 1U : 0U, \
  (axis)->cycle.control.target_yielded ? 1U : 0U, \
  (axis)->cycle.control.outward_current_limited ? 1U : 0U, (axis)->cycle.input_inhibited ? 1U : 0U, \
  (long)GimbalLog_Scaled((float)(axis)->cycle.control.target_angle_raw, 360000.0f / GM6020_ENCODER_COUNTS_PER_REV), \
  (long)GimbalLog_Scaled((axis)->cycle.control.feedback_angle_raw, 360000.0f / GM6020_ENCODER_COUNTS_PER_REV), \
  (long)(axis)->cycle.control.angle_error_raw, (long)(axis)->cycle.snapshot.feedback.angle_total_raw, \
  (long)GimbalLog_Scaled((axis)->cycle.control.target_speed_rpm, 100.0f), \
  (long)GimbalLog_Scaled((axis)->cycle.control.feedback_speed_rpm, 100.0f), \
  (axis)->cycle.snapshot.feedback.speed_rpm, (axis)->cycle.control.speed_loop_current_raw, \
  (axis)->cycle.control.gravity_compensation_current_raw, (axis)->cycle.control.target_current_raw, \
  (axis)->cycle.applied_current_raw, (axis)->cycle.snapshot.feedback.current_raw, \
  (axis)->cycle.output_enabled ? 1U : 0U, (axis)->cycle.can_submitted ? 1U : 0U, \
  (axis)->cycle.limit == GM6020_LIMIT_MIN ? "最小" : \
  ((axis)->cycle.limit == GM6020_LIMIT_MAX ? "最大" : "无"), \
  (unsigned long)(axis)->cycle.feedback_age_ms, \
  (axis)->cycle.snapshot_valid && (axis)->cycle.snapshot.online ? 1U : 0U, \
  (axis)->cycle.snapshot_valid ? 1U : 0U, (axis)->cycle.ins_available ? 1U : 0U, \
  (axis)->cycle.ins.attitude_valid ? 1U : 0U, (unsigned long)(axis)->cycle.gyro_age_ms, \
  (unsigned long)(axis)->cycle.accel_age_ms, (unsigned long)(axis)->cycle.ins.reset_sequence
```

统一日志格式/同周期快照字段；轴参数多次展开，须无副作用。

源文件：[Core/Inc/app/log/gimbal_log.h](../Core/Inc/app/log/gimbal_log.h)。

Core引用：[Core/Src/bsp/gm6020/test_gm6020_angle_loop.c](../Core/Src/bsp/gm6020/test_gm6020_angle_loop.c) 第 87 行；[Core/Src/task/task_pitch/task_pitch_runtime.c](../Core/Src/task/task_pitch/task_pitch_runtime.c) 第 128 行；[Core/Src/task/task_yaw/task_yaw_runtime.c](../Core/Src/task/task_yaw/task_yaw_runtime.c) 第 127 行。

## Core/Inc/app/log/log.h

### APP_LOG_H（第 10 行定义）

```c
#define APP_LOG_H /* 防止日志接口重复包含。 */
```

包含保护：避免同一翻译单元重复声明，没有控制算法。

源文件：[Core/Inc/app/log/log.h](../Core/Inc/app/log/log.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### LOG_ENABLED_CATEGORY_MASK（第 36 行定义）

```c
#define LOG_ENABLED_CATEGORY_MASK LOG_CATEGORY_INS_MAG_CALIBRATION /* 标定独占分类；其他日志暂停，控制和采集继续。 */
```

分类位掩码、0/1开关校验和条件运算；关闭分类不求值格式参数。

源文件：[Core/Inc/app/log/log.h](../Core/Inc/app/log/log.h)。

Core引用：[Core/Inc/app/log/log.h](../Core/Inc/app/log/log.h) 第 54 行。

### LOG_ENABLED_CATEGORY_MASK（第 38 行定义）

```c
#define LOG_ENABLED_CATEGORY_MASK LOG_CATEGORY_DBUS_TRACE /* 输入采集独占带宽；不影响设备控制。 */
```

分类位掩码、0/1开关校验和条件运算；关闭分类不求值格式参数。

源文件：[Core/Inc/app/log/log.h](../Core/Inc/app/log/log.h)。

Core引用：[Core/Inc/app/log/log.h](../Core/Inc/app/log/log.h) 第 54 行。

### LOG_ENABLED_CATEGORY_MASK（第 40 行定义）

```c
#define LOG_ENABLED_CATEGORY_MASK ( \
    (LOG_INS_ENABLE ? LOG_CATEGORY_INS : 0U) | \
    (LOG_TASK_ENABLE && LOG_CHASSIS_ENABLE ? LOG_CATEGORY_CHASSIS : 0U) | \
    (LOG_DBUS_TEXT_ENABLE ? LOG_CATEGORY_DBUS_TEXT : 0U) | \
    (LOG_DBUS_CHART_ENABLE ? LOG_CATEGORY_DBUS_CHART : 0U) | \
    (LOG_TASK_ENABLE && LOG_YAW_ENABLE ? LOG_CATEGORY_YAW : 0U) | \
    (LOG_TASK_ENABLE && LOG_PITCH_ENABLE ? LOG_CATEGORY_PITCH : 0U) | \
    (LOG_TASK_ENABLE && LOG_FEED_MOTOR_ENABLE ? LOG_CATEGORY_FEED_MOTOR : 0U) | \
    (LOG_TEST_ENABLE && LOG_YAW_ENABLE ? LOG_CATEGORY_YAW_TEST : 0U) | \
    (LOG_TEST_ENABLE && LOG_PITCH_ENABLE ? LOG_CATEGORY_PITCH_TEST : 0U) | \
    (LOG_TEST_ENABLE && LOG_FEED_MOTOR_ENABLE ? LOG_CATEGORY_FEED_MOTOR_TEST : 0U)) /* 分类位，无单位。 */
```

分类位掩码、0/1开关校验和条件运算；关闭分类不求值格式参数。

源文件：[Core/Inc/app/log/log.h](../Core/Inc/app/log/log.h)。

Core引用：[Core/Inc/app/log/log.h](../Core/Inc/app/log/log.h) 第 54 行。

### LOG_CATEGORY_ENABLED（第 53 行定义）

```c
#define LOG_CATEGORY_ENABLED(category) (LOG_GLOBAL_ENABLE && LOG_USART1_ENABLE && \
    (((uint32_t)(category) & (uint32_t)LOG_ENABLED_CATEGORY_MASK) != 0U)) /* 查询分类；无格式化或硬件操作。 */
```

分类位掩码、0/1开关校验和条件运算；关闭分类不求值格式参数。

源文件：[Core/Inc/app/log/log.h](../Core/Inc/app/log/log.h)。

Core引用：[Core/Src/bsp/gm6020/test_gm6020_angle_loop.c](../Core/Src/bsp/gm6020/test_gm6020_angle_loop.c) 第 79 行；[Core/Src/bsp/gm6020/test_gm6020_calibration.c](../Core/Src/bsp/gm6020/test_gm6020_calibration.c) 第 53 行；[Core/Src/task/task_dbus/task_dbus_runtime.c](../Core/Src/task/task_dbus/task_dbus_runtime.c) 第 120 行。

### LOG_TRY_PRINTF（第 57 行定义）

```c
#define LOG_TRY_PRINTF(category, ...) (LOG_CATEGORY_ENABLED(category) ? \
    Log_TryPrintf(__VA_ARGS__) : false) /* 直接提交日志；禁用分支不计算格式参数。 */
```

分类位掩码、0/1开关校验和条件运算；关闭分类不求值格式参数。

源文件：[Core/Inc/app/log/log.h](../Core/Inc/app/log/log.h)。

Core引用：[Core/Src/bsp/c610_m2006/test_c610_m2006_angle_step.c](../Core/Src/bsp/c610_m2006/test_c610_m2006_angle_step.c) 第 116 行；[Core/Src/bsp/gm6020/test_gm6020_angle_loop.c](../Core/Src/bsp/gm6020/test_gm6020_angle_loop.c) 第 65 行；[Core/Src/bsp/gm6020/test_gm6020_calibration.c](../Core/Src/bsp/gm6020/test_gm6020_calibration.c) 第 67 行；[Core/Src/bsp/snail_2305/test_snail_2305.c](../Core/Src/bsp/snail_2305/test_snail_2305.c) 第 64 行；[Core/Src/task/task_chassis/task_chassis_runtime.c](../Core/Src/task/task_chassis/task_chassis_runtime.c) 第 59 行；[Core/Src/task/task_dbus/task_dbus_runtime.c](../Core/Src/task/task_dbus/task_dbus_runtime.c) 第 198 行；[Core/Src/task/task_dbus/task_dbus_trace.c](../Core/Src/task/task_dbus/task_dbus_trace.c) 第 210 行；[Core/Src/task/task_feed_motor/task_feed_motor.c](../Core/Src/task/task_feed_motor/task_feed_motor.c) 第 58 行；[Core/Src/task/task_feed_motor/task_feed_motor_runtime.c](../Core/Src/task/task_feed_motor/task_feed_motor_runtime.c) 第 201 行；[Core/Src/task/task_ins/task_ins_runtime.c](../Core/Src/task/task_ins/task_ins_runtime.c) 第 304 行；[Core/Src/task/task_pitch/task_pitch.c](../Core/Src/task/task_pitch/task_pitch.c) 第 39 行；[Core/Src/task/task_pitch/task_pitch_runtime.c](../Core/Src/task/task_pitch/task_pitch_runtime.c) 第 127 行；[Core/Src/task/task_yaw/task_yaw.c](../Core/Src/task/task_yaw/task_yaw.c) 第 39 行；[Core/Src/task/task_yaw/task_yaw_runtime.c](../Core/Src/task/task_yaw/task_yaw_runtime.c) 第 126 行。

### LOG_TRY_PRINTF（第 60 行定义）

```c
#define LOG_TRY_PRINTF(category, ...) (false) /* 总开关关闭，不计算分类和格式参数。 */
```

分类位掩码、0/1开关校验和条件运算；关闭分类不求值格式参数。

源文件：[Core/Inc/app/log/log.h](../Core/Inc/app/log/log.h)。

Core引用：[Core/Src/bsp/c610_m2006/test_c610_m2006_angle_step.c](../Core/Src/bsp/c610_m2006/test_c610_m2006_angle_step.c) 第 116 行；[Core/Src/bsp/gm6020/test_gm6020_angle_loop.c](../Core/Src/bsp/gm6020/test_gm6020_angle_loop.c) 第 65 行；[Core/Src/bsp/gm6020/test_gm6020_calibration.c](../Core/Src/bsp/gm6020/test_gm6020_calibration.c) 第 67 行；[Core/Src/bsp/snail_2305/test_snail_2305.c](../Core/Src/bsp/snail_2305/test_snail_2305.c) 第 64 行；[Core/Src/task/task_chassis/task_chassis_runtime.c](../Core/Src/task/task_chassis/task_chassis_runtime.c) 第 59 行；[Core/Src/task/task_dbus/task_dbus_runtime.c](../Core/Src/task/task_dbus/task_dbus_runtime.c) 第 198 行；[Core/Src/task/task_dbus/task_dbus_trace.c](../Core/Src/task/task_dbus/task_dbus_trace.c) 第 210 行；[Core/Src/task/task_feed_motor/task_feed_motor.c](../Core/Src/task/task_feed_motor/task_feed_motor.c) 第 58 行；[Core/Src/task/task_feed_motor/task_feed_motor_runtime.c](../Core/Src/task/task_feed_motor/task_feed_motor_runtime.c) 第 201 行；[Core/Src/task/task_ins/task_ins_runtime.c](../Core/Src/task/task_ins/task_ins_runtime.c) 第 304 行；[Core/Src/task/task_pitch/task_pitch.c](../Core/Src/task/task_pitch/task_pitch.c) 第 39 行；[Core/Src/task/task_pitch/task_pitch_runtime.c](../Core/Src/task/task_pitch/task_pitch_runtime.c) 第 127 行；[Core/Src/task/task_yaw/task_yaw.c](../Core/Src/task/task_yaw/task_yaw.c) 第 39 行；[Core/Src/task/task_yaw/task_yaw_runtime.c](../Core/Src/task/task_yaw/task_yaw_runtime.c) 第 126 行。

## Core/Inc/app/log/log_config.h

### APP_LOG_CONFIG_H（第 12 行定义）

```c
#define APP_LOG_CONFIG_H /* 防止日志配置重复包含。 */
```

包含保护：避免同一翻译单元重复声明，没有控制算法。

源文件：[Core/Inc/app/log/log_config.h](../Core/Inc/app/log/log_config.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### LOG_GLOBAL_ENABLE（第 15 行定义）

```c
#define LOG_GLOBAL_ENABLE 1U /* 日志总开关，0/1；关闭全部格式化和发送。 */
```

编译期日志开关/图表前缀，仅控制诊断，不控制电机使能。

源文件：[Core/Inc/app/log/log_config.h](../Core/Inc/app/log/log_config.h)。

Core引用：[Core/Inc/app/log/log.h](../Core/Inc/app/log/log.h) 第 56 行；[Core/Inc/app/log/log_config.h](../Core/Inc/app/log/log_config.h) 第 61 行；[Core/Src/app/log/log.c](../Core/Src/app/log/log.c) 第 17 行；[Core/Src/bsp/c610_m2006/test_c610_m2006_angle_step.c](../Core/Src/bsp/c610_m2006/test_c610_m2006_angle_step.c) 第 17 行；[Core/Src/bsp/gm6020/test_gm6020_angle_loop.c](../Core/Src/bsp/gm6020/test_gm6020_angle_loop.c) 第 63 行；[Core/Src/bsp/gm6020/test_gm6020_calibration.c](../Core/Src/bsp/gm6020/test_gm6020_calibration.c) 第 52 行；[Core/Src/bsp/snail_2305/test_snail_2305.c](../Core/Src/bsp/snail_2305/test_snail_2305.c) 第 62 行；[Core/Src/task/task_chassis/task_chassis_runtime.c](../Core/Src/task/task_chassis/task_chassis_runtime.c) 第 52 行；[Core/Src/task/task_dbus/task_dbus_runtime.c](../Core/Src/task/task_dbus/task_dbus_runtime.c) 第 63 行；[Core/Src/task/task_dbus/task_dbus_trace.c](../Core/Src/task/task_dbus/task_dbus_trace.c) 第 18 行；[Core/Src/task/task_feed_motor/task_feed_motor_runtime.c](../Core/Src/task/task_feed_motor/task_feed_motor_runtime.c) 第 24 行；[Core/Src/task/task_ins/task_ins_runtime.c](../Core/Src/task/task_ins/task_ins_runtime.c) 第 280 行；[Core/Src/task/task_pitch/task_pitch_runtime.c](../Core/Src/task/task_pitch/task_pitch_runtime.c) 第 125 行；[Core/Src/task/task_yaw/task_yaw_runtime.c](../Core/Src/task/task_yaw/task_yaw_runtime.c) 第 124 行。

### LOG_USART1_ENABLE（第 18 行定义）

```c
#define LOG_USART1_ENABLE 1U /* USART1 输出开关，0/1；不改变 DBUS 接收。 */
```

编译期日志开关/图表前缀，仅控制诊断，不控制电机使能。

源文件：[Core/Inc/app/log/log_config.h](../Core/Inc/app/log/log_config.h)。

Core引用：[Core/Inc/app/log/log.h](../Core/Inc/app/log/log.h) 第 56 行；[Core/Inc/app/log/log_config.h](../Core/Inc/app/log/log_config.h) 第 61 行；[Core/Src/app/log/log.c](../Core/Src/app/log/log.c) 第 17 行；[Core/Src/bsp/c610_m2006/test_c610_m2006_angle_step.c](../Core/Src/bsp/c610_m2006/test_c610_m2006_angle_step.c) 第 17 行；[Core/Src/bsp/gm6020/test_gm6020_angle_loop.c](../Core/Src/bsp/gm6020/test_gm6020_angle_loop.c) 第 63 行；[Core/Src/bsp/gm6020/test_gm6020_calibration.c](../Core/Src/bsp/gm6020/test_gm6020_calibration.c) 第 52 行；[Core/Src/bsp/snail_2305/test_snail_2305.c](../Core/Src/bsp/snail_2305/test_snail_2305.c) 第 62 行；[Core/Src/task/task_chassis/task_chassis_runtime.c](../Core/Src/task/task_chassis/task_chassis_runtime.c) 第 52 行；[Core/Src/task/task_dbus/task_dbus_runtime.c](../Core/Src/task/task_dbus/task_dbus_runtime.c) 第 63 行；[Core/Src/task/task_dbus/task_dbus_trace.c](../Core/Src/task/task_dbus/task_dbus_trace.c) 第 18 行；[Core/Src/task/task_feed_motor/task_feed_motor_runtime.c](../Core/Src/task/task_feed_motor/task_feed_motor_runtime.c) 第 24 行；[Core/Src/task/task_ins/task_ins_runtime.c](../Core/Src/task/task_ins/task_ins_runtime.c) 第 280 行；[Core/Src/task/task_pitch/task_pitch_runtime.c](../Core/Src/task/task_pitch/task_pitch_runtime.c) 第 125 行；[Core/Src/task/task_yaw/task_yaw_runtime.c](../Core/Src/task/task_yaw/task_yaw_runtime.c) 第 124 行。

### LOG_YAW_ENABLE（第 21 行定义）

```c
#define LOG_YAW_ENABLE 0U /* Yaw 日志开关，0/1；关闭本轴正式、标定和测试日志。 */
```

编译期日志开关/图表前缀，仅控制诊断，不控制电机使能。

源文件：[Core/Inc/app/log/log_config.h](../Core/Inc/app/log/log_config.h)。

Core引用：[Core/Inc/app/log/log.h](../Core/Inc/app/log/log.h) 第 45 行；[Core/Inc/app/log/log_config.h](../Core/Inc/app/log/log_config.h) 第 62 行；[Core/Src/bsp/gm6020/test_gm6020_angle_loop.c](../Core/Src/bsp/gm6020/test_gm6020_angle_loop.c) 第 63 行；[Core/Src/bsp/gm6020/test_gm6020_calibration.c](../Core/Src/bsp/gm6020/test_gm6020_calibration.c) 第 52 行；[Core/Src/task/task_yaw/task_yaw_runtime.c](../Core/Src/task/task_yaw/task_yaw_runtime.c) 第 124 行。

### LOG_PITCH_ENABLE（第 24 行定义）

```c
#define LOG_PITCH_ENABLE 0U /* Pitch 日志开关，0/1；关闭本轴正式、标定和测试日志。 */
```

编译期日志开关/图表前缀，仅控制诊断，不控制电机使能。

源文件：[Core/Inc/app/log/log_config.h](../Core/Inc/app/log/log_config.h)。

Core引用：[Core/Inc/app/log/log.h](../Core/Inc/app/log/log.h) 第 46 行；[Core/Inc/app/log/log_config.h](../Core/Inc/app/log/log_config.h) 第 62 行；[Core/Src/bsp/gm6020/test_gm6020_angle_loop.c](../Core/Src/bsp/gm6020/test_gm6020_angle_loop.c) 第 63 行；[Core/Src/bsp/gm6020/test_gm6020_calibration.c](../Core/Src/bsp/gm6020/test_gm6020_calibration.c) 第 52 行；[Core/Src/task/task_pitch/task_pitch_runtime.c](../Core/Src/task/task_pitch/task_pitch_runtime.c) 第 125 行。

### LOG_FEED_MOTOR_ENABLE（第 27 行定义）

```c
#define LOG_FEED_MOTOR_ENABLE 1U /* 供弹日志开关，0/1；关闭初始化、正式和 C610 测试日志。 */
```

编译期日志开关/图表前缀，仅控制诊断，不控制电机使能。

源文件：[Core/Inc/app/log/log_config.h](../Core/Inc/app/log/log_config.h)。

Core引用：[Core/Inc/app/log/log.h](../Core/Inc/app/log/log.h) 第 47 行；[Core/Inc/app/log/log_config.h](../Core/Inc/app/log/log_config.h) 第 63 行；[Core/Src/bsp/c610_m2006/test_c610_m2006_angle_step.c](../Core/Src/bsp/c610_m2006/test_c610_m2006_angle_step.c) 第 17 行；[Core/Src/bsp/snail_2305/test_snail_2305.c](../Core/Src/bsp/snail_2305/test_snail_2305.c) 第 62 行；[Core/Src/task/task_feed_motor/task_feed_motor_runtime.c](../Core/Src/task/task_feed_motor/task_feed_motor_runtime.c) 第 24 行。

### LOG_CHASSIS_ENABLE（第 30 行定义）

```c
#define LOG_CHASSIS_ENABLE 0U /* 底盘日志开关，0/1；与 TASK 同开，默认关闭以保留串口带宽。 */
```

编译期日志开关/图表前缀，仅控制诊断，不控制电机使能。

源文件：[Core/Inc/app/log/log_config.h](../Core/Inc/app/log/log_config.h)。

Core引用：[Core/Inc/app/log/log.h](../Core/Inc/app/log/log.h) 第 42 行；[Core/Inc/app/log/log_config.h](../Core/Inc/app/log/log_config.h) 第 65 行；[Core/Src/task/task_chassis/task_chassis_runtime.c](../Core/Src/task/task_chassis/task_chassis_runtime.c) 第 52 行。

### LOG_INS_ENABLE（第 33 行定义）

```c
#define LOG_INS_ENABLE 1U /* 姿态状态日志，0/1；独立于TASK，当前开启1Hz六轴状态。 */
```

编译期日志开关/图表前缀，仅控制诊断，不控制电机使能。

源文件：[Core/Inc/app/log/log_config.h](../Core/Inc/app/log/log_config.h)。

Core引用：[Core/Inc/app/log/log.h](../Core/Inc/app/log/log.h) 第 41 行；[Core/Inc/app/log/log_config.h](../Core/Inc/app/log/log_config.h) 第 67 行；[Core/Src/task/task_ins/task_ins_runtime.c](../Core/Src/task/task_ins/task_ins_runtime.c) 第 280 行。

### LOG_INS_MAG_DIAGNOSTIC_ENABLE（第 36 行定义）

```c
#define LOG_INS_MAG_DIAGNOSTIC_ENABLE 0U /* 磁航向诊断，0/1；当前六轴自稳关闭，1随正常INS日志附加磁场/四元数。 */
```

编译期日志开关/图表前缀，仅控制诊断，不控制电机使能。

源文件：[Core/Inc/app/log/log_config.h](../Core/Inc/app/log/log_config.h)。

Core引用：[Core/Inc/app/log/log_config.h](../Core/Inc/app/log/log_config.h) 第 68 行；[Core/Src/task/task_ins/task_ins_runtime.c](../Core/Src/task/task_ins/task_ins_runtime.c) 第 318 行。

### LOG_INS_MAG_CALIBRATION_ENABLE（第 39 行定义）

```c
#define LOG_INS_MAG_CALIBRATION_ENABLE 0U /* MotionCal模式，0/1；1需同时开启磁采集，仅输出Raw帧并暂停磁融合。 */
```

0/1磁标定模式；1时分类独占MotionCal Raw帧并暂禁磁反馈，不暂停电机任务；需日志总门/USART1门及115200/8N1。

源文件：[Core/Inc/app/log/log_config.h](../Core/Inc/app/log/log_config.h)。

Core引用：[Core/Inc/app/log/log.h](../Core/Inc/app/log/log.h) 第 35 行；[Core/Inc/app/log/log_config.h](../Core/Inc/app/log/log_config.h) 第 67 行；[Core/Src/task/task_dbus/task_dbus_trace.c](../Core/Src/task/task_dbus/task_dbus_trace.c) 第 18 行；[Core/Src/task/task_ins/task_ins_runtime.c](../Core/Src/task/task_ins/task_ins_runtime.c) 第 96 行。

### LOG_TASK_ENABLE（第 42 行定义）

```c
#define LOG_TASK_ENABLE 0U /* 正式任务日志总开关，0/1；与各电机开关同时开启才输出。 */
```

编译期日志开关/图表前缀，仅控制诊断，不控制电机使能。

源文件：[Core/Inc/app/log/log_config.h](../Core/Inc/app/log/log_config.h)。

Core引用：[Core/Inc/app/log/log.h](../Core/Inc/app/log/log.h) 第 42 行；[Core/Inc/app/log/log_config.h](../Core/Inc/app/log/log_config.h) 第 63 行；[Core/Src/task/task_chassis/task_chassis_runtime.c](../Core/Src/task/task_chassis/task_chassis_runtime.c) 第 52 行；[Core/Src/task/task_feed_motor/task_feed_motor_runtime.c](../Core/Src/task/task_feed_motor/task_feed_motor_runtime.c) 第 24 行；[Core/Src/task/task_pitch/task_pitch_runtime.c](../Core/Src/task/task_pitch/task_pitch_runtime.c) 第 125 行；[Core/Src/task/task_yaw/task_yaw_runtime.c](../Core/Src/task/task_yaw/task_yaw_runtime.c) 第 124 行。

### LOG_TEST_ENABLE（第 45 行定义）

```c
#define LOG_TEST_ENABLE 1U /* 硬件测试日志总开关，0/1；与各电机开关同时开启才输出。 */
```

编译期正式/测试路径选择，模式不在运行中切换；互斥限制见源#if。

源文件：[Core/Inc/app/log/log_config.h](../Core/Inc/app/log/log_config.h)。

Core引用：[Core/Inc/app/log/log.h](../Core/Inc/app/log/log.h) 第 48 行；[Core/Inc/app/log/log_config.h](../Core/Inc/app/log/log_config.h) 第 64 行；[Core/Src/bsp/c610_m2006/test_c610_m2006_angle_step.c](../Core/Src/bsp/c610_m2006/test_c610_m2006_angle_step.c) 第 17 行；[Core/Src/bsp/gm6020/test_gm6020_angle_loop.c](../Core/Src/bsp/gm6020/test_gm6020_angle_loop.c) 第 63 行；[Core/Src/bsp/gm6020/test_gm6020_calibration.c](../Core/Src/bsp/gm6020/test_gm6020_calibration.c) 第 52 行；[Core/Src/bsp/snail_2305/test_snail_2305.c](../Core/Src/bsp/snail_2305/test_snail_2305.c) 第 62 行。

### LOG_DBUS_TEXT_ENABLE（第 48 行定义）

```c
#define LOG_DBUS_TEXT_ENABLE 0U /* DBUS 文本开关，0/1；输出原始数据和接收状态。 */
```

编译期日志开关/图表前缀，仅控制诊断，不控制电机使能。

源文件：[Core/Inc/app/log/log_config.h](../Core/Inc/app/log/log_config.h)。

Core引用：[Core/Inc/app/log/log.h](../Core/Inc/app/log/log.h) 第 43 行；[Core/Inc/app/log/log_config.h](../Core/Inc/app/log/log_config.h) 第 64 行；[Core/Src/task/task_dbus/task_dbus_runtime.c](../Core/Src/task/task_dbus/task_dbus_runtime.c) 第 63 行。

### LOG_DBUS_TRACE_ENABLE（第 51 行定义）

```c
#define LOG_DBUS_TRACE_ENABLE 0U /* 输入逐帧采集，0/1；1独占日志分类，控制任务继续运行。 */
```

0/1输入采集分类开关；开启时独占日志带宽，MotionCal优先；不暂停设备控制。

源文件：[Core/Inc/app/log/log_config.h](../Core/Inc/app/log/log_config.h)。

Core引用：[Core/Inc/app/log/log.h](../Core/Inc/app/log/log.h) 第 37 行；[Core/Inc/app/log/log_config.h](../Core/Inc/app/log/log_config.h) 第 66 行；[Core/Src/task/task_dbus/task_dbus_trace.c](../Core/Src/task/task_dbus/task_dbus_trace.c) 第 18 行。

### LOG_DBUS_CHART_ENABLE（第 54 行定义）

```c
#define LOG_DBUS_CHART_ENABLE 0U /* DBUS 图表开关，0/1；输出 10 通道 ch: 帧。 */
```

编译期日志开关/图表前缀，仅控制诊断，不控制电机使能。

源文件：[Core/Inc/app/log/log_config.h](../Core/Inc/app/log/log_config.h)。

Core引用：[Core/Inc/app/log/log.h](../Core/Inc/app/log/log.h) 第 44 行；[Core/Inc/app/log/log_config.h](../Core/Inc/app/log/log_config.h) 第 65 行；[Core/Src/task/task_dbus/task_dbus_runtime.c](../Core/Src/task/task_dbus/task_dbus_runtime.c) 第 63 行。

### LOG_CHART_PREFIX（第 57 行定义）

```c
#define LOG_CHART_PREFIX "ch:" /* 图表前缀，1~16 字节；与上位机一致，不含逗号、换行或 %。 */
```

编译期日志开关/图表前缀，仅控制诊断，不控制电机使能。

源文件：[Core/Inc/app/log/log_config.h](../Core/Inc/app/log/log_config.h)。

Core引用：[Core/Src/task/task_dbus/task_dbus_runtime.c](../Core/Src/task/task_dbus/task_dbus_runtime.c) 第 66 行。

### LOG_SWITCH_VALID（第 60 行定义）

```c
#define LOG_SWITCH_VALID(value) ((value) == 0U || (value) == 1U) /* 开关只接受 0/1，拒绝负数和时长。 */
```

分类位掩码、0/1开关校验和条件运算；关闭分类不求值格式参数。

源文件：[Core/Inc/app/log/log_config.h](../Core/Inc/app/log/log_config.h)。

Core引用：[Core/Inc/app/log/log_config.h](../Core/Inc/app/log/log_config.h) 第 61 行。

## Core/Inc/bsp/bmi088/bmi088.h

### BSP_BMI088_H（第 8 行定义）

```c
#define BSP_BMI088_H /* 防止重复包含。 */
```

包含保护：避免同一翻译单元重复声明，没有控制算法。

源文件：[Core/Inc/bsp/bmi088/bmi088.h](../Core/Inc/bsp/bmi088/bmi088.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

## Core/Inc/bsp/c610_m2006/c610_m2006.h

### C610_M2006_H（第 12 行定义）

```c
#define C610_M2006_H /* 防止重复包含。避免定义重复。 */
```

包含保护：避免同一翻译单元重复声明，没有控制算法。

源文件：[Core/Inc/bsp/c610_m2006/c610_m2006.h](../Core/Inc/bsp/c610_m2006/c610_m2006.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### C610_M2006_FRAME_DLC（第 30 行定义）

```c
#define C610_M2006_FRAME_DLC 8U /* C610 反馈和聚合控制帧固定为 8 字节。DATA[0]~DATA[7]。 */
```

CAN帧8字节/DBUS帧18字节长度检查、数组和DMA传输长度。

源文件：[Core/Inc/bsp/c610_m2006/c610_m2006.h](../Core/Inc/bsp/c610_m2006/c610_m2006.h)。

Core引用：[Core/Inc/bsp/c610_m2006/c610_m2006.h](../Core/Inc/bsp/c610_m2006/c610_m2006.h) 第 144 行；[Core/Src/bsp/c610_m2006/c610_m2006.c](../Core/Src/bsp/c610_m2006/c610_m2006.c) 第 80 行。

### C610_M2006_MAX_DEVICE_COUNT（第 31 行定义）

```c
#define C610_M2006_MAX_DEVICE_COUNT 8U /* 静态注册表最多保存 8 个电调句柄。对应协议 ID 1~8。 */
```

协议编号/静态注册表容量，重复注册或非法编号拒绝。

源文件：[Core/Inc/bsp/c610_m2006/c610_m2006.h](../Core/Inc/bsp/c610_m2006/c610_m2006.h)。

Core引用：[Core/Src/bsp/c610_m2006/c610_m2006.c](../Core/Src/bsp/c610_m2006/c610_m2006.c) 第 158 行。

### C610_M2006_MIN_DEVICE_ID（第 32 行定义）

```c
#define C610_M2006_MIN_DEVICE_ID 1U /* RoboMaster 电调编号从 1 开始。0 不是有效电机。 */
```

协议编号/静态注册表容量，重复注册或非法编号拒绝。

源文件：[Core/Inc/bsp/c610_m2006/c610_m2006.h](../Core/Inc/bsp/c610_m2006/c610_m2006.h)。

Core引用：[Core/Src/bsp/c610_m2006/c610_m2006.c](../Core/Src/bsp/c610_m2006/c610_m2006.c) 第 186 行。

### C610_M2006_MAX_DEVICE_ID（第 33 行定义）

```c
#define C610_M2006_MAX_DEVICE_ID 8U /* C610/M2006 支持的最大电调编号。超出范围拒绝注册。 */
```

协议编号/静态注册表容量，重复注册或非法编号拒绝。

源文件：[Core/Inc/bsp/c610_m2006/c610_m2006.h](../Core/Inc/bsp/c610_m2006/c610_m2006.h)。

Core引用：[Core/Src/bsp/c610_m2006/c610_m2006.c](../Core/Src/bsp/c610_m2006/c610_m2006.c) 第 187 行。

### C610_M2006_CONTROL_ID_LOW（第 36 行定义）

```c
#define C610_M2006_CONTROL_ID_LOW 0x200U /* ID1~4 使用的聚合控制帧。DATA[0:7] 分别对应 1~4。 */
```

CAN路由/槽位；GM6020反馈0x204+ID，C610反馈0x200+ID。

源文件：[Core/Inc/bsp/c610_m2006/c610_m2006.h](../Core/Inc/bsp/c610_m2006/c610_m2006.h)。

Core引用：[Core/Src/bsp/c610_m2006/c610_m2006.c](../Core/Src/bsp/c610_m2006/c610_m2006.c) 第 529 行。

### C610_M2006_CONTROL_ID_HIGH（第 37 行定义）

```c
#define C610_M2006_CONTROL_ID_HIGH 0x1FFU /* ID5~8 使用的聚合控制帧。DATA[0:7] 分别对应 5~8。 */
```

CAN路由/槽位；GM6020反馈0x204+ID，C610反馈0x200+ID。

源文件：[Core/Inc/bsp/c610_m2006/c610_m2006.h](../Core/Inc/bsp/c610_m2006/c610_m2006.h)。

Core引用：[Core/Src/bsp/c610_m2006/c610_m2006.c](../Core/Src/bsp/c610_m2006/c610_m2006.c) 第 534 行。

### C610_M2006_FEEDBACK_ID_BASE（第 40 行定义）

```c
#define C610_M2006_FEEDBACK_ID_BASE 0x200U /* 反馈 ID = 0x200 + 电调 ID。接收时按此公式匹配。 */
```

CAN路由/槽位；GM6020反馈0x204+ID，C610反馈0x200+ID。

源文件：[Core/Inc/bsp/c610_m2006/c610_m2006.h](../Core/Inc/bsp/c610_m2006/c610_m2006.h)。

Core引用：[Core/Src/bsp/c610_m2006/c610_m2006.c](../Core/Src/bsp/c610_m2006/c610_m2006.c) 第 405 行。

### C610_M2006_ENCODER_COUNTS_PER_REV（第 41 行定义）

```c
#define C610_M2006_ENCODER_COUNTS_PER_REV 8192U /* 反馈机械角度范围 0~8191，对应一圈。13 位计数。 */
```

每圈编码器count：最短回绕阈值为一半，rpm换count/s用counts/60。

源文件：[Core/Inc/bsp/c610_m2006/c610_m2006.h](../Core/Inc/bsp/c610_m2006/c610_m2006.h)。

Core引用：[Core/Src/bsp/c610_m2006/c610_m2006.c](../Core/Src/bsp/c610_m2006/c610_m2006.c) 第 427 行。

### C610_M2006_CURRENT_RAW_MIN（第 48 行定义）

```c
#define C610_M2006_CURRENT_RAW_MIN (-10000) /* M2006/C610 目标电流原始值下限。协议量，不是安培。 */
```

电流raw：C610协议±10000，GM6020±16384；任务进一步限流或指定测试电流。

源文件：[Core/Inc/bsp/c610_m2006/c610_m2006.h](../Core/Inc/bsp/c610_m2006/c610_m2006.h)。

Core引用：[Core/Src/bsp/c610_m2006/c610_m2006.c](../Core/Src/bsp/c610_m2006/c610_m2006.c) 第 349 行。

### C610_M2006_CURRENT_RAW_MAX（第 49 行定义）

```c
#define C610_M2006_CURRENT_RAW_MAX 10000 /* M2006/C610 目标电流原始值上限。协议量，不是安培。 */
```

电流raw：C610协议±10000，GM6020±16384；任务进一步限流或指定测试电流。

源文件：[Core/Inc/bsp/c610_m2006/c610_m2006.h](../Core/Inc/bsp/c610_m2006/c610_m2006.h)。

Core引用：[Core/Src/bsp/c610_m2006/c610_m2006.c](../Core/Src/bsp/c610_m2006/c610_m2006.c) 第 352 行。

### C610_M2006_FEEDBACK_TIMEOUT_MS（第 50 行定义）

```c
#define C610_M2006_FEEDBACK_TIMEOUT_MS 100U /* 超过 100 ms 无反馈即视为离线。任务必须发零。 */
```

HAL同源年龄≥期限离线，先检查真实首帧；发送前重新判断反馈新鲜度。

源文件：[Core/Inc/bsp/c610_m2006/c610_m2006.h](../Core/Inc/bsp/c610_m2006/c610_m2006.h)。

Core引用：[Core/Src/bsp/c610_m2006/c610_m2006.c](../Core/Src/bsp/c610_m2006/c610_m2006.c) 第 292 行；[Core/Src/task/task_feed_motor/task_feed_motor.c](../Core/Src/task/task_feed_motor/task_feed_motor.c) 第 52 行；[Core/Src/task/task_feed_motor/task_feed_motor_runtime.c](../Core/Src/task/task_feed_motor/task_feed_motor_runtime.c) 第 93 行。

## Core/Inc/bsp/c610_m2006/test_c610_m2006_angle_step.h

### TEST_C610_M2006_ANGLE_STEP_H（第 12 行定义）

```c
#define TEST_C610_M2006_ANGLE_STEP_H /* 防止 C610 角度步长测试接口重复包含。 */
```

包含保护：避免同一翻译单元重复声明，没有控制算法。

源文件：[Core/Inc/bsp/c610_m2006/test_c610_m2006_angle_step.h](../Core/Inc/bsp/c610_m2006/test_c610_m2006_angle_step.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

## Core/Inc/bsp/dbus/dbus.h

### DBUS_H（第 12 行定义）

```c
#define DBUS_H /* 防止 DBUS 接口重复包含。避免类型重复定义。 */
```

包含保护：避免同一翻译单元重复声明，没有控制算法。

源文件：[Core/Inc/bsp/dbus/dbus.h](../Core/Inc/bsp/dbus/dbus.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### DBUS_FRAME_LENGTH（第 16 行定义）

```c
#define DBUS_FRAME_LENGTH 18U /* DJI DBUS 固定帧长，单位字节。9 字节半帧不能解码。 */
```

CAN帧8字节/DBUS帧18字节长度检查、数组和DMA传输长度。

源文件：[Core/Inc/bsp/dbus/dbus.h](../Core/Inc/bsp/dbus/dbus.h)。

Core引用：[Core/Src/bsp/dbus/dbus.c](../Core/Src/bsp/dbus/dbus.c) 第 19 行。

### DBUS_CHANNEL_CENTER（第 17 行定义）

```c
#define DBUS_CHANNEL_CENTER 1024 /* 官方通道中心原始值。减去它后回中为零。 */
```

11位通道解包后减1024，得到有符号摇杆偏移。

源文件：[Core/Inc/bsp/dbus/dbus.h](../Core/Inc/bsp/dbus/dbus.h)。

Core引用：[Core/Src/bsp/dbus/dbus.c](../Core/Src/bsp/dbus/dbus.c) 第 63 行。

### DBUS_CHANNEL_SPAN（第 18 行定义）

```c
#define DBUS_CHANNEL_SPAN 660 /* 官方摇杆满量程偏移，单位原始计数。364~1684 对应 ±660。 */
```

摇杆范围校验±660，raw×1000/660转换为千分比。

源文件：[Core/Inc/bsp/dbus/dbus.h](../Core/Inc/bsp/dbus/dbus.h)。

Core引用：[Core/Src/bsp/dbus/dbus.c](../Core/Src/bsp/dbus/dbus.c) 第 86 行；[Core/Src/task/task_dbus/task_dbus_runtime.c](../Core/Src/task/task_dbus/task_dbus_runtime.c) 第 26 行。

### DBUS_OFFLINE_TIMEOUT_MS（第 19 行定义）

```c
#define DBUS_OFFLINE_TIMEOUT_MS 100U /* 有效帧过期时间，单位 HAL ms。坏帧不能续期。 */
```

HAL同源年龄≥期限离线，先检查真实首帧；发送前重新判断反馈新鲜度。

源文件：[Core/Inc/bsp/dbus/dbus.h](../Core/Inc/bsp/dbus/dbus.h)。

Core引用：[Core/Src/bsp/dbus/dbus.c](../Core/Src/bsp/dbus/dbus.c) 第 371 行。

### DBUS_RETRY_PERIOD_MS（第 20 行定义）

```c
#define DBUS_RETRY_PERIOD_MS 20U /* DMA 启动失败后的重试间隔，单位 HAL ms。不忙等、不停住其它任务。 */
```

初始化/恢复失败节流，比较HAL年龄或转换Tick后等待。

源文件：[Core/Inc/bsp/dbus/dbus.h](../Core/Inc/bsp/dbus/dbus.h)。

Core引用：[Core/Src/bsp/dbus/dbus.c](../Core/Src/bsp/dbus/dbus.c) 第 286 行。

### DBUS_SWITCH_UP（第 21 行定义）

```c
#define DBUS_SWITCH_UP 1U /* 官方拨杆上档编码。不能按自然顺序猜数值。 */
```

拨杆编码上/中/下=1/3/2，当前不参与使能。

源文件：[Core/Inc/bsp/dbus/dbus.h](../Core/Inc/bsp/dbus/dbus.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### DBUS_SWITCH_MIDDLE（第 22 行定义）

```c
#define DBUS_SWITCH_MIDDLE 3U /* 官方拨杆中档编码。中档是 3，不是 2。 */
```

拨杆编码上/中/下=1/3/2，当前不参与使能。

源文件：[Core/Inc/bsp/dbus/dbus.h](../Core/Inc/bsp/dbus/dbus.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### DBUS_SWITCH_DOWN（第 23 行定义）

```c
#define DBUS_SWITCH_DOWN 2U /* 官方拨杆下档编码；只解码，不参与输出使能。 */
```

拨杆编码上/中/下=1/3/2，当前不参与使能。

源文件：[Core/Inc/bsp/dbus/dbus.h](../Core/Inc/bsp/dbus/dbus.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

## Core/Inc/bsp/gm6020/gm6020.h

### GM6020_H（第 12 行定义）

```c
#define GM6020_H /* 防止重复包含。避免类型和宏重复定义。 */
```

包含保护：避免同一翻译单元重复声明，没有控制算法。

源文件：[Core/Inc/bsp/gm6020/gm6020.h](../Core/Inc/bsp/gm6020/gm6020.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### GM6020_FRAME_DLC（第 26 行定义）

```c
#define GM6020_FRAME_DLC 8U /* GM6020 反馈/控制 CAN 帧固定 8 字节。DATA[0]~DATA[7]。 */
```

CAN帧8字节/DBUS帧18字节长度检查、数组和DMA传输长度。

源文件：[Core/Inc/bsp/gm6020/gm6020.h](../Core/Inc/bsp/gm6020/gm6020.h)。

Core引用：[Core/Inc/bsp/gm6020/gm6020.h](../Core/Inc/bsp/gm6020/gm6020.h) 第 174 行；[Core/Src/bsp/gm6020/gm6020.c](../Core/Src/bsp/gm6020/gm6020.c) 第 288 行。

### GM6020_FEEDBACK_ID_YAW（第 27 行定义）

```c
#define GM6020_FEEDBACK_ID_YAW 0x205U /* Yaw 默认反馈标准帧 ID。对应电机 ID 1。 */
```

CAN路由/槽位；GM6020反馈0x204+ID，C610反馈0x200+ID。

源文件：[Core/Inc/bsp/gm6020/gm6020.h](../Core/Inc/bsp/gm6020/gm6020.h)。

Core引用：[Core/Src/task/task_yaw/task_yaw_runtime.c](../Core/Src/task/task_yaw/task_yaw_runtime.c) 第 36 行。

### GM6020_FEEDBACK_ID_PITCH（第 28 行定义）

```c
#define GM6020_FEEDBACK_ID_PITCH 0x206U /* Pitch 默认反馈标准帧 ID。对应电机 ID 2。 */
```

CAN路由/槽位；GM6020反馈0x204+ID，C610反馈0x200+ID。

源文件：[Core/Inc/bsp/gm6020/gm6020.h](../Core/Inc/bsp/gm6020/gm6020.h)。

Core引用：[Core/Src/task/task_pitch/task_pitch_runtime.c](../Core/Src/task/task_pitch/task_pitch_runtime.c) 第 36 行。

### GM6020_FEEDBACK_ID_MIN（第 29 行定义）

```c
#define GM6020_FEEDBACK_ID_MIN 0x205U /* 驱动接受的最小 GM6020 反馈 ID。电机 ID 1。 */
```

CAN路由/槽位；GM6020反馈0x204+ID，C610反馈0x200+ID。

源文件：[Core/Inc/bsp/gm6020/gm6020.h](../Core/Inc/bsp/gm6020/gm6020.h)。

Core引用：[Core/Src/bsp/gm6020/gm6020.c](../Core/Src/bsp/gm6020/gm6020.c) 第 165 行。

### GM6020_FEEDBACK_ID_MAX（第 30 行定义）

```c
#define GM6020_FEEDBACK_ID_MAX 0x20BU /* 手册规定的最大反馈 ID，电机 ID 7。闭区间。 */
```

CAN路由/槽位；GM6020反馈0x204+ID，C610反馈0x200+ID。

源文件：[Core/Inc/bsp/gm6020/gm6020.h](../Core/Inc/bsp/gm6020/gm6020.h)。

Core引用：[Core/Src/bsp/gm6020/gm6020.c](../Core/Src/bsp/gm6020/gm6020.c) 第 166 行。

### GM6020_CURRENT_CONTROL_ID_LOW（第 31 行定义）

```c
#define GM6020_CURRENT_CONTROL_ID_LOW 0x1FEU /* 电流控制帧：电机 ID 1~4。每个占两个字节。 */
```

CAN路由/槽位；GM6020反馈0x204+ID，C610反馈0x200+ID。

源文件：[Core/Inc/bsp/gm6020/gm6020.h](../Core/Inc/bsp/gm6020/gm6020.h)。

Core引用：[Core/Src/bsp/gm6020/gm6020.c](../Core/Src/bsp/gm6020/gm6020.c) 第 382 行。

### GM6020_CURRENT_CONTROL_ID_HIGH（第 32 行定义）

```c
#define GM6020_CURRENT_CONTROL_ID_HIGH 0x2FEU /* 电流控制帧：电机 ID 5~7。每个占两个字节。 */
```

CAN路由/槽位；GM6020反馈0x204+ID，C610反馈0x200+ID。

源文件：[Core/Inc/bsp/gm6020/gm6020.h](../Core/Inc/bsp/gm6020/gm6020.h)。

Core引用：[Core/Src/bsp/gm6020/gm6020.c](../Core/Src/bsp/gm6020/gm6020.c) 第 395 行。

### GM6020_VOLTAGE_CONTROL_ID_LOW（第 33 行定义）

```c
#define GM6020_VOLTAGE_CONTROL_ID_LOW 0x1FFU /* 电压控制帧：电机 ID 1~4。本驱动不发送。当前使用电流环。 */
```

CAN路由/槽位；GM6020反馈0x204+ID，C610反馈0x200+ID。

源文件：[Core/Inc/bsp/gm6020/gm6020.h](../Core/Inc/bsp/gm6020/gm6020.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### GM6020_VOLTAGE_CONTROL_ID_HIGH（第 34 行定义）

```c
#define GM6020_VOLTAGE_CONTROL_ID_HIGH 0x2FFU /* 电压控制帧：电机 ID 5~7。本驱动不发送。避免误切换控制模式。 */
```

CAN路由/槽位；GM6020反馈0x204+ID，C610反馈0x200+ID。

源文件：[Core/Inc/bsp/gm6020/gm6020.h](../Core/Inc/bsp/gm6020/gm6020.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### GM6020_ENCODER_COUNTS_PER_REV（第 35 行定义）

```c
#define GM6020_ENCODER_COUNTS_PER_REV 8192U /* 单圈编码器计数，13 位范围。有效值 0~8191。 */
```

每圈编码器count：最短回绕阈值为一半，rpm换count/s用counts/60。

源文件：[Core/Inc/bsp/gm6020/gm6020.h](../Core/Inc/bsp/gm6020/gm6020.h)。

Core引用：[Core/Inc/app/log/gimbal_log.h](../Core/Inc/app/log/gimbal_log.h) 第 46 行；[Core/Src/app/gimbal/gimbal_axis.c](../Core/Src/app/gimbal/gimbal_axis.c) 第 120 行；[Core/Src/app/gimbal/gimbal_control.c](../Core/Src/app/gimbal/gimbal_control.c) 第 237 行；[Core/Src/bsp/gm6020/gm6020.c](../Core/Src/bsp/gm6020/gm6020.c) 第 94 行；[Core/Src/bsp/gm6020/test_gm6020_angle_loop.c](../Core/Src/bsp/gm6020/test_gm6020_angle_loop.c) 第 59 行。

### GM6020_CURRENT_RAW_MIN（第 36 行定义）

```c
#define GM6020_CURRENT_RAW_MIN (-16384) /* 手册电流给定原始值下限，对应约 -3 A。控制仍使用原始值。 */
```

电流raw：C610协议±10000，GM6020±16384；任务进一步限流或指定测试电流。

源文件：[Core/Inc/bsp/gm6020/gm6020.h](../Core/Inc/bsp/gm6020/gm6020.h)。

Core引用：[Core/Src/bsp/gm6020/gm6020.c](../Core/Src/bsp/gm6020/gm6020.c) 第 219 行。

### GM6020_CURRENT_RAW_MAX（第 37 行定义）

```c
#define GM6020_CURRENT_RAW_MAX 16384 /* 手册电流给定原始值上限，对应约 +3 A。控制仍使用原始值。 */
```

电流raw：C610协议±10000，GM6020±16384；任务进一步限流或指定测试电流。

源文件：[Core/Inc/bsp/gm6020/gm6020.h](../Core/Inc/bsp/gm6020/gm6020.h)。

Core引用：[Core/Src/app/gimbal/gimbal_axis.c](../Core/Src/app/gimbal/gimbal_axis.c) 第 84 行；[Core/Src/bsp/gm6020/gm6020.c](../Core/Src/bsp/gm6020/gm6020.c) 第 220 行。

### GM6020_FEEDBACK_TIMEOUT_MS（第 38 行定义）

```c
#define GM6020_FEEDBACK_TIMEOUT_MS 100U /* 超过该 HAL 毫秒数没有反馈即离线。任务必须走零输出。 */
```

HAL同源年龄≥期限离线，先检查真实首帧；发送前重新判断反馈新鲜度。

源文件：[Core/Inc/bsp/gm6020/gm6020.h](../Core/Inc/bsp/gm6020/gm6020.h)。

Core引用：[Core/Src/app/gimbal/gimbal_axis.c](../Core/Src/app/gimbal/gimbal_axis.c) 第 108 行；[Core/Src/bsp/gm6020/gm6020.c](../Core/Src/bsp/gm6020/gm6020.c) 第 177 行。

### GM6020_MAX_DEVICE_COUNT（第 39 行定义）

```c
#define GM6020_MAX_DEVICE_COUNT 7U /* 手册支持的有效电机 ID 1~7，静态表最多注册 7 个。不动态分配。 */
```

协议编号/静态注册表容量，重复注册或非法编号拒绝。

源文件：[Core/Inc/bsp/gm6020/gm6020.h](../Core/Inc/bsp/gm6020/gm6020.h)。

Core引用：[Core/Src/bsp/gm6020/gm6020.c](../Core/Src/bsp/gm6020/gm6020.c) 第 18 行。

### GM6020_ANGLE_LIMIT_DISABLED_MIN（第 42 行定义）

```c
#define GM6020_ANGLE_LIMIT_DISABLED_MIN INT32_MIN /* 禁用边界时使用的最小哨兵。表示不比较下限。 */
```

INT32极值哨兵代表几何边界禁用，不是实测挡块。

源文件：[Core/Inc/bsp/gm6020/gm6020.h](../Core/Inc/bsp/gm6020/gm6020.h)。

Core引用：[Core/Src/app/gimbal/gimbal_axis.c](../Core/Src/app/gimbal/gimbal_axis.c) 第 116 行；[Core/Src/bsp/gm6020/gm6020.c](../Core/Src/bsp/gm6020/gm6020.c) 第 111 行。

### GM6020_ANGLE_LIMIT_DISABLED_MAX（第 43 行定义）

```c
#define GM6020_ANGLE_LIMIT_DISABLED_MAX INT32_MAX /* 禁用边界时使用的最大哨兵。表示不比较上限。 */
```

INT32极值哨兵代表几何边界禁用，不是实测挡块。

源文件：[Core/Inc/bsp/gm6020/gm6020.h](../Core/Inc/bsp/gm6020/gm6020.h)。

Core引用：[Core/Src/app/gimbal/gimbal_axis.c](../Core/Src/app/gimbal/gimbal_axis.c) 第 118 行；[Core/Src/bsp/gm6020/gm6020.c](../Core/Src/bsp/gm6020/gm6020.c) 第 112 行。

## Core/Inc/bsp/gm6020/test_gm6020_angle_loop.h

### TEST_GM6020_ANGLE_LOOP_H（第 11 行定义）

```c
#define TEST_GM6020_ANGLE_LOOP_H /* 防止 GM6020 角度测试接口重复包含。避免结构体和声明重复。 */
```

包含保护：避免同一翻译单元重复声明，没有控制算法。

源文件：[Core/Inc/bsp/gm6020/test_gm6020_angle_loop.h](../Core/Inc/bsp/gm6020/test_gm6020_angle_loop.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

## Core/Inc/bsp/gm6020/test_gm6020_calibration.h

### TEST_GM6020_CALIBRATION_H（第 11 行定义）

```c
#define TEST_GM6020_CALIBRATION_H /* 防止 GM6020 标定接口重复包含。避免结构体和函数声明重复。 */
```

包含保护：避免同一翻译单元重复声明，没有控制算法。

源文件：[Core/Inc/bsp/gm6020/test_gm6020_calibration.h](../Core/Inc/bsp/gm6020/test_gm6020_calibration.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

## Core/Inc/bsp/ist8310/ist8310.h

### BSP_IST8310_H（第 7 行定义）

```c
#define BSP_IST8310_H /* 防止重复包含。 */
```

包含保护：避免同一翻译单元重复声明，没有控制算法。

源文件：[Core/Inc/bsp/ist8310/ist8310.h](../Core/Inc/bsp/ist8310/ist8310.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

## Core/Inc/bsp/m3508_c620/m3508_c620.h

### BSP_M3508_C620_H（第 9 行定义）

```c
#define BSP_M3508_C620_H /* 防止重复包含。 */
```

包含保护：避免同一翻译单元重复声明，没有控制算法。

源文件：[Core/Inc/bsp/m3508_c620/m3508_c620.h](../Core/Inc/bsp/m3508_c620/m3508_c620.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### C620_M3508_MAX_DEVICES（第 14 行定义）

```c
#define C620_M3508_MAX_DEVICES 8U /* 静态注册容量，个；支持电调 ID1~8。 */
```

静态注册容量8个；同总线ID与句柄地址不能重复。

源文件：[Core/Inc/bsp/m3508_c620/m3508_c620.h](../Core/Inc/bsp/m3508_c620/m3508_c620.h)。

Core引用：[Core/Src/bsp/m3508_c620/m3508_c620.c](../Core/Src/bsp/m3508_c620/m3508_c620.c) 第 12 行。

### C620_M3508_CURRENT_LIMIT_RAW（第 15 行定义）

```c
#define C620_M3508_CURRENT_LIMIT_RAW 16384 /* 官方电流指令绝对上限，raw，约 20 A。 */
```

C620协议电流raw边界±16384约20A；与底盘实际限流分开。

源文件：[Core/Inc/bsp/m3508_c620/m3508_c620.h](../Core/Inc/bsp/m3508_c620/m3508_c620.h)。

Core引用：[Core/Src/bsp/m3508_c620/m3508_c620.c](../Core/Src/bsp/m3508_c620/m3508_c620.c) 第 121 行。

### C620_M3508_FEEDBACK_BASE_ID（第 16 行定义）

```c
#define C620_M3508_FEEDBACK_BASE_ID 0x200U /* 标准反馈 ID=0x200+电调 ID。 */
```

C620标准反馈0x200+ID；ID1~4控制0x200、ID5~8控制0x1FF，每槽大端2字节。

源文件：[Core/Inc/bsp/m3508_c620/m3508_c620.h](../Core/Inc/bsp/m3508_c620/m3508_c620.h)。

Core引用：[Core/Src/bsp/m3508_c620/m3508_c620.c](../Core/Src/bsp/m3508_c620/m3508_c620.c) 第 145 行。

### C620_M3508_CONTROL_LOW_ID（第 17 行定义）

```c
#define C620_M3508_CONTROL_LOW_ID 0x200U /* ID1~4 的标准电流控制帧，每槽 2 字节。 */
```

C620标准反馈0x200+ID；ID1~4控制0x200、ID5~8控制0x1FF，每槽大端2字节。

源文件：[Core/Inc/bsp/m3508_c620/m3508_c620.h](../Core/Inc/bsp/m3508_c620/m3508_c620.h)。

Core引用：[Core/Src/bsp/m3508_c620/m3508_c620.c](../Core/Src/bsp/m3508_c620/m3508_c620.c) 第 191 行。

### C620_M3508_CONTROL_HIGH_ID（第 18 行定义）

```c
#define C620_M3508_CONTROL_HIGH_ID 0x1FFU /* ID5~8 的标准电流控制帧。 */
```

C620标准反馈0x200+ID；ID1~4控制0x200、ID5~8控制0x1FF，每槽大端2字节。

源文件：[Core/Inc/bsp/m3508_c620/m3508_c620.h](../Core/Inc/bsp/m3508_c620/m3508_c620.h)。

Core引用：[Core/Src/bsp/m3508_c620/m3508_c620.c](../Core/Src/bsp/m3508_c620/m3508_c620.c) 第 192 行。

### C620_M3508_FRAME_BYTES（第 19 行定义）

```c
#define C620_M3508_FRAME_BYTES 8U /* C620 标准数据帧长度，字节。 */
```

C620标准数据帧长度8字节；帧校验与负载数组使用。

源文件：[Core/Inc/bsp/m3508_c620/m3508_c620.h](../Core/Inc/bsp/m3508_c620/m3508_c620.h)。

Core引用：[Core/Src/bsp/m3508_c620/m3508_c620.c](../Core/Src/bsp/m3508_c620/m3508_c620.c) 第 144 行。

## Core/Inc/bsp/snail_2305/snail_2305.h

### SNAIL_2305_H（第 11 行定义）

```c
#define SNAIL_2305_H /* 防止重复包含。 */
```

包含保护：避免同一翻译单元重复声明，没有控制算法。

源文件：[Core/Inc/bsp/snail_2305/snail_2305.h](../Core/Inc/bsp/snail_2305/snail_2305.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### SNAIL_2305_MIN_PROTOCOL_PULSE_US（第 18 行定义）

```c
#define SNAIL_2305_MIN_PROTOCOL_PULSE_US 400U /* C615 协议最小脉宽，us；超出范围拒绝输出。 */
```

C615脉宽us；停止/活动/范围各有独立参数，1MHz时CCR数字等于us。

源文件：[Core/Inc/bsp/snail_2305/snail_2305.h](../Core/Inc/bsp/snail_2305/snail_2305.h)。

Core引用：[Core/Inc/bsp/snail_2305/test_snail_2305.h](../Core/Inc/bsp/snail_2305/test_snail_2305.h) 第 58 行；[Core/Src/bsp/snail_2305/snail_2305.c](../Core/Src/bsp/snail_2305/snail_2305.c) 第 21 行；[Core/Src/task/task_feed_motor/task_feed_motor_runtime.c](../Core/Src/task/task_feed_motor/task_feed_motor_runtime.c) 第 58 行。

### SNAIL_2305_MAX_PROTOCOL_PULSE_US（第 19 行定义）

```c
#define SNAIL_2305_MAX_PROTOCOL_PULSE_US 2200U /* C615 协议最大脉宽，us；超出范围拒绝输出。 */
```

C615脉宽us；停止/活动/范围各有独立参数，1MHz时CCR数字等于us。

源文件：[Core/Inc/bsp/snail_2305/snail_2305.h](../Core/Inc/bsp/snail_2305/snail_2305.h)。

Core引用：[Core/Inc/bsp/snail_2305/test_snail_2305.h](../Core/Inc/bsp/snail_2305/test_snail_2305.h) 第 61 行；[Core/Src/bsp/snail_2305/snail_2305.c](../Core/Src/bsp/snail_2305/snail_2305.c) 第 22 行；[Core/Src/task/task_feed_motor/task_feed_motor_runtime.c](../Core/Src/task/task_feed_motor/task_feed_motor_runtime.c) 第 59 行。

## Core/Inc/bsp/snail_2305/test_snail_2305.h

### TEST_SNAIL_2305_H（第 12 行定义）

```c
#define TEST_SNAIL_2305_H /* 防止 C615 测试接口重复包含。 */
```

包含保护：避免同一翻译单元重复声明，没有控制算法。

源文件：[Core/Inc/bsp/snail_2305/test_snail_2305.h](../Core/Inc/bsp/snail_2305/test_snail_2305.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### SNAIL_2305_TEST_ENABLE（第 19 行定义）

```c
#define SNAIL_2305_TEST_ENABLE 0 /* 独立 C615 PWM 测试开关，0/1；与 C610 角度步长测试互斥。 */
```

编译期正式/测试路径选择，模式不在运行中切换；互斥限制见源#if。

源文件：[Core/Inc/bsp/snail_2305/test_snail_2305.h](../Core/Inc/bsp/snail_2305/test_snail_2305.h)。

Core引用：[Core/Inc/bsp/snail_2305/test_snail_2305.h](../Core/Inc/bsp/snail_2305/test_snail_2305.h) 第 49 行；[Core/Src/task/task_feed_motor/task_feed_motor.c](../Core/Src/task/task_feed_motor/task_feed_motor.c) 第 29 行。

### SNAIL_2305_TEST_STOP_PULSE_US（第 22 行定义）

```c
#define SNAIL_2305_TEST_STOP_PULSE_US 1000U /* 测试停止脉宽，us；采用整车例程 FRIC_OFF=1000。 */
```

C615脉宽us；停止/活动/范围各有独立参数，1MHz时CCR数字等于us。

源文件：[Core/Inc/bsp/snail_2305/test_snail_2305.h](../Core/Inc/bsp/snail_2305/test_snail_2305.h)。

Core引用：[Core/Inc/bsp/snail_2305/test_snail_2305.h](../Core/Inc/bsp/snail_2305/test_snail_2305.h) 第 58 行；[Core/Src/bsp/snail_2305/test_snail_2305.c](../Core/Src/bsp/snail_2305/test_snail_2305.c) 第 14 行。

### SNAIL_2305_TEST_CH1_DIRECTION_SIGN（第 25 行定义）

```c
#define SNAIL_2305_TEST_CH1_DIRECTION_SIGN 1 /* 测试 CH1 期望转向，+1/-1；实际方向由 C615 相线或 Assistant 设置。 */
```

方向映射±1；输入、反馈和电流独立。C615只记录方向，不用PWM切换正反转。

源文件：[Core/Inc/bsp/snail_2305/test_snail_2305.h](../Core/Inc/bsp/snail_2305/test_snail_2305.h)。

Core引用：[Core/Inc/bsp/snail_2305/test_snail_2305.h](../Core/Inc/bsp/snail_2305/test_snail_2305.h) 第 52 行；[Core/Src/bsp/snail_2305/test_snail_2305.c](../Core/Src/bsp/snail_2305/test_snail_2305.c) 第 34 行。

### SNAIL_2305_TEST_CH2_DIRECTION_SIGN（第 28 行定义）

```c
#define SNAIL_2305_TEST_CH2_DIRECTION_SIGN (-1) /* 测试 CH2 期望转向，+1/-1；实际方向由 C615 相线或 Assistant 设置。 */
```

方向映射±1；输入、反馈和电流独立。C615只记录方向，不用PWM切换正反转。

源文件：[Core/Inc/bsp/snail_2305/test_snail_2305.h](../Core/Inc/bsp/snail_2305/test_snail_2305.h)。

Core引用：[Core/Inc/bsp/snail_2305/test_snail_2305.h](../Core/Inc/bsp/snail_2305/test_snail_2305.h) 第 54 行；[Core/Src/bsp/snail_2305/test_snail_2305.c](../Core/Src/bsp/snail_2305/test_snail_2305.c) 第 41 行。

### SNAIL_2305_TEST_ACTIVE_PULSE_US（第 31 行定义）

```c
#define SNAIL_2305_TEST_ACTIVE_PULSE_US 1520U /* 测试常规活动脉宽，us；沿用整车例程 FRIC_DOWN=1520。 */
```

C615脉宽us；停止/活动/范围各有独立参数，1MHz时CCR数字等于us。

源文件：[Core/Inc/bsp/snail_2305/test_snail_2305.h](../Core/Inc/bsp/snail_2305/test_snail_2305.h)。

Core引用：[Core/Inc/bsp/snail_2305/test_snail_2305.h](../Core/Inc/bsp/snail_2305/test_snail_2305.h) 第 59 行；[Core/Src/bsp/snail_2305/test_snail_2305.c](../Core/Src/bsp/snail_2305/test_snail_2305.c) 第 13 行。

### SNAIL_2305_TEST_MAX_PULSE_US（第 34 行定义）

```c
#define SNAIL_2305_TEST_MAX_PULSE_US 1550U /* 测试活动上限，us；沿用整车例程 FRIC_UP=1550。 */
```

C615脉宽us；停止/活动/范围各有独立参数，1MHz时CCR数字等于us。

源文件：[Core/Inc/bsp/snail_2305/test_snail_2305.h](../Core/Inc/bsp/snail_2305/test_snail_2305.h)。

Core引用：[Core/Inc/bsp/snail_2305/test_snail_2305.h](../Core/Inc/bsp/snail_2305/test_snail_2305.h) 第 60 行；[Core/Src/bsp/snail_2305/test_snail_2305.c](../Core/Src/bsp/snail_2305/test_snail_2305.c) 第 36 行。

### SNAIL_2305_TEST_RAMP_TIME_MS（第 37 行定义）

```c
#define SNAIL_2305_TEST_RAMP_TIME_MS 500U /* 到活动值和返回停止值的时间，ms；过短会增加冲击。 */
```

Ramp变化最多rate×dt_s；PWM速率=(活动−停止)×1000/RAMP_MS。

源文件：[Core/Inc/bsp/snail_2305/test_snail_2305.h](../Core/Inc/bsp/snail_2305/test_snail_2305.h)。

Core引用：[Core/Inc/bsp/snail_2305/test_snail_2305.h](../Core/Inc/bsp/snail_2305/test_snail_2305.h) 第 62 行；[Core/Src/bsp/snail_2305/test_snail_2305.c](../Core/Src/bsp/snail_2305/test_snail_2305.c) 第 15 行。

### SNAIL_2305_TEST_STARTUP_STOP_TIME_MS（第 40 行定义）

```c
#define SNAIL_2305_TEST_STARTUP_STOP_TIME_MS 3000U /* 启动停止等待，ms；电调未识别停止脉宽时增加。 */
```

测试非阻塞阶段时长；TEST_RUN_TIME=0表示持续活动。

源文件：[Core/Inc/bsp/snail_2305/test_snail_2305.h](../Core/Inc/bsp/snail_2305/test_snail_2305.h)。

Core引用：[Core/Inc/bsp/snail_2305/test_snail_2305.h](../Core/Inc/bsp/snail_2305/test_snail_2305.h) 第 64 行；[Core/Src/bsp/snail_2305/test_snail_2305.c](../Core/Src/bsp/snail_2305/test_snail_2305.c) 第 90 行。

### SNAIL_2305_TEST_RUN_TIME_MS（第 43 行定义）

```c
#define SNAIL_2305_TEST_RUN_TIME_MS 0U /* 活动保持时间，ms；0 表示持续运行，台架风险更高。 */
```

测试非阻塞阶段时长；TEST_RUN_TIME=0表示持续活动。

源文件：[Core/Inc/bsp/snail_2305/test_snail_2305.h](../Core/Inc/bsp/snail_2305/test_snail_2305.h)。

Core引用：[Core/Src/bsp/snail_2305/test_snail_2305.c](../Core/Src/bsp/snail_2305/test_snail_2305.c) 第 93 行。

### SNAIL_2305_TEST_LOG_PERIOD_MS（第 46 行定义）

```c
#define SNAIL_2305_TEST_LOG_PERIOD_MS 500U /* 测试日志周期，ms；串口繁忙时增大。 */
```

诊断/提示限频，通常成功提交后续期，不参与电流控制。

源文件：[Core/Inc/bsp/snail_2305/test_snail_2305.h](../Core/Inc/bsp/snail_2305/test_snail_2305.h)。

Core引用：[Core/Inc/bsp/snail_2305/test_snail_2305.h](../Core/Inc/bsp/snail_2305/test_snail_2305.h) 第 65 行；[Core/Src/bsp/snail_2305/test_snail_2305.c](../Core/Src/bsp/snail_2305/test_snail_2305.c) 第 63 行。

## Core/Inc/can.h

### __CAN_H__（第 22 行定义）

```c
#define __CAN_H__
```

平台/模板项，按原定义、源注释与引用判断；没有位置或速度控制公式。

源文件：[Core/Inc/can.h](../Core/Inc/can.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

## Core/Inc/dma.h

### __DMA_H__（第 22 行定义）

```c
#define __DMA_H__
```

平台/模板项，按原定义、源注释与引用判断；没有位置或速度控制公式。

源文件：[Core/Inc/dma.h](../Core/Inc/dma.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

## Core/Inc/FreeRTOSConfig.h

### FREERTOS_CONFIG_H（第 32 行定义）

```c
#define FREERTOS_CONFIG_H
```

包含保护：避免同一翻译单元重复声明，没有控制算法。

源文件：[Core/Inc/FreeRTOSConfig.h](../Core/Inc/FreeRTOSConfig.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### CMSIS_device_header（第 56 行定义）

```c
#define CMSIS_device_header "stm32f4xx.h"
```

FreeRTOS/CMSIS端口配置：调度、堆、可用API、IRQ阈值，见总文档§13。

源文件：[Core/Inc/FreeRTOSConfig.h](../Core/Inc/FreeRTOSConfig.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### configENABLE_FPU（第 59 行定义）

```c
#define configENABLE_FPU                         0
```

FreeRTOS/CMSIS端口配置：调度、堆、可用API、IRQ阈值，见总文档§13。

源文件：[Core/Inc/FreeRTOSConfig.h](../Core/Inc/FreeRTOSConfig.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### configENABLE_MPU（第 60 行定义）

```c
#define configENABLE_MPU                         0
```

FreeRTOS/CMSIS端口配置：调度、堆、可用API、IRQ阈值，见总文档§13。

源文件：[Core/Inc/FreeRTOSConfig.h](../Core/Inc/FreeRTOSConfig.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### configUSE_PREEMPTION（第 62 行定义）

```c
#define configUSE_PREEMPTION                     1
```

FreeRTOS/CMSIS端口配置：调度、堆、可用API、IRQ阈值，见总文档§13。

源文件：[Core/Inc/FreeRTOSConfig.h](../Core/Inc/FreeRTOSConfig.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### configSUPPORT_STATIC_ALLOCATION（第 63 行定义）

```c
#define configSUPPORT_STATIC_ALLOCATION          1
```

FreeRTOS/CMSIS端口配置：调度、堆、可用API、IRQ阈值，见总文档§13。

源文件：[Core/Inc/FreeRTOSConfig.h](../Core/Inc/FreeRTOSConfig.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### configSUPPORT_DYNAMIC_ALLOCATION（第 64 行定义）

```c
#define configSUPPORT_DYNAMIC_ALLOCATION         1
```

FreeRTOS/CMSIS端口配置：调度、堆、可用API、IRQ阈值，见总文档§13。

源文件：[Core/Inc/FreeRTOSConfig.h](../Core/Inc/FreeRTOSConfig.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### configUSE_IDLE_HOOK（第 65 行定义）

```c
#define configUSE_IDLE_HOOK                      0
```

FreeRTOS/CMSIS端口配置：调度、堆、可用API、IRQ阈值，见总文档§13。

源文件：[Core/Inc/FreeRTOSConfig.h](../Core/Inc/FreeRTOSConfig.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### configUSE_TICK_HOOK（第 66 行定义）

```c
#define configUSE_TICK_HOOK                      0
```

FreeRTOS/CMSIS端口配置：调度、堆、可用API、IRQ阈值，见总文档§13。

源文件：[Core/Inc/FreeRTOSConfig.h](../Core/Inc/FreeRTOSConfig.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### configCPU_CLOCK_HZ（第 67 行定义）

```c
#define configCPU_CLOCK_HZ                       ( SystemCoreClock )
```

FreeRTOS/CMSIS端口配置：调度、堆、可用API、IRQ阈值，见总文档§13。

源文件：[Core/Inc/FreeRTOSConfig.h](../Core/Inc/FreeRTOSConfig.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### configTICK_RATE_HZ（第 68 行定义）

```c
#define configTICK_RATE_HZ                       ((TickType_t)1000)
```

FreeRTOS/CMSIS端口配置：调度、堆、可用API、IRQ阈值，见总文档§13。

源文件：[Core/Inc/FreeRTOSConfig.h](../Core/Inc/FreeRTOSConfig.h)。

Core引用：[Core/Src/task/task_feed_motor/task_feed_motor.c](../Core/Src/task/task_feed_motor/task_feed_motor.c) 第 70 行；[Core/Src/task/task_feed_motor/task_feed_motor_runtime.c](../Core/Src/task/task_feed_motor/task_feed_motor_runtime.c) 第 145 行。

### configMAX_PRIORITIES（第 69 行定义）

```c
#define configMAX_PRIORITIES                     ( 56 )
```

FreeRTOS/CMSIS端口配置：调度、堆、可用API、IRQ阈值，见总文档§13。

源文件：[Core/Inc/FreeRTOSConfig.h](../Core/Inc/FreeRTOSConfig.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### configMINIMAL_STACK_SIZE（第 70 行定义）

```c
#define configMINIMAL_STACK_SIZE                 ((uint16_t)128)
```

FreeRTOS/CMSIS端口配置：调度、堆、可用API、IRQ阈值，见总文档§13。

源文件：[Core/Inc/FreeRTOSConfig.h](../Core/Inc/FreeRTOSConfig.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### configTOTAL_HEAP_SIZE（第 71 行定义）

```c
#define configTOTAL_HEAP_SIZE                    ((size_t)49152)
```

FreeRTOS/CMSIS端口配置：调度、堆、可用API、IRQ阈值，见总文档§13。

源文件：[Core/Inc/FreeRTOSConfig.h](../Core/Inc/FreeRTOSConfig.h)。

Core引用：[Core/Inc/FreeRTOSConfig.h](../Core/Inc/FreeRTOSConfig.h) 第 170 行。

### configMAX_TASK_NAME_LEN（第 72 行定义）

```c
#define configMAX_TASK_NAME_LEN                  ( 16 )
```

FreeRTOS/CMSIS端口配置：调度、堆、可用API、IRQ阈值，见总文档§13。

源文件：[Core/Inc/FreeRTOSConfig.h](../Core/Inc/FreeRTOSConfig.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### configUSE_TRACE_FACILITY（第 73 行定义）

```c
#define configUSE_TRACE_FACILITY                 1
```

FreeRTOS/CMSIS端口配置：调度、堆、可用API、IRQ阈值，见总文档§13。

源文件：[Core/Inc/FreeRTOSConfig.h](../Core/Inc/FreeRTOSConfig.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### configUSE_16_BIT_TICKS（第 74 行定义）

```c
#define configUSE_16_BIT_TICKS                   0
```

FreeRTOS/CMSIS端口配置：调度、堆、可用API、IRQ阈值，见总文档§13。

源文件：[Core/Inc/FreeRTOSConfig.h](../Core/Inc/FreeRTOSConfig.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### configUSE_MUTEXES（第 75 行定义）

```c
#define configUSE_MUTEXES                        1
```

FreeRTOS/CMSIS端口配置：调度、堆、可用API、IRQ阈值，见总文档§13。

源文件：[Core/Inc/FreeRTOSConfig.h](../Core/Inc/FreeRTOSConfig.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### configQUEUE_REGISTRY_SIZE（第 76 行定义）

```c
#define configQUEUE_REGISTRY_SIZE                8
```

FreeRTOS/CMSIS端口配置：调度、堆、可用API、IRQ阈值，见总文档§13。

源文件：[Core/Inc/FreeRTOSConfig.h](../Core/Inc/FreeRTOSConfig.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### configUSE_RECURSIVE_MUTEXES（第 77 行定义）

```c
#define configUSE_RECURSIVE_MUTEXES              1
```

FreeRTOS/CMSIS端口配置：调度、堆、可用API、IRQ阈值，见总文档§13。

源文件：[Core/Inc/FreeRTOSConfig.h](../Core/Inc/FreeRTOSConfig.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### configUSE_COUNTING_SEMAPHORES（第 78 行定义）

```c
#define configUSE_COUNTING_SEMAPHORES            1
```

FreeRTOS/CMSIS端口配置：调度、堆、可用API、IRQ阈值，见总文档§13。

源文件：[Core/Inc/FreeRTOSConfig.h](../Core/Inc/FreeRTOSConfig.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### configUSE_PORT_OPTIMISED_TASK_SELECTION（第 79 行定义）

```c
#define configUSE_PORT_OPTIMISED_TASK_SELECTION  0
```

FreeRTOS/CMSIS端口配置：调度、堆、可用API、IRQ阈值，见总文档§13。

源文件：[Core/Inc/FreeRTOSConfig.h](../Core/Inc/FreeRTOSConfig.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### configMESSAGE_BUFFER_LENGTH_TYPE（第 83 行定义）

```c
#define configMESSAGE_BUFFER_LENGTH_TYPE         size_t /* 消息缓冲区长度字段类型。 */
```

FreeRTOS/CMSIS端口配置：调度、堆、可用API、IRQ阈值，见总文档§13。

源文件：[Core/Inc/FreeRTOSConfig.h](../Core/Inc/FreeRTOSConfig.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### configUSE_CO_ROUTINES（第 87 行定义）

```c
#define configUSE_CO_ROUTINES                    0
```

FreeRTOS/CMSIS端口配置：调度、堆、可用API、IRQ阈值，见总文档§13。

源文件：[Core/Inc/FreeRTOSConfig.h](../Core/Inc/FreeRTOSConfig.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### configMAX_CO_ROUTINE_PRIORITIES（第 88 行定义）

```c
#define configMAX_CO_ROUTINE_PRIORITIES          ( 2 )
```

FreeRTOS/CMSIS端口配置：调度、堆、可用API、IRQ阈值，见总文档§13。

源文件：[Core/Inc/FreeRTOSConfig.h](../Core/Inc/FreeRTOSConfig.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### configUSE_TIMERS（第 91 行定义）

```c
#define configUSE_TIMERS                         1
```

FreeRTOS/CMSIS端口配置：调度、堆、可用API、IRQ阈值，见总文档§13。

源文件：[Core/Inc/FreeRTOSConfig.h](../Core/Inc/FreeRTOSConfig.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### configTIMER_TASK_PRIORITY（第 92 行定义）

```c
#define configTIMER_TASK_PRIORITY                ( 2 )
```

FreeRTOS/CMSIS端口配置：调度、堆、可用API、IRQ阈值，见总文档§13。

源文件：[Core/Inc/FreeRTOSConfig.h](../Core/Inc/FreeRTOSConfig.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### configTIMER_QUEUE_LENGTH（第 93 行定义）

```c
#define configTIMER_QUEUE_LENGTH                 10
```

FreeRTOS/CMSIS端口配置：调度、堆、可用API、IRQ阈值，见总文档§13。

源文件：[Core/Inc/FreeRTOSConfig.h](../Core/Inc/FreeRTOSConfig.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### configTIMER_TASK_STACK_DEPTH（第 94 行定义）

```c
#define configTIMER_TASK_STACK_DEPTH             256
```

FreeRTOS/CMSIS端口配置：调度、堆、可用API、IRQ阈值，见总文档§13。

源文件：[Core/Inc/FreeRTOSConfig.h](../Core/Inc/FreeRTOSConfig.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### configUSE_OS2_THREAD_SUSPEND_RESUME（第 97 行定义）

```c
#define configUSE_OS2_THREAD_SUSPEND_RESUME  1
```

FreeRTOS/CMSIS端口配置：调度、堆、可用API、IRQ阈值，见总文档§13。

源文件：[Core/Inc/FreeRTOSConfig.h](../Core/Inc/FreeRTOSConfig.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### configUSE_OS2_THREAD_ENUMERATE（第 98 行定义）

```c
#define configUSE_OS2_THREAD_ENUMERATE       1
```

FreeRTOS/CMSIS端口配置：调度、堆、可用API、IRQ阈值，见总文档§13。

源文件：[Core/Inc/FreeRTOSConfig.h](../Core/Inc/FreeRTOSConfig.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### configUSE_OS2_EVENTFLAGS_FROM_ISR（第 99 行定义）

```c
#define configUSE_OS2_EVENTFLAGS_FROM_ISR    1
```

FreeRTOS/CMSIS端口配置：调度、堆、可用API、IRQ阈值，见总文档§13。

源文件：[Core/Inc/FreeRTOSConfig.h](../Core/Inc/FreeRTOSConfig.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### configUSE_OS2_THREAD_FLAGS（第 100 行定义）

```c
#define configUSE_OS2_THREAD_FLAGS           1
```

FreeRTOS/CMSIS端口配置：调度、堆、可用API、IRQ阈值，见总文档§13。

源文件：[Core/Inc/FreeRTOSConfig.h](../Core/Inc/FreeRTOSConfig.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### configUSE_OS2_TIMER（第 101 行定义）

```c
#define configUSE_OS2_TIMER                  1
```

FreeRTOS/CMSIS端口配置：调度、堆、可用API、IRQ阈值，见总文档§13。

源文件：[Core/Inc/FreeRTOSConfig.h](../Core/Inc/FreeRTOSConfig.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### configUSE_OS2_MUTEX（第 102 行定义）

```c
#define configUSE_OS2_MUTEX                  1
```

FreeRTOS/CMSIS端口配置：调度、堆、可用API、IRQ阈值，见总文档§13。

源文件：[Core/Inc/FreeRTOSConfig.h](../Core/Inc/FreeRTOSConfig.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### INCLUDE_vTaskPrioritySet（第 106 行定义）

```c
#define INCLUDE_vTaskPrioritySet             1
```

FreeRTOS/CMSIS端口配置：调度、堆、可用API、IRQ阈值，见总文档§13。

源文件：[Core/Inc/FreeRTOSConfig.h](../Core/Inc/FreeRTOSConfig.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### INCLUDE_uxTaskPriorityGet（第 107 行定义）

```c
#define INCLUDE_uxTaskPriorityGet            1
```

FreeRTOS/CMSIS端口配置：调度、堆、可用API、IRQ阈值，见总文档§13。

源文件：[Core/Inc/FreeRTOSConfig.h](../Core/Inc/FreeRTOSConfig.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### INCLUDE_vTaskDelete（第 108 行定义）

```c
#define INCLUDE_vTaskDelete                  1
```

FreeRTOS/CMSIS端口配置：调度、堆、可用API、IRQ阈值，见总文档§13。

源文件：[Core/Inc/FreeRTOSConfig.h](../Core/Inc/FreeRTOSConfig.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### INCLUDE_vTaskCleanUpResources（第 109 行定义）

```c
#define INCLUDE_vTaskCleanUpResources        0
```

FreeRTOS/CMSIS端口配置：调度、堆、可用API、IRQ阈值，见总文档§13。

源文件：[Core/Inc/FreeRTOSConfig.h](../Core/Inc/FreeRTOSConfig.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### INCLUDE_vTaskSuspend（第 110 行定义）

```c
#define INCLUDE_vTaskSuspend                 1
```

FreeRTOS/CMSIS端口配置：调度、堆、可用API、IRQ阈值，见总文档§13。

源文件：[Core/Inc/FreeRTOSConfig.h](../Core/Inc/FreeRTOSConfig.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### INCLUDE_vTaskDelayUntil（第 111 行定义）

```c
#define INCLUDE_vTaskDelayUntil              1
```

FreeRTOS/CMSIS端口配置：调度、堆、可用API、IRQ阈值，见总文档§13。

源文件：[Core/Inc/FreeRTOSConfig.h](../Core/Inc/FreeRTOSConfig.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### INCLUDE_vTaskDelay（第 112 行定义）

```c
#define INCLUDE_vTaskDelay                   1
```

FreeRTOS/CMSIS端口配置：调度、堆、可用API、IRQ阈值，见总文档§13。

源文件：[Core/Inc/FreeRTOSConfig.h](../Core/Inc/FreeRTOSConfig.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### INCLUDE_xTaskGetSchedulerState（第 113 行定义）

```c
#define INCLUDE_xTaskGetSchedulerState       1
```

FreeRTOS/CMSIS端口配置：调度、堆、可用API、IRQ阈值，见总文档§13。

源文件：[Core/Inc/FreeRTOSConfig.h](../Core/Inc/FreeRTOSConfig.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### INCLUDE_xTimerPendFunctionCall（第 114 行定义）

```c
#define INCLUDE_xTimerPendFunctionCall       1
```

FreeRTOS/CMSIS端口配置：调度、堆、可用API、IRQ阈值，见总文档§13。

源文件：[Core/Inc/FreeRTOSConfig.h](../Core/Inc/FreeRTOSConfig.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### INCLUDE_xQueueGetMutexHolder（第 115 行定义）

```c
#define INCLUDE_xQueueGetMutexHolder         1
```

FreeRTOS/CMSIS端口配置：调度、堆、可用API、IRQ阈值，见总文档§13。

源文件：[Core/Inc/FreeRTOSConfig.h](../Core/Inc/FreeRTOSConfig.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### INCLUDE_uxTaskGetStackHighWaterMark（第 116 行定义）

```c
#define INCLUDE_uxTaskGetStackHighWaterMark  1
```

FreeRTOS/CMSIS端口配置：调度、堆、可用API、IRQ阈值，见总文档§13。

源文件：[Core/Inc/FreeRTOSConfig.h](../Core/Inc/FreeRTOSConfig.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### INCLUDE_xTaskGetCurrentTaskHandle（第 117 行定义）

```c
#define INCLUDE_xTaskGetCurrentTaskHandle    1
```

FreeRTOS/CMSIS端口配置：调度、堆、可用API、IRQ阈值，见总文档§13。

源文件：[Core/Inc/FreeRTOSConfig.h](../Core/Inc/FreeRTOSConfig.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### INCLUDE_eTaskGetState（第 118 行定义）

```c
#define INCLUDE_eTaskGetState                1
```

FreeRTOS/CMSIS端口配置：调度、堆、可用API、IRQ阈值，见总文档§13。

源文件：[Core/Inc/FreeRTOSConfig.h](../Core/Inc/FreeRTOSConfig.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### USE_FreeRTOS_HEAP_4（第 124 行定义）

```c
#define USE_FreeRTOS_HEAP_4
```

FreeRTOS/CMSIS端口配置：调度、堆、可用API、IRQ阈值，见总文档§13。

源文件：[Core/Inc/FreeRTOSConfig.h](../Core/Inc/FreeRTOSConfig.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### configPRIO_BITS（第 129 行定义）

```c
 #define configPRIO_BITS         __NVIC_PRIO_BITS
```

FreeRTOS/CMSIS端口配置：调度、堆、可用API、IRQ阈值，见总文档§13。

源文件：[Core/Inc/FreeRTOSConfig.h](../Core/Inc/FreeRTOSConfig.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### configPRIO_BITS（第 131 行定义）

```c
 #define configPRIO_BITS         4
```

FreeRTOS/CMSIS端口配置：调度、堆、可用API、IRQ阈值，见总文档§13。

源文件：[Core/Inc/FreeRTOSConfig.h](../Core/Inc/FreeRTOSConfig.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### configLIBRARY_LOWEST_INTERRUPT_PRIORITY（第 136 行定义）

```c
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY   15
```

FreeRTOS/CMSIS端口配置：调度、堆、可用API、IRQ阈值，见总文档§13。

源文件：[Core/Inc/FreeRTOSConfig.h](../Core/Inc/FreeRTOSConfig.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY（第 142 行定义）

```c
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5
```

FreeRTOS/CMSIS端口配置：调度、堆、可用API、IRQ阈值，见总文档§13。

源文件：[Core/Inc/FreeRTOSConfig.h](../Core/Inc/FreeRTOSConfig.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### configKERNEL_INTERRUPT_PRIORITY（第 146 行定义）

```c
#define configKERNEL_INTERRUPT_PRIORITY 		( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )
```

FreeRTOS/CMSIS端口配置：调度、堆、可用API、IRQ阈值，见总文档§13。

源文件：[Core/Inc/FreeRTOSConfig.h](../Core/Inc/FreeRTOSConfig.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### configMAX_SYSCALL_INTERRUPT_PRIORITY（第 149 行定义）

```c
#define configMAX_SYSCALL_INTERRUPT_PRIORITY 	( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )
```

FreeRTOS/CMSIS端口配置：调度、堆、可用API、IRQ阈值，见总文档§13。

源文件：[Core/Inc/FreeRTOSConfig.h](../Core/Inc/FreeRTOSConfig.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### configASSERT（第 154 行定义）

```c
#define configASSERT( x ) if ((x) == 0) {taskDISABLE_INTERRUPTS(); for( ;; );} /* 断言失败后关闭中断并停机。 */
```

FreeRTOS/CMSIS端口配置：调度、堆、可用API、IRQ阈值，见总文档§13。

源文件：[Core/Inc/FreeRTOSConfig.h](../Core/Inc/FreeRTOSConfig.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### vPortSVCHandler（第 159 行定义）

```c
#define vPortSVCHandler    SVC_Handler
```

FreeRTOS/CMSIS端口配置：调度、堆、可用API、IRQ阈值，见总文档§13。

源文件：[Core/Inc/FreeRTOSConfig.h](../Core/Inc/FreeRTOSConfig.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### xPortPendSVHandler（第 160 行定义）

```c
#define xPortPendSVHandler PendSV_Handler
```

FreeRTOS/CMSIS端口配置：调度、堆、可用API、IRQ阈值，见总文档§13。

源文件：[Core/Inc/FreeRTOSConfig.h](../Core/Inc/FreeRTOSConfig.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### USE_CUSTOM_SYSTICK_HANDLER_IMPLEMENTATION（第 164 行定义）

```c
#define USE_CUSTOM_SYSTICK_HANDLER_IMPLEMENTATION 0
```

FreeRTOS/CMSIS端口配置：调度、堆、可用API、IRQ阈值，见总文档§13。

源文件：[Core/Inc/FreeRTOSConfig.h](../Core/Inc/FreeRTOSConfig.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### configCHECK_FOR_STACK_OVERFLOW（第 168 行定义）

```c
#define configCHECK_FOR_STACK_OVERFLOW 2 /* 用户任务栈边界检查，触发钩子记录任务名。 */
```

FreeRTOS/CMSIS端口配置：调度、堆、可用API、IRQ阈值，见总文档§13。

源文件：[Core/Inc/FreeRTOSConfig.h](../Core/Inc/FreeRTOSConfig.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### configUSE_MALLOC_FAILED_HOOK（第 169 行定义）

```c
#define configUSE_MALLOC_FAILED_HOOK 1 /* 动态分配失败时进入用户故障钩子。 */
```

FreeRTOS/CMSIS端口配置：调度、堆、可用API、IRQ阈值，见总文档§13。

源文件：[Core/Inc/FreeRTOSConfig.h](../Core/Inc/FreeRTOSConfig.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### configTOTAL_HEAP_SIZE（第 171 行定义）

```c
#define configTOTAL_HEAP_SIZE ((size_t)49152) /* heap_4 堆大小，bytes；为第六个INS任务预留，CubeMX也须设49152。 */
```

FreeRTOS/CMSIS端口配置：调度、堆、可用API、IRQ阈值，见总文档§13。

源文件：[Core/Inc/FreeRTOSConfig.h](../Core/Inc/FreeRTOSConfig.h)。

Core引用：[Core/Inc/FreeRTOSConfig.h](../Core/Inc/FreeRTOSConfig.h) 第 170 行。

## Core/Inc/gpio.h

### __GPIO_H__（第 22 行定义）

```c
#define __GPIO_H__
```

平台/模板项，按原定义、源注释与引用判断；没有位置或速度控制公式。

源文件：[Core/Inc/gpio.h](../Core/Inc/gpio.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

## Core/Inc/i2c.h

### __I2C_H__（第 22 行定义）

```c
#define __I2C_H__
```

平台/模板项，按原定义、源注释与引用判断；没有位置或速度控制公式。

源文件：[Core/Inc/i2c.h](../Core/Inc/i2c.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

## Core/Inc/main.h

### __MAIN_H（第 23 行定义）

```c
#define __MAIN_H
```

包含保护：避免同一翻译单元重复声明，没有控制算法。

源文件：[Core/Inc/main.h](../Core/Inc/main.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### FREERTOS_FAULT_TASK_NAME_LENGTH（第 41 行定义）

```c
#define FREERTOS_FAULT_TASK_NAME_LENGTH 16U
```

故障任务名缓冲16字节，复制保留末尾NUL。

源文件：[Core/Inc/main.h](../Core/Inc/main.h)。

Core引用：[Core/Inc/main.h](../Core/Inc/main.h) 第 48 行。

### SPI_MISO_Pin（第 83 行定义）

```c
#define SPI_MISO_Pin GPIO_PIN_4
```

平台/模板项，按原定义、源注释与引用判断；没有位置或速度控制公式。

源文件：[Core/Inc/main.h](../Core/Inc/main.h)。

Core引用：[Core/Src/spi.c](../Core/Src/spi.c) 第 83 行。

### SPI_MISO_GPIO_Port（第 84 行定义）

```c
#define SPI_MISO_GPIO_Port GPIOB
```

平台/模板项，按原定义、源注释与引用判断；没有位置或速度控制公式。

源文件：[Core/Inc/main.h](../Core/Inc/main.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### SPI1_CLK_Pin（第 85 行定义）

```c
#define SPI1_CLK_Pin GPIO_PIN_3
```

平台/模板项，按原定义、源注释与引用判断；没有位置或速度控制公式。

源文件：[Core/Inc/main.h](../Core/Inc/main.h)。

Core引用：[Core/Src/spi.c](../Core/Src/spi.c) 第 83 行。

### SPI1_CLK_GPIO_Port（第 86 行定义）

```c
#define SPI1_CLK_GPIO_Port GPIOB
```

平台/模板项，按原定义、源注释与引用判断；没有位置或速度控制公式。

源文件：[Core/Inc/main.h](../Core/Inc/main.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### RSTN_IST8310_Pin（第 87 行定义）

```c
#define RSTN_IST8310_Pin GPIO_PIN_6
```

平台/模板项，按原定义、源注释与引用判断；没有位置或速度控制公式。

源文件：[Core/Inc/main.h](../Core/Inc/main.h)。

Core引用：[Core/Src/gpio.c](../Core/Src/gpio.c) 第 57 行；[Core/Src/task/task_ins/task_ins_runtime.c](../Core/Src/task/task_ins/task_ins_runtime.c) 第 257 行。

### RSTN_IST8310_GPIO_Port（第 88 行定义）

```c
#define RSTN_IST8310_GPIO_Port GPIOG
```

平台/模板项，按原定义、源注释与引用判断；没有位置或速度控制公式。

源文件：[Core/Inc/main.h](../Core/Inc/main.h)。

Core引用：[Core/Src/gpio.c](../Core/Src/gpio.c) 第 57 行；[Core/Src/task/task_ins/task_ins_runtime.c](../Core/Src/task/task_ins/task_ins_runtime.c) 第 256 行。

### DRDY_IST8310_Pin（第 89 行定义）

```c
#define DRDY_IST8310_Pin GPIO_PIN_3
```

平台/模板项，按原定义、源注释与引用判断；没有位置或速度控制公式。

源文件：[Core/Inc/main.h](../Core/Inc/main.h)。

Core引用：[Core/Src/gpio.c](../Core/Src/gpio.c) 第 73 行；[Core/Src/stm32f4xx_it.c](../Core/Src/stm32f4xx_it.c) 第 205 行；[Core/Src/task/task_ins/task_ins_runtime.c](../Core/Src/task/task_ins/task_ins_runtime.c) 第 154 行。

### DRDY_IST8310_GPIO_Port（第 90 行定义）

```c
#define DRDY_IST8310_GPIO_Port GPIOG
```

平台/模板项，按原定义、源注释与引用判断；没有位置或速度控制公式。

源文件：[Core/Inc/main.h](../Core/Inc/main.h)。

Core引用：[Core/Src/gpio.c](../Core/Src/gpio.c) 第 76 行。

### DRDY_IST8310_EXTI_IRQn（第 91 行定义）

```c
#define DRDY_IST8310_EXTI_IRQn EXTI3_IRQn
```

平台/模板项，按原定义、源注释与引用判断；没有位置或速度控制公式。

源文件：[Core/Inc/main.h](../Core/Inc/main.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### CS1_Accel_Pin（第 92 行定义）

```c
#define CS1_Accel_Pin GPIO_PIN_4
```

平台/模板项，按原定义、源注释与引用判断；没有位置或速度控制公式。

源文件：[Core/Inc/main.h](../Core/Inc/main.h)。

Core引用：[Core/Src/gpio.c](../Core/Src/gpio.c) 第 60 行；[Core/Src/task/task_ins/task_ins_runtime.c](../Core/Src/task/task_ins/task_ins_runtime.c) 第 229 行。

### CS1_Accel_GPIO_Port（第 93 行定义）

```c
#define CS1_Accel_GPIO_Port GPIOA
```

平台/模板项，按原定义、源注释与引用判断；没有位置或速度控制公式。

源文件：[Core/Inc/main.h](../Core/Inc/main.h)。

Core引用：[Core/Src/gpio.c](../Core/Src/gpio.c) 第 60 行；[Core/Src/task/task_ins/task_ins_runtime.c](../Core/Src/task/task_ins/task_ins_runtime.c) 第 229 行。

### INT1_Accel_Pin（第 94 行定义）

```c
#define INT1_Accel_Pin GPIO_PIN_4
```

平台/模板项，按原定义、源注释与引用判断；没有位置或速度控制公式。

源文件：[Core/Inc/main.h](../Core/Inc/main.h)。

Core引用：[Core/Src/gpio.c](../Core/Src/gpio.c) 第 86 行；[Core/Src/stm32f4xx_it.c](../Core/Src/stm32f4xx_it.c) 第 219 行；[Core/Src/task/task_ins/task_ins_runtime.c](../Core/Src/task/task_ins/task_ins_runtime.c) 第 156 行。

### INT1_Accel_GPIO_Port（第 95 行定义）

```c
#define INT1_Accel_GPIO_Port GPIOC
```

平台/模板项，按原定义、源注释与引用判断；没有位置或速度控制公式。

源文件：[Core/Inc/main.h](../Core/Inc/main.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### INT1_Accel_EXTI_IRQn（第 96 行定义）

```c
#define INT1_Accel_EXTI_IRQn EXTI4_IRQn
```

平台/模板项，按原定义、源注释与引用判断；没有位置或速度控制公式。

源文件：[Core/Inc/main.h](../Core/Inc/main.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### INT1_Gyro_Pin（第 97 行定义）

```c
#define INT1_Gyro_Pin GPIO_PIN_5
```

平台/模板项，按原定义、源注释与引用判断；没有位置或速度控制公式。

源文件：[Core/Inc/main.h](../Core/Inc/main.h)。

Core引用：[Core/Src/gpio.c](../Core/Src/gpio.c) 第 86 行；[Core/Src/stm32f4xx_it.c](../Core/Src/stm32f4xx_it.c) 第 259 行；[Core/Src/task/task_ins/task_ins_runtime.c](../Core/Src/task/task_ins/task_ins_runtime.c) 第 155 行。

### INT1_Gyro_GPIO_Port（第 98 行定义）

```c
#define INT1_Gyro_GPIO_Port GPIOC
```

平台/模板项，按原定义、源注释与引用判断；没有位置或速度控制公式。

源文件：[Core/Inc/main.h](../Core/Inc/main.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### INT1_Gyro_EXTI_IRQn（第 99 行定义）

```c
#define INT1_Gyro_EXTI_IRQn EXTI9_5_IRQn
```

平台/模板项，按原定义、源注释与引用判断；没有位置或速度控制公式。

源文件：[Core/Inc/main.h](../Core/Inc/main.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### SPI1_MOSI_Pin（第 100 行定义）

```c
#define SPI1_MOSI_Pin GPIO_PIN_7
```

平台/模板项，按原定义、源注释与引用判断；没有位置或速度控制公式。

源文件：[Core/Inc/main.h](../Core/Inc/main.h)。

Core引用：[Core/Src/spi.c](../Core/Src/spi.c) 第 90 行。

### SPI1_MOSI_GPIO_Port（第 101 行定义）

```c
#define SPI1_MOSI_GPIO_Port GPIOA
```

平台/模板项，按原定义、源注释与引用判断；没有位置或速度控制公式。

源文件：[Core/Inc/main.h](../Core/Inc/main.h)。

Core引用：[Core/Src/spi.c](../Core/Src/spi.c) 第 95 行。

### CS1_Gyro_Pin（第 102 行定义）

```c
#define CS1_Gyro_Pin GPIO_PIN_0
```

平台/模板项，按原定义、源注释与引用判断；没有位置或速度控制公式。

源文件：[Core/Inc/main.h](../Core/Inc/main.h)。

Core引用：[Core/Src/gpio.c](../Core/Src/gpio.c) 第 63 行；[Core/Src/task/task_ins/task_ins_runtime.c](../Core/Src/task/task_ins/task_ins_runtime.c) 第 230 行。

### CS1_Gyro_GPIO_Port（第 103 行定义）

```c
#define CS1_Gyro_GPIO_Port GPIOB
```

平台/模板项，按原定义、源注释与引用判断；没有位置或速度控制公式。

源文件：[Core/Inc/main.h](../Core/Inc/main.h)。

Core引用：[Core/Src/gpio.c](../Core/Src/gpio.c) 第 63 行；[Core/Src/task/task_ins/task_ins_runtime.c](../Core/Src/task/task_ins/task_ins_runtime.c) 第 230 行。

## Core/Inc/spi.h

### __SPI_H__（第 22 行定义）

```c
#define __SPI_H__
```

平台/模板项，按原定义、源注释与引用判断；没有位置或速度控制公式。

源文件：[Core/Inc/spi.h](../Core/Inc/spi.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

## Core/Inc/stm32f4xx_hal_conf.h

### __STM32F4xx_HAL_CONF_H（第 25 行定义）

```c
#define __STM32F4xx_HAL_CONF_H
```

包含保护：避免同一翻译单元重复声明，没有控制算法。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### HAL_MODULE_ENABLED（第 38 行定义）

```c
#define HAL_MODULE_ENABLED
```

HAL驱动编译/回调选项，不意味着项目已有该设备业务。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### HAL_CAN_MODULE_ENABLED（第 42 行定义）

```c
#define HAL_CAN_MODULE_ENABLED
```

HAL驱动编译/回调选项，不意味着项目已有该设备业务。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core引用：[Core/Inc/bsp/c610_m2006/c610_m2006.h](../Core/Inc/bsp/c610_m2006/c610_m2006.h) 第 21 行；[Core/Inc/bsp/gm6020/gm6020.h](../Core/Inc/bsp/gm6020/gm6020.h) 第 16 行；[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h) 第 298 行；[Core/Src/bsp/c610_m2006/c610_m2006.c](../Core/Src/bsp/c610_m2006/c610_m2006.c) 第 15 行；[Core/Src/bsp/gm6020/gm6020.c](../Core/Src/bsp/gm6020/gm6020.c) 第 15 行。

### HAL_I2C_MODULE_ENABLED（第 56 行定义）

```c
#define HAL_I2C_MODULE_ENABLED
```

HAL驱动编译/回调选项，不意味着项目已有该设备业务。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core引用：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h) 第 362 行。

### HAL_SPI_MODULE_ENABLED（第 65 行定义）

```c
#define HAL_SPI_MODULE_ENABLED
```

HAL驱动编译/回调选项，不意味着项目已有该设备业务。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core引用：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h) 第 402 行。

### HAL_TIM_MODULE_ENABLED（第 66 行定义）

```c
#define HAL_TIM_MODULE_ENABLED
```

HAL驱动编译/回调选项，不意味着项目已有该设备业务。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core引用：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h) 第 406 行。

### HAL_UART_MODULE_ENABLED（第 67 行定义）

```c
#define HAL_UART_MODULE_ENABLED
```

HAL驱动编译/回调选项，不意味着项目已有该设备业务。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core引用：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h) 第 410 行。

### HAL_GPIO_MODULE_ENABLED（第 84 行定义）

```c
#define HAL_GPIO_MODULE_ENABLED
```

HAL驱动编译/回调选项，不意味着项目已有该设备业务。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core引用：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h) 第 278 行。

### HAL_EXTI_MODULE_ENABLED（第 85 行定义）

```c
#define HAL_EXTI_MODULE_ENABLED
```

HAL驱动编译/回调选项，不意味着项目已有该设备业务。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core引用：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h) 第 282 行。

### HAL_DMA_MODULE_ENABLED（第 86 行定义）

```c
#define HAL_DMA_MODULE_ENABLED
```

HAL驱动编译/回调选项，不意味着项目已有该设备业务。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core引用：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h) 第 286 行。

### HAL_RCC_MODULE_ENABLED（第 87 行定义）

```c
#define HAL_RCC_MODULE_ENABLED
```

HAL驱动编译/回调选项，不意味着项目已有该设备业务。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core引用：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h) 第 274 行。

### HAL_FLASH_MODULE_ENABLED（第 88 行定义）

```c
#define HAL_FLASH_MODULE_ENABLED
```

HAL驱动编译/回调选项，不意味着项目已有该设备业务。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core引用：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h) 第 334 行。

### HAL_PWR_MODULE_ENABLED（第 89 行定义）

```c
#define HAL_PWR_MODULE_ENABLED
```

HAL驱动编译/回调选项，不意味着项目已有该设备业务。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core引用：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h) 第 382 行。

### HAL_CORTEX_MODULE_ENABLED（第 90 行定义）

```c
#define HAL_CORTEX_MODULE_ENABLED
```

HAL驱动编译/回调选项，不意味着项目已有该设备业务。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core引用：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h) 第 290 行。

### HSE_VALUE（第 99 行定义）

```c
  #define HSE_VALUE    12000000U /*!< Value of the External oscillator in Hz */
```

时钟频率Hz；PLL、总线与外设波特率/计时基于这些值。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core引用：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h) 第 98 行；[Core/Src/system_stm32f4xx.c](../Core/Src/system_stm32f4xx.c) 第 50 行。

### HSE_STARTUP_TIMEOUT（第 103 行定义）

```c
  #define HSE_STARTUP_TIMEOUT    100U   /*!< Time out for HSE start up, in ms */
```

测试非阻塞阶段时长；TEST_RUN_TIME=0表示持续活动。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core引用：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h) 第 102 行。

### HSI_VALUE（第 112 行定义）

```c
  #define HSI_VALUE    ((uint32_t)16000000U) /*!< Value of the Internal oscillator in Hz*/
```

时钟频率Hz；PLL、总线与外设波特率/计时基于这些值。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core引用：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h) 第 111 行；[Core/Src/system_stm32f4xx.c](../Core/Src/system_stm32f4xx.c) 第 54 行。

### LSI_VALUE（第 119 行定义）

```c
 #define LSI_VALUE  32000U       /*!< LSI Typical Value in Hz*/
```

时钟频率Hz；PLL、总线与外设波特率/计时基于这些值。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core引用：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h) 第 118 行。

### LSE_VALUE（第 127 行定义）

```c
 #define LSE_VALUE  32768U    /*!< Value of the External Low Speed oscillator in Hz */
```

时钟频率Hz；PLL、总线与外设波特率/计时基于这些值。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core引用：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h) 第 126 行。

### LSE_STARTUP_TIMEOUT（第 131 行定义）

```c
  #define LSE_STARTUP_TIMEOUT    5000U   /*!< Time out for LSE start up, in ms */
```

测试非阻塞阶段时长；TEST_RUN_TIME=0表示持续活动。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core引用：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h) 第 130 行。

### EXTERNAL_CLOCK_VALUE（第 140 行定义）

```c
  #define EXTERNAL_CLOCK_VALUE    12288000U /*!< Value of the External audio frequency in Hz*/
```

时钟频率Hz；PLL、总线与外设波特率/计时基于这些值。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core引用：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h) 第 139 行。

### VDD_VALUE（第 150 行定义）

```c
#define  VDD_VALUE		      3300U /*!< Value of VDD in mv */
```

HAL平台初始化/时基参数，按源注释单位和HAL消费路径解释。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### TICK_INT_PRIORITY（第 151 行定义）

```c
#define  TICK_INT_PRIORITY            15U   /*!< tick interrupt priority */
```

HAL平台初始化/时基参数，按源注释单位和HAL消费路径解释。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### USE_RTOS（第 152 行定义）

```c
#define  USE_RTOS                     0U
```

HAL平台初始化/时基参数，按源注释单位和HAL消费路径解释。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### PREFETCH_ENABLE（第 153 行定义）

```c
#define  PREFETCH_ENABLE              1U
```

平台/模板项，按原定义、源注释与引用判断；没有位置或速度控制公式。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### INSTRUCTION_CACHE_ENABLE（第 154 行定义）

```c
#define  INSTRUCTION_CACHE_ENABLE     1U
```

平台/模板项，按原定义、源注释与引用判断；没有位置或速度控制公式。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### DATA_CACHE_ENABLE（第 155 行定义）

```c
#define  DATA_CACHE_ENABLE            1U
```

平台/模板项，按原定义、源注释与引用判断；没有位置或速度控制公式。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### USE_HAL_ADC_REGISTER_CALLBACKS（第 157 行定义）

```c
#define  USE_HAL_ADC_REGISTER_CALLBACKS         0U /* ADC register callback disabled       */
```

HAL驱动编译/回调选项，不意味着项目已有该设备业务。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### USE_HAL_CAN_REGISTER_CALLBACKS（第 158 行定义）

```c
#define  USE_HAL_CAN_REGISTER_CALLBACKS         0U /* CAN register callback disabled       */
```

HAL驱动编译/回调选项，不意味着项目已有该设备业务。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### USE_HAL_CEC_REGISTER_CALLBACKS（第 159 行定义）

```c
#define  USE_HAL_CEC_REGISTER_CALLBACKS         0U /* CEC register callback disabled       */
```

HAL驱动编译/回调选项，不意味着项目已有该设备业务。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### USE_HAL_CRYP_REGISTER_CALLBACKS（第 160 行定义）

```c
#define  USE_HAL_CRYP_REGISTER_CALLBACKS        0U /* CRYP register callback disabled      */
```

HAL驱动编译/回调选项，不意味着项目已有该设备业务。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### USE_HAL_DAC_REGISTER_CALLBACKS（第 161 行定义）

```c
#define  USE_HAL_DAC_REGISTER_CALLBACKS         0U /* DAC register callback disabled       */
```

HAL驱动编译/回调选项，不意味着项目已有该设备业务。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### USE_HAL_DCMI_REGISTER_CALLBACKS（第 162 行定义）

```c
#define  USE_HAL_DCMI_REGISTER_CALLBACKS        0U /* DCMI register callback disabled      */
```

HAL驱动编译/回调选项，不意味着项目已有该设备业务。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### USE_HAL_DFSDM_REGISTER_CALLBACKS（第 163 行定义）

```c
#define  USE_HAL_DFSDM_REGISTER_CALLBACKS       0U /* DFSDM register callback disabled     */
```

HAL驱动编译/回调选项，不意味着项目已有该设备业务。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### USE_HAL_DMA2D_REGISTER_CALLBACKS（第 164 行定义）

```c
#define  USE_HAL_DMA2D_REGISTER_CALLBACKS       0U /* DMA2D register callback disabled     */
```

HAL驱动编译/回调选项，不意味着项目已有该设备业务。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### USE_HAL_DSI_REGISTER_CALLBACKS（第 165 行定义）

```c
#define  USE_HAL_DSI_REGISTER_CALLBACKS         0U /* DSI register callback disabled       */
```

HAL驱动编译/回调选项，不意味着项目已有该设备业务。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### USE_HAL_ETH_REGISTER_CALLBACKS（第 166 行定义）

```c
#define  USE_HAL_ETH_REGISTER_CALLBACKS         0U /* ETH register callback disabled       */
```

HAL驱动编译/回调选项，不意味着项目已有该设备业务。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### USE_HAL_HASH_REGISTER_CALLBACKS（第 167 行定义）

```c
#define  USE_HAL_HASH_REGISTER_CALLBACKS        0U /* HASH register callback disabled      */
```

HAL驱动编译/回调选项，不意味着项目已有该设备业务。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### USE_HAL_HCD_REGISTER_CALLBACKS（第 168 行定义）

```c
#define  USE_HAL_HCD_REGISTER_CALLBACKS         0U /* HCD register callback disabled       */
```

HAL驱动编译/回调选项，不意味着项目已有该设备业务。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### USE_HAL_I2C_REGISTER_CALLBACKS（第 169 行定义）

```c
#define  USE_HAL_I2C_REGISTER_CALLBACKS         0U /* I2C register callback disabled       */
```

HAL驱动编译/回调选项，不意味着项目已有该设备业务。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### USE_HAL_FMPI2C_REGISTER_CALLBACKS（第 170 行定义）

```c
#define  USE_HAL_FMPI2C_REGISTER_CALLBACKS      0U /* FMPI2C register callback disabled    */
```

HAL驱动编译/回调选项，不意味着项目已有该设备业务。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### USE_HAL_FMPSMBUS_REGISTER_CALLBACKS（第 171 行定义）

```c
#define  USE_HAL_FMPSMBUS_REGISTER_CALLBACKS    0U /* FMPSMBUS register callback disabled  */
```

HAL驱动编译/回调选项，不意味着项目已有该设备业务。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### USE_HAL_I2S_REGISTER_CALLBACKS（第 172 行定义）

```c
#define  USE_HAL_I2S_REGISTER_CALLBACKS         0U /* I2S register callback disabled       */
```

HAL驱动编译/回调选项，不意味着项目已有该设备业务。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### USE_HAL_IRDA_REGISTER_CALLBACKS（第 173 行定义）

```c
#define  USE_HAL_IRDA_REGISTER_CALLBACKS        0U /* IRDA register callback disabled      */
```

HAL驱动编译/回调选项，不意味着项目已有该设备业务。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### USE_HAL_LPTIM_REGISTER_CALLBACKS（第 174 行定义）

```c
#define  USE_HAL_LPTIM_REGISTER_CALLBACKS       0U /* LPTIM register callback disabled     */
```

HAL驱动编译/回调选项，不意味着项目已有该设备业务。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### USE_HAL_LTDC_REGISTER_CALLBACKS（第 175 行定义）

```c
#define  USE_HAL_LTDC_REGISTER_CALLBACKS        0U /* LTDC register callback disabled      */
```

HAL驱动编译/回调选项，不意味着项目已有该设备业务。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### USE_HAL_MMC_REGISTER_CALLBACKS（第 176 行定义）

```c
#define  USE_HAL_MMC_REGISTER_CALLBACKS         0U /* MMC register callback disabled       */
```

HAL驱动编译/回调选项，不意味着项目已有该设备业务。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### USE_HAL_NAND_REGISTER_CALLBACKS（第 177 行定义）

```c
#define  USE_HAL_NAND_REGISTER_CALLBACKS        0U /* NAND register callback disabled      */
```

HAL驱动编译/回调选项，不意味着项目已有该设备业务。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### USE_HAL_NOR_REGISTER_CALLBACKS（第 178 行定义）

```c
#define  USE_HAL_NOR_REGISTER_CALLBACKS         0U /* NOR register callback disabled       */
```

HAL驱动编译/回调选项，不意味着项目已有该设备业务。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### USE_HAL_PCCARD_REGISTER_CALLBACKS（第 179 行定义）

```c
#define  USE_HAL_PCCARD_REGISTER_CALLBACKS      0U /* PCCARD register callback disabled    */
```

HAL驱动编译/回调选项，不意味着项目已有该设备业务。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### USE_HAL_PCD_REGISTER_CALLBACKS（第 180 行定义）

```c
#define  USE_HAL_PCD_REGISTER_CALLBACKS         0U /* PCD register callback disabled       */
```

HAL驱动编译/回调选项，不意味着项目已有该设备业务。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### USE_HAL_QSPI_REGISTER_CALLBACKS（第 181 行定义）

```c
#define  USE_HAL_QSPI_REGISTER_CALLBACKS        0U /* QSPI register callback disabled      */
```

HAL驱动编译/回调选项，不意味着项目已有该设备业务。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### USE_HAL_RNG_REGISTER_CALLBACKS（第 182 行定义）

```c
#define  USE_HAL_RNG_REGISTER_CALLBACKS         0U /* RNG register callback disabled       */
```

HAL驱动编译/回调选项，不意味着项目已有该设备业务。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### USE_HAL_RTC_REGISTER_CALLBACKS（第 183 行定义）

```c
#define  USE_HAL_RTC_REGISTER_CALLBACKS         0U /* RTC register callback disabled       */
```

HAL驱动编译/回调选项，不意味着项目已有该设备业务。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### USE_HAL_SAI_REGISTER_CALLBACKS（第 184 行定义）

```c
#define  USE_HAL_SAI_REGISTER_CALLBACKS         0U /* SAI register callback disabled       */
```

HAL驱动编译/回调选项，不意味着项目已有该设备业务。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### USE_HAL_SD_REGISTER_CALLBACKS（第 185 行定义）

```c
#define  USE_HAL_SD_REGISTER_CALLBACKS          0U /* SD register callback disabled        */
```

HAL驱动编译/回调选项，不意味着项目已有该设备业务。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### USE_HAL_SMARTCARD_REGISTER_CALLBACKS（第 186 行定义）

```c
#define  USE_HAL_SMARTCARD_REGISTER_CALLBACKS   0U /* SMARTCARD register callback disabled */
```

HAL驱动编译/回调选项，不意味着项目已有该设备业务。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### USE_HAL_SDRAM_REGISTER_CALLBACKS（第 187 行定义）

```c
#define  USE_HAL_SDRAM_REGISTER_CALLBACKS       0U /* SDRAM register callback disabled     */
```

HAL驱动编译/回调选项，不意味着项目已有该设备业务。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### USE_HAL_SRAM_REGISTER_CALLBACKS（第 188 行定义）

```c
#define  USE_HAL_SRAM_REGISTER_CALLBACKS        0U /* SRAM register callback disabled      */
```

HAL驱动编译/回调选项，不意味着项目已有该设备业务。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### USE_HAL_SPDIFRX_REGISTER_CALLBACKS（第 189 行定义）

```c
#define  USE_HAL_SPDIFRX_REGISTER_CALLBACKS     0U /* SPDIFRX register callback disabled   */
```

HAL驱动编译/回调选项，不意味着项目已有该设备业务。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### USE_HAL_SMBUS_REGISTER_CALLBACKS（第 190 行定义）

```c
#define  USE_HAL_SMBUS_REGISTER_CALLBACKS       0U /* SMBUS register callback disabled     */
```

HAL驱动编译/回调选项，不意味着项目已有该设备业务。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### USE_HAL_SPI_REGISTER_CALLBACKS（第 191 行定义）

```c
#define  USE_HAL_SPI_REGISTER_CALLBACKS         0U /* SPI register callback disabled       */
```

HAL驱动编译/回调选项，不意味着项目已有该设备业务。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### USE_HAL_TIM_REGISTER_CALLBACKS（第 192 行定义）

```c
#define  USE_HAL_TIM_REGISTER_CALLBACKS         0U /* TIM register callback disabled       */
```

HAL驱动编译/回调选项，不意味着项目已有该设备业务。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### USE_HAL_UART_REGISTER_CALLBACKS（第 193 行定义）

```c
#define  USE_HAL_UART_REGISTER_CALLBACKS        0U /* UART register callback disabled      */
```

HAL驱动编译/回调选项，不意味着项目已有该设备业务。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### USE_HAL_USART_REGISTER_CALLBACKS（第 194 行定义）

```c
#define  USE_HAL_USART_REGISTER_CALLBACKS       0U /* USART register callback disabled     */
```

HAL驱动编译/回调选项，不意味着项目已有该设备业务。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### USE_HAL_WWDG_REGISTER_CALLBACKS（第 195 行定义）

```c
#define  USE_HAL_WWDG_REGISTER_CALLBACKS        0U /* WWDG register callback disabled      */
```

HAL驱动编译/回调选项，不意味着项目已有该设备业务。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### MAC_ADDR0（第 209 行定义）

```c
#define MAC_ADDR0   2U
```

HAL模板协议/驱动选项，当前没有对应应用业务，不是云台增益。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### MAC_ADDR1（第 210 行定义）

```c
#define MAC_ADDR1   0U
```

HAL模板协议/驱动选项，当前没有对应应用业务，不是云台增益。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### MAC_ADDR2（第 211 行定义）

```c
#define MAC_ADDR2   0U
```

HAL模板协议/驱动选项，当前没有对应应用业务，不是云台增益。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### MAC_ADDR3（第 212 行定义）

```c
#define MAC_ADDR3   0U
```

HAL模板协议/驱动选项，当前没有对应应用业务，不是云台增益。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### MAC_ADDR4（第 213 行定义）

```c
#define MAC_ADDR4   0U
```

HAL模板协议/驱动选项，当前没有对应应用业务，不是云台增益。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### MAC_ADDR5（第 214 行定义）

```c
#define MAC_ADDR5   0U
```

HAL模板协议/驱动选项，当前没有对应应用业务，不是云台增益。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### ETH_RX_BUF_SIZE（第 217 行定义）

```c
#define ETH_RX_BUF_SIZE                ETH_MAX_PACKET_SIZE /* buffer size for receive               */
```

HAL模板协议/驱动选项，当前没有对应应用业务，不是云台增益。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### ETH_TX_BUF_SIZE（第 218 行定义）

```c
#define ETH_TX_BUF_SIZE                ETH_MAX_PACKET_SIZE /* buffer size for transmit              */
```

HAL模板协议/驱动选项，当前没有对应应用业务，不是云台增益。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### ETH_RXBUFNB（第 219 行定义）

```c
#define ETH_RXBUFNB                    4U       /* 4 Rx buffers of size ETH_RX_BUF_SIZE  */
```

HAL模板协议/驱动选项，当前没有对应应用业务，不是云台增益。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### ETH_TXBUFNB（第 220 行定义）

```c
#define ETH_TXBUFNB                    4U       /* 4 Tx buffers of size ETH_TX_BUF_SIZE  */
```

HAL模板协议/驱动选项，当前没有对应应用业务，不是云台增益。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### DP83848_PHY_ADDRESS（第 225 行定义）

```c
#define DP83848_PHY_ADDRESS
```

平台/模板项，按原定义、源注释与引用判断；没有位置或速度控制公式。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### PHY_RESET_DELAY（第 227 行定义）

```c
#define PHY_RESET_DELAY                 0x000000FFU
```

HAL模板协议/驱动选项，当前没有对应应用业务，不是云台增益。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### PHY_CONFIG_DELAY（第 229 行定义）

```c
#define PHY_CONFIG_DELAY                0x00000FFFU
```

FreeRTOS/CMSIS端口配置：调度、堆、可用API、IRQ阈值，见总文档§13。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### PHY_READ_TO（第 231 行定义）

```c
#define PHY_READ_TO                     0x0000FFFFU
```

HAL模板协议/驱动选项，当前没有对应应用业务，不是云台增益。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### PHY_WRITE_TO（第 232 行定义）

```c
#define PHY_WRITE_TO                    0x0000FFFFU
```

HAL模板协议/驱动选项，当前没有对应应用业务，不是云台增益。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### PHY_BCR（第 236 行定义）

```c
#define PHY_BCR                         ((uint16_t)0x0000U)    /*!< Transceiver Basic Control Register   */
```

HAL模板协议/驱动选项，当前没有对应应用业务，不是云台增益。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### PHY_BSR（第 237 行定义）

```c
#define PHY_BSR                         ((uint16_t)0x0001U)    /*!< Transceiver Basic Status Register    */
```

HAL模板协议/驱动选项，当前没有对应应用业务，不是云台增益。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### PHY_RESET（第 239 行定义）

```c
#define PHY_RESET                       ((uint16_t)0x8000U)  /*!< PHY Reset */
```

HAL模板协议/驱动选项，当前没有对应应用业务，不是云台增益。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### PHY_LOOPBACK（第 240 行定义）

```c
#define PHY_LOOPBACK                    ((uint16_t)0x4000U)  /*!< Select loop-back mode */
```

HAL模板协议/驱动选项，当前没有对应应用业务，不是云台增益。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### PHY_FULLDUPLEX_100M（第 241 行定义）

```c
#define PHY_FULLDUPLEX_100M             ((uint16_t)0x2100U)  /*!< Set the full-duplex mode at 100 Mb/s */
```

HAL模板协议/驱动选项，当前没有对应应用业务，不是云台增益。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### PHY_HALFDUPLEX_100M（第 242 行定义）

```c
#define PHY_HALFDUPLEX_100M             ((uint16_t)0x2000U)  /*!< Set the half-duplex mode at 100 Mb/s */
```

HAL模板协议/驱动选项，当前没有对应应用业务，不是云台增益。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### PHY_FULLDUPLEX_10M（第 243 行定义）

```c
#define PHY_FULLDUPLEX_10M              ((uint16_t)0x0100U)  /*!< Set the full-duplex mode at 10 Mb/s  */
```

HAL模板协议/驱动选项，当前没有对应应用业务，不是云台增益。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### PHY_HALFDUPLEX_10M（第 244 行定义）

```c
#define PHY_HALFDUPLEX_10M              ((uint16_t)0x0000U)  /*!< Set the half-duplex mode at 10 Mb/s  */
```

HAL模板协议/驱动选项，当前没有对应应用业务，不是云台增益。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### PHY_AUTONEGOTIATION（第 245 行定义）

```c
#define PHY_AUTONEGOTIATION             ((uint16_t)0x1000U)  /*!< Enable auto-negotiation function     */
```

HAL模板协议/驱动选项，当前没有对应应用业务，不是云台增益。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### PHY_RESTART_AUTONEGOTIATION（第 246 行定义）

```c
#define PHY_RESTART_AUTONEGOTIATION     ((uint16_t)0x0200U)  /*!< Restart auto-negotiation function    */
```

HAL模板协议/驱动选项，当前没有对应应用业务，不是云台增益。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### PHY_POWERDOWN（第 247 行定义）

```c
#define PHY_POWERDOWN                   ((uint16_t)0x0800U)  /*!< Select the power down mode           */
```

HAL模板协议/驱动选项，当前没有对应应用业务，不是云台增益。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### PHY_ISOLATE（第 248 行定义）

```c
#define PHY_ISOLATE                     ((uint16_t)0x0400U)  /*!< Isolate PHY from MII                 */
```

HAL模板协议/驱动选项，当前没有对应应用业务，不是云台增益。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### PHY_AUTONEGO_COMPLETE（第 250 行定义）

```c
#define PHY_AUTONEGO_COMPLETE           ((uint16_t)0x0020U)  /*!< Auto-Negotiation process completed   */
```

HAL模板协议/驱动选项，当前没有对应应用业务，不是云台增益。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### PHY_LINKED_STATUS（第 251 行定义）

```c
#define PHY_LINKED_STATUS               ((uint16_t)0x0004U)  /*!< Valid link established               */
```

HAL模板协议/驱动选项，当前没有对应应用业务，不是云台增益。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### PHY_JABBER_DETECTION（第 252 行定义）

```c
#define PHY_JABBER_DETECTION            ((uint16_t)0x0002U)  /*!< Jabber condition detected            */
```

HAL模板协议/驱动选项，当前没有对应应用业务，不是云台增益。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### PHY_SR（第 255 行定义）

```c
#define PHY_SR                          ((uint16_t))    /*!< PHY status register Offset                      */
```

HAL模板协议/驱动选项，当前没有对应应用业务，不是云台增益。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### PHY_SPEED_STATUS（第 257 行定义）

```c
#define PHY_SPEED_STATUS                ((uint16_t))  /*!< PHY Speed mask                                  */
```

HAL模板协议/驱动选项，当前没有对应应用业务，不是云台增益。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### PHY_DUPLEX_STATUS（第 258 行定义）

```c
#define PHY_DUPLEX_STATUS               ((uint16_t))  /*!< PHY Duplex mask                                 */
```

HAL模板协议/驱动选项，当前没有对应应用业务，不是云台增益。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### USE_SPI_CRC（第 267 行定义）

```c
#define USE_SPI_CRC                     0U
```

HAL模板协议/驱动选项，当前没有对应应用业务，不是云台增益。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### assert_param（第 484 行定义）

```c
  #define assert_param(expr) ((expr) ? (void)0U : assert_failed((uint8_t *)__FILE__, __LINE__))
```

HAL参数断言，USE_FULL_ASSERT分支调用assert_failed，否则空操作。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### assert_param（第 488 行定义）

```c
  #define assert_param(expr) ((void)0U)
```

HAL参数断言，USE_FULL_ASSERT分支调用assert_failed，否则空操作。

源文件：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

## Core/Inc/stm32f4xx_it.h

### __STM32F4xx_IT_H（第 22 行定义）

```c
#define __STM32F4xx_IT_H
```

包含保护：避免同一翻译单元重复声明，没有控制算法。

源文件：[Core/Inc/stm32f4xx_it.h](../Core/Inc/stm32f4xx_it.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

## Core/Inc/task/task_chassis/task_chassis.h

### TASK_CHASSIS_H（第 9 行定义）

```c
#define TASK_CHASSIS_H /* 防止重复包含。 */
```

包含保护：避免同一翻译单元重复声明，没有控制算法。

源文件：[Core/Inc/task/task_chassis/task_chassis.h](../Core/Inc/task/task_chassis/task_chassis.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

## Core/Inc/task/task_chassis/task_chassis_command.h

### TASK_CHASSIS_COMMAND_H（第 9 行定义）

```c
#define TASK_CHASSIS_COMMAND_H /* 防止重复包含。 */
```

包含保护：避免同一翻译单元重复声明，没有控制算法。

源文件：[Core/Inc/task/task_chassis/task_chassis_command.h](../Core/Inc/task/task_chassis/task_chassis_command.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

## Core/Inc/task/task_chassis/task_chassis_config.h

### TASK_CHASSIS_CONFIG_H（第 9 行定义）

```c
#define TASK_CHASSIS_CONFIG_H /* 防止重复包含。 */
```

包含保护：避免同一翻译单元重复声明，没有控制算法。

源文件：[Core/Inc/task/task_chassis/task_chassis_config.h](../Core/Inc/task/task_chassis/task_chassis_config.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### CHASSIS_TASK_PERIOD_MS（第 12 行定义）

```c
#define CHASSIS_TASK_PERIOD_MS 2U /* 调度周期，ms；必须换算为至少一个 Tick。 */
```

底盘vTaskDelayUntil调度周期，经pdMS_TO_TICKS转换；PID使用实际HAL间隔。

源文件：[Core/Inc/task/task_chassis/task_chassis_config.h](../Core/Inc/task/task_chassis/task_chassis_config.h)。

Core引用：[Core/Src/task/task_chassis/task_chassis.c](../Core/Src/task/task_chassis/task_chassis.c) 第 16 行。

### CHASSIS_INIT_RETRY_MS（第 15 行定义）

```c
#define CHASSIS_INIT_RETRY_MS 100U /* 初始化失败重试间隔，ms；避免忙等。 */
```

初始化/恢复失败节流，比较HAL年龄或转换Tick后等待。

源文件：[Core/Inc/task/task_chassis/task_chassis_config.h](../Core/Inc/task/task_chassis/task_chassis_config.h)。

Core引用：[Core/Src/task/task_chassis/task_chassis.c](../Core/Src/task/task_chassis/task_chassis.c) 第 17 行。

### CHASSIS_COMMAND_TIMEOUT_MS（第 18 行定义）

```c
#define CHASSIS_COMMAND_TIMEOUT_MS 100U /* 命令期限，HAL ms，原接收时间不被转发续期。 */
```

原始HAL接收时间戳年龄>100ms，底盘回STOP并清键/历史、四轮零电流。

源文件：[Core/Inc/task/task_chassis/task_chassis_config.h](../Core/Inc/task/task_chassis/task_chassis_config.h)。

Core引用：[Core/Src/task/task_chassis/task_chassis_command.c](../Core/Src/task/task_chassis/task_chassis_command.c) 第 17 行。

### CHASSIS_FEEDBACK_TIMEOUT_MS（第 21 行定义）

```c
#define CHASSIS_FEEDBACK_TIMEOUT_MS 100U /* 单轮反馈期限，HAL ms；超时只停止该轮。 */
```

真实反馈HAL年龄>100ms，仅停止该轮并清其PID，发送入口再次复核。

源文件：[Core/Inc/task/task_chassis/task_chassis_config.h](../Core/Inc/task/task_chassis/task_chassis_config.h)。

Core引用：[Core/Src/task/task_chassis/task_chassis_runtime.c](../Core/Src/task/task_chassis/task_chassis_runtime.c) 第 97 行。

### CHASSIS_CONTROL_GAP_MS（第 24 行定义）

```c
#define CHASSIS_CONTROL_GAP_MS 100U /* 允许的最大控制间隔，HAL ms，超过后清历史并零输出。 */
```

HAL实际间隔为0或超过阈值则四轮零并清历史，正常间隔换秒给PID。

源文件：[Core/Inc/task/task_chassis/task_chassis_config.h](../Core/Inc/task/task_chassis/task_chassis_config.h)。

Core引用：[Core/Src/task/task_chassis/task_chassis_runtime.c](../Core/Src/task/task_chassis/task_chassis_runtime.c) 第 23 行。

### CHASSIS_LOG_PERIOD_MS（第 27 行定义）

```c
#define CHASSIS_LOG_PERIOD_MS 1000U /* 底盘遥测周期，HAL ms；9600 baud 下建议不小于此值。 */
```

诊断/提示限频，通常成功提交后续期，不参与电流控制。

源文件：[Core/Inc/task/task_chassis/task_chassis_config.h](../Core/Inc/task/task_chassis/task_chassis_config.h)。

Core引用：[Core/Src/task/task_chassis/task_chassis_runtime.c](../Core/Src/task/task_chassis/task_chassis_runtime.c) 第 25 行。

### CHASSIS_STICK_DEADBAND_RAW（第 30 行定义）

```c
#define CHASSIS_STICK_DEADBAND_RAW 30 /* 左摇杆中心死区，raw，[0,660)，越大越不敏感。 */
```

严格-阈值<raw<阈值归零；外部raw/660×MAX_SPEED_RPM，不扣死区宽度。

源文件：[Core/Inc/task/task_chassis/task_chassis_config.h](../Core/Inc/task/task_chassis/task_chassis_config.h)。

Core引用：[Core/Src/task/task_chassis/task_chassis_runtime.c](../Core/Src/task/task_chassis/task_chassis_runtime.c) 第 88 行。

### CHASSIS_MAX_SPEED_RPM（第 33 行定义）

```c
#define CHASSIS_MAX_SPEED_RPM 6000.0f /* 转子目标限速，rpm，>0；不是轮轴 rpm，待整定。 */
```

三轴与各轮目标限幅，转子rpm；逐轮混控后钳位，不整体归一化。

源文件：[Core/Inc/task/task_chassis/task_chassis_config.h](../Core/Inc/task/task_chassis/task_chassis_config.h)。

Core引用：[Core/Src/task/task_chassis/task_chassis_runtime.c](../Core/Src/task/task_chassis/task_chassis_runtime.c) 第 83 行。

### CHASSIS_SPEED_KP（第 36 行定义）

```c
#define CHASSIS_SPEED_KP 4.0f /* 比例增益，raw/rpm，>=0；误差约 1229 rpm 时单 P 饱和。 */
```

速度P：Kp×(目标rpm−滤波rpm)，输出raw。

源文件：[Core/Inc/task/task_chassis/task_chassis_config.h](../Core/Inc/task/task_chassis/task_chassis_config.h)。

Core引用：[Core/Src/task/task_chassis/task_chassis_runtime.c](../Core/Src/task/task_chassis/task_chassis_runtime.c) 第 84 行。

### CHASSIS_SPEED_KI（第 39 行定义）

```c
#define CHASSIS_SPEED_KI 2.0f /* 积分增益，raw/(rpm*s)，>=0；增大会加快消除稳态速度误差。 */
```

底盘梯形积分增量Ki×(e+e_prev)/2×dt_s/(1+a×abs(e))，单位raw。

源文件：[Core/Inc/task/task_chassis/task_chassis_config.h](../Core/Inc/task/task_chassis/task_chassis_config.h)。

Core引用：[Core/Src/task/task_chassis/task_chassis_runtime.c](../Core/Src/task/task_chassis/task_chassis_runtime.c) 第 84 行。

### CHASSIS_SPEED_KD（第 42 行定义）

```c
#define CHASSIS_SPEED_KD 0.0f /* 微分增益，raw*s/rpm，>=0；默认关闭，尚未验证噪声。 */
```

速度D：Kd×(本误差−上一误差)/dt_s；首帧微分为零。

源文件：[Core/Inc/task/task_chassis/task_chassis_config.h](../Core/Inc/task/task_chassis/task_chassis_config.h)。

Core引用：[Core/Src/task/task_chassis/task_chassis_runtime.c](../Core/Src/task/task_chassis/task_chassis_runtime.c) 第 84 行。

### CHASSIS_INTEGRAL_LIMIT_RAW（第 45 行定义）

```c
#define CHASSIS_INTEGRAL_LIMIT_RAW 1000.0f /* 积分项上限，raw，[0,实际限流]，防止历史累加。 */
```

底盘四个独立速度PID的积分raw限幅；总电流另行限幅。

源文件：[Core/Inc/task/task_chassis/task_chassis_config.h](../Core/Inc/task/task_chassis/task_chassis_config.h)。

Core引用：[Core/Src/task/task_chassis/task_chassis_runtime.c](../Core/Src/task/task_chassis/task_chassis_runtime.c) 第 85 行。

### CHASSIS_INTEGRAL_ERROR_SCALE（第 48 行定义）

```c
#define CHASSIS_INTEGRAL_ERROR_SCALE 0.01f /* 变速积分 a，1/rpm，>=0；权重 1/(1+a*abs(e))。 */
```

变速积分a，1/rpm；权重1/(1+a×abs(e))，0禁用权重。

源文件：[Core/Inc/task/task_chassis/task_chassis_config.h](../Core/Inc/task/task_chassis/task_chassis_config.h)。

Core引用：[Core/Src/task/task_chassis/task_chassis_runtime.c](../Core/Src/task/task_chassis/task_chassis_runtime.c) 第 86 行。

### CHASSIS_CURRENT_LIMIT_RAW（第 51 行定义）

```c
#define CHASSIS_CURRENT_LIMIT_RAW 4915.0f /* 实际电流限幅，C620 raw，(0,16384]，约 6 A，待实测。 */
```

速度PID实际限流±4915 raw约6A；不得超过C620协议±16384 raw。

源文件：[Core/Inc/task/task_chassis/task_chassis_config.h](../Core/Inc/task/task_chassis/task_chassis_config.h)。

Core引用：[Core/Src/task/task_chassis/task_chassis_runtime.c](../Core/Src/task/task_chassis/task_chassis_runtime.c) 第 87 行。

### CHASSIS_KEYBOARD_T1_MS（第 54 行定义）

```c
#define CHASSIS_KEYBOARD_T1_MS 200U /* 键盘直线段结束时间，ms，>0；值越大起步越慢。 */
```

键盘曲线直线段结束时刻ms，届时达到LINEAR_RATIO；配置换算为秒。

源文件：[Core/Inc/task/task_chassis/task_chassis_config.h](../Core/Inc/task/task_chassis/task_chassis_config.h)。

Core引用：[Core/Src/task/task_chassis/task_chassis_runtime.c](../Core/Src/task/task_chassis/task_chassis_runtime.c) 第 89 行。

### CHASSIS_KEYBOARD_T3_MS（第 57 行定义）

```c
#define CHASSIS_KEYBOARD_T3_MS 1000U /* 键盘加速完成时间，ms，>T1；反向零速段同样用此时长。 */
```

曲线达到终点的时刻ms，反向先用同样时长过零；松键立即零目标。

源文件：[Core/Inc/task/task_chassis/task_chassis_config.h](../Core/Inc/task/task_chassis/task_chassis_config.h)。

Core引用：[Core/Src/task/task_chassis/task_chassis_runtime.c](../Core/Src/task/task_chassis/task_chassis_runtime.c) 第 90 行。

### CHASSIS_KEYBOARD_LINEAR_RATIO（第 60 行定义）

```c
#define CHASSIS_KEYBOARD_LINEAR_RATIO (1.0f / 3.0f) /* T1 时达到目标的比例，(0,1)，影响起步斜率。 */
```

T1时的过渡幅度比例(0,1)，改变起步斜率和对数接入修正。

源文件：[Core/Inc/task/task_chassis/task_chassis_config.h](../Core/Inc/task/task_chassis/task_chassis_config.h)。

Core引用：[Core/Src/task/task_chassis/task_chassis_runtime.c](../Core/Src/task/task_chassis/task_chassis_runtime.c) 第 91 行。

### CHASSIS_KEYBOARD_LOG_SHAPE（第 63 行定义）

```c
#define CHASSIS_KEYBOARD_LOG_SHAPE 4.0f /* 对数曲线形状系数，>0，无量纲；修改后复核单调性。 */
```

平滑对数段形状系数>0；两端三次修正保持值与斜率衔接，须复核单调性。

源文件：[Core/Inc/task/task_chassis/task_chassis_config.h](../Core/Inc/task/task_chassis/task_chassis_config.h)。

Core引用：[Core/Src/task/task_chassis/task_chassis_runtime.c](../Core/Src/task/task_chassis/task_chassis_runtime.c) 第 92 行。

### CHASSIS_ID1_SIGN（第 66 行定义）

```c
#define CHASSIS_ID1_SIGN (-1) /* CAN1 ID1 右前轮安装方向，±1，同时转换反馈与电流。 */
```

安装符号±1，同时变换本轮反馈rpm与发送电流；不是两个独立符号。

源文件：[Core/Inc/task/task_chassis/task_chassis_config.h](../Core/Inc/task/task_chassis/task_chassis_config.h)。

Core引用：[Core/Src/task/task_chassis/task_chassis_runtime.c](../Core/Src/task/task_chassis/task_chassis_runtime.c) 第 16 行。

### CHASSIS_ID2_SIGN（第 69 行定义）

```c
#define CHASSIS_ID2_SIGN 1 /* CAN1 ID2 左前轮安装方向，±1。 */
```

安装符号±1，同时变换本轮反馈rpm与发送电流；不是两个独立符号。

源文件：[Core/Inc/task/task_chassis/task_chassis_config.h](../Core/Inc/task/task_chassis/task_chassis_config.h)。

Core引用：[Core/Src/task/task_chassis/task_chassis_runtime.c](../Core/Src/task/task_chassis/task_chassis_runtime.c) 第 16 行。

### CHASSIS_ID3_SIGN（第 72 行定义）

```c
#define CHASSIS_ID3_SIGN 1 /* CAN1 ID3 左后轮安装方向，±1。 */
```

安装符号±1，同时变换本轮反馈rpm与发送电流；不是两个独立符号。

源文件：[Core/Inc/task/task_chassis/task_chassis_config.h](../Core/Inc/task/task_chassis/task_chassis_config.h)。

Core引用：[Core/Src/task/task_chassis/task_chassis_runtime.c](../Core/Src/task/task_chassis/task_chassis_runtime.c) 第 16 行。

### CHASSIS_ID4_SIGN（第 75 行定义）

```c
#define CHASSIS_ID4_SIGN (-1) /* CAN1 ID4 右后轮安装方向，±1。 */
```

安装符号±1，同时变换本轮反馈rpm与发送电流；不是两个独立符号。

源文件：[Core/Inc/task/task_chassis/task_chassis_config.h](../Core/Inc/task/task_chassis/task_chassis_config.h)。

Core引用：[Core/Src/task/task_chassis/task_chassis_runtime.c](../Core/Src/task/task_chassis/task_chassis_runtime.c) 第 16 行。

## Core/Inc/task/task_chassis/task_chassis_runtime.h

### TASK_CHASSIS_RUNTIME_H（第 9 行定义）

```c
#define TASK_CHASSIS_RUNTIME_H /* 防止重复包含。 */
```

包含保护：避免同一翻译单元重复声明，没有控制算法。

源文件：[Core/Inc/task/task_chassis/task_chassis_runtime.h](../Core/Inc/task/task_chassis/task_chassis_runtime.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

## Core/Inc/task/task_dbus/task_dbus.h

### TASK_DBUS_H（第 16 行定义）

```c
#define TASK_DBUS_H /* 防止 DBUS 任务入口重复声明（不共享轴的控制状态）。 */
```

包含保护：避免同一翻译单元重复声明，没有控制算法。

源文件：[Core/Inc/task/task_dbus/task_dbus.h](../Core/Inc/task/task_dbus/task_dbus.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

## Core/Inc/task/task_dbus/task_dbus_config.h

### TASK_DBUS_CONFIG_H（第 11 行定义）

```c
#define TASK_DBUS_CONFIG_H /* 防止配置重复包含。 */
```

包含保护：避免同一翻译单元重复声明，没有控制算法。

源文件：[Core/Inc/task/task_dbus/task_dbus_config.h](../Core/Inc/task/task_dbus/task_dbus_config.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### DBUS_TASK_PERIOD_MS（第 15 行定义）

```c
#define DBUS_TASK_PERIOD_MS 2U /* 任务周期，ms；修改后检查调度负载。 */
```

绝对唤醒周期；云台按固定配置dt，供弹正式路径测实际Tick差。

源文件：[Core/Inc/task/task_dbus/task_dbus_config.h](../Core/Inc/task/task_dbus/task_dbus_config.h)。

Core引用：[Core/Src/task/task_dbus/task_dbus.c](../Core/Src/task/task_dbus/task_dbus.c) 第 22 行；[Core/Src/task/task_dbus/task_dbus_trace.c](../Core/Src/task/task_dbus/task_dbus_trace.c) 第 212 行。

### DBUS_TASK_LOG_PERIOD_MS（第 16 行定义）

```c
#define DBUS_TASK_LOG_PERIOD_MS 100U /* DBUS 遥测周期，ms；增大会降低日志带宽。 */
```

诊断/提示限频，通常成功提交后续期，不参与电流控制。

源文件：[Core/Inc/task/task_dbus/task_dbus_config.h](../Core/Inc/task/task_dbus/task_dbus_config.h)。

Core引用：[Core/Src/task/task_dbus/task_dbus_runtime.c](../Core/Src/task/task_dbus/task_dbus_runtime.c) 第 64 行。

### DBUS_TRACE_CAPACITY（第 17 行定义）

```c
#define DBUS_TRACE_CAPACITY 128U /* 任务独占诊断环的记录数，16~512；满时丢新记录并计数。 */
```

16~512条任务私有采集记录；满时丢新诊断记录并计数，不改变真实输入。

源文件：[Core/Inc/task/task_dbus/task_dbus_config.h](../Core/Inc/task/task_dbus/task_dbus_config.h)。

Core引用：[Core/Src/task/task_dbus/task_dbus_trace.c](../Core/Src/task/task_dbus/task_dbus_trace.c) 第 41 行。

### DBUS_TRACE_BATCH_BYTES（第 18 行定义）

```c
#define DBUS_TRACE_BATCH_BYTES 704U /* 单次文本批次容量，256~767 字节；小于共用 DMA 缓冲。 */
```

256~767字节文本暂存，必须小于共用日志DMA缓冲；整行组成批次，接受后才出队。

源文件：[Core/Inc/task/task_dbus/task_dbus_config.h](../Core/Inc/task/task_dbus/task_dbus_config.h)。

Core引用：[Core/Src/task/task_dbus/task_dbus_trace.c](../Core/Src/task/task_dbus/task_dbus_trace.c) 第 43 行。

### DBUS_TRACE_HEARTBEAT_MS（第 19 行定义）

```c
#define DBUS_TRACE_HEARTBEAT_MS 100U /* 命令不变时的记录间隔，HAL ms，>0；变化仍逐周期记录。 */
```

HAL ms，正值且小于半个计数周期；无变化时心跳，命令变化仍逐任务周期观察。

源文件：[Core/Inc/task/task_dbus/task_dbus_config.h](../Core/Inc/task/task_dbus/task_dbus_config.h)。

Core引用：[Core/Src/task/task_dbus/task_dbus_trace.c](../Core/Src/task/task_dbus/task_dbus_trace.c) 第 45 行。

### DBUS_TASK_INIT_RETRY_MS（第 20 行定义）

```c
#define DBUS_TASK_INIT_RETRY_MS 100U /* 接收器初始化重试周期，ms；过小会增加故障时 CPU 占用。 */
```

初始化/恢复失败节流，比较HAL年龄或转换Tick后等待。

源文件：[Core/Inc/task/task_dbus/task_dbus_config.h](../Core/Inc/task/task_dbus/task_dbus_config.h)。

Core引用：[Core/Src/task/task_dbus/task_dbus.c](../Core/Src/task/task_dbus/task_dbus.c) 第 36 行。

### DBUS_STICK_DEADBAND_RAW（第 21 行定义）

```c
#define DBUS_STICK_DEADBAND_RAW 10 /* 摇杆回中死区，原始计数；过大会吞掉小动作。 */
```

摇杆|raw|≤阈值输出0；外部按raw×1000/660缩放，不减死区宽度。

源文件：[Core/Inc/task/task_dbus/task_dbus_config.h](../Core/Inc/task/task_dbus/task_dbus_config.h)。

Core引用：[Core/Src/task/task_dbus/task_dbus_runtime.c](../Core/Src/task/task_dbus/task_dbus_runtime.c) 第 26 行；[Core/Src/task/task_dbus/task_dbus_trace.c](../Core/Src/task/task_dbus/task_dbus_trace.c) 第 235 行。

### DBUS_YAW_CHANNEL（第 22 行定义）

```c
#define DBUS_YAW_CHANNEL 0U /* Yaw 摇杆通道，0~3；修改前核对 DBUS 协议映射。 */
```

平台/模板项，按原定义、源注释与引用判断；没有位置或速度控制公式。

源文件：[Core/Inc/task/task_dbus/task_dbus_config.h](../Core/Inc/task/task_dbus/task_dbus_config.h)。

Core引用：[Core/Src/task/task_dbus/task_dbus_runtime.c](../Core/Src/task/task_dbus/task_dbus_runtime.c) 第 28 行；[Core/Src/task/task_dbus/task_dbus_trace.c](../Core/Src/task/task_dbus/task_dbus_trace.c) 第 236 行。

### DBUS_PITCH_CHANNEL（第 23 行定义）

```c
#define DBUS_PITCH_CHANNEL 1U /* Pitch 摇杆通道，0~3；修改前核对 DBUS 协议映射。 */
```

平台/模板项，按原定义、源注释与引用判断；没有位置或速度控制公式。

源文件：[Core/Inc/task/task_dbus/task_dbus_config.h](../Core/Inc/task/task_dbus/task_dbus_config.h)。

Core引用：[Core/Src/task/task_dbus/task_dbus_runtime.c](../Core/Src/task/task_dbus/task_dbus_runtime.c) 第 28 行；[Core/Src/task/task_dbus/task_dbus_trace.c](../Core/Src/task/task_dbus/task_dbus_trace.c) 第 236 行。

### DBUS_YAW_INPUT_SIGN（第 24 行定义）

```c
#define DBUS_YAW_INPUT_SIGN 1 /* Yaw 摇杆方向，+1/-1；方向反时只改此项。 */
```

方向映射±1；输入、反馈和电流独立。C615只记录方向，不用PWM切换正反转。

源文件：[Core/Inc/task/task_dbus/task_dbus_config.h](../Core/Inc/task/task_dbus/task_dbus_config.h)。

Core引用：[Core/Src/task/task_dbus/task_dbus_runtime.c](../Core/Src/task/task_dbus/task_dbus_runtime.c) 第 30 行；[Core/Src/task/task_dbus/task_dbus_trace.c](../Core/Src/task/task_dbus/task_dbus_trace.c) 第 237 行。

### DBUS_PITCH_INPUT_SIGN（第 25 行定义）

```c
#define DBUS_PITCH_INPUT_SIGN 1 /* Pitch 摇杆方向，+1/-1；方向反时只改此项。 */
```

方向映射±1；输入、反馈和电流独立。C615只记录方向，不用PWM切换正反转。

源文件：[Core/Inc/task/task_dbus/task_dbus_config.h](../Core/Inc/task/task_dbus/task_dbus_config.h)。

Core引用：[Core/Src/task/task_dbus/task_dbus_runtime.c](../Core/Src/task/task_dbus/task_dbus_runtime.c) 第 31 行；[Core/Src/task/task_dbus/task_dbus_trace.c](../Core/Src/task/task_dbus/task_dbus_trace.c) 第 237 行。

### DBUS_MOUSE_VIRTUAL_REFERENCE_PERIOD_MS（第 28 行定义）

```c
#define DBUS_MOUSE_VIRTUAL_REFERENCE_PERIOD_MS 14U /* 鼠标测速参考周期，ms，1~INT32_MAX；增大会同比提高速度。 */
```

ms；位移测速的增益参考周期，v=clamp(本批位移×增益×参考周期/接收时间差)。首帧或同毫秒帧用该周期作分母。

源文件：[Core/Inc/task/task_dbus/task_dbus_config.h](../Core/Inc/task/task_dbus/task_dbus_config.h)。

Core引用：[Core/Inc/task/task_dbus/task_dbus_config.h](../Core/Inc/task/task_dbus/task_dbus_config.h) 第 66 行；[Core/Src/task/task_dbus/task_dbus_runtime.c](../Core/Src/task/task_dbus/task_dbus_runtime.c) 第 57 行；[Core/Src/task/task_dbus/task_dbus_trace.c](../Core/Src/task/task_dbus/task_dbus_trace.c) 第 229 行。

### DBUS_YAW_MOUSE_VIRTUAL_GAIN_PERMILLE_PER_COUNT（第 32 行定义）

```c
#define DBUS_YAW_MOUSE_VIRTUAL_GAIN_PERMILLE_PER_COUNT 5.0f /* Yaw 参考周期增益，‰/count，有限非负数；增大会提高灵敏度。 */
```

参考周期下的‰/count增益；新帧按位移×增益×参考周期/接收时间差替换速度，不累计旧速度，小数保留。增大会提高灵敏度和触顶概率。

源文件：[Core/Inc/task/task_dbus/task_dbus_config.h](../Core/Inc/task/task_dbus/task_dbus_config.h)。

Core引用：[Core/Src/task/task_dbus/task_dbus_runtime.c](../Core/Src/task/task_dbus/task_dbus_runtime.c) 第 39 行；[Core/Src/task/task_dbus/task_dbus_trace.c](../Core/Src/task/task_dbus/task_dbus_trace.c) 第 230 行。

### DBUS_YAW_MOUSE_VIRTUAL_HOLD_MS（第 35 行定义）

```c
#define DBUS_YAW_MOUSE_VIRTUAL_HOLD_MS 14U /* Yaw 缺帧保持，ms，可为 0；过小会在正常帧间提前降速。 */
```

无新帧时从本轴最后非零接收时刻保持H毫秒；新零位移帧直接清本轴，另一轴不续期。

源文件：[Core/Inc/task/task_dbus/task_dbus_config.h](../Core/Inc/task/task_dbus/task_dbus_config.h)。

Core引用：[Core/Inc/task/task_dbus/task_dbus_config.h](../Core/Inc/task/task_dbus/task_dbus_config.h) 第 74 行；[Core/Src/task/task_dbus/task_dbus_runtime.c](../Core/Src/task/task_dbus/task_dbus_runtime.c) 第 49 行；[Core/Src/task/task_dbus/task_dbus_trace.c](../Core/Src/task/task_dbus/task_dbus_trace.c) 第 232 行。

### DBUS_YAW_MOUSE_VIRTUAL_DECAY_MS（第 38 行定义）

```c
#define DBUS_YAW_MOUSE_VIRTUAL_DECAY_MS 14U /* Yaw 缺帧回中，ms，>0；增大会延长断续输入的拖尾。 */
```

回中v=v0×(D−(age−H))/D；age≥H+D归零。

源文件：[Core/Inc/task/task_dbus/task_dbus_config.h](../Core/Inc/task/task_dbus/task_dbus_config.h)。

Core引用：[Core/Inc/task/task_dbus/task_dbus_config.h](../Core/Inc/task/task_dbus/task_dbus_config.h) 第 73 行；[Core/Src/task/task_dbus/task_dbus_runtime.c](../Core/Src/task/task_dbus/task_dbus_runtime.c) 第 50 行；[Core/Src/task/task_dbus/task_dbus_trace.c](../Core/Src/task/task_dbus/task_dbus_trace.c) 第 233 行。

### DBUS_YAW_MOUSE_VIRTUAL_SIGN（第 41 行定义）

```c
#define DBUS_YAW_MOUSE_VIRTUAL_SIGN (-1) /* Yaw 鼠标方向，+1/-1；方向反时只改此项。 */
```

方向映射±1；输入、反馈和电流独立。C615只记录方向，不用PWM切换正反转。

源文件：[Core/Inc/task/task_dbus/task_dbus_config.h](../Core/Inc/task/task_dbus/task_dbus_config.h)。

Core引用：[Core/Inc/task/task_dbus/task_dbus_config.h](../Core/Inc/task/task_dbus/task_dbus_config.h) 第 69 行；[Core/Src/task/task_dbus/task_dbus_runtime.c](../Core/Src/task/task_dbus/task_dbus_runtime.c) 第 48 行；[Core/Src/task/task_dbus/task_dbus_trace.c](../Core/Src/task/task_dbus/task_dbus_trace.c) 第 234 行。

### DBUS_PITCH_MOUSE_VIRTUAL_GAIN_PERMILLE_PER_COUNT（第 45 行定义）

```c
#define DBUS_PITCH_MOUSE_VIRTUAL_GAIN_PERMILLE_PER_COUNT 5.0f /* Pitch 参考周期增益，‰/count，有限非负数；增大会提高灵敏度。 */
```

参考周期下的‰/count增益；新帧按位移×增益×参考周期/接收时间差替换速度，不累计旧速度，小数保留。增大会提高灵敏度和触顶概率。

源文件：[Core/Inc/task/task_dbus/task_dbus_config.h](../Core/Inc/task/task_dbus/task_dbus_config.h)。

Core引用：[Core/Src/task/task_dbus/task_dbus_runtime.c](../Core/Src/task/task_dbus/task_dbus_runtime.c) 第 40 行；[Core/Src/task/task_dbus/task_dbus_trace.c](../Core/Src/task/task_dbus/task_dbus_trace.c) 第 231 行。

### DBUS_PITCH_MOUSE_VIRTUAL_HOLD_MS（第 48 行定义）

```c
#define DBUS_PITCH_MOUSE_VIRTUAL_HOLD_MS 14U /* Pitch 缺帧保持，ms，可为 0；过小会在正常帧间提前降速。 */
```

无新帧时从本轴最后非零接收时刻保持H毫秒；新零位移帧直接清本轴，另一轴不续期。

源文件：[Core/Inc/task/task_dbus/task_dbus_config.h](../Core/Inc/task/task_dbus/task_dbus_config.h)。

Core引用：[Core/Inc/task/task_dbus/task_dbus_config.h](../Core/Inc/task/task_dbus/task_dbus_config.h) 第 79 行；[Core/Src/task/task_dbus/task_dbus_runtime.c](../Core/Src/task/task_dbus/task_dbus_runtime.c) 第 54 行；[Core/Src/task/task_dbus/task_dbus_trace.c](../Core/Src/task/task_dbus/task_dbus_trace.c) 第 232 行。

### DBUS_PITCH_MOUSE_VIRTUAL_DECAY_MS（第 51 行定义）

```c
#define DBUS_PITCH_MOUSE_VIRTUAL_DECAY_MS 14U /* Pitch 缺帧回中，ms，>0；增大会延长断续输入的拖尾。 */
```

回中v=v0×(D−(age−H))/D；age≥H+D归零。

源文件：[Core/Inc/task/task_dbus/task_dbus_config.h](../Core/Inc/task/task_dbus/task_dbus_config.h)。

Core引用：[Core/Inc/task/task_dbus/task_dbus_config.h](../Core/Inc/task/task_dbus/task_dbus_config.h) 第 78 行；[Core/Src/task/task_dbus/task_dbus_runtime.c](../Core/Src/task/task_dbus/task_dbus_runtime.c) 第 55 行；[Core/Src/task/task_dbus/task_dbus_trace.c](../Core/Src/task/task_dbus/task_dbus_trace.c) 第 233 行。

### DBUS_PITCH_MOUSE_VIRTUAL_SIGN（第 54 行定义）

```c
#define DBUS_PITCH_MOUSE_VIRTUAL_SIGN 1 /* Pitch 鼠标方向，+1/-1；方向反时只改此项。 */
```

方向映射±1；输入、反馈和电流独立。C615只记录方向，不用PWM切换正反转。

源文件：[Core/Inc/task/task_dbus/task_dbus_config.h](../Core/Inc/task/task_dbus/task_dbus_config.h)。

Core引用：[Core/Inc/task/task_dbus/task_dbus_config.h](../Core/Inc/task/task_dbus/task_dbus_config.h) 第 71 行；[Core/Src/task/task_dbus/task_dbus_runtime.c](../Core/Src/task/task_dbus/task_dbus_runtime.c) 第 53 行；[Core/Src/task/task_dbus/task_dbus_trace.c](../Core/Src/task/task_dbus/task_dbus_trace.c) 第 234 行。

### DBUS_MOUSE_VIRTUAL_ENABLE（第 58 行定义）

```c
#define DBUS_MOUSE_VIRTUAL_ENABLE 1U /* 虚拟鼠标总开关，0/1；异常时可关闭鼠标控制。 */
```

平台/模板项，按原定义、源注释与引用判断；没有位置或速度控制公式。

源文件：[Core/Inc/task/task_dbus/task_dbus_config.h](../Core/Inc/task/task_dbus/task_dbus_config.h)。

Core引用：[Core/Inc/task/task_dbus/task_dbus_config.h](../Core/Inc/task/task_dbus/task_dbus_config.h) 第 64 行；[Core/Src/task/task_dbus/task_dbus_runtime.c](../Core/Src/task/task_dbus/task_dbus_runtime.c) 第 136 行；[Core/Src/task/task_dbus/task_dbus_trace.c](../Core/Src/task/task_dbus/task_dbus_trace.c) 第 150 行。

### DBUS_MOUSE_VIRTUAL_OUTPUT_LIMIT_PERMILLE（第 61 行定义）

```c
#define DBUS_MOUSE_VIRTUAL_OUTPUT_LIMIT_PERMILLE 300 /* 鼠标上限，1~1000‰；两轴满输入40rpm时鼠标目标速度最高12rpm。 */
```

鼠标分量单独限幅±L‰；目标角速度上限为L/1000×轴满输入rpm×6，单位deg/s。摇杆合成后再限幅±1000‰，不限制闭环纠偏。

源文件：[Core/Inc/task/task_dbus/task_dbus_config.h](../Core/Inc/task/task_dbus/task_dbus_config.h)。

Core引用：[Core/Inc/task/task_dbus/task_dbus_config.h](../Core/Inc/task/task_dbus/task_dbus_config.h) 第 83 行；[Core/Src/task/task_dbus/task_dbus_runtime.c](../Core/Src/task/task_dbus/task_dbus_runtime.c) 第 58 行；[Core/Src/task/task_dbus/task_dbus_trace.c](../Core/Src/task/task_dbus/task_dbus_trace.c) 第 235 行。

## Core/Inc/task/task_dbus/task_dbus_runtime.h

### TASK_DBUS_RUNTIME_H（第 14 行定义）

```c
#define TASK_DBUS_RUNTIME_H /* 防止 DBUS 周期接口重复包含。 */
```

包含保护：避免同一翻译单元重复声明，没有控制算法。

源文件：[Core/Inc/task/task_dbus/task_dbus_runtime.h](../Core/Inc/task/task_dbus/task_dbus_runtime.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

## Core/Inc/task/task_dbus/task_dbus_trace.h

### TASK_DBUS_TRACE_H（第 9 行定义）

```c
#define TASK_DBUS_TRACE_H /* 防止输入采集接口重复包含。 */
```

包含保护：避免同一翻译单元重复声明，没有控制算法。

源文件：[Core/Inc/task/task_dbus/task_dbus_trace.h](../Core/Inc/task/task_dbus/task_dbus_trace.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

## Core/Inc/task/task_feed_motor/task_feed_motor.h

### TASK_FEED_MOTOR_H（第 18 行定义）

```c
#define TASK_FEED_MOTOR_H /* 防止供弹任务入口声明被重复包含（避免同一声明出现两次）。 */
```

包含保护：避免同一翻译单元重复声明，没有控制算法。

源文件：[Core/Inc/task/task_feed_motor/task_feed_motor.h](../Core/Inc/task/task_feed_motor/task_feed_motor.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

## Core/Inc/task/task_feed_motor/task_feed_motor_command.h

### TASK_FEED_MOTOR_COMMAND_H（第 13 行定义）

```c
#define TASK_FEED_MOTOR_COMMAND_H /* 防止重复包含。 */
```

包含保护：避免同一翻译单元重复声明，没有控制算法。

源文件：[Core/Inc/task/task_feed_motor/task_feed_motor_command.h](../Core/Inc/task/task_feed_motor/task_feed_motor_command.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

## Core/Inc/task/task_feed_motor/task_feed_motor_config.h

### TASK_FEED_MOTOR_CONFIG_H（第 14 行定义）

```c
#define TASK_FEED_MOTOR_CONFIG_H /* 防止重复包含。 */
```

包含保护：避免同一翻译单元重复声明，没有控制算法。

源文件：[Core/Inc/task/task_feed_motor/task_feed_motor_config.h](../Core/Inc/task/task_feed_motor/task_feed_motor_config.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### FEED_MOTOR_TASK_PERIOD_MS（第 18 行定义）

```c
#define FEED_MOTOR_TASK_PERIOD_MS 2U /* 控制周期，ms；修改后检查调度负载。 */
```

绝对唤醒周期；云台按固定配置dt，供弹正式路径测实际Tick差。

源文件：[Core/Inc/task/task_feed_motor/task_feed_motor_config.h](../Core/Inc/task/task_feed_motor/task_feed_motor_config.h)。

Core引用：[Core/Src/task/task_feed_motor/task_feed_motor.c](../Core/Src/task/task_feed_motor/task_feed_motor.c) 第 72 行；[Core/Src/task/task_feed_motor/task_feed_motor_runtime.c](../Core/Src/task/task_feed_motor/task_feed_motor_runtime.c) 第 51 行。

### FEED_MOTOR_INIT_RETRY_MS（第 19 行定义）

```c
#define FEED_MOTOR_INIT_RETRY_MS 100U /* 初始化重试间隔，ms；避免故障忙等。 */
```

初始化/恢复失败节流，比较HAL年龄或转换Tick后等待。

源文件：[Core/Inc/task/task_feed_motor/task_feed_motor_config.h](../Core/Inc/task/task_feed_motor/task_feed_motor_config.h)。

Core引用：[Core/Src/task/task_feed_motor/task_feed_motor.c](../Core/Src/task/task_feed_motor/task_feed_motor.c) 第 60 行。

### FEED_MOTOR_COMMAND_TIMEOUT_MS（第 20 行定义）

```c
#define FEED_MOTOR_COMMAND_TIMEOUT_MS 100U /* DBUS 命令期限，HAL ms；过期立即停机。 */
```

真实接收时间戳过期：云台冻结目标保持，供弹清除当前状态并输出零电流/停止脉宽。

源文件：[Core/Inc/task/task_feed_motor/task_feed_motor_config.h](../Core/Inc/task/task_feed_motor/task_feed_motor_config.h)。

Core引用：[Core/Src/task/task_feed_motor/task_feed_motor_command.c](../Core/Src/task/task_feed_motor/task_feed_motor_command.c) 第 61 行。

### FEED_MOTOR_ID（第 21 行定义）

```c
#define FEED_MOTOR_ID 1U /* CAN2 C610 ID1；反馈标准帧 0x201，控制标准帧 0x200。 */
```

CAN路由/槽位；GM6020反馈0x204+ID，C610反馈0x200+ID。

源文件：[Core/Inc/task/task_feed_motor/task_feed_motor_config.h](../Core/Inc/task/task_feed_motor/task_feed_motor_config.h)。

Core引用：[Core/Src/task/task_feed_motor/task_feed_motor.c](../Core/Src/task/task_feed_motor/task_feed_motor.c) 第 51 行；[Core/Src/task/task_feed_motor/task_feed_motor_runtime.c](../Core/Src/task/task_feed_motor/task_feed_motor_runtime.c) 第 92 行。

### FEED_MOTOR_STEP_COUNTS（第 23 行定义）

```c
#define FEED_MOTOR_STEP_COUNTS (4000LL) /* 每次供弹的电机转子连续逻辑 count，非零；符号决定目标和逻辑电流方向。 */
```

每次供弹的电机转子连续count增量；目标加法先检查int64边界，符号决定目标比较和逻辑电流方向；等长位移提供反向反馈兜底。

源文件：[Core/Inc/task/task_feed_motor/task_feed_motor_config.h](../Core/Inc/task/task_feed_motor/task_feed_motor_config.h)。

Core引用：[Core/Inc/task/task_feed_motor/task_feed_motor_config.h](../Core/Inc/task/task_feed_motor/task_feed_motor_config.h) 第 73 行；[Core/Src/task/task_feed_motor/task_feed_motor_control.c](../Core/Src/task/task_feed_motor/task_feed_motor_control.c) 第 13 行。

### FEED_MOTOR_CONTINUOUS_PRESS_MS（第 25 行定义）

```c
#define FEED_MOTOR_CONTINUOUS_PRESS_MS 400U /* 长按阈值，ms；短按完成当前步后停止，长按才建立下一步。 */
```

从左键按下沿计FreeRTOS Tick差；达到后仍需每步到位后的零电流停顿结束才能重复。

源文件：[Core/Inc/task/task_feed_motor/task_feed_motor_config.h](../Core/Inc/task/task_feed_motor/task_feed_motor_config.h)。

Core引用：[Core/Inc/task/task_feed_motor/task_feed_motor_config.h](../Core/Inc/task/task_feed_motor/task_feed_motor_config.h) 第 74 行；[Core/Src/task/task_feed_motor/task_feed_motor_control.c](../Core/Src/task/task_feed_motor/task_feed_motor_control.c) 第 14 行。

### FEED_MOTOR_STEP_PAUSE_MS（第 27 行定义）

```c
#define FEED_MOTOR_STEP_PAUSE_MS 100U /* 长按每步完成后的零电流停顿，ms，范围 (0,INT32_MAX]；增大会降低重复频率。 */
```

每步到位周期输出零并记录完成Tick；直到Tick差达到pdMS_TO_TICKS(本停顿)且长按达阈值才建立下一目标。

源文件：[Core/Inc/task/task_feed_motor/task_feed_motor_config.h](../Core/Inc/task/task_feed_motor/task_feed_motor_config.h)。

Core引用：[Core/Inc/task/task_feed_motor/task_feed_motor_config.h](../Core/Inc/task/task_feed_motor/task_feed_motor_config.h) 第 76 行；[Core/Src/task/task_feed_motor/task_feed_motor_control.c](../Core/Src/task/task_feed_motor/task_feed_motor_control.c) 第 16 行。

### FEED_MOTOR_FEED_CURRENT_RAW（第 29 行定义）

```c
#define FEED_MOTOR_FEED_CURRENT_RAW 5000 /* M2006 连续供弹电流，C610 raw；增加会增大力矩和温升。 */
```

STEP阶段固定M2006电流幅值raw；乘步长符号和CURRENT_SIGN得到协议电流。

源文件：[Core/Inc/task/task_feed_motor/task_feed_motor_config.h](../Core/Inc/task/task_feed_motor/task_feed_motor_config.h)。

Core引用：[Core/Inc/task/task_feed_motor/task_feed_motor_config.h](../Core/Inc/task/task_feed_motor/task_feed_motor_config.h) 第 78 行；[Core/Src/task/task_feed_motor/task_feed_motor_control.c](../Core/Src/task/task_feed_motor/task_feed_motor_control.c) 第 18 行。

### FEED_MOTOR_MAX_CURRENT_RAW（第 30 行定义）

```c
#define FEED_MOTOR_MAX_CURRENT_RAW 5000 /* C610 电流安全上限，raw，范围 (0,10000]；用于钳位供弹电流。 */
```

电流raw：C610协议±10000，GM6020±16384；任务进一步限流或指定测试电流。

源文件：[Core/Inc/task/task_feed_motor/task_feed_motor_config.h](../Core/Inc/task/task_feed_motor/task_feed_motor_config.h)。

Core引用：[Core/Inc/task/task_feed_motor/task_feed_motor_config.h](../Core/Inc/task/task_feed_motor/task_feed_motor_config.h) 第 79 行；[Core/Src/task/task_feed_motor/task_feed_motor_control.c](../Core/Src/task/task_feed_motor/task_feed_motor_control.c) 第 19 行。

### FEED_MOTOR_FEEDBACK_SIGN（第 31 行定义）

```c
#define FEED_MOTOR_FEEDBACK_SIGN 1 /* 角度/速度反馈到逻辑坐标的符号，±1；待实测。 */
```

方向映射±1；输入、反馈和电流独立。C615只记录方向，不用PWM切换正反转。

源文件：[Core/Inc/task/task_feed_motor/task_feed_motor_config.h](../Core/Inc/task/task_feed_motor/task_feed_motor_config.h)。

Core引用：[Core/Inc/task/task_feed_motor/task_feed_motor_config.h](../Core/Inc/task/task_feed_motor/task_feed_motor_config.h) 第 81 行；[Core/Src/task/task_feed_motor/task_feed_motor_runtime.c](../Core/Src/task/task_feed_motor/task_feed_motor_runtime.c) 第 183 行。

### FEED_MOTOR_CURRENT_SIGN（第 32 行定义）

```c
#define FEED_MOTOR_CURRENT_SIGN 1 /* 逻辑电流到协议电流的符号，±1；最终电流=步长符号×本符号×电流幅值。 */
```

供弹协议电流=sign(STEP_COUNTS)×FEED_CURRENT_RAW×CURRENT_SIGN；本符号取±1，不能代替步长方向。

源文件：[Core/Inc/task/task_feed_motor/task_feed_motor_config.h](../Core/Inc/task/task_feed_motor/task_feed_motor_config.h)。

Core引用：[Core/Inc/task/task_feed_motor/task_feed_motor_config.h](../Core/Inc/task/task_feed_motor/task_feed_motor_config.h) 第 82 行；[Core/Src/task/task_feed_motor/task_feed_motor_control.c](../Core/Src/task/task_feed_motor/task_feed_motor_control.c) 第 131 行。

### FEED_MOTOR_LOG_PERIOD_MS（第 33 行定义）

```c
#define FEED_MOTOR_LOG_PERIOD_MS 500U /* 诊断间隔，ms；串口繁忙时重试。 */
```

诊断/提示限频，通常成功提交后续期，不参与电流控制。

源文件：[Core/Inc/task/task_feed_motor/task_feed_motor_config.h](../Core/Inc/task/task_feed_motor/task_feed_motor_config.h)。

Core引用：[Core/Src/task/task_feed_motor/task_feed_motor_runtime.c](../Core/Src/task/task_feed_motor/task_feed_motor_runtime.c) 第 196 行。

### FEED_MOTOR_SNAIL_STOP_PULSE_US（第 35 行定义）

```c
#define FEED_MOTOR_SNAIL_STOP_PULSE_US 1000U /* C615 停止脉宽，us；停转仍异常时先重新校准。 */
```

C615脉宽us；停止/活动/范围各有独立参数，1MHz时CCR数字等于us。

源文件：[Core/Inc/task/task_feed_motor/task_feed_motor_config.h](../Core/Inc/task/task_feed_motor/task_feed_motor_config.h)。

Core引用：[Core/Src/task/task_feed_motor/task_feed_motor_runtime.c](../Core/Src/task/task_feed_motor/task_feed_motor_runtime.c) 第 58 行。

### FEED_MOTOR_SNAIL_CH1_DIRECTION_SIGN（第 38 行定义）

```c
#define FEED_MOTOR_SNAIL_CH1_DIRECTION_SIGN 1 /* 正式 CH1 期望转向，+1/-1；实际方向由 C615 相线或 Assistant 设置。 */
```

方向映射±1；输入、反馈和电流独立。C615只记录方向，不用PWM切换正反转。

源文件：[Core/Inc/task/task_feed_motor/task_feed_motor_config.h](../Core/Inc/task/task_feed_motor/task_feed_motor_config.h)。

Core引用：[Core/Src/task/task_feed_motor/task_feed_motor_runtime.c](../Core/Src/task/task_feed_motor/task_feed_motor_runtime.c) 第 54 行。

### FEED_MOTOR_SNAIL_CH2_DIRECTION_SIGN（第 41 行定义）

```c
#define FEED_MOTOR_SNAIL_CH2_DIRECTION_SIGN (-1) /* 正式 CH2 期望转向，+1/-1；实际方向由 C615 相线或 Assistant 设置。 */
```

方向映射±1；输入、反馈和电流独立。C615只记录方向，不用PWM切换正反转。

源文件：[Core/Inc/task/task_feed_motor/task_feed_motor_config.h](../Core/Inc/task/task_feed_motor/task_feed_motor_config.h)。

Core引用：[Core/Src/task/task_feed_motor/task_feed_motor_runtime.c](../Core/Src/task/task_feed_motor/task_feed_motor_runtime.c) 第 56 行。

### FEED_MOTOR_SNAIL_MAX_PULSE_US（第 44 行定义）

```c
#define FEED_MOTOR_SNAIL_MAX_PULSE_US 1550U /* C615 正式运行上限，us；沿用整车例程 FRIC_UP=1550。 */
```

C615脉宽us；停止/活动/范围各有独立参数，1MHz时CCR数字等于us。

源文件：[Core/Inc/task/task_feed_motor/task_feed_motor_config.h](../Core/Inc/task/task_feed_motor/task_feed_motor_config.h)。

Core引用：[Core/Src/task/task_feed_motor/task_feed_motor_runtime.c](../Core/Src/task/task_feed_motor/task_feed_motor_runtime.c) 第 59 行。

### FEED_MOTOR_SNAIL_ACTIVE_PULSE_US（第 47 行定义）

```c
#define FEED_MOTOR_SNAIL_ACTIVE_PULSE_US 1400U /* C615 正式活动脉宽，us，范围 (STOP,MAX]；增大可能提高转速和温升。 */
```

C615脉宽us；停止/活动/范围各有独立参数，1MHz时CCR数字等于us。

源文件：[Core/Inc/task/task_feed_motor/task_feed_motor_config.h](../Core/Inc/task/task_feed_motor/task_feed_motor_config.h)。

Core引用：[Core/Src/task/task_feed_motor/task_feed_motor_runtime.c](../Core/Src/task/task_feed_motor/task_feed_motor_runtime.c) 第 60 行。

### FEED_MOTOR_SNAIL_RAMP_TIME_MS（第 50 行定义）

```c
#define FEED_MOTOR_SNAIL_RAMP_TIME_MS 10U /* C615 启动 Ramp 时间，ms，须为正；松键停止不等待 Ramp。 */
```

正式C615启动速率=(活动−停止)×1000/RAMP_MS；Shift释放和故障直接停止，不等待Ramp。

源文件：[Core/Inc/task/task_feed_motor/task_feed_motor_config.h](../Core/Inc/task/task_feed_motor/task_feed_motor_config.h)。

Core引用：[Core/Src/task/task_feed_motor/task_feed_motor_runtime.c](../Core/Src/task/task_feed_motor/task_feed_motor_runtime.c) 第 52 行。

### FEED_MOTOR_ANGLE_STEP_TEST_ENABLE（第 56 行定义）

```c
#define FEED_MOTOR_ANGLE_STEP_TEST_ENABLE 0 /* C610 手动角度步长测试开关，0/1；开启后只读测量，完成后恢复 0。 */
```

编译期正式/测试路径选择，模式不在运行中切换；互斥限制见源#if。

源文件：[Core/Inc/task/task_feed_motor/task_feed_motor_config.h](../Core/Inc/task/task_feed_motor/task_feed_motor_config.h)。

Core引用：[Core/Inc/task/task_feed_motor/task_feed_motor_config.h](../Core/Inc/task/task_feed_motor/task_feed_motor_config.h) 第 65 行；[Core/Src/task/task_feed_motor/task_feed_motor.c](../Core/Src/task/task_feed_motor/task_feed_motor.c) 第 29 行。

### FEED_MOTOR_ANGLE_STEP_TEST_FEEDBACK_SIGN（第 59 行定义）

```c
#define FEED_MOTOR_ANGLE_STEP_TEST_FEEDBACK_SIGN (-1) /* 测试反馈方向，±1；与正式 FEED_MOTOR_FEEDBACK_SIGN 独立。 */
```

方向映射±1；输入、反馈和电流独立。C615只记录方向，不用PWM切换正反转。

源文件：[Core/Inc/task/task_feed_motor/task_feed_motor_config.h](../Core/Inc/task/task_feed_motor/task_feed_motor_config.h)。

Core引用：[Core/Inc/task/task_feed_motor/task_feed_motor_config.h](../Core/Inc/task/task_feed_motor/task_feed_motor_config.h) 第 68 行；[Core/Src/task/task_feed_motor/task_feed_motor.c](../Core/Src/task/task_feed_motor/task_feed_motor.c) 第 46 行。

### FEED_MOTOR_ANGLE_STEP_TEST_LOG_PERIOD_MS（第 62 行定义）

```c
#define FEED_MOTOR_ANGLE_STEP_TEST_LOG_PERIOD_MS 100U /* 当前角度日志间隔，ms；成功提交后再计时。 */
```

诊断/提示限频，通常成功提交后续期，不参与电流控制。

源文件：[Core/Inc/task/task_feed_motor/task_feed_motor_config.h](../Core/Inc/task/task_feed_motor/task_feed_motor_config.h)。

Core引用：[Core/Inc/task/task_feed_motor/task_feed_motor_config.h](../Core/Inc/task/task_feed_motor/task_feed_motor_config.h) 第 70 行；[Core/Src/task/task_feed_motor/task_feed_motor.c](../Core/Src/task/task_feed_motor/task_feed_motor.c) 第 47 行。

## Core/Inc/task/task_feed_motor/task_feed_motor_control.h

### TASK_FEED_MOTOR_CONTROL_H（第 11 行定义）

```c
#define TASK_FEED_MOTOR_CONTROL_H /* 防止重复包含。 */
```

包含保护：避免同一翻译单元重复声明，没有控制算法。

源文件：[Core/Inc/task/task_feed_motor/task_feed_motor_control.h](../Core/Inc/task/task_feed_motor/task_feed_motor_control.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

## Core/Inc/task/task_feed_motor/task_feed_motor_runtime.h

### TASK_FEED_MOTOR_RUNTIME_H（第 16 行定义）

```c
#define TASK_FEED_MOTOR_RUNTIME_H /* 防止重复包含。 */
```

包含保护：避免同一翻译单元重复声明，没有控制算法。

源文件：[Core/Inc/task/task_feed_motor/task_feed_motor_runtime.h](../Core/Inc/task/task_feed_motor/task_feed_motor_runtime.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

## Core/Inc/task/task_ins/task_ins.h

### TASK_INS_H（第 7 行定义）

```c
#define TASK_INS_H /* 防止重复包含。 */
```

包含保护：避免同一翻译单元重复声明，没有控制算法。

源文件：[Core/Inc/task/task_ins/task_ins.h](../Core/Inc/task/task_ins/task_ins.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

## Core/Inc/task/task_ins/task_ins_config.h

### TASK_INS_CONFIG_H（第 9 行定义）

```c
#define TASK_INS_CONFIG_H /* 防止重复包含。 */
```

包含保护：避免同一翻译单元重复声明，没有控制算法。

源文件：[Core/Inc/task/task_ins/task_ins_config.h](../Core/Inc/task/task_ins/task_ins_config.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### INS_MAG_ENABLE（第 12 行定义）

```c
#define INS_MAG_ENABLE 0U /* 磁采集总开关，0/1；0跳过IST事务/中断通知/硬件检查并禁止磁融合。 */
```

0/1磁采集总门；0跳过IST事务、DRDY通知及I2C/IST中断硬件检查，并强制有效磁融合许可为0，保留标定参数。MotionCal须同时开采集。

源文件：[Core/Inc/task/task_ins/task_ins_config.h](../Core/Inc/task/task_ins/task_ins_config.h)。

Core引用：[Core/Inc/task/task_ins/task_ins_config.h](../Core/Inc/task/task_ins/task_ins_config.h) 第 126 行；[Core/Src/task/task_ins/task_ins_runtime.c](../Core/Src/task/task_ins/task_ins_runtime.c) 第 96 行。

### INS_MOUNT_CONFIRMED（第 15 行定义）

```c
#define INS_MOUNT_CONFIRMED 1U /* 0/1；1允许发布READY姿态供复测，0强制无效；本开关不代表物理三轴已验收。 */
```

无量纲0/1发布门；0强制姿态无效，1允许通过READY和数据检查的姿态供使用或方向复测。本开关不自动证明物理三轴验收，当前实测状态见INS模块。

源文件：[Core/Inc/task/task_ins/task_ins_config.h](../Core/Inc/task/task_ins/task_ins_config.h)。

Core引用：[Core/Inc/task/task_ins/task_ins_config.h](../Core/Inc/task/task_ins/task_ins_config.h) 第 126 行；[Core/Src/task/task_ins/task_ins_runtime.c](../Core/Src/task/task_ins/task_ins_runtime.c) 第 116 行。

### INS_BMI_ROTATION（第 18 行定义）

```c
#define INS_BMI_ROTATION {{-1.0f,0.0f,0.0f},{0.0f,1.0f,0.0f},{0.0f,0.0f,-1.0f}} /* 芯片→前右下，无量纲正交矩阵，det=+1；同步转换比力和角速度。 */
```

BMI088芯片→云台前右下的无量纲正交矩阵，须det=+1；同时转换比力和角速度，实测来源及待复核方向见INS模块。

源文件：[Core/Inc/task/task_ins/task_ins_config.h](../Core/Inc/task/task_ins/task_ins_config.h)。

Core引用：[Core/Src/task/task_ins/task_ins_runtime.c](../Core/Src/task/task_ins/task_ins_runtime.c) 第 115 行。

### INS_MAG_ROTATION（第 21 行定义）

```c
#define INS_MAG_ROTATION {{1.0f,0.0f,0.0f},{0.0f,-1.0f,0.0f},{0.0f,0.0f,-1.0f}} /* 磁芯片→前右下，无量纲正交矩阵，det=+1；决定磁航向反馈方向。 */
```

IST8310芯片→云台前右下的无量纲正交矩阵，须det=+1；磁标定后使用，必须独立确认，不能照抄BMI088。

源文件：[Core/Inc/task/task_ins/task_ins_config.h](../Core/Inc/task/task_ins/task_ins_config.h)。

Core引用：[Core/Src/task/task_ins/task_ins_runtime.c](../Core/Src/task/task_ins/task_ins_runtime.c) 第 115 行。

### INS_MAG_CALIBRATED（第 24 行定义）

```c
#define INS_MAG_CALIBRATED 1U /* 保留的磁校正确认门，0/1；仅INS_MAG_ENABLE=1且非MotionCal模式时允许磁融合，不代表质量已验收。 */
```

先在磁芯片坐标减μT硬铁偏置并乘对称软铁矩阵，再安装旋转；有效磁许可还要求INS_MAG_ENABLE=1且非MotionCal模式，当前采集关闭时仅保留参数。

源文件：[Core/Inc/task/task_ins/task_ins_config.h](../Core/Inc/task/task_ins/task_ins_config.h)。

Core引用：[Core/Inc/task/task_ins/task_ins_config.h](../Core/Inc/task/task_ins/task_ins_config.h) 第 126 行；[Core/Src/task/task_ins/task_ins_runtime.c](../Core/Src/task/task_ins/task_ins_runtime.c) 第 117 行。

### INS_MAG_BIAS_UT（第 27 行定义）

```c
#define INS_MAG_BIAS_UT {29.3624f,16.6561f,38.2947f} /* 芯片系硬铁偏置，μT；先减偏置，再应用软铁矩阵。 */
```

先在磁芯片坐标减μT硬铁偏置并乘对称软铁矩阵，再安装旋转；有效磁许可还要求INS_MAG_ENABLE=1且非MotionCal模式，当前采集关闭时仅保留参数。

源文件：[Core/Inc/task/task_ins/task_ins_config.h](../Core/Inc/task/task_ins/task_ins_config.h)。

Core引用：[Core/Src/task/task_ins/task_ins_runtime.c](../Core/Src/task/task_ins/task_ins_runtime.c) 第 115 行。

### INS_MAG_SOFTIRON（第 30 行定义）

```c
#define INS_MAG_SOFTIRON {{0.984267f,0.031144f,-0.071657f},{0.031144f,1.086981f,0.040221f},{-0.071657f,0.040221f,0.928752f}} /* 软铁校正矩阵，无量纲、对称正定；在芯片系减偏置后左乘。 */
```

先在磁芯片坐标减μT硬铁偏置并乘对称软铁矩阵，再安装旋转；有效磁许可还要求INS_MAG_ENABLE=1且非MotionCal模式，当前采集关闭时仅保留参数。

源文件：[Core/Inc/task/task_ins/task_ins_config.h](../Core/Inc/task/task_ins/task_ins_config.h)。

Core引用：[Core/Src/task/task_ins/task_ins_runtime.c](../Core/Src/task/task_ins/task_ins_runtime.c) 第 116 行。

### INS_STILL_MS（第 33 行定义）

```c
#define INS_STILL_MS 2000U /* 连续静止窗口，ms，1~60000；运动或过期会重开。 */
```

连续静止窗口的时间、角速度/比力和每轴方差门；运动/过期/方差不合格重开，完成后估计机体系零偏。

源文件：[Core/Inc/task/task_ins/task_ins_config.h](../Core/Inc/task/task_ins/task_ins_config.h)。

Core引用：[Core/Src/task/task_ins/task_ins_runtime.c](../Core/Src/task/task_ins/task_ins_runtime.c) 第 118 行。

### INS_STILL_GYRO_RAD_S（第 36 行定义）

```c
#define INS_STILL_GYRO_RAD_S 0.05f /* 静止角速度模长上限，rad/s，>0；不能识别阈值内恒速慢转。 */
```

连续静止窗口的时间、角速度/比力和每轴方差门；运动/过期/方差不合格重开，完成后估计机体系零偏。

源文件：[Core/Inc/task/task_ins/task_ins_config.h](../Core/Inc/task/task_ins/task_ins_config.h)。

Core引用：[Core/Src/task/task_ins/task_ins_runtime.c](../Core/Src/task/task_ins/task_ins_runtime.c) 第 118 行。

### INS_STILL_ACCEL_FRACTION（第 39 行定义）

```c
#define INS_STILL_ACCEL_FRACTION 0.05f /* 静止比力相对 g 容差，比例，>0；振动大时应先处理安装。 */
```

连续静止窗口的时间、角速度/比力和每轴方差门；运动/过期/方差不合格重开，完成后估计机体系零偏。

源文件：[Core/Inc/task/task_ins/task_ins_config.h](../Core/Inc/task/task_ins/task_ins_config.h)。

Core引用：[Core/Src/task/task_ins/task_ins_runtime.c](../Core/Src/task/task_ins/task_ins_runtime.c) 第 118 行。

### INS_STILL_GYRO_STD_RAD_S（第 42 行定义）

```c
#define INS_STILL_GYRO_STD_RAD_S 0.005f /* 窗口每轴标准差上限，rad/s，>0；越小越难完成。 */
```

连续静止窗口的时间、角速度/比力和每轴方差门；运动/过期/方差不合格重开，完成后估计机体系零偏。

源文件：[Core/Inc/task/task_ins/task_ins_config.h](../Core/Inc/task/task_ins/task_ins_config.h)。

Core引用：[Core/Src/task/task_ins/task_ins_runtime.c](../Core/Src/task/task_ins/task_ins_runtime.c) 第 119 行。

### INS_AHRS_KP（第 45 行定义）

```c
#define INS_AHRS_KP 0.5f /* Mahony 比例增益，1/s，>=0；越大重力/航向收敛越快。 */
```

AHRS实际dt积分、重力降权及磁模长/创新/速率/恢复门；精确单位、范围与副作用见宏原文和INS说明。

源文件：[Core/Inc/task/task_ins/task_ins_config.h](../Core/Inc/task/task_ins/task_ins_config.h)。

Core引用：[Core/Src/task/task_ins/task_ins_runtime.c](../Core/Src/task/task_ins/task_ins_runtime.c) 第 110 行。

### INS_AHRS_KI（第 48 行定义）

```c
#define INS_AHRS_KI 0.0f /* Mahony 重力反馈积分增益，1/s²，>=0；0 禁用，不替代启动零偏。 */
```

AHRS实际dt积分、重力降权及磁模长/创新/速率/恢复门；精确单位、范围与副作用见宏原文和INS说明。

源文件：[Core/Inc/task/task_ins/task_ins_config.h](../Core/Inc/task/task_ins/task_ins_config.h)。

Core引用：[Core/Src/task/task_ins/task_ins_runtime.c](../Core/Src/task/task_ins/task_ins_runtime.c) 第 110 行。

### INS_AHRS_INTEGRAL_LIMIT_RAD_S（第 51 行定义）

```c
#define INS_AHRS_INTEGRAL_LIMIT_RAD_S 0.05f /* 每轴重力积分补偿上限，rad/s，>=0。 */
```

AHRS实际dt积分、重力降权及磁模长/创新/速率/恢复门；精确单位、范围与副作用见宏原文和INS说明。

源文件：[Core/Inc/task/task_ins/task_ins_config.h](../Core/Inc/task/task_ins/task_ins_config.h)。

Core引用：[Core/Src/task/task_ins/task_ins_runtime.c](../Core/Src/task/task_ins/task_ins_runtime.c) 第 110 行。

### INS_GRAVITY_MPS2（第 54 行定义）

```c
#define INS_GRAVITY_MPS2 9.80665f /* 标准重力，m/s²，>0；水平静止机体系应为 [0,0,-g]。 */
```

AHRS实际dt积分、重力降权及磁模长/创新/速率/恢复门；精确单位、范围与副作用见宏原文和INS说明。

源文件：[Core/Inc/task/task_ins/task_ins_config.h](../Core/Inc/task/task_ins/task_ins_config.h)。

Core引用：[Core/Src/task/task_ins/task_ins_runtime.c](../Core/Src/task/task_ins/task_ins_runtime.c) 第 111 行。

### INS_ACCEL_FULL_FRACTION（第 57 行定义）

```c
#define INS_ACCEL_FULL_FRACTION 0.05f /* 模长偏离 g 不超过此比例时全权重，>=0。 */
```

AHRS实际dt积分、重力降权及磁模长/创新/速率/恢复门；精确单位、范围与副作用见宏原文和INS说明。

源文件：[Core/Inc/task/task_ins/task_ins_config.h](../Core/Inc/task/task_ins/task_ins_config.h)。

Core引用：[Core/Src/task/task_ins/task_ins_runtime.c](../Core/Src/task/task_ins/task_ins_runtime.c) 第 111 行。

### INS_ACCEL_REJECT_FRACTION（第 60 行定义）

```c
#define INS_ACCEL_REJECT_FRACTION 0.20f /* 模长偏差达到此比例时拒绝重力校正，须大于全权重阈值。 */
```

AHRS实际dt积分、重力降权及磁模长/创新/速率/恢复门；精确单位、范围与副作用见宏原文和INS说明。

源文件：[Core/Inc/task/task_ins/task_ins_config.h](../Core/Inc/task/task_ins/task_ins_config.h)。

Core引用：[Core/Src/task/task_ins/task_ins_runtime.c](../Core/Src/task/task_ins/task_ins_runtime.c) 第 112 行。

### INS_MAG_MIN_UT（第 63 行定义）

```c
#define INS_MAG_MIN_UT 10.0f /* 校正磁场模长下限，μT，>0；过小不参与融合。 */
```

AHRS实际dt积分、重力降权及磁模长/创新/速率/恢复门；精确单位、范围与副作用见宏原文和INS说明。

源文件：[Core/Inc/task/task_ins/task_ins_config.h](../Core/Inc/task/task_ins/task_ins_config.h)。

Core引用：[Core/Src/task/task_ins/task_ins_runtime.c](../Core/Src/task/task_ins/task_ins_runtime.c) 第 112 行。

### INS_MAG_MAX_UT（第 66 行定义）

```c
#define INS_MAG_MAX_UT 100.0f /* 校正磁场模长上限，μT，须大于下限；不能证明范围内没有干扰。 */
```

AHRS实际dt积分、重力降权及磁模长/创新/速率/恢复门；精确单位、范围与副作用见宏原文和INS说明。

源文件：[Core/Inc/task/task_ins/task_ins_config.h](../Core/Inc/task/task_ins/task_ins_config.h)。

Core引用：[Core/Src/task/task_ins/task_ins_runtime.c](../Core/Src/task/task_ins/task_ins_runtime.c) 第 112 行。

### INS_MAG_RELATIVE_TOLERANCE（第 69 行定义）

```c
#define INS_MAG_RELATIVE_TOLERANCE 0.25f /* 相对初始场强容差，比例 (0,1)，超出回退六轴。 */
```

AHRS实际dt积分、重力降权及磁模长/创新/速率/恢复门；精确单位、范围与副作用见宏原文和INS说明。

源文件：[Core/Inc/task/task_ins/task_ins_config.h](../Core/Inc/task/task_ins/task_ins_config.h)。

Core引用：[Core/Src/task/task_ins/task_ins_runtime.c](../Core/Src/task/task_ins/task_ins_runtime.c) 第 113 行。

### INS_MAG_INNOVATION_RAD（第 72 行定义）

```c
#define INS_MAG_INNOVATION_RAD 0.7853981634f /* 最大航向创新，rad，默认45°；更大差异被当作异常。 */
```

AHRS实际dt积分、重力降权及磁模长/创新/速率/恢复门；精确单位、范围与副作用见宏原文和INS说明。

源文件：[Core/Inc/task/task_ins/task_ins_config.h](../Core/Inc/task/task_ins/task_ins_config.h)。

Core引用：[Core/Src/task/task_ins/task_ins_runtime.c](../Core/Src/task/task_ins/task_ins_runtime.c) 第 113 行。

### INS_MAG_RATE_LIMIT_RAD_S（第 75 行定义）

```c
#define INS_MAG_RATE_LIMIT_RAD_S 0.0523598776f /* 最大磁航向修正速度，rad/s，默认3°/s；>0。 */
```

AHRS实际dt积分、重力降权及磁模长/创新/速率/恢复门；精确单位、范围与副作用见宏原文和INS说明。

源文件：[Core/Inc/task/task_ins/task_ins_config.h](../Core/Inc/task/task_ins/task_ins_config.h)。

Core引用：[Core/Src/task/task_ins/task_ins_runtime.c](../Core/Src/task/task_ins/task_ins_runtime.c) 第 114 行。

### INS_MAG_RECOVERY_S（第 78 行定义）

```c
#define INS_MAG_RECOVERY_S 2.0f /* 磁观测恢复到全权重时间，s，>0；期间不重置四元数。 */
```

AHRS实际dt积分、重力降权及磁模长/创新/速率/恢复门；精确单位、范围与副作用见宏原文和INS说明。

源文件：[Core/Inc/task/task_ins/task_ins_config.h](../Core/Inc/task/task_ins/task_ins_config.h)。

Core引用：[Core/Src/task/task_ins/task_ins_runtime.c](../Core/Src/task/task_ins/task_ins_runtime.c) 第 114 行。

### INS_MAX_DT_S（第 81 行定义）

```c
#define INS_MAX_DT_S 0.01f /* 最大积分间隔，s，(0,0.1]；超过重启静止标定，不补积分。 */
```

AHRS实际dt积分、重力降权及磁模长/创新/速率/恢复门；精确单位、范围与副作用见宏原文和INS说明。

源文件：[Core/Inc/task/task_ins/task_ins_config.h](../Core/Inc/task/task_ins/task_ins_config.h)。

Core引用：[Core/Src/task/task_ins/task_ins_runtime.c](../Core/Src/task/task_ins/task_ins_runtime.c) 第 114 行。

### INS_GYRO_TIMEOUT_MS（第 84 行定义）

```c
#define INS_GYRO_TIMEOUT_MS 10U /* 陀螺新鲜度和等待上限，HAL ms，>0；超时使姿态无效。 */
```

INS采集新鲜度、事务超时、重试、事件或日志周期；HAL ms与FreeRTOS Tick分别使用，不能互相相减。

源文件：[Core/Inc/task/task_ins/task_ins_config.h](../Core/Inc/task/task_ins/task_ins_config.h)。

Core引用：[Core/Src/task/task_ins/task_ins_runtime.c](../Core/Src/task/task_ins/task_ins_runtime.c) 第 92 行。

### INS_ACCEL_TIMEOUT_MS（第 87 行定义）

```c
#define INS_ACCEL_TIMEOUT_MS 10U /* 加速度新鲜度上限，HAL ms，>0；超时姿态无效并重新初始化。 */
```

INS采集新鲜度、事务超时、重试、事件或日志周期；HAL ms与FreeRTOS Tick分别使用，不能互相相减。

源文件：[Core/Inc/task/task_ins/task_ins_config.h](../Core/Inc/task/task_ins/task_ins_config.h)。

Core引用：[Core/Src/task/task_ins/task_ins_runtime.c](../Core/Src/task/task_ins/task_ins_runtime.c) 第 120 行。

### INS_MAG_TIMEOUT_MS（第 90 行定义）

```c
#define INS_MAG_TIMEOUT_MS 50U /* 磁场新鲜度上限，HAL ms，>0；超时回退六轴，不刷新旧样本时间。 */
```

INS采集新鲜度、事务超时、重试、事件或日志周期；HAL ms与FreeRTOS Tick分别使用，不能互相相减。

源文件：[Core/Inc/task/task_ins/task_ins_config.h](../Core/Inc/task/task_ins/task_ins_config.h)。

Core引用：[Core/Src/task/task_ins/task_ins_runtime.c](../Core/Src/task/task_ins/task_ins_runtime.c) 第 120 行。

### INS_DMA_TIMEOUT_MS（第 93 行定义）

```c
#define INS_DMA_TIMEOUT_MS 2U /* 单次 SPI DMA 最大等待，ms，>0；超时中止 SPI 并重启。 */
```

INS采集新鲜度、事务超时、重试、事件或日志周期；HAL ms与FreeRTOS Tick分别使用，不能互相相减。

源文件：[Core/Inc/task/task_ins/task_ins_config.h](../Core/Inc/task/task_ins/task_ins_config.h)。

Core引用：[Core/Src/task/task_ins/task_ins_runtime.c](../Core/Src/task/task_ins/task_ins_runtime.c) 第 90 行。

### INS_SPI_TIMEOUT_MS（第 96 行定义）

```c
#define INS_SPI_TIMEOUT_MS 5U /* 初始化单次 SPI HAL 超时，ms，>0；正常采集不使用轮询。 */
```

INS采集新鲜度、事务超时、重试、事件或日志周期；HAL ms与FreeRTOS Tick分别使用，不能互相相减。

源文件：[Core/Inc/task/task_ins/task_ins_config.h](../Core/Inc/task/task_ins/task_ins_config.h)。

Core引用：[Core/Src/task/task_ins/task_ins_runtime.c](../Core/Src/task/task_ins/task_ins_runtime.c) 第 230 行。

### INS_I2C_TIMEOUT_MS（第 99 行定义）

```c
#define INS_I2C_TIMEOUT_MS 1U /* 单次 I2C HAL 超时，ms，>0；失败后退避，避免长期占用 CPU。 */
```

INS采集新鲜度、事务超时、重试、事件或日志周期；HAL ms与FreeRTOS Tick分别使用，不能互相相减。

源文件：[Core/Inc/task/task_ins/task_ins_config.h](../Core/Inc/task/task_ins/task_ins_config.h)。

Core引用：[Core/Src/task/task_ins/task_ins_runtime.c](../Core/Src/task/task_ins/task_ins_runtime.c) 第 257 行。

### INS_RETRY_MS（第 102 行定义）

```c
#define INS_RETRY_MS 1000U /* 初始化/掉线重试间隔，ms，>0；用 FreeRTOS Tick 调度。 */
```

INS采集新鲜度、事务超时、重试、事件或日志周期；HAL ms与FreeRTOS Tick分别使用，不能互相相减。

源文件：[Core/Inc/task/task_ins/task_ins_config.h](../Core/Inc/task/task_ins/task_ins_config.h)。

Core引用：[Core/Src/task/task_ins/task_ins.c](../Core/Src/task/task_ins/task_ins.c) 第 15 行；[Core/Src/task/task_ins/task_ins_runtime.c](../Core/Src/task/task_ins/task_ins_runtime.c) 第 91 行。

### INS_TEMPERATURE_PERIOD_MS（第 105 行定义）

```c
#define INS_TEMPERATURE_PERIOD_MS 1000U /* 温度读取间隔，HAL ms，>0；传感器本身约1.28s更新。 */
```

INS采集新鲜度、事务超时、重试、事件或日志周期；HAL ms与FreeRTOS Tick分别使用，不能互相相减。

源文件：[Core/Inc/task/task_ins/task_ins_config.h](../Core/Inc/task/task_ins/task_ins_config.h)。

Core引用：[Core/Src/task/task_ins/task_ins_runtime.c](../Core/Src/task/task_ins/task_ins_runtime.c) 第 93 行。

### INS_LOG_PERIOD_MS（第 108 行定义）

```c
#define INS_LOG_PERIOD_MS 1000U /* 姿态状态日志间隔，ms，>0；9600 baud 下禁止逐帧打印。 */
```

INS采集新鲜度、事务超时、重试、事件或日志周期；HAL ms与FreeRTOS Tick分别使用，不能互相相减。

源文件：[Core/Inc/task/task_ins/task_ins_config.h](../Core/Inc/task/task_ins/task_ins_config.h)。

Core引用：[Core/Src/task/task_ins/task_ins_runtime.c](../Core/Src/task/task_ins/task_ins_runtime.c) 第 93 行。

### INS_MAG_CALIBRATION_PERIOD_MS（第 111 行定义）

```c
#define INS_MAG_CALIBRATION_PERIOD_MS 100U /* MotionCal原始磁场输出间隔，HAL ms，>=100；最多10Hz，仅标定模式使用。 */
```

MotionCal原始磁帧的最小发送尝试间隔，HAL ms、>=100；新鲜且未重复才发，忙时丢帧，不补发队列。

源文件：[Core/Inc/task/task_ins/task_ins_config.h](../Core/Inc/task/task_ins/task_ins_config.h)。

Core引用：[Core/Src/task/task_ins/task_ins_runtime.c](../Core/Src/task/task_ins/task_ins_runtime.c) 第 95 行。

### INS_ACCEL_RANGE_REG（第 114 行定义）

```c
#define INS_ACCEL_RANGE_REG 1U /* BMI088 ACC_RANGE 寄存器编码0~3，默认1=±6g。 */
```

BMI088量程/滤波/ODR的寄存器编码；改变后须重新核对物理量、DRDY频率与执行预算。

源文件：[Core/Inc/task/task_ins/task_ins_config.h](../Core/Inc/task/task_ins/task_ins_config.h)。

Core引用：[Core/Src/task/task_ins/task_ins_runtime.c](../Core/Src/task/task_ins/task_ins_runtime.c) 第 231 行。

### INS_ACCEL_CONF_REG（第 117 行定义）

```c
#define INS_ACCEL_CONF_REG 0xABU /* ACC_CONF，默认normal/800Hz；高半字节8~10、低半字节5~12。 */
```

BMI088量程/滤波/ODR的寄存器编码；改变后须重新核对物理量、DRDY频率与执行预算。

源文件：[Core/Inc/task/task_ins/task_ins_config.h](../Core/Inc/task/task_ins/task_ins_config.h)。

Core引用：[Core/Src/task/task_ins/task_ins_runtime.c](../Core/Src/task/task_ins/task_ins_runtime.c) 第 231 行。

### INS_GYRO_RANGE_REG（第 120 行定义）

```c
#define INS_GYRO_RANGE_REG 0U /* GYRO_RANGE 编码0~4，默认0=±2000°/s。 */
```

BMI088量程/滤波/ODR的寄存器编码；改变后须重新核对物理量、DRDY频率与执行预算。

源文件：[Core/Inc/task/task_ins/task_ins_config.h](../Core/Inc/task/task_ins/task_ins_config.h)。

Core引用：[Core/Src/task/task_ins/task_ins_runtime.c](../Core/Src/task/task_ins/task_ins_runtime.c) 第 231 行。

### INS_GYRO_BANDWIDTH_REG（第 123 行定义）

```c
#define INS_GYRO_BANDWIDTH_REG 2U /* GYRO_BANDWIDTH 编码0~7，默认2=1000Hz/116Hz。 */
```

BMI088量程/滤波/ODR的寄存器编码；改变后须重新核对物理量、DRDY频率与执行预算。

源文件：[Core/Inc/task/task_ins/task_ins_config.h](../Core/Inc/task/task_ins/task_ins_config.h)。

Core引用：[Core/Src/task/task_ins/task_ins_runtime.c](../Core/Src/task/task_ins/task_ins_runtime.c) 第 232 行。

## Core/Inc/task/task_ins/task_ins_runtime.h

### TASK_INS_RUNTIME_H（第 7 行定义）

```c
#define TASK_INS_RUNTIME_H /* 防止重复包含。 */
```

包含保护：避免同一翻译单元重复声明，没有控制算法。

源文件：[Core/Inc/task/task_ins/task_ins_runtime.h](../Core/Inc/task/task_ins/task_ins_runtime.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

## Core/Inc/task/task_pitch/task_pitch.h

### TASK_PITCH_H（第 18 行定义）

```c
#define TASK_PITCH_H /* 防止 Pitch 任务入口声明被重复包含（避免同一函数声明出现两次）。 */
```

包含保护：避免同一翻译单元重复声明，没有控制算法。

源文件：[Core/Inc/task/task_pitch/task_pitch.h](../Core/Inc/task/task_pitch/task_pitch.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

## Core/Inc/task/task_pitch/task_pitch_command.h

### TASK_PITCH_COMMAND_H（第 14 行定义）

```c
#define TASK_PITCH_COMMAND_H /* 防止 Pitch 命令类型重复定义。 */
```

包含保护：避免同一翻译单元重复声明，没有控制算法。

源文件：[Core/Inc/task/task_pitch/task_pitch_command.h](../Core/Inc/task/task_pitch/task_pitch_command.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

## Core/Inc/task/task_pitch/task_pitch_config.h

### TASK_PITCH_CONFIG_H（第 11 行定义）

```c
#define TASK_PITCH_CONFIG_H /* 防止配置重复包含。 */
```

包含保护：避免同一翻译单元重复声明，没有控制算法。

源文件：[Core/Inc/task/task_pitch/task_pitch_config.h](../Core/Inc/task/task_pitch/task_pitch_config.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### PITCH_STABILIZATION_ENABLE（第 15 行定义）

```c
#define PITCH_STABILIZATION_ENABLE 1U /* 惯性自稳开关，0/1；1自动进入，0使用原编码器正式控制。 */
```

0/1正式反馈选择；1使用六轴惯性姿态，未就绪/无效则捕获编码器保持并暂停人工输入；固定机械角测试始终绕过INS。

源文件：[Core/Inc/task/task_pitch/task_pitch_config.h](../Core/Inc/task/task_pitch/task_pitch_config.h)。

Core引用：[Core/Inc/task/task_pitch/task_pitch_config.h](../Core/Inc/task/task_pitch/task_pitch_config.h) 第 33 行；[Core/Src/task/task_pitch/task_pitch_runtime.c](../Core/Src/task/task_pitch/task_pitch_runtime.c) 第 50 行。

### PITCH_IMU_FEEDBACK_SIGN（第 18 行定义）

```c
#define PITCH_IMU_FEEDBACK_SIGN 1 /* 惯性角及欧拉角速度到编码器方向的映射，±1；两者使用同一符号。 */
```

±1惯性角/欧拉角速度到编码器方向映射，当前Yaw=-1、Pitch=+1；不替代电流/编码器测速符号。

源文件：[Core/Inc/task/task_pitch/task_pitch_config.h](../Core/Inc/task/task_pitch/task_pitch_config.h)。

Core引用：[Core/Src/task/task_pitch/task_pitch_runtime.c](../Core/Src/task/task_pitch/task_pitch_runtime.c) 第 52 行。

### PITCH_IMU_READY_MS（第 21 行定义）

```c
#define PITCH_IMU_READY_MS 100U /* 连续有效窗口，HAL ms，须大于超时；增大延迟自动进入。 */
```

HAL ms，>0且大于新鲜度期限，当前100；READY有效且样本持续更新满足窗口后捕获当时姿态，只在切换边沿锁定。

源文件：[Core/Inc/task/task_pitch/task_pitch_config.h](../Core/Inc/task/task_pitch/task_pitch_config.h)。

Core引用：[Core/Src/task/task_pitch/task_pitch_runtime.c](../Core/Src/task/task_pitch/task_pitch_runtime.c) 第 53 行。

### PITCH_IMU_TIMEOUT_MS（第 24 行定义）

```c
#define PITCH_IMU_TIMEOUT_MS 10U /* 陀螺/加计最大年龄，HAL ms，>0；过期退回编码器保持。 */
```

HAL ms，>0且小于资格窗口，当前10；陀螺/加计任一年龄超过期限即捕获编码器保持，不撤销正常CAN闭环许可。

源文件：[Core/Inc/task/task_pitch/task_pitch_config.h](../Core/Inc/task/task_pitch/task_pitch_config.h)。

Core引用：[Core/Src/task/task_pitch/task_pitch_runtime.c](../Core/Src/task/task_pitch/task_pitch_runtime.c) 第 54 行。

### PITCH_IMU_EXIT_PITCH_DEG（第 27 行定义）

```c
#define PITCH_IMU_EXIT_PITCH_DEG 80.0f /* 绝对Pitch退出阈值，deg，(0,90)；避免欧拉速度奇异。 */
```

deg，0<值<90，当前80；绝对Pitch达到阈值转编码器保持，避免ZYX测速除cos(Pitch)接近零。

源文件：[Core/Inc/task/task_pitch/task_pitch_config.h](../Core/Inc/task/task_pitch/task_pitch_config.h)。

Core引用：[Core/Src/task/task_pitch/task_pitch_runtime.c](../Core/Src/task/task_pitch/task_pitch_runtime.c) 第 55 行。

### PITCH_IMU_ENTER_PITCH_DEG（第 30 行定义）

```c
#define PITCH_IMU_ENTER_PITCH_DEG 75.0f /* 重新进入的绝对Pitch上限，deg，低于退出阈值；形成迟滞。 */
```

deg，0<值<退出阈值，当前75；编码器保持时回到该内区且连续有效更新后才能重入惯性反馈。

源文件：[Core/Inc/task/task_pitch/task_pitch_config.h](../Core/Inc/task/task_pitch/task_pitch_config.h)。

Core引用：[Core/Src/task/task_pitch/task_pitch_runtime.c](../Core/Src/task/task_pitch/task_pitch_runtime.c) 第 56 行。

### PITCH_HARDWARE_TEST_MODE_OFF（第 35 行定义）

```c
#define PITCH_HARDWARE_TEST_MODE_OFF 0U /* 正式控制模式，0；使用摇杆和鼠标命令。 */
```

编译期正式/测试路径选择，模式不在运行中切换；互斥限制见源#if。

源文件：[Core/Inc/task/task_pitch/task_pitch_config.h](../Core/Inc/task/task_pitch/task_pitch_config.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### PITCH_HARDWARE_TEST_MODE_CALIBRATION（第 36 行定义）

```c
#define PITCH_HARDWARE_TEST_MODE_CALIBRATION 1U /* 只读标定模式，1；手动移动并记录角度。 */
```

编译期正式/测试路径选择，模式不在运行中切换；互斥限制见源#if。

源文件：[Core/Inc/task/task_pitch/task_pitch_config.h](../Core/Inc/task/task_pitch/task_pitch_config.h)。

Core引用：[Core/Src/task/task_pitch/task_pitch.c](../Core/Src/task/task_pitch/task_pitch.c) 第 48 行。

### PITCH_HARDWARE_TEST_MODE_ANGLE_LOOP（第 37 行定义）

```c
#define PITCH_HARDWARE_TEST_MODE_ANGLE_LOOP 2U /* 固定角度测试模式，2；验证位置保持时使用。 */
```

编译期正式/测试路径选择，模式不在运行中切换；互斥限制见源#if。

源文件：[Core/Inc/task/task_pitch/task_pitch_config.h](../Core/Inc/task/task_pitch/task_pitch_config.h)。

Core引用：[Core/Src/task/task_pitch/task_pitch.c](../Core/Src/task/task_pitch/task_pitch.c) 第 52 行。

### PITCH_HARDWARE_TEST_MODE（第 39 行定义）

```c
#define PITCH_HARDWARE_TEST_MODE PITCH_HARDWARE_TEST_MODE_OFF /* 任务模式，0/1/2；正式运行选 OFF。 */
```

编译期正式/测试路径选择，模式不在运行中切换；互斥限制见源#if。

源文件：[Core/Inc/task/task_pitch/task_pitch_config.h](../Core/Inc/task/task_pitch/task_pitch_config.h)。

Core引用：[Core/Src/task/task_pitch/task_pitch.c](../Core/Src/task/task_pitch/task_pitch.c) 第 48 行。

### PITCH_CALIBRATION_VALID（第 42 行定义）

```c
#define PITCH_CALIBRATION_VALID 1U /* 标定许可，0/1；完成本轴标定后置 1。 */
```

标定门要求valid且min<center<max，否则零输出。

源文件：[Core/Inc/task/task_pitch/task_pitch_config.h](../Core/Inc/task/task_pitch/task_pitch_config.h)。

Core引用：[Core/Src/task/task_pitch/task_pitch_runtime.c](../Core/Src/task/task_pitch/task_pitch_runtime.c) 第 44 行。

### PITCH_TASK_PERIOD_MS（第 43 行定义）

```c
#define PITCH_TASK_PERIOD_MS 2U /* 控制周期，ms；改变后检查调度负载。 */
```

绝对唤醒周期；云台按固定配置dt，供弹正式路径测实际Tick差。

源文件：[Core/Inc/task/task_pitch/task_pitch_config.h](../Core/Inc/task/task_pitch/task_pitch_config.h)。

Core引用：[Core/Src/task/task_pitch/task_pitch.c](../Core/Src/task/task_pitch/task_pitch.c) 第 23 行。

### PITCH_COMMAND_TIMEOUT_MS（第 44 行定义）

```c
#define PITCH_COMMAND_TIMEOUT_MS 100U /* 输入期限，HAL ms；过期冻结目标并继续位置保持。 */
```

真实接收时间戳过期：云台冻结目标保持，供弹清除当前状态并输出零电流/停止脉宽。

源文件：[Core/Inc/task/task_pitch/task_pitch_config.h](../Core/Inc/task/task_pitch/task_pitch_config.h)。

Core引用：[Core/Src/task/task_pitch/task_pitch_command.c](../Core/Src/task/task_pitch/task_pitch_command.c) 第 57 行。

### PITCH_TASK_LOG_PERIOD_MS（第 45 行定义）

```c
#define PITCH_TASK_LOG_PERIOD_MS 500U /* 诊断周期，ms；串口刷屏时增大。 */
```

诊断/提示限频，通常成功提交后续期，不参与电流控制。

源文件：[Core/Inc/task/task_pitch/task_pitch_config.h](../Core/Inc/task/task_pitch/task_pitch_config.h)。

Core引用：[Core/Src/task/task_pitch/task_pitch_runtime.c](../Core/Src/task/task_pitch/task_pitch_runtime.c) 第 126 行。

### PITCH_FEEDBACK_TIMEOUT_MS（第 46 行定义）

```c
#define PITCH_FEEDBACK_TIMEOUT_MS 100U /* 反馈期限，HAL ms；增大会延迟掉线清零。 */
```

HAL同源年龄≥期限离线，先检查真实首帧；发送前重新判断反馈新鲜度。

源文件：[Core/Inc/task/task_pitch/task_pitch_config.h](../Core/Inc/task/task_pitch/task_pitch_config.h)。

Core引用：[Core/Src/task/task_pitch/task_pitch_runtime.c](../Core/Src/task/task_pitch/task_pitch_runtime.c) 第 39 行。

### PITCH_CALIBRATION_CENTER_ANGLE_RAW（第 47 行定义）

```c
#define PITCH_CALIBRATION_CENTER_ANGLE_RAW 8753 /* 中位连续角度，count；重新标定后填写。 */
```

连续count：中心作为首帧/补偿参考；min/max作为软件边界。

源文件：[Core/Inc/task/task_pitch/task_pitch_config.h](../Core/Inc/task/task_pitch/task_pitch_config.h)。

Core引用：[Core/Src/task/task_pitch/task_pitch_runtime.c](../Core/Src/task/task_pitch/task_pitch_runtime.c) 第 45 行。

### PITCH_CALIBRATION_MIN_ANGLE_RAW（第 48 行定义）

```c
#define PITCH_CALIBRATION_MIN_ANGLE_RAW 8136 /* 人工软件下界，连续 count；只过滤向下人工增量，不裁剪自稳纠偏。 */
```

连续count：中心作为首帧/补偿参考；min/max作为软件边界。

源文件：[Core/Inc/task/task_pitch/task_pitch_config.h](../Core/Inc/task/task_pitch/task_pitch_config.h)。

Core引用：[Core/Src/task/task_pitch/task_pitch_runtime.c](../Core/Src/task/task_pitch/task_pitch_runtime.c) 第 46 行。

### PITCH_CALIBRATION_MAX_ANGLE_RAW（第 49 行定义）

```c
#define PITCH_CALIBRATION_MAX_ANGLE_RAW 9350 /* 软件上界，连续 count；按实测边界设置并保留余量。 */
```

连续count：中心作为首帧/补偿参考；min/max作为软件边界。

源文件：[Core/Inc/task/task_pitch/task_pitch_config.h](../Core/Inc/task/task_pitch/task_pitch_config.h)。

Core引用：[Core/Src/task/task_pitch/task_pitch_runtime.c](../Core/Src/task/task_pitch/task_pitch_runtime.c) 第 47 行。

### PITCH_MOTOR_CURRENT_SIGN（第 50 行定义）

```c
#define PITCH_MOTOR_CURRENT_SIGN 1 /* 闭环电流方向，+1/-1；正误差驱动反向时核对。 */
```

方向映射±1；输入、反馈和电流独立。C615只记录方向，不用PWM切换正反转。

源文件：[Core/Inc/task/task_pitch/task_pitch_config.h](../Core/Inc/task/task_pitch/task_pitch_config.h)。

Core引用：[Core/Src/task/task_pitch/task_pitch_runtime.c](../Core/Src/task/task_pitch/task_pitch_runtime.c) 第 37 行。

### PITCH_FEEDBACK_SPEED_SIGN（第 51 行定义）

```c
#define PITCH_FEEDBACK_SPEED_SIGN 1 /* 反馈速度方向，+1/-1；与连续角度增量同向。 */
```

方向映射±1；输入、反馈和电流独立。C615只记录方向，不用PWM切换正反转。

源文件：[Core/Inc/task/task_pitch/task_pitch_config.h](../Core/Inc/task/task_pitch/task_pitch_config.h)。

Core引用：[Core/Src/task/task_pitch/task_pitch_runtime.c](../Core/Src/task/task_pitch/task_pitch_runtime.c) 第 38 行。

### PITCH_GRAVITY_COMPENSATION_ENABLE（第 53 行定义）

```c
#define PITCH_GRAVITY_COMPENSATION_ENABLE 1U /* 重力补偿开关，0/1；水平轴关闭，负载下垂时检查。 */
```

补偿开关，关闭返回0；禁用模型也校验参数。

源文件：[Core/Inc/task/task_pitch/task_pitch_config.h](../Core/Inc/task/task_pitch/task_pitch_config.h)。

Core引用：[Core/Src/task/task_pitch/task_pitch_runtime.c](../Core/Src/task/task_pitch/task_pitch_runtime.c) 第 77 行。

### PITCH_GRAVITY_COMPENSATION_BIAS_CURRENT_RAW（第 54 行定义）

```c
#define PITCH_GRAVITY_COMPENSATION_BIAS_CURRENT_RAW 2000.0f /* 重力偏置，GM6020 raw；保持时下垂可增加，温升高时降低。 */
```

余弦补偿常值B，模型sign×(B+A×cos(theta))。

源文件：[Core/Inc/task/task_pitch/task_pitch_config.h](../Core/Inc/task/task_pitch/task_pitch_config.h)。

Core引用：[Core/Src/task/task_pitch/task_pitch_runtime.c](../Core/Src/task/task_pitch/task_pitch_runtime.c) 第 79 行。

### PITCH_GRAVITY_COMPENSATION_AMPLITUDE_CURRENT_RAW（第 55 行定义）

```c
#define PITCH_GRAVITY_COMPENSATION_AMPLITUDE_CURRENT_RAW 2000.0f /* 角度补偿幅值，GM6020 raw；不同角度静差不同时调整。 */
```

余弦补偿幅值A，要求B+A≤补偿模型电流上限。

源文件：[Core/Inc/task/task_pitch/task_pitch_config.h](../Core/Inc/task/task_pitch/task_pitch_config.h)。

Core引用：[Core/Src/task/task_pitch/task_pitch_runtime.c](../Core/Src/task/task_pitch/task_pitch_runtime.c) 第 81 行。

### PITCH_GRAVITY_CURRENT_SIGN（第 56 行定义）

```c
#define PITCH_GRAVITY_CURRENT_SIGN 1 /* 重力电流方向，+1/-1；独立于闭环电流方向。 */
```

补偿在控制器坐标中的方向；合成后仍乘最终轴电流方向。

源文件：[Core/Inc/task/task_pitch/task_pitch_config.h](../Core/Inc/task/task_pitch/task_pitch_config.h)。

Core引用：[Core/Src/task/task_pitch/task_pitch_runtime.c](../Core/Src/task/task_pitch/task_pitch_runtime.c) 第 82 行。

### PITCH_MAX_COMMAND_SPEED_RPM（第 57 行定义）

```c
#define PITCH_MAX_COMMAND_SPEED_RPM 70.0f /* 满输入速度，rpm；整体动作过快时减小。 */
```

输入p/1000×Vmax为rpm，目标count积分增量rpm×8192/60×dt_s。

源文件：[Core/Inc/task/task_pitch/task_pitch_config.h](../Core/Inc/task/task_pitch/task_pitch_config.h)。

Core引用：[Core/Src/task/task_pitch/task_pitch_runtime.c](../Core/Src/task/task_pitch/task_pitch_runtime.c) 第 61 行。

### PITCH_VELOCITY_FEEDFORWARD_GAIN（第 59 行定义）

```c
#define PITCH_VELOCITY_FEEDFORWARD_GAIN 0.8f /* 速度前馈，无量纲且非负；跟随落后时增加，超前时降低。 */
```

位置外环输出叠加Kff×有效命令rpm；边界向外命令先归零。

源文件：[Core/Inc/task/task_pitch/task_pitch_config.h](../Core/Inc/task/task_pitch/task_pitch_config.h)。

Core引用：[Core/Src/task/task_pitch/task_pitch_runtime.c](../Core/Src/task/task_pitch/task_pitch_runtime.c) 第 71 行。

### PITCH_MAX_SPEED_TARGET_RPM（第 61 行定义）

```c
#define PITCH_MAX_SPEED_TARGET_RPM 120.0f /* 速度目标上限，rpm；过快时降低，饱和时先检查此项。 */
```

外环目标速度限幅；位置P变大无法突破该rpm上限。

源文件：[Core/Inc/task/task_pitch/task_pitch_config.h](../Core/Inc/task/task_pitch/task_pitch_config.h)。

Core引用：[Core/Src/task/task_pitch/task_pitch_runtime.c](../Core/Src/task/task_pitch/task_pitch_runtime.c) 第 62 行。

### PITCH_POSITION_KP_RPM_PER_RAW（第 63 行定义）

```c
#define PITCH_POSITION_KP_RPM_PER_RAW 0.01f /* 位置 P，rpm/count；响应慢时增加，过冲时降低。 */
```

位置P：位置误差count乘增益得到rpm（供弹得到电流raw）；见总文档§5/6/7。

源文件：[Core/Inc/task/task_pitch/task_pitch_config.h](../Core/Inc/task/task_pitch/task_pitch_config.h)。

Core引用：[Core/Src/task/task_pitch/task_pitch_runtime.c](../Core/Src/task/task_pitch/task_pitch_runtime.c) 第 63 行。

### PITCH_POSITION_KI_RPM_PER_RAW_S（第 64 行定义）

```c
#define PITCH_POSITION_KI_RPM_PER_RAW_S 0.0f /* 位置 I，rpm/(count·s)；持续静差时增加，摆动时降低。 */
```

位置积分：Ki×误差×dt_s；目标速度饱和且误差向外时恢复上一积分。

源文件：[Core/Inc/task/task_pitch/task_pitch_config.h](../Core/Inc/task/task_pitch/task_pitch_config.h)。

Core引用：[Core/Src/task/task_pitch/task_pitch_runtime.c](../Core/Src/task/task_pitch/task_pitch_runtime.c) 第 64 行。

### PITCH_POSITION_INTEGRAL_LIMIT_RPM（第 65 行定义）

```c
#define PITCH_POSITION_INTEGRAL_LIMIT_RPM 5.0f /* 位置积分上限，rpm，0~MAX_SPEED_TARGET；过冲时降低。 */
```

外环积分限制在±该rpm；不是总目标速度上限。

源文件：[Core/Inc/task/task_pitch/task_pitch_config.h](../Core/Inc/task/task_pitch/task_pitch_config.h)。

Core引用：[Core/Src/task/task_pitch/task_pitch_runtime.c](../Core/Src/task/task_pitch/task_pitch_runtime.c) 第 65 行。

### PITCH_POSITION_KD_RPM_S_PER_RAW（第 66 行定义）

```c
#define PITCH_POSITION_KD_RPM_S_PER_RAW 0.0f /* 位置 D，rpm·s/count；增加制动，过大会放大噪声。 */
```

位置阻尼：Kd×(命令rpm−滤波rpm)×8192/60。

源文件：[Core/Inc/task/task_pitch/task_pitch_config.h](../Core/Inc/task/task_pitch/task_pitch_config.h)。

Core引用：[Core/Src/task/task_pitch/task_pitch_runtime.c](../Core/Src/task/task_pitch/task_pitch_runtime.c) 第 66 行。

### PITCH_SPEED_KP_CURRENT_PER_RPM（第 67 行定义）

```c
#define PITCH_SPEED_KP_CURRENT_PER_RPM 200.0f /* 速度 P，raw/rpm；跟随慢时增加，测速噪声大时降低。 */
```

速度P：Kp×(目标rpm−滤波rpm)，输出raw。

源文件：[Core/Inc/task/task_pitch/task_pitch_config.h](../Core/Inc/task/task_pitch/task_pitch_config.h)。

Core引用：[Core/Src/task/task_pitch/task_pitch_runtime.c](../Core/Src/task/task_pitch/task_pitch_runtime.c) 第 67 行。

### PITCH_SPEED_KI_CURRENT_PER_RPM_S（第 68 行定义）

```c
#define PITCH_SPEED_KI_CURRENT_PER_RPM_S 20.0f /* 速度 I，raw/(rpm·s)；持续速度静差时增加，摆动时降低。 */
```

速度I：Ki×速度误差×dt_s；候选积分限幅和条件保存。

源文件：[Core/Inc/task/task_pitch/task_pitch_config.h](../Core/Inc/task/task_pitch/task_pitch_config.h)。

Core引用：[Core/Src/task/task_pitch/task_pitch_runtime.c](../Core/Src/task/task_pitch/task_pitch_runtime.c) 第 68 行。

### PITCH_MAX_CURRENT_RAW（第 69 行定义）

```c
#define PITCH_MAX_CURRENT_RAW 7000.0f /* 总电流上限，GM6020 raw，(0,16384]；增大会提高力矩和温升。 */
```

电流raw：C610协议±10000，GM6020±16384；任务进一步限流或指定测试电流。

源文件：[Core/Inc/task/task_pitch/task_pitch_config.h](../Core/Inc/task/task_pitch/task_pitch_config.h)。

Core引用：[Core/Src/task/task_pitch/task_pitch_runtime.c](../Core/Src/task/task_pitch/task_pitch_runtime.c) 第 72 行。

### PITCH_SPEED_KD_CURRENT_S_PER_RPM（第 70 行定义）

```c
#define PITCH_SPEED_KD_CURRENT_S_PER_RPM 1.5f /* 速度 D，raw·s/rpm；测速跳变造成冲击时降低。 */
```

速度D：Kd×(本误差−上一误差)/dt_s；首帧微分为零。

源文件：[Core/Inc/task/task_pitch/task_pitch_config.h](../Core/Inc/task/task_pitch/task_pitch_config.h)。

Core引用：[Core/Src/task/task_pitch/task_pitch_runtime.c](../Core/Src/task/task_pitch/task_pitch_runtime.c) 第 69 行。

### PITCH_SPEED_INTEGRAL_LIMIT_RAW（第 71 行定义）

```c
#define PITCH_SPEED_INTEGRAL_LIMIT_RAW PITCH_MAX_CURRENT_RAW /* 速度积分上限，raw，0~MAX_CURRENT；恢复冲击大时降低。 */
```

速度PID积分历史限幅，不能替代补偿合成后的总电流限幅。

源文件：[Core/Inc/task/task_pitch/task_pitch_config.h](../Core/Inc/task/task_pitch/task_pitch_config.h)。

Core引用：[Core/Src/task/task_pitch/task_pitch_runtime.c](../Core/Src/task/task_pitch/task_pitch_runtime.c) 第 70 行。

### PITCH_CURRENT_SLEW_RAW_PER_S（第 72 行定义）

```c
#define PITCH_CURRENT_SLEW_RAW_PER_S 60000.0f /* 电流斜率，raw/s；冲击大时降低，制动滞后时增加。 */
```

Ramp变化最多rate×dt_s；PWM速率=(活动−停止)×1000/RAMP_MS。

源文件：[Core/Inc/task/task_pitch/task_pitch_config.h](../Core/Inc/task/task_pitch/task_pitch_config.h)。

Core引用：[Core/Src/task/task_pitch/task_pitch_runtime.c](../Core/Src/task/task_pitch/task_pitch_runtime.c) 第 73 行。

### PITCH_SPEED_FILTER_ALPHA（第 73 行定义）

```c
#define PITCH_SPEED_FILTER_ALPHA 0.15f /* 测速权重，(0,1]；噪声大时降低，制动滞后时增加。 */
```

一阶低通y←y+alpha×(x−y)；权重越小响应越滞后。

源文件：[Core/Inc/task/task_pitch/task_pitch_config.h](../Core/Inc/task/task_pitch/task_pitch_config.h)。

Core引用：[Core/Src/task/task_pitch/task_pitch_runtime.c](../Core/Src/task/task_pitch/task_pitch_runtime.c) 第 74 行。

### PITCH_ANGLE_LOOP_TARGET_ANGLE_DEG（第 75 行定义）

```c
#define PITCH_ANGLE_LOOP_TARGET_ANGLE_DEG 0.0f /* 测试目标，相对中位 deg；保持测试时设置，必须在软件边界内。 */
```

固定目标中心count+deg×8192/360；转整数后检查标定范围。

源文件：[Core/Inc/task/task_pitch/task_pitch_config.h](../Core/Inc/task/task_pitch/task_pitch_config.h)。

Core引用：[Core/Src/task/task_pitch/task_pitch_runtime.c](../Core/Src/task/task_pitch/task_pitch_runtime.c) 第 97 行。

### PITCH_ANGLE_LOOP_LOG_PERIOD_MS（第 76 行定义）

```c
#define PITCH_ANGLE_LOOP_LOG_PERIOD_MS 200U /* 角度测试日志周期，ms；串口刷屏时增大。 */
```

诊断/提示限频，通常成功提交后续期，不参与电流控制。

源文件：[Core/Inc/task/task_pitch/task_pitch_config.h](../Core/Inc/task/task_pitch/task_pitch_config.h)。

Core引用：[Core/Src/task/task_pitch/task_pitch_runtime.c](../Core/Src/task/task_pitch/task_pitch_runtime.c) 第 98 行。

### PITCH_CALIBRATION_LOG_PERIOD_MS（第 78 行定义）

```c
#define PITCH_CALIBRATION_LOG_PERIOD_MS 200U /* 标定日志周期，ms；漏看角度变化时减小。 */
```

诊断/提示限频，通常成功提交后续期，不参与电流控制。

源文件：[Core/Inc/task/task_pitch/task_pitch_config.h](../Core/Inc/task/task_pitch/task_pitch_config.h)。

Core引用：[Core/Src/task/task_pitch/task_pitch_runtime.c](../Core/Src/task/task_pitch/task_pitch_runtime.c) 第 93 行。

### PITCH_CALIBRATION_PROMPT_PERIOD_MS（第 79 行定义）

```c
#define PITCH_CALIBRATION_PROMPT_PERIOD_MS 5000U /* 标定提示周期，ms；提示过密时增大。 */
```

诊断/提示限频，通常成功提交后续期，不参与电流控制。

源文件：[Core/Inc/task/task_pitch/task_pitch_config.h](../Core/Inc/task/task_pitch/task_pitch_config.h)。

Core引用：[Core/Src/task/task_pitch/task_pitch_runtime.c](../Core/Src/task/task_pitch/task_pitch_runtime.c) 第 94 行。

## Core/Inc/task/task_pitch/task_pitch_runtime.h

### TASK_PITCH_RUNTIME_H（第 14 行定义）

```c
#define TASK_PITCH_RUNTIME_H /* 防止 Pitch 运行时重复包含（避免运行时结构重复定义）。 */
```

包含保护：避免同一翻译单元重复声明，没有控制算法。

源文件：[Core/Inc/task/task_pitch/task_pitch_runtime.h](../Core/Inc/task/task_pitch/task_pitch_runtime.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

## Core/Inc/task/task_yaw/task_yaw.h

### TASK_YAW_H（第 18 行定义）

```c
#define TASK_YAW_H /* 防止 Yaw 任务入口声明被重复包含（避免同一函数声明出现两次）。 */
```

包含保护：避免同一翻译单元重复声明，没有控制算法。

源文件：[Core/Inc/task/task_yaw/task_yaw.h](../Core/Inc/task/task_yaw/task_yaw.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

## Core/Inc/task/task_yaw/task_yaw_command.h

### TASK_YAW_COMMAND_H（第 16 行定义）

```c
#define TASK_YAW_COMMAND_H /* 防止 Yaw 命令接口被重复包含（避免同一份声明出现两次）。 */
```

包含保护：避免同一翻译单元重复声明，没有控制算法。

源文件：[Core/Inc/task/task_yaw/task_yaw_command.h](../Core/Inc/task/task_yaw/task_yaw_command.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

## Core/Inc/task/task_yaw/task_yaw_config.h

### TASK_YAW_CONFIG_H（第 11 行定义）

```c
#define TASK_YAW_CONFIG_H /* 防止配置重复包含。 */
```

包含保护：避免同一翻译单元重复声明，没有控制算法。

源文件：[Core/Inc/task/task_yaw/task_yaw_config.h](../Core/Inc/task/task_yaw/task_yaw_config.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### YAW_STABILIZATION_ENABLE（第 17 行定义）

```c
#define YAW_STABILIZATION_ENABLE 1U /* 惯性自稳开关，0/1；1自动进入，0使用原编码器正式控制。 */
```

0/1正式反馈选择；1使用六轴惯性姿态，未就绪/无效则捕获编码器保持并暂停人工输入；固定机械角测试始终绕过INS。

源文件：[Core/Inc/task/task_yaw/task_yaw_config.h](../Core/Inc/task/task_yaw/task_yaw_config.h)。

Core引用：[Core/Inc/task/task_yaw/task_yaw_config.h](../Core/Inc/task/task_yaw/task_yaw_config.h) 第 35 行；[Core/Src/task/task_yaw/task_yaw_runtime.c](../Core/Src/task/task_yaw/task_yaw_runtime.c) 第 50 行。

### YAW_IMU_FEEDBACK_SIGN（第 20 行定义）

```c
#define YAW_IMU_FEEDBACK_SIGN (-1) /* 惯性角及欧拉角速度到编码器方向的映射，±1；两者使用同一符号。 */
```

±1惯性角/欧拉角速度到编码器方向映射，当前Yaw=-1、Pitch=+1；不替代电流/编码器测速符号。

源文件：[Core/Inc/task/task_yaw/task_yaw_config.h](../Core/Inc/task/task_yaw/task_yaw_config.h)。

Core引用：[Core/Src/task/task_yaw/task_yaw_runtime.c](../Core/Src/task/task_yaw/task_yaw_runtime.c) 第 52 行。

### YAW_IMU_READY_MS（第 23 行定义）

```c
#define YAW_IMU_READY_MS 100U /* 连续有效窗口，HAL ms，须大于超时；增大延迟自动进入。 */
```

HAL ms，>0且大于新鲜度期限，当前100；READY有效且样本持续更新满足窗口后捕获当时姿态，只在切换边沿锁定。

源文件：[Core/Inc/task/task_yaw/task_yaw_config.h](../Core/Inc/task/task_yaw/task_yaw_config.h)。

Core引用：[Core/Src/task/task_yaw/task_yaw_runtime.c](../Core/Src/task/task_yaw/task_yaw_runtime.c) 第 53 行。

### YAW_IMU_TIMEOUT_MS（第 26 行定义）

```c
#define YAW_IMU_TIMEOUT_MS 10U /* 陀螺/加计最大年龄，HAL ms，>0；过期退回编码器保持。 */
```

HAL ms，>0且小于资格窗口，当前10；陀螺/加计任一年龄超过期限即捕获编码器保持，不撤销正常CAN闭环许可。

源文件：[Core/Inc/task/task_yaw/task_yaw_config.h](../Core/Inc/task/task_yaw/task_yaw_config.h)。

Core引用：[Core/Src/task/task_yaw/task_yaw_runtime.c](../Core/Src/task/task_yaw/task_yaw_runtime.c) 第 54 行。

### YAW_IMU_EXIT_PITCH_DEG（第 29 行定义）

```c
#define YAW_IMU_EXIT_PITCH_DEG 80.0f /* 绝对Pitch退出阈值，deg，(0,90)；避免欧拉速度奇异。 */
```

deg，0<值<90，当前80；绝对Pitch达到阈值转编码器保持，避免ZYX测速除cos(Pitch)接近零。

源文件：[Core/Inc/task/task_yaw/task_yaw_config.h](../Core/Inc/task/task_yaw/task_yaw_config.h)。

Core引用：[Core/Src/task/task_yaw/task_yaw_runtime.c](../Core/Src/task/task_yaw/task_yaw_runtime.c) 第 55 行。

### YAW_IMU_ENTER_PITCH_DEG（第 32 行定义）

```c
#define YAW_IMU_ENTER_PITCH_DEG 75.0f /* 重新进入的绝对Pitch上限，deg，低于退出阈值；形成迟滞。 */
```

deg，0<值<退出阈值，当前75；编码器保持时回到该内区且连续有效更新后才能重入惯性反馈。

源文件：[Core/Inc/task/task_yaw/task_yaw_config.h](../Core/Inc/task/task_yaw/task_yaw_config.h)。

Core引用：[Core/Src/task/task_yaw/task_yaw_runtime.c](../Core/Src/task/task_yaw/task_yaw_runtime.c) 第 56 行。

### YAW_BOUNDARY_YIELD_ENABLE（第 38 行定义）

```c
#define YAW_BOUNDARY_YIELD_ENABLE 1U /* 正式边界策略，0/1；1丢弃不可达向外误差并禁止向外电流，固定角度测试不启用。 */
```

无量纲0/1，默认1；正式Yaw在编码器边界丢弃不可达向外误差、清向外积分并在Ramp后禁止向外电流及同步历史。0只限制人工增量/前馈；固定机械角测试始终关闭让步，不改变IMU连续航向。

源文件：[Core/Inc/task/task_yaw/task_yaw_config.h](../Core/Inc/task/task_yaw/task_yaw_config.h)。

Core引用：[Core/Inc/task/task_yaw/task_yaw_config.h](../Core/Inc/task/task_yaw/task_yaw_config.h) 第 40 行；[Core/Src/task/task_yaw/task_yaw_runtime.c](../Core/Src/task/task_yaw/task_yaw_runtime.c) 第 61 行。

### YAW_HARDWARE_TEST_MODE_OFF（第 42 行定义）

```c
#define YAW_HARDWARE_TEST_MODE_OFF 0U /* 正式控制模式，0；使用摇杆和鼠标命令。 */
```

编译期正式/测试路径选择，模式不在运行中切换；互斥限制见源#if。

源文件：[Core/Inc/task/task_yaw/task_yaw_config.h](../Core/Inc/task/task_yaw/task_yaw_config.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

### YAW_HARDWARE_TEST_MODE_CALIBRATION（第 43 行定义）

```c
#define YAW_HARDWARE_TEST_MODE_CALIBRATION 1U /* 只读标定模式，1；手动移动并记录角度。 */
```

编译期正式/测试路径选择，模式不在运行中切换；互斥限制见源#if。

源文件：[Core/Inc/task/task_yaw/task_yaw_config.h](../Core/Inc/task/task_yaw/task_yaw_config.h)。

Core引用：[Core/Src/task/task_yaw/task_yaw.c](../Core/Src/task/task_yaw/task_yaw.c) 第 48 行。

### YAW_HARDWARE_TEST_MODE_ANGLE_LOOP（第 44 行定义）

```c
#define YAW_HARDWARE_TEST_MODE_ANGLE_LOOP 2U /* 固定角度测试模式，2；验证位置保持时使用。 */
```

编译期正式/测试路径选择，模式不在运行中切换；互斥限制见源#if。

源文件：[Core/Inc/task/task_yaw/task_yaw_config.h](../Core/Inc/task/task_yaw/task_yaw_config.h)。

Core引用：[Core/Src/task/task_yaw/task_yaw.c](../Core/Src/task/task_yaw/task_yaw.c) 第 52 行。

### YAW_HARDWARE_TEST_MODE（第 47 行定义）

```c
#define YAW_HARDWARE_TEST_MODE YAW_HARDWARE_TEST_MODE_OFF /* 任务模式，0/1/2；正式运行选 OFF。 */
```

编译期正式/测试路径选择，模式不在运行中切换；互斥限制见源#if。

源文件：[Core/Inc/task/task_yaw/task_yaw_config.h](../Core/Inc/task/task_yaw/task_yaw_config.h)。

Core引用：[Core/Src/task/task_yaw/task_yaw.c](../Core/Src/task/task_yaw/task_yaw.c) 第 48 行。

### YAW_CALIBRATION_VALID（第 50 行定义）

```c
#define YAW_CALIBRATION_VALID 1U /* 标定许可，0/1；完成本轴标定后置 1。 */
```

标定门要求valid且min<center<max，否则零输出。

源文件：[Core/Inc/task/task_yaw/task_yaw_config.h](../Core/Inc/task/task_yaw/task_yaw_config.h)。

Core引用：[Core/Src/task/task_yaw/task_yaw_runtime.c](../Core/Src/task/task_yaw/task_yaw_runtime.c) 第 44 行。

### YAW_TASK_PERIOD_MS（第 51 行定义）

```c
#define YAW_TASK_PERIOD_MS 2U /* 控制周期，ms；改变后检查调度负载。 */
```

绝对唤醒周期；云台按固定配置dt，供弹正式路径测实际Tick差。

源文件：[Core/Inc/task/task_yaw/task_yaw_config.h](../Core/Inc/task/task_yaw/task_yaw_config.h)。

Core引用：[Core/Src/task/task_yaw/task_yaw.c](../Core/Src/task/task_yaw/task_yaw.c) 第 23 行。

### YAW_FEEDBACK_TIMEOUT_MS（第 52 行定义）

```c
#define YAW_FEEDBACK_TIMEOUT_MS 100U /* 反馈期限，HAL ms；增大会延迟掉线清零。 */
```

HAL同源年龄≥期限离线，先检查真实首帧；发送前重新判断反馈新鲜度。

源文件：[Core/Inc/task/task_yaw/task_yaw_config.h](../Core/Inc/task/task_yaw/task_yaw_config.h)。

Core引用：[Core/Src/task/task_yaw/task_yaw_runtime.c](../Core/Src/task/task_yaw/task_yaw_runtime.c) 第 39 行。

### YAW_COMMAND_TIMEOUT_MS（第 53 行定义）

```c
#define YAW_COMMAND_TIMEOUT_MS 100U /* 输入期限，HAL ms；过期冻结目标并继续位置保持。 */
```

真实接收时间戳过期：云台冻结目标保持，供弹清除当前状态并输出零电流/停止脉宽。

源文件：[Core/Inc/task/task_yaw/task_yaw_config.h](../Core/Inc/task/task_yaw/task_yaw_config.h)。

Core引用：[Core/Src/task/task_yaw/task_yaw_command.c](../Core/Src/task/task_yaw/task_yaw_command.c) 第 91 行。

### YAW_TASK_LOG_PERIOD_MS（第 54 行定义）

```c
#define YAW_TASK_LOG_PERIOD_MS 500U /* 诊断周期，ms；串口刷屏时增大。 */
```

诊断/提示限频，通常成功提交后续期，不参与电流控制。

源文件：[Core/Inc/task/task_yaw/task_yaw_config.h](../Core/Inc/task/task_yaw/task_yaw_config.h)。

Core引用：[Core/Src/task/task_yaw/task_yaw_runtime.c](../Core/Src/task/task_yaw/task_yaw_runtime.c) 第 125 行。

### YAW_CALIBRATION_CENTER_ANGLE_RAW（第 55 行定义）

```c
#define YAW_CALIBRATION_CENTER_ANGLE_RAW 2100 /* 中位连续角度，count；重新标定后填写。 */
```

连续count：中心作为首帧/补偿参考；min/max作为软件边界。

源文件：[Core/Inc/task/task_yaw/task_yaw_config.h](../Core/Inc/task/task_yaw/task_yaw_config.h)。

Core引用：[Core/Src/task/task_yaw/task_yaw_runtime.c](../Core/Src/task/task_yaw/task_yaw_runtime.c) 第 45 行。

### YAW_CALIBRATION_MIN_ANGLE_RAW（第 56 行定义）

```c
#define YAW_CALIBRATION_MIN_ANGLE_RAW 115 /* 软件下界，连续 count；按实测边界设置并保留余量。 */
```

连续count：中心作为首帧/补偿参考；min/max作为软件边界。

源文件：[Core/Inc/task/task_yaw/task_yaw_config.h](../Core/Inc/task/task_yaw/task_yaw_config.h)。

Core引用：[Core/Src/task/task_yaw/task_yaw_runtime.c](../Core/Src/task/task_yaw/task_yaw_runtime.c) 第 46 行。

### YAW_CALIBRATION_MAX_ANGLE_RAW（第 57 行定义）

```c
#define YAW_CALIBRATION_MAX_ANGLE_RAW 4085 /* 软件上界，连续 count；按实测边界设置并保留余量。 */
```

连续count：中心作为首帧/补偿参考；min/max作为软件边界。

源文件：[Core/Inc/task/task_yaw/task_yaw_config.h](../Core/Inc/task/task_yaw/task_yaw_config.h)。

Core引用：[Core/Src/task/task_yaw/task_yaw_runtime.c](../Core/Src/task/task_yaw/task_yaw_runtime.c) 第 47 行。

### YAW_MOTOR_CURRENT_SIGN（第 58 行定义）

```c
#define YAW_MOTOR_CURRENT_SIGN 1 /* 闭环电流方向，+1/-1；正误差驱动反向时核对。 */
```

方向映射±1；输入、反馈和电流独立。C615只记录方向，不用PWM切换正反转。

源文件：[Core/Inc/task/task_yaw/task_yaw_config.h](../Core/Inc/task/task_yaw/task_yaw_config.h)。

Core引用：[Core/Src/task/task_yaw/task_yaw_runtime.c](../Core/Src/task/task_yaw/task_yaw_runtime.c) 第 37 行。

### YAW_FEEDBACK_SPEED_SIGN（第 59 行定义）

```c
#define YAW_FEEDBACK_SPEED_SIGN 1 /* 反馈速度方向，+1/-1；与连续角度增量同向。 */
```

方向映射±1；输入、反馈和电流独立。C615只记录方向，不用PWM切换正反转。

源文件：[Core/Inc/task/task_yaw/task_yaw_config.h](../Core/Inc/task/task_yaw/task_yaw_config.h)。

Core引用：[Core/Src/task/task_yaw/task_yaw_runtime.c](../Core/Src/task/task_yaw/task_yaw_runtime.c) 第 38 行。

### YAW_GRAVITY_COMPENSATION_ENABLE（第 61 行定义）

```c
#define YAW_GRAVITY_COMPENSATION_ENABLE 0U /* 重力补偿开关，0/1；水平轴关闭，负载下垂时检查。 */
```

补偿开关，关闭返回0；禁用模型也校验参数。

源文件：[Core/Inc/task/task_yaw/task_yaw_config.h](../Core/Inc/task/task_yaw/task_yaw_config.h)。

Core引用：[Core/Src/task/task_yaw/task_yaw_runtime.c](../Core/Src/task/task_yaw/task_yaw_runtime.c) 第 78 行。

### YAW_GRAVITY_COMPENSATION_BIAS_CURRENT_RAW（第 62 行定义）

```c
#define YAW_GRAVITY_COMPENSATION_BIAS_CURRENT_RAW 0.0f /* 重力偏置，GM6020 raw；保持时下垂可增加，温升高时降低。 */
```

余弦补偿常值B，模型sign×(B+A×cos(theta))。

源文件：[Core/Inc/task/task_yaw/task_yaw_config.h](../Core/Inc/task/task_yaw/task_yaw_config.h)。

Core引用：[Core/Src/task/task_yaw/task_yaw_runtime.c](../Core/Src/task/task_yaw/task_yaw_runtime.c) 第 80 行。

### YAW_GRAVITY_COMPENSATION_AMPLITUDE_CURRENT_RAW（第 63 行定义）

```c
#define YAW_GRAVITY_COMPENSATION_AMPLITUDE_CURRENT_RAW 0.0f /* 角度补偿幅值，GM6020 raw；不同角度静差不同时调整。 */
```

余弦补偿幅值A，要求B+A≤补偿模型电流上限。

源文件：[Core/Inc/task/task_yaw/task_yaw_config.h](../Core/Inc/task/task_yaw/task_yaw_config.h)。

Core引用：[Core/Src/task/task_yaw/task_yaw_runtime.c](../Core/Src/task/task_yaw/task_yaw_runtime.c) 第 81 行。

### YAW_GRAVITY_CURRENT_SIGN（第 64 行定义）

```c
#define YAW_GRAVITY_CURRENT_SIGN 1 /* 重力电流方向，+1/-1；独立于闭环电流方向。 */
```

补偿在控制器坐标中的方向；合成后仍乘最终轴电流方向。

源文件：[Core/Inc/task/task_yaw/task_yaw_config.h](../Core/Inc/task/task_yaw/task_yaw_config.h)。

Core引用：[Core/Src/task/task_yaw/task_yaw_runtime.c](../Core/Src/task/task_yaw/task_yaw_runtime.c) 第 82 行。

### YAW_MAX_COMMAND_SPEED_RPM（第 66 行定义）

```c
#define YAW_MAX_COMMAND_SPEED_RPM 70.0f /* 满输入速度，rpm；整体动作过快时减小。 */
```

输入p/1000×Vmax为rpm，目标count积分增量rpm×8192/60×dt_s。

源文件：[Core/Inc/task/task_yaw/task_yaw_config.h](../Core/Inc/task/task_yaw/task_yaw_config.h)。

Core引用：[Core/Src/task/task_yaw/task_yaw_runtime.c](../Core/Src/task/task_yaw/task_yaw_runtime.c) 第 62 行。

### YAW_VELOCITY_FEEDFORWARD_GAIN（第 68 行定义）

```c
#define YAW_VELOCITY_FEEDFORWARD_GAIN 1.2f /* 速度前馈，无量纲且非负；跟随落后时增加，超前时降低。 */
```

位置外环输出叠加Kff×有效命令rpm；边界向外命令先归零。

源文件：[Core/Inc/task/task_yaw/task_yaw_config.h](../Core/Inc/task/task_yaw/task_yaw_config.h)。

Core引用：[Core/Src/task/task_yaw/task_yaw_runtime.c](../Core/Src/task/task_yaw/task_yaw_runtime.c) 第 72 行。

### YAW_MAX_SPEED_TARGET_RPM（第 70 行定义）

```c
#define YAW_MAX_SPEED_TARGET_RPM 120.0f /* 速度目标上限，rpm；过快时降低，饱和时先检查此项。 */
```

外环目标速度限幅；位置P变大无法突破该rpm上限。

源文件：[Core/Inc/task/task_yaw/task_yaw_config.h](../Core/Inc/task/task_yaw/task_yaw_config.h)。

Core引用：[Core/Src/task/task_yaw/task_yaw_runtime.c](../Core/Src/task/task_yaw/task_yaw_runtime.c) 第 63 行。

### YAW_POSITION_KP_RPM_PER_RAW（第 72 行定义）

```c
#define YAW_POSITION_KP_RPM_PER_RAW 0.01f /* 位置 P，rpm/count；响应慢时增加，过冲时降低。 */
```

位置P：位置误差count乘增益得到rpm（供弹得到电流raw）；见总文档§5/6/7。

源文件：[Core/Inc/task/task_yaw/task_yaw_config.h](../Core/Inc/task/task_yaw/task_yaw_config.h)。

Core引用：[Core/Src/task/task_yaw/task_yaw_runtime.c](../Core/Src/task/task_yaw/task_yaw_runtime.c) 第 64 行。

### YAW_POSITION_KI_RPM_PER_RAW_S（第 73 行定义）

```c
#define YAW_POSITION_KI_RPM_PER_RAW_S 0.0f /* 位置 I，rpm/(count·s)；持续静差时增加，摆动时降低。 */
```

位置积分：Ki×误差×dt_s；目标速度饱和且误差向外时恢复上一积分。

源文件：[Core/Inc/task/task_yaw/task_yaw_config.h](../Core/Inc/task/task_yaw/task_yaw_config.h)。

Core引用：[Core/Src/task/task_yaw/task_yaw_runtime.c](../Core/Src/task/task_yaw/task_yaw_runtime.c) 第 65 行。

### YAW_POSITION_INTEGRAL_LIMIT_RPM（第 74 行定义）

```c
#define YAW_POSITION_INTEGRAL_LIMIT_RPM 5.0f /* 位置积分上限，rpm，0~MAX_SPEED_TARGET；过冲时降低。 */
```

外环积分限制在±该rpm；不是总目标速度上限。

源文件：[Core/Inc/task/task_yaw/task_yaw_config.h](../Core/Inc/task/task_yaw/task_yaw_config.h)。

Core引用：[Core/Src/task/task_yaw/task_yaw_runtime.c](../Core/Src/task/task_yaw/task_yaw_runtime.c) 第 66 行。

### YAW_POSITION_KD_RPM_S_PER_RAW（第 75 行定义）

```c
#define YAW_POSITION_KD_RPM_S_PER_RAW 0.0f /* 位置 D，rpm·s/count；增加制动，过大会放大噪声。 */
```

位置阻尼：Kd×(命令rpm−滤波rpm)×8192/60。

源文件：[Core/Inc/task/task_yaw/task_yaw_config.h](../Core/Inc/task/task_yaw/task_yaw_config.h)。

Core引用：[Core/Src/task/task_yaw/task_yaw_runtime.c](../Core/Src/task/task_yaw/task_yaw_runtime.c) 第 67 行。

### YAW_SPEED_KP_CURRENT_PER_RPM（第 76 行定义）

```c
#define YAW_SPEED_KP_CURRENT_PER_RPM 200.0f /* 速度 P，raw/rpm；跟随慢时增加，测速噪声大时降低。 */
```

速度P：Kp×(目标rpm−滤波rpm)，输出raw。

源文件：[Core/Inc/task/task_yaw/task_yaw_config.h](../Core/Inc/task/task_yaw/task_yaw_config.h)。

Core引用：[Core/Src/task/task_yaw/task_yaw_runtime.c](../Core/Src/task/task_yaw/task_yaw_runtime.c) 第 68 行。

### YAW_SPEED_KI_CURRENT_PER_RPM_S（第 77 行定义）

```c
#define YAW_SPEED_KI_CURRENT_PER_RPM_S 20.0f /* 速度 I，raw/(rpm·s)；持续速度静差时增加，摆动时降低。 */
```

速度I：Ki×速度误差×dt_s；候选积分限幅和条件保存。

源文件：[Core/Inc/task/task_yaw/task_yaw_config.h](../Core/Inc/task/task_yaw/task_yaw_config.h)。

Core引用：[Core/Src/task/task_yaw/task_yaw_runtime.c](../Core/Src/task/task_yaw/task_yaw_runtime.c) 第 69 行。

### YAW_MAX_CURRENT_RAW（第 79 行定义）

```c
#define YAW_MAX_CURRENT_RAW 7000.0f /* 总电流上限，GM6020 raw，(0,16384]；增大会提高力矩和温升。 */
```

电流raw：C610协议±10000，GM6020±16384；任务进一步限流或指定测试电流。

源文件：[Core/Inc/task/task_yaw/task_yaw_config.h](../Core/Inc/task/task_yaw/task_yaw_config.h)。

Core引用：[Core/Src/task/task_yaw/task_yaw_runtime.c](../Core/Src/task/task_yaw/task_yaw_runtime.c) 第 73 行。

### YAW_SPEED_KD_CURRENT_S_PER_RPM（第 80 行定义）

```c
#define YAW_SPEED_KD_CURRENT_S_PER_RPM 1.5f /* 速度 D，raw·s/rpm；测速跳变造成冲击时降低。 */
```

速度D：Kd×(本误差−上一误差)/dt_s；首帧微分为零。

源文件：[Core/Inc/task/task_yaw/task_yaw_config.h](../Core/Inc/task/task_yaw/task_yaw_config.h)。

Core引用：[Core/Src/task/task_yaw/task_yaw_runtime.c](../Core/Src/task/task_yaw/task_yaw_runtime.c) 第 70 行。

### YAW_SPEED_INTEGRAL_LIMIT_RAW（第 81 行定义）

```c
#define YAW_SPEED_INTEGRAL_LIMIT_RAW YAW_MAX_CURRENT_RAW /* 速度积分上限，raw，0~MAX_CURRENT；恢复冲击大时降低。 */
```

速度PID积分历史限幅，不能替代补偿合成后的总电流限幅。

源文件：[Core/Inc/task/task_yaw/task_yaw_config.h](../Core/Inc/task/task_yaw/task_yaw_config.h)。

Core引用：[Core/Src/task/task_yaw/task_yaw_runtime.c](../Core/Src/task/task_yaw/task_yaw_runtime.c) 第 71 行。

### YAW_CURRENT_SLEW_RAW_PER_S（第 82 行定义）

```c
#define YAW_CURRENT_SLEW_RAW_PER_S 60000.0f /* 电流斜率，raw/s；冲击大时降低，制动滞后时增加。 */
```

Ramp变化最多rate×dt_s；PWM速率=(活动−停止)×1000/RAMP_MS。

源文件：[Core/Inc/task/task_yaw/task_yaw_config.h](../Core/Inc/task/task_yaw/task_yaw_config.h)。

Core引用：[Core/Src/task/task_yaw/task_yaw_runtime.c](../Core/Src/task/task_yaw/task_yaw_runtime.c) 第 74 行。

### YAW_SPEED_FILTER_ALPHA（第 83 行定义）

```c
#define YAW_SPEED_FILTER_ALPHA 0.20f /* 测速权重，(0,1]；噪声大时降低，制动滞后时增加。 */
```

一阶低通y←y+alpha×(x−y)；权重越小响应越滞后。

源文件：[Core/Inc/task/task_yaw/task_yaw_config.h](../Core/Inc/task/task_yaw/task_yaw_config.h)。

Core引用：[Core/Src/task/task_yaw/task_yaw_runtime.c](../Core/Src/task/task_yaw/task_yaw_runtime.c) 第 75 行。

### YAW_ANGLE_LOOP_TARGET_ANGLE_DEG（第 85 行定义）

```c
#define YAW_ANGLE_LOOP_TARGET_ANGLE_DEG 30.0f /* 测试目标，相对中位 deg；保持测试时设置，必须在软件边界内。 */
```

固定目标中心count+deg×8192/360；转整数后检查标定范围。

源文件：[Core/Inc/task/task_yaw/task_yaw_config.h](../Core/Inc/task/task_yaw/task_yaw_config.h)。

Core引用：[Core/Src/task/task_yaw/task_yaw_runtime.c](../Core/Src/task/task_yaw/task_yaw_runtime.c) 第 97 行。

### YAW_ANGLE_LOOP_LOG_PERIOD_MS（第 86 行定义）

```c
#define YAW_ANGLE_LOOP_LOG_PERIOD_MS 200U /* 角度测试日志周期，ms；串口刷屏时增大。 */
```

诊断/提示限频，通常成功提交后续期，不参与电流控制。

源文件：[Core/Inc/task/task_yaw/task_yaw_config.h](../Core/Inc/task/task_yaw/task_yaw_config.h)。

Core引用：[Core/Src/task/task_yaw/task_yaw_runtime.c](../Core/Src/task/task_yaw/task_yaw_runtime.c) 第 98 行。

### YAW_CALIBRATION_LOG_PERIOD_MS（第 88 行定义）

```c
#define YAW_CALIBRATION_LOG_PERIOD_MS 200U /* 标定日志周期，ms；漏看角度变化时减小。 */
```

诊断/提示限频，通常成功提交后续期，不参与电流控制。

源文件：[Core/Inc/task/task_yaw/task_yaw_config.h](../Core/Inc/task/task_yaw/task_yaw_config.h)。

Core引用：[Core/Src/task/task_yaw/task_yaw_runtime.c](../Core/Src/task/task_yaw/task_yaw_runtime.c) 第 93 行。

### YAW_CALIBRATION_PROMPT_PERIOD_MS（第 89 行定义）

```c
#define YAW_CALIBRATION_PROMPT_PERIOD_MS 3000U /* 标定提示周期，ms；提示过密时增大。 */
```

诊断/提示限频，通常成功提交后续期，不参与电流控制。

源文件：[Core/Inc/task/task_yaw/task_yaw_config.h](../Core/Inc/task/task_yaw/task_yaw_config.h)。

Core引用：[Core/Src/task/task_yaw/task_yaw_runtime.c](../Core/Src/task/task_yaw/task_yaw_runtime.c) 第 94 行。

## Core/Inc/task/task_yaw/task_yaw_runtime.h

### TASK_YAW_RUNTIME_H（第 14 行定义）

```c
#define TASK_YAW_RUNTIME_H /* 防止 Yaw 运行时重复包含（避免运行时结构重复定义）。 */
```

包含保护：避免同一翻译单元重复声明，没有控制算法。

源文件：[Core/Inc/task/task_yaw/task_yaw_runtime.h](../Core/Inc/task/task_yaw/task_yaw_runtime.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

## Core/Inc/tim.h

### __TIM_H__（第 9 行定义）

```c
#define __TIM_H__
```

平台/模板项，按原定义、源注释与引用判断；没有位置或速度控制公式。

源文件：[Core/Inc/tim.h](../Core/Inc/tim.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

## Core/Inc/usart.h

### __USART_H__（第 22 行定义）

```c
#define __USART_H__
```

平台/模板项，按原定义、源注释与引用判断；没有位置或速度控制公式。

源文件：[Core/Inc/usart.h](../Core/Inc/usart.h)。

Core无额外引用命中：可能为包含保护、第三方消费、模板未用项或条件分支，检查预处理结果再判断生效性。

## Core/Src/algorithm/ahrs/ahrs.c

### AHRS_PI_RAD（第 13 行定义）

```c
#define AHRS_PI_RAD 3.14159265358979323846f /* π，rad，用于验证创新角范围。 */
```

惯性/磁驱动的协议常量、单位换算或数值保护；来源和单位见宏注释，接口与状态机见INS说明。

源文件：[Core/Src/algorithm/ahrs/ahrs.c](../Core/Src/algorithm/ahrs/ahrs.c)。

Core引用：[Core/Src/algorithm/ahrs/ahrs.c](../Core/Src/algorithm/ahrs/ahrs.c) 第 58 行。

### AHRS_NORM_EPSILON（第 14 行定义）

```c
#define AHRS_NORM_EPSILON 1.0e-8f /* 向量平方模下限，避免零除。 */
```

惯性/磁驱动的协议常量、单位换算或数值保护；来源和单位见宏注释，接口与状态机见INS说明。

源文件：[Core/Src/algorithm/ahrs/ahrs.c](../Core/Src/algorithm/ahrs/ahrs.c)。

Core引用：[Core/Src/algorithm/ahrs/ahrs.c](../Core/Src/algorithm/ahrs/ahrs.c) 第 105 行。

## Core/Src/algorithm/gravity_compensation/gravity_compensation.c

### GRAVITY_COMPENSATION_TWO_PI（第 14 行定义）

```c
#define GRAVITY_COMPENSATION_TWO_PI 6.28318530717958647692f /* 一圈弧度，rad；用于 8192 count/rev 的角度换算。 */
```

编码器角差×2π/8192转换为余弦输入rad。

源文件：[Core/Src/algorithm/gravity_compensation/gravity_compensation.c](../Core/Src/algorithm/gravity_compensation/gravity_compensation.c)。

Core引用：[Core/Src/algorithm/gravity_compensation/gravity_compensation.c](../Core/Src/algorithm/gravity_compensation/gravity_compensation.c) 第 84 行。

### GRAVITY_COMPENSATION_COUNTS_PER_REV（第 15 行定义）

```c
#define GRAVITY_COMPENSATION_COUNTS_PER_REV 8192LL /* GM6020 编码器一圈 8192 count；算法不依赖 CAN 驱动。 */
```

补偿角差int64求差后按8192取余，不改写控制目标。

源文件：[Core/Src/algorithm/gravity_compensation/gravity_compensation.c](../Core/Src/algorithm/gravity_compensation/gravity_compensation.c)。

Core引用：[Core/Src/algorithm/gravity_compensation/gravity_compensation.c](../Core/Src/algorithm/gravity_compensation/gravity_compensation.c) 第 83 行。

## Core/Src/app/gimbal/gimbal_axis.c

### GIMBAL_AXIS_DEG_TO_RAD（第 14 行定义）

```c
#define GIMBAL_AXIS_DEG_TO_RAD 0.017453292519943295f /* deg 转 rad，用于 ZYX 欧拉角速度关系。 */
```

deg→rad、rad/s→rpm及deg→等效count换算；ZYX陀螺投影与姿态保持共享单位，不能直接使用机体gyroZ作为Yaw速度。

源文件：[Core/Src/app/gimbal/gimbal_axis.c](../Core/Src/app/gimbal/gimbal_axis.c)。

Core引用：[Core/Src/app/gimbal/gimbal_axis.c](../Core/Src/app/gimbal/gimbal_axis.c) 第 236 行。

### GIMBAL_AXIS_RAD_S_TO_RPM（第 15 行定义）

```c
#define GIMBAL_AXIS_RAD_S_TO_RPM 9.54929658551372f /* rad/s 转 rpm，60/(2π)。 */
```

deg→rad、rad/s→rpm及deg→等效count换算；ZYX陀螺投影与姿态保持共享单位，不能直接使用机体gyroZ作为Yaw速度。

源文件：[Core/Src/app/gimbal/gimbal_axis.c](../Core/Src/app/gimbal/gimbal_axis.c)。

Core引用：[Core/Src/app/gimbal/gimbal_axis.c](../Core/Src/app/gimbal/gimbal_axis.c) 第 244 行。

### GIMBAL_AXIS_RAW_PER_DEG（第 16 行定义）

```c
#define GIMBAL_AXIS_RAW_PER_DEG ((float)GM6020_ENCODER_COUNTS_PER_REV / 360.0f) /* 8192 count/rev 对应的等效 count/deg。 */
```

deg→rad、rad/s→rpm及deg→等效count换算；ZYX陀螺投影与姿态保持共享单位，不能直接使用机体gyroZ作为Yaw速度。

源文件：[Core/Src/app/gimbal/gimbal_axis.c](../Core/Src/app/gimbal/gimbal_axis.c)。

Core引用：[Core/Src/app/gimbal/gimbal_axis.c](../Core/Src/app/gimbal/gimbal_axis.c) 第 243 行。

## Core/Src/app/ins/ins.c

### INS_RAD_TO_DEG（第 14 行定义）

```c
#define INS_RAD_TO_DEG 57.29577951308232f /* rad 转 degree。 */
```

惯性/磁驱动的协议常量、单位换算或数值保护；来源和单位见宏注释，接口与状态机见INS说明。

源文件：[Core/Src/app/ins/ins.c](../Core/Src/app/ins/ins.c)。

Core引用：[Core/Src/app/ins/ins.c](../Core/Src/app/ins/ins.c) 第 180 行。

## Core/Src/app/log/log.c

### LOG_TX_BUFFER_SIZE（第 15 行定义）

```c
#define LOG_TX_BUFFER_SIZE 768U /* DMA 缓冲区大小，字节；最大文本长度为 767 字节。 */
```

唯一DMA缓冲区768字节，允许1~767字节；超长整条拒绝。

源文件：[Core/Src/app/log/log.c](../Core/Src/app/log/log.c)。

Core引用：[Core/Src/app/log/log.c](../Core/Src/app/log/log.c) 第 18 行。

## Core/Src/bsp/bmi088/bmi088.c

### BMI088_READ_BIT（第 10 行定义）

```c
#define BMI088_READ_BIT 0x80U /* SPI 地址 bit7=1 表示读，手册 §6.1。 */
```

惯性/磁驱动的协议常量、单位换算或数值保护；来源和单位见宏注释，接口与状态机见INS说明。

源文件：[Core/Src/bsp/bmi088/bmi088.c](../Core/Src/bsp/bmi088/bmi088.c)。

Core引用：[Core/Src/bsp/bmi088/bmi088.c](../Core/Src/bsp/bmi088/bmi088.c) 第 30 行。

### BMI088_RESET_CMD（第 11 行定义）

```c
#define BMI088_RESET_CMD 0xB6U /* 两颗传感器的软复位命令。 */
```

惯性/磁驱动的协议常量、单位换算或数值保护；来源和单位见宏注释，接口与状态机见INS说明。

源文件：[Core/Src/bsp/bmi088/bmi088.c](../Core/Src/bsp/bmi088/bmi088.c)。

Core引用：[Core/Src/bsp/bmi088/bmi088.c](../Core/Src/bsp/bmi088/bmi088.c) 第 78 行。

### BMI088_GRAVITY_MPS2（第 12 行定义）

```c
#define BMI088_GRAVITY_MPS2 9.80665f /* 标准重力加速度，m/s²。 */
```

惯性/磁驱动的协议常量、单位换算或数值保护；来源和单位见宏注释，接口与状态机见INS说明。

源文件：[Core/Src/bsp/bmi088/bmi088.c](../Core/Src/bsp/bmi088/bmi088.c)。

Core引用：[Core/Src/bsp/bmi088/bmi088.c](../Core/Src/bsp/bmi088/bmi088.c) 第 140 行。

### BMI088_DEG_TO_RAD（第 13 行定义）

```c
#define BMI088_DEG_TO_RAD 0.017453292519943295f /* °/s 转 rad/s。 */
```

惯性/磁驱动的协议常量、单位换算或数值保护；来源和单位见宏注释，接口与状态机见INS说明。

源文件：[Core/Src/bsp/bmi088/bmi088.c](../Core/Src/bsp/bmi088/bmi088.c)。

Core引用：[Core/Src/bsp/bmi088/bmi088.c](../Core/Src/bsp/bmi088/bmi088.c) 第 141 行。

## Core/Src/bsp/c610_m2006/test_c610_m2006_angle_step.c

### C610_ANGLE_STEP_COUNT_TEXT_SIZE（第 15 行定义）

```c
#define C610_ANGLE_STEP_COUNT_TEXT_SIZE 24U /* int64_t 十进制文本最大长度，含符号和 NUL。 */
```

平台/模板项，按原定义、源注释与引用判断；没有位置或速度控制公式。

源文件：[Core/Src/bsp/c610_m2006/test_c610_m2006_angle_step.c](../Core/Src/bsp/c610_m2006/test_c610_m2006_angle_step.c)。

Core引用：[Core/Src/bsp/c610_m2006/test_c610_m2006_angle_step.c](../Core/Src/bsp/c610_m2006/test_c610_m2006_angle_step.c) 第 25 行。

## Core/Src/bsp/dbus/dbus.c

### DBUS_FRAME_MAILBOX_LENGTH（第 26 行定义）

```c
#define DBUS_FRAME_MAILBOX_LENGTH 16U /* 固定邮箱槽位；任务短暂延迟时保留连续鼠标帧，溢出只记录次数。 */
```

环形数组16槽，保留一个空槽区分满/空，最多15帧；满时丢新帧计数。

源文件：[Core/Src/bsp/dbus/dbus.c](../Core/Src/bsp/dbus/dbus.c)。

Core引用：[Core/Src/bsp/dbus/dbus.c](../Core/Src/bsp/dbus/dbus.c) 第 27 行。

## Core/Src/bsp/gm6020/test_gm6020_angle_loop.c

### GM6020_ANGLE_TEST_DEFAULT_LOG_PERIOD_MS（第 14 行定义）

```c
#define GM6020_ANGLE_TEST_DEFAULT_LOG_PERIOD_MS 200U /**
 * @brief  执行一次固定目标角度环硬件调参。沿用正式控制、Ramp 和软件边界过滤。
 * @param  motor 该轴 GM6020 句柄，用于读取本周期一致快照。必须等于 &axis->motor。
 * @param  calibration 该轴中心与边界配置。函数检查它是否仍与 axis 配置一致。
 * @param  axis 正式通用角度控制实例，不能与其它轴共享。测试直接复用这份历史状态。
 * @param  test 该轴独立测试状态和目标角度参数。Yaw/Pitch 不能共用。
 * @param  now_ms HAL_GetTick() 当前时间，单位毫秒。用于反馈年龄和日志限频。
 * @param  dt_ms 控制周期，单位毫秒。必须大于零，传给角度/速度环。
 * @note   先 GimbalAxis_Init(&axis, &config) 注册电机，任务每周期直接调用：
 * Gm6020_TestAngleLoop_Run(&axis.motor, &axis.config.calibration,
 * &axis, &test, HAL_GetTick(), 2U);
 * test 置零，指定轴名称、目标度数和日志间隔。不需要测试 Init。
 * 只在任务上下文调用，禁止 CAN ISR 调用。motor 必须为 &axis.motor。避免读错轴。
 * PID、Ramp 和边界参数改对应轴配置，确认后正式模式使用同一组参数。测试不复制算法。
 * @retval None 输出结果写入对象或参数，函数无返回值。
 */
```

诊断/提示限频，通常成功提交后续期，不参与电流控制。

源文件：[Core/Src/bsp/gm6020/test_gm6020_angle_loop.c](../Core/Src/bsp/gm6020/test_gm6020_angle_loop.c)。

Core引用：[Core/Src/bsp/gm6020/test_gm6020_angle_loop.c](../Core/Src/bsp/gm6020/test_gm6020_angle_loop.c) 第 83 行。

## Core/Src/bsp/gm6020/test_gm6020_calibration.c

### GM6020_CALIBRATION_LOG_PERIOD_MS（第 16 行定义）

```c
#define GM6020_CALIBRATION_LOG_PERIOD_MS 200U /* 连续角度日志最短间隔，单位 HAL ms。避免串口刷屏。 */
```

诊断/提示限频，通常成功提交后续期，不参与电流控制。

源文件：[Core/Src/bsp/gm6020/test_gm6020_calibration.c](../Core/Src/bsp/gm6020/test_gm6020_calibration.c)。

Core引用：[Core/Src/bsp/gm6020/test_gm6020_calibration.c](../Core/Src/bsp/gm6020/test_gm6020_calibration.c) 第 57 行。

### GM6020_CALIBRATION_PROMPT_PERIOD_MS（第 17 行定义）

```c
#define GM6020_CALIBRATION_PROMPT_PERIOD_MS 5000U /**
 * @brief  执行一次非阻塞的手动角度标定。每次只读一份反馈并尝试发零电流。
 * @param  motor 已初始化并注册接收的 GM6020 句柄。函数不会修改标定值。只访问驱动反馈。
 * @param  calibration 该轴标定值，仅用于日志状态提示。不会写回配置。
 * @param  test 该轴独立的测试状态，不能在 Yaw/Pitch 间共享。各轴分别记录限频时间。
 * @param  now_ms HAL_GetTick() 当前时间，单位毫秒。必须与反馈时间戳同源。
 * @note   调用者应在任务初始化电机后每周期调用。test 可先清零，不需要额外 Init。
 * 只能在任务上下文调用。每次调用发送零电流，禁止在 CAN ISR 调用。ISR 只收帧。
 * @retval None 输出结果写入对象或参数，函数无返回值。
 */
```

诊断/提示限频，通常成功提交后续期，不参与电流控制。

源文件：[Core/Src/bsp/gm6020/test_gm6020_calibration.c](../Core/Src/bsp/gm6020/test_gm6020_calibration.c)。

Core引用：[Core/Src/bsp/gm6020/test_gm6020_calibration.c](../Core/Src/bsp/gm6020/test_gm6020_calibration.c) 第 59 行。

## Core/Src/bsp/ist8310/ist8310.c

### IST8310_HAL_ADDRESS（第 9 行定义）

```c
#define IST8310_HAL_ADDRESS (0x0EU << 1U) /* 七位地址 0x0E，HAL 要求左移一位。 */
```

惯性/磁驱动的协议常量、单位换算或数值保护；来源和单位见宏注释，接口与状态机见INS说明。

源文件：[Core/Src/bsp/ist8310/ist8310.c](../Core/Src/bsp/ist8310/ist8310.c)。

Core引用：[Core/Src/bsp/ist8310/ist8310.c](../Core/Src/bsp/ist8310/ist8310.c) 第 14 行。

### IST8310_UT_PER_LSB（第 10 行定义）

```c
#define IST8310_UT_PER_LSB 0.3f /* 教程 §11.4.3，μT/LSB。 */
```

惯性/磁驱动的协议常量、单位换算或数值保护；来源和单位见宏注释，接口与状态机见INS说明。

源文件：[Core/Src/bsp/ist8310/ist8310.c](../Core/Src/bsp/ist8310/ist8310.c)。

Core引用：[Core/Src/bsp/ist8310/ist8310.c](../Core/Src/bsp/ist8310/ist8310.c) 第 71 行。

## Core/Src/system_stm32f4xx.c

### HSE_VALUE（第 51 行定义）

```c
  #define HSE_VALUE    ((uint32_t)25000000) /*!< Default value of the External oscillator in Hz */
```

时钟频率Hz；PLL、总线与外设波特率/计时基于这些值。

源文件：[Core/Src/system_stm32f4xx.c](../Core/Src/system_stm32f4xx.c)。

Core引用：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h) 第 98 行；[Core/Src/system_stm32f4xx.c](../Core/Src/system_stm32f4xx.c) 第 50 行。

### HSI_VALUE（第 55 行定义）

```c
  #define HSI_VALUE    ((uint32_t)16000000) /*!< Value of the Internal oscillator in Hz*/
```

时钟频率Hz；PLL、总线与外设波特率/计时基于这些值。

源文件：[Core/Src/system_stm32f4xx.c](../Core/Src/system_stm32f4xx.c)。

Core引用：[Core/Inc/stm32f4xx_hal_conf.h](../Core/Inc/stm32f4xx_hal_conf.h) 第 111 行；[Core/Src/system_stm32f4xx.c](../Core/Src/system_stm32f4xx.c) 第 54 行。

### VECT_TAB_BASE_ADDRESS（第 101 行定义）

```c
#define VECT_TAB_BASE_ADDRESS   SRAM_BASE       /*!< Vector Table base address field.
                                                     This value must be a multiple of 0x200. */
```

CMSIS向量表/外存选项，是否生效按条件编译分支判断。

源文件：[Core/Src/system_stm32f4xx.c](../Core/Src/system_stm32f4xx.c)。

Core引用：[Core/Src/system_stm32f4xx.c](../Core/Src/system_stm32f4xx.c) 第 180 行。

### VECT_TAB_BASE_ADDRESS（第 104 行定义）

```c
#define VECT_TAB_BASE_ADDRESS   FLASH_BASE      /*!< Vector Table base address field.
                                                     This value must be a multiple of 0x200. */
```

CMSIS向量表/外存选项，是否生效按条件编译分支判断。

源文件：[Core/Src/system_stm32f4xx.c](../Core/Src/system_stm32f4xx.c)。

Core引用：[Core/Src/system_stm32f4xx.c](../Core/Src/system_stm32f4xx.c) 第 180 行。

### VECT_TAB_OFFSET（第 108 行定义）

```c
#define VECT_TAB_OFFSET         0x00000000U     /*!< Vector Table offset field.
                                                     This value must be a multiple of 0x200. */
```

CMSIS向量表/外存选项，是否生效按条件编译分支判断。

源文件：[Core/Src/system_stm32f4xx.c](../Core/Src/system_stm32f4xx.c)。

Core引用：[Core/Src/system_stm32f4xx.c](../Core/Src/system_stm32f4xx.c) 第 107 行。

## Core/Src/task/task_dbus/task_dbus_trace.c

### DBUS_TRACE_INTERVAL_UNKNOWN_US（第 38 行定义）

```c
#define DBUS_TRACE_INTERVAL_UNKNOWN_US UINT32_MAX /* 无前帧或间隔过长时的微秒间隔哨兵。 */
```

平台/模板项，按原定义、源注释与引用判断；没有位置或速度控制公式。

源文件：[Core/Src/task/task_dbus/task_dbus_trace.c](../Core/Src/task/task_dbus/task_dbus_trace.c)。

Core引用：[Core/Src/task/task_dbus/task_dbus_trace.c](../Core/Src/task/task_dbus/task_dbus_trace.c) 第 124 行。

### DBUS_TRACE_STATUS_MS（第 39 行定义）

```c
#define DBUS_TRACE_STATUS_MS 1000U /* 接收、排队与丢样统计周期，HAL ms。 */
```

平台/模板项，按原定义、源注释与引用判断；没有位置或速度控制公式。

源文件：[Core/Src/task/task_dbus/task_dbus_trace.c](../Core/Src/task/task_dbus/task_dbus_trace.c)。

Core引用：[Core/Src/task/task_dbus/task_dbus_trace.c](../Core/Src/task/task_dbus/task_dbus_trace.c) 第 125 行。

## Core/Src/task/task_feed_motor/task_feed_motor_runtime.c

### FEED_MOTOR_COUNT_TEXT_SIZE（第 22 行定义）

```c
#define FEED_MOTOR_COUNT_TEXT_SIZE 24U /* int64_t 十进制文本最大长度，含符号和 NUL。 */
```

平台/模板项，按原定义、源注释与引用判断；没有位置或速度控制公式。

源文件：[Core/Src/task/task_feed_motor/task_feed_motor_runtime.c](../Core/Src/task/task_feed_motor/task_feed_motor_runtime.c)。

Core引用：[Core/Src/task/task_feed_motor/task_feed_motor_runtime.c](../Core/Src/task/task_feed_motor/task_feed_motor_runtime.c) 第 32 行。

## Core/Src/task/task_ins/task_ins_runtime.c

### INS_EVENT_SENSOR（第 23 行定义）

```c
#define INS_EVENT_SENSOR 1UL /* 任务通知 bit0：至少一个 DRDY，序号保存真实事件。 */
```

INS采集新鲜度、事务超时、重试、事件或日志周期；HAL ms与FreeRTOS Tick分别使用，不能互相相减。

源文件：[Core/Src/task/task_ins/task_ins_runtime.c](../Core/Src/task/task_ins/task_ins_runtime.c)。

Core引用：[Core/Src/task/task_ins/task_ins_runtime.c](../Core/Src/task/task_ins/task_ins_runtime.c) 第 147 行。

### INS_EVENT_DMA（第 24 行定义）

```c
#define INS_EVENT_DMA 2UL /* 任务通知 bit1：DMA 完成/错误，状态由驱动保存。 */
```

INS采集新鲜度、事务超时、重试、事件或日志周期；HAL ms与FreeRTOS Tick分别使用，不能互相相减。

源文件：[Core/Src/task/task_ins/task_ins_runtime.c](../Core/Src/task/task_ins/task_ins_runtime.c)。

Core引用：[Core/Src/task/task_ins/task_ins_runtime.c](../Core/Src/task/task_ins/task_ins_runtime.c) 第 164 行。

### INS_MAG_POLL_MS（第 25 行定义）

```c
#define INS_MAG_POLL_MS 5U /* 200Hz 磁力计状态兜底轮询间隔，HAL ms。 */
```

INS采集新鲜度、事务超时、重试、事件或日志周期；HAL ms与FreeRTOS Tick分别使用，不能互相相减。

源文件：[Core/Src/task/task_ins/task_ins_runtime.c](../Core/Src/task/task_ins/task_ins_runtime.c)。

Core引用：[Core/Src/task/task_ins/task_ins_runtime.c](../Core/Src/task/task_ins/task_ins_runtime.c) 第 264 行。

### INS_MOTIONCAL_COUNTS_PER_LSB（第 26 行定义）

```c
#define INS_MOTIONCAL_COUNTS_PER_LSB 3L /* PJRC UT_PER_COUNT=0.1μT；IST8310=0.3μT/LSB，芯片原始值乘3。 */
```

IST8310 0.3μT/LSB转MotionCal 0.1μT/count，原始有符号值先扩展再乘3；越出int16范围整帧拒绝。

源文件：[Core/Src/task/task_ins/task_ins_runtime.c](../Core/Src/task/task_ins/task_ins_runtime.c)。

Core引用：[Core/Src/task/task_ins/task_ins_runtime.c](../Core/Src/task/task_ins/task_ins_runtime.c) 第 301 行。

共覆盖 542 个未注释的宏定义位置。
