# Robomaster-for-VS-Code-Studying

RoboMaster 开发板 C 型 / STM32F407 的嵌入式学习示例集合。

本仓库把常见外设、传感器、通信协议和 RoboMaster 相关模块拆成多个独立工程，适合用 **VS Code + CMake + STM32CubeMX + arm-none-eabi-gcc** 逐个学习、构建、烧录和调试。

> 每个示例目录都是一个独立 STM32 工程。使用时先进入目标模块目录，再执行配置、构建和烧录。

## 适合谁

- 刚开始学习 STM32F4 / HAL / CMake 工程结构的同学
- 想用 VS Code 替代传统 IDE 管理 STM32CubeMX 工程的同学
- RoboMaster 队伍中需要快速查阅基础外设、CAN、电机、遥控器、IMU 等示例的成员
- 想把单个外设示例拆开验证，再逐步组合到完整机器人代码中的开发者

## 快速开始

以下以 `LED` 示例为例。其他模块的使用方式相同，只需要把目录名换成目标模块。

```powershell
# 进入一个独立示例工程
cd LED

# 使用 Debug 预设配置工程
cmake --preset Debug

# 构建 Debug 版本
cmake --build --preset Debug

# 查看生成的固件文件
Get-ChildItem build\Debug\*.elf
```

构建产物通常位于当前模块的 `build/Debug/` 目录，例如：

- `.elf`：用于调试和烧录的主固件文件
- `.map`：链接映射文件，可查看段分布和内存占用
- `.hex`：部分模块会在构建后额外生成 Intel HEX 文件

## 仓库结构

```text
.
├── adc/              # ADC 示例
├── aRGB_LED/         # RGB LED / PWM 示例
├── botton/           # 按键输入示例，目录名保留原始拼写
├── buzzer/           # 蜂鸣器 PWM 示例
├── can/              # CAN 基础封装示例
├── can_example/      # CAN 应用示例
├── flash/            # Flash 读写示例
├── ist8310/          # IST8310 磁力计示例
├── key/              # 按键抽象与状态处理示例
├── LED/              # 板载 LED 示例
├── oled/             # OLED 显示示例
├── servo/            # 舵机 PWM 示例
├── spi_bmi088/       # BMI088 IMU 示例
├── tim_light/        # 定时器灯光效果示例
├── uart_dma_dbus/    # UART DMA + DBUS 遥控器示例
├── usart/            # USART 通信示例
├── 笔记/             # 学习笔记
├── 参考文档/         # 芯片、开发板、协议和器件资料
└── readme.md
```

典型模块内部结构如下：

```text
模块名/
├── .ioc                         # STM32CubeMX 配置文件
├── CMakeLists.txt               # 当前模块的 CMake 工程入口
├── CMakePresets.json            # Debug / Release 构建预设
├── cmake/
│   ├── gcc-arm-none-eabi.cmake   # GCC 交叉编译工具链配置
│   ├── starm-clang.cmake         # 可选工具链配置
│   └── stm32cubemx/
├── Core/
│   ├── Inc/                      # 用户头文件和 CubeMX 生成头文件
│   └── Src/                      # 用户源文件和 CubeMX 生成源文件
├── Drivers/
│   ├── CMSIS/
│   └── STM32F4xx_HAL_Driver/
├── startup_stm32f407xx.s         # 启动文件
└── STM32F407XX_FLASH.ld          # 链接脚本
```

## 示例模块索引

