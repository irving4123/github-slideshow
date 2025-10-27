#include "bemf_sensorless.h"

// Global motor control instance
static motor_control_t motor;
static volatile uint32_t system_tick = 0;
static volatile uint8_t bemf_blank_counter = 0;

// Commutation table for 6-step control
// Each step defines which phases are active (high/low/floating)
static const uint8_t commutation_table[6][3] = {
    // Phase A, Phase B, Phase C (0=float, 1=high, 2=low)
    {1, 2, 0},  // Step 1: A+, B-, C floating
    {1, 0, 2},  // Step 2: A+, B floating, C-
    {0, 1, 2},  // Step 3: A floating, B+, C-
    {2, 1, 0},  // Step 4: A-, B+, C floating
    {2, 0, 1},  // Step 5: A-, B floating, C+
    {0, 2, 1}   // Step 6: A floating, B-, C+
};

// BEMF zero crossing detection table
// Expected comparator states for each commutation step
static const uint8_t bemf_expected[6] = {
    0x04,  // Step 1: expect C comparator high
    0x02,  // Step 2: expect B comparator high  
    0x01,  // Step 3: expect A comparator high
    0x04,  // Step 4: expect C comparator low
    0x02,  // Step 5: expect B comparator low
    0x01   // Step 6: expect A comparator low
};

/**
 * Initialize motor control system
 */
void motor_init(void) {
    // Initialize motor control structure
    motor.state = MOTOR_STOP;
    motor.current_step = STEP_1;
    motor.pwm_duty = 0;
    motor.target_speed = 0;
    motor.current_speed = 0;
    motor.startup_counter = 0;
    
    // Initialize BEMF detector
    motor.bemf.phase_a_cmp = 0;
    motor.bemf.phase_b_cmp = 0;
    motor.bemf.phase_c_cmp = 0;
    motor.bemf.zero_cross_detected = 0;
    motor.bemf.last_zero_cross_time = 0;
    motor.bemf.commutation_period = 1000; // Initial period
    
    // Configure GPIO pins for motor phases
    PHASE_A_OUTPUT();
    PHASE_B_OUTPUT();
    PHASE_C_OUTPUT();
    
    // Turn off all phases initially
    PHASE_A_LOW();
    PHASE_B_LOW();
    PHASE_C_LOW();
    
    // Initialize PWM timer (Timer1 for 16-bit resolution)
    // Fast PWM mode, non-inverting
    TCCR1A = (1<<COM1A1) | (1<<COM1B1) | (1<<COM1C1) | (1<<WGM11);
    TCCR1B = (1<<WGM13) | (1<<WGM12) | (1<<CS10); // No prescaler for 24MHz
    ICR1 = PWM_PERIOD - 1; // Set PWM period
    
    // Initialize system timer (Timer0 for system tick)
    TCCR0A = (1<<WGM01); // CTC mode
    TCCR0B = (1<<CS02) | (1<<CS00); // Prescaler 1024
    OCR0A = (F_CPU / 1024 / 1000) - 1; // 1ms tick
    TIMSK0 |= (1<<OCIE0A); // Enable compare match interrupt
    
    // Initialize analog comparators
    // Comparator 0 for Phase A
    ACSR = (1<<ACIE); // Enable comparator interrupt
    
    // Enable global interrupts
    sei();
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
    
    // Turn off all phases
    PHASE_A_LOW();
    PHASE_B_LOW();
    PHASE_C_LOW();
    
    // Disable PWM outputs
    OCR1A = 0;
    OCR1B = 0;
    OCR1C = 0;
}

/**
 * Set motor target speed
 */
void motor_set_speed(uint16_t speed) {
    if (speed > MAX_PWM_DUTY) {
        speed = MAX_PWM_DUTY;
    }
    motor.target_speed = speed;
}

/**
 * Main motor control task - call this regularly in main loop
 */
void motor_control_task(void) {
    switch (motor.state) {
        case MOTOR_STARTUP:
            startup_commutation();
            break;
            
        case MOTOR_RUN:
            if (detect_bemf_zero_crossing()) {
                // Wait for 30 degrees electrical (commutation delay)
                uint32_t delay = motor.bemf.commutation_period / 6;
                
                // Simple delay implementation for 24MHz MCU
                for (volatile uint32_t i = 0; i < delay; i++) {
                    asm("nop");
                }
                
                commutate_motor();
                motor.bemf.zero_cross_detected = 0;
            }
            
            // Speed control - simple PI controller
            if (motor.current_speed < motor.target_speed) {
                if (motor.pwm_duty < MAX_PWM_DUTY) {
                    motor.pwm_duty += 2;
                }
            } else if (motor.current_speed > motor.target_speed) {
                if (motor.pwm_duty > MIN_PWM_DUTY) {
                    motor.pwm_duty -= 2;
                }
            }
            
            set_pwm_duty(motor.pwm_duty);
            break;
            
        case MOTOR_BRAKE:
            // Regenerative braking - short all phases
            PHASE_A_LOW();
            PHASE_B_LOW();
            PHASE_C_LOW();
            break;
            
        case MOTOR_STOP:
        default:
            // Do nothing
            break;
    }
}

/**
 * Perform motor commutation based on current step
 */
