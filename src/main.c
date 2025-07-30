#include "config.h"
#include "motor_control.h"
#include "bemf_detection.h"
#include "pwm_driver.h"
#include "adc_driver.h"
#include "timer_driver.h"
#include "uart_driver.h"

// ============================================================================
// 全局变量
// ============================================================================

// 系统状态
static system_state_t system_state = SYSTEM_STATE_INIT;
static fault_code_t system_fault = FAULT_NONE;

// 系统计时器
static uint32_t system_timer = 0;
static uint32_t main_loop_counter = 0;

// 任务调度变量
static uint32_t task_timer_high = 0;    // 高优先级任务计时器
static uint32_t task_timer_medium = 0;  // 中优先级任务计时器
static uint32_t task_timer_low = 0;     // 低优先级任务计时器

// ============================================================================
// 函数声明
// ============================================================================

// 系统初始化
void system_init(void);

// 系统状态机
void system_state_machine(void);

// 任务调度
void task_scheduler(void);

// 高优先级任务
void high_priority_task(void);

// 中优先级任务
void medium_priority_task(void);

// 低优先级任务
void low_priority_task(void);

// 故障处理
void fault_handler(void);

// 系统监控
void system_monitor(void);

// 调试输出
void debug_output(void);

// ============================================================================
// 主函数
// ============================================================================

int main(void)
{
    // 系统初始化
    system_init();
    
    // 主循环
    while (1) {
        // 系统状态机
        system_state_machine();
        
        // 任务调度
        task_scheduler();
        
        // 系统监控
        system_monitor();
        
        // 调试输出
        if (DEBUG_ENABLE) {
            debug_output();
        }
        
        main_loop_counter++;
    }
}

// ============================================================================
// 系统初始化
// ============================================================================

void system_init(void)
{
    // 禁用中断
    EA = 0;
    
    // 初始化硬件驱动
    uart_driver_init();
    timer_driver_init();
    adc_driver_init();
    pwm_driver_init();
    
    // 初始化电机控制
    motor_control_init();
    
    // 初始化BEMF检测
    bemf_detector_init();
    
    // 初始化系统变量
    system_state = SYSTEM_STATE_INIT;
    system_fault = FAULT_NONE;
    system_timer = 0;
    main_loop_counter = 0;
    
    // 初始化任务计时器
    task_timer_high = 0;
    task_timer_medium = 0;
    task_timer_low = 0;
    
    // 使能中断
    EA = 1;
    
    // 设置系统状态为待机
    system_state = SYSTEM_STATE_STANDBY;
    
    // 发送初始化完成消息
    uart_send_string("System initialized successfully\r\n");
}

// ============================================================================
// 系统状态机
// ============================================================================

void system_state_machine(void)
{
    switch (system_state) {
        case SYSTEM_STATE_INIT:
            // 初始化状态，等待初始化完成
            break;
            
        case SYSTEM_STATE_STANDBY:
            // 待机状态，等待启动命令
            break;
            
        case SYSTEM_STATE_STARTUP:
            // 启动状态，执行启动过程
            motor_startup_state_machine();
            break;
            
        case SYSTEM_STATE_RUNNING:
            // 运行状态，执行正常运行
            motor_state_machine();
            break;
            
        case SYSTEM_STATE_STOP:
            // 停止状态，执行停止过程
            motor_stop();
            system_state = SYSTEM_STATE_STANDBY;
            break;
            
        case SYSTEM_STATE_FAULT:
            // 故障状态，执行故障处理
            fault_handler();
            break;
            
        default:
            system_state = SYSTEM_STATE_INIT;
            break;
    }
}

// ============================================================================
// 任务调度
// ============================================================================

void task_scheduler(void)
{
    // 高优先级任务 (1kHz)
    if (system_timer - task_timer_high >= 1) {
        high_priority_task();
        task_timer_high = system_timer;
    }
    
    // 中优先级任务 (100Hz)
    if (system_timer - task_timer_medium >= 10) {
        medium_priority_task();
        task_timer_medium = system_timer;
    }
    
    // 低优先级任务 (10Hz)
    if (system_timer - task_timer_low >= 100) {
        low_priority_task();
        task_timer_low = system_timer;
    }
}

// ============================================================================
// 高优先级任务 (1kHz)
// ============================================================================

void high_priority_task(void)
{
    // ADC采样
    adc_start_conversion();
    
    // 电流检测
    adc_read_current();
    
    // 电压检测
    adc_read_voltage();
    
    // 温度检测
    adc_read_temperature();
    
    // 电机控制计算
    if (system_state == SYSTEM_STATE_RUNNING) {
        motor_run();
    }
    
    // BEMF检测
    bemf_detection_process();
    
    // PWM更新
    pwm_update();
}

// ============================================================================
// 中优先级任务 (100Hz)
// ============================================================================

void medium_priority_task(void)
{
    // 速度控制
    if (system_state == SYSTEM_STATE_RUNNING) {
        // 速度PID控制
        motor_state_t* state = motor_get_state();
        float speed_error = state->speed_reference - state->speed;
        float speed_output = pid_calculate(&motor_get_control()->speed_pid, 
                                         state->speed_reference, state->speed);
        
        // 更新q轴电流参考值
        state->current_reference_q = speed_output;
    }
    
    // 通信处理
    uart_process_rx();
    
    // 参数更新
    motor_parameter_display();
}

