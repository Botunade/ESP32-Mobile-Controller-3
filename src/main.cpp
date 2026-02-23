#include <Arduino.h>
#include <Preferences.h>
#include "sensor.h"
#include "pid_controller.h"
#include "webserver.h"
#include "menu.h"

// Hardware Pins
const int PRESSURE_SENSOR_PIN = 34;
const int VALVE_CONTROL_PIN = 25; // DAC Output

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
const int CONTROL_LOOP_MS = 100; // 10Hz PID loop

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

    sensor.begin();

    pid.setTunings(settings.kp, settings.ki, settings.kd);
    // Map voltage limits (0-3.3V) to DAC limits (0-255)
    float minDac = (settings.minVoltage / 3.3f) * 255.0f;
    float maxDac = (settings.maxVoltage / 3.3f) * 255.0f;
    pid.setOutputLimits(minDac, maxDac);
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

        // 3. Compute PID
        // Ensure PID tunings and limits are updated if they changed via web/menu
        pid.setTunings(settings.kp, settings.ki, settings.kd);
        float minDac = (settings.minVoltage / 3.3f) * 255.0f;
        float maxDac = (settings.maxVoltage / 3.3f) * 255.0f;
        pid.setOutputLimits(minDac, maxDac);

        state.pidOutput = pid.compute(settings.setpoint, state.pressure);

        // 4. Map to DAC 0-255 (PID output is already in DAC units 0-255)
        state.dacValue = (int)state.pidOutput;
        if (state.dacValue > 255) state.dacValue = 255;
        if (state.dacValue < 0) state.dacValue = 0;

        // 5. Soft Ramp Limiting
        static float currentDacValue = 0;
        const float maxStep = 25.0f; // Limit change to 25 units per 100ms
        if (state.dacValue > currentDacValue + maxStep) {
            currentDacValue += maxStep;
        } else if (state.dacValue < currentDacValue - maxStep) {
            currentDacValue -= maxStep;
        } else {
            currentDacValue = state.dacValue;
        }

        // 6. Write to GPIO25
        dacWrite(VALVE_CONTROL_PIN, (uint8_t)currentDacValue);

        // Calculate control voltage for display
        state.controlVoltage = (state.dacValue / 255.0f) * 3.3f;

        // Diagnostic flag
        state.isStatic = pid.isStatic();
        if (state.isStatic) {
            Serial.println("WARNING: PID output not changing – check scaling");
        }

        // Serial Debug
        Serial.print("P: "); Serial.print(state.pressure);
        Serial.print(" PSI | SP: "); Serial.print(settings.setpoint);
        Serial.print(" | ERR: "); Serial.print(settings.setpoint - state.pressure);
        Serial.print(" | PID: "); Serial.print(state.pidOutput);
        Serial.print(" | DAC: "); Serial.println(state.dacValue);
    }

    // Update Web and Menu
    webServer.handle();
    menu.update();

    // Save settings periodically if changed (simple logic: every 30s)
    static unsigned long lastSave = 0;
    if (now - lastSave > 30000) {
        lastSave = now;
        saveSettings();
    }
}
