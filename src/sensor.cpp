#include "sensor.h"

PressureSensor::PressureSensor(int pin, int samples)
    : _pin(pin), _numSamples(samples), _alpha(0.1f), _filteredADC(0.0f), _bufferIndex(0), _bufferSum(0.0f) {
    _movingAvgBuffer = new float[_numSamples];
    for (int i = 0; i < _numSamples; i++) {
        _movingAvgBuffer[i] = 0.0f;
    }
}

PressureSensor::~PressureSensor() {
    delete[] _movingAvgBuffer;
}

void PressureSensor::begin() {
    pinMode(_pin, INPUT);
    analogReadResolution(12);
    analogSetAttenuation(ADC_11db); // 0 - 3.3V

    // Initialize filter with a first read
    int initialRead = analogRead(_pin);
    _filteredADC = (float)initialRead;
    for (int i = 0; i < _numSamples; i++) {
        _movingAvgBuffer[i] = (float)initialRead;
    }
    _bufferSum = (float)initialRead * _numSamples;
}

float PressureSensor::readPressure(float lowV, float highV, float calFactor) {
    // 1. Oversampling (>= 16 reads)
    long sum = 0;
    for (int i = 0; i < _numSamples; i++) {
        sum += analogRead(_pin);
    }
    float currentADC = (float)sum / _numSamples;

    // 2. Moving Average Filter
    _bufferSum -= _movingAvgBuffer[_bufferIndex];
    _movingAvgBuffer[_bufferIndex] = currentADC;
    _bufferSum += _movingAvgBuffer[_bufferIndex];
    _bufferIndex = (_bufferIndex + 1) % _numSamples;
    float movingAvg = _bufferSum / _numSamples;

    // 3. Exponential Smoothing
    _filteredADC = (_alpha * movingAvg) + ((1.0f - _alpha) * _filteredADC);

    return getPressurePercent(lowV, highV, calFactor);
}

float PressureSensor::adcToVoltage(float adcValue) {
    // ESP32 ADC is 12-bit (0-4095)
    // At 11dB attenuation, range is approx 0-3.3V
    // Applying linear correction for ESP32 ADC non-linearity (approximate)
    if (adcValue < 1) return 0.0f;
    float voltage = (adcValue / 4095.0f) * 3.1f + 0.15f;
    if (voltage > 3.3f) voltage = 3.3f;
    if (voltage < 0.0f) voltage = 0.0f;
    return voltage;
}

float PressureSensor::getRawVoltage(float calFactor) {
    // Apply voltage divider compensation using calibrationFactor
    return adcToVoltage(_filteredADC) * calFactor;
}

float PressureSensor::getPressurePercent(float lowV, float highV, float calFactor) {
    float voltage = getRawVoltage(calFactor);

    // Linear mapping to 0-100% tank pressure
    if (highV == lowV) return 0.0f;
    float percent = (voltage - lowV) / (highV - lowV) * 100.0f;

    // Clamp 0-100%
    if (percent < 0.0f) percent = 0.0f;
    if (percent > 100.0f) percent = 100.0f;

    return percent;
}
