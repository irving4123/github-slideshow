#ifndef BEMF_8051_H
#define BEMF_8051_H

#include <reg51.h>
#include <intrins.h>

// System configuration for enhanced 8051
#define F_CPU 48000000UL  // 48MHz internal RC oscillator
#define PWM_FREQUENCY 20000  // 20kHz PWM frequency
#define SYSTEM_TICK_MS 1     // 1ms system tick

// Motor control states
typedef enum {
    MOTOR_STOP = 0,
    MOTOR_STARTUP,
    MOTOR_RUN,
    MOTOR_BRAKE,
    MOTOR_FAULT
} motor_state_t;

// Commutation steps (6-step)
typedef enum {
    STEP_1 = 0,  // A+B- (PWM1A+PWM1B-, PWM1C floating)
    STEP_2,      // A+C- (PWM1A+PWM1C-, PWM1B floating)
    STEP_3,      // B+C- (PWM1B+PWM1C-, PWM1A floating)
    STEP_4,      // B+A- (PWM1B+PWM1A-, PWM1C floating)
    STEP_5,      // C+A- (PWM1C+PWM1A-, PWM1B floating)
    STEP_6       // C+B- (PWM1C+PWM1B-, PWM1A floating)
} commutation_step_t;

// BEMF detection structure
typedef struct {
    unsigned char cmp0_state;     // Comparator 0 output
    unsigned char cmp1_state;     // Comparator 1 output
    unsigned char zero_cross_detected;
    unsigned long last_zero_cross_time;
    unsigned int commutation_period;
    unsigned char floating_phase; // Current floating phase (0=A, 1=B, 2=C)
} bemf_detector_t;

// Motor control structure
typedef struct {
    motor_state_t state;
    commutation_step_t current_step;
    unsigned int pwm_duty;
    unsigned int target_speed;
    unsigned int current_speed;
    unsigned int startup_counter;
    bemf_detector_t bemf;
    unsigned char fault_flags;
} motor_control_t;

// PWM1 register definitions (assuming standard enhanced 8051 layout)
sfr PWM1_CTRL = 0xD1;    // PWM1 control register
sfr PWM1_CFG = 0xD2;     // PWM1 configuration register
sfr PWM1_PERIOD_L = 0xD3; // PWM1 period low byte
sfr PWM1_PERIOD_H = 0xD4; // PWM1 period high byte
sfr PWM1_DT = 0xD5;      // PWM1 dead time register

// PWM1 duty cycle registers
sfr PWM1A_DUTY_L = 0xD6;
sfr PWM1A_DUTY_H = 0xD7;
sfr PWM1B_DUTY_L = 0xD8;
sfr PWM1B_DUTY_H = 0xD9;
sfr PWM1C_DUTY_L = 0xDA;
sfr PWM1C_DUTY_H = 0xDB;

// PWM1 output enable register
sfr PWM1_OE = 0xDC;

// Comparator registers
sfr CMP0_CTRL = 0xE1;    // Comparator 0 control
sfr CMP0_MUX = 0xE2;     // Comparator 0 input multiplexer
sfr CMP1_CTRL = 0xE3;    // Comparator 1 control
sfr CMP1_MUX = 0xE4;     // Comparator 1 input multiplexer
sfr CMP_STATUS = 0xE5;   // Comparator status register

// ADC registers for monitoring
sfr ADC_CTRL = 0xF1;     // ADC control register
sfr ADC_CFG = 0xF2;      // ADC configuration
sfr ADC_DATA_L = 0xF3;   // ADC data low byte
sfr ADC_DATA_H = 0xF4;   // ADC data high byte
sfr ADC_CMP_L = 0xF5;    // ADC compare low byte
sfr ADC_CMP_H = 0xF6;    // ADC compare high byte

// Timer2 enhanced registers
sfr T2CON_EX = 0xC9;     // Timer2 extended control
sfr T2MOD = 0xC9;        // Timer2 mode register

// Configuration constants
#define STARTUP_PWM_DUTY    300   // 30% duty cycle for startup
#define MAX_PWM_DUTY        900   // 90% maximum duty cycle
#define MIN_PWM_DUTY        100   // 10% minimum duty cycle
#define STARTUP_STEPS       120   // Number of startup commutations
#define BEMF_BLANK_TIME     20    // BEMF blanking time in ms
#define ZERO_CROSS_TIMEOUT  200   // Timeout for zero crossing detection (ms)
#define DEAD_TIME_NS        1000  // 1μs dead time

