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
    
    uart_send_string("BEMF Motor Control System - Basic Example\r\n");
    uart_send_string("========================================\r\n");
    
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
    
    // 4. 保护功能测试
    uart_send_string("Step 4: Protection function test\r\n");
    example_protection_test();
    
    // 5. 安全停机
    uart_send_string("Step 5: Safe shutdown\r\n");
    target_speed = 0;
    delay_ms(3000);  // 等待电机停止
    pwm_disable_all();
    example_running = 0;
    
    uart_send_string("Example completed successfully\r\n");
    
    // 程序结束后进入无限循环
    while(1) {
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