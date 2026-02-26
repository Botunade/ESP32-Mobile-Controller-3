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
      _currentState(HOME),
      _lastLcdUpdate(0),
      _cursorPos(0),
      _targetFloat(nullptr),
      _targetInt(nullptr) {
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

    if (millis() - _lastLcdUpdate > 200) {
        _lastLcdUpdate = millis();
        updateLCD();
    }
}

void MenuSystem::handleKey(char key) {
    if (_currentState == EDIT_VALUE) {
        if (key >= '0' && key <= '9') {
            _inputBuffer += key;
        } else if (key == '*') { // Decimal point
            if (_inputBuffer.indexOf('.') == -1) _inputBuffer += '.';
        } else if (key == 'A') { // Confirm
            finishEditing(true);
        } else if (key == 'B') { // Cancel
            finishEditing(false);
        } else if (key == 'C') { // Backspace
            if (_inputBuffer.length() > 0) _inputBuffer.remove(_inputBuffer.length() - 1);
        }
        return;
    }

    // Navigation
    if (key == 'A') { _currentState = HOME; _cursorPos = 0; }
    else if (key == 'B') { _currentState = TANK_SETTINGS; _cursorPos = 0; }
    else if (key == 'C') { _currentState = PID_SETTINGS; _cursorPos = 0; }
    else if (key == 'D') { _currentState = SYSTEM_SETTINGS; _cursorPos = 0; }
    else if (key == '*') { _state->systemActive = true; }
    else if (key == '#') { _state->systemActive = false; _currentState = HOME; }
    else if (key == '2') { // UP
        _cursorPos--;
        if (_cursorPos < 0) _cursorPos = 3;
    }
    else if (key == '8') { // DOWN
        _cursorPos++;
        if (_cursorPos > 3) _cursorPos = 0;
    }
    else if (key == '5') { // Select for editing
        switch (_currentState) {
            case PID_SETTINGS:
                if (_cursorPos == 0) startEditing("Kp", &_settings->kp);
                else if (_cursorPos == 1) startEditing("Ki", &_settings->ki);
                else if (_cursorPos == 2) startEditing("Kd", &_settings->kd);
                else if (_cursorPos == 3) startEditing("SampleTime", &_settings->sampleTime);
                break;
            case TANK_SETTINGS:
                if (_cursorPos == 0) startEditing("Volume", &_settings->tankVolume);
                else if (_cursorPos == 1) startEditing("Max Press", &_settings->maxPressure);
                else if (_cursorPos == 2) startEditing("Units", &_settings->units);
                break;
            case OUTPUT_SETTINGS:
                if (_cursorPos == 0) startEditing("Min V", &_settings->minVoltage);
                else if (_cursorPos == 1) startEditing("Max V", &_settings->maxVoltage);
                else if (_cursorPos == 2) startEditing("Ramp Rate", &_settings->rampRate);
                break;
            case SYSTEM_SETTINGS:
                if (_cursorPos == 0) startEditing("Low V", &_settings->lowVoltage);
                else if (_cursorPos == 1) startEditing("High V", &_settings->highVoltage);
                else if (_cursorPos == 2) startEditing("Cal Factor", &_settings->calibrationFactor);
                else if (_cursorPos == 3) { _currentState = OUTPUT_SETTINGS; _cursorPos = 0; }
                break;
            case HOME:
                startEditing("Setpoint", &_settings->setpoint);
                break;
            default: break;
        }
    }
}

void MenuSystem::startEditing(String label, float* target) {
    _previousState = _currentState;
    _currentState = EDIT_VALUE;
    _editLabel = label;
    _targetFloat = target;
    _targetInt = nullptr;
    _inputBuffer = String(*target, 3);
}

void MenuSystem::startEditing(String label, int* target) {
    _previousState = _currentState;
    _currentState = EDIT_VALUE;
    _editLabel = label;
    _targetFloat = nullptr;
    _targetInt = target;
    _inputBuffer = String(*target);
}

void MenuSystem::finishEditing(bool save) {
    if (save) {
        if (_targetFloat) *_targetFloat = _inputBuffer.toFloat();
        else if (_targetInt) *_targetInt = _inputBuffer.toInt();
    }
    _currentState = _previousState;
}

void MenuSystem::updateLCD() {
    _lcd.clear();
    switch (_currentState) {
        case HOME: drawHome(); break;
        case PID_SETTINGS: drawPIDSettings(); break;
        case TANK_SETTINGS: drawTankSettings(); break;
        case OUTPUT_SETTINGS: drawOutputSettings(); break;
        case SYSTEM_SETTINGS: drawSystemSettings(); break;
        case EDIT_VALUE: drawEditValue(); break;
    }
}

