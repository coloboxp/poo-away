#pragma once
#include <cstdint>

namespace pooaway::sensors
{

    struct SensorDiagnostics
    {
        uint32_t read_count{0};
        uint32_t error_count{0};
        float min_value{99999.0F};
        float max_value{0.0F};
        float avg_value{0.0F};
        uint32_t alert_count{0};
        uint32_t calibration_count{0};
        float last_voltage{0.0F};
        float last_resistance{0.0F};
        unsigned long last_read_time{0};
        unsigned long total_active_time{0};
        bool is_healthy{true};
    };

} // namespace pooaway::sensors