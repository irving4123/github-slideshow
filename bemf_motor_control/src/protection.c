#include "../include/protection.h"
#include "../include/adc_handler.h"
#include "../include/pwm_driver.h"
#include "../include/bemf_control.h"

// 全局变量
volatile protection_type_t active_protection_fault = PROTECTION_NONE;
volatile unsigned char protection_fault_flags = 0;
volatile unsigned int protection_fault_counters[8] = {0};

// 静态变量
static protection_config_t protection_config = {
    .overcurrent_enabled = 1,
    .overvoltage_enabled = 1,
    .undervoltage_enabled = 1,
    .overtemp_enabled = 1,
    .stall_enabled = 1,
    .overspeed_enabled = 1
};

static protection_thresholds_t protection_thresholds = {
    .max_current_ma = MAX_CURRENT,
    .max_voltage_mv = MAX_VOLTAGE,
    .min_voltage_mv = 8000,  // 8V最小电压
    .max_temperature_c = MAX_TEMPERATURE,
    .max_speed_rpm = 5000,   // 最大5000RPM
    .stall_timeout_ms = STALL_TIMEOUT,
    .protection_delay_ms = 100  // 100ms保护延迟
};

static protection_callback_t protection_callback = 0;
static unsigned int protection_timers[8] = {0};
static unsigned int last_speed_check_time = 0;
static unsigned int stall_start_time = 0;
static unsigned char protection_delay_active = 0;

// 保护初始化
void protection_init(void) {
    // 清除所有故障标志
    protection_fault_flags = 0;
    active_protection_fault = PROTECTION_NONE;
    
    // 重置故障计数器
    unsigned char i;
    for(i = 0; i < 8; i++) {
        protection_fault_counters[i] = 0;
        protection_timers[i] = 0;
    }
    
    // 初始化保护定时器
    last_speed_check_time = get_system_time_ms();
    stall_start_time = 0;
    protection_delay_active = 0;
    
    // 配置比较器用于硬件保护
    comparator_set_threshold(COMP_CURRENT, protection_thresholds.max_current_ma * 100 / 1000);
}

// 主保护检查函数
unsigned char protection_check(void) {
    unsigned char fault_detected = 0;
    
    // 如果保护延迟激活，等待延迟结束
    if(protection_delay_active) {
        static unsigned int delay_start_time = 0;
        if(delay_start_time == 0) {
            delay_start_time = get_system_time_ms();
        }
        
        if((get_system_time_ms() - delay_start_time) < protection_thresholds.protection_delay_ms) {
            return 0;  // 延迟期间不检查保护
        } else {
            protection_delay_active = 0;
            delay_start_time = 0;
        }
    }
    
    // 过流保护检查
    if(protection_config.overcurrent_enabled && check_overcurrent_protection()) {
        protection_trigger_fault(PROTECTION_OVERCURRENT);
        fault_detected = 1;
    }
    
    // 过压保护检查
    if(protection_config.overvoltage_enabled && check_overvoltage_protection()) {
        protection_trigger_fault(PROTECTION_OVERVOLTAGE);
        fault_detected = 1;
    }
    
    // 欠压保护检查
    if(protection_config.undervoltage_enabled && check_undervoltage_protection()) {
        protection_trigger_fault(PROTECTION_UNDERVOLTAGE);
        fault_detected = 1;
    }
    
    // 过温保护检查
    if(protection_config.overtemp_enabled && check_overtemp_protection()) {
        protection_trigger_fault(PROTECTION_OVERTEMP);
        fault_detected = 1;
    }
    
    // 堵转保护检查
    if(protection_config.stall_enabled && check_stall_protection()) {
        protection_trigger_fault(PROTECTION_STALL);
        fault_detected = 1;
    }
    
    // 超速保护检查
    if(protection_config.overspeed_enabled && check_overspeed_protection()) {
        protection_trigger_fault(PROTECTION_OVERSPEED);
        fault_detected = 1;
    }
    
    return fault_detected;
}

// 过流保护检查
unsigned char check_overcurrent_protection(void) {
    static unsigned int overcurrent_count = 0;
    
    unsigned int current_ma = adc_read_current_ma();
    
    if(current_ma > protection_thresholds.max_current_ma) {
        overcurrent_count++;
        
        // 连续3次检测到过流才触发保护
        if(overcurrent_count >= 3) {
            overcurrent_count = 0;
            return 1;
        }
    } else {
        overcurrent_count = 0;
    }
    
    return 0;
}

// 过压保护检查
unsigned char check_overvoltage_protection(void) {
    static unsigned int overvoltage_count = 0;
    
    unsigned int voltage_mv = adc_read_voltage_mv();
    
    if(voltage_mv > protection_thresholds.max_voltage_mv) {
        overvoltage_count++;
        
        // 连续5次检测到过压才触发保护
        if(overvoltage_count >= 5) {
            overvoltage_count = 0;
            return 1;
        }
    } else {
        overvoltage_count = 0;
    }
    
    return 0;
}

