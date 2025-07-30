# Sensorless BEMF Motor Control System

A high-performance sensorless BLDC motor control system using Back EMF (BEMF) detection with comparators only, optimized for 24MHz MCU operation.

## Features

- **Sensorless Operation**: Uses BEMF zero-crossing detection instead of Hall sensors
- **Comparator-Only Design**: No ADC required, uses only analog comparators
- **24MHz Optimized**: Specifically designed for 24MHz MCU operation
- **Six-Step Commutation**: Standard BLDC motor control with 120° electrical commutation
- **Smooth Startup**: Gradual acceleration from standstill using timed commutation
- **Speed Control**: Variable speed control with smooth ramping
- **Emergency Protection**: Overcurrent and stall detection with automatic shutdown
- **Minimal Hardware**: Requires only comparators, no complex sensor feedback

## System Requirements

### Hardware
- **MCU**: AVR microcontroller (ATmega328P recommended) running at 24MHz
- **Motor**: 3-phase BLDC motor
- **Power Stage**: 3-phase inverter (MOSFETs + gate drivers)
- **Comparators**: 3x analog comparators for BEMF detection
- **Crystal**: 24MHz external crystal oscillator

### Software
- AVR-GCC toolchain
- AVRDUDE for programming
- Make utility

## Architecture

### Core Components

1. **BEMF Detection (`bemf_sensorless.c/h`)**
   - Zero-crossing detection using comparators
   - Six-step commutation logic
   - Startup sequence management
   - Speed calculation and control

2. **MCU Configuration (`mcu_config.h`)**
   - 24MHz clock setup
   - Timer configuration for PWM and system tick
   - Comparator initialization
   - GPIO pin definitions

3. **Main Application (`main.c`)**
   - User interface (button, LED, potentiometer)
   - Speed ramping and control
   - Emergency stop functionality
   - System monitoring

## Technical Specifications

- **MCU Frequency**: 24MHz
- **PWM Frequency**: 20kHz
- **Commutation Method**: Six-step (120° electrical)
- **BEMF Detection**: Comparator-based zero-crossing
- **Startup Method**: Timed commutation sequence
- **Speed Range**: Configurable (typically 500-5000 RPM)
- **Control Resolution**: 16-bit PWM (65536 steps)

## Pin Configuration

### Motor Control Pins
```
PA0 - Phase A High-side control
PA1 - Phase B High-side control  
PA2 - Phase C High-side control
PB1 - Phase A PWM output (OC1A)
PB2 - Phase B PWM output (OC1B)
PB3 - Phase C PWM output (OC1C)
```

### BEMF Sensing Pins
```
PA4 - Phase A BEMF input (AIN0+)
PA5 - Neutral point reference (AIN1-)
PA6 - Phase B BEMF input (AIN2+)
PA7 - Phase C BEMF input (AIN3+)
```

### User Interface
```
PC2 - Start/Stop button (active low)
PC3 - Status LED
PC4 - Speed control potentiometer (ADC4)
```

## Building the Project

### Prerequisites
Install the AVR toolchain:
```bash
# Ubuntu/Debian
sudo apt-get install gcc-avr binutils-avr avr-libc avrdude

# macOS with Homebrew
brew install avr-gcc avrdude

# Windows
# Install WinAVR or Atmel Studio
```

### Compilation
```bash
# Build the project
make all

# Build with debug symbols
make debug

# Build optimized release
make release

# Show memory usage
make size

# Clean build files
make clean
```

### Programming
```bash
# Program with USBasp
make program-usbasp

# Program with Arduino as ISP
make program-arduino

# Set fuses for 24MHz external crystal
make fuses

# Read current fuse settings
make read-fuses
```

## Usage

### Basic Operation

1. **Power On**: Connect power to the system
2. **Initialize**: MCU initializes and blinks LED slowly
3. **Start Motor**: Press the start/stop button
4. **Speed Control**: Adjust potentiometer to change speed
5. **Stop Motor**: Press the start/stop button again

### LED Status Indicators
- **Slow Blink**: System ready, motor stopped
- **Fast Blink**: Motor running normally
- **Solid On**: Emergency stop or fault condition

