#ifndef FIREBASE_MANAGER_H
#define FIREBASE_MANAGER_H

#include "webserver.h"
#include <Arduino.h>
#include <Firebase_ESP_Client.h>
#include <WiFi.h>

// Internal includes for Firebase Manager only
// We will include addons in the CPP file to avoid multiple definition errors

#define FIREBASE_API_KEY "AIzaSyDPPqJp0Gk3BGlV6-WkATQ6nrQfswRozVQ"
#define FIREBASE_DATABASE_URL                                                  \
  "pressure-control-17b6e-default-rtdb.firebaseio.com"

class FirebaseManager {
public:
  FirebaseManager();
  void begin(SystemSettings *settings, SystemState *state);
  void handle();

private:
  FirebaseData fbdo;
  FirebaseAuth auth;
  FirebaseConfig config;

  SystemSettings *_settings;
  SystemState *_state;

  bool _firebaseReady;
  unsigned long _lastUploadTime;
  const unsigned long UPLOAD_INTERVAL = 1000; // 1 second

  void setupFirebase();
  void uploadState();
  void checkCommands();
  void downloadSettings();
};

#endif
