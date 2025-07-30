# Hardware Setup Guide for Sensorless BEMF BLDC Controller

## Circuit Overview

This guide describes the hardware implementation of a sensorless BLDC motor controller using BEMF detection with comparators. The design uses an Arduino Uno as the control unit with external gate drivers and MOSFETs for the power stage.

## Complete Circuit Diagram (ASCII Art)

```
                    +12V Motor Supply
                         |
                    +----+----+
                    |    |    |
                   [C]  [B]  [A]  <-- Motor Phases
                    |    |    |
              +-----+    |    +-----+
              |          |          |
              |     +----+----+     |
              |     |         |     |
              |    Q6         Q4    Q2  <-- High Side MOSFETs
              |     |         |     |
              +-----+----+----+-----+
                    |    |    |
            Gate    |    |    |    Gate Drivers
            Drive   |    |    |    (IR2104 x3)
                    |    |    |
              +-----+    |    +-----+
              |          |          |
              |         Q5         Q1  <-- Low Side MOSFETs
              |          |          |
              +----------+----------+
                         |
                        GND

Arduino Connections:
Pin 11 (OC2A) --> Phase A Gate Driver HIN
Pin 10 (OC1B) --> Phase B Gate Driver HIN  
Pin 9  (OC1A) --> Phase C Gate Driver HIN
Pin 5        --> Phase A Gate Driver LIN
Pin 4        --> Phase B Gate Driver LIN
Pin 3        --> Phase C Gate Driver LIN

BEMF Sensing Network:
Pin 6 (AIN0) --> Virtual Neutral Point
Pin 7 (AIN1) --> Phase A BEMF (via voltage divider)
Pin A2       --> Phase B BEMF (via voltage divider)
Pin A3       --> Phase C BEMF (via voltage divider)
Pin A4       --> Throttle Input (0-5V)
```

## Component List

### Power Stage Components
| Component | Specification | Quantity | Notes |
|-----------|---------------|----------|-------|
| MOSFETs | IRFZ44N (55V, 49A) | 6 | Or equivalent N-channel |
| Gate Drivers | IR2104 | 3 | Half-bridge gate driver |
| Bootstrap Diodes | 1N4148 | 3 | Fast switching diodes |
| Bootstrap Caps | 10µF/25V | 3 | Ceramic or electrolytic |
| Gate Resistors | 10Ω | 6 | Current limiting |

### Control Circuit Components
| Component | Specification | Quantity | Notes |
|-----------|---------------|----------|-------|
| Microcontroller | Arduino Uno/Nano | 1 | ATmega328P based |
| BEMF Resistors | 33kΩ, 1/4W | 6 | Voltage divider high side |
| BEMF Resistors | 10kΩ, 1/4W | 3 | Voltage divider low side |
| Filter Capacitors | 100nF | 3 | BEMF noise filtering |
| Pull-up Resistors | 10kΩ | 3 | Gate driver inputs |

### Power Supply Components
| Component | Specification | Quantity | Notes |
|-----------|---------------|----------|-------|
| Motor Supply | 12V, 10A+ | 1 | Depends on motor rating |
| Logic Supply | 5V, 1A | 1 | For Arduino and drivers |
| Bulk Capacitor | 1000µF/25V | 1 | Power supply filtering |
| Ceramic Caps | 100nF | 6 | Local decoupling |

## Detailed Circuit Sections

### 1. Power Stage (Three-Phase Bridge)

Each phase consists of two MOSFETs in a half-bridge configuration:

```
     +12V
       |
       +-- Q_High (IRFZ44N)
       |     |
Phase  +-----+
Output |     |
       +-- Q_Low (IRFZ44N)
       |
      GND
```

### 2. Gate Driver Circuit (IR2104)

```
IR2104 Pinout:
Pin 1 (LO)  --> Low side MOSFET gate
Pin 2 (HO)  --> High side MOSFET gate  
Pin 3 (VB)  --> Bootstrap voltage (+12V via diode)
Pin 4 (VS)  --> Phase output node
Pin 5 (LIN) --> Arduino low side control
Pin 6 (HIN) --> Arduino high side PWM
Pin 7 (VDD) --> +5V logic supply
Pin 8 (VCC) --> +12V power supply

Bootstrap Circuit:
+12V --[1N4148]--+--[10µF]--VB (Pin 3)
                 |
                VS (Pin 4)
```

### 3. BEMF Sensing Circuit

For each motor phase:
```
Motor Phase A --[33kΩ]--+-- To Arduino Pin 7 (AIN1)
                        |
                      [10kΩ]--[100nF]
                        |       |
                       GND     GND

Virtual Neutral Point:
Phase A --[33kΩ]--+
Phase B --[33kΩ]--+-- To Arduino Pin 6 (AIN0)
Phase C --[33kΩ]--+
```

### 4. Power Supply Circuit

