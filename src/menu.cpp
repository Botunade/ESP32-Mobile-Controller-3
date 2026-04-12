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
      _state(nullptr),
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
    // --- KEYPAD HEARTBEAT (5 seconds) ---
    static unsigned long lastHeartbeat = 0;
    if (millis() - lastHeartbeat > 5000) {
        lastHeartbeat = millis();
        Serial.println("[DIAG] Keypad Scanning Active...");
    }

    char key = _keypad.getKey();
    if (key) {
        Serial.print("SUCCESS: Key Pressed: [");
        Serial.print(key);
        Serial.println("]");
        
        handleKey(key);
        updateLCD();
    }

    if (millis() - _lastLcdUpdate > 500) {
        _lastLcdUpdate = millis();
        if (_currentState == HOME) {
            updateLCD();
        }
    }
}

void MenuSystem::handleKey(char key) {
    // Global Commands
    if (key == 'A') { _currentState = HOME; _editParamIndex = 0; }
    else if (key == 'B') { _currentState = VESSEL_SIZE; _editParamIndex = 0; }
    else if (key == '1') { _currentState = PID_PARAMS; _editParamIndex = 0; } // Was '9' and 'C', remapped for bad row
    else if (key == 'D') { _currentState = SETPOINT_VOLTAGE; _editParamIndex = 0; }
    else if (key == '*') { 
        _state->systemActive = true; 
    }
    else if (key == '8') { // STOP (Remapped from # for faulty hardware)
        _state->systemActive = false;
        _state->solenoidState = false; // Hardware Safety Cutoff Marker
        _currentState = HOME;
    }

    // Navigation and Editing (only in setting menus)
    if (_currentState != HOME) {
        int maxParams = (_currentState == VESSEL_SIZE) ? 0 : 2;
        
        if (key == '2') { // UP
            _editParamIndex--;
            if (_editParamIndex < 0) _editParamIndex = maxParams; 
        }
        else if (key == '0') { // DOWN - Was '8' but remapped due to hardware fault
            _editParamIndex++;
            if (_editParamIndex > maxParams) _editParamIndex = 0;
        }
        else if (key == '4') { // LEFT (Decrement)
            switch (_currentState) {
                case VESSEL_SIZE:
                    _settings->tankVolume -= 1;
                    if (_settings->tankVolume < 0) _settings->tankVolume = 0;
                    break;
                case PID_PARAMS:
                    if (_editParamIndex == 0) _settings->kp -= 0.01f;
                    else if (_editParamIndex == 1) _settings->ki -= 0.01f;
                    else if (_editParamIndex == 2) _settings->kd -= 0.01f;
                    break;
                case SETPOINT_VOLTAGE:
                    if (_editParamIndex == 0) _settings->spPercent -= 1.0f; // 1% steps
                    else if (_editParamIndex == 1) _settings->deadband -= 0.1f;
                    else if (_editParamIndex == 2) _settings->workingMaxBar -= 0.5f;
                    
                    if (_settings->spPercent < 0) _settings->spPercent = 0;
                    if (_settings->deadband < 0) _settings->deadband = 0;
                    if (_settings->workingMaxBar < 1.0f) _settings->workingMaxBar = 1.0f;
                    break;
                default: break;
            }
        }
        else if (key == '5') { // RIGHT (Increment - Remapped from 6 for faulty hardware)
            switch (_currentState) {
                case VESSEL_SIZE:
                    _settings->tankVolume += 1;
                    if (_settings->tankVolume > 30) _settings->tankVolume = 30; // Capped at 30L
                    break;
                case PID_PARAMS:
                    if (_editParamIndex == 0) _settings->kp += 0.01f;
                    else if (_editParamIndex == 1) _settings->ki += 0.01f;
                    else if (_editParamIndex == 2) _settings->kd += 0.01f;
                    break;
                case SETPOINT_VOLTAGE:
                    if (_editParamIndex == 0) _settings->spPercent += 1.0f; // 1% steps
                    else if (_editParamIndex == 1) _settings->deadband += 0.1f;
                    else if (_editParamIndex == 2) _settings->workingMaxBar += 0.5f;
                    
                    if (_settings->spPercent > 100) _settings->spPercent = 100;
                    if (_settings->deadband > 5.0f) _settings->deadband = 5.0f;
                    if (_settings->workingMaxBar > 12.0f) _settings->workingMaxBar = 12.0f;
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
    
    // Row 1: Primary Telemetry
    _lcd.setCursor(0, 1);
    _lcd.print("PV: "); _lcd.print(_state->displayPV, 1); _lcd.print("%  "); 
    _lcd.print("SP: "); _lcd.print(_settings->spPercent, 0); _lcd.print("%");
    
    // Row 2: Controller Output
    _lcd.setCursor(0, 2);
    _lcd.print("OUT: "); _lcd.print(_state->displayOUT, 1); _lcd.print("%");
    _lcd.print("         "); // Clear voltage area
    
    // Row 3: Status
    _lcd.setCursor(0, 3);
    _lcd.print("STATUS: ");
    if (!_state->systemActive) {
        _lcd.print("[IDLE]    ");
    } else {
        _lcd.print("[RUNNING] ");
    }
}

void MenuSystem::drawVesselSize() {
    _lcd.setCursor(0, 0); _lcd.print("--- VESSEL SIZE ---");
    _lcd.setCursor(0, 1);
    _lcd.print(_editParamIndex == 0 ? "> Volume: " : "  Volume: ");
    _lcd.print(_settings->tankVolume); _lcd.print(" L (Max 30)");
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
    _lcd.print(_editParamIndex == 0 ? "> SP:  " : "  SP:  "); _lcd.print(_settings->spPercent, 1); _lcd.print("%    ");
    _lcd.setCursor(0, 2);
    _lcd.print(_editParamIndex == 1 ? "> Dband: " : "  Dband: "); _lcd.print(_settings->deadband, 1); _lcd.print(" BAR ");
    _lcd.setCursor(0, 3);
    _lcd.print(_editParamIndex == 2 ? "> MaxP: " : "  MaxP: "); _lcd.print(_settings->workingMaxBar, 1); _lcd.print(" BAR ");
}