| 模块 | 主要内容 | 学习重点 |
|---|---|---|
| `adc` | ADC 采样 | 模拟量读取、ADC 配置、数据转换 |
| `aRGB_LED` | RGB LED 控制 | 定时器 PWM、多通道占空比控制 |
| `botton` | 按键输入 | GPIO 输入、外部中断、按键触发逻辑 |
| `buzzer` | 蜂鸣器 | PWM 输出、频率和占空比控制 |
| `can` | CAN 基础封装 | CAN 初始化、收发接口、基础抽象 |
| `can_example` | CAN 应用 | RoboMaster 常见 CAN 设备通信示例 |
| `flash` | Flash 存储 | SPI 通信、擦除、写入、读取 |
| `ist8310` | IST8310 磁力计 | I2C 通信、传感器寄存器读写 |
| `key` | 按键抽象 | C 语言结构体封装、状态处理循环 |
| `LED` | 板载 LED | GPIO 输出、基础延时、最小上手工程 |
| `oled` | OLED 显示 | I2C 显示驱动、字符显示、显存刷新 |
| `servo` | 舵机控制 | 50 Hz PWM、脉宽控制、角度映射 |
| `spi_bmi088` | BMI088 IMU | SPI 通信、加速度计、陀螺仪、数据读取 |
| `tim_light` | 灯光效果 | 定时器、PWM、周期性状态变化 |
| `uart_dma_dbus` | 遥控器 DBUS | UART、DMA、RoboMaster 遥控器数据解析 |
| `usart` | 串口通信 | USART 初始化、中断或轮询收发、调试输出 |

## 开发环境

建议准备以下工具：

| 工具 | 用途 |
|---|---|
| VS Code | 代码编辑、CMake 集成、调试入口 |
| STM32CubeMX | 打开和调整 `.ioc` 配置，重新生成初始化代码 |
| CMake 3.22 或更新版本 | 配置和生成构建系统 |
| Ninja | 配合当前 `CMakePresets.json` 使用 |
| Arm GNU Toolchain | 提供 `arm-none-eabi-gcc`、`arm-none-eabi-objcopy` 等工具 |
| STM32CubeProgrammer | 烧录、擦除、读取 STM32 固件 |
| OpenOCD / ST-Link / J-Link | 在线调试或命令行烧录，根据手头调试器选择 |

常用 VS Code 扩展：

- CMake Tools
- clangd 或 C/C++ 扩展
- Cortex-Debug
- STM32CubeMX / STM32 VS Code 相关扩展，可按个人工作流选择

确认工具链是否可用：

```powershell
cmake --version
ninja --version
arm-none-eabi-gcc --version
```

## 构建、烧录与调试

### 1. 选择模块

不要在仓库根目录直接构建。先进入要学习的示例目录：

```powershell
cd uart_dma_dbus
```

### 2. 配置和构建

```powershell
cmake --preset Debug
cmake --build --preset Debug
```

如果要构建 Release 版本：

```powershell
cmake --preset Release
cmake --build --preset Release
```

### 3. 烧录

推荐先使用 STM32CubeProgrammer 手动选择当前模块生成的 `.elf` 或 `.hex` 文件烧录。

如果使用 OpenOCD，可以按自己的调试器配置调整命令。以 `LED` 模块为例：

```powershell
openocd -f interface/cmsis-dap.cfg -f target/stm32f4x.cfg -c "program build/Debug/led.elf verify reset exit"
```

不同模块的 `.elf` 文件名由各自 `CMakeLists.txt` 中的工程名决定。烧录前可以先查看：

```powershell
Get-ChildItem build\Debug\*.elf
```

### 4. 调试

部分模块包含 `.vscode/launch.json`，可以直接从 VS Code 的 Run and Debug 面板启动调试。

调试配置按模块保存。不同模块的配置数量不完全相同，例如 `LED` 当前只保留 J-Link 配置，而 `uart_dma_dbus` 同时包含 ST-Link、J-Link 和 OpenOCD(DAPLINK) 三种配置。下面保留一个完整的 `launch.json` 示例，使用前按本机调试器和安装路径调整。