// PWM1 configuration bits
#define PWM1_EN         0x01  // PWM1 enable
#define PWM1_CENTER     0x02  // Center-aligned mode
#define PWM1_COMP_MODE  0x04  // Complementary mode
#define PWM1_RELOAD_EN  0x08  // Reload enable
#define PWM1_BRAKE_EN   0x10  // Brake function enable

// Comparator configuration bits
#define CMP_EN          0x01  // Comparator enable
#define CMP_INT_EN      0x02  // Comparator interrupt enable
#define CMP_OUT_POL     0x04  // Output polarity
#define CMP_HYST_EN     0x08  // Hysteresis enable
#define CMP_FILT_EN     0x10  // Digital filter enable

// Fault flags
#define FAULT_OVERCURRENT   0x01
#define FAULT_OVERVOLTAGE   0x02
#define FAULT_STALL         0x04
#define FAULT_BEMF_LOST     0x08

// Function prototypes
void motor_init(void);
void motor_start(void);
void motor_stop(void);
void motor_set_speed(unsigned int speed);
void motor_control_task(void);
void commutate_motor(void);
bit detect_bemf_zero_crossing(void);
void startup_commutation(void);
void set_pwm_duty(unsigned int duty);
void configure_pwm1(void);
void configure_comparators(void);
void configure_adc(void);
unsigned long get_system_tick(void);
void emergency_brake(void);

// Hardware abstraction macros for enhanced 8051
#define ENABLE_PWM1_PHASE_A()   (PWM1_OE |= 0x03)   // Enable PWM1A+ and PWM1A-
#define ENABLE_PWM1_PHASE_B()   (PWM1_OE |= 0x0C)   // Enable PWM1B+ and PWM1B-
#define ENABLE_PWM1_PHASE_C()   (PWM1_OE |= 0x30)   // Enable PWM1C+ and PWM1C-

#define DISABLE_PWM1_PHASE_A()  (PWM1_OE &= ~0x03)  // Disable PWM1A+ and PWM1A-
#define DISABLE_PWM1_PHASE_B()  (PWM1_OE &= ~0x0C)  // Disable PWM1B+ and PWM1B-
#define DISABLE_PWM1_PHASE_C()  (PWM1_OE &= ~0x30)  // Disable PWM1C+ and PWM1C-

#define FLOAT_PWM1_PHASE_A()    DISABLE_PWM1_PHASE_A()
#define FLOAT_PWM1_PHASE_B()    DISABLE_PWM1_PHASE_B()
#define FLOAT_PWM1_PHASE_C()    DISABLE_PWM1_PHASE_C()

// Comparator reading macros
#define READ_CMP0()             (CMP_STATUS & 0x01)
#define READ_CMP1()             (CMP_STATUS & 0x02)

// Fast math using hardware multiplier/divider
#define HW_MUL(a, b)            ((unsigned long)(a) * (b))
#define HW_DIV(a, b)            ((unsigned int)(a) / (b))

// Interrupt vectors for enhanced 8051
#define PWM1_INT_VECTOR         13  // PWM1 interrupt vector
#define CMP0_INT_VECTOR         14  // Comparator 0 interrupt vector
#define CMP1_INT_VECTOR         15  // Comparator 1 interrupt vector
#define ADC_INT_VECTOR          16  // ADC interrupt vector

// Global variables
extern motor_control_t xdata motor;
extern volatile unsigned long xdata system_tick;
extern volatile unsigned char xdata bemf_blank_counter;

// Inline functions for performance
#define SET_PWM1A_DUTY(duty) do { \
    PWM1A_DUTY_L = (duty) & 0xFF; \
    PWM1A_DUTY_H = ((duty) >> 8) & 0xFF; \
} while(0)

#define SET_PWM1B_DUTY(duty) do { \
    PWM1B_DUTY_L = (duty) & 0xFF; \
    PWM1B_DUTY_H = ((duty) >> 8) & 0xFF; \
} while(0)

#define SET_PWM1C_DUTY(duty) do { \
    PWM1C_DUTY_L = (duty) & 0xFF; \
    PWM1C_DUTY_H = ((duty) >> 8) & 0xFF; \
} while(0)

// Commutation table for 6-step control
// Format: [step][phase] = {A_enable, B_enable, C_enable}
extern code unsigned char commutation_table[6][3];

// BEMF detection table - which comparator to monitor for each step
extern code unsigned char bemf_monitor_table[6];

// PWM period calculation for 48MHz and 20kHz
// PWM_PERIOD = F_CPU / PWM_FREQ = 48MHz / 20kHz = 2400
#define PWM_PERIOD 2400

#endif // BEMF_8051_H