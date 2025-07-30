/*
 * BEMF无传感器电机控制系统 - 基本使用示例
 * 
 * 本示例展示了如何使用BEMF电机控制系统的基本功能：
 * 1. 系统初始化
 * 2. 电机启动
 * 3. 速度控制
 * 4. 状态监控
 * 5. 故障处理
 */

#include "../include/bemf_config.h"
#include "../include/bemf_control.h"
#include "../include/pwm_driver.h"
#include "../include/adc_handler.h"
#include "../include/startup.h"
#include "../include/protection.h"

// 示例程序全局变量
volatile unsigned int example_target_speed = 1500;  // 目标转速1500RPM
volatile unsigned char example_running = 0;         // 运行标志
volatile unsigned int example_runtime = 0;          // 运行时间计数

// 示例1: 基本初始化和启动
void example_basic_startup(void) {
    // 1. 系统初始化
    system_init();
    
    // 2. 设置目标转速
    target_speed = example_target_speed;
    
    // 3. 启动电机
    motor_state = MOTOR_STARTUP;
    startup_init();
    
    // 4. 等待启动完成
    while(!startup_is_complete()) {
        // 执行启动过程
        startup_process();
        
        // 检查保护
        if(protection_check()) {
            // 启动失败，处理故障
            uart_send_string("Startup failed - Protection triggered\r\n");
            return;
        }
        
        // 延时1ms
        delay_ms(1);
    }
    
    uart_send_string("Motor started successfully\r\n");
    example_running = 1;
}

// 示例2: 速度控制演示
void example_speed_control_demo(void) {
    unsigned int speed_sequence[] = {1000, 1500, 2000, 2500, 2000, 1500, 1000};
    unsigned char speed_index = 0;
    unsigned int speed_timer = 0;
    
    uart_send_string("Starting speed control demo\r\n");
    
    while(example_running && speed_index < 7) {
        // 每5秒切换一次速度
        if(speed_timer >= 5000) {
            target_speed = speed_sequence[speed_index];
            
            uart_send_string("Target speed: ");
            uart_send_number(target_speed);
            uart_send_string(" RPM\r\n");
            
            speed_index++;
            speed_timer = 0;
        }
        
        // 主控制循环
        main_control_loop();
        
        speed_timer++;
        delay_ms(1);
    }
    
    uart_send_string("Speed control demo completed\r\n");
}

// 示例3: 保护功能测试
void example_protection_test(void) {
    protection_thresholds_t test_thresholds;
    
    uart_send_string("Testing protection functions\r\n");
    
    // 获取当前保护阈值
    protection_get_thresholds(&test_thresholds);
    
    // 临时降低过流阈值进行测试
    unsigned int original_current_limit = test_thresholds.max_current_ma;
    protection_set_current_limit(1000);  // 设置为1A
    
    uart_send_string("Overcurrent protection test - limit set to 1A\r\n");
    
    // 逐步增加占空比直到触发过流保护
    unsigned char test_duty = 10;
    while(test_duty <= 80 && !protection_has_fault()) {
        pwm_set_duty_cycle(test_duty);
        
        uart_send_string("Duty cycle: ");
        uart_send_number(test_duty);
        uart_send_string("%, Current: ");
        uart_send_number(adc_read_current_ma());
        uart_send_string(" mA\r\n");
        
        test_duty += 5;
        delay_ms(500);
    }
    
    if(protection_has_fault()) {
        uart_send_string("Overcurrent protection triggered successfully\r\n");
        
        // 显示故障信息
        protection_type_t fault = protection_get_active_fault();
        uart_send_string("Fault type: ");
        uart_send_number(fault);
        uart_send_string("\r\n");
        
        // 清除故障
        protection_clear_all_faults();
    }
    
    // 恢复原始电流限制
    protection_set_current_limit(original_current_limit);
    
    uart_send_string("Protection test completed\r\n");
}

// 示例4: 参数调整演示
void example_parameter_adjustment(void) {
    startup_params_t startup_params;
    protection_config_t protection_config;
    
    uart_send_string("Parameter adjustment demo\r\n");
    
    // 1. 调整启动参数
    startup_get_params(&startup_params);
    
    uart_send_string("Original startup parameters:\r\n");
    uart_send_string("Initial duty: ");
    uart_send_number(startup_params.initial_duty);
    uart_send_string("%\r\n");
    uart_send_string("Ramp time: ");
    uart_send_number(startup_params.ramp_time_ms);
    uart_send_string(" ms\r\n");
    
    // 修改启动参数
    startup_params.initial_duty = 15;        // 降低初始占空比
    startup_params.ramp_time_ms = 150;       // 增加斜坡时间
    startup_set_params(&startup_params);
    
    uart_send_string("Updated startup parameters\r\n");
    
    // 2. 调整保护配置
    protection_get_config(&protection_config);
    
    // 临时禁用过温保护
    protection_config.overtemp_enabled = 0;
    protection_set_config(&protection_config);
    
    uart_send_string("Overtemperature protection disabled\r\n");
    
    // 3. 调整BEMF检测参数
    // 这里可以通过修改全局变量或配置寄存器来调整BEMF参数
    
    uart_send_string("Parameter adjustment completed\r\n");
}

