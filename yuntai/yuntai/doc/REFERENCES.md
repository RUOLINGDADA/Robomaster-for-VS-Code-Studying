# 参考资料与转换记录

本轮实现参考 `C:\Users\zhoujinyuan\Desktop\wj\Robomaster-for-VS-Code-Studying\参考文档\Robomaster\Code\Development-Board-C-Examples-master\20.standard_robot` 的 DBUS、发射和 CAN 组织方式，并核对以下 PDF：

| 主题 | 版本 / PDF 页数 | Markdown 来源 | 用途 |
|---|---|---|---|
| C615 | V1.0，2019.10 / 6 页 | `参考文档/markdown/RoboMaster_C615_无刷电机调速器说明书.md` | PWM 脉宽、频率和行程校准 |
| Snail 2305 | V1.0，2019.10 / 4 页 | `参考文档/markdown/RoboMaster_Snail_2305_直流无刷电机说明书.md` | 电机与电调配套说明 |
| C610 | V1.0，2025.11 / 20 页 | `参考文档/markdown/C610_无刷电机调速器使用说明.md` | M2006 CAN 电流范围和反馈字段 |
| M2006 | V1.0，2019.03 / 10 页 | `参考文档/markdown/RM_M2006_P36直流无刷减速电机使用说明.md` | 电机与减速箱参数 |
| GM6020 | V1.4，2023.10 / 13 页 | `参考文档/markdown/RoboMaster_GM6020直流无刷电机使用说明20231013.md` | 云台 CAN 控制帧与反馈字段 |
| 开发板 C 使用说明 | V1.0，2019.11 / 14 页 | `参考文档/markdown/RoboMaster  开发板 C 型使用说明.md` | 引脚和板级外设 |
| 开发板 C 用户手册 | V1.0，2020.01 / 21 页 | `参考文档/markdown/RoboMaster  开发板 C 型用户手册.md` | 板级连接和配置背景 |
| 开发板 C 嵌入式教程 | V1.0，2020.01 / 296 页 | `参考文档/markdown/RoboMaster开发板C型嵌入式软件教程文档.md` | CubeMX、HAL、FreeRTOS 和 CAN 示例 |
| CAN 协议 V2.0 | V2.0，文本未标日期 / 36 页 | `参考文档/markdown/CAN总线协议-V2.0（中文）.md` | CAN 标准帧背景 |

## 转换方法与限制

Markdown 文件由原始 PDF 的文本层转换生成，文件头保留原始 PDF 名称。文本转换使用
`pypdf 6.10.0`，每页使用 `## Page N` 保留页号。PDF 中的图片、接线图、曲线、字体布局和
部分复杂表格不保证在 Markdown 中保留；涉及引脚、PWM 行程或协议字段时，仍以原始 PDF
和实机手册为准。原始 PDF 未删除或覆盖。

## 采用的参考实现

- 两份本地 `20.standard_robot` 的完整对照见 [参考工程差异](REFERENCE_STANDARD_ROBOT_COMPARISON.md)：275个源码/配置/库文件中260个相同、15个不同；INS和AHRS库相同，均未启用实际磁场读取。区别主要为ARMCC5/AC6适配及摩擦轮PWM值，不能将“整车”目录作为已验证九轴实现。

- `20.standard_robot/application/gimbal_behaviour.c`：鼠标 X 负号、Y 正号与摇杆叠加；本项目换为千分比速度接口。
- `20.standard_robot/application/shoot.c`、`shoot.h`：摩擦轮 Ramp、按键触发与 0.314 rad 步进、0.05 rad 到位窗口；本项目按用户计划换为电机轴计数并加入速度/稳定确认。
- `20.standard_robot/application/remote_control.c`：DBUS 通道、鼠标和键盘字段解码。
- `20.standard_robot/application/CAN_receive.c`：CAN 反馈分发和聚合控制帧组织。
- `20.standard_robot/bsp/boards/bsp_fric.c` 与 `Src/tim.c`：TIM1 CH1/CH2 PWM 写入与 PE9/PE11 复用。

