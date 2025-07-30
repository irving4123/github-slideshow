/*
 * Sensorless BLDC Motor Control using BEMF Detection
 * 
 * This implementation uses the internal analog comparator of Arduino
 * to detect BEMF zero-crossing events for sensorless motor control.
 * 
 * Hardware Requirements:
 * - Arduino Uno/Nano (ATmega328P)
 * - 3-phase BLDC motor
 * - 6x N-channel MOSFETs (IRFZ44N or similar)
 * - 3x Gate drivers (IR2104 or IR2101)
 * - Voltage dividers for BEMF sensing
 * - Bootstrap capacitors and diodes
 * 
 * Pin Assignments:
 * - Pins 3,4,5: Low side gate drivers
 * - Pins 9,10,11: High side PWM outputs
 * - Pin 6 (AIN0): Virtual neutral point (comparator +)
 * - Pin 7 (AIN1): BEMF Phase A (comparator -)
 * - A2: BEMF Phase B
 * - A3: BEMF Phase C
 * - A4: Throttle input
 */

// Pin definitions
#define PHASE_A_HIGH    11  // PWM pin for Phase A high side (OC2A)
#define PHASE_A_LOW     5   // Phase A low side
#define PHASE_B_HIGH    10  // PWM pin for Phase B high side (OC1B)
#define PHASE_B_LOW     4   // Phase B low side
#define PHASE_C_HIGH    9   // PWM pin for Phase C high side (OC1A)
#define PHASE_C_LOW     3   // Phase C low side

#define VIRTUAL_NEUTRAL 6   // AIN0 - Comparator positive input
#define BEMF_A          7   // AIN1 - Comparator negative input
#define BEMF_B          A2  // ADC2 - BEMF Phase B
#define BEMF_C          A3  // ADC3 - BEMF Phase C
#define THROTTLE        A4  // Throttle input

// Motor control constants
#define PWM_MAX_DUTY    255
#define PWM_MIN_DUTY    50
#define PWM_START_DUTY  120
#define STARTUP_STEPS   36  // 6 electrical cycles

// Motor state variables
volatile uint8_t bldc_step = 0;
volatile uint16_t motor_speed = PWM_START_DUTY;
volatile bool zero_cross_detected = false;
volatile unsigned long last_zero_cross = 0;
volatile unsigned long commutation_delay = 1000;

// Commutation lookup table
// [Step][A_H, A_L, B_H, B_L, C_H, C_L]
const uint8_t commutation_table[6][6] = {
  {1, 0, 0, 1, 0, 0},  // Step 0: A+ B- (sense C)
  {1, 0, 0, 0, 0, 1},  // Step 1: A+ C- (sense B)
  {0, 0, 1, 0, 0, 1},  // Step 2: B+ C- (sense A)
  {0, 1, 1, 0, 0, 0},  // Step 3: B+ A- (sense C)
  {0, 1, 0, 0, 1, 0},  // Step 4: C+ A- (sense B)
  {0, 0, 0, 1, 1, 0}   // Step 5: C+ B- (sense A)
};

// BEMF sensing configuration for each step
const uint8_t bemf_config[6] = {
  3,  // Step 0: Sense C (ADC3)
  2,  // Step 1: Sense B (ADC2) 
  1,  // Step 2: Sense A (Comparator)
  3,  // Step 3: Sense C (ADC3)
  2,  // Step 4: Sense B (ADC2)
  1   // Step 5: Sense A (Comparator)
};

// Majority filter for noise reduction
#define FILTER_SIZE 6
bool majority_filter_buffer[FILTER_SIZE] = {0};
uint8_t filter_index = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("Sensorless BLDC Controller Starting...");
  
  // Initialize GPIO pins
  pinMode(PHASE_A_HIGH, OUTPUT);
  pinMode(PHASE_A_LOW, OUTPUT);
  pinMode(PHASE_B_HIGH, OUTPUT);
  pinMode(PHASE_B_LOW, OUTPUT);
  pinMode(PHASE_C_HIGH, OUTPUT);
  pinMode(PHASE_C_LOW, OUTPUT);
  
  // Ensure all outputs are off initially
  digitalWrite(PHASE_A_HIGH, LOW);
  digitalWrite(PHASE_A_LOW, LOW);
  digitalWrite(PHASE_B_HIGH, LOW);
  digitalWrite(PHASE_B_LOW, LOW);
  pinMode(PHASE_C_HIGH, LOW);
  digitalWrite(PHASE_C_LOW, LOW);
  
  // Configure PWM timers
  setup_pwm_timers();
  
  // Configure ADC for BEMF sensing
  setup_adc();
  
  // Configure analog comparator
  setup_comparator();
  
  // Motor startup sequence
  Serial.println("Starting motor...");
  startup_motor();
  
  // Enable comparator interrupt for zero-crossing detection
  enable_zero_cross_detection();
  
  Serial.println("Motor started - entering sensorless mode");
}

