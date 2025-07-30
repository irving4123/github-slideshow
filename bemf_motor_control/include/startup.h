#ifndef STARTUP_H
#define STARTUP_H

#include "bemf_config.h"

// 启动状态定义
typedef enum {
    STARTUP_IDLE = 0,
    STARTUP_ALIGN,
    STARTUP_RAMP,
    STARTUP_TRANSITION,
    STARTUP_COMPLETE
} startup_state_t;

// 启动参数结构
typedef struct {
    unsigned char initial_duty;         // 初始占空比
    unsigned char final_duty;           // 最终占空比
    unsigned int ramp_time_ms;          // 斜坡时间
    unsigned int align_time_ms;         // 对齐时间
    unsigned int step_time_ms;          // 每步时间
    unsigned char transition_speed;     // 切换到BEMF控制的速度阈值
} startup_params_t;

// 启动控制函数
void startup_init(void);
unsigned char startup_process(void);
void startup_stop(void);
void startup_reset(void);

// 启动参数配置
void startup_set_params(const startup_params_t* params);
void startup_get_params(startup_params_t* params);
void startup_set_initial_duty(unsigned char duty);
void startup_set_ramp_time(unsigned int time_ms);

// 启动状态查询
startup_state_t startup_get_state(void);
unsigned char startup_is_complete(void);
unsigned char startup_get_progress(void);  // 返回0-100的进度百分比

// 开环控制函数
void open_loop_control_init(void);
void open_loop_control_process(void);
void open_loop_set_speed(unsigned int rpm);
void open_loop_set_duty(unsigned char duty);

// 电机对齐函数
void motor_alignment_start(void);
unsigned char motor_alignment_process(void);
void motor_alignment_stop(void);

// 斜坡控制函数
void ramp_control_init(void);
void ramp_control_process(void);
void ramp_set_target(unsigned char target_duty, unsigned int ramp_time);

// 启动到BEMF切换
unsigned char check_bemf_transition_ready(void);
void perform_bemf_transition(void);

// 启动保护功能
unsigned char startup_protection_check(void);
void startup_handle_fault(void);

// 全局变量声明
extern volatile startup_state_t current_startup_state;
extern volatile unsigned char startup_duty_cycle;
extern volatile unsigned int startup_timer;
extern volatile unsigned char startup_step;

#endif // STARTUP_H