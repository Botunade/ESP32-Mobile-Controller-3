# Pressure Control System Firmware

## Hardware Pinout (ESP32 Dev Module)

### Sensors & Actuators
| Component | Pin | Notes |
|-----------|-----|-------|
| Pressure Sensor | **GPIO 34** | Analog Input (12-bit ADC) |
| Control Valve | **GPIO 25** | PWM Output (LEDC) + RC Filter |

**RC Filter Recommendation**: 4.7kΩ resistor and 10µF capacitor to smooth PWM into analog DC.

### Display (20x4 LCD via I2C)
| Component | Pin | Notes |
|-----------|-----|-------|
| SDA | **GPIO 21** | Standard I2C Data |
| SCL | **GPIO 22** | Standard I2C Clock |
| I2C Address | **0x27** | Default for most PCF8574 modules |

### 4x4 Matrix Keypad
| Row/Col | Pin |
|---------|-----|
| Row 1 | **GPIO 13** |
| Row 2 | **GPIO 12** |
| Row 3 | **GPIO 14** |
| Row 4 | **GPIO 27** |
| Col 1 | **GPIO 26** |
| Col 2 | **GPIO 33** |
| Col 3 | **GPIO 32** |
| Col 4 | **GPIO 15** |

## Control Loop
The system runs a 10Hz PID loop:
1. Read & Filter ADC (16x oversampling + Moving Average + Exponential Smoothing).
2. Calculate PSI (mapped for 1-5V sensor, 0-150 PSI user range).
3. Compute PID output with anti-windup and derivative filtering.
4. Apply Soft Ramp limiting to DAC output.
5. Write to DAC (GPIO 25).
6. Update LCD and Web Dashboard.

## Tuning Guide
If the output voltage starts oscillating:
1. **Reduce Ki**: Integral gain can cause overshoot and oscillation if too high.
2. **Reduce Kp**: If the system reacts too violently to small errors.
3. **Increase Kd**: Derivative gain can help dampen oscillations, but too much can introduce noise.
4. **Increase Sample Time**: If the ESP32 can't keep up (current is 50ms).

## Features
- **WiFi Access Point**: `PressureControl_AP` (IP: 192.168.4.1)
- **Captive Portal**: Dashboard opens automatically when connected.
- **Live Graphing**: Real-time pressure visualization via Chart.js.
- **NVS Storage**: All PID and system parameters are saved to Flash.
- **Diagnostics**: Detects and warns if the PID output becomes static.
