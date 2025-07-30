#include "bemf_8051.h"

// Global motor control instance in external RAM for better performance
motor_control_t xdata motor;
volatile unsigned long xdata system_tick = 0;
volatile unsigned char xdata bemf_blank_counter = 0;

// Commutation table for 6-step control using PWM1 array
// Format: [step][phase] = {A_enable, B_enable, C_enable}
// 0=floating, 1=PWM high-side, 2=PWM low-side
code unsigned char commutation_table[6][3] = {
    {1, 2, 0},  // Step 1: A+, B-, C floating
    {1, 0, 2},  // Step 2: A+, B floating, C-
    {0, 1, 2},  // Step 3: A floating, B+, C-
    {2, 1, 0},  // Step 4: A-, B+, C floating
    {2, 0, 1},  // Step 5: A-, B floating, C+
    {0, 2, 1}   // Step 6: A floating, B-, C+
};

// BEMF detection table - which phase to monitor for each step
// 0=Phase A, 1=Phase B, 2=Phase C
code unsigned char bemf_monitor_table[6] = {
    2, 1, 0, 2, 1, 0  // Monitor floating phase for zero crossing
};

// Comparator input multiplexer settings for BEMF detection
code unsigned char bemf_cmp_mux[3] = {
    0x01,  // Phase A BEMF input
    0x02,  // Phase B BEMF input  
    0x03   // Phase C BEMF input
};

/**
 * Initialize motor control system for enhanced 8051
 */
void motor_init(void) {
    // Initialize motor control structure
    motor.state = MOTOR_STOP;
    motor.current_step = STEP_1;
    motor.pwm_duty = 0;
    motor.target_speed = 0;
    motor.current_speed = 0;
    motor.startup_counter = 0;
    motor.fault_flags = 0;
    
    // Initialize BEMF detector
    motor.bemf.cmp0_state = 0;
    motor.bemf.cmp1_state = 0;
    motor.bemf.zero_cross_detected = 0;
    motor.bemf.last_zero_cross_time = 0;
    motor.bemf.commutation_period = 1000;
    motor.bemf.floating_phase = 2; // Start with C floating
    
    // Configure system clock to 48MHz internal RC
    // Most enhanced 8051s auto-configure to max speed on reset
    
    // Configure Timer0 for system tick (1ms)
    TMOD &= 0xF0;    // Clear Timer0 mode bits
    TMOD |= 0x01;    // Timer0 mode 1 (16-bit)
    TH0 = 0x3C;      // Load for 1ms at 48MHz (48000 cycles)
    TL0 = 0xB0;      // 65536 - 48000 = 17536 = 0x4480, but need 0x3CB0
    ET0 = 1;         // Enable Timer0 interrupt
    TR0 = 1;         // Start Timer0
    
    // Configure PWM1 for 3-phase motor control
    configure_pwm1();
    
    // Configure analog comparators for BEMF detection
    configure_comparators();
    
    // Configure ADC for monitoring (optional)
    configure_adc();
    
    // Enable global interrupts
    EA = 1;
}

/**
 * Configure PWM1 array for 3-phase motor control
 */
void configure_pwm1(void) {
    // Set PWM1 period for 20kHz at 48MHz
    PWM1_PERIOD_L = (PWM_PERIOD - 1) & 0xFF;
    PWM1_PERIOD_H = ((PWM_PERIOD - 1) >> 8) & 0xFF;
    
    // Configure dead time (1μs at 48MHz = 48 clock cycles)
    PWM1_DT = 48;
    
    // Configure PWM1 for complementary mode with center alignment
    PWM1_CFG = PWM1_COMP_MODE | PWM1_CENTER | PWM1_RELOAD_EN | PWM1_BRAKE_EN;
    
    // Initialize all duty cycles to 0
    SET_PWM1A_DUTY(0);
    SET_PWM1B_DUTY(0);
    SET_PWM1C_DUTY(0);
    
    // Disable all PWM outputs initially
    PWM1_OE = 0x00;
    
    // Enable PWM1 with brake protection
    PWM1_CTRL = PWM1_EN | PWM1_BRAKE_EN;
    
    // Enable PWM1 period match interrupt for timing
    // This depends on specific MCU implementation
    // Typically: PWM1_INT_EN = 1;
}

