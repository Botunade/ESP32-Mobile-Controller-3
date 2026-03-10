# ESP32 Mobile Controller & Cloud Pressure System

A comprehensive industrial-grade pressure control system built on the ESP32. This system features real-time local and cloud telemetry, autonomous PID control, a 4-20mA pressure transmitter interface, and multi-channel safety overrides.

## 🛠 Hardware Pinout (ESP32 Dev Module)

### Core Components & Actuators
| Component | Pin | Notes |
|-----------|-----|-------|
| **4-20mA Pressure Sensor** | **GPIO 34** | Analog Input (12-bit ADC). Drops across 117Ω resistor (0.468V - 2.34V) |
| **Control Valve** | **GPIO 25** | PWM Output (LEDC) + RC Filter (4.7kΩ + 10µF) for Analog DC Conversion |
| **Solenoid Valve**| **GPIO 5**  | Digital Relay Output (Main Safety / Vent Valve) |
| **Digital Follower**| **GPIO 4** | Hidden ON/OFF Output (Mirrors PID active intent for external relays) |

*(Note: GPIO 2 is deliberately unused to prevent hardware shorts caused by the built-in LED drawing excessive current).*

### Display & Input (Local UI)
| Component | Pin(s) | Notes |
|-----------|--------|-------|
| **20x4 LCD** | **SCL: 22, SDA: 21** | I2C Address: 0x27 |
| **4x4 Keypad** | **R: 13, 12, 14, 27** | **C: 26, 33, 32, 15** |

---

## 🎛 Keypad Interface Guide

The 4x4 matrix keypad is the primary physical interface for configuring the system locally.

### Mode Selection (Letters)
- **`A` (Home):** Returns to the main telemetry display (Live Pressure, Solenoid, System Status).
- **`B` (Vessel Size):** Enters the Tank Configuration menu.
- **`C` (PID Tuning):** Enters the PID Parameters menu (Kp, Ki, Kd edit).
- **`D` (Settings):** Enters the Setpoint & Voltage Scaling menu.

### System Control (Symbols)
- **`*` (Start):** Enables the PID loop. System becomes ACTIVE and attempts to reach the Setpoint.
- **`#` (Stop/E-Stop):** Disables the PID loop, returns to Home, and forces Solenoid CLOSED.

### Navigation & Editing (Numbers)
*When inside a configuration menu (B, C, or D):*
- **`2` (Up):** Move cursor UP to the previous parameter.
- **`8` (Down):** Move cursor DOWN to the next parameter.
- **`4` (Decrease):** Decrement the selected value.
- **`6` (Increase):** Increment the selected value.

---

## 🚀 System Architecture & Capabilities

### 1. Advanced Pressure Sensing & Scaling (BAR)
- The system operates entirely in **BAR metric units**.
- Calculates pressure from a **4-20mA transmitter** (Hardware: 117Ω shunt resistor).
- **ADC Calibration:** Fully configurable voltage boundaries (`sensorMinV`: 0.468V, `sensorMaxV`: 2.34V) mapping to the sensor's physical range (`sensorMaxBar`: 12.0 BAR).
- **Accuracy Out:** The MCU provides a secondary high-resolution scale (mapped 0.66V - 3.3V) indicating the ratio of current pressure to the `workingMaxBar`.

### 2. Triple-Redundant Safety System
1.  **PID Ramping:** The Control Valve (GPIO 25) output is smoothly ramped to prevent violent pressure surges inside the vessel.
2.  **Solenoid Overpressure Bypass:** If live pressure exceeds the user's `setpoint` by the `safetyAllowance` (Default: +0.5 BAR), the Solenoid acts as an immediate pressure relief and opens automatically.
3.  **Stall Detection:** The system monitors the PID output and warns via the dashboard if the controller saturates (Valve 100% open but pressure isn't rising).

### 3. Multi-Tier Dashboards
The system features two synchronization UI layers:

#### **Local Captive Portal Dashboard (`192.168.4.1`)**
- Broadcasts as **PressureControl_AP**.
- Provides a direct HTML interface indicating Network Setup, WiFi Credentials Config, array of live ADC/Voltage telemetry, and a **Live Trend Graph** (Chart.js plotting physical pressure over time).
- Features NVS persistence (saves configs to non-volatile flash) and auto-reboots when network settings are saved.

#### **Firebase Real-time Cloud Dashboard**
- Hosted securely on Firebase Hosting (`pressure-control-17b6e.web.app`).
- **Low-Latency Streams:** Upgraded from polling delays to **Firebase Server-Sent Events (Streams)**. Pushing a command (like Solenoid Toggle or System Start) from the web executes on the physical hardware in under 500ms.
- **Float Sanitization:** Telemetry floats are sanitized to prevent `NaN` or `Infinity` JSON crashes on the Firebase backend during sensor disconnects.
- Instant, visual mirrored updates of the LCD's PID and Solenoid states anywhere in the world. 

---
### Getting Started (PlatformIO)
1. Install VS Code and the **PlatformIO** extension.
2. Open this folder.
3. Execute `PlatformIO: Upload` (`env:esp32dev`).
4. (Optional) Run `npm install` and `firebase deploy` within `pressure-control-dashboard/` to push changes to the Web interface.
