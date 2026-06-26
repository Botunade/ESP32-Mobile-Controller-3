#include <Arduino.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <esp_system.h>
#include "sensor.h"
#include "pressure_webserver.h"
#include "menu.h"
#include "firebase_manager.h"
#include "digital_follower.h"
#include <QuickPID.h>

// Hardware Pins Configuration
// Pressure Sensor: GPIO 35 (Analog Input)
const int PRESSURE_SENSOR_PIN = 35;
// Control Valve: Move to Pin 18 (safe digital output, avoids ADC2 Wi-Fi conflict)
const int VALVE_CONTROL_PIN = 18; 
// Solenoid Relay Valve: GPIO 5
const int SOLENOID_PIN = 5;
// I2C Pins for LCD: SDA = GPIO 21, SCL = GPIO 22 (Default)
// Keypad Pins: Rows = {13, 12, 14, 27}, Cols = {26, 33, 32, 15}

// Global Objects
PressureSensor sensor(PRESSURE_SENSOR_PIN, 64); // Increased to 64 samples for 0.05 Bar stability
PressureWebServer webServer;
MenuSystem menu;
Preferences preferences;
FirebaseManager firebaseManager;
float pidInput, pidOutput, pidSetpoint;
QuickPID myPID(&pidInput, &pidOutput, &pidSetpoint);

// Global State
SystemSettings settings;
SystemState state;
SystemSettings lastSavedSettings; // Track for changes

// Forward Declarations
void firebaseTask(void* pvParameters);
void controlLoopTask(void* pvParameters);

// Timing
unsigned long lastControlLoop = 0;
const int CONTROL_LOOP_MS = 10; // 100Hz loop for stability and keypad response

void loadSettings() {
    Serial.println("[NVS] Loading settings...");
    preferences.begin("pressure-ctrl", false);
    settings.kp = preferences.getFloat("kp", 1.0f);
    settings.ki = preferences.getFloat("ki", 3.0f);
    settings.kd = preferences.getFloat("kd", 0.0f);
    settings.setpoint = preferences.getFloat("setpoint", 1.0f);
    if (settings.setpoint < 0.05f) settings.setpoint = 1.0f; // Recovery Guard
    settings.minVoltage = preferences.getFloat("minV", 0.0f);
    settings.maxVoltage = preferences.getFloat("maxV", 3.1f);
    settings.tankVolume = preferences.getInt("tankVol", 5); // Default 5L
    
    // 4-20mA Sensor Defaults (77.5 ohm theorized resistor)
    settings.sensorMinV = preferences.getFloat("sMinV", 0.310f); // theorized zero
    settings.sensorMaxV = preferences.getFloat("sMaxV", 1.550f); // 1.240V span (16mA)
    settings.sensorMaxBar = preferences.getFloat("sMaxP", 12.0f);
    settings.workingMaxBar = preferences.getFloat("wMaxP", 2.0f);
    settings.accuracyMinV = preferences.getFloat("aMinV", 0.66f);
    settings.accuracyMaxV = preferences.getFloat("aMaxV", 3.3f);
    settings.safetyAllowance = preferences.getFloat("safeAllow", 0.5f);
    settings.controlBandPercent = preferences.getFloat("bandPerc", 5.0f);
    settings.minOnTimeMS = preferences.getInt("minOn", 2000);
    settings.minOffTimeMS = preferences.getInt("minOff", 50);
    settings.exhaustBurstMS = preferences.getInt("exBurst", 10);
    String ssid = preferences.getString("wifiSSID", "");
    strncpy(settings.wifiSSID, ssid.c_str(), sizeof(settings.wifiSSID) - 1);
    settings.wifiSSID[sizeof(settings.wifiSSID) - 1] = '\0';
    String pass = preferences.getString("wifiPass", "");
    strncpy(settings.wifiPassword, pass.c_str(), sizeof(settings.wifiPassword) - 1);
    settings.wifiPassword[sizeof(settings.wifiPassword) - 1] = '\0';
    
    settings.autoTuningEnabled = preferences.getBool("autoTune", false); // Default to OFF
    settings.tuningStep = preferences.getFloat("tuneStep", 0.01f);
    settings.valveFloor = preferences.getFloat("vFloor", 0.0f); // 0.0 = Needs Calibration
    settings.simulationMode = preferences.getBool("simMode", true); // Default to simulation for safety
    preferences.end();
}