/**
 * Configure analog comparators for BEMF detection
 */
void configure_comparators(void) {
    // Configure CMP0 for BEMF detection
    // Enable comparator with hysteresis and digital filter
    CMP0_CTRL = CMP_EN | CMP_HYST_EN | CMP_FILT_EN | CMP_INT_EN;
    
    // Configure CMP1 for secondary BEMF detection or overcurrent
    CMP1_CTRL = CMP_EN | CMP_HYST_EN | CMP_FILT_EN;
    
    // Set initial input multiplexer (will be changed during operation)
    CMP0_MUX = bemf_cmp_mux[motor.bemf.floating_phase];
    CMP1_MUX = 0x00; // Reference to neutral or ground
    
    // Enable comparator interrupts
    // This depends on specific MCU interrupt structure
}

/**
 * Configure 12-bit ADC for monitoring
 */
void configure_adc(void) {
    // Configure ADC for 12-bit resolution
    // Use VDD as reference, enable temperature sensor
    ADC_CFG = 0x0F;  // 12-bit mode, VDD reference
    
    // Enable ADC with auto-trigger from PWM1
    ADC_CTRL = 0x01; // Enable ADC
    
    // Set ADC compare values for overcurrent/overvoltage protection
    ADC_CMP_L = 0x00;  // Low threshold
    ADC_CMP_H = 0x0F;  // High threshold (adjust based on application)
}

/**
 * Start motor operation
 */
void motor_start(void) {
    if (motor.state == MOTOR_STOP) {
        motor.state = MOTOR_STARTUP;
        motor.startup_counter = 0;
        motor.current_step = STEP_1;
        motor.pwm_duty = STARTUP_PWM_DUTY;
        motor.fault_flags = 0;
        
        // Start with first commutation step
        commutate_motor();
    }
}

/**
 * Stop motor operation
 */
void motor_stop(void) {
    motor.state = MOTOR_STOP;
    motor.pwm_duty = 0;
    
    // Disable all PWM outputs
    PWM1_OE = 0x00;
    
    // Set all duty cycles to 0
    SET_PWM1A_DUTY(0);
    SET_PWM1B_DUTY(0);
    SET_PWM1C_DUTY(0);
}

/**
 * Emergency brake using PWM1 brake function
 */
void emergency_brake(void) {
    motor.state = MOTOR_BRAKE;
    
    // Activate PWM1 brake function (shorts all low-side switches)
    PWM1_CTRL |= PWM1_BRAKE_EN;
    
    // Set fault flag
    motor.fault_flags |= FAULT_STALL;
}

/**
 * Set motor target speed
 */
void motor_set_speed(unsigned int speed) {
    if (speed > MAX_PWM_DUTY) {
        speed = MAX_PWM_DUTY;
    }
    motor.target_speed = speed;
}

/**
 * Main motor control task
 */
