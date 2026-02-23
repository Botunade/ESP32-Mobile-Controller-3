#ifndef PID_CONTROLLER_H
#define PID_CONTROLLER_H

#include <Arduino.h>

class PIDController {
public:
    PIDController(float kp, float ki, float kd);
    void setTunings(float kp, float ki, float kd);
    void setOutputLimits(float min, float max);
    void setSampleTime(int millis);
    float compute(float setpoint, float input);

    void reset();

    float getKp() const { return _kp; }
    float getKi() const { return _ki; }
    float getKd() const { return _kd; }
    float getOutput() const { return _output; }

    bool isStatic() const { return _isStatic; }

private:
    float _kp, _ki, _kd;
    float _minOutput, _maxOutput;
    int _sampleTime;
    unsigned long _lastTime;

    float _integral;
    float _lastInput;
    float _lastOutput;
    float _output;

    // Derivative filter
    float _dTerm;
    float _dFilterAlpha; // 0.0 to 1.0, lower is more filtering

    // Diagnostics
    int _staticCounter;
    bool _isStatic;
};

#endif
