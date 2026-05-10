#ifndef MENU_H
#define MENU_H

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include "pressure_webserver.h" // For SystemSettings and SystemState

enum MenuState {
    HOME,
    VESSEL_SIZE,
    PID_PARAMS,
    SETPOINT_VOLTAGE,
    KEYPAD_DIAGNOSTIC
};

class MenuSystem {
public:
    MenuSystem();
    void begin(SystemSettings* settings, SystemState* state);
    void update();
    char getLastKey() { return _lastKey == ' ' ? '?' : _lastKey; } // Returns '?' if no key yet

private:
    LiquidCrystal_I2C _lcd;
    Keypad _keypad;
    SystemSettings* _settings;
    SystemState* _state;
    MenuState _currentState;

    unsigned long _lastLcdUpdate;
    int _editParamIndex; // To track which parameter is selected for 2/8 scrolling
    char _lastKey;       // Track last pressed key for diagnostics

    void handleKey(char key);
    void drawHome();
    void drawVesselSize();
    void drawPIDParams();
    void drawSetpointVoltage();
    void drawKeypadDiagnostic();

    void updateLCD();
};

#endif
