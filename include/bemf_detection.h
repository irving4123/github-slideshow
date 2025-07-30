#ifndef BEMF_DETECTION_H
#define BEMF_DETECTION_H

#include "config.h"
#include "motor_control.h"

// ============================================================================
// BEMF检测数据结构
// ============================================================================

// BEMF检测状态
typedef enum {
    BEMF_STATE_INIT = 0,         // 初始化状态
    BEMF_STATE_DETECTING,        // 检测状态
    BEMF_STATE_DETECTED,         // 已检测状态
    BEMF_STATE_LOST,             // 丢失状态
    BEMF_STATE_ERROR             // 错误状态
} bemf_state_t;

// BEMF检测参数结构体
typedef struct {
    float threshold;              // 检测阈值
    float filter_time_const;      // 滤波时间常数
    float detection_delay;        // 检测延迟
    float min_speed;             // 最小检测速度
    float max_speed;             // 最大检测速度
    uint32_t detection_timeout;   // 检测超时时间
} bemf_detection_params_t;

// BEMF检测结果结构体
typedef struct {
    float bemf_amplitude;        // BEMF幅值
    float bemf_frequency;        // BEMF频率
    float bemf_phase;            // BEMF相位
    float estimated_position;     // 估算位置
    float estimated_speed;       // 估算速度
    float detection_confidence;   // 检测置信度
    bemf_state_t state;          // 检测状态
    uint32_t detection_count;    // 检测计数
    uint32_t lost_count;         // 丢失计数
} bemf_detection_result_t;

// BEMF检测器结构体
typedef struct {
    bemf_detection_params_t params;      // 检测参数
    bemf_detection_result_t result;      // 检测结果
    
    // 滤波器
    float bemf_filter_alpha;             // BEMF滤波系数
    float position_filter_alpha;          // 位置滤波系数
    float speed_filter_alpha;             // 速度滤波系数
    
    // 检测变量
    float bemf_voltage_alpha;            // α轴BEMF电压
    float bemf_voltage_beta;             // β轴BEMF电压
    float bemf_voltage_magnitude;        // BEMF电压幅值
    float bemf_voltage_angle;            // BEMF电压角度
    
    // 估算变量
    float position_estimator_gain;       // 位置估算增益
    float speed_estimator_gain;          // 速度估算增益
    float position_error;                // 位置误差
    float speed_error;                   // 速度误差
    
    // 状态变量
    uint32_t sample_count;               // 采样计数
    uint32_t detection_timer;            // 检测计时器
    uint8_t is_initialized;              // 初始化标志
} bemf_detector_t;

// ============================================================================
// 函数声明
// ============================================================================

// BEMF检测器初始化
void bemf_detector_init(void);

// BEMF检测器参数设置
void bemf_set_detection_params(bemf_detection_params_t* params);

// BEMF检测主函数
void bemf_detection_process(void);

// BEMF电压计算
void bemf_voltage_calculate(three_phase_voltage_t* voltage, 
                           three_phase_current_t* current,
                           float* bemf_alpha, float* bemf_beta);

// BEMF电压滤波
void bemf_voltage_filter(float* bemf_alpha, float* bemf_beta);

// BEMF检测判断
uint8_t bemf_detection_check(float bemf_alpha, float bemf_beta);

// 位置估算
float bemf_position_estimate(float bemf_alpha, float bemf_beta);

// 速度估算
float bemf_speed_estimate(float position, float dt);

// 位置和速度滤波
void bemf_position_speed_filter(float* position, float* speed);

// 检测置信度计算
float bemf_confidence_calculate(void);

// 检测状态更新
void bemf_state_update(void);

// 获取BEMF检测结果
bemf_detection_result_t* bemf_get_result(void);

// 获取BEMF检测器
bemf_detector_t* bemf_get_detector(void);

// BEMF检测器重置
void bemf_detector_reset(void);

// BEMF检测器使能/禁用
void bemf_detector_enable(uint8_t enable);

// BEMF检测器自检
uint8_t bemf_detector_self_test(void);

// BEMF检测器校准
void bemf_detector_calibrate(void);

// BEMF检测器调试
void bemf_debug_print(void);

// BEMF检测器参数显示
void bemf_parameter_display(void);

// ============================================================================
// 内联函数
// ============================================================================

// 计算BEMF电压幅值
static inline float bemf_voltage_magnitude_calculate(float alpha, float beta) {
    return sqrtf(alpha * alpha + beta * beta);
}

// 计算BEMF电压角度
static inline float bemf_voltage_angle_calculate(float alpha, float beta) {
    return atan2f(beta, alpha);
}

// 检查BEMF检测是否有效
static inline uint8_t bemf_detection_is_valid(void) {
    bemf_detection_result_t* result = bemf_get_result();
    return (result->state == BEMF_STATE_DETECTED && 
            result->detection_confidence > 0.5f);
}

// 获取估算位置
static inline float bemf_get_estimated_position(void) {
    bemf_detection_result_t* result = bemf_get_result();
    return result->estimated_position;
}

// 获取估算速度
static inline float bemf_get_estimated_speed(void) {
    bemf_detection_result_t* result = bemf_get_result();
    return result->estimated_speed;
}

#endif // BEMF_DETECTION_H