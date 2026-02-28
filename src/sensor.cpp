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

float PressureSensor::readPressure(float minV, float maxV, float maxBar, float workingMaxBar, float accuracyMinV, float accuracyMaxV, float& normalized, float& scaled3v3) {
    // 1. Oversampling
    long sum = 0;
    for (int i = 0; i < _numSamples; i++) {
        sum += analogRead(_pin);
    }
    float currentADC = (float)sum / _numSamples;

    // 2. Moving Average Filter (Exponential Smoothing)
    // We use exponential smoothing as the primary filter for a 4-20mA stable signal
    _filteredADC = (_alpha * currentADC) + ((1.0f - _alpha) * _filteredADC);

    // 3. Conversion Chain
    float voltage = adcToVoltage(_filteredADC);
    float pressureRaw = voltageToPressure(voltage, minV, maxV, maxBar);
    
    // Normalized calculation (0-1.0 over working range)
    normalized = pressureRaw / workingMaxBar;
    if (normalized > 1.0f) normalized = 1.0f;
    if (normalized < 0.0f) normalized = 0.0f;

    // Accuracy Scaling: Map 0-1.0 (normalized) to Accuracy Range (e.g. 0.66V - 3.3V)
    // This maps the used portion of the sensor to the full monitor/control window.
    scaled3v3 = accuracyMinV + (normalized * (accuracyMaxV - accuracyMinV));

    return pressureRaw;
}

float PressureSensor::adcToVoltage(float adcValue) {
    // 12-bit ADC (0-4095) for 3.3V range
    return (adcValue / 4095.0f) * 3.3f;
}

float PressureSensor::getRawVoltage() {
    return adcToVoltage(_filteredADC);
}

float PressureSensor::voltageToPressure(float voltage, float minV, float maxV, float maxBar) {
    // Linear interpolation for 4-20mA signal
    // pressure = ((V - Vmin) / (Vmax - Vmin)) * Pmax
    float pressure = ((voltage - minV) / (maxV - minV)) * maxBar;
    
    // Clamping
    if (pressure < 0) pressure = 0;
    if (pressure > maxBar) pressure = maxBar;
    
    return pressure;
}
