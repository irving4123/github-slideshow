#include "../include/pwm_driver.h"

// 全局变量
volatile unsigned char current_duty_cycle = 0;
volatile unsigned char phase_duty[3] = {0, 0, 0};
volatile unsigned char pwm_enabled = 0;

// 静态变量
static unsigned int pwm_period = 0;
static unsigned char dead_time = 2;  // 默认2us死区时间
static unsigned char phase_high_state[3] = {0, 0, 0};
static unsigned char phase_low_state[3] = {0, 0, 0};

// PWM初始化
void pwm_init(void) {
    // 计算PWM周期
    pwm_period = SYSTEM_CLOCK / PWM_FREQUENCY;
    
    // 配置PWM相关寄存器 (假设使用专用PWM模块)
    // 这里需要根据具体MCU的PWM寄存器进行配置
    
    // 设置PWM频率
    pwm_set_frequency(PWM_FREQUENCY);
    
    // 初始化PWM输出引脚
    PWM_U_HIGH = 0;
    PWM_U_LOW = 0;
    PWM_V_HIGH = 0;
    PWM_V_LOW = 0;
    PWM_W_HIGH = 0;
    PWM_W_LOW = 0;
    
    // 设置死区时间
    pwm_set_dead_time(dead_time);
    
    // 初始化占空比
    current_duty_cycle = 0;
    phase_duty[0] = 0;
    phase_duty[1] = 0;
    phase_duty[2] = 0;
    
    // 禁用所有相
    pwm_disable_all();
    
    // 启用PWM中断
    ET1 = 1;  // 假设使用Timer1作为PWM时基
}

// 设置PWM占空比 (全局)
void pwm_set_duty_cycle(unsigned char duty) {
    if(duty > 100) duty = 100;
    
    current_duty_cycle = duty;
    
    // 更新所有相的占空比
    pwm_set_phase_duty(PWM_PHASE_U, duty);
    pwm_set_phase_duty(PWM_PHASE_V, duty);
    pwm_set_phase_duty(PWM_PHASE_W, duty);
}

// 设置单相PWM占空比
void pwm_set_phase_duty(pwm_phase_t phase, unsigned char duty) {
    if(phase >= 3) return;
    if(duty > 100) duty = 100;
    
    phase_duty[phase] = duty;
    
    // 计算实际的PWM比较值
    unsigned int compare_value = (unsigned int)((unsigned long)pwm_period * duty / 100);
    
    // 根据相位更新相应的PWM寄存器
    switch(phase) {
        case PWM_PHASE_U:
            // 更新U相PWM比较寄存器
            // PWM_U_COMPARE = compare_value;
            break;
        case PWM_PHASE_V:
            // 更新V相PWM比较寄存器
            // PWM_V_COMPARE = compare_value;
            break;
        case PWM_PHASE_W:
            // 更新W相PWM比较寄存器
            // PWM_W_COMPARE = compare_value;
            break;
    }
}

// 设置相位状态
void pwm_set_phase_state(pwm_phase_t phase, unsigned char high_state, unsigned char low_state) {
    if(phase >= 3) return;
    
    // 确保不会同时导通上下桥臂
    if(high_state && low_state) {
        high_state = 0;
        low_state = 0;
    }
    
    phase_high_state[phase] = high_state;
    phase_low_state[phase] = low_state;
    
    // 更新实际的PWM输出
    update_pwm_outputs();
}

// 更新PWM输出
void update_pwm_outputs(void) {
    // U相输出控制
    if(phase_high_state[PWM_PHASE_U] && pwm_enabled) {
        // U相上桥臂PWM输出
        // 这里需要根据实际PWM模块设置
    } else {
        PWM_U_HIGH = 0;
    }
    
    if(phase_low_state[PWM_PHASE_U] && pwm_enabled) {
        PWM_U_LOW = 1;
    } else {
        PWM_U_LOW = 0;
    }
    
    // V相输出控制
    if(phase_high_state[PWM_PHASE_V] && pwm_enabled) {
        // V相上桥臂PWM输出
    } else {
        PWM_V_HIGH = 0;
    }
    
    if(phase_low_state[PWM_PHASE_V] && pwm_enabled) {
        PWM_V_LOW = 1;
    } else {
        PWM_V_LOW = 0;
    }
    
    // W相输出控制
    if(phase_high_state[PWM_PHASE_W] && pwm_enabled) {
        // W相上桥臂PWM输出
    } else {
        PWM_W_HIGH = 0;
    }
    
    if(phase_low_state[PWM_PHASE_W] && pwm_enabled) {
        PWM_W_LOW = 1;
    } else {
        PWM_W_LOW = 0;
    }
}

