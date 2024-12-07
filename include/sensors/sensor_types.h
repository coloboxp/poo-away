#pragma once
#include <cstddef>
#include <Arduino.h>

namespace pooaway::sensors
{

    enum class SensorType
    {
        PEE,         // NH3 sensor
        POO,         // CH4 sensor
        SENSOR_COUNT // Total number of sensors
    };

    // Constants for sensor operations
    constexpr int ERROR_THRESHOLD = 10;                                            // Maximum allowed errors before marking unhealthy
    constexpr float MAX_VOLTAGE_DELTA = 0.5F;                                      // Maximum allowed voltage change between readings
    constexpr size_t SENSOR_COUNT = static_cast<size_t>(SensorType::SENSOR_COUNT); // Number of sensors

} // namespace pooaway::sensors