#ifndef BEMF_SENSORLESS_H
#define BEMF_SENSORLESS_H

#include <stdint.h>
#include <stdbool.h>

// System configuration
#define F_CPU 24000000UL  // 24MHz MCU frequency
#define PWM_FREQUENCY 20000  // 20kHz PWM frequency
#define PWM_PERIOD (F_CPU / PWM_FREQUENCY)

// Motor control states
typedef enum {
    MOTOR_STOP = 0,
    MOTOR_STARTUP,
    MOTOR_RUN,
    MOTOR_BRAKE
} motor_state_t;

// Commutation states (6-step)
typedef enum {
    STEP_1 = 0,  // A+B-
    STEP_2,      // A+C-
    STEP_3,      // B+C-
    STEP_4,      // B+A-
    STEP_5,      // C+A-
    STEP_6       // C+B-
} commutation_step_t;

// BEMF detection structure
typedef struct {
    uint8_t phase_a_cmp;  // Comparator result for phase A
    uint8_t phase_b_cmp;  // Comparator result for phase B
    uint8_t phase_c_cmp;  // Comparator result for phase C
    uint8_t zero_cross_detected;
    uint32_t last_zero_cross_time;
    uint32_t commutation_period;
} bemf_detector_t;

// Motor control structure
typedef struct {
    motor_state_t state;
    commutation_step_t current_step;
    uint16_t pwm_duty;
    uint16_t target_speed;
    uint16_t current_speed;
    uint32_t startup_counter;
    bemf_detector_t bemf;
} motor_control_t;

// Hardware abstraction macros
#define PHASE_A_HIGH()      (PORTA |= (1<<PA0))
#define PHASE_A_LOW()       (PORTA &= ~(1<<PA0))
#define PHASE_B_HIGH()      (PORTA |= (1<<PA1))
#define PHASE_B_LOW()       (PORTA &= ~(1<<PA1))
#define PHASE_C_HIGH()      (PORTA |= (1<<PA2))
#define PHASE_C_LOW()       (PORTA &= ~(1<<PA2))

#define PHASE_A_FLOAT()     (DDRA &= ~(1<<PA0))
#define PHASE_B_FLOAT()     (DDRA &= ~(1<<PA1))
#define PHASE_C_FLOAT()     (DDRA &= ~(1<<PA2))

#define PHASE_A_OUTPUT()    (DDRA |= (1<<PA0))
#define PHASE_B_OUTPUT()    (DDRA |= (1<<PA1))
#define PHASE_C_OUTPUT()    (DDRA |= (1<<PA2))

// Comparator readings (assuming analog comparator module)
#define READ_CMP_A()        ((ACSR & (1<<ACO)) ? 1 : 0)
#define READ_CMP_B()        ((ACSR1 & (1<<ACO1)) ? 1 : 0)  
#define READ_CMP_C()        ((ACSR2 & (1<<ACO2)) ? 1 : 0)

// Configuration constants
#define STARTUP_PWM_DUTY    512   // 25% duty cycle for startup
#define MAX_PWM_DUTY        2048  // Maximum PWM duty cycle
#define MIN_PWM_DUTY        256   // Minimum PWM duty cycle
#define STARTUP_STEPS       100   // Number of startup commutations
#define BEMF_BLANK_TIME     50    // BEMF blanking time in timer ticks
#define ZERO_CROSS_TIMEOUT  5000  // Timeout for zero crossing detection

// Function prototypes
void motor_init(void);
void motor_start(void);
void motor_stop(void);
void motor_set_speed(uint16_t speed);
void motor_control_task(void);
void commutate_motor(void);
bool detect_bemf_zero_crossing(void);
void startup_commutation(void);
void set_pwm_duty(uint16_t duty);
uint32_t get_system_tick(void);

// Interrupt service routines
void timer_overflow_isr(void);
void comparator_isr(void);

#endif // BEMF_SENSORLESS_H