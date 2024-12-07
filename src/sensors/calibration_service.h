#pragma once
#include "sensors/interfaces.h"
#include "esp_log.h"

namespace pooaway::sensors
{
    class CalibrationService
    {
    public:
        static float calibrate_sensor(ISensorReading &sensor, int samples)
        {
            float sum = 0.0F;
            int valid_readings = 0;

            for (int i = 0; i < samples; i++)
            {
                const float raw_value = sensor.read_raw();
                if (sensor.validate_reading(raw_value))
                {
                    const float rs = sensor.calculate_rs(
                        raw_value * (VCC / static_cast<float>(ADC_RESOLUTION)));
                    if (rs > 0.0F)
                    {
                        sum += rs;
                        valid_readings++;
                    }
                }
                delay(100);
            }

            return valid_readings > 0 ? sum / static_cast<float>(valid_readings) : 10000.0F;
        }
    };
}