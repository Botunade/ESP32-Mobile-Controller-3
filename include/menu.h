#ifndef MENU_H
#define MENU_H

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include "webserver.h"

enum MenuState {
    HOME,
    PID_SETTINGS,
    TANK_SETTINGS,
    OUTPUT_SETTINGS,
    SYSTEM_SETTINGS,
    EDIT_VALUE
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
    MenuState _currentState;
    MenuState _previousState;

    unsigned long _lastLcdUpdate;
    int _cursorPos;
    float* _targetFloat;
    int* _targetInt;
    String _inputBuffer;
    String _editLabel;

    void handleKey(char key);
    void drawHome();
    void drawPIDSettings();
    void drawTankSettings();
    void drawOutputSettings();
    void drawSystemSettings();
    void drawEditValue();

    void startEditing(String label, float* target);
    void startEditing(String label, int* target);
    void finishEditing(bool save);

    void updateLCD();
};

#endif
