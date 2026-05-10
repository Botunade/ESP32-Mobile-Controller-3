#include "sensor.h"

PressureSensor::PressureSensor(int pin, int samples)
    : _pin(pin), _numSamples(samples), _alpha(0.05f), _filteredADC(0.0f), _bufferIndex(0), _bufferSum(0.0f) {
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
    _filteredADC = (_alpha * currentADC) + ((1.0f - _alpha) * _filteredADC);

    // 3. Conversion Chain
    float voltage = adcToVoltage(_filteredADC);
    float pressureRaw = voltageToPressure(voltage, minV, maxV, maxBar);
    
    // Normalized calculation (0-1.0 over working range)
    normalized = pressureRaw / (workingMaxBar > 0 ? workingMaxBar : 1.0f);
    if (normalized > 1.0f) normalized = 1.0f;
    if (normalized < 0.0f) normalized = 0.0f;

    // Accuracy Scaling for monitor/control window
    scaled3v3 = accuracyMinV + (normalized * (accuracyMaxV - accuracyMinV));

    return pressureRaw;
}

float PressureSensor::adcToVoltage(float adcValue) {
    // 12-bit ADC (0-4095) for ~3.3V range with 11dB attenuation
    return (adcValue / 4095.0f) * 3.3f;
}

float PressureSensor::getRawVoltage() {
    return adcToVoltage(_filteredADC);
}

float PressureSensor::voltageToPressure(float voltage, float minV, float maxV, float maxBar) {
    // ================================================================
    // HARDCODED LINEAR SCALING (0.00 - 12.00 BAR)
    // ================================================================
    // Theorized Profile: 0.31V Zero @ 77.5 ohm resistor (4mA-20mA)
    //
    // BAR    | VOLTAGE  | ADC (12b)| Note
    // ------------------------------------------------------------
    // 0.00   | 0.3100 V | 385      | Zero Baseline (4mA)
    // 0.05   | 0.3152 V | 391      | (Step Resolution)
    // 1.00   | 0.4133 V | 513      | 
    // 3.00   | 0.6200 V | 769      | 
    // 6.00   | 0.9300 V | 1154     | (Midpoint)
    // 9.00   | 1.2400 V | 1539     | 
    // 12.00  | 1.5500 V | 1923     | (Full Scale)
    // ================================================================
    
    // Physical Constants
    const float V_ZERO = 0.3100f;
    const float V_SPAN = 1.2400f; // Total span for 12.0 Bar (16mA)
    const float MAX_P  = 12.000f;

    // Boundary check (ignore noise below zero)
    if (voltage <= V_ZERO) return 0.0f;

    // Linear Conversion: P = (V - V_zero) * (MaxP / V_span)
    float rawPressure = (voltage - V_ZERO) * (MAX_P / V_SPAN);
    
    // --- STEPPED RESOLUTION (0.05 BAR) ---
    // This snaps the output to the nearest 0.05 bar point.
    // Every 0.05 Bar increment is sensed as a ~5.16mV change.
    float pressure = round(rawPressure * 100.0f) / 100.0f;
    
    // Hard Limit to 12.0 Bar
    if (pressure > MAX_P) pressure = MAX_P;
    if (pressure < 0) pressure = 0;
    
    return pressure;
}
