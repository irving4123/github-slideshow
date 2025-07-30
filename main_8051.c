#include "bemf_8051.h"

// Application configuration
#define MOTOR_START_SPEED    400   // Initial target speed
#define MOTOR_MAX_SPEED      800   // Maximum speed
#define SPEED_RAMP_STEP      20    // Speed increment per step
#define SPEED_RAMP_DELAY     100   // Delay between speed changes (ms)

// GPIO pin definitions for user interface
sbit START_BUTTON = P1^0;    // Start/Stop button (active low)
sbit STATUS_LED = P1^1;      // Status LED
sbit FAULT_LED = P1^2;       // Fault indicator LED
sbit SPEED_POT_EN = P1^3;    // Speed potentiometer enable

// Global variables
volatile bit motor_running = 0;
volatile bit button_pressed = 0;
volatile unsigned int target_speed = MOTOR_START_SPEED;
volatile unsigned char led_blink_counter = 0;

// Function prototypes
void system_init(void);
void user_interface_task(void);
void button_scan(void);
void update_status_leds(void);
unsigned int read_speed_potentiometer(void);
void handle_speed_control(void);
void fault_handler(void);
void delay_ms(unsigned int ms);

/**
 * Main application function
 */
void main(void) {
    // Initialize system
    system_init();
    
    // Startup delay
    delay_ms(500);
    
    // Main application loop
    while (1) {
        // Handle user interface
        user_interface_task();
        
        // Run motor control task
        motor_control_task();
        
        // Handle speed control
        handle_speed_control();
        
        // Update status indicators
        update_status_leds();
        
        // Check for fault conditions
        if (motor.fault_flags != 0) {
            fault_handler();
        }
        
        // Small delay to prevent excessive CPU usage
        delay_ms(10);
    }
}

/**
 * Initialize system for enhanced 8051
 */
void system_init(void) {
    // Configure I/O ports
    P1 = 0xFF;  // Set P1 as inputs with pull-ups
    P2 = 0x00;  // Set P2 as outputs (for PWM)
    P3 = 0xFF;  // Set P3 for UART and external interrupts
    
    // Configure user interface pins
    START_BUTTON = 1;   // Enable pull-up for button
    STATUS_LED = 0;     // LED off initially
    FAULT_LED = 0;      // Fault LED off
    SPEED_POT_EN = 1;   // Enable speed potentiometer
    
    // Initialize motor control system
    motor_init();
    
    // Initialize variables
    motor_running = 0;
    button_pressed = 0;
    target_speed = MOTOR_START_SPEED;
    led_blink_counter = 0;
    
    // Configure external interrupt for button (optional)
    IT0 = 1;    // Edge triggered interrupt
    EX0 = 1;    // Enable external interrupt 0
}

/**
 * User interface task
 */
void user_interface_task(void) {
    static unsigned long last_ui_time = 0;
    unsigned long current_time = get_system_tick();
    
    // Run UI task every 50ms
    if (current_time - last_ui_time > 50) {
        button_scan();
        last_ui_time = current_time;
    }
    
    // Handle button press
    if (button_pressed) {
        button_pressed = 0;
        
        if (!motor_running) {
            // Start motor
            motor_running = 1;
            target_speed = MOTOR_START_SPEED;
            motor_set_speed(target_speed);
            motor_start();
        } else {
            // Stop motor
            motor_stop();
            motor_running = 0;
            target_speed = 0;
        }
        
        // Button debounce delay
        delay_ms(200);
    }
}

/**
 * Button scanning with debounce
 */
void button_scan(void) {
    static unsigned char button_state = 1;  // Released state
    static unsigned char debounce_counter = 0;
    
    unsigned char current_button = START_BUTTON;
    
    if (current_button != button_state) {
        debounce_counter++;
        if (debounce_counter >= 3) {  // 3 * 50ms = 150ms debounce
            button_state = current_button;
            debounce_counter = 0;
            
            if (button_state == 0) {  // Button pressed (active low)
                button_pressed = 1;
            }
        }
    } else {
        debounce_counter = 0;
    }
}

/**
 * Update status LEDs
 */
