#pragma once
#include "sensors/interfaces.h"
#include <vector>

namespace pooaway::sensors
{
    class CalibrationService
    {
    public:
        static constexpr int CALIBRATION_SAMPLES = 10;

        static float calibrate_sensor(BaseSensor &sensor, int samples)
        {
            std::vector<float> readings;
            readings.reserve(samples);

            // Take multiple readings
            for (int i = 0; i < samples; i++)
            {
                const float raw_value = sensor.read_raw();
                if (!sensor.validate_reading(raw_value))
                {
                    continue;
                }

                const float rs = sensor.calculate_rs(raw_value);
                readings.push_back(rs);
                delay(100); // Small delay between readings
            }

            if (readings.empty())
            {
                return 0.0F;
            }

            // Calculate average R0
            float sum = 0.0F;
            for (float reading : readings)
            {
                sum += reading;
            }

            return sum / static_cast<float>(readings.size());
        }
    };
} // namespace pooaway::sensors