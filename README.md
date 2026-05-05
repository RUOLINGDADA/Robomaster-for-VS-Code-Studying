# Robomaster-for-VS-Code-Studying

[![License](https://img.shields.io/badge/License-MIT-blue.svg)](https://opensource.org/licenses/MIT)
[![Language](https://img.shields.io/badge/Language-C%2B%2B%2FCMake-green.svg)](https://isocpp.org/)
[![Platform](https://img.shields.io/badge/Platform-STM32F407-orange.svg)](https://www.st.com/en/microcontrollers-microprocessors/stm32f407.html)
[![RoboMaster](https://img.shields.io/badge/RoboMaster-Learning-red.svg)](https://www.robomaster.com/)

RoboMaster 开发板学习项目，基于 VS Code + CMake + STM32CubeMX 的嵌入式开发工作流。本项目包含多个硬件模块的示例代码，用于学习STM32F407微控制器的各种外设和接口。

## 项目结构

本项目由多个独立的模块组成，每个模块对应一个硬件功能或外设。每个模块文件夹包含：

- `.ioc` 文件：STM32CubeMX 项目配置文件
- `CMakeLists.txt`：CMake 构建脚本
- `CMakePresets.json`：CMake 预设配置
- `Core/`：STM32CubeMX 生成的核心代码（Inc/ 和 Src/）
- `Drivers/`：HAL 库和 CMSIS 驱动
- `build/`：构建输出目录
- `cmake/`：CMake 工具链配置

### 模块列表

| 模块名称 | 功能描述 |
|----------|----------|
| `adc` | 模数转换器 (ADC) 示例，演示如何读取模拟信号 |
| `aRGB_LED` | RGB LED 控制，使用 PWM 实现颜色变化 |
| `botton` | 按钮输入检测，包含去抖动处理 |
| `buzzer` | 蜂鸣器驱动，通过定时器产生音频信号 |
| `flash` | 外部闪存读写操作 |
| `ist8310` | IST8310 磁力计传感器驱动 |
| `LED` | 板载 LED 灯控制 |
| `oled` | OLED 显示屏驱动，支持文本和图形显示 |
| `servo` | 舵机控制，使用 PWM 信号驱动 |
| `spi_bmi088` | BMI088 IMU 传感器 (SPI 接口) |
| `tim_light` | 定时器控制的灯光效果 |
| `uart_dma_dbus` | UART DMA DBUS 通信协议实现 |
| `usart` | 通用串口通信示例 |

## 硬件平台

- **MCU**: STM32F407IGH6 (UFBGA176 封装)
- **系统主频**: 168MHz (HSE + PLL)
- **调试接口**: SWD (PA13/PA14)
- **板上 LED**: PH10 (蓝) / PH11 (绿) / PH12 (红)

## 开发环境

- **IDE**: VS Code + clangd 语言服务器
- **构建系统**: CMake + arm-none-eabi-gcc 交叉编译工具链
- **配置工具**: STM32CubeMX (生成初始化代码与 CMakeLists)
- **烧录方式**: 生成 .elf / .hex 文件，可使用 ST-Link 或 J-Link 烧录

## 构建步骤

1. **安装依赖**：
   - 安装 [arm-none-eabi-gcc](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm) 工具链
   - 安装 CMake (版本 3.16+)
   - 安装 VS Code 扩展：C/C++、CMake Tools、clangd

2. **克隆项目**：
   ```bash
   git clone <repository-url>
   cd Robomaster-for-VS-Code-Studying
   ```

3. **构建特定模块**：
   ```bash
   cd <module-name>  # 例如 cd adc
   mkdir build
   cd build
   cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/gcc-arm-none-eabi.cmake
   make
   ```

4. **烧录到开发板**：
   使用 ST-Link Utility 或将生成的 .elf 文件烧录到 STM32F407。

## 使用说明

每个模块都是独立的示例项目，可以单独构建和运行。进入对应模块目录，按照上述构建步骤操作即可。

- 修改代码后，重新运行 `make` 即可编译
- 使用 VS Code 的调试功能，可以通过 Cortex-Debug 扩展进行在线调试

## 调试配置

本项目使用 VS Code 的 `.vscode/launch.json` 进行在线调试，支持三种调试器。以 `uart_dma_dbus` 模块为例：

```jsonc
{
    "version": "0.2.0",
    "configurations": [
        {
            // ============================================================
            // 配置 1：ST-Link GDB Server（ST 官方调试器）
            // ============================================================
            "type": "stlinkgdbtarget",                    // 调试器类型：ST-Link GDB 目标
            "request": "launch",                          // 启动模式：每次调试重新启动程序
            "name": "STM32Cube: Launch ST-Link GDB Server", // 配置名称（VS Code 下拉菜单显示）
            "origin": "snippet",                          // 来源：预设代码片段
            "cwd": "${workspaceFolder}",                  // 工作目录：当前打开的项目文件夹
            "preBuild": "${command:st-stm32-ide-debug-launch.build}",
                                                          // 调试前构建命令：自动调用 STM32 IDE 构建任务
            "runEntry": "main",                           // 运行入口：从 main 函数开始
            "imagesAndSymbols": [
                {
                    "imageFileName": "${command:st-stm32-ide-debug-launch.get-projects-binary-from-context1}"
                                                          // 符号文件：自动获取当前项目的 .elf 文件路径
                }
            ]
        },
        {
            // ============================================================
            // 配置 2：J-Link GDB Server（SEGGER 调试器）
            // ============================================================
            "type": "jlinkgdbtarget",                    // 调试器类型：J-Link GDB 目标
            "request": "launch",                          // 启动模式：每次调试重新启动程序
            "name": "STM32Cube: Launch JLink GDB Server", // 配置名称（VS Code 下拉菜单显示）
            "origin": "snippet",                          // 来源：预设代码片段
            "cwd": "${workspaceFolder}",                  // 工作目录：当前打开的项目文件夹
            "preBuild": "${command:st-stm32-ide-debug-launch.build}",
                                                          // 调试前构建命令：自动调用 STM32 IDE 构建任务
            "runEntry": "main",                           // 运行入口：从 main 函数开始
            "imagesAndSymbols": [
                {
                    "imageFileName": "${command:st-stm32-ide-debug-launch.get-projects-binary-from-context1}"
                                                          // 符号文件：自动获取当前项目的 .elf 文件路径
                }
            ]
        },
        {
            // ============================================================
            // 配置 3：OpenOCD + CMSIS-DAP（DAPLINK 调试器）
            //   通用开源调试方案，支持 DAPLINK / ST-Link / J-Link 等
            // ============================================================
            "cwd": "${workspaceRoot}",                    // 工作目录：VS Code 工作区根目录
            "executable": "${command:st-stm32-ide-debug-launch.get-projects-binary-from-context1}",
                                                          // 可执行文件：自动获取当前项目的 .elf 文件路径
            "name": "Debug with OpenOCD(DAPLINK)",        // 配置名称（VS Code 下拉菜单显示）
            "request": "launch",                          // 启动模式：每次调试重新启动程序
            "type": "cortex-debug",                       // 调试器类型：Cortex-Debug 扩展（通用 ARM Cortex-M 调试）
            "servertype": "openocd",                      // GDB Server 类型：使用 OpenOCD 作为后端
            "configFiles": [
                "interface/cmsis-dap.cfg",                // 调试器接口配置：CMSIS-DAP 协议（DAPLINK）
                "target/stm32f4x.cfg"                     // 目标芯片配置文件：STM32F4 系列
            ],
            "searchDir": [],                              // OpenOCD 脚本搜索目录（留空使用默认路径）
            "runToEntryPoint": "main",                    // 程序入口：下载后自动运行到 main 函数断点
            "showDevDebugOutput": "none",                 // GDB Server 调试输出级别：none（不显示原始输出）
            "openOCDPath": "D:/xpack-openocd-0.12.0-7/bin/openocd.exe"
                                                          // OpenOCD 可执行文件路径（需根据本地安装路径修改）
        }
    ]
}
```

### 三种调试器对比

| 特性 | ST-Link | J-Link | OpenOCD + DAPLINK |
|------|---------|--------|-------------------|
| **硬件成本** | 低（开发板自带） | 较高（需额外购买） | 低（开源方案） |
| **调试速度** | 较快 | 最快 | 中等 |
| **软件安装** | STM32CubeIDE 内置 | 需安装 J-Link 驱动 | 需安装 OpenOCD |
| **适用芯片** | STM32 全系列 | 所有 ARM Cortex-M 芯片 | 所有 ARM Cortex-M 芯片 |
| **断点数量** | 6 个硬件断点 | 无限制（Flash 断点） | 6 个硬件断点 |

### 使用前提

- **ST-Link / J-Link**：需安装 [STM32CubeIDE](https://www.st.com/en/development-tools/stm32cubeide.html) 或对应的 VS Code 扩展
- **OpenOCD + DAPLINK**：需安装 [Cortex-Debug](https://marketplace.visualstudio.com/items?itemName=marus25.cortex-debug) 扩展及 [xPack OpenOCD](https://github.com/xpack-dev-tools/openocd-xpack/releases)
- 调试前需完成项目构建，生成 `.elf` 文件
- `openOCDPath` 路径需根据本地 OpenOCD 安装位置修改

## 代码示例

以下是一些关键模块的核心代码示例：

### LED 控制 (LED 模块)

```c
// main.c 中的主循环，实现 LED 闪烁
while (1)
{
    HAL_GPIO_TogglePin(GPIOH, LED_B_Pin);
    nop_delay_ms(1000);
    HAL_GPIO_TogglePin(GPIOH, LED_B_Pin);
    nop_delay_ms(1000);
}
```

### ADC 读取 (ADC 模块)

```c
// 获取电池电压
float get_battery_voltage(void)
{
    float bat_adc = get_adcx_value(&hadc3, ADC_CHANNEL_8, 0, 1, ADC_SAMPLETIME_480CYCLES, 100);
    // 10.09为分压比
    float bat_voltage = bat_adc * voltage_calibration_coefficient() * 10.09f;
    return bat_voltage;
}

// 获取温度
float get_temperature(void)
{
    uint16_t adcx = 0;
    float temperate;
    adcx = get_adcx_value(&hadc1, ADC_CHANNEL_TEMPSENSOR, 0, 1, ADC_SAMPLETIME_480CYCLES, 100);
    temperate = (float)adcx * voltage_calibration_coefficient();
    temperate = (temperate - 0.76f) * 400.0f + 25.0f;
    return temperate;
}
```

### 串口通信 (USART 模块)

```c
// main.c 中的串口发送示例
while (1)
{
    HAL_GPIO_WritePin(GPIOH, GREEN_LED_Pin, SET);  // 绿灯亮
    // 启动中断发送
    if (HAL_UART_Transmit_IT(&huart1, str, sizeof(str) - 1) != HAL_OK)
    {
        // 如果启动失败，强制切回空闲状态
        HAL_GPIO_WritePin(GPIOH, GREEN_LED_Pin, RESET);
    }
    HAL_Delay(1000);
}
```

## 参考资料

- [STM32F407 数据手册](https://www.st.com/resource/en/datasheet/stm32f407vg.pdf)
- [STM32 HAL 库文档](https://www.st.com/en/embedded-software/stm32cube-mcu-packages.html)
- [RoboMaster 开发板资料](https://www.robomaster.com/)
- [ARM Cortex-M4 技术参考手册](https://developer.arm.com/documentation/ddi0439/latest/)

## 许可证

本项目仅供学习交流使用。HAL 库及 CMSIS 代码遵循 ST 和 ARM 各自的许可证协议。

## 贡献

欢迎提交 Issue 和 Pull Request 来改进这个项目！

## 问题反馈

如果您在使用过程中遇到问题，请：

1. 检查构建环境是否正确配置
2. 查看相关模块的代码注释
3. 在 GitHub Issues 中提交详细的问题描述

## 标签

- **嵌入式开发**: STM32, ARM Cortex-M4, 嵌入式系统
- **机器人**: RoboMaster, 机器人学习
- **工具链**: VS Code, CMake, STM32CubeMX
- **外设**: ADC, GPIO, UART, SPI, I2C, PWM, 传感器
