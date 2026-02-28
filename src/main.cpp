#include <Arduino.h>
#include <Preferences.h>
#include "sensor.h"
#include "pid_controller.h"
#include "webserver.h"
#include "menu.h"
#include "firebase_manager.h"

// Hardware Pins Configuration
// Pressure Sensor: GPIO 34 (Analog Input)
const int PRESSURE_SENSOR_PIN = 34;
// Control Valve: GPIO 25 (DAC Output 1)
const int VALVE_CONTROL_PIN = 25;
// Solenoid Relay Valve: GPIO 5
const int SOLENOID_PIN = 5;
// I2C Pins for LCD: SDA = GPIO 21, SCL = GPIO 22 (Default)
// Keypad Pins: Rows = {13, 12, 14, 27}, Cols = {26, 33, 32, 15}

// Global Objects
PressureSensor sensor(PRESSURE_SENSOR_PIN);
PIDController pid(1.0, 0.1, 0.01);
PressureWebServer webServer;
MenuSystem menu;
Preferences preferences;
FirebaseManager firebaseManager;

// Global State
SystemSettings settings;
SystemState state;

// Timing
unsigned long lastControlLoop = 0;
const int CONTROL_LOOP_MS = 50; // 20Hz PID loop (refined requirement)

void loadSettings() {
    preferences.begin("pressure-ctrl", false);
    settings.kp = preferences.getFloat("kp", 2.0f);
    settings.ki = preferences.getFloat("ki", 0.5f);
    settings.kd = preferences.getFloat("kd", 0.1f);
    settings.setpoint = preferences.getFloat("sp", 1.0f); // default 1.0 BAR
    settings.minVoltage = preferences.getFloat("minV", 0.0f);
    settings.maxVoltage = preferences.getFloat("maxV", 3.1f);
    settings.tankVolume = preferences.getInt("tankVol", 5); // Default 5L
    
    // 4-20mA Sensor Defaults (117 ohm resistor)
    settings.sensorMinV = preferences.getFloat("sMinV", 0.468f);
    settings.sensorMaxV = preferences.getFloat("sMaxV", 2.34f);
    settings.sensorMaxBar = preferences.getFloat("sMaxP", 12.0f);
    settings.workingMaxBar = preferences.getFloat("wMaxP", 2.0f);
    settings.accuracyMinV = preferences.getFloat("aMinV", 0.66f);
    settings.accuracyMaxV = preferences.getFloat("aMaxV", 3.3f);
    settings.safetyAllowance = preferences.getFloat("safeAllow", 0.5f);
    String ssid = preferences.getString("wifiSSID", "");
    strncpy(settings.wifiSSID, ssid.c_str(), sizeof(settings.wifiSSID) - 1);
    settings.wifiSSID[sizeof(settings.wifiSSID) - 1] = '\0';
    String pass = preferences.getString("wifiPass", "");
    strncpy(settings.wifiPassword, pass.c_str(), sizeof(settings.wifiPassword) - 1);
    settings.wifiPassword[sizeof(settings.wifiPassword) - 1] = '\0';
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
    preferences.putFloat("sMinV", settings.sensorMinV);
    preferences.putFloat("sMaxV", settings.sensorMaxV);
    preferences.putFloat("sMaxP", settings.sensorMaxBar);
    preferences.putFloat("wMaxP", settings.workingMaxBar);
    preferences.putFloat("aMinV", settings.accuracyMinV);
    preferences.putFloat("aMaxV", settings.accuracyMaxV);
    preferences.putFloat("safeAllow", settings.safetyAllowance);
    preferences.putString("wifiSSID", settings.wifiSSID);
    preferences.putString("wifiPass", settings.wifiPassword);
    preferences.end();
}

void setup() {
    Serial.begin(115200);
    Serial.println("--- Pressure Control System Initializing ---");

    loadSettings();
    state.systemActive = false;
    state.solenoidState = false;

    // Output Pins Initialization
    pinMode(SOLENOID_PIN, OUTPUT);
    digitalWrite(SOLENOID_PIN, LOW);

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
    Serial.println("Local Dashboard AP Started: PressureControl_AP");
    Serial.print("Local IP: "); Serial.println(WiFi.softAPIP());

    menu.begin(&settings, &state);
    firebaseManager.begin(&settings, &state);

    Serial.println("System Ready.");
}

