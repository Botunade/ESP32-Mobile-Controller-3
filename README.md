# Pressure Control System Firmware

## Hardware Pinout (ESP32 Dev Module)

### Sensors & Actuators
| Component | Pin | Notes |
|-----------|-----|-------|
| Pressure Sensor | **GPIO 34** | Analog Input (12-bit ADC) |
| Control Valve | **GPIO 25** | PWM Output (LEDC) + RC Filter |
| **Solenoid Valve**| **GPIO 5**  | Relay / Actuator Control (Manual Override) |
| **WiFi Status LED**| **GPIO 2** | Built-in Blue LED (Blinks=Searching, Solid=Linked) |

**RC Filter Recommendation**: 4.7kΩ resistor and 10µF capacitor to smooth PWM into analog DC.

### Display & Input
| Component | Pin(s) | Notes |
|-----------|--------|-------|
| 20x4 LCD (I2C) | **SCL:22, SDA:21** | I2C Address: 0x27 |
| 4x4 Keypad | **Rows:13,12,14,27** | Cols: 26,33,32,15 |

## Software Architecture

### 1. Control Loop (10Hz)
- **Oversampling**: 16x ADC reading + Moving Average filter.
- **PID Compute**: Real-time compute with anti-windup and derivative smoothing.
- **Ramping**: Target DAC output is ramped to prevent pressure spikes.

### 2. Multi-Channel Dashboards
- **Local Dashboard**: Accessible via `192.168.4.1` (Captive Portal). Includes real-time Chart.js telemetry.
- **Cloud Dashboard**: Hosted web interface syncing via Firebase Realtime Database.
- **Dynamic Solenoid**: Local dashboard features an "Optimistic UI" toggle for instant valve feedback.

### 3. Cloud Connectivity (Firebase)
- **Auth**: Uses Firebase Anonymous Authentication (Enabled in Console).
- **Commands**: Supports remote `start_system`, `stop_system`, `toggle_solenoid`, and `update_settings`.
- **Latency**: ESP32 provides instant feedback by uploading its state immediately after receiving a cloud command.

## Local Dashboard & AP
- **SSID**: `PressureControl_AP`
- **IP Address**: `192.168.4.1` (Static)
- **Features**: Capture portal for WiFi configuration, NVS setting persistence, and live telemetry.

## Features & Diagnostics
- **Non-blocking WiFi**: System remains responsive even while searching for WiFi.
- **Stall Detection**: Monitors and warns via dashboard if PID output becomes static/saturated.
- **Tank Geometry**: Supports configurable **Tank Volume (L)** and **Tank Height (m)**.
- **Auto-Reboot**: System reboots automatically after saving WiFi credentials to apply changes.
