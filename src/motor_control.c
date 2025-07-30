#include "motor_control.h"
#include "bemf_detection.h"
#include "pwm_driver.h"
#include "adc_driver.h"
#include <math.h>

// ============================================================================
// 全局变量
// ============================================================================

// 电机控制结构体
static motor_control_t motor_control;

// 电机状态结构体
static motor_state_t motor_state;

// 启动状态机变量
static uint8_t startup_state = 0;
static uint32_t startup_timer = 0;
static float startup_frequency = STARTUP_FREQ_INIT;
static float startup_amplitude = 0.1f;

// ============================================================================
// 电机控制初始化
// ============================================================================

void motor_control_init(void)
{
    // 初始化电机状态
    memset(&motor_state, 0, sizeof(motor_state_t));
    motor_state.speed_reference = 0.0f;
    motor_state.current_reference_d = 0.0f;
    motor_state.current_reference_q = 0.0f;
    motor_state.is_running = 0;
    motor_state.is_startup = 0;
    motor_state.is_bemf_detected = 0;
    
    // 初始化PID控制器
    pid_init(&motor_control.speed_pid, SPEED_CONTROL_KP, SPEED_CONTROL_KI, SPEED_CONTROL_KD, -10.0f, 10.0f);
    pid_init(&motor_control.current_d_pid, CURRENT_CONTROL_KP, CURRENT_CONTROL_KI, CURRENT_CONTROL_KD, -24.0f, 24.0f);
    pid_init(&motor_control.current_q_pid, CURRENT_CONTROL_KP, CURRENT_CONTROL_KI, CURRENT_CONTROL_KD, -24.0f, 24.0f);
    
    // 初始化启动参数
    motor_control.startup_frequency = STARTUP_FREQ_INIT;
    motor_control.startup_amplitude = 0.1f;
    motor_control.startup_phase = 0.0f;
    motor_control.startup_timer = 0;
    
    // 初始化保护参数
    motor_control.current_limit = STARTUP_CURRENT_LIMIT;
    motor_control.voltage_limit = MOTOR_RATED_VOLTAGE;
    motor_control.speed_limit = MOTOR_RATED_SPEED;
    
    // 初始化状态机
    startup_state = 0;
    startup_timer = 0;
    
    // 初始化PWM
    pwm_driver_init();
    
    // 初始化ADC
    adc_driver_init();
}

// ============================================================================
// 电机启动
// ============================================================================

void motor_start(void)
{
    // 重置启动参数
    startup_state = 0;
    startup_timer = 0;
    startup_frequency = STARTUP_FREQ_INIT;
    startup_amplitude = 0.1f;
    
    // 重置PID控制器
    pid_reset(&motor_control.speed_pid);
    pid_reset(&motor_control.current_d_pid);
    pid_reset(&motor_control.current_q_pid);
    
    // 设置启动状态
    motor_state.is_startup = 1;
    motor_state.is_running = 0;
    motor_state.is_bemf_detected = 0;
    
    // 使能PWM
    pwm_enable(1);
    
    // 发送启动消息
    uart_send_string("Motor starting...\r\n");
}

// ============================================================================
// 电机停止
// ============================================================================

void motor_stop(void)
{
    // 禁用PWM
    pwm_enable(0);
    
    // 重置状态
    motor_state.is_running = 0;
    motor_state.is_startup = 0;
    motor_state.is_bemf_detected = 0;
    
    // 重置PID控制器
    pid_reset(&motor_control.speed_pid);
    pid_reset(&motor_control.current_d_pid);
    pid_reset(&motor_control.current_q_pid);
    
    // 清除PWM输出
    pwm_set_three_phase_duty(0, 0, 0);
    
    // 发送停止消息
    uart_send_string("Motor stopped\r\n");
}

// ============================================================================
// 电机运行控制
// ============================================================================