void commutate_motor(void) {
    uint8_t step = motor.current_step;
    
    // Set phases according to commutation table
    // Phase A
    if (commutation_table[step][0] == 1) {
        PHASE_A_OUTPUT();
        PHASE_A_HIGH();
    } else if (commutation_table[step][0] == 2) {
        PHASE_A_OUTPUT();
        PHASE_A_LOW();
    } else {
        PHASE_A_FLOAT();
    }
    
    // Phase B
    if (commutation_table[step][1] == 1) {
        PHASE_B_OUTPUT();
        PHASE_B_HIGH();
    } else if (commutation_table[step][1] == 2) {
        PHASE_B_OUTPUT();
        PHASE_B_LOW();
    } else {
        PHASE_B_FLOAT();
    }
    
    // Phase C
    if (commutation_table[step][2] == 1) {
        PHASE_C_OUTPUT();
        PHASE_C_HIGH();
    } else if (commutation_table[step][2] == 2) {
        PHASE_C_OUTPUT();
        PHASE_C_LOW();
    } else {
        PHASE_C_FLOAT();
    }
    
    // Advance to next step
    motor.current_step = (motor.current_step + 1) % 6;
    
    // Start BEMF blanking period
    bemf_blank_counter = BEMF_BLANK_TIME;
}

/**
 * Detect BEMF zero crossing using comparators
 */
bool detect_bemf_zero_crossing(void) {
    // Skip detection during blanking time
    if (bemf_blank_counter > 0) {
        return false;
    }
    
    // Read comparator states
    motor.bemf.phase_a_cmp = READ_CMP_A();
    motor.bemf.phase_b_cmp = READ_CMP_B();
    motor.bemf.phase_c_cmp = READ_CMP_C();
    
    // Combine comparator states into single value
    uint8_t cmp_state = (motor.bemf.phase_a_cmp) | 
                       (motor.bemf.phase_b_cmp << 1) | 
                       (motor.bemf.phase_c_cmp << 2);
    
    // Check for expected zero crossing based on current step
    uint8_t expected = bemf_expected[motor.current_step];
    bool zero_cross = false;
    
    // Detect transition in the floating phase
    switch (motor.current_step) {
        case STEP_1: // C floating, detect C going high
            zero_cross = (cmp_state & 0x04) && !(motor.bemf.phase_c_cmp);
            break;
        case STEP_2: // B floating, detect B going high
            zero_cross = (cmp_state & 0x02) && !(motor.bemf.phase_b_cmp);
            break;
        case STEP_3: // A floating, detect A going high
            zero_cross = (cmp_state & 0x01) && !(motor.bemf.phase_a_cmp);
            break;
        case STEP_4: // C floating, detect C going low
            zero_cross = !(cmp_state & 0x04) && (motor.bemf.phase_c_cmp);
            break;
        case STEP_5: // B floating, detect B going low
            zero_cross = !(cmp_state & 0x02) && (motor.bemf.phase_b_cmp);
            break;
        case STEP_6: // A floating, detect A going low
            zero_cross = !(cmp_state & 0x01) && (motor.bemf.phase_a_cmp);
            break;
    }
    
    if (zero_cross) {
        uint32_t current_time = system_tick;
        motor.bemf.commutation_period = current_time - motor.bemf.last_zero_cross_time;
        motor.bemf.last_zero_cross_time = current_time;
        motor.bemf.zero_cross_detected = 1;
        
        // Calculate current speed
        if (motor.bemf.commutation_period > 0) {
            motor.current_speed = 60000000UL / (motor.bemf.commutation_period * 6);
        }
        
        return true;
    }
    
    return false;
}

/**
 * Startup commutation sequence
 */
void startup_commutation(void) {
    static uint32_t last_startup_time = 0;
    uint32_t current_time = system_tick;
    
    // Fixed timing commutation during startup
    if (current_time - last_startup_time > 50) { // 50ms between steps
        commutate_motor();
        last_startup_time = current_time;
        motor.startup_counter++;
        
        // Gradually increase PWM duty during startup
        if (motor.startup_counter < STARTUP_STEPS / 2) {
            motor.pwm_duty = STARTUP_PWM_DUTY + (motor.startup_counter * 4);
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
 * Set PWM duty cycle
 */
void set_pwm_duty(uint16_t duty) {
    if (duty > PWM_PERIOD) {
        duty = PWM_PERIOD;
    }
    
    // Set PWM duty for all three phases
    OCR1A = duty;
    OCR1B = duty;
    OCR1C = duty;
}

/**
 * Get system tick counter
 */
uint32_t get_system_tick(void) {
    return system_tick;
}

/**
 * Timer overflow interrupt service routine
 */
ISR(TIMER0_COMPA_vect) {
    system_tick++;
    
    // Decrement BEMF blanking counter
    if (bemf_blank_counter > 0) {
        bemf_blank_counter--;
    }
}

/**
 * Comparator interrupt service routine
 */
ISR(ANALOG_COMP_vect) {
    // This ISR can be used for faster zero crossing detection
    // if the MCU supports comparator interrupts
    if (motor.state == MOTOR_RUN) {
        // Set flag for main loop processing
        motor.bemf.zero_cross_detected = 1;
    }
}