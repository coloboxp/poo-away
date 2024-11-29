#include "Sensor.h"

Sensor::Sensor(int pin, const char* name, float alpha, float tolerance, 
               float vcc, int adcResolution)
    : pin(pin), name(name), alpha(alpha), tolerance(tolerance),
      baselineEMA(0), firstReading(true), value(0),
      vcc(vcc), adcResolution(adcResolution) {
    pinMode(pin, INPUT);
}

float Sensor::readSensor() {
    return analogRead(pin) * (vcc / adcResolution);
}

void Sensor::updateEMA() {
    if (firstReading) {
        baselineEMA = value;
        firstReading = false;
    } else {
        baselineEMA = (alpha * value) + ((1 - alpha) * baselineEMA);
    }
}

void Sensor::update() {
    value = readSensor();
    updateEMA();
}

bool Sensor::isAlertTriggered() const {
    return value > (baselineEMA + tolerance);
}

const char* Sensor::getName() const {
    return name;
}

float Sensor::getValue() const {
    return value;
}

float Sensor::getThreshold() const {
    return baselineEMA + tolerance;
} 