// 欠压保护检查
unsigned char check_undervoltage_protection(void) {
    static unsigned int undervoltage_count = 0;
    
    unsigned int voltage_mv = adc_read_voltage_mv();
    
    if(voltage_mv < protection_thresholds.min_voltage_mv) {
        undervoltage_count++;
        
        // 连续10次检测到欠压才触发保护
        if(undervoltage_count >= 10) {
            undervoltage_count = 0;
            return 1;
        }
    } else {
        undervoltage_count = 0;
    }
    
    return 0;
}

// 过温保护检查
unsigned char check_overtemp_protection(void) {
    static unsigned int overtemp_count = 0;
    
    unsigned int temperature_c = adc_read_temperature_c();
    
    if(temperature_c > protection_thresholds.max_temperature_c) {
        overtemp_count++;
        
        // 连续5次检测到过温才触发保护
        if(overtemp_count >= 5) {
            overtemp_count = 0;
            return 1;
        }
    } else {
        overtemp_count = 0;
    }
    
    return 0;
}

// 堵转保护检查
unsigned char check_stall_protection(void) {
    unsigned int current_time = get_system_time_ms();
    unsigned int current_speed = calculate_speed();
    
    // 检查速度是否过低
    if(current_speed < 100 && pwm_get_duty_cycle() > 20) {  // 速度低于100RPM且占空比大于20%
        if(stall_start_time == 0) {
            stall_start_time = current_time;
        } else if((current_time - stall_start_time) > protection_thresholds.stall_timeout_ms) {
            stall_start_time = 0;
            return 1;  // 堵转超时
        }
    } else {
        stall_start_time = 0;  // 重置堵转计时器
    }
    
    return 0;
}

// 超速保护检查
unsigned char check_overspeed_protection(void) {
    unsigned int current_speed = calculate_speed();
    
    if(current_speed > protection_thresholds.max_speed_rpm) {
        return 1;
    }
    
    return 0;
}

// 触发保护故障
void protection_trigger_fault(protection_type_t type) {
    if(type >= 8) return;
    
    // 设置故障标志
    protection_fault_flags |= (1 << type);
    active_protection_fault = type;
    
    // 增加故障计数器
    protection_fault_counters[type]++;
    
    // 根据故障类型采取相应动作
    switch(type) {
        case PROTECTION_OVERCURRENT:
            protection_emergency_stop();
            break;
            
        case PROTECTION_OVERVOLTAGE:
            protection_soft_stop();
            break;
            
        case PROTECTION_UNDERVOLTAGE:
            protection_soft_stop();
            break;
            
        case PROTECTION_OVERTEMP:
            protection_reduce_power();
            break;
            
        case PROTECTION_STALL:
            protection_emergency_stop();
            break;
            
        case PROTECTION_OVERSPEED:
            protection_reduce_power();
            break;
            
        default:
            protection_soft_stop();
            break;
    }
    
    // 调用回调函数
    if(protection_callback) {
        protection_callback(type);
    }
}

// 紧急停止
void protection_emergency_stop(void) {
    pwm_emergency_stop();
    motor_state = MOTOR_FAULT;
}

// 软停止
void protection_soft_stop(void) {
    // 逐步减少占空比
    unsigned char current_duty = pwm_get_duty_cycle();
    if(current_duty > 0) {
        pwm_set_duty_cycle(current_duty / 2);
    } else {
        pwm_disable_all();
        motor_state = MOTOR_FAULT;
    }
}

// 功率限制
void protection_reduce_power(void) {
    unsigned char current_duty = pwm_get_duty_cycle();
    
    // 将占空比限制到50%
    if(current_duty > 50) {
        pwm_set_duty_cycle(50);
    }
}

// 清除故障
void protection_clear_fault(protection_type_t type) {
    if(type >= 8) return;
    
    protection_fault_flags &= ~(1 << type);
    
    if(active_protection_fault == type) {
        active_protection_fault = PROTECTION_NONE;
    }
}

// 清除所有故障
void protection_clear_all_faults(void) {
    protection_fault_flags = 0;
    active_protection_fault = PROTECTION_NONE;
}

// 检查是否有故障
unsigned char protection_has_fault(void) {
    return (protection_fault_flags != 0);
}

// 检查特定故障是否激活
unsigned char protection_is_fault_active(protection_type_t type) {
    if(type >= 8) return 0;
    return (protection_fault_flags & (1 << type)) ? 1 : 0;
}

// 获取激活的故障类型
protection_type_t protection_get_active_fault(void) {
    return active_protection_fault;
}

// 设置保护配置
void protection_set_config(const protection_config_t* config) {
    protection_config = *config;
}

// 设置保护阈值
void protection_set_thresholds(const protection_thresholds_t* thresholds) {
    protection_thresholds = *thresholds;
    
    // 更新比较器阈值
    comparator_set_threshold(COMP_CURRENT, thresholds->max_current_ma * 100 / 1000);
}

// 获取保护配置
void protection_get_config(protection_config_t* config) {
    *config = protection_config;
}

// 获取保护阈值
void protection_get_thresholds(protection_thresholds_t* thresholds) {
    *thresholds = protection_thresholds;
}