void MenuSystem::drawHome() {
    _lcd.setCursor(0, 0); _lcd.print("--- SYSTEM HOME ---");
    _lcd.setCursor(0, 1);
    String unit = _settings->units == 1 ? "PSI" : (_settings->units == 2 ? "BAR" : "%");
    _lcd.print("SP: "); _lcd.print(_settings->setpoint, 1); _lcd.print(" "); _lcd.print(unit);
    _lcd.setCursor(0, 2);
    _lcd.print("Pres: "); _lcd.print(_state->pressure, 1); _lcd.print(" "); _lcd.print(unit);
    _lcd.setCursor(0, 3);
    _lcd.print("Out: "); _lcd.print(_state->controlVoltage, 2); _lcd.print("V ");
    _lcd.print(_state->systemActive ? "[RUN]" : "[IDLE]");
}

void MenuSystem::drawPIDSettings() {
    _lcd.setCursor(0, 0); _lcd.print("--- PID SETTINGS ---");
    _lcd.setCursor(0, 1); _lcd.print(_cursorPos == 0 ? ">Kp: " : " Kp: "); _lcd.print(_settings->kp, 2);
    _lcd.setCursor(10, 1); _lcd.print(_cursorPos == 1 ? ">Ki: " : " Ki: "); _lcd.print(_settings->ki, 2);
    _lcd.setCursor(0, 2); _lcd.print(_cursorPos == 2 ? ">Kd: " : " Kd: "); _lcd.print(_settings->kd, 2);
    _lcd.setCursor(10, 2); _lcd.print(_cursorPos == 3 ? ">Ts: " : " Ts: "); _lcd.print(_settings->sampleTime);
    _lcd.setCursor(0, 3); _lcd.print("5:Edit A:Home");
}

void MenuSystem::drawTankSettings() {
    _lcd.setCursor(0, 0); _lcd.print("--- TANK SETTINGS ---");
    _lcd.setCursor(0, 1); _lcd.print(_cursorPos == 0 ? ">Vol: " : " Vol: "); _lcd.print(_settings->tankVolume);
    _lcd.setCursor(0, 2); _lcd.print(_cursorPos == 1 ? ">MaxP: " : " MaxP: "); _lcd.print(_settings->maxPressure, 1);
    _lcd.setCursor(0, 3); _lcd.print(_cursorPos == 2 ? ">Unit: " : " Unit: "); _lcd.print(_settings->units);
}

void MenuSystem::drawOutputSettings() {
    _lcd.setCursor(0, 0); _lcd.print("-- OUTPUT SETTINGS --");
    _lcd.setCursor(0, 1); _lcd.print(_cursorPos == 0 ? ">MinV: " : " MinV: "); _lcd.print(_settings->minVoltage, 2);
    _lcd.setCursor(0, 2); _lcd.print(_cursorPos == 1 ? ">MaxV: " : " MaxV: "); _lcd.print(_settings->maxVoltage, 2);
    _lcd.setCursor(0, 3); _lcd.print(_cursorPos == 2 ? ">Ramp: " : " Ramp: "); _lcd.print(_settings->rampRate, 1);
}

void MenuSystem::drawSystemSettings() {
    _lcd.setCursor(0, 0); _lcd.print("-- SYSTEM SETTINGS --");
    _lcd.setCursor(0, 1); _lcd.print(_cursorPos == 0 ? ">LowV: " : " LowV: "); _lcd.print(_settings->lowVoltage, 3);
    _lcd.setCursor(0, 2); _lcd.print(_cursorPos == 1 ? ">HighV: " : " HighV: "); _lcd.print(_settings->highVoltage, 3);
    _lcd.setCursor(0, 3); _lcd.print(_cursorPos == 2 ? ">CalF: " : " CalF: "); _lcd.print(_settings->calibrationFactor, 2);
    _lcd.setCursor(12, 3); _lcd.print(_cursorPos == 3 ? ">OUT" : " OUT");
}

void MenuSystem::drawEditValue() {
    _lcd.setCursor(0, 0); _lcd.print("EDIT: "); _lcd.print(_editLabel);
    _lcd.setCursor(0, 1); _lcd.print("Value: "); _lcd.print(_inputBuffer); _lcd.print("_");
    _lcd.setCursor(0, 3); _lcd.print("A:OK B:CNCL C:BS *:.");
}
