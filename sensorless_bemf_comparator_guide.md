# Sensorless BLDC Motor Control Using BEMF and Comparators

## Overview

Sensorless BLDC (Brushless DC) motor control using Back Electromotive Force (BEMF) detection with comparators is a cost-effective method that eliminates the need for Hall effect sensors. This approach uses the motor's own generated BEMF to determine rotor position and timing for commutation.

## Theory of Operation

### BEMF Generation
When a BLDC motor rotates, each winding generates BEMF that opposes the applied voltage according to Lenz's law. The BEMF is proportional to:
- Motor speed (ω)
- Magnetic field strength (B)
- Number of turns (N)

**BEMF = N × l × r × B × ω**

### Zero-Crossing Detection
In six-step commutation, only two phases are energized at any time, leaving one phase floating. The floating phase's BEMF can be monitored for zero-crossing events, which occur 30° before the next commutation point.

### Commutation Sequence
The motor follows a six-step commutation pattern:
1. A+B- (sense C)
2. A+C- (sense B)  
3. B+C- (sense A)
4. B+A- (sense C)
5. C+A- (sense B)
6. C+B- (sense A)

## Circuit Implementation

### Basic Comparator Circuit

```
Motor Phase A ----[33kΩ]----+---- To Comparator Input
                             |
                           [10kΩ]
                             |
                            GND

Virtual Neutral Point:
Phase A ----[33kΩ]----+
Phase B ----[33kΩ]----+---- Virtual Neutral (To Comparator Reference)
Phase C ----[33kΩ]----+
```

### Complete Control Circuit

**Power Stage:**
- 6 x N-Channel MOSFETs (e.g., IRFZ44N, IRF540)
- 3 x Gate Drivers (IR2104, IR2101)
- Bootstrap capacitors and diodes

**Control Circuit:**
- Microcontroller (Arduino, PIC, STM32)
- 3 x Voltage dividers for BEMF sensing
- 1 x Virtual neutral point generation
- PWM generation and comparator interface

### Voltage Divider Design

For 12V system:
- R1 = 33kΩ (high side)
- R2 = 10kΩ (low side)
- Output voltage = 12V × (10k/(33k+10k)) = 2.79V max

For higher voltages, scale resistors accordingly:
- 48V system: R1 = 150kΩ, R2 = 10kΩ
- 400V system: R1 = 390kΩ, R2 = 2.7kΩ

## Software Implementation

### Arduino Example Code

```cpp
// Pin definitions
#define PHASE_A_HIGH    9   // PWM pin for Phase A high side
#define PHASE_A_LOW     3   // Phase A low side
#define PHASE_B_HIGH    10  // PWM pin for Phase B high side  
#define PHASE_B_LOW     4   // Phase B low side
#define PHASE_C_HIGH    11  // PWM pin for Phase C high side
#define PHASE_C_LOW     5   // Phase C low side

#define BEMF_A          A0  // BEMF sensing for Phase A
#define BEMF_B          A1  // BEMF sensing for Phase B  
#define BEMF_C          A2  // BEMF sensing for Phase C
#define VIRTUAL_NEUTRAL A3  // Virtual neutral point

// Motor control variables
volatile uint8_t commutation_step = 0;
volatile uint16_t motor_speed = 100;
volatile bool zero_cross_detected = false;

// Commutation table
const uint8_t commutation_table[6][6] = {
  // A_H, A_L, B_H, B_L, C_H, C_L
  {1, 0, 0, 1, 0, 0},  // Step 0: A+B-
  {1, 0, 0, 0, 0, 1},  // Step 1: A+C-
  {0, 0, 1, 0, 0, 1},  // Step 2: B+C-
  {0, 1, 1, 0, 0, 0},  // Step 3: B+A-
  {0, 1, 0, 0, 1, 0},  // Step 4: C+A-
  {0, 0, 0, 1, 1, 0}   // Step 5: C+B-
};

const uint8_t bemf_channel[6] = {BEMF_C, BEMF_B, BEMF_A, BEMF_C, BEMF_B, BEMF_A};

void setup() {
  // Initialize PWM pins
  pinMode(PHASE_A_HIGH, OUTPUT);
  pinMode(PHASE_A_LOW, OUTPUT);
  pinMode(PHASE_B_HIGH, OUTPUT);
  pinMode(PHASE_B_LOW, OUTPUT);
  pinMode(PHASE_C_HIGH, OUTPUT);
  pinMode(PHASE_C_LOW, OUTPUT);
  
  // Configure PWM frequency (31.25 kHz)
  TCCR1A = 0;
  TCCR1B = 0x01;  // No prescaling
  TCCR2A = 0;
  TCCR2B = 0x01;  // No prescaling
  
  // Initialize ADC for BEMF sensing
  ADCSRA = 0x87;  // Enable ADC, prescaler 128
  ADMUX = 0x40;   // AVCC reference
  
  // Start motor with forced commutation
  startup_sequence();
  
  // Enable zero-crossing detection
  setup_zero_cross_detection();
}

void loop() {
  if (zero_cross_detected) {
    delay_30_degrees();  // Wait 30° after zero crossing
    commutate_motor();
    zero_cross_detected = false;
  }
  
  // Speed control and other tasks
  update_motor_speed();
}

void startup_sequence() {
  // Forced commutation startup
  motor_speed = 150;  // Start with higher duty cycle
  
  for (int i = 0; i < 30; i++) {  // 5 electrical cycles
    commutate_motor();
    delay(5000 / (i + 1));  // Decreasing delay
  }
}

void commutate_motor() {
  // Apply commutation pattern
  digitalWrite(PHASE_A_HIGH, commutation_table[commutation_step][0]);
  digitalWrite(PHASE_A_LOW, commutation_table[commutation_step][1]);
  digitalWrite(PHASE_B_HIGH, commutation_table[commutation_step][2]);
  digitalWrite(PHASE_B_LOW, commutation_table[commutation_step][3]);
  digitalWrite(PHASE_C_HIGH, commutation_table[commutation_step][4]);
  digitalWrite(PHASE_C_LOW, commutation_table[commutation_step][5]);
  
  // Set PWM duty cycle for active high side
  if (commutation_table[commutation_step][0]) {
    analogWrite(PHASE_A_HIGH, motor_speed);
  } else if (commutation_table[commutation_step][2]) {
    analogWrite(PHASE_B_HIGH, motor_speed);
  } else if (commutation_table[commutation_step][4]) {
    analogWrite(PHASE_C_HIGH, motor_speed);
  }
  
  // Advance to next step
  commutation_step = (commutation_step + 1) % 6;
}

void setup_zero_cross_detection() {
  // Configure Timer1 for zero-crossing detection timing
  // This would typically use an interrupt-based approach
  // for more precise timing
}

bool detect_zero_crossing() {
  uint16_t bemf_voltage = analogRead(bemf_channel[commutation_step]);
  uint16_t neutral_voltage = analogRead(VIRTUAL_NEUTRAL);
  
  // Simple zero-crossing detection
  // In practice, you'd want debouncing and filtering
  static bool last_state = false;
  bool current_state = (bemf_voltage > neutral_voltage);
  
  if (current_state != last_state) {
    last_state = current_state;
    return true;
  }
  
  return false;
}

void delay_30_degrees() {
  // Calculate delay based on current motor speed
  // This should be dynamically calculated based on
  // the time between zero crossings
  delayMicroseconds(500);  // Placeholder value
}

void update_motor_speed() {
  // Read throttle input and update motor_speed
  int throttle = analogRead(A4);
  motor_speed = map(throttle, 0, 1023, 50, 255);
}
```

