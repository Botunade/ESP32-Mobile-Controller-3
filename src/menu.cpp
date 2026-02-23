#include "menu.h"

const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {13, 12, 14, 27};
byte colPins[COLS] = {26, 33, 32, 15};

MenuSystem::MenuSystem()
    : _lcd(0x27, 20, 4),
      _keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS),
      _currentPage(MAIN_SCREEN),
      _lastLcdUpdate(0),
      _isEditing(false) {
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
        switch (_currentPage) {
            case MAIN_SCREEN: drawMainScreen(); break;
            case PID_SETTINGS: drawPIDSettings(); break;
            case TANK_SETTINGS: drawTankSettings(); break;
            case OUTPUT_SETTINGS: drawOutputSettings(); break;
            case WIFI_INFO: drawWiFiInfo(); break;
        }
    }
}

void MenuSystem::handleKey(char key) {
    if (key == 'A') _currentPage = PID_SETTINGS;
    else if (key == 'B') _currentPage = TANK_SETTINGS;
    else if (key == 'C') _currentPage = OUTPUT_SETTINGS;
    else if (key == 'D') _currentPage = WIFI_INFO;
    else if (key == '*') {
        _currentPage = MAIN_SCREEN;
        _isEditing = false;
        _tempValue = 0;
    }
    else if (key >= '0' && key <= '9') {
        if (!_isEditing) {
            _isEditing = true;
            _tempValue = (key - '0');
        } else {
            _tempValue = _tempValue * 10 + (key - '0');
        }
    }
    else if (key == '#') {
        if (_isEditing) {
            // Apply temp value to current page parameter
            if (_currentPage == MAIN_SCREEN) _settings->setpoint = _tempValue;
            else if (_currentPage == PID_SETTINGS) _settings->kp = _tempValue;
            _isEditing = false;
            _tempValue = 0;
        }
    }
}

void MenuSystem::drawMainScreen() {
    _lcd.setCursor(0, 0);
    _lcd.print("Pressure: ");
    _lcd.print(_state->pressure, 1);
    _lcd.print(" PSI  ");

    _lcd.setCursor(0, 1);
    _lcd.print("Level: ");
    _lcd.print(_state->pressurePercent, 1);
    _lcd.print("%    ");

    _lcd.setCursor(0, 2);
    _lcd.print("Output: ");
    _lcd.print(_state->controlVoltage, 2);
    _lcd.print("V    ");

    _lcd.setCursor(0, 3);
    if (_isEditing) {
        _lcd.print("Edit SP: ");
        _lcd.print(_tempValue, 0);
        _lcd.print("_    ");
    } else {
        _lcd.print("Setpoint: ");
        _lcd.print(_settings->setpoint, 1);
        _lcd.print("     ");
    }
}

void MenuSystem::drawPIDSettings() {
    _lcd.setCursor(0, 0); _lcd.print("--- PID SETTINGS ---");
    _lcd.setCursor(0, 1); _lcd.print("Kp: "); _lcd.print(_settings->kp); _lcd.print("     ");
    _lcd.setCursor(0, 2); _lcd.print("Ki: "); _lcd.print(_settings->ki); _lcd.print("     ");
    _lcd.setCursor(0, 3); _lcd.print("Kd: "); _lcd.print(_settings->kd); _lcd.print("     ");
}

void MenuSystem::drawTankSettings() {
    _lcd.setCursor(0, 0); _lcd.print("--- TANK SETTINGS --");
    _lcd.setCursor(0, 1); _lcd.print("Volume: "); _lcd.print(_settings->tankVolume); _lcd.print(" L   ");
    _lcd.setCursor(0, 2); _lcd.print("                    ");
    _lcd.setCursor(0, 3); _lcd.print("                    ");
}

void MenuSystem::drawOutputSettings() {
    _lcd.setCursor(0, 0); _lcd.print("-- OUTPUT LIMITS --");
    _lcd.setCursor(0, 1); _lcd.print("Min: "); _lcd.print(_settings->minVoltage); _lcd.print("V   ");
    _lcd.setCursor(0, 2); _lcd.print("Max: "); _lcd.print(_settings->maxVoltage); _lcd.print("V   ");
    _lcd.setCursor(0, 3); _lcd.print("                    ");
}

void MenuSystem::drawWiFiInfo() {
    _lcd.setCursor(0, 0); _lcd.print("--- WIFI INFO ---");
    _lcd.setCursor(0, 1); _lcd.print("AP: PressCtrl_AP  ");
    _lcd.setCursor(0, 2); _lcd.print("IP: 192.168.4.1   ");
    _lcd.setCursor(0, 3); _lcd.print("                    ");
}
