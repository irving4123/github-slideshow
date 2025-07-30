#ifndef PWM_DRIVER_H
#define PWM_DRIVER_H

#include "config.h"

// ============================================================================
// PWM配置定义
// ============================================================================

// PWM通道定义
typedef enum {
    PWM_CHANNEL_A = 0,      // A相上桥臂
    PWM_CHANNEL_B,          // B相上桥臂
    PWM_CHANNEL_C,          // C相上桥臂
    PWM_CHANNEL_A_LOW,      // A相下桥臂
    PWM_CHANNEL_B_LOW,      // B相下桥臂
    PWM_CHANNEL_C_LOW       // C相下桥臂
} pwm_channel_t;

// PWM模式定义
typedef enum {
    PWM_MODE_DISABLE = 0,   // 禁用模式
    PWM_MODE_COMPLEMENTARY,  // 互补模式
    PWM_MODE_INDEPENDENT,    // 独立模式
    PWM_MODE_SYNCHRONOUS     // 同步模式
} pwm_mode_t;

// PWM死区时间配置
typedef struct {
    uint16_t dead_time_rising;   // 上升沿死区时间
    uint16_t dead_time_falling;  // 下降沿死区时间
    uint8_t dead_time_enable;    // 死区时间使能
} pwm_dead_time_config_t;

// PWM配置结构体
typedef struct {
    uint32_t frequency;          // PWM频率(Hz)
    uint16_t period;             // PWM周期
    uint16_t duty_cycle[6];      // 各通道占空比
    pwm_mode_t mode;             // PWM模式
    pwm_dead_time_config_t dead_time; // 死区时间配置
    uint8_t enable_mask;         // 通道使能掩码
} pwm_config_t;

// PWM状态结构体
typedef struct {
    uint8_t is_initialized;      // 初始化状态
    uint8_t is_enabled;          // 使能状态
    uint8_t channel_status[6];   // 各通道状态
    uint32_t update_count;       // 更新计数
    uint32_t fault_count;        // 故障计数
} pwm_status_t;

// ============================================================================
// 函数声明
// ============================================================================

// PWM驱动初始化
void pwm_driver_init(void);

// PWM配置设置
void pwm_set_config(pwm_config_t* config);

// PWM使能/禁用
void pwm_enable(uint8_t enable);

// PWM通道使能/禁用
void pwm_channel_enable(pwm_channel_t channel, uint8_t enable);

// 设置PWM频率
void pwm_set_frequency(uint32_t frequency);

// 设置PWM周期
void pwm_set_period(uint16_t period);

// 设置PWM占空比
void pwm_set_duty_cycle(pwm_channel_t channel, uint16_t duty_cycle);

// 设置三相PWM占空比
void pwm_set_three_phase_duty(uint16_t duty_a, uint16_t duty_b, uint16_t duty_c);

// 设置PWM模式
void pwm_set_mode(pwm_mode_t mode);

// 设置死区时间
void pwm_set_dead_time(pwm_dead_time_config_t* dead_time);

// 设置PWM同步
void pwm_set_sync(uint8_t enable);

// PWM更新
void pwm_update(void);

// PWM紧急停止
void pwm_emergency_stop(void);

// PWM故障处理
void pwm_fault_handler(void);

// 获取PWM配置
pwm_config_t* pwm_get_config(void);

// 获取PWM状态
pwm_status_t* pwm_get_status(void);

// PWM自检
uint8_t pwm_self_test(void);

// PWM校准
void pwm_calibrate(void);

// PWM调试
void pwm_debug_print(void);

// PWM参数显示
void pwm_parameter_display(void);

// ============================================================================
// 内联函数
// ============================================================================

// 检查PWM是否初始化
static inline uint8_t pwm_is_initialized(void) {
    pwm_status_t* status = pwm_get_status();
    return status->is_initialized;
}

// 检查PWM是否使能
static inline uint8_t pwm_is_enabled(void) {
    pwm_status_t* status = pwm_get_status();
    return status->is_enabled;
}

// 检查PWM通道是否使能
static inline uint8_t pwm_channel_is_enabled(pwm_channel_t channel) {
    pwm_status_t* status = pwm_get_status();
    return status->channel_status[channel];
}

// 设置PWM占空比百分比
static inline void pwm_set_duty_percent(pwm_channel_t channel, float percent) {
    uint16_t duty = (uint16_t)(percent * 65535.0f / 100.0f);
    pwm_set_duty_cycle(channel, duty);
}

// 设置三相PWM占空比百分比
static inline void pwm_set_three_phase_duty_percent(float percent_a, 
                                                   float percent_b, 
                                                   float percent_c) {
    uint16_t duty_a = (uint16_t)(percent_a * 65535.0f / 100.0f);
    uint16_t duty_b = (uint16_t)(percent_b * 65535.0f / 100.0f);
    uint16_t duty_c = (uint16_t)(percent_c * 65535.0f / 100.0f);
    pwm_set_three_phase_duty(duty_a, duty_b, duty_c);
}

#endif // PWM_DRIVER_H