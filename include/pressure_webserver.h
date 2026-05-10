#ifndef PRESSURE_WEBSERVER_H
#define PRESSURE_WEBSERVER_H

#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <ArduinoJson.h>

struct SystemSettings {
    float kp;
    float ki;
    float kd;
    float setpoint;     // in BAR
    float minVoltage;   // for DAC Output
    float maxVoltage;   // for DAC Output
    int tankVolume;     // in Liters (Max 8)
    float sensorMinV;   // Default 0.468V
    float sensorMaxV;   // Default 2.34V
    float sensorMaxBar; // Default 12.0 BAR
    float workingMaxBar;// Default 2.0 BAR
    float accuracyMinV; // New: editable 0.66V
    float accuracyMaxV; // New: editable 3.3V
    float safetyAllowance; // Keep safety bypass
    float controlBandPercent; // e.g. 20.0%
    int minOnTimeMS;       // Minimum SUPPLY time
    int minOffTimeMS;      // Minimum EXHAUST PAUSE time
    int exhaustBurstMS;    // Duration of a single exhaust burst
    char wifiSSID[32];
    char wifiPassword[64];
    bool autoTuningEnabled; // AUTO or MANUAL mode
    float tuningStep;       // small_step for auto-tuning
    float valveFloor;       // Discovered hardware hiss point (0.0-100.0)
};

struct SystemState {
    float pressure;          // in BAR
    float pressurePercent;
    float setpointPercent;   // New: unified SP percentage
    float normalizedPressure; // 0.0 - 1.0
    float scaledTo3v3;       // For external feedback
    float sensorVoltage;
    float controlVoltage;
    float pidOutput;
    float dacValue;
    float rawADC;
    bool isStatic;
    bool systemActive;
    bool solenoidState;
    bool sensorConnected;    // New: track hardware connection
    float airVolume;         // in Liters
    float controlCurrent;    // in mA
    float displayPressure;   // Smoothed "User Value"
    int controlState;        // 0=IDLE, 1=SUPPLY, 2=EXHAUST
    int cycleTime;          // Adaptive cycle time in ms
    float integral;         // PID integral accumulator
    float prevError;        // PID previous error
    float prevPressure;     // For rate calculation
    float rate;             // PV change rate
    bool outputState;       // Current ON/OFF state (for time-proportioning)
    float pTerm;            // Current P component for telemetry
    float iTerm;            // Current I component for telemetry
    float dTerm;            // Current D component for telemetry
    bool forceCalibration;   // Trigger for the ramp test
};

class PressureWebServer {
public:
    PressureWebServer();
    void begin(SystemSettings* settings, SystemState* state);
    void handle();

private:
    WebServer _server;
    DNSServer _dnsServer;
    SystemSettings* _settings;
    SystemState* _state;

    void setupRoutes();
    void handleRoot();
    void handleData();
    void handleUpdate();
    void handleNotFound();
};

#endif
