#include "bemf_detection.h"
#include "motor_control.h"
#include "adc_driver.h"
#include <math.h>

// ============================================================================
// 全局变量
// ============================================================================

// BEMF检测器结构体
static bemf_detector_t bemf_detector;

// BEMF检测结果
static bemf_detection_result_t bemf_result;

// 滤波器变量
static float bemf_alpha_filtered = 0.0f;
static float bemf_beta_filtered = 0.0f;
static float position_filtered = 0.0f;
static float speed_filtered = 0.0f;

// 位置估算变量
static float position_previous = 0.0f;
static float speed_previous = 0.0f;
static uint32_t last_update_time = 0;

// ============================================================================
// BEMF检测器初始化
// ============================================================================

void bemf_detector_init(void)
{
    // 初始化检测参数
    bemf_detector.params.threshold = BEMF_DETECTION_THRESHOLD;
    bemf_detector.params.filter_time_const = BEMF_FILTER_TIME_CONST;
    bemf_detector.params.detection_delay = BEMF_DETECTION_DELAY;
    bemf_detector.params.min_speed = 50.0f;  // 最小检测速度50RPM
    bemf_detector.params.max_speed = 10000.0f; // 最大检测速度10000RPM
    bemf_detector.params.detection_timeout = 1000; // 检测超时1秒
    
    // 初始化滤波器系数
    bemf_detector.bemf_filter_alpha = 0.95f;
    bemf_detector.position_filter_alpha = 0.98f;
    bemf_detector.speed_filter_alpha = 0.9f;
    
    // 初始化估算增益
    bemf_detector.position_estimator_gain = POSITION_ESTIMATION_GAIN;
    bemf_detector.speed_estimator_gain = SPEED_ESTIMATION_GAIN;
    
    // 初始化状态变量
    bemf_detector.sample_count = 0;
    bemf_detector.detection_timer = 0;
    bemf_detector.is_initialized = 1;
    
    // 初始化检测结果
    memset(&bemf_result, 0, sizeof(bemf_detection_result_t));
    bemf_result.state = BEMF_STATE_INIT;
    bemf_result.detection_confidence = 0.0f;
    
    // 初始化滤波器
    bemf_alpha_filtered = 0.0f;
    bemf_beta_filtered = 0.0f;
    position_filtered = 0.0f;
    speed_filtered = 0.0f;
    
    // 初始化位置估算
    position_previous = 0.0f;
    speed_previous = 0.0f;
    last_update_time = 0;
}

// ============================================================================
// BEMF检测主函数
// ============================================================================

void bemf_detection_process(void)
{
    // 读取三相电压和电流
    three_phase_voltage_t voltage;
    three_phase_current_t current;
    
    voltage.va = adc_get_voltage_a();
    voltage.vb = adc_get_voltage_b();
    voltage.vc = adc_get_voltage_c();
    
    current.ia = adc_get_current_a();
    current.ib = adc_get_current_b();
    current.ic = adc_get_current_c();
    
    // 计算BEMF电压
    float bemf_alpha, bemf_beta;
    bemf_voltage_calculate(&voltage, &current, &bemf_alpha, &bemf_beta);
    
    // BEMF电压滤波
    bemf_voltage_filter(&bemf_alpha, &bemf_beta);
    
    // 检测BEMF
    uint8_t bemf_detected = bemf_detection_check(bemf_alpha, bemf_beta);
    
    // 位置估算
    float position = bemf_position_estimate(bemf_alpha, bemf_beta);
    
    // 速度估算
    float dt = 0.001f; // 1ms采样周期
    float speed = bemf_speed_estimate(position, dt);
    
    // 位置和速度滤波
    bemf_position_speed_filter(&position, &speed);
    
    // 更新检测结果
    bemf_result.estimated_position = position;
    bemf_result.estimated_speed = speed;
    bemf_result.bemf_amplitude = sqrtf(bemf_alpha * bemf_alpha + bemf_beta * bemf_beta);
    bemf_result.bemf_frequency = speed / 60.0f * MOTOR_POLE_PAIRS; // 转换为Hz
    bemf_result.bemf_phase = atan2f(bemf_beta, bemf_alpha);
    
    // 更新检测状态
    bemf_state_update();
    
    // 计算检测置信度
    bemf_result.detection_confidence = bemf_confidence_calculate();
    
    // 更新检测计数
    if (bemf_detected) {
        bemf_result.detection_count++;
        bemf_result.lost_count = 0;
    } else {
        bemf_result.lost_count++;
    }
    
    // 更新采样计数
    bemf_detector.sample_count++;
}

// ============================================================================
// BEMF电压计算
// ============================================================================

