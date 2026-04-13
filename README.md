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

## 项目结构

```
├── LED/                # 实验1：LED 闪烁（延时方式）
│   ├── Core/
│   │   ├── Inc/        # 头文件 (main.h, gpio.h, user_delay.h)
│   │   └── Src/        # 源文件 (main.c, gpio.c, user_delay.c)
│   ├── Drivers/        # HAL 库 + CMSIS
│   ├── cmake/          # CMake 工具链配置
│   ├── CMakeLists.txt
│   └── led.ioc         # STM32CubeMX 工程文件
│
├── aRGB_LED/           # 实验2：RGB LED 控制（PWM 方式）
│   ├── Core/
│   │   ├── Inc/        # 头文件 (main.h, gpio.h, tim.h)
│   │   └── Src/        # 源文件 (main.c, gpio.c, tim.c)
│   ├── Drivers/        # HAL 库 + CMSIS
│   ├── cmake/          # CMake 工具链配置
│   ├── CMakeLists.txt
│   └── aRGB_LED.ioc    # STM32CubeMX 工程文件
│
└── README.md
```

## 实验内容

### 实验1：LED — GPIO 延时闪烁

使用 GPIO 推挽输出控制 LED，通过自定义延时函数实现闪烁。

- **引脚配置**: PH10/PH11/PH12 → GPIO_Output (推挽输出)
- **核心逻辑**: `HAL_GPIO_TogglePin()` + `nop_delay_ms(1000)` 翻转蓝灯
- **自定义延时函数** ([user_delay.c](LED/Core/Src/user_delay.c)):
  | 函数 | 实现方式 | 精度 |
  |------|---------|------|
  | `user_delay_us()` | 嵌套 for 循环 + volatile | 低 (误差 5~10 倍) |
  | `nop_delay_us()` | `__NOP()` 空操作计数 | 中 (基于 168MHz) |
  | `nop_delay_ms()` | `__NOP()` 毫秒延时 | 中 (基于 168MHz) |
  | `delay_us()` | `SysTick->VAL` 计数器 | 高 (最大约 99864μs) |

### 实验2：aRGB_LED — TIM5 PWM RGB 控制

使用 TIM5 的 3 路 PWM 通道驱动 RGB LED，实现多种灯效。

- **引脚配置**: PH10→TIM5_CH1(蓝), PH11→TIM5_CH2(绿), PH12→TIM5_CH3(红)
- **定时器参数**: 预分频=0, ARR=254, 时钟分频=1 → PWM 分辨率 255 级
- **颜色格式**: `0x00RRGGBB` (32 位，高 8 位未用)
- **灯效函数** ([tim.c](aRGB_LED/Core/Src/tim.c)):
  | 函数 | 功能 |
  |------|------|
  | `aRGB_LED_Show(uint32_t)` | 显示指定颜色，提取 R/G/B 分量写入 CCR |
  | `fadeRGB()` | 线性渐变：红→绿→蓝依次呼吸 |
  | `Breath_LED_Smooth(uint8_t shift)` | 正弦波呼吸灯，单色柔和渐亮渐灭 |
  | `RGB_Flow_Fade()` | 彩色流转：蓝→绿→红→蓝，每 500ms 平滑过渡 |

- **当前运行效果**: `RGB_Flow_Fade()` — RGB 三色循环渐变流转

## 快速开始

### 依赖安装

1. **ARM 交叉编译工具链**
   ```bash
   # Windows: 下载并安装 arm-none-eabi-gcc
   # https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm
   ```

2. **CMake** (≥ 3.22)
   ```bash
   # Windows: 通过 winget 或官网下载安装
   winget install Kitware.CMake
   ```

3. **Ninja** (推荐构建工具)
   ```bash
   winget install Ninja-build.Ninja
   ```

4. **VS Code 插件**
   - clangd (代码补全/跳转)
   - Cortex-Debug (调试支持)

### 构建项目

```bash
# 进入项目目录
cd LED

# 配置 CMake (使用 Ninja 生成器)
cmake -B build -G Ninja

# 编译
cmake --build build

# 输出文件
# build/led.elf  - ELF 格式可执行文件
# build/led.hex  - Intel HEX 格式烧录文件
```

