#include "firebase_manager.h"
#include "addons/TokenHelper.h"

FirebaseManager::FirebaseManager()
    : _settings(nullptr), _state(nullptr), _firebaseReady(false), _lastUploadTime(0), _lastStartReceived(0) {}

void FirebaseManager::begin(SystemSettings* settings, SystemState* state) {
    _settings = settings;
    _state = state;
}

void FirebaseManager::setupFirebase() {
    Serial.println("Setting up Firebase...");

    config.api_key = FIREBASE_API_KEY;
    config.database_url = FIREBASE_DATABASE_URL;

    // Sign up / Log in anonymously
    if (Firebase.signUp(&config, &auth, "", "")) {
        Serial.println("Firebase Auth Setup OK");
        _firebaseReady = true;
    } else {
        _firebaseReady = false;
        Serial.printf("Firebase Auth Error: %s\n", config.signer.signupError.message.c_str());
        // Detailed error for debugging
        Serial.printf("Error Reason: %s\n", fbdo.errorReason().c_str());
    }

    // config.token_status_callback = tokenStatusCallback; // Silenced to reduce log spam
    
    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);

    if (_firebaseReady) {
        Serial.println("Firebase Initialized Successfully.");
        Serial.println("Purging stale cloud commands...");
        Firebase.RTDB.deleteNode(&fbdo, "/devices/esp32_controller_1/commands");
        uploadState();
    }
}

