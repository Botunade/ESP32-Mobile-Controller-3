#ifndef SENSOR_H
#define SENSOR_H

#include <Arduino.h>

class PressureSensor {
public:
    PressureSensor(int pin, int samples = 16);
    ~PressureSensor();
    void begin();
    float readPressure(float lowV, float highV, float calFactor);
    float getRawVoltage(float calFactor);
    float getPressurePercent(float lowV, float highV, float calFactor);

private:
    int _pin;
    int _numSamples;
    float _alpha; // Exponential smoothing factor
    float _filteredADC;
    float* _movingAvgBuffer;
    int _bufferIndex;
    float _bufferSum;

    float adcToVoltage(float adcValue);
};

#endif
