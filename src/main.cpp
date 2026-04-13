#include <Arduino.h>
#include <Preferences.h>
#include "digital_follower.h"
#include "firebase_manager.h"
#include "menu.h"
#include "pid_controller.h"
#include "sensor.h"
#include "webserver.h"

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
  settings.kp = preferences.getFloat("kp", 0.5f); // Smoother for visual PID
  settings.ki = preferences.getFloat("ki", 0.1f);
  settings.kd = preferences.getFloat("kd", 0.05f);
  settings.setpoint = preferences.getFloat("sp", 1.0f);
  settings.spPercent = preferences.getFloat("spPerc", 0.0f);
  settings.minVoltage = preferences.getFloat("minV", 0.48f); // Offset 4mA
  settings.maxVoltage = preferences.getFloat("maxV", 2.40f); // Max 20mA
  settings.tankVolume = preferences.getInt("tankVol", 30);   // 30L Vessel

  // 4-20mA Sensor Defaults
  settings.sensorMinV = preferences.getFloat("sMinV", 0.468f);
  settings.sensorMaxV = preferences.getFloat("sMaxV", 2.34f);
  settings.sensorMaxBar = preferences.getFloat("sMaxP", 12.0f);
  settings.workingMaxBar = preferences.getFloat("wMaxP", 6.0f); // 6.0 Bar Max
  settings.accuracyMinV = preferences.getFloat("aMinV", 0.66f);
  settings.accuracyMaxV = preferences.getFloat("aMaxV", 3.3f);
  settings.safetyAllowance = preferences.getFloat("safeAllow", 0.5f);
  settings.deadband = preferences.getFloat("dband", 0.5f);
  String ssid = preferences.getString("wifiSSID", "");
  strncpy(settings.wifiSSID, ssid.c_str(), sizeof(settings.wifiSSID) - 1);
  settings.wifiSSID[sizeof(settings.wifiSSID) - 1] = '\0';
  String pass = preferences.getString("wifiPass", "");
  strncpy(settings.wifiPassword, pass.c_str(), sizeof(settings.wifiPassword) - 1);
  settings.wifiPassword[sizeof(settings.wifiPassword) - 1] = '\0';

  // --- HARD OVERRIDE FOR NEW HARDWARE CONFIG ---
  settings.tankVolume = 30;     // Forces 30L
  settings.workingMaxBar = 6.0f; // Forces 6.0 Bar Max
  // ----------------------------------------------
  
  preferences.end();
}

void saveSettings() {
  preferences.begin("pressure-ctrl", false);
  preferences.putFloat("kp", settings.kp);
  preferences.putFloat("ki", settings.ki);
  preferences.putFloat("kd", settings.kd);
  preferences.putFloat("setpoint", settings.setpoint);
  preferences.putFloat("spPerc", settings.spPercent);
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
  preferences.putFloat("dband", settings.deadband);
  preferences.putString("wifiSSID", settings.wifiSSID);
  preferences.putString("wifiPass", settings.wifiPassword);
  preferences.end();
}

