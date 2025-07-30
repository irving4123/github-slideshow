#ifndef BEMF_CONTROL_H
#define BEMF_CONTROL_H

#include "bemf_config.h"

// BEMF控制相关函数声明
void bemf_control_init(void);
void bemf_control_process(void);
unsigned char detect_bemf_zero_crossing(void);
void commutate_motor(unsigned char step);
unsigned int calculate_speed(void);
void update_commutation_timing(void);

// 换相相关函数
void set_commutation_step(unsigned char step);
void next_commutation_step(void);
unsigned char get_current_step(void);

// BEMF检测函数
unsigned int read_bemf_voltage(unsigned char phase);
unsigned char is_bemf_zero_crossing(unsigned char phase);
void bemf_filter_process(void);

// 速度计算函数
void speed_measurement_init(void);
void update_speed_measurement(void);

// 全局变量声明
extern volatile unsigned char current_commutation_step;
extern volatile unsigned int commutation_period;
extern volatile unsigned char bemf_detection_enabled;
extern volatile unsigned int speed_rpm;

#endif // BEMF_CONTROL_H