void update_status_leds(void) {
    static unsigned char blink_counter = 0;
    
    blink_counter++;
    
    if (motor.fault_flags != 0) {
        // Fault condition - solid fault LED
        FAULT_LED = 1;
        STATUS_LED = 0;
    } else {
        FAULT_LED = 0;
        
        if (motor_running) {
            // Fast blink when running (every 10 cycles * 10ms = 100ms)
            if (blink_counter >= 10) {
                STATUS_LED = !STATUS_LED;
                blink_counter = 0;
            }
        } else {
            // Slow blink when stopped (every 50 cycles * 10ms = 500ms)
            if (blink_counter >= 50) {
                STATUS_LED = !STATUS_LED;
                blink_counter = 0;
            }
        }
    }
}

/**
 * Read speed potentiometer using 12-bit ADC
 */
unsigned int read_speed_potentiometer(void) {
    unsigned int adc_value;
    
    // Select ADC channel for speed potentiometer (e.g., channel 0)
    ADC_CFG = (ADC_CFG & 0xF0) | 0x00;  // Select channel 0
    
    // Start ADC conversion
    ADC_CTRL |= 0x02;  // Start conversion bit
    
    // Wait for conversion complete
    while (ADC_CTRL & 0x02) {
        // Wait for conversion to finish
    }
    
    // Read ADC result
    adc_value = (ADC_DATA_H << 8) | ADC_DATA_L;
    
    // Convert to speed range (0-4095 -> MIN_PWM_DUTY to MAX_PWM_DUTY)
    return MIN_PWM_DUTY + HW_MUL(adc_value, (MAX_PWM_DUTY - MIN_PWM_DUTY)) / 4095;
}

/**
 * Handle speed control with ramping
 */
void handle_speed_control(void) {
    static unsigned long last_speed_update = 0;
    unsigned long current_time = get_system_tick();
    
    if (!motor_running) {
        return;
    }
    
    // Update speed every SPEED_RAMP_DELAY ms
    if (current_time - last_speed_update > SPEED_RAMP_DELAY) {
        // Read desired speed from potentiometer
        unsigned int desired_speed = read_speed_potentiometer();
        
        // Gradually adjust target speed towards desired speed
        if (target_speed < desired_speed) {
            target_speed += SPEED_RAMP_STEP;
            if (target_speed > desired_speed) {
                target_speed = desired_speed;
            }
        } else if (target_speed > desired_speed) {
            target_speed -= SPEED_RAMP_STEP;
            if (target_speed < desired_speed) {
                target_speed = desired_speed;
            }
        }
        
        // Limit speed to maximum
        if (target_speed > MOTOR_MAX_SPEED) {
            target_speed = MOTOR_MAX_SPEED;
        }
        
        // Apply new target speed
        motor_set_speed(target_speed);
        
        last_speed_update = current_time;
    }
}

/**
 * Fault handler
 */
void fault_handler(void) {
    static unsigned long fault_start_time = 0;
    unsigned long current_time = get_system_tick();
    
    // Record fault start time
    if (fault_start_time == 0) {
        fault_start_time = current_time;
    }
    
    // Stop motor immediately on fault
    if (motor_running) {
        emergency_brake();
        motor_running = 0;
    }
    
    // Auto-recovery after 5 seconds if no critical faults
    if ((current_time - fault_start_time > 5000) && 
        !(motor.fault_flags & (FAULT_OVERCURRENT | FAULT_OVERVOLTAGE))) {
        
        // Clear non-critical faults
        motor.fault_flags &= (FAULT_OVERCURRENT | FAULT_OVERVOLTAGE);
        
        if (motor.fault_flags == 0) {
            fault_start_time = 0;  // Reset fault timer
        }
    }
}

/**
 * Millisecond delay function optimized for 48MHz
 */
void delay_ms(unsigned int ms) {
    unsigned int i, j;
    
    for (i = 0; i < ms; i++) {
        // Approximately 1ms delay at 48MHz
        // Adjust loop count based on actual timing measurements
        for (j = 0; j < 2000; j++) {
            _nop_();  // No operation
        }
    }
}

/**
 * External interrupt 0 service routine (button interrupt)
 */