void loop() {
    unsigned long now = millis();

    // CRITICAL CONTROL LOOP
    if (now - lastControlLoop >= CONTROL_LOOP_MS) {
        lastControlLoop = now;

        // 1. Read & Filter (modular 4-20mA logic)
        state.pressure = sensor.readPressure(
            settings.sensorMinV, 
            settings.sensorMaxV, 
            settings.sensorMaxBar, 
            settings.workingMaxBar,
            settings.accuracyMinV,
            settings.accuracyMaxV,
            state.normalizedPressure,
            state.scaledTo3v3
        );

        // Update pressure percentage (based on working range)
        state.pressurePercent = state.normalizedPressure * 100.0f;
        
        // Air Volume = Pressure (BAR) * Tank Volume (L)
        state.airVolume = state.pressure * (float)settings.tankVolume;

        // 3. Compute PID (BAR units)
        pid.setTunings(settings.kp, settings.ki, settings.kd);
        pid.setOutputLimits(settings.minVoltage, settings.maxVoltage);

        state.pidOutput = pid.compute(settings.setpoint, state.pressure);

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
        const float maxStep = 50.0f;
        if (state.dacValue > currentPWMValue + maxStep) {
            currentPWMValue += maxStep;
        } else if (state.dacValue < currentPWMValue - maxStep) {
            currentPWMValue -= maxStep;
        } else {
            currentPWMValue = state.dacValue;
        }

        // 6. Write to GPIO25
        ledcWrite(0, (uint32_t)currentPWMValue);

        state.dacValue = currentPWMValue;
        state.controlVoltage = (state.dacValue / 1023.0f) * 3.3f;
        state.sensorVoltage = sensor.getRawVoltage();
        state.rawADC = sensor.getFilteredADC();

        // 7. Safety Bypass: Solenoid opens if pressure > setpoint + safetyAllowance
        if (state.pressure > settings.setpoint + settings.safetyAllowance) {
            state.solenoidState = true; 
        }

        // Diagnostic flag
        state.isStatic = pid.isStatic();
        static unsigned long lastPidWarn = 0;
        if (state.systemActive && state.isStatic && (now - lastPidWarn > 5000)) {
            lastPidWarn = now;
            Serial.println("!! WARNING: PID Output Saturated/Stagnant (Check Sensor).");
        }

        // Plotting values (silenced for cleaner logs)
        // Serial.print(targetVoltage); Serial.print(","); Serial.println(currentVoltage);
    }

    // Solenoid Actuation Trigger
    digitalWrite(SOLENOID_PIN, state.solenoidState ? HIGH : LOW);

    // Update Web and Menu
    webServer.handle();
    menu.update();
    firebaseManager.handle();

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

    // PERIODIC SYSTEM STATUS REPORT (Every 5 seconds)
    static unsigned long lastStatusReport = 0;
    if (now - lastStatusReport > 5000) {
        lastStatusReport = now;
        Serial.println("\n--- [ System Status Report ] ---");
        
        // WiFi Status
        Serial.print("WiFi Mode: ");
        if (WiFi.getMode() == WIFI_AP) Serial.println("Access Point Only");
        else if (WiFi.getMode() == WIFI_STA) Serial.println("Station Only");
        else if (WiFi.getMode() == WIFI_AP_STA) Serial.println("AP + Station");
        
        Serial.print("AP SSID: "); Serial.println("PressureControl_AP");
        
        Serial.print("WiFi (STA) Status: ");
        if (WiFi.status() == WL_CONNECTED) {
            Serial.print("CONNECTED (IP: ");
            Serial.print(WiFi.localIP());
            Serial.println(")");
        } else {
            Serial.println("DISCONNECTED / Searching...");
        }

        Serial.print("Solenoid Valve: ");
        Serial.println(state.solenoidState ? "OPEN" : "CLOSED");
        Serial.println("--------------------------------\n");
    }
}
