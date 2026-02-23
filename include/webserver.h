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
    float setpoint;
    float minVoltage;
    float maxVoltage;
    int tankVolume;
};

struct SystemState {
    float pressure;
    float pressurePercent;
    float controlVoltage;
    float pidOutput;
    float dacValue;
    bool isStatic;
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