void bemf_voltage_calculate(three_phase_voltage_t* voltage, 
                           three_phase_current_t* current,
                           float* bemf_alpha, float* bemf_beta)
{
    // 计算相电压的α-β分量
    alpha_beta_t voltage_ab;
    clarke_transform(current, &voltage_ab); // 使用电流进行Clarke变换
    
    // 计算相电流的α-β分量
    alpha_beta_t current_ab;
    clarke_transform(current, &current_ab);
    
    // 计算定子电阻压降
    float voltage_drop_alpha = current_ab.alpha * MOTOR_PHASE_RESISTANCE;
    float voltage_drop_beta = current_ab.beta * MOTOR_PHASE_RESISTANCE;
    
    // 计算定子电感压降 (简化计算)
    float voltage_inductive_alpha = 0.0f;
    float voltage_inductive_beta = 0.0f;
    
    // BEMF电压 = 端电压 - 电阻压降 - 电感压降
    *bemf_alpha = voltage_ab.alpha - voltage_drop_alpha - voltage_inductive_alpha;
    *bemf_beta = voltage_ab.beta - voltage_drop_beta - voltage_inductive_beta;
}

// ============================================================================
// BEMF电压滤波
// ============================================================================

void bemf_voltage_filter(float* bemf_alpha, float* bemf_beta)
{
    // 低通滤波器
    bemf_alpha_filtered = bemf_detector.bemf_filter_alpha * bemf_alpha_filtered + 
                          (1.0f - bemf_detector.bemf_filter_alpha) * (*bemf_alpha);
    bemf_beta_filtered = bemf_detector.bemf_filter_alpha * bemf_beta_filtered + 
                         (1.0f - bemf_detector.bemf_filter_alpha) * (*bemf_beta);
    
    *bemf_alpha = bemf_alpha_filtered;
    *bemf_beta = bemf_beta_filtered;
}

// ============================================================================
// BEMF检测判断
// ============================================================================

uint8_t bemf_detection_check(float bemf_alpha, float bemf_beta)
{
    // 计算BEMF电压幅值
    float bemf_magnitude = sqrtf(bemf_alpha * bemf_alpha + bemf_beta * bemf_beta);
    
    // 检查BEMF电压是否超过阈值
    if (bemf_magnitude > bemf_detector.params.threshold) {
        return 1; // 检测到BEMF
    }
    
    return 0; // 未检测到BEMF
}

// ============================================================================
// 位置估算
// ============================================================================

float bemf_position_estimate(float bemf_alpha, float bemf_beta)
{
    // 使用BEMF电压角度估算位置
    float position = atan2f(bemf_beta, bemf_alpha);
    
    // 位置连续性处理
    static float position_accumulated = 0.0f;
    static float position_previous_angle = 0.0f;
    
    // 检测位置跳变
    float position_diff = position - position_previous_angle;
    
    // 处理角度跳变
    if (position_diff > M_PI) {
        position_diff -= 2.0f * M_PI;
    } else if (position_diff < -M_PI) {
        position_diff += 2.0f * M_PI;
    }
    
    // 累积位置
    position_accumulated += position_diff;
    position_previous_angle = position;
    
    return position_accumulated;
}

// ============================================================================
// 速度估算
// ============================================================================

float bemf_speed_estimate(float position, float dt)
{
    // 使用位置差分估算速度
    float position_diff = position - position_previous;
    
    // 处理位置跳变
    if (position_diff > M_PI) {
        position_diff -= 2.0f * M_PI;
    } else if (position_diff < -M_PI) {
        position_diff += 2.0f * M_PI;
    }
    
    // 计算角速度 (rad/s)
    float angular_speed = position_diff / dt;
    
    // 转换为转速 (RPM)
    float speed = angular_speed * 60.0f / (2.0f * M_PI * MOTOR_POLE_PAIRS);
    
    // 更新位置
    position_previous = position;
    
    return speed;
}

// ============================================================================
// 位置和速度滤波
// ============================================================================

void bemf_position_speed_filter(float* position, float* speed)
{
    // 位置滤波
    position_filtered = bemf_detector.position_filter_alpha * position_filtered + 
                       (1.0f - bemf_detector.position_filter_alpha) * (*position);
    *position = position_filtered;
    
    // 速度滤波
    speed_filtered = bemf_detector.speed_filter_alpha * speed_filtered + 
                    (1.0f - bemf_detector.speed_filter_alpha) * (*speed);
    *speed = speed_filtered;
}

// ============================================================================
// 检测置信度计算
// ============================================================================