## Advanced Techniques

### Digital Filtering
Implement majority function filtering to reduce noise:

```cpp
#define FILTER_LENGTH 6
bool majority_filter(bool new_sample) {
  static bool filter_buffer[FILTER_LENGTH] = {0};
  static int buffer_index = 0;
  
  // Shift in new sample
  filter_buffer[buffer_index] = new_sample;
  buffer_index = (buffer_index + 1) % FILTER_LENGTH;
  
  // Count true values
  int true_count = 0;
  for (int i = 0; i < FILTER_LENGTH; i++) {
    if (filter_buffer[i]) true_count++;
  }
  
  // Return majority decision
  return (true_count > FILTER_LENGTH / 2);
}
```

### PWM Synchronization
For better noise immunity, sample BEMF during PWM ON time:

```cpp
void sample_bemf_synchronized() {
  // Wait for PWM high period
  while (digitalRead(current_pwm_pin) == LOW);
  delayMicroseconds(10);  // Wait for switching transients
  
  // Sample BEMF
  uint16_t bemf_sample = analogRead(current_bemf_channel);
  
  // Process sample...
}
```

### Adaptive Timing
Calculate commutation timing based on motor speed:

```cpp
void calculate_commutation_timing() {
  static unsigned long last_zero_cross = 0;
  unsigned long current_time = micros();
  unsigned long period = current_time - last_zero_cross;
  
  // 30° delay = period / 12 (360° / 30°)
  commutation_delay = period / 12;
  
  last_zero_cross = current_time;
}
```

## Design Considerations

### Hardware Requirements
1. **Microcontroller**: Sufficient ADC channels and processing power
2. **Gate Drivers**: Isolated high-side drivers (IR2104, IR2101)
3. **MOSFETs**: Fast switching, appropriate voltage/current ratings
4. **Filtering**: RC filters on BEMF inputs to reduce switching noise
5. **Protection**: Overcurrent, overvoltage, and thermal protection

### Software Considerations
1. **Startup**: Forced commutation sequence until sufficient BEMF
2. **Zero-crossing detection**: Noise filtering and debouncing
3. **Timing**: Precise 30° delay after zero-crossing
4. **Speed control**: Closed-loop or open-loop speed regulation
5. **Protection**: Software-based fault detection and handling

### Performance Optimization
1. **Interrupt-driven**: Use interrupts for time-critical operations
2. **Look-up tables**: Pre-calculated commutation patterns
3. **Fixed-point math**: Avoid floating-point calculations
4. **DMA**: Use DMA for ADC sampling if available

## Advantages and Limitations

### Advantages
- **Cost-effective**: No Hall sensors required
- **Reliability**: Fewer components to fail
- **Compact**: Smaller PCB footprint
- **Efficiency**: Good efficiency at medium to high speeds

### Limitations
- **Startup**: Requires forced commutation at low speeds
- **Load sensitivity**: Performance affected by sudden load changes
- **Speed range**: Limited low-speed performance
- **Noise sensitivity**: Susceptible to electrical noise

## Applications
- Fans and blowers
- Pumps and compressors  
- Electric vehicles (auxiliary motors)
- Power tools
- Drones and RC aircraft
- Industrial automation

## Conclusion

Sensorless BEMF control with comparators provides an excellent balance of cost, performance, and reliability for many BLDC motor applications. While it has limitations at very low speeds, the technique works well for applications where the motor operates primarily at medium to high speeds.

The key to successful implementation is careful attention to:
- Proper circuit design with adequate filtering
- Robust software with noise immunity
- Appropriate startup sequences
- Reliable zero-crossing detection

This approach democratizes BLDC motor control by making it accessible with standard microcontrollers and simple analog circuits.