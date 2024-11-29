#pragma once
#include <Arduino.h>

enum class SensorType {
    PEE,
    POO,
    SENSOR_COUNT
};

class Sensor {
private:
    const int pin;
    const char* name;
    const float alpha;
    const float tolerance;
    float baselineEMA;
    bool firstReading;
    float value;
    const float vcc;
    const int adcResolution;

public:
    Sensor(int pin, const char* name, float alpha, float tolerance, 
           float vcc = 3.3, int adcResolution = 4095);
    
    void update();
    bool isAlertTriggered() const;
    const char* getName() const;
    float getValue() const;
    float getThreshold() const;
    
private:
    float readSensor();
    void updateEMA();
}; 