void saveSettings() {
    Serial.println("[NVS] Saving settings...");
    preferences.begin("pressure-ctrl", false);
    preferences.putFloat("kp", settings.kp);
    preferences.putFloat("ki", settings.ki);
    preferences.putFloat("kd", settings.kd);
    preferences.putFloat("setpoint", settings.setpoint); // Standardized key
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
    preferences.putFloat("bandPerc", settings.controlBandPercent);
    preferences.putInt("minOn", settings.minOnTimeMS);
    preferences.putInt("minOff", settings.minOffTimeMS);
    preferences.putInt("exBurst", settings.exhaustBurstMS);
    preferences.putString("wifiSSID", settings.wifiSSID);
    preferences.putString("wifiPass", settings.wifiPassword);
    preferences.putBool("autoTune", settings.autoTuningEnabled);
    preferences.putFloat("tuneStep", settings.tuningStep);
    preferences.putFloat("vFloor", settings.valveFloor);
    preferences.putBool("simMode", settings.simulationMode);
    preferences.end();
}

void printSystemDiagnostics() {
    Serial.println("\n===== [ ADAPTIVE PID DIAGNOSTICS ] =====");
    // Calculate live terms for display
    float pContrib = settings.kp * (settings.setpoint - state.pressure);
    float iContrib = settings.ki * state.integral;
    
    // Check if auto-tuning is currently active based on zone
    float delta = 0.08f * settings.setpoint;
    bool inPidZone = (state.pressure >= (settings.setpoint - delta)) && (state.pressure <= (settings.setpoint + delta));
    const char* atStatus = !settings.autoTuningEnabled ? "OFF" : (inPidZone ? "ACTIVE" : "STANDBY");

    Serial.printf("  [PID] Kp:%.2f Ki:%.3f Kd:%.4f | AT:%s\n", 
                  settings.kp, settings.ki, settings.kd, atStatus);
    Serial.printf("  [TRM] P_Term:%.1f | I_Term:%.1f | D_Term:%.1f | SP:%.1f\n",
                  state.pTerm, state.iTerm, state.dTerm, settings.setpoint);
    Serial.printf("  [ADP] Cycle:%d ms | Error:%.3f | Integral:%.3f | Rate:%.3f B/s\n", 
                  (int)state.cycleTime, (settings.setpoint - state.pressure), state.integral, state.rate);
    Serial.printf("  [SEN] Raw:%.2fV (ADC:%d) | P:%.1f BAR (Display:%.1f)\n", 
                  sensor.getRawVoltage(), (int)sensor.getFilteredADC(), state.pressure, state.displayPressure);
    Serial.printf("  [OUT] Control:%.2fV (DAC:%d) | Current:%.2f mA | Output:%s (%d%%)\n", 
                  state.controlVoltage, (int)state.dacValue, state.controlCurrent, 
                  state.outputState ? "ON" : "OFF", (int)state.pidOutput);
    Serial.printf("  [STA] Active:%s | Solenoid:%s | Sensor:%s\n", 
                  state.systemActive ? "RUNNING" : "IDLE", 
                  state.solenoidState ? "OPEN" : "CLOSED",
                  state.sensorConnected ? "OK" : "DISCONNECTED");
    Serial.printf("  [KEY] Last Key Pressed: [%c]\n", menu.getLastKey());
    Serial.printf("  [WIF] SSID:%s | IP:%s | RSSI:%d dBm\n", 
                  strlen(settings.wifiSSID) > 0 ? settings.wifiSSID : "NONE",
                  WiFi.localIP().toString().c_str(),
                  WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0);
    Serial.println("==========================================\n");
}

