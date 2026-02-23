#ifndef MENU_H
#define MENU_H

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include "webserver.h" // For SystemSettings and SystemState

enum MenuPage {
    MAIN_SCREEN,
    PID_SETTINGS,
    TANK_SETTINGS,
    OUTPUT_SETTINGS,
    WIFI_INFO
};

class MenuSystem {
public:
    MenuSystem();
    void begin(SystemSettings* settings, SystemState* state);
    void update();

private:
    LiquidCrystal_I2C _lcd;
    Keypad _keypad;
    SystemSettings* _settings;
    SystemState* _state;
    MenuPage _currentPage;

    unsigned long _lastLcdUpdate;

    void handleKey(char key);
    void drawMainScreen();
    void drawPIDSettings();
    void drawTankSettings();
    void drawOutputSettings();
    void drawWiFiInfo();

    // Simple numeric editor
    float _tempValue;
    bool _isEditing;
    int _editParamIndex;
};

#endif