```
AC Input --> Bridge Rectifier --> Filter Cap --> 12V Reg --> Motor Supply
                                      |
                                   [1000µF]
                                      |
                                     GND

12V --> 5V Regulator (7805) --> Arduino + Gate Drivers
             |
          [100µF]
             |
            GND
```

## PCB Layout Considerations

### 1. Power Stage Layout
- Keep high-current traces short and wide (minimum 2mm width)
- Use separate ground planes for power and signal
- Place gate drivers close to MOSFETs
- Add thermal vias under MOSFETs for heat dissipation

### 2. Signal Routing
- Route BEMF sensing lines away from switching nodes
- Use ground guard traces around analog signals
- Keep Arduino crystal away from switching circuits
- Add ferrite beads on power supply lines

### 3. Component Placement
- Place bootstrap components close to gate drivers
- Keep decoupling capacitors near IC power pins
- Separate analog and digital sections
- Provide adequate spacing for heat sinks

## Assembly Instructions

### Step 1: Power Supply Section
1. Install bridge rectifier and filter capacitors
2. Mount voltage regulators with heat sinks
3. Add decoupling capacitors near regulators
4. Test output voltages before proceeding

### Step 2: Gate Driver Section
1. Mount IR2104 ICs on PCB
2. Install bootstrap diodes and capacitors
3. Add pull-up resistors on control inputs
4. Connect power supply connections

### Step 3: Power MOSFETs
1. Mount MOSFETs with appropriate heat sinks
2. Use thermal compound for good heat transfer
3. Connect source, drain, and gate connections
4. Double-check for short circuits

### Step 4: BEMF Sensing
1. Install voltage divider resistors (33kΩ/10kΩ)
2. Add filter capacitors (100nF)
3. Route sensing lines to Arduino pins
4. Test voltage levels with multimeter

### Step 5: Control Section
1. Mount Arduino Uno/Nano
2. Connect PWM outputs to gate drivers
3. Connect BEMF sensing inputs
4. Add throttle input connector

## Testing and Calibration

### Initial Power-On Tests
1. **Power Supply Check**
   - Verify 12V motor supply
   - Verify 5V logic supply
   - Check for proper ground connections

2. **Gate Driver Test**
   - Apply PWM signals to inputs
   - Verify gate drive outputs with oscilloscope
   - Check bootstrap voltage levels

3. **BEMF Sensing Test**
   - Manually rotate motor
   - Observe BEMF signals on oscilloscope
   - Verify virtual neutral point voltage

### Motor Testing Procedure
1. **Safety First**
   - Use current-limited power supply
   - Have emergency stop readily available
   - Ensure motor is securely mounted

2. **Startup Test**
   - Load Arduino code
   - Set throttle to minimum
   - Power on system
   - Gradually increase throttle

3. **Performance Tuning**
   - Adjust PWM frequency if needed
   - Tune startup sequence timing
   - Optimize zero-crossing detection

## Troubleshooting Guide

### Common Issues and Solutions

1. **Motor Won't Start**
   - Check power supply connections
   - Verify gate driver outputs
   - Ensure correct commutation sequence
   - Check for short circuits in windings

2. **Erratic Operation**
   - Check BEMF sensing circuit
   - Verify virtual neutral point
   - Add more filtering to BEMF signals
   - Check for loose connections

3. **Overheating**
   - Reduce PWM duty cycle
   - Check for shoot-through current
   - Improve heat sinking
   - Verify proper gate drive timing

4. **Poor Speed Control**
   - Calibrate throttle input range
   - Adjust speed control algorithm
   - Check for electrical noise
   - Verify zero-crossing detection

## Safety Considerations

### Electrical Safety
- Use proper fusing on power supplies
- Implement overcurrent protection
- Add emergency stop functionality
- Ensure proper grounding

### Mechanical Safety
- Secure motor mounting
- Use guards around rotating parts
- Implement software speed limits
- Add thermal protection

### EMI/RFI Considerations
- Use shielded cables for motor connections
- Add ferrite cores on power lines
- Implement proper PCB ground planes
- Consider using spread-spectrum PWM

## Performance Optimization

### Software Optimizations
- Use interrupt-driven zero-crossing detection
- Implement adaptive timing algorithms
- Add closed-loop speed control
- Optimize PWM frequency for efficiency

### Hardware Optimizations
- Use faster MOSFETs for higher frequencies
- Implement synchronous rectification
- Add current sensing for torque control
- Use higher resolution ADC for better BEMF sensing

## Conclusion

This hardware setup provides a solid foundation for sensorless BLDC motor control using BEMF detection. The modular design allows for easy modification and upgrading of individual sections. With proper assembly and testing, this controller can achieve good performance for a wide range of BLDC motor applications.

Remember to always prioritize safety during assembly and testing, and don't hesitate to start with lower power levels when first testing the system.