```jsonc
{
    "version": "0.2.0",
    "configurations": [
        {
            "type": "stlinkgdbtarget",
            "request": "launch",
            "name": "STM32Cube: Launch ST-Link GDB Server",
            "origin": "snippet",
            "cwd": "${workspaceFolder}",
            "preBuild": "${command:st-stm32-ide-debug-launch.build}",
            "runEntry": "main",
            "imagesAndSymbols": [
                {
                    "imageFileName": "${command:st-stm32-ide-debug-launch.get-projects-binary-from-context1}"
                }
            ]
        },
        {
            "type": "jlinkgdbtarget",
            "request": "launch",
            "name": "STM32Cube: Launch JLink GDB Server",
            "origin": "snippet",
            "cwd": "${workspaceFolder}",
            "preBuild": "${command:st-stm32-ide-debug-launch.build}",
            "runEntry": "main",
            "imagesAndSymbols": [
                {
                    "imageFileName": "${command:st-stm32-ide-debug-launch.get-projects-binary-from-context1}"
                }
            ]
        },
        {
            "cwd": "${workspaceRoot}",
            "executable": "${command:st-stm32-ide-debug-launch.get-projects-binary-from-context1}",
            "name": "Debug with OpenOCD(DAPLINK)",
            "request": "launch",
            "type": "cortex-debug",
            "servertype": "openocd",
            "configFiles": [
                "interface/cmsis-dap.cfg",
                "target/stm32f4x.cfg"
            ],
            "searchDir": [],
            "runToEntryPoint": "main",
            "showDevDebugOutput": "none",
            "openOCDPath": "D:/xpack-openocd-0.12.0-7/bin/openocd.exe"
        }
    ]
}
```

调试前需要确认：

- 当前 VS Code 打开的目录是具体模块目录，而不是仓库根目录
- 已经成功构建并生成 `.elf`
- `launch.json` 中的 OpenOCD、ST-Link 或 J-Link 路径符合本机安装位置
- 开发板已连接，BOOT 和供电状态正确

## 学习路线建议

建议按从基础到综合的顺序阅读和实验：

1. `LED`：确认工具链、烧录和 GPIO 输出流程可用
2. `botton` / `key`：学习输入、事件和状态处理
3. `buzzer` / `servo` / `tim_light` / `aRGB_LED`：学习定时器和 PWM
4. `usart` / `uart_dma_dbus`：学习串口、DMA 和遥控器数据解析
5. `adc` / `flash` / `oled`：学习采样、存储和显示
6. `ist8310` / `spi_bmi088`：学习传感器通信和数据读取
7. `can` / `can_example`：学习 RoboMaster 场景中的 CAN 通信

## 学习笔记

`笔记/` 目录用于补充工程外的基础知识和学习记录：

- [`笔记/代码的一生.md`](笔记/代码的一生.md)：从源代码到编译、链接、烧录、启动运行的流程梳理
- [`笔记/字符串与字符串字面量.md`](笔记/字符串与字符串字面量.md)：C 语言字符串和字符串字面量相关知识

## 参考文档

`参考文档/` 目录保存了本仓库学习过程中会用到的本地资料，包括：

- RoboMaster 开发板 C 型用户手册、使用说明、原理图和位号图
- STM32F4 参考手册和相关芯片资料
- CAN 总线协议资料
- RoboMaster 电机和电调资料
- BMI088、IST8310 等器件资料

这些资料适合在调引脚、查寄存器、确认接口电气连接和理解通信协议时配合代码阅读。

## 注意事项

- 本仓库中的示例工程相互独立，修改一个模块不会自动同步到其他模块。
- 使用 STM32CubeMX 重新生成代码前，先确认用户代码写在 `USER CODE BEGIN` / `USER CODE END` 区域或独立文件中。
- `botton` 目录名保留当前仓库原始拼写，实际功能是按键输入。
- 不同电脑上的 OpenOCD、J-Link、ST-Link 路径可能不同，需要按本机环境修改调试配置。
- 仓库根目录未提供统一许可证文件；引用 ST HAL、CMSIS 或厂商资料时，请遵循对应来源的许可要求。