// 启用单相PWM
void pwm_enable_phase(pwm_phase_t phase) {
    if(phase >= 3) return;
    
    // 这里可以添加单相启用逻辑
    update_pwm_outputs();
}

// 禁用单相PWM
void pwm_disable_phase(pwm_phase_t phase) {
    if(phase >= 3) return;
    
    phase_high_state[phase] = 0;
    phase_low_state[phase] = 0;
    
    switch(phase) {
        case PWM_PHASE_U:
            PWM_U_HIGH = 0;
            PWM_U_LOW = 0;
            break;
        case PWM_PHASE_V:
            PWM_V_HIGH = 0;
            PWM_V_LOW = 0;
            break;
        case PWM_PHASE_W:
            PWM_W_HIGH = 0;
            PWM_W_LOW = 0;
            break;
    }
}

// 启用所有PWM
void pwm_enable_all(void) {
    pwm_enabled = 1;
    update_pwm_outputs();
}

// 禁用所有PWM
void pwm_disable_all(void) {
    pwm_enabled = 0;
    
    // 关闭所有PWM输出
    PWM_U_HIGH = 0;
    PWM_U_LOW = 0;
    PWM_V_HIGH = 0;
    PWM_V_LOW = 0;
    PWM_W_HIGH = 0;
    PWM_W_LOW = 0;
}

// 设置PWM频率
void pwm_set_frequency(unsigned int frequency) {
    if(frequency < 1000) frequency = 1000;   // 最小1kHz
    if(frequency > 50000) frequency = 50000; // 最大50kHz
    
    pwm_period = SYSTEM_CLOCK / frequency;
    
    // 更新PWM周期寄存器
    // PWM_PERIOD_REG = pwm_period;
    
    // 重新计算所有相的占空比
    pwm_set_phase_duty(PWM_PHASE_U, phase_duty[0]);
    pwm_set_phase_duty(PWM_PHASE_V, phase_duty[1]);
    pwm_set_phase_duty(PWM_PHASE_W, phase_duty[2]);
}

// 紧急停止
void pwm_emergency_stop(void) {
    // 立即关闭所有PWM输出
    EA = 0;  // 关闭中断
    
    PWM_U_HIGH = 0;
    PWM_U_LOW = 0;
    PWM_V_HIGH = 0;
    PWM_V_LOW = 0;
    PWM_W_HIGH = 0;
    PWM_W_LOW = 0;
    
    pwm_enabled = 0;
    
    EA = 1;  // 重新开启中断
}

// 设置死区时间
void pwm_set_dead_time(unsigned char dead_time_us) {
    if(dead_time_us > 10) dead_time_us = 10;  // 最大10us
    dead_time = dead_time_us;
    
    // 这里需要根据实际PWM模块设置死区时间寄存器
    // PWM_DEAD_TIME_REG = dead_time;
}

// 获取当前占空比
unsigned char pwm_get_duty_cycle(void) {
    return current_duty_cycle;
}

// 获取单相占空比
unsigned char pwm_get_phase_duty(pwm_phase_t phase) {
    if(phase >= 3) return 0;
    return phase_duty[phase];
}

// 检查相位是否启用
unsigned char pwm_is_phase_enabled(pwm_phase_t phase) {
    if(phase >= 3) return 0;
    return (phase_high_state[phase] || phase_low_state[phase]) && pwm_enabled;
}

// PWM中断处理函数
void pwm_interrupt_handler(void) interrupt 3 {
    // PWM中断处理
    // 这里可以添加PWM周期中断处理逻辑
    
    // 清除中断标志
    TF1 = 0;
}