void motor_run(void)
{
    // 读取电流和电压
    motor_state.current.ia = adc_get_current_a();
    motor_state.current.ib = adc_get_current_b();
    motor_state.current.ic = adc_get_current_c();
    
    motor_state.voltage.va = adc_get_voltage_a();
    motor_state.voltage.vb = adc_get_voltage_b();
    motor_state.voltage.vc = adc_get_voltage_c();
    
    // 坐标变换
    clarke_transform(&motor_state.current, &motor_state.current_ab);
    
    // 获取BEMF估算位置
    float position = bemf_get_estimated_position();
    motor_state.position = position;
    
    // Park变换
    park_transform(&motor_state.current_ab, &motor_state.current_dq, position);
    
    // 电流控制
    float voltage_d = pid_calculate(&motor_control.current_d_pid, 
                                   motor_state.current_reference_d, 
                                   motor_state.current_dq.d);
    
    float voltage_q = pid_calculate(&motor_control.current_q_pid, 
                                   motor_state.current_reference_q, 
                                   motor_state.current_dq.q);
    
    // 反Park变换
    dq_t voltage_dq = {voltage_d, voltage_q};
    inverse_park_transform(&voltage_dq, &motor_state.voltage_ab, position);
    
    // 反Clarke变换
    inverse_clarke_transform(&motor_state.voltage_ab, &motor_state.voltage);
    
    // 计算PWM占空比
    float duty_a = (motor_state.voltage.va / MOTOR_RATED_VOLTAGE) * 0.5f + 0.5f;
    float duty_b = (motor_state.voltage.vb / MOTOR_RATED_VOLTAGE) * 0.5f + 0.5f;
    float duty_c = (motor_state.voltage.vc / MOTOR_RATED_VOLTAGE) * 0.5f + 0.5f;
    
    // 限制占空比
    duty_a = (duty_a > 1.0f) ? 1.0f : ((duty_a < 0.0f) ? 0.0f : duty_a);
    duty_b = (duty_b > 1.0f) ? 1.0f : ((duty_b < 0.0f) ? 0.0f : duty_b);
    duty_c = (duty_c > 1.0f) ? 1.0f : ((duty_c < 0.0f) ? 0.0f : duty_c);
    
    // 设置PWM输出
    pwm_set_three_phase_duty_percent(duty_a * 100.0f, duty_b * 100.0f, duty_c * 100.0f);
    
    // 更新速度
    motor_state.speed = bemf_get_estimated_speed();
}

// ============================================================================
// 带载启动状态机
// ============================================================================

void motor_startup_state_machine(void)
{
    static uint32_t startup_step_timer = 0;
    static uint8_t startup_step = 0;
    
    switch (startup_step) {
        case 0: // 初始化启动
            startup_step_timer = 0;
            startup_frequency = STARTUP_FREQ_INIT;
            startup_amplitude = 0.1f;
            startup_step = 1;
            uart_send_string("Startup Step 0: Initialization\r\n");
            break;
            
        case 1: // 开环启动
            if (startup_step_timer < 1000) { // 1秒开环启动
                // 生成开环电压矢量
                float angle = 2.0f * M_PI * startup_frequency * startup_step_timer / 1000.0f;
                float va = startup_amplitude * cosf(angle);
                float vb = startup_amplitude * cosf(angle - 2.0f * M_PI / 3.0f);
                float vc = startup_amplitude * cosf(angle + 2.0f * M_PI / 3.0f);
                
                // 设置PWM输出
                pwm_set_three_phase_duty_percent(
                    (va + 1.0f) * 50.0f,
                    (vb + 1.0f) * 50.0f,
                    (vc + 1.0f) * 50.0f
                );
                
                // 增加频率和幅值
                startup_frequency += STARTUP_FREQ_RAMP / 1000.0f;
                startup_amplitude += 0.001f;
                
                startup_step_timer++;
            } else {
                startup_step = 2;
                uart_send_string("Startup Step 1: Open Loop Complete\r\n");
            }
            break;
            
        case 2: // 等待BEMF检测
            if (bemf_detection_is_valid()) {
                startup_step = 3;
                uart_send_string("Startup Step 2: BEMF Detected\r\n");
            } else {
                // 继续开环运行
                float angle = 2.0f * M_PI * startup_frequency * startup_step_timer / 1000.0f;
                float va = startup_amplitude * cosf(angle);
                float vb = startup_amplitude * cosf(angle - 2.0f * M_PI / 3.0f);
                float vc = startup_amplitude * cosf(angle + 2.0f * M_PI / 3.0f);
                
                pwm_set_three_phase_duty_percent(
                    (va + 1.0f) * 50.0f,
                    (vb + 1.0f) * 50.0f,
                    (vc + 1.0f) * 50.0f
                );
                
                startup_step_timer++;
            }
            break;
            
        case 3: // 切换到闭环控制
            motor_state.is_startup = 0;
            motor_state.is_running = 1;
            motor_state.is_bemf_detected = 1;
            startup_step = 4;
            uart_send_string("Startup Step 3: Closed Loop Control\r\n");
            break;
            
        case 4: // 启动完成
            uart_send_string("Motor startup completed successfully\r\n");
            break;
            
        default:
            startup_step = 0;
            break;
    }
}

// ============================================================================
// 坐标变换函数
// ============================================================================

// Clarke变换 (三相到两相静止坐标系)
void clarke_transform(three_phase_current_t* current, alpha_beta_t* ab)
{
    ab->alpha = current->ia;
    ab->beta = (current->ia + 2.0f * current->ib) / sqrtf(3.0f);
}

// Park变换 (静止坐标系到旋转坐标系)
void park_transform(alpha_beta_t* ab, dq_t* dq, float angle)
{
    float cos_angle = cosf(angle);
    float sin_angle = sinf(angle);
    
    dq->d = ab->alpha * cos_angle + ab->beta * sin_angle;
    dq->q = -ab->alpha * sin_angle + ab->beta * cos_angle;
}

