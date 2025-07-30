#ifndef MOTOR_CONTROL_H
#define MOTOR_CONTROL_H

#include "config.h"

// ============================================================================
// 数据结构定义
// ============================================================================

// 三相电压矢量
typedef struct {
    float va;      // A相电压
    float vb;      // B相电压
    float vc;      // C相电压
} three_phase_voltage_t;

// 三相电流矢量
typedef struct {
    float ia;      // A相电流
    float ib;      // B相电流
    float ic;      // C相电流
} three_phase_current_t;

// 两相静止坐标系(α-β)
typedef struct {
    float alpha;   // α轴分量
    float beta;    // β轴分量
} alpha_beta_t;

// 两相旋转坐标系(d-q)
typedef struct {
    float d;       // d轴分量
    float q;       // q轴分量
} dq_t;

// 电机状态结构体
typedef struct {
    // 电气量
    three_phase_voltage_t voltage;    // 三相电压
    three_phase_current_t current;    // 三相电流
    alpha_beta_t voltage_ab;          // 电压α-β分量
    alpha_beta_t current_ab;          // 电流α-β分量
    dq_t voltage_dq;                  // 电压d-q分量
    dq_t current_dq;                  // 电流d-q分量
    
    // 机械量
    float speed;                      // 转速(RPM)
    float position;                   // 位置(rad)
    float torque;                     // 转矩(Nm)
    
    // 控制量
    float speed_reference;            // 速度参考值
    float current_reference_d;        // d轴电流参考值
    float current_reference_q;        // q轴电流参考值
    
    // 状态标志
    uint8_t is_running;              // 运行状态
    uint8_t is_startup;              // 启动状态
    uint8_t is_bemf_detected;        // BEMF检测状态
} motor_state_t;

// PID控制器结构体
typedef struct {
    float kp;                         // 比例系数
    float ki;                         // 积分系数
    float kd;                         // 微分系数
    float setpoint;                   // 设定值
    float feedback;                   // 反馈值
    float output;                     // 输出值
    float integral;                   // 积分项
    float derivative;                 // 微分项
    float output_min;                 // 输出最小值
    float output_max;                 // 输出最大值
    float integral_min;               // 积分最小值
    float integral_max;               // 积分最大值
} pid_controller_t;

// 电机控制结构体
typedef struct {
    motor_state_t state;              // 电机状态
    pid_controller_t speed_pid;       // 速度PID控制器
    pid_controller_t current_d_pid;   // d轴电流PID控制器
    pid_controller_t current_q_pid;   // q轴电流PID控制器
    
    // 启动参数
    float startup_frequency;          // 启动频率
    float startup_amplitude;          // 启动幅值
    float startup_phase;              // 启动相位
    uint32_t startup_timer;           // 启动计时器
    
    // 保护参数
    float current_limit;              // 电流限制
    float voltage_limit;              // 电压限制
    float speed_limit;                // 速度限制
} motor_control_t;

// ============================================================================
// 函数声明
// ============================================================================

// 电机控制初始化
void motor_control_init(void);

// 电机启动
void motor_start(void);

// 电机停止
void motor_stop(void);

// 电机运行控制
void motor_run(void);

// 设置速度参考值
void motor_set_speed_reference(float speed_rpm);

// 设置电流参考值
void motor_set_current_reference(float id_ref, float iq_ref);

// 获取电机状态
motor_state_t* motor_get_state(void);

// 获取电机控制结构
motor_control_t* motor_get_control(void);

// 坐标变换函数
void clarke_transform(three_phase_current_t* current, alpha_beta_t* ab);
void park_transform(alpha_beta_t* ab, dq_t* dq, float angle);
void inverse_park_transform(dq_t* dq, alpha_beta_t* ab, float angle);
void inverse_clarke_transform(alpha_beta_t* ab, three_phase_voltage_t* voltage);

// PID控制器函数
void pid_init(pid_controller_t* pid, float kp, float ki, float kd, 
              float output_min, float output_max);
float pid_calculate(pid_controller_t* pid, float setpoint, float feedback);
void pid_reset(pid_controller_t* pid);

// 保护功能
uint8_t motor_protection_check(void);
void motor_protection_action(fault_code_t fault);

// 状态机函数
void motor_state_machine(void);
void motor_startup_state_machine(void);

// 调试函数
void motor_debug_print(void);
void motor_parameter_display(void);

#endif // MOTOR_CONTROL_H