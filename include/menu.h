#ifndef MENU_H
#define MENU_H

#include "webserver.h" // For SystemSettings and SystemState
#include <Arduino.h>
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>

enum MenuState { HOME, VESSEL_SIZE, PID_PARAMS, SETPOINT_VOLTAGE };

class MenuSystem {
public:
  MenuSystem();
  void begin(SystemSettings *settings, SystemState *state);
  void update();

private:
  LiquidCrystal_I2C _lcd;
  Keypad _keypad;
  SystemSettings *_settings;
  SystemState *_state;
  MenuState _currentState;

  unsigned long _lastLcdUpdate;
  int _editParamIndex; // To track which parameter is selected for 2/8 scrolling

  void handleKey(char key);
  void drawHome();
  void drawVesselSize();
  void drawPIDParams();
  void drawSetpointVoltage();

  void updateLCD();
};

#endif