void setup() {
    Serial.begin(115200);
    delay(500); // Give serial some time
    Serial.println("\n\n--- [BOOT] Pressure Control System ---");
    
    // Diagnostic: Print Reset Reason
    esp_reset_reason_t reason = esp_reset_reason();
    Serial.print("Reset Reason: ");
    switch (reason) {
        case ESP_RST_POWERON: Serial.println("Power On / Reset Button"); break;
        case ESP_RST_EXT:     Serial.println("External Pin Reset"); break;
        case ESP_RST_SW:      Serial.println("Software Reset"); break;
        case ESP_RST_PANIC:   Serial.println("Software Panic / Crash"); break;
        case ESP_RST_INT_WDT: Serial.println("Interrupt Watchdog"); break;
        case ESP_RST_TASK_WDT:Serial.println("Task Watchdog"); break;
        case ESP_RST_WDT:     Serial.println("Other Watchdog"); break;
        case ESP_RST_BROWNOUT:Serial.println("BROWNOUT (Voltage Drop detected!)"); break;
        default:              Serial.printf("Other (%d)\n", reason); break;
    }

    loadSettings();
    
    lastSavedSettings = settings; 
    state.systemActive = false;
    state.solenoidState = false;
    state.cycleTime = 800; // Default idle heartbeat

    // 1. IMMEDIATE HARDWARE INIT
    pinMode(SOLENOID_PIN, OUTPUT);
    pinMode(VALVE_CONTROL_PIN, OUTPUT);
    digitalWrite(SOLENOID_PIN, LOW);
    digitalWrite(VALVE_CONTROL_PIN, LOW);
    digitalFollower_init();

    sensor.begin();
    menu.begin(&settings, &state); // Start the LCD/Keypad immediately

    state.pressure = sensor.readPressure(
        settings.sensorMinV, settings.sensorMaxV, settings.sensorMaxBar, settings.workingMaxBar,
        settings.accuracyMinV, settings.accuracyMaxV, state.normalizedPressure, state.scaledTo3v3
    );
    state.displayPressure = state.pressure;
    state.prevPressure = state.pressure;
    state.controlState = 0; // IDLE

    // Initialize QuickPID Engine
    myPID.SetTunings(settings.kp, settings.ki, settings.kd);
    myPID.SetOutputLimits(55.0f, 75.0f); // 75% Ceiling to protect ITV hardware (Max 20mA)
    myPID.SetSampleTimeUs(CONTROL_LOOP_MS * 1000); // 10ms in microseconds
    myPID.SetMode(myPID.Control::automatic); // Turn it on

    // 2. START THE HEART (Core 1) - Do this BEFORE WiFi
    xTaskCreatePinnedToCore(
        controlLoopTask, 
        "ControlTask", 
        10240, 
        NULL, 
        10, 
        NULL, 
        1
    );

    // 3. START BACKGROUND INTERNET (Core 0)
    // We tell WiFi to start, but we DON'T wait for it.
    WiFi.softAP("PressureControl_AP", "12345678");
    if (strlen(settings.wifiSSID) > 0) {
        WiFi.begin(settings.wifiSSID, settings.wifiPassword);
        Serial.print("Connecting to WiFi: "); Serial.println(settings.wifiSSID);
    }

    webServer.begin(&settings, &state);
    Serial.println("Local Dashboard AP Started: PressureControl_AP");
    Serial.print("Local IP: "); Serial.println(WiFi.softAPIP());

    xTaskCreatePinnedToCore(
        firebaseTask,
        "CloudTask",
        16384,
        NULL,
        0,
        NULL,
        0
    );

    Serial.println("System Online (Offline Mode Active)");
    printSystemDiagnostics();
}

