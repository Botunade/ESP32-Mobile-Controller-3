#include "menu.h"

const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
// Rows: 13, 12, 14, 27
// Cols: 26, 33, 32, 15
byte rowPins[ROWS] = {13, 12, 14, 27};
byte colPins[COLS] = {26, 33, 32, 15};

MenuSystem::MenuSystem()
    : _lcd(0x27, 20, 4),
      _keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS),
      _currentState(HOME),
      _lastLcdUpdate(0),
      _editParamIndex(0) {
}

void MenuSystem::begin(SystemSettings* settings, SystemState* state) {
    _settings = settings;
    _state = state;
    _lcd.init();
    _lcd.backlight();
}

void MenuSystem::update() {
    char key = _keypad.getKey();
    if (key) handleKey(key);

    if (millis() - _lastLcdUpdate > 500) {
        _lastLcdUpdate = millis();
        updateLCD();
    }
}

void MenuSystem::handleKey(char key) {
    // Global Commands
    if (key == 'A') { _currentState = HOME; _editParamIndex = 0; }
    else if (key == 'B') { _currentState = VESSEL_SIZE; _editParamIndex = 0; }
    else if (key == 'C') { _currentState = PID_PARAMS; _editParamIndex = 0; }
    else if (key == 'D') { _currentState = SETPOINT_VOLTAGE; _editParamIndex = 0; }
    else if (key == '*') { _state->systemActive = true; }
    else if (key == '#') {
        _state->systemActive = false;
        _currentState = HOME;
    }

    // Navigation and Editing (only in setting menus)
    if (_currentState != HOME) {
        if (key == '2') { // UP
            _editParamIndex--;
            if (_editParamIndex < 0) _editParamIndex = 2; 
        }
        else if (key == '8') { // DOWN
            _editParamIndex++;
            if (_editParamIndex > 2) _editParamIndex = 0;
        }
        else if (key == '4') { // LEFT (Decrement)
            switch (_currentState) {
                case VESSEL_SIZE:
                    _settings->tankVolume -= 1;
                    if (_settings->tankVolume < 0) _settings->tankVolume = 0;
                    break;
                case PID_PARAMS:
                    if (_editParamIndex == 0) _settings->kp -= 0.1f;
                    else if (_editParamIndex == 1) _settings->ki -= 0.1f;
                    else if (_editParamIndex == 2) _settings->kd -= 0.01f;
                    break;
                case SETPOINT_VOLTAGE:
                    if (_editParamIndex == 0) _settings->setpoint -= 0.1f;
                    else if (_editParamIndex == 1) _settings->minVoltage -= 0.1f;
                    else if (_editParamIndex == 2) _settings->maxVoltage -= 0.1f;
                    break;
                default: break;
            }
        }
        else if (key == '6') { // RIGHT (Increment)
            switch (_currentState) {
                case VESSEL_SIZE:
                    _settings->tankVolume += 1;
                    if (_settings->tankVolume > 8) _settings->tankVolume = 8;
                    break;
                case PID_PARAMS:
                    if (_editParamIndex == 0) _settings->kp += 0.1f;
                    else if (_editParamIndex == 1) _settings->ki += 0.1f;
                    else if (_editParamIndex == 2) _settings->kd += 0.01f;
                    break;
                case SETPOINT_VOLTAGE:
                    if (_editParamIndex == 0) _settings->setpoint += 0.1f;
                    else if (_editParamIndex == 1) _settings->minVoltage += 0.1f;
                    else if (_editParamIndex == 2) _settings->maxVoltage += 0.1f;
                    break;
                default: break;
            }
        }
    }
}

void MenuSystem::updateLCD() {
    switch (_currentState) {
        case HOME: drawHome(); break;
        case VESSEL_SIZE: drawVesselSize(); break;
        case PID_PARAMS: drawPIDParams(); break;
        case SETPOINT_VOLTAGE: drawSetpointVoltage(); break;
    }
}

void MenuSystem::drawHome() {
    _lcd.setCursor(0, 0);
    _lcd.print("--- SYSTEM HOME ---");
    _lcd.setCursor(0, 1);
    _lcd.print("Setpoint: "); _lcd.print(_settings->setpoint, 2); _lcd.print(" BAR ");
    _lcd.setCursor(0, 2);
    _lcd.print("Pressure: "); _lcd.print(_state->pressure, 2); _lcd.print(" BAR ");
    _lcd.setCursor(0, 3);
    _lcd.print("Vol: "); _lcd.print(_state->airVolume, 1); _lcd.print("L ");
    if (!_state->systemActive) {
        _lcd.setCursor(14, 3); _lcd.print("[IDLE]");
    } else {
        _lcd.setCursor(14, 3); _lcd.print("[RUN ]");
    }
}

void MenuSystem::drawVesselSize() {
    _lcd.setCursor(0, 0); _lcd.print("--- VESSEL SIZE ---");
    _lcd.setCursor(0, 1);
    _lcd.print(_editParamIndex == 0 ? "> Volume: " : "  Volume: ");
    _lcd.print(_settings->tankVolume); _lcd.print(" L (Max 8)");
    _lcd.setCursor(0, 2); _lcd.print("                    ");
    _lcd.setCursor(0, 3); _lcd.print("                    ");
}

void MenuSystem::drawPIDParams() {
    _lcd.setCursor(0, 0); _lcd.print("--- PID PARAMS ---");
    _lcd.setCursor(0, 1);
    _lcd.print(_editParamIndex == 0 ? "> Kp: " : "  Kp: "); _lcd.print(_settings->kp, 2); _lcd.print("     ");
    _lcd.setCursor(0, 2);
    _lcd.print(_editParamIndex == 1 ? "> Ki: " : "  Ki: "); _lcd.print(_settings->ki, 3); _lcd.print("     ");
    _lcd.setCursor(0, 3);
    _lcd.print(_editParamIndex == 2 ? "> Kd: " : "  Kd: "); _lcd.print(_settings->kd, 4); _lcd.print("     ");
}

void MenuSystem::drawSetpointVoltage() {
    _lcd.setCursor(0, 0); _lcd.print("-- SETPOINT/VOLT --");
    _lcd.setCursor(0, 1);
    _lcd.print(_editParamIndex == 0 ? "> SP:  " : "  SP:  "); _lcd.print(_settings->setpoint, 2); _lcd.print(" BAR ");
    _lcd.setCursor(0, 2);
    _lcd.print(_editParamIndex == 1 ? "> Min: " : "  Min: "); _lcd.print(_settings->minVoltage, 1); _lcd.print("V    ");
    _lcd.setCursor(0, 3);
    _lcd.print(_editParamIndex == 2 ? "> Max: " : "  Max: "); _lcd.print(_settings->maxVoltage, 1); _lcd.print("V    ");
}
