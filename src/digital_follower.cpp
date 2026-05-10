#include "digital_follower.h"
#include <Arduino.h>

// --- Main Valve Actuator (Follower Logic) ---
// Controls the primary valve on GPIO 4.
// Operates on a simple high/low threshold with hysteresis.

static const int VALVE_PIN = 4;
static bool _valveOpen = false;

// --- PID Follower PWM (Active Filter Source) ---
// Produces a 20kHz PWM signal on GPIO 18 to follow PID output.
// Frequency: 20kHz for smooth analog conversion via active filter.
static const int PID_FOLLOWER_PIN = 18;
static const int PID_FOLLOWER_CHAN = 1;
static const int PID_FOLLOWER_FREQ = 20000;
static const int PID_FOLLOWER_RES = 10; // 0-1023

void digitalFollower_init() {
    // Standard Valve Initialization
    pinMode(VALVE_PIN, OUTPUT);
    digitalWrite(VALVE_PIN, LOW);

    // PID Follower PWM Initialization (LEDC)
    ledcSetup(PID_FOLLOWER_CHAN, PID_FOLLOWER_FREQ, PID_FOLLOWER_RES);
    ledcAttachPin(PID_FOLLOWER_PIN, PID_FOLLOWER_CHAN);
    ledcWrite(PID_FOLLOWER_CHAN, 0);
}

void digitalFollower_update(const SystemState& state, const SystemSettings& settings) {
    // 1. PWM Follower Logic (20kHz Signal)
    // Always follows the PID output (0-100%) mapped to 10-bit range (0-1023)
    uint32_t duty = 0;
    if (state.systemActive) {
        float effort = state.pidOutput;
        if (effort < 0) effort = 0;
        if (effort > 100) effort = 100;
        duty = (uint32_t)((effort / 100.0f) * 1023.0f);
    }
    ledcWrite(PID_FOLLOWER_CHAN, duty);

    // 2. Secondary Valve Logic (GPIO 4)
    // Synchronized with the main state machine for consistent supply/exhaust cycles
    if (state.controlState == 1) { // SUPPLY
        _valveOpen = true;
    } else {
        _valveOpen = false; // EXHAUST or IDLE
    }

    // Actuate the physical pin
    digitalWrite(VALVE_PIN, _valveOpen ? HIGH : LOW);
}