void ext0_isr(void) interrupt 0 {
    // Button interrupt - set flag for main loop processing
    // Actual debouncing is done in main loop
    static unsigned long last_interrupt_time = 0;
    unsigned long current_time = get_system_tick();
    
    // Simple interrupt-level debouncing
    if (current_time - last_interrupt_time > 200) {
        button_pressed = 1;
        last_interrupt_time = current_time;
    }
}

/**
 * UART communication for debugging (optional)
 */
#ifdef ENABLE_UART_DEBUG

void uart_init(void) {
    // Configure UART0 for 9600 baud at 48MHz
    SCON = 0x50;    // 8-bit, variable baud rate
    TMOD |= 0x20;   // Timer1 mode 2 (8-bit auto-reload)
    TH1 = 0xFA;     // 9600 baud at 48MHz
    TR1 = 1;        // Start Timer1
    TI = 1;         // Set transmit interrupt flag
}

void uart_send_char(unsigned char c) {
    SBUF = c;
    while (!TI);
    TI = 0;
}

void uart_send_string(unsigned char *str) {
    while (*str) {
        uart_send_char(*str++);
    }
}

void uart_send_hex(unsigned int value) {
    unsigned char hex_digits[] = "0123456789ABCDEF";
    uart_send_char(hex_digits[(value >> 12) & 0x0F]);
    uart_send_char(hex_digits[(value >> 8) & 0x0F]);
    uart_send_char(hex_digits[(value >> 4) & 0x0F]);
    uart_send_char(hex_digits[value & 0x0F]);
}

void debug_print_motor_status(void) {
    uart_send_string("Motor State: ");
    uart_send_hex(motor.state);
    uart_send_string(" Speed: ");
    uart_send_hex(motor.current_speed);
    uart_send_string(" PWM: ");
    uart_send_hex(motor.pwm_duty);
    uart_send_string(" Faults: ");
    uart_send_hex(motor.fault_flags);
    uart_send_string("\r\n");
}

#endif // ENABLE_UART_DEBUG

/**
 * Advanced features for enhanced performance
 */

/**
 * Temperature monitoring using internal temperature sensor
 */
unsigned int read_temperature(void) {
    unsigned int temp_value;
    
    // Select internal temperature sensor channel
    ADC_CFG = (ADC_CFG & 0xF0) | 0x0E;  // Internal temp sensor
    
    // Start ADC conversion
    ADC_CTRL |= 0x02;
    
    // Wait for conversion
    while (ADC_CTRL & 0x02);
    
    // Read temperature value
    temp_value = (ADC_DATA_H << 8) | ADC_DATA_L;
    
    // Convert to temperature (implementation depends on MCU specifications)
    // Typically: Temp_C = (ADC_Value - Offset) / Gain
    return temp_value;
}

/**
 * Power monitoring using ADC
 */
void monitor_power_supply(void) {
    static unsigned long last_monitor_time = 0;
    unsigned long current_time = get_system_tick();
    
    if (current_time - last_monitor_time > 1000) {  // Every 1 second
        unsigned int vdd_voltage;
        
        // Select VDD monitoring channel
        ADC_CFG = (ADC_CFG & 0xF0) | 0x0F;  // VDD monitor
        
        // Start conversion
        ADC_CTRL |= 0x02;
        while (ADC_CTRL & 0x02);
        
        vdd_voltage = (ADC_DATA_H << 8) | ADC_DATA_L;
        
        // Check for overvoltage/undervoltage
        if (vdd_voltage > 3700 || vdd_voltage < 2200) {  // Adjust thresholds
            motor.fault_flags |= FAULT_OVERVOLTAGE;
        }
        
        last_monitor_time = current_time;
    }
}

/**
 * Performance optimization using hardware accelerators
 */
void optimize_commutation_timing(void) {
    // Use hardware multiplier for precise timing calculations
    unsigned long optimal_delay;
    
    if (motor.bemf.commutation_period > 0) {
        // Calculate optimal commutation advance using hardware math
        optimal_delay = HW_MUL(motor.bemf.commutation_period, 30) / 180;  // 30° advance
        
        // Apply timing compensation based on speed
        if (motor.current_speed > 500) {
            optimal_delay = HW_MUL(optimal_delay, 90) / 100;  // 10% advance at high speed
        }
    }
}