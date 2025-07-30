#include "bemf_sensorless.h"
#include "mcu_config.h"
#include <util/delay.h>

// Application configuration
#define MOTOR_START_SPEED    800   // Initial target speed
#define MOTOR_MAX_SPEED      1800  // Maximum speed
#define SPEED_RAMP_STEP      50    // Speed increment per step
#define SPEED_RAMP_DELAY     500   // Delay between speed changes (ms)

// Global variables
volatile uint8_t motor_running = 0;
volatile uint16_t target_speed = MOTOR_START_SPEED;

/**
 * Simple button debounce function
 */
uint8_t button_pressed(void) {
    static uint8_t button_state = 0;
    static uint8_t last_button = 0;
    static uint32_t debounce_time = 0;
    
    uint8_t current_button = !(PINC & (1 << PC2)); // Active low button
    uint32_t current_time = get_system_tick();
    
    if (current_button != last_button) {
        debounce_time = current_time;
    }
    
    if ((current_time - debounce_time) > 50) { // 50ms debounce
        if (current_button != button_state) {
            button_state = current_button;
            if (button_state) {
                last_button = current_button;
                return 1; // Button pressed
            }
        }
    }
    
    last_button = current_button;
    return 0;
}

/**
 * Initialize application-specific hardware
 */
void app_init(void) {
    // Configure button input (PC2)
    DDRC &= ~(1 << PC2);    // Input
    PORTC |= (1 << PC2);    // Enable pull-up
    
    // Configure LED output for status indication (PC3)
    DDRC |= (1 << PC3);     // Output
    PORTC &= ~(1 << PC3);   // Initially off
    
    // Configure speed control potentiometer (optional, PC4)
    DDRC &= ~(1 << PC4);    // Input for ADC
}

/**
 * Read speed control potentiometer (if available)
 */
uint16_t read_speed_control(void) {
    // Start ADC conversion on PC4
    ADMUX = (1 << REFS0) | 0x04; // AVCC reference, ADC4
    ADCSRA |= (1 << ADSC);       // Start conversion
    
    // Wait for conversion to complete
    while (ADCSRA & (1 << ADSC));
    
    // Convert ADC result to speed (0-1023 -> MIN_PWM_DUTY to MAX_PWM_DUTY)
    uint16_t adc_value = ADC;
    uint16_t speed = MIN_PWM_DUTY + 
                    ((uint32_t)adc_value * (MAX_PWM_DUTY - MIN_PWM_DUTY)) / 1023;
    
    return speed;
}

/**
 * Update status LED based on motor state
 */
void update_status_led(void) {
    static uint32_t last_blink_time = 0;
    static uint8_t led_state = 0;
    uint32_t current_time = get_system_tick();
    
    if (motor_running) {
        // Fast blink when running
        if (current_time - last_blink_time > 200) {
            led_state = !led_state;
            if (led_state) {
                PORTC |= (1 << PC3);   // LED on
            } else {
                PORTC &= ~(1 << PC3);  // LED off
            }
            last_blink_time = current_time;
        }
    } else {
        // Slow blink when stopped
        if (current_time - last_blink_time > 1000) {
            led_state = !led_state;
            if (led_state) {
                PORTC |= (1 << PC3);   // LED on
            } else {
                PORTC &= ~(1 << PC3);  // LED off
            }
            last_blink_time = current_time;
        }
    }
}

/**
 * Speed ramping function for smooth acceleration
 */
void handle_speed_ramping(void) {
    static uint32_t last_ramp_time = 0;
    uint32_t current_time = get_system_tick();
    
    if (motor_running && (current_time - last_ramp_time > SPEED_RAMP_DELAY)) {
        // Read speed control input (potentiometer)
        uint16_t desired_speed = read_speed_control();
        
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
        
        last_ramp_time = current_time;
    }
}

/**
 * Emergency stop function
 */
void emergency_stop(void) {
    motor_stop();
    motor_running = 0;
    target_speed = 0;
    
    // Turn on LED solid for emergency indication
    PORTC |= (1 << PC3);
    
    // Wait for button release
    while (!(PINC & (1 << PC2))) {
        _delay_ms(10);
    }
    
    // Turn off LED
    PORTC &= ~(1 << PC3);
}

/**
 * Main application function
 */
int main(void) {
    // Initialize MCU
    mcu_init();
    
    // Initialize application hardware
    app_init();
    
    // Initialize motor control system
    motor_init();
    
    // Startup delay
    _delay_ms(1000);
    
    // Main application loop
    while (1) {
        // Handle button press for start/stop
        if (button_pressed()) {
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
        }
        
        // Run motor control task
        motor_control_task();
        
        // Handle speed ramping
        handle_speed_ramping();
        
        // Update status LED
        update_status_led();
        
        // Check for emergency conditions
        // Example: overcurrent, overvoltage, etc.
        // For now, just check if motor has stalled
        static uint32_t last_speed_check = 0;
        uint32_t current_time = get_system_tick();
        
        if (motor_running && (current_time - last_speed_check > 5000)) {
            // Check if motor is actually running
            // If BEMF detection fails for too long, stop motor
            if (get_system_tick() - motor.bemf.last_zero_cross_time > ZERO_CROSS_TIMEOUT) {
                emergency_stop();
            }
            last_speed_check = current_time;
        }
        
        // Small delay to prevent excessive CPU usage
        _delay_ms(1);
    }
    
    return 0;
}

/**
 * Example of advanced features that can be added:
 */

#ifdef ADVANCED_FEATURES

/**
 * Motor diagnostics function
 */
void motor_diagnostics(void) {
    // Read motor parameters
    uint16_t current_rpm = motor.current_speed;
    uint16_t target_rpm = motor.target_speed;
    uint32_t commutation_period = motor.bemf.commutation_period;
    
    // Calculate efficiency, power consumption, etc.
    // This would require additional sensors and calculations
}

/**
 * Adaptive timing adjustment
 */
void adaptive_timing(void) {
    // Adjust commutation timing based on motor performance
    // This can improve efficiency and reduce noise
    
    static uint32_t performance_history[10];
    static uint8_t history_index = 0;
    
    // Store performance metrics
    performance_history[history_index] = motor.bemf.commutation_period;
    history_index = (history_index + 1) % 10;
    
    // Analyze trends and adjust timing accordingly
    // Implementation would depend on specific motor characteristics
}

/**
 * Temperature compensation
 */
void temperature_compensation(void) {
    // Adjust motor parameters based on temperature
    // BEMF characteristics change with temperature
    
    // Read temperature sensor (if available)
    // Adjust comparator thresholds or timing accordingly
}

#endif // ADVANCED_FEATURES