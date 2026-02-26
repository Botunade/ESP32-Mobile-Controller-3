#include <Arduino.h>
#include <Preferences.h>
#include "sensor.h"
#include "pid_controller.h"
#include "webserver.h"
#include "menu.h"

// Hardware Pins
const int PRESSURE_SENSOR_PIN = 34;
const int VALVE_CONTROL_PIN = 25; // DAC1

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

void loadSettings() {
    preferences.begin("pressure-ctrl", false);
    // PID
    settings.kp = preferences.getFloat("kp", 1.0f);
    settings.ki = preferences.getFloat("ki", 0.1f);
    settings.kd = preferences.getFloat("kd", 0.01f);
    settings.sampleTime = preferences.getInt("sampleTime", 50);

    // Pressure Control
    settings.setpoint = preferences.getFloat("setpoint", 50.0f);
    settings.maxPressure = preferences.getFloat("maxPress", 150.0f);
    settings.units = preferences.getInt("units", 0);

    // Output
    settings.minVoltage = preferences.getFloat("minV", 0.0f);
    settings.maxVoltage = preferences.getFloat("maxV", 3.3f);
    settings.rampRate = preferences.getFloat("rampRate", 50.0f);
    settings.calibrationFactor = preferences.getFloat("calFact", 1.0f);

    // Tank
    settings.tankVolume = preferences.getInt("tankVol", 100);

    // Sensor Calibration
    settings.lowVoltage = preferences.getFloat("lowV", 0.434f);
    settings.highVoltage = preferences.getFloat("highV", 0.712f);
    preferences.end();
}

void saveSettings() {
    preferences.begin("pressure-ctrl", false);
    preferences.putFloat("kp", settings.kp);
    preferences.putFloat("ki", settings.ki);
    preferences.putFloat("kd", settings.kd);
    preferences.putInt("sampleTime", settings.sampleTime);
    preferences.putFloat("setpoint", settings.setpoint);
    preferences.putFloat("maxPress", settings.maxPressure);
    preferences.putInt("units", settings.units);
    preferences.putFloat("minV", settings.minVoltage);
    preferences.putFloat("maxV", settings.maxVoltage);
    preferences.putFloat("rampRate", settings.rampRate);
    preferences.putFloat("calFact", settings.calibrationFactor);
    preferences.putInt("tankVol", settings.tankVolume);
    preferences.putFloat("lowV", settings.lowVoltage);
    preferences.putFloat("highV", settings.highVoltage);
    preferences.end();
}

void setup() {
    Serial.begin(115200);
    Serial.println("--- Pressure Control System Initializing ---");

    loadSettings();
    state.systemActive = false;

    sensor.begin();

    pid.setTunings(settings.kp, settings.ki, settings.kd);
    pid.setOutputLimits(settings.minVoltage, settings.maxVoltage);
    pid.setSampleTime(settings.sampleTime);

    webServer.begin(&settings, &state);
    menu.begin(&settings, &state);

    Serial.println("System Ready.");
}

void loop() {
    unsigned long now = millis();

    // CRITICAL CONTROL LOOP
    if (now - lastControlLoop >= settings.sampleTime) {
        lastControlLoop = now;

        // 1. Read ADC, Oversample, Filter, and Map to 0-100%
        state.pressurePercent = sensor.readPressure(settings.lowVoltage, settings.highVoltage, settings.calibrationFactor);

        // Map pressure percentage to current units for display and PID
        if (settings.units == 1) { // PSI
            state.pressure = (state.pressurePercent / 100.0f) * settings.maxPressure;
        } else if (settings.units == 2) { // BAR (approx 1 BAR = 14.5 PSI)
            state.pressure = (state.pressurePercent / 100.0f) * (settings.maxPressure / 14.5038f);
        } else { // %
            state.pressure = state.pressurePercent;
        }

        // 3. Compute PID
        pid.setTunings(settings.kp, settings.ki, settings.kd);
        pid.setOutputLimits(settings.minVoltage, settings.maxVoltage);
        pid.setSampleTime(settings.sampleTime);

        // Normalize PID: Target and Input should be in the same scale (Voltage)
        // Setpoint is in current units, convert to percent then to voltage
        float setpointPercent = 0;
        if (settings.units == 1) setpointPercent = (settings.setpoint / settings.maxPressure) * 100.0f;
        else if (settings.units == 2) setpointPercent = (settings.setpoint / (settings.maxPressure / 14.5038f)) * 100.0f;
        else setpointPercent = settings.setpoint;

        float targetVoltage = settings.lowVoltage + (setpointPercent / 100.0f) * (settings.highVoltage - settings.lowVoltage);
        float currentVoltage = sensor.getRawVoltage(settings.calibrationFactor);

        state.pidOutput = pid.compute(targetVoltage, currentVoltage);

        // 4. Output Logic
        if (state.systemActive) {
            float targetOutput = state.pidOutput;

            // Soft Ramp Limiting
            static float currentOutputVoltage = 0;
            float maxStep = (settings.rampRate * settings.sampleTime) / 1000.0f;

            if (targetOutput > currentOutputVoltage + maxStep) {
                currentOutputVoltage += maxStep;
            } else if (targetOutput < currentOutputVoltage - maxStep) {
                currentOutputVoltage -= maxStep;
            } else {
                currentOutputVoltage = targetOutput;
            }

            state.controlVoltage = currentOutputVoltage;
            // Map voltage (0-3.3V) to DAC (0-255)
            state.dacValue = (state.controlVoltage / 3.3f) * 255.0f;
            if (state.dacValue > 255) state.dacValue = 255;
            if (state.dacValue < 0) state.dacValue = 0;
        } else {
            state.dacValue = 0;
            state.controlVoltage = 0;
        }

        // Write to DAC1 (GPIO 25)
        dacWrite(VALVE_CONTROL_PIN, (uint8_t)state.dacValue);

        // Diagnostics
        state.isStatic = pid.isStatic();
        if (state.isStatic && abs(targetVoltage - currentVoltage) > 0.01f) {
            Serial.println("WARNING: PID output static with nonzero error");
        }

        // Serial Debug: Setpoint, pressure %, PID output, DAC value
        Serial.print(settings.setpoint, 2);
        Serial.print(",");
        Serial.print(state.pressurePercent, 2);
        Serial.print(",");
        Serial.print(state.pidOutput, 3);
        Serial.print(",");
        Serial.println(state.dacValue);
    }

    // Update Web and Menu
    webServer.handle();
    menu.update();

    // Save settings if they have changed (immediate save as per requirement)
    static SystemSettings lastSavedSettings = {0};
    if (memcmp(&settings, &lastSavedSettings, sizeof(SystemSettings)) != 0) {
        saveSettings();
        memcpy(&lastSavedSettings, &settings, sizeof(SystemSettings));
        Serial.println("Settings saved to NVS.");
    }
}