// High-priority control task to ensure PID and Safety logic run even if Firebase hangs
void controlLoopTask(void* pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(10); 
    const TickType_t xSafeFrequency = (xFrequency > 0) ? xFrequency : 1;

    static bool lastSystemActive = false;
    static unsigned long windowStartTime = 0;
    const unsigned long windowSizeMs = 40; // 25Hz execution window

    for (;;) {
        // Handle startup transition
        if (state.systemActive && !lastSystemActive) {
            Serial.println(">>> TRACE: systemActive changed FALSE -> TRUE");
            state.integral = 0;
            state.prevError = 0;
            windowStartTime = millis();
            myPID.Reset();
            state.controlState = 0; 
            Serial.println("[SYSTEM] Started.");
        } else if (!state.systemActive && lastSystemActive) {
            Serial.println(">>> TRACE: systemActive changed TRUE -> FALSE");
        }

        // ====================================================================
        // 1. SENSOR INTEGRATION (HARDWARE OR SIMULATION)
        // ====================================================================
        if (settings.simulationMode) {
            // DIGITAL TRANSMITTER SIMULATION (FEEDFORWARD ONLY)
            if (state.systemActive) {
                // Smoothly move towards target setpoint
                float riseSpeed = 0.02f;
                state.pressure = state.pressure + (riseSpeed * (settings.setpoint - state.pressure));

                // Add tiny industrial signal noise for dashboard realism
                float noise = ((float)random(-3, 4) / 1000.0f);
                state.pressure += noise;
                if (state.pressure < 0.0f) state.pressure = 0.0f;
            } else {
                // Decay pressure when stopped
                if (state.pressure > 0.02f) {
                    state.pressure = state.pressure - (0.05f * state.pressure);
                } else {
                    state.pressure = 0.0f;
                }
            }
            state.sensorConnected = true; // Simulated always "connected"

            // Calculate simulated raw sensor voltage and raw ADC based on a 4-20mA sensor profile
            const float V_ZERO = 0.3100f;
            const float V_SPAN = 1.2400f;
            const float MAX_P  = 12.000f;
            state.sensorVoltage = V_ZERO + (state.pressure * V_SPAN / MAX_P);
            if (state.sensorVoltage < 0.0f) state.sensorVoltage = 0.0f;
            state.rawADC = (state.sensorVoltage / 3.3f) * 4095.0f;

        } else {
            // ACTUAL HARDWARE READ
            state.pressure = sensor.readPressure(
                settings.sensorMinV, settings.sensorMaxV, settings.sensorMaxBar, settings.workingMaxBar,
                settings.accuracyMinV, settings.accuracyMaxV, state.normalizedPressure, state.scaledTo3v3
            );
            state.sensorConnected = sensor.isConnected();
            state.sensorVoltage = sensor.getRawVoltage();
            state.rawADC = sensor.getFilteredADC();
        }

        // Manual Calibration / Ramp Test logic
        static bool calibrationRunning = false;
        static unsigned long rampStart = 0;

        if (state.forceCalibration) {
            if (!calibrationRunning) {
                rampStart = millis();
                calibrationRunning = true;
            }

            unsigned long elapsed = millis() - rampStart;
            if (elapsed < 5000) {
                state.pressure = (float)elapsed * (settings.setpoint / 5000.0f);
            } else {
                state.pressure = settings.setpoint;
                state.forceCalibration = false;
                calibrationRunning = false;
            }
        } else {
            calibrationRunning = false;
        }

        // --- Recalculate derived metrics (Ensures calibration/simulation/hardware are all consistent) ---
        if (settings.workingMaxBar > 0) {
            state.normalizedPressure = state.pressure / settings.workingMaxBar;
        } else {
            state.normalizedPressure = state.pressure / 1.0f;
        }
        if (state.normalizedPressure > 1.0f) state.normalizedPressure = 1.0f;
        if (state.normalizedPressure < 0.0f) state.normalizedPressure = 0.0f;
        state.scaledTo3v3 = settings.accuracyMinV + (state.normalizedPressure * (settings.accuracyMaxV - settings.accuracyMinV));

        // Sync raw metrics during calibration ramp for dashboard realism
        if (state.forceCalibration) {
            const float V_ZERO = 0.3100f;
            const float V_SPAN = 1.2400f;
            const float MAX_P  = 12.000f;
            state.sensorVoltage = V_ZERO + (state.pressure * V_SPAN / MAX_P);
            state.rawADC = (state.sensorVoltage / 3.3f) * 4095.0f;
        }

        // Scale percentages for display variables
        if (settings.workingMaxBar > 0) {
            state.pressurePercent = (state.pressure / settings.workingMaxBar) * 100.0f;
            state.setpointPercent = (settings.setpoint / settings.workingMaxBar) * 100.0f;
        }
        state.airVolume = state.pressure * (float)settings.tankVolume;

        // ====================================================================
        // 2. PID CORE COMPUTATION
        // ====================================================================
        if (state.systemActive) {
            // Maintain dynamic floor bounds cleanly
            float dynamicFloor = (20.0f * settings.setpoint) + 35.0f;
            if (dynamicFloor < 48.0f) dynamicFloor = 48.0f;
            if (dynamicFloor > 70.0f) dynamicFloor = 70.0f;

            myPID.SetOutputLimits(dynamicFloor, 75.0f);
            myPID.SetMode(myPID.Control::automatic);
            
            if (myPID.GetKp() != settings.kp || myPID.GetKi() != settings.ki || myPID.GetKd() != settings.kd) {
                myPID.SetTunings(settings.kp, settings.ki, settings.kd);
            }

            pidSetpoint = settings.setpoint;
            pidInput = state.pressure; 
            myPID.Compute(); 
            
            state.pTerm = myPID.GetPterm();
            state.iTerm = myPID.GetIterm();
            state.dTerm = myPID.GetDterm();
            state.integral = myPID.GetIterm();
            
            state.pidOutput = pidOutput;
        } else {
            state.pidOutput = 0.0f;
            state.integral = 0;
            myPID.SetMode(myPID.Control::manual);
            pidOutput = 0.0f;
        }

        // ====================================================================
        // 3. HARDWARE ACTUATION & ANALOG TRACKING LOGIC
        // ====================================================================
        unsigned long now = millis();
        if (now - windowStartTime >= windowSizeMs) {
            windowStartTime = now;
        }

        // Map the real-time duty cycle cleanly
        unsigned long onTimeMs = (unsigned long)((state.pidOutput / 100.0f) * windowSizeMs);

        if (state.systemActive && ((now - windowStartTime) < onTimeMs)) {
            digitalWrite(VALVE_CONTROL_PIN, HIGH);
            state.outputState = true;
        } else {
            digitalWrite(VALVE_CONTROL_PIN, LOW);
            state.outputState = false;
        }

        // PREVENT DASHBOARD FLICKER: Calculate voltage/current based on true target duty, 
        // not the instantaneous state of the high-speed switching pin!
        if (state.systemActive) {
            // Smoothly track actual voltage out of your low-pass filter / DAC
            state.controlVoltage = (state.pidOutput / 100.0f) * 3.3f;
            // Perfect 4-20mA Conversion Math: 4mA floor + (16mA span * duty ratio)
            state.controlCurrent = 4.0f + ((state.pidOutput / 100.0f) * 16.0f);
            state.dacValue = (int)((state.pidOutput / 100.0f) * 1023.0f);
            state.controlState = 1;
        } else {
            state.controlVoltage = 0.0f;
            state.controlCurrent = 0.0f;
            state.dacValue = 0;
            state.controlState = 0;
        }


        state.prevError = settings.setpoint - state.pressure;

        // ====================================================================
        // 4. SAFETY SYSTEMS & UI SMOOTHING
        // ====================================================================
        if (state.pressure > settings.setpoint + settings.safetyAllowance) {
            state.solenoidState = true; 
        } else if (state.pressure <= settings.setpoint) {
            state.solenoidState = false; // Add auto-reset for defense stability
        }
        digitalWrite(SOLENOID_PIN, state.solenoidState ? HIGH : LOW);

        // Display Smoothing
        float displayDiff = abs(state.pressure - settings.setpoint);
        float targetDisplay = state.pressure;
        if (displayDiff < 0.05f && state.systemActive) {
            targetDisplay = settings.setpoint; 
        }
        
        float displayAlpha = 0.2f;
        state.displayPressure = (displayAlpha * targetDisplay) + (1.0f - displayAlpha) * state.displayPressure;

        // Periodic Diagnostic Logger
        static int diagCount = 0;
        if (++diagCount % 100 == 0) { 
            Serial.printf("[ADP] SP:%.1f PV:%.1fB Out:%s (%d%%) Volts:%.2fV Current:%.2fmA\n", 
                          settings.setpoint, state.displayPressure, 
                          state.outputState ? "ON" : "OFF", (int)state.pidOutput,
                          state.controlVoltage, state.controlCurrent);
        }

        digitalFollower_update(state, settings);

        vTaskDelayUntil(&xLastWakeTime, xSafeFrequency);
        lastSystemActive = state.systemActive; 
    }
}

