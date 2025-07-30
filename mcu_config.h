#ifndef MCU_CONFIG_H
#define MCU_CONFIG_H

#include <avr/io.h>
#include <avr/interrupt.h>

// MCU Configuration for 24MHz operation
#define F_CPU 24000000UL

// Clock configuration macros
#define CLOCK_PRESCALER_1    0x00
#define CLOCK_PRESCALER_8    0x02
#define CLOCK_PRESCALER_64   0x03
#define CLOCK_PRESCALER_256  0x04
#define CLOCK_PRESCALER_1024 0x05

// GPIO Pin Definitions for Motor Control
#define MOTOR_PHASE_A_PIN    PA0
#define MOTOR_PHASE_B_PIN    PA1
#define MOTOR_PHASE_C_PIN    PA2
#define MOTOR_PHASE_A_PORT   PORTA
#define MOTOR_PHASE_B_PORT   PORTA
#define MOTOR_PHASE_C_PORT   PORTA
#define MOTOR_PHASE_A_DDR    DDRA
#define MOTOR_PHASE_B_DDR    DDRA
#define MOTOR_PHASE_C_DDR    DDRA

// PWM Output Pins (Timer1)
#define PWM_A_PIN           PB1  // OC1A
#define PWM_B_PIN           PB2  // OC1B
#define PWM_C_PIN           PB3  // OC1C

// Comparator Input Pins
#define CMP_A_POS_PIN       PA4  // AIN0 - Positive input for Phase A
#define CMP_A_NEG_PIN       PA5  // AIN1 - Negative input (neutral point)
#define CMP_B_POS_PIN       PA6  // AIN2 - Positive input for Phase B
#define CMP_C_POS_PIN       PA7  // AIN3 - Positive input for Phase C

// ADC pins for voltage monitoring (optional)
#define VBUS_SENSE_PIN      PC0
#define CURRENT_SENSE_PIN   PC1

/**
 * Initialize MCU for 24MHz operation
 */
static inline void mcu_clock_init(void) {
    // Configure internal oscillator for 24MHz
    // This assumes the MCU has a calibrated internal oscillator
    // For external crystal, additional configuration would be needed
    
    // Enable internal 24MHz oscillator (MCU dependent)
    // Example for ATmega328P with external 24MHz crystal:
    // CLKPR = (1<<CLKPCE);  // Enable clock prescaler change
    // CLKPR = 0x00;         // No prescaling (24MHz / 1 = 24MHz)
}

/**
 * Initialize GPIO pins for motor control
 */
static inline void gpio_init(void) {
    // Configure motor phase pins as outputs
    MOTOR_PHASE_A_DDR |= (1 << MOTOR_PHASE_A_PIN);
    MOTOR_PHASE_B_DDR |= (1 << MOTOR_PHASE_B_PIN);
    MOTOR_PHASE_C_DDR |= (1 << MOTOR_PHASE_C_PIN);
    
    // Initialize all phases to low
    MOTOR_PHASE_A_PORT &= ~(1 << MOTOR_PHASE_A_PIN);
    MOTOR_PHASE_B_PORT &= ~(1 << MOTOR_PHASE_B_PIN);
    MOTOR_PHASE_C_PORT &= ~(1 << MOTOR_PHASE_C_PIN);
    
    // Configure PWM output pins
    DDRB |= (1 << PWM_A_PIN) | (1 << PWM_B_PIN) | (1 << PWM_C_PIN);
    
    // Configure comparator input pins as inputs
    DDRA &= ~((1 << CMP_A_POS_PIN) | (1 << CMP_A_NEG_PIN) | 
              (1 << CMP_B_POS_PIN) | (1 << CMP_C_POS_PIN));
    
    // Disable pull-ups on comparator inputs
    PORTA &= ~((1 << CMP_A_POS_PIN) | (1 << CMP_A_NEG_PIN) | 
               (1 << CMP_B_POS_PIN) | (1 << CMP_C_POS_PIN));
}

/**
 * Initialize Timer1 for PWM generation at 24MHz
 */
static inline void pwm_timer_init(void) {
    // Timer1 configuration for 3-phase PWM
    // Fast PWM mode with ICR1 as TOP
    TCCR1A = (1 << COM1A1) | (1 << COM1B1) | (1 << COM1C1) | (1 << WGM11);
    TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS10); // No prescaler
    
    // Set PWM frequency to 20kHz at 24MHz
    // PWM_FREQ = F_CPU / (2 * N * TOP)
    // 20000 = 24000000 / (2 * 1 * TOP)
    // TOP = 24000000 / (2 * 20000) = 600
    ICR1 = 600 - 1;  // TOP value for 20kHz PWM
    
    // Initialize duty cycles to 0
    OCR1A = 0;
    OCR1B = 0;
    OCR1C = 0;
}

