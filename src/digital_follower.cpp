#include "digital_follower.h"
#include <Arduino.h>

// --- Hidden Digital PID Follower ---
// Silently mirrors PID demand as a pure ON/OFF signal on GPIO 4.
// Reads state only. Never modifies PID, DAC, UI, or any shared state.

static const int FOLLOWER_PIN = 4;
static bool _reachedSetpoint = false;

void digitalFollower_init() {
    pinMode(FOLLOWER_PIN, OUTPUT);
    digitalWrite(FOLLOWER_PIN, LOW);
}

void digitalFollower_update(const SystemState& state, const SystemSettings& settings) {
    // Guard: system not active → force LOW, reset tracking
    if (!state.systemActive) {
        digitalWrite(FOLLOWER_PIN, LOW);
        _reachedSetpoint = false;
        return;
    }

    // Hysteresis: 1% of working pressure range to prevent rapid toggling
    float hysteresis = settings.workingMaxBar * 0.01f;

    // Track whether pressure has reached setpoint
    if (state.pressure >= settings.setpoint) {
        _reachedSetpoint = true;
    } else if (state.pressure < settings.setpoint - hysteresis) {
        _reachedSetpoint = false;
    }

    // HIGH = control needed (below setpoint), LOW = setpoint reached
    digitalWrite(FOLLOWER_PIN, _reachedSetpoint ? LOW : HIGH);
}
