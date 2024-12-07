#pragma once
#include <Arduino.h>
#include "config.h"
#include "sensors/sensor_types.h"

// Calibration data structure
struct SensorCalibration
{
    float r0{1.0F};                     // Base resistance in clean air
    const int numReadingsForR0{100};    // Number of readings to establish R0
    const float preheatingTime{180.0F}; // Time in seconds needed for preheating
    const float a{0.0F};                // Exponential coefficient a in: ppm = a * e^(b * rs_r0)
    const float b{0.0F};                // Exponential coefficient b
};

// Sensor data structure
struct SensorData
{
    const int pin;                  // ADC pin number
    const char *model;              // Sensor model
    const char *name;               // Sensor name for logging
    const float alpha;              // EMA filter coefficient
    const float tolerance;          // Threshold for alerts
    float baselineEMA{1.0F};        // Baseline for comparison
    bool firstReading{true};        // First reading flag
    float value{1.0F};              // Current sensor value
    bool alertsEnabled{false};      // Alert state
    SensorCalibration cal;          // Calibration data
    const int minDetectMs;          // Minimum detection time before alert
    unsigned long detectStart{0UL}; // Detection start timestamp
};

// Sensor initialization data
extern SensorData sensors[pooaway::sensors::SENSOR_COUNT];