// ============================================================================
// 低优先级任务 (10Hz)
// ============================================================================

void low_priority_task(void)
{
    // 系统监控
    system_monitor();
    
    // 调试输出
    if (DEBUG_ENABLE) {
        debug_output();
    }
    
    // 状态报告
    uart_send_status();
}

// ============================================================================
// 故障处理
// ============================================================================

void fault_handler(void)
{
    // 紧急停止
    pwm_emergency_stop();
    motor_stop();
    
    // 故障诊断
    switch (system_fault) {
        case FAULT_OVER_CURRENT:
            uart_send_string("Fault: Over Current\r\n");
            break;
            
        case FAULT_OVER_VOLTAGE:
            uart_send_string("Fault: Over Voltage\r\n");
            break;
            
        case FAULT_UNDER_VOLTAGE:
            uart_send_string("Fault: Under Voltage\r\n");
            break;
            
        case FAULT_OVER_TEMP:
            uart_send_string("Fault: Over Temperature\r\n");
            break;
            
        case FAULT_BEMF_DETECTION:
            uart_send_string("Fault: BEMF Detection Failed\r\n");
            break;
            
        case FAULT_COMMUNICATION:
            uart_send_string("Fault: Communication Error\r\n");
            break;
            
        default:
            uart_send_string("Fault: Unknown Error\r\n");
            break;
    }
    
    // 等待故障清除
    // 这里可以添加故障清除逻辑
}

// ============================================================================
// 系统监控
// ============================================================================

void system_monitor(void)
{
    // 检查过流保护
    motor_state_t* state = motor_get_state();
    float current_magnitude = sqrtf(state->current.ia * state->current.ia + 
                                   state->current.ib * state->current.ib + 
                                   state->current.ic * state->current.ic);
    
    if (current_magnitude > OVER_CURRENT_THRESHOLD) {
        system_fault = FAULT_OVER_CURRENT;
        system_state = SYSTEM_STATE_FAULT;
        return;
    }
    
    // 检查过压保护
    float voltage_dc = adc_get_dc_voltage();
    if (voltage_dc > OVER_VOLTAGE_THRESHOLD) {
        system_fault = FAULT_OVER_VOLTAGE;
        system_state = SYSTEM_STATE_FAULT;
        return;
    }
    
    // 检查欠压保护
    if (voltage_dc < UNDER_VOLTAGE_THRESHOLD) {
        system_fault = FAULT_UNDER_VOLTAGE;
        system_state = SYSTEM_STATE_FAULT;
        return;
    }
    
    // 检查过温保护
    float temperature = adc_get_temperature();
    if (temperature > OVER_TEMP_THRESHOLD) {
        system_fault = FAULT_OVER_TEMP;
        system_state = SYSTEM_STATE_FAULT;
        return;
    }
    
    // 检查BEMF检测
    if (system_state == SYSTEM_STATE_RUNNING && !bemf_detection_is_valid()) {
        system_fault = FAULT_BEMF_DETECTION;
        system_state = SYSTEM_STATE_FAULT;
        return;
    }
}

// ============================================================================
// 调试输出
// ============================================================================

void debug_output(void)
{
    static uint32_t debug_counter = 0;
    
    if (++debug_counter >= 100) {  // 每秒输出一次
        debug_counter = 0;
        
        motor_state_t* state = motor_get_state();
        bemf_detection_result_t* bemf_result = bemf_get_result();
        
        // 输出系统状态
        uart_send_string("System State: ");
        switch (system_state) {
            case SYSTEM_STATE_INIT: uart_send_string("INIT"); break;
            case SYSTEM_STATE_STANDBY: uart_send_string("STANDBY"); break;
            case SYSTEM_STATE_STARTUP: uart_send_string("STARTUP"); break;
            case SYSTEM_STATE_RUNNING: uart_send_string("RUNNING"); break;
            case SYSTEM_STATE_STOP: uart_send_string("STOP"); break;
            case SYSTEM_STATE_FAULT: uart_send_string("FAULT"); break;
        }
        uart_send_string("\r\n");
        
        // 输出电机状态
        uart_send_string("Speed: ");
        uart_send_float(state->speed);
        uart_send_string(" RPM, Position: ");
        uart_send_float(state->position);
        uart_send_string(" rad\r\n");
        
        // 输出BEMF检测状态
        uart_send_string("BEMF: ");
        uart_send_float(bemf_result->estimated_speed);
        uart_send_string(" RPM, Confidence: ");
        uart_send_float(bemf_result->detection_confidence);
        uart_send_string("\r\n");
    }
}

// ============================================================================
// 中断服务程序
// ============================================================================

// 定时器中断 (1kHz)
void timer0_isr(void) interrupt 1
{
    system_timer++;
}

// ADC中断
void adc_isr(void) interrupt 5
{
    adc_conversion_complete();
}

// UART中断
void uart0_isr(void) interrupt 4
{
    uart_rx_handler();
}