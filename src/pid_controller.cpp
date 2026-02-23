#include "pid_controller.h"

PIDController::PIDController(float kp, float ki, float kd)
    : _kp(kp), _ki(ki), _kd(kd), _minOutput(0), _maxOutput(255), _sampleTime(100) {
    reset();
    _dFilterAlpha = 0.5f;
    _staticCounter = 0;
    _isStatic = false;
}

void PIDController::setTunings(float kp, float ki, float kd) {
    _kp = kp;
    _ki = ki;
    _kd = kd;
}

void PIDController::setOutputLimits(float min, float max) {
    if (min >= max) return;
    _minOutput = min;
    _maxOutput = max;
}

void PIDController::setSampleTime(int millis) {
    if (millis > 0) _sampleTime = millis;
}

void PIDController::reset() {
    _integral = 0;
    _lastInput = 0;
    _lastOutput = 0;
    _output = 0;
    _dTerm = 0;
    _lastTime = millis() - _sampleTime;
    _staticCounter = 0;
    _isStatic = false;
}

float PIDController::compute(float setpoint, float input) {
    unsigned long now = millis();
    unsigned long timeChange = (now - _lastTime);

    if (timeChange >= _sampleTime) {
        float error = setpoint - input;

        // Proportional term
        float pTerm = _kp * error;

        // Integral term with anti-windup (clamping)
        _integral += _ki * error * ((float)_sampleTime / 1000.0f);

        // Derivative term with filtering
        float dInput = (input - _lastInput);
        float currentDTerm = -_kd * dInput / ((float)_sampleTime / 1000.0f);
        _dTerm = (_dFilterAlpha * currentDTerm) + (1.0f - _dFilterAlpha) * _dTerm;

        // Compute total output
        float rawOutput = pTerm + _integral + _dTerm;

        // Apply limits and integral clamping
        if (rawOutput > _maxOutput) {
            _output = _maxOutput;
            // Anti-windup: if output is saturated, stop increasing integral if it would make it worse
            if (error > 0) _integral -= _ki * error * ((float)_sampleTime / 1000.0f);
        } else if (rawOutput < _minOutput) {
            _output = _minOutput;
            if (error < 0) _integral -= _ki * error * ((float)_sampleTime / 1000.0f);
        } else {
            _output = rawOutput;
        }

        // Diagnostics: check for static output
        if (abs(_output - _lastOutput) < 0.001f) {
            _staticCounter++;
        } else {
            _staticCounter = 0;
            _isStatic = false;
        }

        if (_staticCounter >= 5) {
            _isStatic = true;
        }

        _lastInput = input;
        _lastOutput = _output;
        _lastTime = now;
    }

    return _output;
}