参考代码只用于协议和任务组织交叉核对。当前工程的 C615 Ramp、M2006 连续角度和安全
超时逻辑按本项目的 `.ioc`、任务边界和硬件假设单独实现，不能把示例中的电机参数直接
视为本板卡的实测值。


## 2026-10-17 INS 资料核对

- 直接读取并检查原PDF：`参考文档/RoboMaster开发板C型嵌入式软件教程文档.pdf`（296页），第11/13/18章；对第260页原理图做渲染检查。IST8310使用PA8 SCL、PC9 SDA，教程第261页标签有颠倒，不能照抄该标签。
- `参考文档/BMI088.PDF`（62页）：BST-BMI088-DS000-19，Rev1.9，2024-01。核对初始化、dummy byte、SPI10MHz上限、量程/ODR、低有效中断、温度11位补码和第56页轴向图。
- 用户指定的 `spi_bmi088`、`ist8310` 保持只读；同时核对本地官方 `Development-Board-C-Examples-master/18.ins_task`。本项目修正磁模式0x03→0x0B、磁场单位和陀螺归一化分母32768，并采用/16，避免当前APB2/8=10.5MHz超过手册上限。未复制示例的直接外设寄存器DMA调度、软件EXTI唤醒或加热控制。
- PDF用于技术依据；安装方向、磁校准和实机精度仍以用户设备测量为准。临时渲染图在系统临时目录，不加入工程。

## 2026-10-18 MotionCal 协议核对

用户粘贴的文章用于说明磁标定流程；广告、演示参数和地理航向示例不作为工程指令或本机标定结果。实现核对PJRC官方源码，固定版本为 `c24740cd8f8a2ccd5f8aa06be8965ff52b7ee920`（2026-10-18用git ls-remote确认仍为HEAD）：

- [下载入口](https://www.pjrc.com/store/prop_shield.html)。
- [serialdata.c](https://github.com/PaulStoffregen/MotionCal/blob/c24740cd8f8a2ccd5f8aa06be8965ff52b7ee920/serialdata.c)：`Raw:`解析存入int16_t[9]；RAW状态不接受空格，CR提交、LF忽略。Windows COM及POSIX串口固定115200，当前用户已生成115200/8N1。旧固件带空格格式被官方解析器复现拒收，已改为无空格CRLF。
- [imuread.h](https://github.com/PaulStoffregen/MotionCal/blob/c24740cd8f8a2ccd5f8aa06be8965ff52b7ee920/imuread.h)：`UT_PER_COUNT=0.1`，硬铁V单位μT，invW为无量纲软铁校正矩阵。IST8310原始值乘3匹配0.3μT/LSB，不使用文章中的任意×1000。
- [rawdata.c](https://github.com/PaulStoffregen/MotionCal/blob/c24740cd8f8a2ccd5f8aa06be8965ff52b7ee920/rawdata.c)：前三字段为加速度、接着三个为陀螺、末三字段为磁场。本模式前六项固定零，仅做磁标定，不承诺姿态动画正确。
- [gui.cpp](https://github.com/PaulStoffregen/MotionCal/blob/c24740cd8f8a2ccd5f8aa06be8965ff52b7ee920/gui.cpp)：`Magnetic Offset`显示V（μT），`Magnetic Mapping`按行列显示invW；质量参考为Gaps<15%、Variance<4.5%、Wobble<4%、Fit Error<5%。固件不接收Send Cal，使用人工回填。

本次只读获取官方源码，在临时主机程序中执行实际ASCII解析函数及 `rawdata.c`，验证格式能进入真实磁标定缓冲；融合/拟合依赖使用替身。随后用户提供MotionCal COM8实际点云截图及候选参数，接收显示已确认；四项质量均未达参考条件，补采及融合效果待验证，数值与来源记录在INS模块。用户所用二进制的确切版本未确认。
