#ifndef PWM_DRIVER_H
#define PWM_DRIVER_H

#include "bemf_config.h"

// PWM相位定义
typedef enum {
    PWM_PHASE_U = 0,
    PWM_PHASE_V = 1,
    PWM_PHASE_W = 2
} pwm_phase_t;

// PWM驱动函数声明
void pwm_init(void);
void pwm_set_duty_cycle(unsigned char duty);
void pwm_set_phase_duty(pwm_phase_t phase, unsigned char duty);
void pwm_set_phase_state(pwm_phase_t phase, unsigned char high_state, unsigned char low_state);
void pwm_enable_phase(pwm_phase_t phase);
void pwm_disable_phase(pwm_phase_t phase);
void pwm_enable_all(void);
void pwm_disable_all(void);
void pwm_set_frequency(unsigned int frequency);
void pwm_emergency_stop(void);
void update_pwm_outputs(void);

// PWM状态查询函数
unsigned char pwm_get_duty_cycle(void);
unsigned char pwm_get_phase_duty(pwm_phase_t phase);
unsigned char pwm_is_phase_enabled(pwm_phase_t phase);

// 死区时间设置
void pwm_set_dead_time(unsigned char dead_time_us);

// PWM中断处理
void pwm_interrupt_handler(void) interrupt 3;

// 全局变量声明
extern volatile unsigned char current_duty_cycle;
extern volatile unsigned char phase_duty[3];
extern volatile unsigned char pwm_enabled;

#endif // PWM_DRIVER_H