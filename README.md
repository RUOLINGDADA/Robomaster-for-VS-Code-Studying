# Robomaster-for-VS-Code-Studying

RoboMaster 开发板学习项目，基于 VS Code + CMake + STM32CubeMX 的嵌入式开发工作流。

## 硬件平台

- **MCU**: STM32F407IGH6 (UFBGA176 封装)
- **系统主频**: 168MHz (HSE + PLL)
- **调试接口**: SWD (PA13/PA14)
- **板上 LED**: PH10 (蓝) / PH11 (绿) / PH12 (红)

## 开发环境

- **IDE**: VS Code + clangd
- **构建系统**: CMake + arm-none-eabi-gcc 交叉编译
- **配置工具**: STM32CubeMX (生成初始化代码与 CMakeLists)
- **烧录方式**: 生成 .elf / .hex 文件

## 参考资料

- [STM32F407 数据手册](https://www.st.com/resource/en/datasheet/stm32f407vg.pdf)
- [STM32 HAL 库文档](https://www.st.com/en/embedded-software/stm32cube-mcu-packages.html)
- [RoboMaster 开发板资料](https://www.robomaster.com/)
- [ARM Cortex-M4 技术参考手册](https://developer.arm.com/documentation/ddi0439/latest/)

## 许可证

本项目仅供学习交流使用。HAL 库及 CMSIS 代码遵循 ST 和 ARM 各自的许可证协议。
