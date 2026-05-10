#ifndef SENSOR_H
#define SENSOR_H

#include <Arduino.h>

class PressureSensor {
public:
    PressureSensor(int pin, int samples = 16);
    ~PressureSensor();
    void begin();
    float readPressure(float minV, float maxV, float maxBar, float workingMaxBar, float accuracyMinV, float accuracyMaxV, float& normalized, float& scaled3v3);
    float getRawVoltage();
    float getFilteredADC() { return _filteredADC; }
    bool isConnected() { return getRawVoltage() > 0.1f; }

private:
    int _pin;
    int _numSamples;
    float _alpha; // Exponential smoothing factor
    float _filteredADC;
    float* _movingAvgBuffer;
    int _bufferIndex;
    float _bufferSum;

    float adcToVoltage(float adcValue);
    float voltageToPressure(float voltage, float minV, float maxV, float maxBar);
};

#endif