void loop() {
  // Check for zero crossing detection
  if (zero_cross_detected) {
    handle_zero_crossing();
    zero_cross_detected = false;
  }
  
  // Update motor speed based on throttle
  update_throttle();
  
  // Optional: Print debug information
  static unsigned long last_debug = 0;
  if (millis() - last_debug > 1000) {
    Serial.print("Step: ");
    Serial.print(bldc_step);
    Serial.print(", Speed: ");
    Serial.print(motor_speed);
    Serial.print(", Delay: ");
    Serial.println(commutation_delay);
    last_debug = millis();
  }
  
  delay(1);
}

void setup_pwm_timers() {
  // Configure Timer1 for pins 9 and 10 (Phase C and B)
  TCCR1A = 0;
  TCCR1B = 0x01;  // No prescaling, PWM off initially
  
  // Configure Timer2 for pin 11 (Phase A)
  TCCR2A = 0;
  TCCR2B = 0x01;  // No prescaling, PWM off initially
}

void setup_adc() {
  // Configure ADC for fast conversion
  ADCSRA = 0x87;  // Enable ADC, prescaler = 128
  ADMUX = 0x40;   // AVCC reference, right adjusted
}

void setup_comparator() {
  // Configure analog comparator
  // AIN0 (pin 6) = positive input (virtual neutral)
  // AIN1 (pin 7) = negative input (BEMF Phase A)
  ACSR = 0x10;    // Disable comparator interrupt initially
  ADCSRB = 0x00;  // Use AIN1 as negative input
}

void startup_motor() {
  // Forced commutation startup sequence
  motor_speed = PWM_START_DUTY;
  
  unsigned long step_delay = 5000;  // Start with 5ms delay
  
  for (int i = 0; i < STARTUP_STEPS; i++) {
    commutate_motor();
    delayMicroseconds(step_delay);
    
    // Gradually decrease delay to accelerate motor
    if (step_delay > 500) {
      step_delay -= 100;
    }
    
    // Advance to next commutation step
    bldc_step = (bldc_step + 1) % 6;
  }
  
  // Set initial commutation delay for sensorless operation
  commutation_delay = step_delay / 3;  // 30 degrees = 1/12 of full period
}

void commutate_motor() {
  // Turn off all outputs first
  turn_off_all_phases();
  
  // Small delay to prevent shoot-through
  delayMicroseconds(1);
  
  // Apply new commutation pattern
  digitalWrite(PHASE_A_LOW, commutation_table[bldc_step][1]);
  digitalWrite(PHASE_B_LOW, commutation_table[bldc_step][3]);
  digitalWrite(PHASE_C_LOW, commutation_table[bldc_step][5]);
  
  // Enable PWM on the appropriate high-side phase
  if (commutation_table[bldc_step][0]) {
    // Phase A high
    TCCR2A = 0x81;  // Enable PWM on OC2A (pin 11)
    OCR2A = motor_speed;
  } else if (commutation_table[bldc_step][2]) {
    // Phase B high  
    TCCR1A = 0x21;  // Enable PWM on OC1B (pin 10)
    OCR1B = motor_speed;
  } else if (commutation_table[bldc_step][4]) {
    // Phase C high
    TCCR1A = 0x81;  // Enable PWM on OC1A (pin 9)
    OCR1A = motor_speed;
  }
}

void turn_off_all_phases() {
  // Disable all PWM outputs
  TCCR1A = 0;
  TCCR2A = 0;
  
  // Turn off all low-side switches
  digitalWrite(PHASE_A_LOW, LOW);
  digitalWrite(PHASE_B_LOW, LOW);
  digitalWrite(PHASE_C_LOW, LOW);
}

void enable_zero_cross_detection() {
  // Configure comparator for the current step
  configure_bemf_sensing();
  
  // Enable comparator interrupt
  ACSR |= 0x08;  // Enable analog comparator interrupt
}

