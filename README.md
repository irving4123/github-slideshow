# 8051 MCU 无传感器 BEMF 电机控制解决方案

## 项目概述

本项目基于1T 8051内核FLASH型MCU，实现无传感器BEMF（反电动势）电机控制，支持带载启动功能。

## MCU 规格

- **内核**: 1T 8051 内核
- **主频**: 24MHz
- **存储器**: 18KB FLASH ROM, 2KB SRAM, 128B EEPROM-like
- **ADC**: 12位 ADC
- **模拟功能**: 2个模拟比较器, 1个运算放大器
- **PWM**: 6通道 16位 PWM（电机专用）
- **定时器**: 4个 16位定时器
- **通信**: 2路 UART, SPI
- **其他**: 硬件乘除法器、双两线调试

## 项目结构

```
├── src/
│   ├── main.c              # 主程序入口
│   ├── motor_control.c     # 电机控制核心算法
│   ├── bemf_detection.c    # BEMF检测算法
│   ├── pwm_driver.c        # PWM驱动
│   ├── adc_driver.c        # ADC驱动
│   ├── timer_driver.c      # 定时器驱动
│   ├── uart_driver.c       # UART通信
│   └── startup.c           # 启动和初始化
├── include/
│   ├── motor_control.h     # 电机控制头文件
│   ├── bemf_detection.h    # BEMF检测头文件
│   ├── pwm_driver.h        # PWM驱动头文件
│   ├── adc_driver.h        # ADC驱动头文件
│   ├── timer_driver.h      # 定时器驱动头文件
│   ├── uart_driver.h       # UART驱动头文件
│   └── config.h            # 配置文件
├── lib/
│   └── startup.a           # 启动库
└── docs/
    ├── hardware_spec.md    # 硬件规格说明
    ├── algorithm.md        # 算法说明
    └── user_manual.md      # 用户手册
```

## 核心功能

### 1. 无传感器BEMF检测
- 利用反电动势检测转子位置
- 实时相位检测和补偿
- 高精度位置估算算法

### 2. 带载启动
- 开环启动算法
- 平滑过渡到闭环控制
- 负载自适应启动策略

### 3. 电机控制
- 6通道PWM控制
- 电流闭环控制
- 速度闭环控制
- 位置闭环控制

## 编译和烧录

```bash
# 编译项目
make clean
make all

# 烧录到MCU
make flash
```

## 配置参数

在 `include/config.h` 中配置以下参数：
- 电机参数（极对数、电阻、电感等）
- 控制参数（PID参数、启动参数等）
- 硬件参数（PWM频率、ADC配置等）

## 许可证

MIT License
