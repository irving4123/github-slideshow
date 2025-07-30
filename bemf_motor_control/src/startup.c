#include "../include/startup.h"
#include "../include/pwm_driver.h"
#include "../include/bemf_control.h"
#include "../include/adc_handler.h"

// 全局变量
volatile startup_state_t current_startup_state = STARTUP_IDLE;
volatile unsigned char startup_duty_cycle = 0;
volatile unsigned int startup_timer = 0;
volatile unsigned char startup_step = 0;

// 静态变量
static startup_params_t startup_params = {
    .initial_duty = STARTUP_DUTY,
    .final_duty = 40,
    .ramp_time_ms = STARTUP_RAMP_TIME,
    .align_time_ms = 50,
    .step_time_ms = 20,
    .transition_speed = BEMF_DETECT_SPEED
};

static unsigned int step_timer = 0;
static unsigned char ramp_step_count = 0;
static unsigned char alignment_step = 0;
static unsigned int startup_start_time = 0;

// 开环换相序列 (简化的六步换相)
static const unsigned char open_loop_sequence[6] = {0, 1, 2, 3, 4, 5};
static unsigned int step_period = 1000;  // 初始步进周期 (us)

// 启动初始化
void startup_init(void) {
    current_startup_state = STARTUP_ALIGN;
    startup_duty_cycle = startup_params.initial_duty;
    startup_timer = 0;
    startup_step = 0;
    step_timer = 0;
    ramp_step_count = 0;
    alignment_step = 0;
    
    // 记录启动开始时间
    startup_start_time = get_system_time_ms();
    
    // 初始化PWM
    pwm_set_duty_cycle(startup_duty_cycle);
    pwm_enable_all();
    
    // 禁用BEMF检测
    bemf_detection_enabled = 0;
    
    // 开始电机对齐
    motor_alignment_start();
}

// 启动主处理函数
unsigned char startup_process(void) {
    switch(current_startup_state) {
        case STARTUP_IDLE:
            return 0;  // 未启动
            
        case STARTUP_ALIGN:
            if(motor_alignment_process()) {
                // 对齐完成，进入斜坡加速
                current_startup_state = STARTUP_RAMP;
                ramp_control_init();
            }
            break;
            
        case STARTUP_RAMP:
            ramp_control_process();
            
            // 检查是否达到BEMF检测速度
            if(check_bemf_transition_ready()) {
                current_startup_state = STARTUP_TRANSITION;
            }
            break;
            
        case STARTUP_TRANSITION:
            if(perform_bemf_transition()) {
                current_startup_state = STARTUP_COMPLETE;
                return 1;  // 启动完成
            }
            break;
            
        case STARTUP_COMPLETE:
            return 1;  // 启动完成
    }
    
    // 启动保护检查
    if(startup_protection_check()) {
        startup_handle_fault();
        return 0;
    }
    
    startup_timer++;
    return 0;  // 启动未完成
}

// 电机对齐开始
void motor_alignment_start(void) {
    alignment_step = 0;
    step_timer = 0;
    
    // 设置初始对齐位置 (U+, V-, W浮空)
    set_commutation_step(0);
    pwm_set_duty_cycle(startup_params.initial_duty);
}

// 电机对齐处理
unsigned char motor_alignment_process(void) {
    step_timer++;
    
    // 对齐时间到达
    if(step_timer >= startup_params.align_time_ms) {
        return 1;  // 对齐完成
    }
    
    return 0;  // 对齐进行中
}

// 斜坡控制初始化
void ramp_control_init(void) {
    startup_step = 0;
    step_timer = 0;
    ramp_step_count = 0;
    step_period = 1000;  // 初始步进周期1ms
    
    // 计算斜坡参数
    unsigned int total_steps = startup_params.ramp_time_ms / startup_params.step_time_ms;
    
    // 开始开环控制
    open_loop_control_init();
}

// 斜坡控制处理
void ramp_control_process(void) {
    step_timer++;
    
    // 检查是否到达步进时间
    if(step_timer >= startup_params.step_time_ms) {
        step_timer = 0;
        ramp_step_count++;
        
        // 执行下一步换相
        startup_step = (startup_step + 1) % 6;
        set_commutation_step(startup_step);
        
        // 逐步增加占空比
        if(startup_duty_cycle < startup_params.final_duty) {
            startup_duty_cycle++;
            pwm_set_duty_cycle(startup_duty_cycle);
        }
        
        // 逐步减少步进周期 (加速)
        if(step_period > 100) {  // 最小100us
            step_period -= 10;
        }
        
        // 更新步进时间
        if(startup_params.step_time_ms > 5) {
            startup_params.step_time_ms--;
        }
    }
}

// 开环控制初始化
void open_loop_control_init(void) {
    startup_step = 0;
    step_period = 1000;  // 初始1ms
}

// 开环控制处理
void open_loop_control_process(void) {
    static unsigned int open_loop_timer = 0;
    
    open_loop_timer++;
    
    // 根据设定的步进周期执行换相
    if(open_loop_timer >= (step_period / 100)) {  // 转换为100us单位
        open_loop_timer = 0;
        
        // 执行换相
        startup_step = (startup_step + 1) % 6;
        set_commutation_step(startup_step);
    }
}