void loop() {
    unsigned long now = millis();

    // 1. Slow Tasks (WiFi, Firebase, Menu, WebServer)
    webServer.handle();
    menu.update();

        // 3. Save USER settings periodically (Ignore auto-tuning jitter for cloud sync)
        static SystemSettings lastUserManualSettings;
        static bool firstRun = true;
        static unsigned long lastSave = 0;
        if (firstRun) { lastUserManualSettings = settings; firstRun = false; }

        if (now - lastSave > 10000) { // Check every 10 seconds
            lastSave = now;
            // Only trigger cloud and NVS if the core USER settings changed (Setpoint, PID, Tank, etc)
            if (settings.setpoint != lastUserManualSettings.setpoint || 
                settings.kp != lastUserManualSettings.kp ||
                settings.ki != lastUserManualSettings.ki ||
                settings.kd != lastUserManualSettings.kd ||
                settings.workingMaxBar != lastUserManualSettings.workingMaxBar ||
                settings.tankVolume != lastUserManualSettings.tankVolume) {
                
                // 1. Save to NVS so they survive a reboot
                saveSettings();
                
                // 2. Update the QuickPID engine!
                myPID.SetTunings(settings.kp, settings.ki, settings.kd);
                
                // 3. Update our tracker
                lastUserManualSettings = settings;
                firebaseManager.triggerUpload();
            } else if (memcmp(&settings, &lastSavedSettings, sizeof(SystemSettings)) != 0) {
                // Just save auto-tuned gains to NVS, don't force cloud
                saveSettings();
                lastSavedSettings = settings;
            }
        }

    // 4. PERIODIC SYSTEM STATUS REPORT (Every 5 seconds)
    static unsigned long lastStatusReport = 0;
    if (now - lastStatusReport > 5000) {
        lastStatusReport = now;
        Serial.println("[SYSTEM] Heartbeat: Main Loop is alive.");
        printSystemDiagnostics();
    }
}

// Low-Priority Cloud Task (Core 0)
void firebaseTask(void* pvParameters) {
    bool timeSynced = false;
    bool firebaseStarted = false;
    bool ntpRequested = false;

    for (;;) {
        // Only try to sync time if WiFi is connected and we haven't done it yet
        if (WiFi.status() == WL_CONNECTED && !timeSynced) {
            if (!ntpRequested) {
                configTime(0, 0, "pool.ntp.org", "time.nist.gov");
                ntpRequested = true;
            }
            time_t now = time(nullptr);
            if (now > 100000) { // Check if we actually got a real date
                timeSynced = true;
                Serial.println("[CLOUD] Time Synced. Initializing Firebase...");
            }
        }

        // Only start Firebase if we have time sync
        if (timeSynced && !firebaseStarted) {
            firebaseManager.begin(&settings, &state);
            firebaseStarted = true;
        }

        // Run the cloud updates only if everything is ready
        if (firebaseStarted) {
            firebaseManager.handle();
        }

        vTaskDelay(pdMS_TO_TICKS(500)); // Check every half-second
    }
}
