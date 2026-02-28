#ifndef SENSOR_H
#define SENSOR_H

#include <Arduino.h>

class PressureSensor {
public:
    PressureSensor(int pin, int samples = 16);
    ~PressureSensor();
    void begin();
    float readPressure();
    float getRawVoltage();
    float getPSI();
    float getFilteredADC() { return _filteredADC; }

private:
    int _pin;
    int _numSamples;
    float _alpha; // Exponential smoothing factor
    float _filteredADC;
    float* _movingAvgBuffer;
    int _bufferIndex;
    float _bufferSum;

    float adcToVoltage(int adcValue);
    float voltageToPSI(float voltage);
};

#endif