### Speed Control
- **Potentiometer**: Real-time speed adjustment (0-100%)
- **Ramping**: Smooth acceleration/deceleration
- **Limits**: Configurable minimum and maximum speeds

## Configuration

### Motor Parameters
Edit `bemf_sensorless.h` to adjust motor-specific parameters:

```c
#define STARTUP_PWM_DUTY    512   // Startup power level
#define MAX_PWM_DUTY        2048  // Maximum speed
#define MIN_PWM_DUTY        256   // Minimum speed  
#define STARTUP_STEPS       100   // Startup duration
#define BEMF_BLANK_TIME     50    // BEMF blanking period
```

### Timing Parameters
Adjust timing for different motor characteristics:

```c
#define PWM_FREQUENCY       20000 // PWM frequency (Hz)
#define ZERO_CROSS_TIMEOUT  5000  // Stall detection timeout
```

## Theory of Operation

### BEMF Detection
When a BLDC motor rotates, it generates Back EMF in the non-energized phase. This BEMF crosses zero at the optimal commutation point (30° after the previous commutation). The system uses comparators to detect these zero-crossings and trigger the next commutation step.

### Six-Step Commutation
The motor uses a six-step commutation sequence:
1. A+ B- (C floating)
2. A+ C- (B floating)  
3. B+ C- (A floating)
4. B+ A- (C floating)
5. C+ A- (B floating)
6. C+ B- (A floating)

### Startup Sequence
Since BEMF is only generated when the motor is rotating, a special startup sequence is used:
1. **Initial Positioning**: Brief energization to align rotor
2. **Timed Commutation**: Fixed-time commutation steps to accelerate
3. **BEMF Transition**: Switch to BEMF-based commutation when sufficient speed is reached

## Performance Optimization

### 24MHz Specific Optimizations
- **Timer Prescaling**: Optimized for 24MHz operation
- **PWM Resolution**: Maximum resolution at 20kHz PWM frequency
- **Interrupt Timing**: Minimized interrupt latency
- **Delay Loops**: Calibrated for precise timing

### Memory Usage
- **Program Memory**: ~4KB (typical)
- **RAM Usage**: ~256 bytes
- **Stack Depth**: ~64 bytes maximum

## Troubleshooting

### Common Issues

**Motor Won't Start**
- Check power connections
- Verify motor phase wiring
- Ensure 24MHz crystal is working
- Check comparator reference voltage

**Erratic Operation**
- Adjust BEMF blanking time
- Check for electrical noise
- Verify PWM frequency settings
- Inspect motor connections

**Stalling at High Speed**
- Increase commutation advance angle
- Check power supply capacity
- Verify BEMF detection sensitivity
- Reduce maximum speed limit

### Debug Features
Enable debug mode for additional diagnostics:
```bash
make debug
```

This enables:
- Extended error checking
- Performance monitoring
- Communication via UART (if available)

## Safety Considerations

- **Overcurrent Protection**: Implement hardware current limiting
- **Overvoltage Protection**: Use appropriate power supply regulation
- **Emergency Stop**: Always provide manual emergency stop capability
- **Thermal Protection**: Monitor power stage temperature
- **Proper Grounding**: Ensure all grounds are properly connected

## Advanced Features

### Optional Enhancements
- **Closed-Loop Speed Control**: PID controller for precise speed regulation
- **Current Sensing**: Monitor motor current for efficiency optimization
- **Temperature Compensation**: Adjust parameters based on temperature
- **Fault Diagnostics**: Comprehensive fault detection and reporting

### Customization
The system is designed to be easily customizable:
- **Motor Types**: Adapt for different BLDC motor configurations
- **Control Methods**: Implement sinusoidal or space vector modulation
- **Communication**: Add UART, SPI, or I2C interfaces
- **Sensors**: Integrate additional sensors for enhanced control

## License

This project is provided as-is for educational and development purposes. Modify and use according to your requirements.

## Contributing

Contributions are welcome! Please:
1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

## Support

For questions and support:
- Check the troubleshooting section
- Review the code comments
- Consult AVR documentation
- Test with known-good hardware

---

**Note**: This implementation is optimized for 24MHz operation but can be adapted for other frequencies by adjusting the timing constants in `mcu_config.h`.
