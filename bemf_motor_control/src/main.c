#include "../include/bemf_config.h"
#include "../include/bemf_control.h"
#include "../include/pwm_driver.h"
#include "../include/adc_handler.h"
#include "../include/startup.h"
#include "../include/protection.h"

// 全局变量
volatile motor_state_t motor_state = MOTOR_STOP;
volatile unsigned int target_speed = 1000;  // 目标转速 (RPM)
volatile unsigned int current_speed = 0;    // 当前转速
volatile unsigned char duty_cycle = 0;      // 当前占空比
volatile fault_type_t fault_status = FAULT_NONE;

// 函数声明
void system_init(void);
void main_control_loop(void);
void speed_control_process(void);
void fault_handler(void);
void status_monitor(void);
void delay_us(unsigned int us);
void delay_ms(unsigned int ms);
void timer_init(void);
void uart_init(void);
void uart_send_string(const char* str);
void uart_send_number(unsigned int num);

// 系统初始化
void system_init(void) {
    // 关闭全局中断
    EA = 0;
    
    // 初始化PWM
    pwm_init();
    
    // 初始化ADC
    adc_init();
    
    // 初始化比较器
    comparator_init();
    
    // 初始化定时器
    timer_init();
    
    // 初始化UART (用于调试)
    uart_init();
    
    // 初始化保护功能
    protection_init();
    
    // 开启全局中断
    EA = 1;
}

// 主控制循环
void main_control_loop(void) {
    switch(motor_state) {
        case MOTOR_STOP:
            // 电机停止状态
            pwm_disable_all();
            if(target_speed > 0) {
                motor_state = MOTOR_STARTUP;
                startup_init();
            }
            break;
            
        case MOTOR_STARTUP:
            // 启动状态
            if(startup_process()) {
                // 启动完成，切换到运行状态
                motor_state = MOTOR_RUN;
                bemf_control_init();
            }
            break;
            
        case MOTOR_RUN:
            // 运行状态
            bemf_control_process();
            speed_control_process();
            break;
            
        case MOTOR_FAULT:
            // 故障状态
            pwm_disable_all();
            fault_handler();
            break;
    }
}

// 速度控制处理
void speed_control_process(void) {
    static unsigned int speed_count = 0;
    int speed_error;
    static int integral = 0;
    static int last_error = 0;
    
    // 每100ms执行一次速度控制
    if(++speed_count >= 100) {
        speed_count = 0;
        
        // 计算转速
        current_speed = calculate_speed();
        
        // PID控制
        speed_error = target_speed - current_speed;
        integral += speed_error;
        
        // 积分限幅
        if(integral > 1000) integral = 1000;
        if(integral < -1000) integral = -1000;
        
        // PID输出
        int pid_output = (speed_error * 2) + (integral / 10) + ((speed_error - last_error) * 1);
        last_error = speed_error;
        
        // 更新占空比
        duty_cycle += pid_output / 100;
        if(duty_cycle > MAX_DUTY_CYCLE) duty_cycle = MAX_DUTY_CYCLE;
        if(duty_cycle < MIN_DUTY_CYCLE) duty_cycle = MIN_DUTY_CYCLE;
        
        // 更新PWM占空比
        pwm_set_duty_cycle(duty_cycle);
    }
}

// 故障处理
void fault_handler(void) {
    static unsigned int fault_count = 0;
    
    // 检查故障是否清除
    if(check_fault_cleared()) {
        if(++fault_count > 1000) {  // 1秒后重试
            fault_count = 0;
            fault_status = FAULT_NONE;
            motor_state = MOTOR_STOP;
        }
    } else {
        fault_count = 0;
    }
}

// 主函数
void main(void) {
    // 系统初始化
    system_init();
    
    // 发送启动信息
    uart_send_string("BEMF Motor Control System Started\r\n");
    
    // 主循环
    while(1) {
        // 保护检查
        if(protection_check()) {
            motor_state = MOTOR_FAULT;
        }
        
        // 主控制循环
        main_control_loop();
        
        // 状态监控和通信
        status_monitor();
        
        // 短暂延时
        delay_us(100);
    }
}

// 状态监控
void status_monitor(void) {
    static unsigned int monitor_count = 0;
    
    // 每1秒发送一次状态信息
    if(++monitor_count >= 10000) {
        monitor_count = 0;
        
        // 发送状态信息
        uart_send_string("Speed: ");
        uart_send_number(current_speed);
        uart_send_string(" RPM, Duty: ");
        uart_send_number(duty_cycle);
        uart_send_string("%, State: ");
        uart_send_number(motor_state);
        uart_send_string("\r\n");
    }
}

// 定时器初始化
void timer_init(void) {
    // 配置Timer0用于系统时钟
    TMOD |= 0x01;  // Timer0, Mode 1 (16-bit)
    TH0 = 0xFC;    // 1ms @ 12MHz
    TL0 = 0x18;
    ET0 = 1;       // 使能Timer0中断
    TR0 = 1;       // 启动Timer0
}

// UART初始化
void uart_init(void) {
    SCON = 0x50;   // 8-bit, variable baud rate
    TMOD |= 0x20;  // Timer1, Mode 2 (8-bit auto-reload)
    TH1 = 0xFD;    // 9600 baud @ 11.0592MHz
    TL1 = 0xFD;
    TR1 = 1;       // 启动Timer1
    ES = 1;        // 使能串口中断
}

// UART发送字符串
void uart_send_string(const char* str) {
    while(*str) {
        SBUF = *str;
        while(!TI);
        TI = 0;
        str++;
    }
}

// UART发送数字
void uart_send_number(unsigned int num) {
    char buffer[8];
    unsigned char i = 0;
    
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

// 延时函数 (微秒)
void delay_us(unsigned int us) {
    unsigned int i;
    for(i = 0; i < us; i++) {
        _nop_();
        _nop_();
        _nop_();
        _nop_();
    }
}

// 延时函数 (毫秒)
void delay_ms(unsigned int ms) {
    unsigned int i;
    for(i = 0; i < ms; i++) {
        delay_us(1000);
    }
}

// Timer0中断服务程序
void timer0_isr(void) interrupt 1 {
    TH0 = 0xFC;  // 重装载
    TL0 = 0x18;
    // 系统时钟处理
}

// 串口中断服务程序
void uart_isr(void) interrupt 4 {
    if(RI) {
        RI = 0;
        // 接收数据处理
    }
    if(TI) {
        TI = 0;
    }
}