### 烧录方式

推荐使用 **OpenOCD** 或 **ST-Link Utility**：

```bash
# OpenOCD 烧录示例
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
  -c "program build/led.elf verify reset exit"
```

或使用 ST-Link Utility 图形界面加载 `.hex` 文件。

## 技术原理

### 延时函数实现原理

| 方法 | 原理 | 优点 | 缺点 |
|------|------|------|------|
| 软件循环 | `for + volatile` 消耗 CPU 周期 | 简单、无依赖 | 精度差、易被优化 |
| NOP 延时 | `__NOP()` 空指令精确计数 | 精度较高 | 占用 CPU、频率相关 |
| SysTick | 读取 `SysTick->VAL` 计数器 | 高精度、硬件级 | 有最大时长限制 |

**NOP 延时计算公式** (168MHz 主频):
```
1μs = 168 个时钟周期
NOP 延时循环次数 = 168 / 10 = 16.8 ≈ 17 次 (考虑循环开销)
```

### PWM 颜色控制原理

TIM5 配置为 PWM 模式，三通道分别控制 RGB 三色 LED：

```
PWM 频率 = 168MHz / (PSC + 1) / (ARR + 1)
         = 168MHz / 1 / 255
         ≈ 658.8 kHz

亮度等级 = CCR / ARR × 100%
        = CCR / 254 × 100%
```

**颜色混合示例**:
```c
// 纯红: R=255, G=0, B=0
aRGB_LED_Show(0x00FF0000);

// 紫色: R=255, G=0, B=255
aRGB_LED_Show(0x00FF00FF);

// 白色: R=255, G=255, B=255
aRGB_LED_Show(0x00FFFFFF);
```

### RGB_Flow_Fade 渐变算法

```
颜色流转: 蓝 → 绿 → 红 → 蓝 (循环)

每 500ms 完成一次颜色过渡:
1. 提取当前颜色 (R, G, B)
2. 计算目标颜色与当前颜色的差值
3. 差值 / 500ms = 每 1ms 的增量
4. 每 1ms 累加增量并更新 PWM 输出
```

## 代码示例

### 实验1: LED 闪烁主循环

```c
// LED/Core/Src/main.c
while (1)
{
    HAL_GPIO_TogglePin(GPIOH, LED_B_Pin);
    nop_delay_ms(1000);
    HAL_GPIO_TogglePin(GPIOH, LED_B_Pin);
    nop_delay_ms(1000);
}
```

### 实验2: RGB 渐变流转

```c
// aRGB_LED/Core/Src/main.c
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_TIM5_Init();
    
    // 启动三路 PWM
    HAL_TIM_PWM_Start(&htim5, TIM_CHANNEL_1);  // 蓝
    HAL_TIM_PWM_Start(&htim5, TIM_CHANNEL_2);  // 绿
    HAL_TIM_PWM_Start(&htim5, TIM_CHANNEL_3);  // 红
    
    while (1)
    {
        RGB_Flow_Fade();  // 彩色流转
    }
}
```

## 注意事项

1. **时钟配置**: 项目使用外部晶振 (HSE)，确保开发板晶振频率与代码配置一致 (6MHz 输入经 PLL 倍频至 168MHz)

2. **引脚冲突**: LED 项目使用 GPIO 推挽输出，aRGB_LED 项目使用 TIM5 复用功能，两者不能同时运行

3. **延时精度**: NOP 延时依赖 CPU 主频，若修改时钟配置需重新计算延时参数

4. **编译优化**: 高优化等级可能影响软件延时精度，建议使用 `-O0` 或 `-O1`

## 参考资料

- [STM32F407 数据手册](https://www.st.com/resource/en/datasheet/stm32f407vg.pdf)
- [STM32 HAL 库文档](https://www.st.com/en/embedded-software/stm32cube-mcu-packages.html)
- [RoboMaster 开发板资料](https://www.robomaster.com/)
- [ARM Cortex-M4 技术参考手册](https://developer.arm.com/documentation/ddi0439/latest/)

## 许可证

本项目仅供学习交流使用。HAL 库及 CMSIS 代码遵循 ST 和 ARM 各自的许可证协议。
