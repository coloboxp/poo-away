#include "sensors.h"
#include "config.h"
#pragma warning disable format
SensorData sensors[pooaway::sensors::SENSOR_COUNT] = {
    // NH3 sensor (GM-802B)
    {
        .pin = config::hardware::PEE_SENSOR_PIN,
        .model = "GM-802B",
        .name = "PEE",
        .alpha = 0.01F,
        .tolerance = 0.2F,
        .baselineEMA = 1.0F,
        .firstReading = true,
        .value = 1.0F,
        .alertsEnabled = true,
        .cal = {
            .r0 = 1.0F,
            .numReadingsForR0 = 100,
            .preheatingTime = 180.0F,
            .a = 10.938F,
            .b = 1.7742F},
        .minDetectMs = 5000,
        .detectStart = 0UL},
    // CH4 sensor (GM-402B)
    {.pin = config::hardware::POO_SENSOR_PIN, .model = "GM-402B", .name = "POO", .alpha = 0.005F, .tolerance = 0.3F, .baselineEMA = 1.0F, .firstReading = true, .value = 1.0F, .alertsEnabled = false, .cal = {.r0 = 1.0F, .numReadingsForR0 = 100, .preheatingTime = 180.0F, .a = 26.572F, .b = 1.2894F}, .minDetectMs = 5000, .detectStart = 0UL}};