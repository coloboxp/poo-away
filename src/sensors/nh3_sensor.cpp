#include "sensors/nh3_sensor.h"

namespace pooaway::sensors
{
    static constexpr char const *TAG = "NH3Sensor";
    bool NH3Sensor::validate_reading(float raw_value) const
    {
        const float voltage = raw_value * (VCC / static_cast<float>(ADC_RESOLUTION));

        // NH3 sensor specific validation
        constexpr float MIN_VALID_VOLTAGE = 0.0F;
        constexpr float MAX_VALID_VOLTAGE = 0.1F; // NH3 sensor typically operates in low voltage

        if (voltage < MIN_VALID_VOLTAGE || voltage > MAX_VALID_VOLTAGE)
        {
            ESP_LOGW(TAG, "[%s] Voltage out of range: %f", m_name, voltage);
            return false;
        }

        return true;
    }

    float NH3Sensor::calculate_ppm(float raw_value) const
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

    bool NH3Sensor::is_valid_ppm(float ppm) const
    {
        // NH3 specific PPM validation
        constexpr float MIN_PPM = 0.0F;
        constexpr float MAX_PPM = 100.0F;
        return ppm >= MIN_PPM && ppm <= MAX_PPM;
    }

    float NH3Sensor::calculate_rs(float voltage) const
    {
        if (voltage < 0.001F)
        {
            return RL;
        }
        return RL * (VCC - voltage) / voltage;
    }
}