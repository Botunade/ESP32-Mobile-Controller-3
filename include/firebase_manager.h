#ifndef FIREBASE_MANAGER_H
#define FIREBASE_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <LittleFS.h>
#include <Firebase_ESP_Client.h>
#include "pressure_webserver.h"

// Internal includes for Firebase Manager only
// We will include addons in the CPP file to avoid multiple definition errors

#define FIREBASE_API_KEY "AIzaSyDPPqJp0Gk3BGlV6-WkATQ6nrQfswRozVQ"
#define FIREBASE_DATABASE_URL "pressure-control-17b6e-default-rtdb.firebaseio.com"

class FirebaseManager {
public:
    FirebaseManager();
    void begin(SystemSettings* settings, SystemState* state);
    void handle();
    void triggerUpload();

private:
    FirebaseData fbdo;
    FirebaseAuth auth;
    FirebaseConfig config;
    
    SystemSettings* _settings;
    SystemState* _state;

    bool _firebaseReady;
    unsigned long _lastUploadTime;
    unsigned long _lastStartReceived;
    const unsigned long UPLOAD_INTERVAL = 5000; // Increased to 5s to reduce loop stalling

    void setupFirebase();
    void uploadState();
    void checkCommands();
    void downloadSettings();
};

#endif
