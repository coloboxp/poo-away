#include "sensors/ch4_sensor.h"

namespace pooaway::sensors
{
    static constexpr char const *TAG = "CH4Sensor";

    bool CH4Sensor::validate_reading(float raw_value) const
    {
        const float voltage = raw_value * (VCC / static_cast<float>(ADC_RESOLUTION));

        if (voltage < MIN_VALID_VOLTAGE || voltage > MAX_VALID_VOLTAGE)
        {
            ESP_LOGW(TAG, "Voltage out of range: %f", m_name, voltage);
            return false;
        }

        return true;
    }

    float CH4Sensor::calculate_ppm(float raw_value) const
    {
        const float voltage = raw_value * (VCC / static_cast<float>(ADC_RESOLUTION));
        const float rs = calculate_rs(voltage);
        const float rs_r0_ratio = rs / m_r0;

        if (rs_r0_ratio <= 0.0F)
        {
            return m_value;
        }

        return m_coeff_a * std::exp(m_coeff_b * rs_r0_ratio);
    }

    bool CH4Sensor::is_valid_ppm(float ppm) const
    {
        return ppm >= MIN_VALID_PPM && ppm <= MAX_VALID_PPM;
    }

    float CH4Sensor::calculate_rs(float voltage) const
    {
        if (voltage < 0.001F)
        {
            return RL;
        }
        return RL * (VCC - voltage) / voltage;
    }
}