// 设置电流限制
void protection_set_current_limit(unsigned int limit_ma) {
    protection_thresholds.max_current_ma = limit_ma;
    comparator_set_threshold(COMP_CURRENT, limit_ma * 100 / 1000);
}

// 设置电压限制
void protection_set_voltage_limits(unsigned int max_mv, unsigned int min_mv) {
    protection_thresholds.max_voltage_mv = max_mv;
    protection_thresholds.min_voltage_mv = min_mv;
}

// 设置温度限制
void protection_set_temperature_limit(unsigned int limit_c) {
    protection_thresholds.max_temperature_c = limit_c;
}

// 设置速度限制
void protection_set_speed_limit(unsigned int limit_rpm) {
    protection_thresholds.max_speed_rpm = limit_rpm;
}

// 启用保护
void protection_enable(protection_type_t type) {
    switch(type) {
        case PROTECTION_OVERCURRENT:
            protection_config.overcurrent_enabled = 1;
            break;
        case PROTECTION_OVERVOLTAGE:
            protection_config.overvoltage_enabled = 1;
            break;
        case PROTECTION_UNDERVOLTAGE:
            protection_config.undervoltage_enabled = 1;
            break;
        case PROTECTION_OVERTEMP:
            protection_config.overtemp_enabled = 1;
            break;
        case PROTECTION_STALL:
            protection_config.stall_enabled = 1;
            break;
        case PROTECTION_OVERSPEED:
            protection_config.overspeed_enabled = 1;
            break;
    }
}

// 禁用保护
void protection_disable(protection_type_t type) {
    switch(type) {
        case PROTECTION_OVERCURRENT:
            protection_config.overcurrent_enabled = 0;
            break;
        case PROTECTION_OVERVOLTAGE:
            protection_config.overvoltage_enabled = 0;
            break;
        case PROTECTION_UNDERVOLTAGE:
            protection_config.undervoltage_enabled = 0;
            break;
        case PROTECTION_OVERTEMP:
            protection_config.overtemp_enabled = 0;
            break;
        case PROTECTION_STALL:
            protection_config.stall_enabled = 0;
            break;
        case PROTECTION_OVERSPEED:
            protection_config.overspeed_enabled = 0;
            break;
    }
}

// 启用所有保护
void protection_enable_all(void) {
    protection_config.overcurrent_enabled = 1;
    protection_config.overvoltage_enabled = 1;
    protection_config.undervoltage_enabled = 1;
    protection_config.overtemp_enabled = 1;
    protection_config.stall_enabled = 1;
    protection_config.overspeed_enabled = 1;
}

// 禁用所有保护
void protection_disable_all(void) {
    protection_config.overcurrent_enabled = 0;
    protection_config.overvoltage_enabled = 0;
    protection_config.undervoltage_enabled = 0;
    protection_config.overtemp_enabled = 0;
    protection_config.stall_enabled = 0;
    protection_config.overspeed_enabled = 0;
}

// 注册保护回调
void protection_register_callback(protection_callback_t callback) {
    protection_callback = callback;
}

// 获取故障计数
unsigned int protection_get_fault_count(protection_type_t type) {
    if(type >= 8) return 0;
    return protection_fault_counters[type];
}

// 获取总故障计数
unsigned int protection_get_total_fault_count(void) {
    unsigned int total = 0;
    unsigned char i;
    
    for(i = 0; i < 8; i++) {
        total += protection_fault_counters[i];
    }
    
    return total;
}

// 重置故障计数器
void protection_reset_fault_counters(void) {
    unsigned char i;
    for(i = 0; i < 8; i++) {
        protection_fault_counters[i] = 0;
    }
}

// 保护监控处理
void protection_monitor_process(void) {
    // 定期更新保护定时器
    unsigned char i;
    for(i = 0; i < 8; i++) {
        if(protection_timers[i] > 0) {
            protection_timers[i]--;
        }
    }
}

// 保护自检
unsigned char protection_self_test(void) {
    // 测试ADC通道
    if(adc_read_current_ma() == 0 && adc_read_voltage_mv() == 0) {
        return 0;  // ADC故障
    }
    
    // 测试比较器
    // 这里可以添加比较器自检逻辑
    
    return 1;  // 自检通过
}

// 检查故障是否已清除
unsigned char check_fault_cleared(void) {
    // 检查当前状态是否正常
    if(protection_check()) {
        return 0;  // 故障仍存在
    }
    
    // 检查电机是否可以正常运行
    unsigned int current_ma = adc_read_current_ma();
    unsigned int voltage_mv = adc_read_voltage_mv();
    unsigned int temperature_c = adc_read_temperature_c();
    
    if(current_ma < protection_thresholds.max_current_ma &&
       voltage_mv > protection_thresholds.min_voltage_mv &&
       voltage_mv < protection_thresholds.max_voltage_mv &&
       temperature_c < protection_thresholds.max_temperature_c) {
        return 1;  // 故障已清除
    }
    
    return 0;  // 故障未清除
}