void motor_control_task(void) {
    switch (motor.state) {
        case MOTOR_STARTUP:
            startup_commutation();
            break;
            
        case MOTOR_RUN:
            if (detect_bemf_zero_crossing()) {
                // Calculate commutation delay (30° electrical)
                unsigned int delay = HW_DIV(motor.bemf.commutation_period, 6);
                
                // Wait for optimal commutation point
                // Use Timer2 for precise delay if available
                unsigned long target_time = system_tick + (delay / 1000);
                while (system_tick < target_time) {
                    // Wait for optimal commutation timing
                }
                
                commutate_motor();
                motor.bemf.zero_cross_detected = 0;
            }
            
            // Speed control using hardware math
            if (motor.current_speed < motor.target_speed) {
                if (motor.pwm_duty < MAX_PWM_DUTY) {
                    motor.pwm_duty += 5;
                }
            } else if (motor.current_speed > motor.target_speed) {
                if (motor.pwm_duty > MIN_PWM_DUTY) {
                    motor.pwm_duty -= 5;
                }
            }
            
            set_pwm_duty(motor.pwm_duty);
            
            // Check for stall condition
            if ((system_tick - motor.bemf.last_zero_cross_time) > ZERO_CROSS_TIMEOUT) {
                emergency_brake();
            }
            break;
            
        case MOTOR_BRAKE:
            // Regenerative braking active
            break;
            
        case MOTOR_FAULT:
            // Handle fault conditions
            motor_stop();
            break;
            
        case MOTOR_STOP:
        default:
            // Do nothing
            break;
    }
}

/**
 * Perform motor commutation using PWM1 array
 */
void commutate_motor(void) {
    unsigned char step = motor.current_step;
    
    // Disable all phases first to avoid shoot-through
    PWM1_OE = 0x00;
    
    // Configure phases according to commutation table
    if (commutation_table[step][0] == 1) {
        // Phase A high-side PWM
        ENABLE_PWM1_PHASE_A();
        SET_PWM1A_DUTY(motor.pwm_duty);
    } else if (commutation_table[step][0] == 2) {
        // Phase A low-side on (complementary PWM handles this)
        ENABLE_PWM1_PHASE_A();
        SET_PWM1A_DUTY(0); // Low-side on
    } else {
        // Phase A floating
        FLOAT_PWM1_PHASE_A();
        SET_PWM1A_DUTY(0);
    }
    
    if (commutation_table[step][1] == 1) {
        // Phase B high-side PWM
        ENABLE_PWM1_PHASE_B();
        SET_PWM1B_DUTY(motor.pwm_duty);
    } else if (commutation_table[step][1] == 2) {
        // Phase B low-side on
        ENABLE_PWM1_PHASE_B();
        SET_PWM1B_DUTY(0);
    } else {
        // Phase B floating
        FLOAT_PWM1_PHASE_B();
        SET_PWM1B_DUTY(0);
    }
    
    if (commutation_table[step][2] == 1) {
        // Phase C high-side PWM
        ENABLE_PWM1_PHASE_C();
        SET_PWM1C_DUTY(motor.pwm_duty);
    } else if (commutation_table[step][2] == 2) {
        // Phase C low-side on
        ENABLE_PWM1_PHASE_C();
        SET_PWM1C_DUTY(0);
    } else {
        // Phase C floating
        FLOAT_PWM1_PHASE_C();
        SET_PWM1C_DUTY(0);
    }
    
    // Update floating phase for BEMF detection
    motor.bemf.floating_phase = bemf_monitor_table[step];
    
    // Configure comparator input for floating phase
    CMP0_MUX = bemf_cmp_mux[motor.bemf.floating_phase];
    
    // Advance to next step
    motor.current_step = (motor.current_step + 1) % 6;
    
    // Start BEMF blanking period
    bemf_blank_counter = BEMF_BLANK_TIME;
}

/**
 * Detect BEMF zero crossing using comparators
 */
bit detect_bemf_zero_crossing(void) {
    static unsigned char last_cmp_state = 0;
    
    // Skip detection during blanking time
    if (bemf_blank_counter > 0) {
        return 0;
    }
    
    // Read current comparator state
    unsigned char current_cmp_state = READ_CMP0();
    
    // Detect zero crossing (transition)
    if (current_cmp_state != last_cmp_state) {
        unsigned long current_time = system_tick;
        
        // Calculate commutation period using hardware math
        motor.bemf.commutation_period = current_time - motor.bemf.last_zero_cross_time;
        motor.bemf.last_zero_cross_time = current_time;
        
        // Calculate current speed (RPM) using hardware divider
        if (motor.bemf.commutation_period > 0) {
            // Speed = 60000 / (commutation_period_ms * 6)
            motor.current_speed = HW_DIV(10000UL, motor.bemf.commutation_period);
        }
        
        last_cmp_state = current_cmp_state;
        return 1;
    }
    
    last_cmp_state = current_cmp_state;
    return 0;
}

