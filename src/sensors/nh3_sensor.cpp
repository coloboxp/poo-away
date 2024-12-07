#include "sensors/nh3_sensor.h"
#include <cmath>

namespace pooaway::sensors
{
    static constexpr char const *TAG = "NH3Sensor";

    bool NH3Sensor::validate_reading(float raw_value) const
    {
        const float voltage = raw_value * (VCC / static_cast<float>(ADC_RESOLUTION));

        if (voltage < MIN_VALID_VOLTAGE || voltage > MAX_VALID_VOLTAGE)
        {
            ESP_LOGW(TAG, "[%s] Voltage out of range: %.2fV (raw: %.0f)",
                     m_name, voltage, raw_value);
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
            ESP_LOGW(TAG, "[%s] Invalid Rs/R0 ratio: %.2f", m_name, rs_r0_ratio);
            return m_value;
        }

        const float ppm = m_coeff_a * std::pow(rs_r0_ratio, m_coeff_b);

        ESP_LOGD(TAG, "[%s] V=%.2f Rs=%.0f R0=%.0f ratio=%.2f PPM=%.1f",
                 m_name, voltage, rs, m_r0, rs_r0_ratio, ppm);

        return ppm;
    }

    bool NH3Sensor::is_valid_ppm(float ppm) const
    {
        if (ppm < MIN_VALID_PPM || ppm > MAX_VALID_PPM)
        {
            ESP_LOGW(TAG, "[%s] PPM out of range: %.1f", m_name, ppm);
            return false;
        }
        return true;
    }

    float NH3Sensor::calculate_rs(float voltage) const
    {
        if (voltage < 0.001F)
        {
            ESP_LOGV(TAG, "[%s] Voltage too low for Rs calculation: %.3f", m_name, voltage);
            return RL;
        }
        return RL * (VCC - voltage) / voltage;
    }

    bool NH3Sensor::validate_r0(float r0) const
    {
        if (r0 <= 0.0F)
        {
            ESP_LOGE(TAG, "[%s] Invalid R0 value: %.1f", m_name, r0);
            return false;
        }

        if (r0 < MIN_VALID_R0 || r0 > MAX_VALID_R0)
        {
            ESP_LOGW(TAG, "[%s] R0 out of typical range: %.1f", m_name, r0);
            return false;
        }

        return true;
    }

    void NH3Sensor::set_r0(float r0)
    {
        if (!validate_r0(r0))
        {
            const float voltage = read_raw() * (VCC / static_cast<float>(ADC_RESOLUTION));
            const float rs = calculate_rs(voltage);
            const float default_r0 = std::max(rs, MIN_VALID_R0);

            ESP_LOGW(TAG, "[%s] Invalid R0 (%.1f), using calculated value: %.1f",
                     m_name, r0, default_r0);
            m_r0 = default_r0;
        }
        else
        {
            m_r0 = r0;
        }

        m_needs_calibration = false;
        ESP_LOGI(TAG, "[%s] Sensor calibrated with R0=%.1f", m_name, m_r0);
    }
}