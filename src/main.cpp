#include <Arduino.h>
#include <Preferences.h>
#include "sensor.h"
#include "pid_controller.h"
#include "webserver.h"
#include "menu.h"

// Hardware Pins Configuration
// Pressure Sensor: GPIO 34 (Analog Input)
const int PRESSURE_SENSOR_PIN = 34;
// Control Valve: GPIO 25 (DAC Output 1)
const int VALVE_CONTROL_PIN = 25;
// I2C Pins for LCD: SDA = GPIO 21, SCL = GPIO 22 (Default)
// Keypad Pins: Rows = {13, 12, 14, 27}, Cols = {26, 33, 32, 15}

// Global Objects
PressureSensor sensor(PRESSURE_SENSOR_PIN);
PIDController pid(1.0, 0.1, 0.01);
PressureWebServer webServer;
MenuSystem menu;
Preferences preferences;

// Global State
SystemSettings settings;
SystemState state;

// Timing
unsigned long lastControlLoop = 0;
const int CONTROL_LOOP_MS = 50; // 20Hz PID loop (refined requirement)

void loadSettings() {
    preferences.begin("pressure-ctrl", false);
    settings.kp = preferences.getFloat("kp", 1.0f);
    settings.ki = preferences.getFloat("ki", 0.1f);
    settings.kd = preferences.getFloat("kd", 0.01f);
    settings.setpoint = preferences.getFloat("setpoint", 50.0f);
    settings.minVoltage = preferences.getFloat("minV", 0.0f);
    settings.maxVoltage = preferences.getFloat("maxV", 3.3f);
    settings.tankVolume = preferences.getInt("tankVol", 100);
    preferences.end();
}

void saveSettings() {
    preferences.begin("pressure-ctrl", false);
    preferences.putFloat("kp", settings.kp);
    preferences.putFloat("ki", settings.ki);
    preferences.putFloat("kd", settings.kd);
    preferences.putFloat("setpoint", settings.setpoint);
    preferences.putFloat("minV", settings.minVoltage);
    preferences.putFloat("maxV", settings.maxVoltage);
    preferences.putInt("tankVol", settings.tankVolume);
    preferences.end();
}

void setup() {
    Serial.begin(115200);
    Serial.println("--- Pressure Control System Initializing ---");

    loadSettings();
    state.systemActive = false;

    sensor.begin();

    // PWM Setup (LEDC)
    const int freq = 5000;
    const int resolution = 10; // 10-bit (0-1023)
    const int channel = 0;
    ledcSetup(channel, freq, resolution);
    ledcAttachPin(VALVE_CONTROL_PIN, channel);

    pid.setTunings(settings.kp, settings.ki, settings.kd);
    // Normalization: PID operates on 0.0 - 3.3V scale
    pid.setOutputLimits(settings.minVoltage, settings.maxVoltage);
    pid.setSampleTime(CONTROL_LOOP_MS);

    webServer.begin(&settings, &state);
    menu.begin(&settings, &state);

    Serial.println("System Ready.");
}

void loop() {
    unsigned long now = millis();

    // CRITICAL CONTROL LOOP
    if (now - lastControlLoop >= CONTROL_LOOP_MS) {
        lastControlLoop = now;

        // 1. Read & Filter ADC (inside readPressure)
        // 2. Convert to voltage & PSI (inside readPressure)
        state.pressure = sensor.readPressure();

        // Update pressure percentage (based on 0-150 PSI range)
        state.pressurePercent = (state.pressure / 150.0f) * 100.0f;

        // 3. Compute PID (Normalized to 0-3.3V)
        pid.setTunings(settings.kp, settings.ki, settings.kd);
        pid.setOutputLimits(settings.minVoltage, settings.maxVoltage);

        // Map PSI setpoint to Voltage for PID
        float targetVoltage = (((settings.setpoint / 1250.0f) + 1.0f) * (3.3f / 5.0f));
        float currentVoltage = sensor.getRawVoltage();

        state.pidOutput = pid.compute(targetVoltage, currentVoltage);

        // 4. Map to PWM 0-1023
        if (state.systemActive) {
            state.dacValue = (int)((state.pidOutput / 3.3f) * 1023.0f);
            if (state.dacValue > 1023) state.dacValue = 1023;
            if (state.dacValue < 0) state.dacValue = 0;
        } else {
            state.dacValue = 0;
        }

        // 5. Soft Ramp Limiting
        static float currentPWMValue = 0;
        const float maxStep = 50.0f; // Limit change to 50 units per 50ms
        if (state.dacValue > currentPWMValue + maxStep) {
            currentPWMValue += maxStep;
        } else if (state.dacValue < currentPWMValue - maxStep) {
            currentPWMValue -= maxStep;
        } else {
            currentPWMValue = state.dacValue;
        }

        // 6. Write to GPIO25 via LEDC
        ledcWrite(0, (uint32_t)currentPWMValue);

        // Calculate control voltage for display
        state.controlVoltage = (state.dacValue / 1023.0f) * 3.3f;

        // Diagnostic flag
        state.isStatic = pid.isStatic();
        if (state.isStatic) {
            Serial.println("WARNING: PID output not changing – check scaling");
        }

        // Serial Plotter Output: Setpoint (V), Input (V)
        Serial.print(targetVoltage);
        Serial.print(",");
        Serial.println(currentVoltage);
    }

    // Update Web and Menu
    webServer.handle();
    menu.update();

    // Save settings periodically if they have changed
    static unsigned long lastSave = 0;
    static SystemSettings lastSavedSettings = {0};
    if (now - lastSave > 30000) {
        lastSave = now;
        if (memcmp(&settings, &lastSavedSettings, sizeof(SystemSettings)) != 0) {
            saveSettings();
            memcpy(&lastSavedSettings, &settings, sizeof(SystemSettings));
            Serial.println("Settings saved to NVS.");
        }
    }
}