/**
 * Startup commutation sequence
 */
void startup_commutation(void) {
    static unsigned long last_startup_time = 0;
    unsigned long current_time = system_tick;
    
    // Calculate startup commutation period (decreasing for acceleration)
    unsigned int startup_period = 100 - (motor.startup_counter >> 1);
    if (startup_period < 20) startup_period = 20; // Minimum period
    
    if (current_time - last_startup_time > startup_period) {
        commutate_motor();
        last_startup_time = current_time;
        motor.startup_counter++;
        
        // Gradually increase PWM duty during startup
        if (motor.startup_counter < (STARTUP_STEPS >> 1)) {
            motor.pwm_duty = STARTUP_PWM_DUTY + (motor.startup_counter << 2);
            if (motor.pwm_duty > MAX_PWM_DUTY) {
                motor.pwm_duty = MAX_PWM_DUTY;
            }
        }
        
        set_pwm_duty(motor.pwm_duty);
        
        // Switch to BEMF control after startup
        if (motor.startup_counter >= STARTUP_STEPS) {
            motor.state = MOTOR_RUN;
            motor.bemf.last_zero_cross_time = current_time;
        }
    }
}

/**
 * Set PWM duty cycle for all active phases
 */
void set_pwm_duty(unsigned int duty) {
    if (duty > PWM_PERIOD) {
        duty = PWM_PERIOD;
    }
    
    motor.pwm_duty = duty;
    
    // Update duty cycles based on current commutation step
    unsigned char step = motor.current_step;
    
    if (commutation_table[step][0] == 1) {
        SET_PWM1A_DUTY(duty);
    }
    if (commutation_table[step][1] == 1) {
        SET_PWM1B_DUTY(duty);
    }
    if (commutation_table[step][2] == 1) {
        SET_PWM1C_DUTY(duty);
    }
}

/**
 * Get system tick counter
 */
unsigned long get_system_tick(void) {
    return system_tick;
}

/**
 * Timer0 interrupt service routine for system tick
 */
void timer0_isr(void) interrupt 1 {
    // Reload Timer0 for 1ms tick
    TH0 = 0x3C;
    TL0 = 0xB0;
    
    // Increment system tick
    system_tick++;
    
    // Decrement BEMF blanking counter
    if (bemf_blank_counter > 0) {
        bemf_blank_counter--;
    }
}

/**
 * Comparator interrupt service routine for fast BEMF detection
 */
void cmp0_isr(void) interrupt CMP0_INT_VECTOR {
    if (motor.state == MOTOR_RUN && bemf_blank_counter == 0) {
        // Fast zero crossing detection
        motor.bemf.zero_cross_detected = 1;
        motor.bemf.last_zero_cross_time = system_tick;
    }
}

/**
 * PWM1 interrupt service routine for synchronization
 */
void pwm1_isr(void) interrupt PWM1_INT_VECTOR {
    // PWM1 period match - can be used for precise timing
    // or ADC trigger synchronization
}

/**
 * ADC interrupt service routine for monitoring
 */
void adc_isr(void) interrupt ADC_INT_VECTOR {
    unsigned int adc_value;
    
    // Read ADC result
    adc_value = (ADC_DATA_H << 8) | ADC_DATA_L;
    
    // Check for overcurrent or overvoltage
    if (adc_value > ((ADC_CMP_H << 8) | ADC_CMP_L)) {
        // Fault detected - emergency brake
        motor.fault_flags |= FAULT_OVERCURRENT;
        emergency_brake();
    }
}