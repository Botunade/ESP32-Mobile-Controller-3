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
      _editParamIndex(0),
      _lastKey(' ') {
}

void MenuSystem::begin(SystemSettings* settings, SystemState* state) {
    _settings = settings;
    _state = state;
    _lcd.init();
    _lcd.backlight();
}

void MenuSystem::update() {
    char key = _keypad.getKey();
    if (key) {
        Serial.print("KEYPAD RAW: [");
        Serial.print(key);
        Serial.println("]");
        
        handleKey(key);
        updateLCD();
    }

    if (millis() - _lastLcdUpdate > 500) {
        _lastLcdUpdate = millis();
        if (_currentState == HOME || _currentState == KEYPAD_DIAGNOSTIC) {
            updateLCD();
        }
    }
}

void MenuSystem::handleKey(char key) {
    // Global Commands
    if (key == '*') { _currentState = HOME; _editParamIndex = 0; }
    else if (key == 'B') { _currentState = VESSEL_SIZE; _editParamIndex = 0; }
    else if (key == '1') { _currentState = PID_PARAMS; _editParamIndex = 0; } // Was '9' and 'C', remapped for bad row
    else if (key == 'D') { _currentState = SETPOINT_VOLTAGE; _editParamIndex = 0; }
    else if (key == 'A') { 
        Serial.println("[ACTION] System START requested from Keypad");
        _state->systemActive = true; 
    }
    else if (key == '5') {
        Serial.println("[ACTION] Manual Calibration requested from Keypad");
        _state->forceCalibration = true;
        _state->systemActive = true; 
    }
    else if (key == '2') {
        Serial.println("[ACTION] Emergency STOP requested from Keypad");
        _state->systemActive = false;
        _state->solenoidState = false; // Hardware Safety Cutoff Marker
        _currentState = HOME;
    }
    else if (key == '#') {
        _currentState = KEYPAD_DIAGNOSTIC;
        _editParamIndex = 0;
    }

    _lastKey = key; // Store for diagnostics

    // Navigation and Editing (only in setting menus)
    if (_currentState != HOME) {
        int maxParams = (_currentState == VESSEL_SIZE) ? 0 : 2;
        
        if (key == '0') { // UP (Moved from 2)
            _editParamIndex--;
            if (_editParamIndex < 0) _editParamIndex = maxParams; 
        }
        else if (key == '8') { // DOWN
            _editParamIndex++;
            if (_editParamIndex > maxParams) _editParamIndex = 0;
        }
        else if (key == '4') { // LEFT (Decrement)
            switch (_currentState) {
                case VESSEL_SIZE:
                    _settings->tankVolume -= 1;
                    if (_settings->tankVolume < 0) _settings->tankVolume = 0;
                    break;
                case PID_PARAMS: // QuickPID Tuning
                    if (_editParamIndex == 0) _settings->kp -= 0.5f;
                    else if (_editParamIndex == 1) _settings->ki -= 0.05f;
                    else if (_editParamIndex == 2) _settings->kd -= 0.05f;
                    
                    if (_settings->kp < 0.0f) _settings->kp = 0.0f;
                    if (_settings->ki < 0.0f) _settings->ki = 0.0f;
                    if (_settings->kd < 0.0f) _settings->kd = 0.0f;
                    break;
                case SETPOINT_VOLTAGE: // Rebranded as "Calibration"
                    if (_editParamIndex == 0) _settings->setpoint -= 0.05f;
                    else if (_editParamIndex == 1) _settings->workingMaxBar -= 0.05f;
                    else if (_editParamIndex == 2) _settings->controlBandPercent -= 0.1f;
                    
                    // Clamp
                    if (_settings->setpoint < 0.0f) _settings->setpoint = 0.0f;
                    if (_settings->workingMaxBar < 0.1f) _settings->workingMaxBar = 0.1f;
                    if (_settings->controlBandPercent < 0) _settings->controlBandPercent = 0;
                    break;
                default: break;
            }
        }
        else if (key == '5') { // RIGHT (Increment)
            switch (_currentState) {
                case VESSEL_SIZE:
                    _settings->tankVolume += 1;
                    if (_settings->tankVolume > 8) _settings->tankVolume = 8;
                    break;
                case PID_PARAMS: // QuickPID Tuning
                    if (_editParamIndex == 0) _settings->kp += 0.5f;
                    else if (_editParamIndex == 1) _settings->ki += 0.05f;
                    else if (_editParamIndex == 2) _settings->kd += 0.05f;
                    break;
                case SETPOINT_VOLTAGE: // Rebranded as "Calibration"
                    if (_editParamIndex == 0) _settings->setpoint += 0.05f;
                    else if (_editParamIndex == 1) _settings->workingMaxBar += 0.05f;
                    else if (_editParamIndex == 2) _settings->controlBandPercent += 0.1f;
                    
                    // Clamp
                    if (_settings->setpoint > _settings->workingMaxBar) _settings->setpoint = _settings->workingMaxBar;
                    if (_settings->workingMaxBar > 12.0f) _settings->workingMaxBar = 12.0f;
                    if (_settings->controlBandPercent > 20.0f) _settings->controlBandPercent = 20.0f;
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
        case KEYPAD_DIAGNOSTIC: drawKeypadDiagnostic(); break;
    }
}

void MenuSystem::drawHome() {
    _lcd.setCursor(0, 0);
    _lcd.print("--- SYSTEM HOME ---");
    _lcd.setCursor(0, 1);
    _lcd.print("PV%: ");
    _lcd.print(_state->pressurePercent, 1);
    _lcd.print("% ");
    
    _lcd.print("SP%: ");
    _lcd.print(_state->setpointPercent, 1);
    _lcd.print("% ");
    
    _lcd.setCursor(0, 2);
    _lcd.print("Out%: ");
    // Controller Output % based on PID effort (already 0-100 range)
    float outPercent = _state->pidOutput;
    if (outPercent < 0) outPercent = 0;
    if (outPercent > 100) outPercent = 100;
    _lcd.print(outPercent, 1);
    _lcd.print("%  ");
    _lcd.setCursor(0, 3);

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
    _lcd.setCursor(0, 0);
    _lcd.print("--- PID TUNING ---");
    
    _lcd.setCursor(0, 1);
    _lcd.print(_editParamIndex == 0 ? "> Kp: " : "  Kp: ");
    _lcd.print(_settings->kp, 2); _lcd.print("      ");
    
    _lcd.setCursor(0, 2);
    _lcd.print(_editParamIndex == 1 ? "> Ki: " : "  Ki: ");
    _lcd.print(_settings->ki, 3); _lcd.print("      ");
    
    _lcd.setCursor(0, 3);
    _lcd.print(_editParamIndex == 2 ? "> Kd: " : "  Kd: ");
    _lcd.print(_settings->kd, 4); _lcd.print("      ");
}

void MenuSystem::drawSetpointVoltage() {
    _lcd.setCursor(0, 0); _lcd.print("--- CALIBRATION ---");
    _lcd.setCursor(0, 1);
    _lcd.print(_editParamIndex == 0 ? "> Setpoint: " : "  Setpoint: "); 
    _lcd.print(_settings->setpoint, 2); _lcd.print(" BAR ");
    
    _lcd.setCursor(0, 2);
    _lcd.print(_editParamIndex == 1 ? "> Max Pres: " : "  Max Pres: "); 
    _lcd.print(_settings->workingMaxBar, 1); _lcd.print(" BAR ");
    
    _lcd.setCursor(0, 3);
    _lcd.print(_editParamIndex == 2 ? "> Band %: " : "  Band %: "); 
    _lcd.print(_settings->controlBandPercent, 1); _lcd.print("%    ");
}

void MenuSystem::drawKeypadDiagnostic() {
    _lcd.setCursor(0, 0);
    _lcd.print("--- KEYPAD TEST --- ");
    _lcd.setCursor(0, 1);
    _lcd.print("Last Char: [");
    _lcd.print(_lastKey);
    _lcd.print("]   ");
    
    _lcd.setCursor(0, 2);
    char buf[21];
    snprintf(buf, 20, "ADC:%.0f  V:%.2f", _state->rawADC, _state->sensorVoltage);
    _lcd.print(buf);

    _lcd.setCursor(0, 3);
    _lcd.print("*:EXIT MODE         ");
}
