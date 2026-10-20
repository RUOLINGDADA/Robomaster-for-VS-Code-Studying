# 两份本地 standard_robot 工程对照

核对日期：2026-10-18。目的：确认“整车”参考工程是否启用磁力计，以及它与指定官方目录的实际差异。

## 来源与核对范围

- A（本地官方目录）：`C:/Users/zhoujinyuan/Desktop/wj/Robomaster-for-VS-Code-Studying/参考文档/Robomaster/Code/Development-Board-C-Examples-master/20.standard_robot`。
- B（本地整车目录）：`C:/Users/zhoujinyuan/Desktop/wj/Robomaster-for-VS-Code-Studying/参考文档/Robomaster/Code/整车/20.standard_robot`。

这里的“官方”指用户指定的本地目录，不断言它与上游发布原件完全一致。两目录均只读；未烧录、未构建两份参考工程，不能由目录名称推断其硬件效果或改动先后。

使用Python标准库逐文件比较全部 `.c/.h/.s/.ioc/.uvprojx/.lib/.ld/.sct` 文件，包含HAL、FreeRTOS等目录：两边各275个，路径集合相同；260个逐字节相同，15个内容不同，没有仅换行不同的文件。编译输出、IDE调试设置及缓存不在此统计中。

## 磁力计与姿态结论

两份 `application/INS_task.c` 和 `.h` 逐字节相同。两者都初始化IST8310，但没有启用实际磁场数据读取；不能作为已运行九轴的对照。

| 位置（两份行号相同） | 实际行为 |
| --- | --- |
| `INS_task.c:4` | 文件说明明确写 `no use ist8310` |
| `INS_task.c:184` | 执行 `ist8310_init()`，只证明走初始化路径 |
| `INS_task.c:189` | 启动读取BMI088，没有读取磁场 |
| `INS_task.c:194`、`:267` | AHRS接口仍传入 `INS_mag`，但传参不等于有新磁观测 |
| `INS_task.c:271`～`:276` | DRDY标志分支保留，唯一磁读取调用被注释：`// ist8310_read_mag(ist8310_real_data.mag);` |
| `INS_task.c:534` | IST中断只更新检测/事件标志，没有读取数据 |
| `calibrate_task.c:7`、`:248` | 说明磁标定未实现且不使用磁场；加速度/磁场标定hook均为NULL |

全工程搜索 `ist8310_read_mag()`、`ist8310_read_over()`，只发现驱动定义/声明和上述被注释的调用。`ist8310_real_data`、`mag_offset` 为未显式初始化的全局对象，默认零；未找到更新磁测量或启用硬铁/软铁校准的执行路径。变量、getter、标定数据结构存在，不代表相关功能已实现。

BMI088/IST8310驱动 `.c`、IST中间层及AHRS库相同。`components/algorithm/AHRS.lib` 的两份SHA-256均为：

```text
ace29e114d892a48a473cc3421f0c22d16841c1316313746650b8f441749413a
```

AHRS内部实现封装在该库，未从公开头文件推断其具体磁异常门控或算法细节。

## 安装矩阵也相同

两份 `INS_task.c:43`～`:52` 均为：

```c
BMI088: {{0,1,0},{-1,0,0},{0,0,1}}
IST8310: {{1,0,0},{0,1,0},{0,0,1}}
```

即参考工程的BMI输出 `[chip_y,-chip_x,chip_z]`，磁场预留转换为芯片原方向。当前yuntai按用户确认使用前右下坐标和分别配置的 `diag(1,-1,-1)`，不能直接照搬参考矩阵。尤其参考IST没有实际融合，单位矩阵不能作为本机磁轴已经正确的证据；必须结合传感器相对安装和真实磁观测验证。

## 15个差异文件

| 文件 | A → B 的差异 | 影响 |
| --- | --- | --- |
| `MDK-ARM/standard_robot.uvprojx` | ARMCLANG 6.24 → ARMCC 5.06 update6；AC6开关1→0；B增加 `__CC_ARM`；CMSIS组件/包版本、优化/语言/警告选项不同 | 本地编译工具链与配置不同，不是姿态算法升级 |
| `Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS/cmsis_os.c` | A使用 `cmsis_compiler.h`；B使用ARMCC/GCC/IAR条件分支 | 编译器适配 |
| `Middlewares/Third_Party/FreeRTOS/Source/portable/RVDS/ARM_CM4F/port.c` | A包含AC6 GNU风格汇编分支；B保留ARMCC风格汇编、不同FPU检查和屏障内建函数 | RTOS移植层适配；不宜跨工具链直接替换 |
| 同目录 `portmacro.h` | A有AC6内联/寄存器访问适配；B使用 `__forceinline`、`__clz` 等ARMCC形式 | 同上 |
| `application/calibrate_task.h` | A的 `__PACKED_STRUCT` → B的 `__packed struct`，B去掉编译器头 | 结构体打包语法适配；未增加磁标定 |
| `application/detect_task.h` | 同类打包语法/头文件差异 | 编译器适配 |
| `application/referee.h` | 同类打包语法/头文件差异 | 协议字段定义未改变 |
| `application/remote_control.h` | 同类打包语法/头文件差异 | 遥控字段定义未改变 |
| `components/algorithm/user_lib.h` | 同类打包语法/头文件差异 | Ramp等结构定义语法适配 |
| `components/devices/BMI088driver.h` | 同类打包语法/头文件差异 | BMI088数据结构语法适配，驱动实现相同 |
| `components/devices/OLED.h` | 同类打包语法/头文件差异 | OLED结构定义语法适配 |
| `application/chassis_task.c` | 前向声明由A的 `static void` 改成B的 `void`；函数定义两边仍为 `static void` | 声明作用域存在差异，B前后声明不一致；底盘控制公式/参数没有变化 |
| `bsp/boards/bsp_flash.c` | `ger_sector()`前向声明由A的 `uint32_t` 改成B的 `static uint32_t` | 内部链接声明差异，Flash操作函数体相同 |
| `bsp/boards/bsp_fric.h` | `FRIC_UP` 1400→1550；`FRIC_DOWN` 1320→1520；`FRIC_OFF`均1000 | 真正改变摩擦轮PWM比较值/运行目标；仅为参考工程参数，不是本机实测值 |
| `application/shoot.c` | 增加中文说明，并在注释中明确Q开启/E关闭摩擦轮 | 仅注释改变，执行逻辑相同；两份 `shoot.h` 原本就均为Q/E |

摩擦轮配置值是TIM1的比较计数；两份 `Src/tim.c`相同，TIM1分频167、周期19999。对本地配置进行解释时应以其定时器时钟为准，不把这些数值当作电流或RPM。

以下关键文件也逐字节相同：`application/gimbal_task.c`、`gimbal_behaviour.c`、`chassis_behaviour.c`、`calibrate_task.c`、`shoot.h`、`Src/freertos.c`、`Src/tim.c`、`bsp/boards/bsp_fric.c`。

## 对当前yuntai排查的意义

两份参考工程没有不同的磁校准参数或已启用的九轴路径可用来替换当前实现。参考项目中有“磁力计驱动/标定结构/磁参数”不等于实际读磁并闭环校正Yaw。当前yuntai的磁有效与磁融合标记反映真实采集/门控，仍需按最终安装位置的三维标定和实际姿态一致性验收。比较期间没有改动当前固件配置、九轴开关或临时磁诊断。
