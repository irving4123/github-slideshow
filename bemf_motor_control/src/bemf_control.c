#include "../include/bemf_control.h"
#include "../include/adc_handler.h"
#include "../include/pwm_driver.h"

// 全局变量
volatile unsigned char current_commutation_step = 0;
volatile unsigned int commutation_period = 0;
volatile unsigned char bemf_detection_enabled = 0;
volatile unsigned int speed_rpm = 0;

// 换相序列表 (六步换相)
const commutation_step_t commutation_table[6] = {
    // Step 0: U+, W-
    {1, 0, 0, 0, 0, 1},
    // Step 1: U+, V-  
    {1, 0, 0, 1, 0, 0},
    // Step 2: V+, W-
    {0, 0, 1, 0, 0, 1},
    // Step 3: V+, U-
    {0, 1, 1, 0, 0, 0},
    // Step 4: W+, U-
    {0, 1, 0, 0, 1, 0},
    // Step 5: W+, V-
    {0, 0, 0, 1, 1, 0}
};

// BEMF检测相位表
const unsigned char bemf_phase_table[6] = {
    ADC_BEMF_V,  // Step 0: 检测V相BEMF
    ADC_BEMF_W,  // Step 1: 检测W相BEMF
    ADC_BEMF_U,  // Step 2: 检测U相BEMF
    ADC_BEMF_W,  // Step 3: 检测W相BEMF
    ADC_BEMF_V,  // Step 4: 检测V相BEMF
    ADC_BEMF_U   // Step 5: 检测U相BEMF
};

// BEMF过零检测期望极性 (1为正向过零, 0为负向过零)
const unsigned char bemf_polarity_table[6] = {1, 0, 1, 0, 1, 0};

// 静态变量
static unsigned int bemf_filter[BEMF_FILTER_COUNT];
static unsigned char bemf_filter_index = 0;
static unsigned long last_commutation_time = 0;
static unsigned int commutation_delay_count = 0;

// BEMF控制初始化
void bemf_control_init(void) {
    current_commutation_step = 0;
    bemf_detection_enabled = 1;
    commutation_period = 0;
    
    // 初始化BEMF滤波器
    unsigned char i;
    for(i = 0; i < BEMF_FILTER_COUNT; i++) {
        bemf_filter[i] = BEMF_THRESHOLD;
    }
    bemf_filter_index = 0;
    
    // 设置初始换相步骤
    set_commutation_step(0);
    
    // 初始化速度测量
    speed_measurement_init();
}

// BEMF控制主处理函数
void bemf_control_process(void) {
    if(!bemf_detection_enabled) return;
    
    // 检测BEMF过零点
    if(detect_bemf_zero_crossing()) {
        // 等待换相延迟
        if(commutation_delay_count > 0) {
            commutation_delay_count--;
            return;
        }
        
        // 执行换相
        next_commutation_step();
        
        // 更新换相时序
        update_commutation_timing();
        
        // 设置下次换相延迟
        commutation_delay_count = COMMUTATION_DELAY;
    }
}

// 检测BEMF过零点
unsigned char detect_bemf_zero_crossing(void) {
    unsigned char current_phase = bemf_phase_table[current_commutation_step];
    unsigned char expected_polarity = bemf_polarity_table[current_commutation_step];
    
    // 读取当前相的BEMF电压
    unsigned int bemf_voltage = read_bemf_voltage(current_phase);
    
    // BEMF滤波处理
    bemf_filter[bemf_filter_index] = bemf_voltage;
    bemf_filter_index = (bemf_filter_index + 1) % BEMF_FILTER_COUNT;
    
    // 计算滤波后的BEMF值
    unsigned int filtered_bemf = 0;
    unsigned char i;
    for(i = 0; i < BEMF_FILTER_COUNT; i++) {
        filtered_bemf += bemf_filter[i];
    }
    filtered_bemf /= BEMF_FILTER_COUNT;
    
    // 检测过零点
    if(expected_polarity) {
        // 正向过零检测
        return (filtered_bemf > BEMF_THRESHOLD);
    } else {
        // 负向过零检测
        return (filtered_bemf < BEMF_THRESHOLD);
    }
}

// 读取BEMF电压
unsigned int read_bemf_voltage(unsigned char phase) {
    return adc_read_channel(phase);
}

// 设置换相步骤
void set_commutation_step(unsigned char step) {
    if(step >= 6) step = 0;
    
    current_commutation_step = step;
    
    // 应用换相序列
    const commutation_step_t* comm_step = &commutation_table[step];
    
    // 更新PWM输出
    pwm_set_phase_state(PWM_PHASE_U, comm_step->u_high, comm_step->u_low);
    pwm_set_phase_state(PWM_PHASE_V, comm_step->v_high, comm_step->v_low);
    pwm_set_phase_state(PWM_PHASE_W, comm_step->w_high, comm_step->w_low);
}

// 下一个换相步骤
void next_commutation_step(void) {
    unsigned char next_step = (current_commutation_step + 1) % 6;
    set_commutation_step(next_step);
}

// 获取当前换相步骤
unsigned char get_current_step(void) {
    return current_commutation_step;
}

// 更新换相时序
void update_commutation_timing(void) {
    unsigned long current_time = get_system_time_us();
    
    if(last_commutation_time != 0) {
        commutation_period = current_time - last_commutation_time;
        
        // 更新速度测量
        update_speed_measurement();
    }
    
    last_commutation_time = current_time;
}

// 速度测量初始化
void speed_measurement_init(void) {
    speed_rpm = 0;
    last_commutation_time = 0;
    commutation_period = 0;
}

// 更新速度测量
void update_speed_measurement(void) {
    if(commutation_period > 0) {
        // 计算RPM: 60 * 1000000 / (commutation_period * 6 * pole_pairs)
        // 简化计算避免溢出
        unsigned long temp = 10000000UL / (commutation_period * MOTOR_POLE_PAIRS);
        speed_rpm = (unsigned int)temp;
    }
}

// 计算当前转速
unsigned int calculate_speed(void) {
    return speed_rpm;
}

// 获取系统时间 (微秒)
unsigned long get_system_time_us(void) {
    // 这里需要根据实际的定时器实现
    // 假设使用Timer0作为系统时钟
    static unsigned long system_time = 0;
    static unsigned char last_timer_value = 0;
    
    unsigned char current_timer = TH0;
    if(current_timer < last_timer_value) {
        // 定时器溢出
        system_time += 256;
    }
    system_time += (current_timer - last_timer_value);
    last_timer_value = current_timer;
    
    // 转换为微秒 (假设定时器频率为1MHz)
    return system_time;
}