#ifndef PRESSURE_WEBSERVER_H
#define PRESSURE_WEBSERVER_H

#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <ArduinoJson.h>

struct SystemSettings {
    // PID
    float kp;
    float ki;
    float kd;
    int sampleTime;

    // Pressure Control
    float setpoint;
    float maxPressure;
    int units; // 0: %, 1: PSI, 2: BAR

    // Output
    float minVoltage;
    float maxVoltage;
    float rampRate;
    float calibrationFactor;

    // Tank
    int tankVolume;

    // Sensor Calibration
    float lowVoltage;
    float highVoltage;
};

struct SystemState {
    float pressure;
    float pressurePercent;
    float controlVoltage;
    float pidOutput;
    float dacValue;
    bool isStatic;
    bool systemActive;
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
