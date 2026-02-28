#include "firebase_manager.h"
#include "addons/TokenHelper.h"

FirebaseManager::FirebaseManager()
    : _settings(nullptr), _state(nullptr), _firebaseReady(false), _lastUploadTime(0) {}

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
        Serial.print("Base Path: ");
        Serial.println("/devices/esp32_controller_1");
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
        checkCommands();
    }
}

void FirebaseManager::uploadState() {
    String basePath = "/devices/esp32_controller_1/state";
    
    // Create a JSON object to send all state parameters at once
    FirebaseJson json;
    json.set("pressure", _state->pressure);
    json.set("pressurePercent", _state->pressurePercent);
    json.set("normalizedPressure", _state->normalizedPressure);
    json.set("scaledTo3v3", _state->scaledTo3v3);
    json.set("controlVoltage", _state->controlVoltage);
    json.set("pidOutput", _state->pidOutput);
    json.set("dacValue", _state->dacValue);
    json.set("isStatic", _state->isStatic);
    json.set("systemActive", _state->systemActive);
    json.set("solenoidState", _state->solenoidState);
    json.set("tankVolume", _settings->tankVolume);
    json.set("airVolume", _state->airVolume);
    json.set("safetyAllowance", _settings->safetyAllowance);
    
    if (Firebase.RTDB.updateNode(&fbdo, basePath.c_str(), &json)) {
        static unsigned long lastNotify = 0;
        if (millis() - lastNotify > 10000) {
            Serial.printf("State synced to cloud (P:%.2f, SOL:%d)\n", _state->pressure, _state->solenoidState);
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
    String commandPath = "/devices/esp32_controller_1/commands/update_settings";
    
    if (Firebase.RTDB.getBool(&fbdo, commandPath.c_str())) {
        if (fbdo.dataType() == "boolean" && fbdo.boolData() == true) {
            Serial.println("Cloud command received: update_settings");
            downloadSettings();
            
            // acknowledge command
            Firebase.RTDB.setBool(&fbdo, commandPath.c_str(), false);
        }
    }
    
    String startPath = "/devices/esp32_controller_1/commands/start_system";
    if (Firebase.RTDB.getBool(&fbdo, startPath.c_str())) {
        if (fbdo.dataType() == "boolean" && fbdo.boolData() == true) {
            Serial.println("Cloud command received: start_system");
            _state->systemActive = true;
            Firebase.RTDB.setBool(&fbdo, startPath.c_str(), false);
            uploadState(); // Immediate sync back
        }
    }
    
    String stopPath = "/devices/esp32_controller_1/commands/stop_system";
    if (Firebase.RTDB.getBool(&fbdo, stopPath.c_str())) {
        if (fbdo.dataType() == "boolean" && fbdo.boolData() == true) {
            Serial.println("Cloud command received: stop_system");
            _state->systemActive = false;
            Firebase.RTDB.setBool(&fbdo, stopPath.c_str(), false);
            uploadState(); // Immediate sync back
        }
    }

    String solPath = "/devices/esp32_controller_1/commands/toggle_solenoid";
    if (Firebase.RTDB.getBool(&fbdo, solPath.c_str())) {
        if (fbdo.dataType() == "boolean" && fbdo.boolData() == true) {
            Serial.println("Cloud command received: toggle_solenoid");
            _state->solenoidState = !_state->solenoidState;
            Firebase.RTDB.setBool(&fbdo, solPath.c_str(), false);
            uploadState(); // Immediate sync back
        }
    }

    // Periodic state upload (every 2 seconds)
    static unsigned long lastUpload = 0;
    if (millis() - lastUpload > 2000) {
        lastUpload = millis();
        uploadState();
    }
}

void FirebaseManager::downloadSettings() {
    String basePath = "/devices/esp32_controller_1/settings";
    
    if (Firebase.RTDB.getJSON(&fbdo, basePath.c_str())) {
        FirebaseJson& json = fbdo.jsonObject();
        FirebaseJsonData jsonData;
        
        json.get(jsonData, "kp");
        if (jsonData.success) _settings->kp = jsonData.doubleValue;
        
        json.get(jsonData, "ki");
        if (jsonData.success) _settings->ki = jsonData.doubleValue;
        
        json.get(jsonData, "kd");
        if (jsonData.success) _settings->kd = jsonData.doubleValue;
        
        json.get(jsonData, "setpoint");
        if (jsonData.success) _settings->setpoint = jsonData.doubleValue;
        
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
        
        Serial.println("Settings downloaded from Firebase.");
    } else {
        Serial.println("Failed to read settings: " + fbdo.errorReason());
    }
}
