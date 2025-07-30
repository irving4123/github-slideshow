#include "pwm_driver.h"
#include "uart_driver.h"
#include <string.h>

// ============================================================================
// 全局变量
// ============================================================================

// PWM配置结构体
static pwm_config_t pwm_config;

// PWM状态结构体
static pwm_status_t pwm_status;

// PWM寄存器模拟 (实际应用中需要根据硬件寄存器定义)
static uint16_t pwm_registers[6] = {0};

// ============================================================================
// PWM驱动初始化
// ============================================================================

void pwm_driver_init(void)
{
    // 初始化PWM配置
    pwm_config.frequency = PWM_FREQ;
    pwm_config.period = 65535; // 16位PWM
    pwm_config.mode = PWM_MODE_COMPLEMENTARY;
    pwm_config.dead_time.dead_time_rising = 100;
    pwm_config.dead_time.dead_time_falling = 100;
    pwm_config.dead_time.dead_time_enable = 1;
    pwm_config.enable_mask = 0x3F; // 使能所有6个通道
    
    // 初始化PWM状态
    pwm_status.is_initialized = 1;
    pwm_status.is_enabled = 0;
    memset(pwm_status.channel_status, 0, sizeof(pwm_status.channel_status));
    pwm_status.update_count = 0;
    pwm_status.fault_count = 0;
    
    // 初始化PWM寄存器
    memset(pwm_registers, 0, sizeof(pwm_registers));
    
    // 配置PWM寄存器 (这里需要根据实际MCU寄存器进行配置)
    // 示例代码，需要根据实际硬件调整
    /*
    PWM_CON = 0x80;  // 使能PWM
    PWM_FREQ = pwm_config.frequency;
    PWM_PERIOD = pwm_config.period;
    PWM_DEAD_TIME = pwm_config.dead_time.dead_time_rising;
    */
    
    uart_send_string("PWM driver initialized\r\n");
}

// ============================================================================
// PWM配置设置
// ============================================================================

void pwm_set_config(pwm_config_t* config)
{
    if (config != NULL) {
        memcpy(&pwm_config, config, sizeof(pwm_config_t));
    }
}

// ============================================================================
// PWM使能/禁用
// ============================================================================

void pwm_enable(uint8_t enable)
{
    if (enable) {
        // 使能PWM
        // PWM_CON |= 0x80;
        pwm_status.is_enabled = 1;
        uart_send_string("PWM enabled\r\n");
    } else {
        // 禁用PWM
        // PWM_CON &= ~0x80;
        pwm_status.is_enabled = 0;
        uart_send_string("PWM disabled\r\n");
    }
}

// ============================================================================
// PWM通道使能/禁用
// ============================================================================

void pwm_channel_enable(pwm_channel_t channel, uint8_t enable)
{
    if (channel < 6) {
        if (enable) {
            pwm_config.enable_mask |= (1 << channel);
            pwm_status.channel_status[channel] = 1;
        } else {
            pwm_config.enable_mask &= ~(1 << channel);
            pwm_status.channel_status[channel] = 0;
        }
    }
}

// ============================================================================
// 设置PWM频率
// ============================================================================

void pwm_set_frequency(uint32_t frequency)
{
    pwm_config.frequency = frequency;
    // 更新硬件寄存器
    // PWM_FREQ = frequency;
}

// ============================================================================
// 设置PWM周期
// ============================================================================

void pwm_set_period(uint16_t period)
{
    pwm_config.period = period;
    // 更新硬件寄存器
    // PWM_PERIOD = period;
}

// ============================================================================
// 设置PWM占空比
// ============================================================================

void pwm_set_duty_cycle(pwm_channel_t channel, uint16_t duty_cycle)
{
    if (channel < 6) {
        pwm_config.duty_cycle[channel] = duty_cycle;
        pwm_registers[channel] = duty_cycle;
        
        // 更新硬件寄存器
        // PWM_DUTY[channel] = duty_cycle;
    }
}

// ============================================================================
// 设置三相PWM占空比
// ============================================================================

void pwm_set_three_phase_duty(uint16_t duty_a, uint16_t duty_b, uint16_t duty_c)
{
    pwm_set_duty_cycle(PWM_CHANNEL_A, duty_a);
    pwm_set_duty_cycle(PWM_CHANNEL_B, duty_b);
    pwm_set_duty_cycle(PWM_CHANNEL_C, duty_c);
    
    // 设置互补通道
    pwm_set_duty_cycle(PWM_CHANNEL_A_LOW, 65535 - duty_a);
    pwm_set_duty_cycle(PWM_CHANNEL_B_LOW, 65535 - duty_b);
    pwm_set_duty_cycle(PWM_CHANNEL_C_LOW, 65535 - duty_c);
}

// ============================================================================
// 设置PWM模式
// ============================================================================

void pwm_set_mode(pwm_mode_t mode)
{
    pwm_config.mode = mode;
    // 更新硬件寄存器
    // PWM_MODE = mode;
}

// ============================================================================
// 设置死区时间
// ============================================================================