void FirebaseManager::handle() {
    // Only run if WiFi is connected
    if (WiFi.status() != WL_CONNECTED) {
        if (_firebaseReady) {
            Serial.println("WiFi Disconnected. Firebase waiting...");
            _firebaseReady = false; 
        }
        return;
    }

    if (!_firebaseReady && (WiFi.status() == WL_CONNECTED)) {
        setupFirebase();
    }

    if (Firebase.ready() && (millis() - _lastUploadTime > UPLOAD_INTERVAL || _lastUploadTime == 0)) {
        _lastUploadTime = millis();
        uploadState();
        
        // Periodically refresh ALL settings from the cloud to catch "Silent Saves" from the website
        static unsigned long lastSettingsRefresh = 0;
        if (millis() - lastSettingsRefresh > 2000) { // FAST REFRESH: Every 2 seconds
            lastSettingsRefresh = millis();
            downloadSettings(); 
        }

        vTaskDelay(pdMS_TO_TICKS(10));
        checkCommands();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void FirebaseManager::triggerUpload() {
    _lastUploadTime = 0; // Force immediate upload on next handle()
}

// Helper to prevent NaN or Infinity from crashing the Firebase JSON parser
static inline double safeFloat(double val) {
    if (isnan(val) || isinf(val)) return 0.0;
    return val;
}

void FirebaseManager::uploadState() {
    String basePath = "/devices/esp32_controller_1/state";
    
    // Create a JSON object to send all state parameters at once
    FirebaseJson json;
    json.set("pressure", safeFloat(_state->pressure));
    json.set("pressurePercent", safeFloat(_state->pressurePercent));
    json.set("normalizedPressure", safeFloat(_state->normalizedPressure));
    json.set("scaledTo3v3", safeFloat(_state->scaledTo3v3));
    json.set("controlVoltage", safeFloat(_state->controlVoltage));
    json.set("pidOutput", safeFloat(_state->pidOutput));
    json.set("dacValue", _state->dacValue); // integer, generally safe
    json.set("isStatic", _state->isStatic);
    json.set("systemActive", _state->systemActive);
    json.set("solenoidState", _state->solenoidState);
    json.set("tankVolume", safeFloat(_settings->tankVolume));
    json.set("airVolume", safeFloat(_state->airVolume));
    json.set("safetyAllowance", safeFloat(_settings->safetyAllowance));
    json.set("workingMaxBar", safeFloat(_settings->workingMaxBar));
    json.set("controlBandPercent", safeFloat(_settings->controlBandPercent));
    json.set("setpointPercent", safeFloat(_state->setpointPercent));
    json.set("sensorConnected", _state->sensorConnected);
    json.set("currentMA", safeFloat(_state->controlCurrent));
    json.set("displayPressure", safeFloat(_state->displayPressure));
    json.set("controlState", _state->controlState);
    json.set("last_seen/.sv", "timestamp"); // Firebase Server-side heartbeat
    
    if (Firebase.RTDB.updateNode(&fbdo, basePath.c_str(), &json)) {
        static unsigned long lastNotify = 0;
        if (millis() - lastNotify > 10000) {
            Serial.printf("State synced to cloud (P:%.2f, User:%.2f, State:%d)\n", _state->pressure, _state->displayPressure, _state->controlState);
            lastNotify = millis();
        }
        static bool firstSync = true;
        if (firstSync) {
            Serial.println(">> Database Connection Established. Monitoring Cloud Commands...");
            firstSync = false;
        }
    } else {
        Serial.print("Firebase Sync Error: ");
        Serial.println(fbdo.errorReason());
    }
}

void FirebaseManager::checkCommands() {
    String commandPath = "/devices/esp32_controller_1/commands";
    static unsigned long lastStartReceived = 0;
    
    if (!Firebase.ready()) return;

    if (Firebase.RTDB.getJSON(&fbdo, commandPath.c_str())) {
        if (fbdo.dataType() == "json") {
            FirebaseJson& json = fbdo.jsonObject();
            FirebaseJsonData jsonData;

            // 1. Start System Command (High Priority)
            json.get(jsonData, "start_system");
            if (jsonData.success && jsonData.boolValue == true) {
                Firebase.RTDB.deleteNode(&fbdo, commandPath + "/start_system"); 
                Firebase.RTDB.deleteNode(&fbdo, commandPath + "/stop_system"); // Immediate clear
                
                Serial.println("Cloud command: start_system (Protecting for 5s)");
                _state->systemActive = true;
                _lastStartReceived = millis(); // Using the class member variable
            }

            // 2. Stop System Command (Ignore if we just started)
            json.get(jsonData, "stop_system");
            if (jsonData.success && jsonData.boolValue == true) {
                Firebase.RTDB.deleteNode(&fbdo, commandPath + "/stop_system");
                
                if (millis() - _lastStartReceived > 5000) { // 5-second hard lock
                    Serial.println("Cloud command: stop_system");
                    _state->systemActive = false;
                } else {
                    Serial.println("!! CLOUD STOP IGNORED: Within 5s start-protection window.");
                }
            }

            // 3. Update Settings Command
            json.get(jsonData, "update_settings");
            if (jsonData.success && jsonData.boolValue == true) {
                Firebase.RTDB.deleteNode(&fbdo, commandPath + "/update_settings");
                Serial.println("Cloud command: update_settings");
                downloadSettings();
            }
            
            // 2. Setpoint Command
            json.get(jsonData, "setpoint");
            if (jsonData.success && jsonData.doubleValue > 0.01f) {
                _settings->setpoint = jsonData.doubleValue;
                Firebase.RTDB.deleteNode(&fbdo, commandPath + "/setpoint");
                Serial.printf("Cloud command: Setpoint updated to %.2f\n", _settings->setpoint);
            }
            
            // 3. PID Parameter Commands
            if (json.get(jsonData, "kp") && jsonData.success) {
                _settings->kp = jsonData.doubleValue;
                Firebase.RTDB.deleteNode(&fbdo, commandPath + "/kp");
                Serial.printf("[CLOUD] Kp updated manually to %.2f\n", _settings->kp);
            }
            if (json.get(jsonData, "ki") && jsonData.success) {
                _settings->ki = jsonData.doubleValue;
                Firebase.RTDB.deleteNode(&fbdo, commandPath + "/ki");
                Serial.printf("[CLOUD] Ki updated manually to %.3f\n", _settings->ki);
            }
            if (json.get(jsonData, "kd") && jsonData.success) {
                _settings->kd = jsonData.doubleValue;
                Firebase.RTDB.deleteNode(&fbdo, commandPath + "/kd");
                Serial.printf("[CLOUD] Kd updated manually to %.4f\n", _settings->kd);
            }
            if (json.get(jsonData, "auto_tune") && jsonData.success) {
                _settings->autoTuningEnabled = jsonData.boolValue;
                Firebase.RTDB.deleteNode(&fbdo, commandPath + "/auto_tune");
                Serial.printf("[CLOUD] Auto-Tune toggled to %s\n", _settings->autoTuningEnabled ? "ON" : "OFF");
            }

            // 4. Toggle Solenoid Command
            json.get(jsonData, "toggle_solenoid");
            if (jsonData.success && jsonData.boolValue == true) {
                Firebase.RTDB.deleteNode(&fbdo, commandPath + "/toggle_solenoid");
                Serial.println("Cloud command: toggle_solenoid");
                _state->solenoidState = !_state->solenoidState;
            }
        }
    }
}

void FirebaseManager::downloadSettings() {
    String basePath = "/devices/esp32_controller_1/settings";
    
    if (Firebase.RTDB.getJSON(&fbdo, basePath.c_str())) {
        FirebaseJson& json = fbdo.jsonObject();
        FirebaseJsonData jsonData;
        
        // ALLOW ALL VALUES (Removed the thresholds to allow zeros)
        json.get(jsonData, "kp");
        if (jsonData.success) _settings->kp = jsonData.doubleValue;
        
        json.get(jsonData, "ki");
        if (jsonData.success) _settings->ki = jsonData.doubleValue;
        
        json.get(jsonData, "kd");
        if (jsonData.success) _settings->kd = jsonData.doubleValue;
        
        json.get(jsonData, "setpoint");
        if (jsonData.success) {
            _settings->setpoint = jsonData.doubleValue;
        }
        
        json.get(jsonData, "minVoltage");
        if (jsonData.success) _settings->minVoltage = jsonData.doubleValue;
        
        json.get(jsonData, "maxVoltage");
        if (jsonData.success) _settings->maxVoltage = jsonData.doubleValue;
        
        json.get(jsonData, "tankVolume");
        if (jsonData.success) _settings->tankVolume = jsonData.intValue;

        json.get(jsonData, "sensorMinV");
        if (jsonData.success) _settings->sensorMinV = jsonData.doubleValue;
        
        json.get(jsonData, "sensorMaxV");
        if (jsonData.success) _settings->sensorMaxV = jsonData.doubleValue;
        
        json.get(jsonData, "sensorMaxBar");
        if (jsonData.success) _settings->sensorMaxBar = jsonData.doubleValue;
        
        json.get(jsonData, "workingMaxBar");
        if (jsonData.success) _settings->workingMaxBar = jsonData.doubleValue;

        json.get(jsonData, "accuracyMinV");
        if (jsonData.success) _settings->accuracyMinV = jsonData.doubleValue;

        json.get(jsonData, "accuracyMaxV");
        if (jsonData.success) _settings->accuracyMaxV = jsonData.doubleValue;

        json.get(jsonData, "safetyAllowance");
        if (jsonData.success) _settings->safetyAllowance = jsonData.doubleValue;

        json.get(jsonData, "controlBandPercent");
        if (jsonData.success) _settings->controlBandPercent = jsonData.doubleValue;
        
        json.get(jsonData, "minOnTimeMS");
        if (jsonData.success) _settings->minOnTimeMS = jsonData.intValue;

        json.get(jsonData, "minOffTimeMS");
        if (jsonData.success) _settings->minOffTimeMS = jsonData.intValue;
        
        Serial.printf("Settings downloaded from Firebase. [Kp:%.2f, Ki:%.3f, Kd:%.4f]\n", 
                      _settings->kp, _settings->ki, _settings->kd);
    } else {
        Serial.println("Failed to read settings: " + fbdo.errorReason());
    }
}
