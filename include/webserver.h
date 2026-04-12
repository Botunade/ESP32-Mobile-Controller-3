#ifndef PRESSURE_WEBSERVER_H
#define PRESSURE_WEBSERVER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <WiFi.h>

struct SystemSettings {
  float kp;
  float ki;
  float kd;
  float setpoint;        // in BAR
  float minVoltage;      // for DAC Output
  float maxVoltage;      // for DAC Output
  int tankVolume;        // in Liters (Max 8)
  float sensorMinV;      // Default 0.468V
  float sensorMaxV;      // Default 2.34V
  float sensorMaxBar;    // Default 12.0 BAR
  float workingMaxBar;   // Default 2.0 BAR
  float accuracyMinV;    // New: editable 0.66V
  float accuracyMaxV;    // New: editable 3.3V
  float safetyAllowance; // New: default 0.5 BAR
  float deadband;        // Default 0.5 BAR
  float spPercent;       // New: operator-facing Setpoint (0-100%)
  char wifiSSID[32];
  char wifiPassword[64];
};

struct SystemState {
  float pressure; // in BAR
  float pressurePercent;
  float normalizedPressure; // 0.0 - 1.0
  float scaledTo3v3;        // For external feedback
  float sensorVoltage;
  float controlVoltage;
  float pidOutput;
  float dacValue;
  float rawADC;
  bool isStatic;
  bool systemActive;
  bool solenoidState;
  float airVolume; // in Liters

  // Dual-Track Abstractions (0-100%)
  float displaySP;
  float displayPV;
  float displayOUT;
  float track2SimOutput; // Simulated PID output (V)
};

class PressureWebServer {
public:
  PressureWebServer();
  void begin(SystemSettings *settings, SystemState *state);
  void handle();

private:
  WebServer _server;
  DNSServer _dnsServer;
  SystemSettings *_settings;
  SystemState *_state;

  void setupRoutes();
  void handleRoot();
  void handleData();
  void handleUpdate();
  void handleNotFound();
};

#endif