float bemf_confidence_calculate(void)
{
    float confidence = 0.0f;
    
    // 基于BEMF幅值的置信度
    float bemf_amplitude_confidence = bemf_result.bemf_amplitude / 1.0f; // 归一化
    if (bemf_amplitude_confidence > 1.0f) bemf_amplitude_confidence = 1.0f;
    
    // 基于速度的置信度
    float speed_confidence = 0.0f;
    if (bemf_result.estimated_speed >= bemf_detector.params.min_speed && 
        bemf_result.estimated_speed <= bemf_detector.params.max_speed) {
        speed_confidence = 1.0f;
    }
    
    // 基于检测计数的置信度
    float detection_confidence = (float)bemf_result.detection_count / 100.0f;
    if (detection_confidence > 1.0f) detection_confidence = 1.0f;
    
    // 综合置信度
    confidence = (bemf_amplitude_confidence + speed_confidence + detection_confidence) / 3.0f;
    
    return confidence;
}

// ============================================================================
// 检测状态更新
// ============================================================================

void bemf_state_update(void)
{
    switch (bemf_result.state) {
        case BEMF_STATE_INIT:
            // 初始化状态，等待开始检测
            bemf_result.state = BEMF_STATE_DETECTING;
            break;
            
        case BEMF_STATE_DETECTING:
            // 检测状态
            if (bemf_result.detection_confidence > 0.5f) {
                bemf_result.state = BEMF_STATE_DETECTED;
            }
            break;
            
        case BEMF_STATE_DETECTED:
            // 已检测状态
            if (bemf_result.detection_confidence < 0.3f) {
                bemf_result.state = BEMF_STATE_LOST;
            }
            break;
            
        case BEMF_STATE_LOST:
            // 丢失状态
            if (bemf_result.detection_confidence > 0.5f) {
                bemf_result.state = BEMF_STATE_DETECTED;
            } else if (bemf_result.lost_count > 100) {
                bemf_result.state = BEMF_STATE_ERROR;
            }
            break;
            
        case BEMF_STATE_ERROR:
            // 错误状态
            if (bemf_result.detection_confidence > 0.7f) {
                bemf_result.state = BEMF_STATE_DETECTED;
            }
            break;
            
        default:
            bemf_result.state = BEMF_STATE_INIT;
            break;
    }
}

// ============================================================================
// 获取函数
// ============================================================================

bemf_detection_result_t* bemf_get_result(void)
{
    return &bemf_result;
}

bemf_detector_t* bemf_get_detector(void)
{
    return &bemf_detector;
}

// ============================================================================
// 控制函数
// ============================================================================

void bemf_detector_reset(void)
{
    // 重置检测器
    bemf_detector_init();
}

void bemf_detector_enable(uint8_t enable)
{
    bemf_detector.is_initialized = enable;
}

uint8_t bemf_detector_self_test(void)
{
    // 简单的自检功能
    if (bemf_detector.is_initialized && 
        bemf_detector.params.threshold > 0.0f) {
        return 1; // 自检通过
    }
    return 0; // 自检失败
}

void bemf_detector_calibrate(void)
{
    // 校准功能
    // 这里可以添加校准算法
    uart_send_string("BEMF detector calibration completed\r\n");
}

// ============================================================================
// 调试函数
// ============================================================================

void bemf_debug_print(void)
{
    uart_send_string("BEMF Detection Debug:\r\n");
    uart_send_string("State: ");
    switch (bemf_result.state) {
        case BEMF_STATE_INIT: uart_send_string("INIT"); break;
        case BEMF_STATE_DETECTING: uart_send_string("DETECTING"); break;
        case BEMF_STATE_DETECTED: uart_send_string("DETECTED"); break;
        case BEMF_STATE_LOST: uart_send_string("LOST"); break;
        case BEMF_STATE_ERROR: uart_send_string("ERROR"); break;
    }
    uart_send_string("\r\n");
    
    uart_send_string("Amplitude: ");
    uart_send_float(bemf_result.bemf_amplitude);
    uart_send_string(" V\r\n");
    
    uart_send_string("Frequency: ");
    uart_send_float(bemf_result.bemf_frequency);
    uart_send_string(" Hz\r\n");
    
    uart_send_string("Confidence: ");
    uart_send_float(bemf_result.detection_confidence);
    uart_send_string("\r\n");
}

void bemf_parameter_display(void)
{
    uart_send_string("BEMF Parameters:\r\n");
    uart_send_string("Threshold: ");
    uart_send_float(bemf_detector.params.threshold);
    uart_send_string(" V\r\n");
    
    uart_send_string("Filter Time Const: ");
    uart_send_float(bemf_detector.params.filter_time_const);
    uart_send_string(" s\r\n");
    
    uart_send_string("Min Speed: ");
    uart_send_float(bemf_detector.params.min_speed);
    uart_send_string(" RPM\r\n");
    
    uart_send_string("Max Speed: ");
    uart_send_float(bemf_detector.params.max_speed);
    uart_send_string(" RPM\r\n");
}