void configure_bemf_sensing() {
  uint8_t bemf_channel = bemf_config[bldc_step];
  
  if (bemf_channel == 1) {
    // Use analog comparator for Phase A
    ADCSRB = 0x00;  // Use AIN1 as negative input
    ACSR |= 0x03;   // Set interrupt on rising edge
  } else {
    // Use ADC channels for Phase B or C
    // This would require additional logic for ADC-based comparison
    // For simplicity, we'll use polling in this example
  }
}

bool detect_zero_crossing() {
  uint8_t bemf_channel = bemf_config[bldc_step];
  bool zero_cross = false;
  
  if (bemf_channel == 1) {
    // Use analog comparator output
    zero_cross = (ACSR & 0x20) ? true : false;
  } else {
    // Use ADC for phases B and C
    uint16_t bemf_voltage, neutral_voltage;
    
    // Read virtual neutral
    ADMUX = 0x46;  // Select ADC6 (but we use pin 6 as analog input)
    // Note: This is a simplified approach - in practice you'd need
    // to properly configure the ADC multiplexer
    
    // Read BEMF voltage
    if (bemf_channel == 2) {
      ADMUX = 0x42;  // ADC2
    } else {
      ADMUX = 0x43;  // ADC3
    }
    
    // Start conversion and wait
    ADCSRA |= 0x40;
    while (ADCSRA & 0x40);
    bemf_voltage = ADC;
    
    // For this example, we'll use a simple threshold
    zero_cross = (bemf_voltage > 512);  // Assuming 5V reference, 2.5V threshold
  }
  
  return majority_filter(zero_cross);
}

bool majority_filter(bool new_sample) {
  // Add new sample to circular buffer
  majority_filter_buffer[filter_index] = new_sample;
  filter_index = (filter_index + 1) % FILTER_SIZE;
  
  // Count true values
  uint8_t true_count = 0;
  for (int i = 0; i < FILTER_SIZE; i++) {
    if (majority_filter_buffer[i]) {
      true_count++;
    }
  }
  
  // Return majority decision
  return (true_count > FILTER_SIZE / 2);
}

void handle_zero_crossing() {
  unsigned long current_time = micros();
  
  // Calculate time since last zero crossing
  unsigned long period = current_time - last_zero_cross;
  if (last_zero_cross > 0 && period > 1000) {  // Ignore very short periods (noise)
    // Update commutation delay (30 degrees = period/12)
    commutation_delay = period / 12;
    
    // Limit delay to reasonable bounds
    if (commutation_delay < 100) commutation_delay = 100;
    if (commutation_delay > 10000) commutation_delay = 10000;
  }
  
  last_zero_cross = current_time;
  
  // Wait for 30 degree delay, then commutate
  delayMicroseconds(commutation_delay);
  
  // Advance to next step and commutate
  bldc_step = (bldc_step + 1) % 6;
  commutate_motor();
  
  // Reconfigure BEMF sensing for new step
  configure_bemf_sensing();
}

void update_throttle() {
  static unsigned long last_throttle_read = 0;
  
  // Read throttle every 50ms
  if (millis() - last_throttle_read > 50) {
    int throttle_raw = analogRead(THROTTLE);
    
    // Map throttle to PWM duty cycle
    motor_speed = map(throttle_raw, 0, 1023, PWM_MIN_DUTY, PWM_MAX_DUTY);
    
    // Apply speed limits
    if (motor_speed < PWM_MIN_DUTY) motor_speed = PWM_MIN_DUTY;
    if (motor_speed > PWM_MAX_DUTY) motor_speed = PWM_MAX_DUTY;
    
    last_throttle_read = millis();
  }
}

// Analog Comparator Interrupt Service Routine
ISR(ANALOG_COMP_vect) {
  // Simple debouncing
  static unsigned long last_interrupt = 0;
  unsigned long current_time = micros();
  
  if (current_time - last_interrupt > 100) {  // 100us debounce
    zero_cross_detected = true;
    last_interrupt = current_time;
  }
  
  // Clear interrupt flag
  ACSR |= 0x10;
}

// Emergency stop function
void emergency_stop() {
  // Disable all interrupts
  cli();
  
  // Turn off all phases
  turn_off_all_phases();
  
  // Disable comparator interrupt
  ACSR &= ~0x08;
  
  Serial.println("EMERGENCY STOP ACTIVATED");
  
  // Halt execution
  while(1) {
    delay(1000);
  }
}