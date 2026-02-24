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

float PressureSensor::readPressure() {
    // 1. Oversampling
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

    return getPSI();
}

float PressureSensor::adcToVoltage(int adcValue) {
    // ESP32 ADC is 12-bit (0-4095)
    // At 11dB attenuation, range is approx 0-3.3V
    // Adding linear correction for ESP32 ADC non-linearity
    if (adcValue < 1) return 0.0f;
    float voltage = ((float)adcValue / 4095.0f) * 3.1f + 0.15f;
    if (voltage > 3.3f) voltage = 3.3f;
    if (voltage < 0.0f) voltage = 0.0f;
    return voltage;
}

float PressureSensor::getRawVoltage() {
    // This returns the voltage at the ADC pin
    return adcToVoltage((int)_filteredADC);
}

float PressureSensor::getPSI() {
    float vAdc = getRawVoltage();
    // Externally scaled 0-5V to 0-3.3V
    // So V_sensor = V_adc * (5.0 / 3.3)
    float vSensor = vAdc * (5.0f / 3.3f);

    // Sensor: 1V = 0 PSI, 5V = 5000 PSI
    // PSI = (V_sensor - 1.0) * (5000 / 4.0)
    float psi = (vSensor - 1.0f) * 1250.0f;

    if (psi < 0) psi = 0;
    return psi;
}