/**
 * Initialize system timer for timing functions
 */
static inline void system_timer_init(void) {
    // Timer0 for system tick (1ms intervals)
    TCCR0A = (1 << WGM01);  // CTC mode
    TCCR0B = (1 << CS02) | (1 << CS00);  // Prescaler 1024
    
    // Calculate OCR0A for 1ms tick at 24MHz
    // Timer_freq = F_CPU / prescaler = 24MHz / 1024 = 23437.5 Hz
    // For 1ms: OCR0A = (23437.5 / 1000) - 1 = 22.4 ≈ 23
    OCR0A = 23;
    
    // Enable Timer0 compare match interrupt
    TIMSK0 |= (1 << OCIE0A);
}

/**
 * Initialize analog comparators for BEMF detection
 */
static inline void comparator_init(void) {
    // Analog Comparator Control and Status Register
    // Enable comparator, enable interrupt on output toggle
    ACSR = (1 << ACIE) | (1 << ACIS1) | (1 << ACIS0);
    
    // For multiple comparators (if available):
    // Some MCUs have multiple comparators or multiplexed inputs
    
    // Disable analog comparator power reduction
    PRR &= ~(1 << PRADC);
    
    // Configure comparator reference
    // Use internal bandgap reference or external reference
    // This depends on the specific MCU and application requirements
    
    // For BEMF detection, we typically compare each phase against
    // the neutral point (motor center tap) or virtual neutral
}

/**
 * Initialize ADC for optional voltage/current monitoring
 */
static inline void adc_init(void) {
    // ADC configuration for 24MHz operation
    // ADC clock should be between 50kHz and 200kHz for 10-bit resolution
    // ADC_FREQ = F_CPU / prescaler
    // For 24MHz: prescaler = 128 gives 187.5kHz (good)
    
    ADMUX = (1 << REFS0);  // Use AVCC as reference
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);  // Enable ADC, prescaler 128
}

/**
 * Complete MCU initialization for BEMF motor control
 */
static inline void mcu_init(void) {
    // Disable interrupts during initialization
    cli();
    
    // Initialize clock system
    mcu_clock_init();
    
    // Initialize GPIO
    gpio_init();
    
    // Initialize PWM timer
    pwm_timer_init();
    
    // Initialize system timer
    system_timer_init();
    
    // Initialize comparators
    comparator_init();
    
    // Initialize ADC (optional)
    adc_init();
    
    // Enable global interrupts
    sei();
}

/**
 * Utility functions for 24MHz timing
 */
static inline void delay_us(uint16_t microseconds) {
    // Precise microsecond delay for 24MHz
    // Each loop iteration takes approximately 4 clock cycles
    // At 24MHz: 4 cycles = 4/24MHz = 0.167μs
    // So we need 6 iterations per microsecond
    volatile uint16_t count = microseconds * 6;
    while (count--) {
        asm volatile ("nop");
    }
}

static inline void delay_ms(uint16_t milliseconds) {
    for (uint16_t i = 0; i < milliseconds; i++) {
        delay_us(1000);
    }
}

/**
 * Fast GPIO manipulation macros for motor control
 */
#define FAST_PHASE_A_HIGH()  (MOTOR_PHASE_A_PORT |= (1 << MOTOR_PHASE_A_PIN))
#define FAST_PHASE_A_LOW()   (MOTOR_PHASE_A_PORT &= ~(1 << MOTOR_PHASE_A_PIN))
#define FAST_PHASE_B_HIGH()  (MOTOR_PHASE_B_PORT |= (1 << MOTOR_PHASE_B_PIN))
#define FAST_PHASE_B_LOW()   (MOTOR_PHASE_B_PORT &= ~(1 << MOTOR_PHASE_B_PIN))
#define FAST_PHASE_C_HIGH()  (MOTOR_PHASE_C_PORT |= (1 << MOTOR_PHASE_C_PIN))
#define FAST_PHASE_C_LOW()   (MOTOR_PHASE_C_PORT &= ~(1 << MOTOR_PHASE_C_PIN))

/**
 * Comparator reading macros
 */
#define READ_COMPARATOR_A()  ((ACSR & (1 << ACO)) ? 1 : 0)

// For MCUs with multiple comparators:
#ifdef ACSR1
#define READ_COMPARATOR_B()  ((ACSR1 & (1 << ACO1)) ? 1 : 0)
#endif

#ifdef ACSR2
#define READ_COMPARATOR_C()  ((ACSR2 & (1 << ACO2)) ? 1 : 0)
#endif

/**
 * PWM duty cycle setting macros
 */
#define SET_PWM_A(duty)  (OCR1A = duty)
#define SET_PWM_B(duty)  (OCR1B = duty)
#define SET_PWM_C(duty)  (OCR1C = duty)

#endif // MCU_CONFIG_H