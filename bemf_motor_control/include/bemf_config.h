#ifndef BEMF_CONFIG_H
#define BEMF_CONFIG_H

#include <reg51.h>
#include <intrins.h>

// 系统配置
#define SYSTEM_CLOCK        24000000UL  // 24MHz系统时钟
#define PWM_FREQUENCY       20000       // 20kHz PWM频率
#define ADC_RESOLUTION      4096        // 12位ADC分辨率

// 电机参数配置
#define MOTOR_POLE_PAIRS    4           // 电机极对数
#define STARTUP_DUTY        20          // 启动占空比 (%)
#define MAX_DUTY_CYCLE      90          // 最大占空比 (%)
#define MIN_DUTY_CYCLE      5           // 最小占空比 (%)

// BEMF检测参数
#define BEMF_THRESHOLD      512         // BEMF检测阈值 (ADC值)
#define BEMF_FILTER_COUNT   3           // BEMF滤波计数
#define COMMUTATION_DELAY   30          // 换相延迟 (us)

// 启动参数
#define STARTUP_STEPS       6           // 启动步数
#define STARTUP_RAMP_TIME   100         // 启动斜坡时间 (ms)
#define BEMF_DETECT_SPEED   500         // BEMF检测切换转速 (RPM)

// 保护参数
#define MAX_CURRENT         5000        // 最大电流 (mA)
#define MAX_VOLTAGE         48000       // 最大电压 (mV)
#define MAX_TEMPERATURE     85          // 最大温度 (°C)
#define STALL_TIMEOUT       2000        // 堵转超时 (ms)

// PWM通道定义
#define PWM_U_HIGH          P1_0        // U相上桥臂
#define PWM_U_LOW           P1_1        // U相下桥臂
#define PWM_V_HIGH          P1_2        // V相上桥臂
#define PWM_V_LOW           P1_3        // V相下桥臂
#define PWM_W_HIGH          P1_4        // W相上桥臂
#define PWM_W_LOW           P1_5        // W相下桥臂

// ADC通道定义
#define ADC_BEMF_U          0           // U相BEMF检测通道
#define ADC_BEMF_V          1           // V相BEMF检测通道
#define ADC_BEMF_W          2           // W相BEMF检测通道
#define ADC_CURRENT         3           // 电流检测通道
#define ADC_VOLTAGE         4           // 电压检测通道
#define ADC_TEMPERATURE     5           // 温度检测通道

// 比较器配置
#define COMP0_BEMF          0           // 比较器0用于BEMF检测
#define COMP1_CURRENT       1           // 比较器1用于过流保护

// 状态定义
typedef enum {
    MOTOR_STOP = 0,
    MOTOR_STARTUP,
    MOTOR_RUN,
    MOTOR_FAULT
} motor_state_t;

typedef enum {
    FAULT_NONE = 0,
    FAULT_OVERCURRENT,
    FAULT_OVERVOLTAGE,
    FAULT_OVERTEMP,
    FAULT_STALL
} fault_type_t;

// 换相序列定义
typedef struct {
    unsigned char u_high;
    unsigned char u_low;
    unsigned char v_high;
    unsigned char v_low;
    unsigned char w_high;
    unsigned char w_low;
} commutation_step_t;

#endif // BEMF_CONFIG_H