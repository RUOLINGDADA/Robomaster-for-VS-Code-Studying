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

- `20.standard_robot/application/gimbal_behaviour.c`：鼠标 X 负号、Y 正号与摇杆叠加；本项目换为千分比速度接口。
- `20.standard_robot/application/shoot.c`、`shoot.h`：摩擦轮 Ramp、按键触发与 0.314 rad 步进、0.05 rad 到位窗口；本项目按用户计划换为电机轴计数并加入速度/稳定确认。
- `20.standard_robot/application/remote_control.c`：DBUS 通道、鼠标和键盘字段解码。
- `20.standard_robot/application/CAN_receive.c`：CAN 反馈分发和聚合控制帧组织。
- `20.standard_robot/bsp/boards/bsp_fric.c` 与 `Src/tim.c`：TIM1 CH1/CH2 PWM 写入与 PE9/PE11 复用。

参考代码只用于协议和任务组织交叉核对。当前工程的 C615 Ramp、M2006 连续角度和安全
超时逻辑按本项目的 `.ioc`、任务边界和硬件假设单独实现，不能把示例中的电机参数直接
视为本板卡的实测值。