void setup() {
  Serial.begin(115200);
  delay(1000); 
  Serial.println("\n--- Pressure Control System Initializing ---");

  loadSettings();
  state.systemActive = false;
  state.solenoidState = false;

  pinMode(SOLENOID_PIN, OUTPUT);
  digitalWrite(SOLENOID_PIN, LOW);
  digitalFollower_init();

  sensor.begin();

  const int freq = 5000;
  const int resolution = 10; 
  const int channel = 0;
  ledcSetup(channel, freq, resolution);
  ledcAttachPin(VALVE_CONTROL_PIN, channel);

  pid.setTunings(settings.kp, settings.ki, settings.kd);
  pid.setOutputLimits(settings.minVoltage, settings.maxVoltage);
  pid.setSampleTime(CONTROL_LOOP_MS);

  webServer.begin(&settings, &state);
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
        settings.sensorMinV, settings.sensorMaxV, settings.sensorMaxBar,
        settings.workingMaxBar, settings.accuracyMinV, settings.accuracyMaxV,
        state.normalizedPressure, state.scaledTo3v3);

    // Update display percentages (0-100% relative to workingMaxBar)
    state.displayPV = (state.pressure / settings.workingMaxBar) * 100.0f;
    if (state.displayPV > 100.0f)
      state.displayPV = 100.0f;
    if (state.displayPV < 0.0f)
      state.displayPV = 0.0f;

    state.displaySP = settings.spPercent;

    // Air Volume = Pressure (BAR) * Tank Volume (L)
    state.airVolume = state.pressure * (float)settings.tankVolume;

    // --- TRACK 2: VISUAL SIMULATION (PID) ---
    if (state.systemActive) {
      pid.setTunings(settings.kp, settings.ki, settings.kd);
      pid.setOutputLimits(0.0f, 100.0f); // Display is 0-100%
      state.displayOUT = pid.compute(state.displaySP, state.displayPV);
    } else {
      state.displayOUT = 0.0f;
      pid.reset(); // Clear integral and history to allow clean start
    }

    // --- TRACK 1: PHYSICAL HARDWARE (BANG-BANG) ---
    float targetBar = (settings.spPercent / 100.0f) * settings.workingMaxBar;
    static float lastHardwareVoltage = settings.minVoltage;

    if (state.systemActive) {
      if (state.pressure < targetBar - settings.deadband) {
        state.controlVoltage = settings.maxVoltage; // Open (2.40V)
      } else if (state.pressure > targetBar + settings.deadband) {
        state.controlVoltage = settings.minVoltage; // Close (0.48V)
      } else {
        state.controlVoltage = lastHardwareVoltage; // Hold state (Hysteresis)
      }
    } else {
      state.controlVoltage = settings.minVoltage; // Shutdown state
    }
    lastHardwareVoltage = state.controlVoltage;

    // Map voltage to DAC (0-1023)
    // Formula: (V / 3.3) * 1023
    state.dacValue = (int)((state.controlVoltage / 3.3f) * 1023.0f);
    if (state.dacValue > 1023)
      state.dacValue = 1023;
    if (state.dacValue < 0)
      state.dacValue = 0;

    // 5. Soft Ramp Limiting (Reduced for Bang-Bang responsiveness but kept for
    // safety)
    static float currentPWMValue = 0;
    const float maxStep = 100.0f; // Snappier for Bang-Bang
    if (state.dacValue > currentPWMValue + maxStep) {
      currentPWMValue += maxStep;
    } else if (state.dacValue < currentPWMValue - maxStep) {
      currentPWMValue -= maxStep;
    } else {
      currentPWMValue = state.dacValue;
    }

    // 6. Write to GPIO25 (Channel 0)
    ledcWrite(0, (uint32_t)currentPWMValue);

    state.dacValue = currentPWMValue;
    state.sensorVoltage = sensor.getRawVoltage();
    state.rawADC = sensor.getFilteredADC();

    // 7. Safety Bypass: Solenoid opens if pressure > targetBar +
    // safetyAllowance
    if (state.pressure > targetBar + settings.safetyAllowance) {
      state.solenoidState = true;
    }

    // Diagnostic flag
    state.isStatic = pid.isStatic();
    static unsigned long lastPidWarn = 0;
    if (state.systemActive && state.isStatic && (now - lastPidWarn > 5000)) {
      lastPidWarn = now;
      Serial.println(
          "!! WARNING: PID Output Saturated/Stagnant (Check Sensor).");
    }

    // Plotting values (silenced for cleaner logs)
    // Serial.print(targetVoltage); Serial.print(",");
    // Serial.println(currentVoltage);

    digitalFollower_update(state, settings);
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
    if (WiFi.getMode() == WIFI_AP)
      Serial.println("Access Point Only");
    else if (WiFi.getMode() == WIFI_STA)
      Serial.println("Station Only");
    else if (WiFi.getMode() == WIFI_AP_STA)
      Serial.println("AP + Station");

    Serial.print("AP SSID: ");
    Serial.println("PressureControl_AP");

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