// 检查BEMF切换准备就绪
unsigned char check_bemf_transition_ready(void) {
    // 检查运行时间
    unsigned int current_time = get_system_time_ms();
    if((current_time - startup_start_time) < startup_params.ramp_time_ms) {
        return 0;
    }
    
    // 检查BEMF信号质量
    unsigned int bemf_u = adc_read_bemf_u();
    unsigned int bemf_v = adc_read_bemf_v();
    unsigned int bemf_w = adc_read_bemf_w();
    unsigned int neutral = adc_get_bemf_neutral_point();
    
    // 检查BEMF信号幅度是否足够
    unsigned int bemf_amplitude = 0;
    if(bemf_u > neutral) bemf_amplitude = bemf_u - neutral;
    else bemf_amplitude = neutral - bemf_u;
    
    // BEMF幅度应该大于阈值的一定比例
    if(bemf_amplitude > (BEMF_THRESHOLD / 2)) {
        return 1;  // 可以切换到BEMF控制
    }
    
    return 0;
}

// 执行BEMF切换
unsigned char perform_bemf_transition(void) {
    static unsigned char transition_step = 0;
    static unsigned int transition_timer = 0;
    
    transition_timer++;
    
    switch(transition_step) {
        case 0:
            // 第一步：启用BEMF检测
            bemf_detection_enabled = 1;
            transition_step = 1;
            transition_timer = 0;
            break;
            
        case 1:
            // 第二步：等待BEMF同步
            if(transition_timer > 50) {  // 等待50ms
                transition_step = 2;
                transition_timer = 0;
            }
            break;
            
        case 2:
            // 第三步：逐步减少开环控制，增加BEMF控制
            if(transition_timer > 10) {  // 每10ms调整一次
                transition_timer = 0;
                
                // 这里可以实现渐进式切换
                // 例如：减少开环控制的权重，增加BEMF控制的权重
                
                // 简化处理：直接切换到BEMF控制
                transition_step = 3;
            }
            break;
            
        case 3:
            // 切换完成
            return 1;
    }
    
    return 0;  // 切换未完成
}

// 启动保护检查
unsigned char startup_protection_check(void) {
    // 检查启动超时
    unsigned int current_time = get_system_time_ms();
    if((current_time - startup_start_time) > (startup_params.ramp_time_ms * 3)) {
        return 1;  // 启动超时
    }
    
    // 检查过流
    unsigned int current_ma = adc_read_current_ma();
    if(current_ma > MAX_CURRENT) {
        return 1;  // 过流
    }
    
    // 检查过压
    unsigned int voltage_mv = adc_read_voltage_mv();
    if(voltage_mv > MAX_VOLTAGE) {
        return 1;  // 过压
    }
    
    return 0;  // 无故障
}

// 启动故障处理
void startup_handle_fault(void) {
    // 停止PWM输出
    pwm_emergency_stop();
    
    // 重置启动状态
    current_startup_state = STARTUP_IDLE;
    
    // 设置故障状态
    fault_status = FAULT_STALL;
}

// 停止启动
void startup_stop(void) {
    current_startup_state = STARTUP_IDLE;
    pwm_disable_all();
    bemf_detection_enabled = 0;
}

// 重置启动
void startup_reset(void) {
    startup_stop();
    startup_timer = 0;
    startup_step = 0;
    startup_duty_cycle = startup_params.initial_duty;
}

// 设置启动参数
void startup_set_params(const startup_params_t* params) {
    startup_params = *params;
}

// 获取启动参数
void startup_get_params(startup_params_t* params) {
    *params = startup_params;
}

// 设置初始占空比
void startup_set_initial_duty(unsigned char duty) {
    if(duty <= MAX_DUTY_CYCLE) {
        startup_params.initial_duty = duty;
    }
}

// 设置斜坡时间
void startup_set_ramp_time(unsigned int time_ms) {
    startup_params.ramp_time_ms = time_ms;
}

// 获取启动状态
startup_state_t startup_get_state(void) {
    return current_startup_state;
}

// 检查启动是否完成
unsigned char startup_is_complete(void) {
    return (current_startup_state == STARTUP_COMPLETE);
}

// 获取启动进度
unsigned char startup_get_progress(void) {
    switch(current_startup_state) {
        case STARTUP_IDLE:
            return 0;
        case STARTUP_ALIGN:
            return 10;
        case STARTUP_RAMP:
            return 10 + (ramp_step_count * 70 / (startup_params.ramp_time_ms / startup_params.step_time_ms));
        case STARTUP_TRANSITION:
            return 85;
        case STARTUP_COMPLETE:
            return 100;
    }
    return 0;
}

// 获取系统时间 (毫秒)
unsigned int get_system_time_ms(void) {
    // 这里需要根据实际的定时器实现
    // 简化实现
    static unsigned int ms_counter = 0;
    return ms_counter++;
}