void pwm_set_dead_time(pwm_dead_time_config_t* dead_time)
{
    if (dead_time != NULL) {
        memcpy(&pwm_config.dead_time, dead_time, sizeof(pwm_dead_time_config_t));
        // 更新硬件寄存器
        // PWM_DEAD_TIME_RISING = dead_time->dead_time_rising;
        // PWM_DEAD_TIME_FALLING = dead_time->dead_time_falling;
    }
}

// ============================================================================
// 设置PWM同步
// ============================================================================

void pwm_set_sync(uint8_t enable)
{
    if (enable) {
        // 使能PWM同步
        // PWM_SYNC |= 0x01;
    } else {
        // 禁用PWM同步
        // PWM_SYNC &= ~0x01;
    }
}

// ============================================================================
// PWM更新
// ============================================================================

void pwm_update(void)
{
    if (pwm_status.is_enabled) {
        // 更新PWM寄存器
        for (uint8_t i = 0; i < 6; i++) {
            if (pwm_status.channel_status[i]) {
                // 更新硬件寄存器
                // PWM_DUTY[i] = pwm_registers[i];
            }
        }
        
        pwm_status.update_count++;
    }
}

// ============================================================================
// PWM紧急停止
// ============================================================================

void pwm_emergency_stop(void)
{
    // 立即停止所有PWM输出
    for (uint8_t i = 0; i < 6; i++) {
        pwm_registers[i] = 0;
        // PWM_DUTY[i] = 0;
    }
    
    // 禁用PWM
    pwm_enable(0);
    
    uart_send_string("PWM emergency stop\r\n");
}

// ============================================================================
// PWM故障处理
// ============================================================================

void pwm_fault_handler(void)
{
    // 处理PWM故障
    pwm_status.fault_count++;
    
    // 紧急停止
    pwm_emergency_stop();
    
    uart_send_string("PWM fault detected\r\n");
}

// ============================================================================
// PWM自检
// ============================================================================

uint8_t pwm_self_test(void)
{
    if (!pwm_status.is_initialized) {
        return 0;
    }
    
    // 简单的自检：检查配置是否有效
    if (pwm_config.frequency == 0 || pwm_config.period == 0) {
        return 0;
    }
    
    return 1; // 自检通过
}

// ============================================================================
// PWM校准
// ============================================================================

void pwm_calibrate(void)
{
    uart_send_string("PWM calibration started\r\n");
    
    // 设置测试占空比
    for (uint8_t i = 0; i < 6; i++) {
        pwm_set_duty_cycle(i, 32768); // 50%占空比
    }
    
    // 更新PWM
    pwm_update();
    
    uart_send_string("PWM calibration completed\r\n");
}

// ============================================================================
// 获取函数
// ============================================================================

pwm_config_t* pwm_get_config(void)
{
    return &pwm_config;
}

pwm_status_t* pwm_get_status(void)
{
    return &pwm_status;
}

// ============================================================================
// 调试函数
// ============================================================================

void pwm_debug_print(void)
{
    uart_send_string("PWM Debug Info:\r\n");
    uart_send_string("Initialized: ");
    uart_send_int(pwm_status.is_initialized);
    uart_send_string("\r\n");
    
    uart_send_string("Enabled: ");
    uart_send_int(pwm_status.is_enabled);
    uart_send_string("\r\n");
    
    uart_send_string("Frequency: ");
    uart_send_int(pwm_config.frequency);
    uart_send_string(" Hz\r\n");
    
    uart_send_string("Period: ");
    uart_send_int(pwm_config.period);
    uart_send_string("\r\n");
    
    uart_send_string("Update Count: ");
    uart_send_int(pwm_status.update_count);
    uart_send_string("\r\n");
    
    uart_send_string("Fault Count: ");
    uart_send_int(pwm_status.fault_count);
    uart_send_string("\r\n");
    
    // 显示各通道占空比
    for (uint8_t i = 0; i < 6; i++) {
        uart_send_string("Channel ");
        uart_send_int(i);
        uart_send_string(" Duty: ");
        uart_send_int(pwm_config.duty_cycle[i]);
        uart_send_string("\r\n");
    }
}

void pwm_parameter_display(void)
{
    uart_send_string("PWM Parameters:\r\n");
    uart_send_string("Frequency: ");
    uart_send_int(pwm_config.frequency);
    uart_send_string(" Hz\r\n");
    
    uart_send_string("Period: ");
    uart_send_int(pwm_config.period);
    uart_send_string("\r\n");
    
    uart_send_string("Mode: ");
    uart_send_int(pwm_config.mode);
    uart_send_string("\r\n");
    
    uart_send_string("Dead Time Rising: ");
    uart_send_int(pwm_config.dead_time.dead_time_rising);
    uart_send_string("\r\n");
    
    uart_send_string("Dead Time Falling: ");
    uart_send_int(pwm_config.dead_time.dead_time_falling);
    uart_send_string("\r\n");
    
    uart_send_string("Enable Mask: 0x");
    uart_send_hex(pwm_config.enable_mask);
    uart_send_string("\r\n");
}