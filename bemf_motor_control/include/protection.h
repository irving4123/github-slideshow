#ifndef PROTECTION_H
#define PROTECTION_H

#include "bemf_config.h"

// 保护类型定义
typedef enum {
    PROTECTION_NONE = 0,
    PROTECTION_OVERCURRENT,
    PROTECTION_OVERVOLTAGE,
    PROTECTION_UNDERVOLTAGE,
    PROTECTION_OVERTEMP,
    PROTECTION_STALL,
    PROTECTION_OVERSPEED,
    PROTECTION_HALL_ERROR
} protection_type_t;

// 保护状态定义
typedef struct {
    unsigned char overcurrent_enabled;
    unsigned char overvoltage_enabled;
    unsigned char undervoltage_enabled;
    unsigned char overtemp_enabled;
    unsigned char stall_enabled;
    unsigned char overspeed_enabled;
} protection_config_t;

// 保护阈值设置
typedef struct {
    unsigned int max_current_ma;
    unsigned int max_voltage_mv;
    unsigned int min_voltage_mv;
    unsigned int max_temperature_c;
    unsigned int max_speed_rpm;
    unsigned int stall_timeout_ms;
    unsigned int protection_delay_ms;
} protection_thresholds_t;

// 保护初始化和配置
void protection_init(void);
void protection_set_config(const protection_config_t* config);
void protection_set_thresholds(const protection_thresholds_t* thresholds);
void protection_get_config(protection_config_t* config);
void protection_get_thresholds(protection_thresholds_t* thresholds);

// 保护检查函数
unsigned char protection_check(void);
protection_type_t protection_get_active_fault(void);
unsigned char protection_is_fault_active(protection_type_t type);
void protection_clear_fault(protection_type_t type);
void protection_clear_all_faults(void);

// 具体保护检查函数
unsigned char check_overcurrent_protection(void);
unsigned char check_overvoltage_protection(void);
unsigned char check_undervoltage_protection(void);
unsigned char check_overtemp_protection(void);
unsigned char check_stall_protection(void);
unsigned char check_overspeed_protection(void);

// 保护动作函数
void protection_trigger_fault(protection_type_t type);
void protection_emergency_stop(void);
void protection_soft_stop(void);
void protection_reduce_power(void);

// 保护状态查询
unsigned char protection_has_fault(void);
unsigned int protection_get_fault_count(protection_type_t type);
unsigned int protection_get_total_fault_count(void);
void protection_reset_fault_counters(void);

// 保护参数动态调整
void protection_set_current_limit(unsigned int limit_ma);
void protection_set_voltage_limits(unsigned int max_mv, unsigned int min_mv);
void protection_set_temperature_limit(unsigned int limit_c);
void protection_set_speed_limit(unsigned int limit_rpm);

// 保护使能/禁用
void protection_enable(protection_type_t type);
void protection_disable(protection_type_t type);
void protection_enable_all(void);
void protection_disable_all(void);

// 故障处理回调
typedef void (*protection_callback_t)(protection_type_t type);
void protection_register_callback(protection_callback_t callback);

// 保护监控和诊断
void protection_monitor_process(void);
unsigned char protection_self_test(void);
void protection_get_status_string(char* buffer, unsigned int buffer_size);

// 全局变量声明
extern volatile protection_type_t active_protection_fault;
extern volatile unsigned char protection_fault_flags;
extern volatile unsigned int protection_fault_counters[8];

#endif // PROTECTION_H