// 反Park变换 (旋转坐标系到静止坐标系)
void inverse_park_transform(dq_t* dq, alpha_beta_t* ab, float angle)
{
    float cos_angle = cosf(angle);
    float sin_angle = sinf(angle);
    
    ab->alpha = dq->d * cos_angle - dq->q * sin_angle;
    ab->beta = dq->d * sin_angle + dq->q * cos_angle;
}

// 反Clarke变换 (两相静止坐标系到三相)
void inverse_clarke_transform(alpha_beta_t* ab, three_phase_voltage_t* voltage)
{
    voltage->va = ab->alpha;
    voltage->vb = -0.5f * ab->alpha + sqrtf(3.0f) * 0.5f * ab->beta;
    voltage->vc = -0.5f * ab->alpha - sqrtf(3.0f) * 0.5f * ab->beta;
}

// ============================================================================
// PID控制器函数
// ============================================================================

void pid_init(pid_controller_t* pid, float kp, float ki, float kd, 
              float output_min, float output_max)
{
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
    pid->setpoint = 0.0f;
    pid->feedback = 0.0f;
    pid->output = 0.0f;
    pid->integral = 0.0f;
    pid->derivative = 0.0f;
    pid->output_min = output_min;
    pid->output_max = output_max;
    pid->integral_min = output_min;
    pid->integral_max = output_max;
}

float pid_calculate(pid_controller_t* pid, float setpoint, float feedback)
{
    float error = setpoint - feedback;
    
    // 比例项
    float proportional = pid->kp * error;
    
    // 积分项
    pid->integral += pid->ki * error;
    if (pid->integral > pid->integral_max) pid->integral = pid->integral_max;
    if (pid->integral < pid->integral_min) pid->integral = pid->integral_min;
    
    // 微分项
    float derivative = pid->kd * (error - pid->derivative);
    pid->derivative = error;
    
    // 计算输出
    pid->output = proportional + pid->integral + derivative;
    
    // 限制输出
    if (pid->output > pid->output_max) pid->output = pid->output_max;
    if (pid->output < pid->output_min) pid->output = pid->output_min;
    
    return pid->output;
}

void pid_reset(pid_controller_t* pid)
{
    pid->integral = 0.0f;
    pid->derivative = 0.0f;
    pid->output = 0.0f;
}

// ============================================================================
// 保护功能
// ============================================================================

uint8_t motor_protection_check(void)
{
    // 检查过流
    float current_magnitude = sqrtf(motor_state.current.ia * motor_state.current.ia + 
                                   motor_state.current.ib * motor_state.current.ib + 
                                   motor_state.current.ic * motor_state.current.ic);
    
    if (current_magnitude > motor_control.current_limit) {
        return 0; // 保护触发
    }
    
    // 检查过压
    float voltage_dc = adc_get_dc_voltage();
    if (voltage_dc > motor_control.voltage_limit) {
        return 0; // 保护触发
    }
    
    return 1; // 正常
}

void motor_protection_action(fault_code_t fault)
{
    // 紧急停止
    motor_stop();
    
    // 记录故障
    switch (fault) {
        case FAULT_OVER_CURRENT:
            uart_send_string("Protection: Over Current\r\n");
            break;
        case FAULT_OVER_VOLTAGE:
            uart_send_string("Protection: Over Voltage\r\n");
            break;
        default:
            uart_send_string("Protection: Unknown Fault\r\n");
            break;
    }
}

// ============================================================================
// 状态机函数
// ============================================================================

void motor_state_machine(void)
{
    // 检查保护
    if (!motor_protection_check()) {
        motor_protection_action(FAULT_OVER_CURRENT);
        return;
    }
    
    // 正常运行
    if (motor_state.is_running) {
        motor_run();
    }
}

// ============================================================================
// 设置函数
// ============================================================================

void motor_set_speed_reference(float speed_rpm)
{
    motor_state.speed_reference = speed_rpm;
}

void motor_set_current_reference(float id_ref, float iq_ref)
{
    motor_state.current_reference_d = id_ref;
    motor_state.current_reference_q = iq_ref;
}

// ============================================================================
// 获取函数
// ============================================================================

motor_state_t* motor_get_state(void)
{
    return &motor_state;
}

motor_control_t* motor_get_control(void)
{
    return &motor_control;
}

// ============================================================================
// 调试函数
// ============================================================================

void motor_debug_print(void)
{
    uart_send_string("Motor Debug Info:\r\n");
    uart_send_string("Speed: ");
    uart_send_float(motor_state.speed);
    uart_send_string(" RPM\r\n");
    uart_send_string("Position: ");
    uart_send_float(motor_state.position);
    uart_send_string(" rad\r\n");
    uart_send_string("Current d: ");
    uart_send_float(motor_state.current_dq.d);
    uart_send_string(" A\r\n");
    uart_send_string("Current q: ");
    uart_send_float(motor_state.current_dq.q);
    uart_send_string(" A\r\n");
}

void motor_parameter_display(void)
{
    // 这里可以添加参数显示功能
}