// 示例5: 状态监控和数据记录
void example_status_monitoring(void) {
    unsigned int monitor_count = 0;
    unsigned int max_current = 0;
    unsigned int min_voltage = 65535;
    unsigned int max_temperature = 0;
    
    uart_send_string("Status monitoring started\r\n");
    
    while(example_running && monitor_count < 300) {  // 监控5分钟
        // 读取当前状态
        unsigned int current_rpm = calculate_speed();
        unsigned int current_ma = adc_read_current_ma();
        unsigned int voltage_mv = adc_read_voltage_mv();
        unsigned int temperature_c = adc_read_temperature_c();
        unsigned char duty = pwm_get_duty_cycle();
        
        // 更新统计数据
        if(current_ma > max_current) max_current = current_ma;
        if(voltage_mv < min_voltage) min_voltage = voltage_mv;
        if(temperature_c > max_temperature) max_temperature = temperature_c;
        
        // 每10秒输出一次状态
        if(monitor_count % 10 == 0) {
            uart_send_string("Status - Speed: ");
            uart_send_number(current_rpm);
            uart_send_string(" RPM, Current: ");
            uart_send_number(current_ma);
            uart_send_string(" mA, Voltage: ");
            uart_send_number(voltage_mv / 1000);
            uart_send_string(" V, Temp: ");
            uart_send_number(temperature_c);
            uart_send_string(" C, Duty: ");
            uart_send_number(duty);
            uart_send_string("%\r\n");
        }
        
        // 检查异常状态
        if(protection_has_fault()) {
            uart_send_string("Fault detected during monitoring\r\n");
            break;
        }
        
        monitor_count++;
        delay_ms(1000);  // 1秒间隔
    }
    
    // 输出统计结果
    uart_send_string("Monitoring completed. Statistics:\r\n");
    uart_send_string("Max current: ");
    uart_send_number(max_current);
    uart_send_string(" mA\r\n");
    uart_send_string("Min voltage: ");
    uart_send_number(min_voltage / 1000);
    uart_send_string(" V\r\n");
    uart_send_string("Max temperature: ");
    uart_send_number(max_temperature);
    uart_send_string(" C\r\n");
}

// 示例6: 完整的应用程序流程
void example_complete_application(void) {
    uart_send_string("BEMF Motor Control System - Complete Example\r\n");
    uart_send_string("===========================================\r\n");
    
    // 1. 基本启动
    uart_send_string("Step 1: Basic startup\r\n");
    example_basic_startup();
    
    if(!example_running) {
        uart_send_string("Startup failed, exiting\r\n");
        return;
    }
    
    // 2. 短暂运行以稳定系统
    uart_send_string("Step 2: System stabilization\r\n");
    delay_ms(2000);
    
    // 3. 速度控制演示
    uart_send_string("Step 3: Speed control demonstration\r\n");
    example_speed_control_demo();
    
    // 4. 参数调整
    uart_send_string("Step 4: Parameter adjustment\r\n");
    example_parameter_adjustment();
    
    // 5. 保护功能测试
    uart_send_string("Step 5: Protection function test\r\n");
    example_protection_test();
    
    // 6. 状态监控
    uart_send_string("Step 6: Status monitoring\r\n");
    example_status_monitoring();
    
    // 7. 安全停机
    uart_send_string("Step 7: Safe shutdown\r\n");
    target_speed = 0;
    delay_ms(3000);  // 等待电机停止
    pwm_disable_all();
    example_running = 0;
    
    uart_send_string("Example completed successfully\r\n");
}

// 错误处理回调函数
void example_protection_callback(protection_type_t type) {
    uart_send_string("Protection callback - Fault type: ");
    uart_send_number(type);
    uart_send_string("\r\n");
    
    switch(type) {
        case PROTECTION_OVERCURRENT:
            uart_send_string("Overcurrent protection activated\r\n");
            break;
        case PROTECTION_OVERVOLTAGE:
            uart_send_string("Overvoltage protection activated\r\n");
            break;
        case PROTECTION_OVERTEMP:
            uart_send_string("Overtemperature protection activated\r\n");
            break;
        case PROTECTION_STALL:
            uart_send_string("Stall protection activated\r\n");
            break;
        default:
            uart_send_string("Unknown protection type\r\n");
            break;
    }
    
    // 设置停止标志
    example_running = 0;
}

// 主函数 - 选择要运行的示例
void main(void) {
    // 注册保护回调函数
    protection_register_callback(example_protection_callback);
    
    // 运行完整示例
    example_complete_application();
    
    // 程序结束后进入无限循环
    while(1) {
        // 可以在这里添加其他功能或进入低功耗模式
        delay_ms(1000);
    }
}

// UART发送字符串函数 (需要根据实际硬件实现)
void uart_send_string(const char* str) {
    while(*str) {
        // SBUF = *str;  // 发送字符到UART
        // while(!TI);   // 等待发送完成
        // TI = 0;       // 清除发送标志
        str++;
    }
}

// UART发送数字函数
void uart_send_number(unsigned int num) {
    char buffer[8];
    unsigned char i = 0;
    
    // 转换数字为字符串
    if(num == 0) {
        buffer[i++] = '0';
    } else {
        while(num > 0) {
            buffer[i++] = '0' + (num % 10);
            num /= 10;
        }
    }
    
    // 反转字符串
    unsigned char j;
    for(j = 0; j < i/2; j++) {
        char temp = buffer[j];
        buffer[j] = buffer[i-1-j];
        buffer[i-1-j] = temp;
    }
    
    buffer[i] = '\0';
    uart_send_string(buffer);
}