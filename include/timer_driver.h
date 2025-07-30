#ifndef TIMER_DRIVER_H
#define TIMER_DRIVER_H

#include "config.h"

// ============================================================================
// 定时器配置定义
// ============================================================================

// 定时器编号定义
typedef enum {
    TIMER_0 = 0,            // 定时器0
    TIMER_1,                // 定时器1
    TIMER_2,                // 定时器2
    TIMER_3                 // 定时器3
} timer_id_t;

// 定时器模式定义
typedef enum {
    TIMER_MODE_16BIT = 0,   // 16位模式
    TIMER_MODE_13BIT,       // 13位模式
    TIMER_MODE_8BIT,        // 8位模式
    TIMER_MODE_AUTO_RELOAD  // 自动重载模式
} timer_mode_t;

// 定时器配置结构体
typedef struct {
    timer_mode_t mode;       // 定时器模式
    uint16_t reload_value;   // 重载值
    uint16_t compare_value;  // 比较值
    uint8_t prescaler;      // 预分频器
    uint8_t interrupt_enable; // 中断使能
} timer_config_t;

// 定时器状态结构体
typedef struct {
    uint8_t is_initialized;  // 初始化状态
    uint8_t is_enabled;      // 使能状态
    uint8_t is_running;      // 运行状态
    uint32_t overflow_count; // 溢出计数
    uint32_t interrupt_count; // 中断计数
} timer_status_t;

// ============================================================================
// 函数声明
// ============================================================================

// 定时器驱动初始化
void timer_driver_init(void);

// 定时器配置设置
void timer_set_config(timer_id_t timer, timer_config_t* config);

// 定时器使能/禁用
void timer_enable(timer_id_t timer, uint8_t enable);

// 定时器启动/停止
void timer_start(timer_id_t timer);
void timer_stop(timer_id_t timer);

// 定时器设置重载值
void timer_set_reload(timer_id_t timer, uint16_t reload_value);

// 定时器设置比较值
void timer_set_compare(timer_id_t timer, uint16_t compare_value);

// 定时器设置预分频器
void timer_set_prescaler(timer_id_t timer, uint8_t prescaler);

// 定时器中断使能/禁用
void timer_interrupt_enable(timer_id_t timer, uint8_t enable);

// 定时器获取计数值
uint16_t timer_get_count(timer_id_t timer);

// 定时器设置计数值
void timer_set_count(timer_id_t timer, uint16_t count);

// 定时器清除溢出标志
void timer_clear_overflow(timer_id_t timer);

// 定时器清除中断标志
void timer_clear_interrupt(timer_id_t timer);

// 定时器检查溢出标志
uint8_t timer_is_overflow(timer_id_t timer);

// 定时器检查中断标志
uint8_t timer_is_interrupt(timer_id_t timer);

// 定时器延时函数
void timer_delay_ms(uint16_t ms);
void timer_delay_us(uint16_t us);

// 定时器获取系统时间
uint32_t timer_get_system_time(void);

// 定时器中断处理
void timer_interrupt_handler(timer_id_t timer);

// 定时器自检
uint8_t timer_self_test(timer_id_t timer);

// 获取定时器配置
timer_config_t* timer_get_config(timer_id_t timer);

// 获取定时器状态
timer_status_t* timer_get_status(timer_id_t timer);

// 定时器调试
void timer_debug_print(void);

// 定时器参数显示
void timer_parameter_display(void);

// ============================================================================
// 内联函数
// ============================================================================

// 检查定时器是否初始化
static inline uint8_t timer_is_initialized(timer_id_t timer) {
    timer_status_t* status = timer_get_status(timer);
    return status->is_initialized;
}

// 检查定时器是否使能
static inline uint8_t timer_is_enabled(timer_id_t timer) {
    timer_status_t* status = timer_get_status(timer);
    return status->is_enabled;
}

// 检查定时器是否运行
static inline uint8_t timer_is_running(timer_id_t timer) {
    timer_status_t* status = timer_get_status(timer);
    return status->is_running;
}

// 定时器设置频率 (Hz)
static inline void timer_set_frequency(timer_id_t timer, uint32_t frequency) {
    uint32_t reload_value = SYSTEM_CLOCK_FREQ / (TIMER_CLOCK_DIV * frequency) - 1;
    timer_set_reload(timer, (uint16_t)reload_value);
}

// 定时器设置周期 (ms)
static inline void timer_set_period_ms(timer_id_t timer, uint16_t period_ms) {
    uint32_t frequency = 1000 / period_ms;
    timer_set_frequency(timer, frequency);
}

// 定时器设置周期 (us)
static inline void timer_set_period_us(timer_id_t timer, uint16_t period_us) {
    uint32_t frequency = 1000000 / period_us;
    timer_set_frequency(timer, frequency);
}

#endif // TIMER_DRIVER_H