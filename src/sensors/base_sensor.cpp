#include "sensors/base_sensor.h"
#include "sensors/calibration_service.h"

namespace pooaway::sensors
{
    static constexpr char const *TAG = "BaseSensor";
    static constexpr int CALIBRATION_SAMPLES = 10;

    BaseSensor::BaseSensor(const char *model, const char *name, int pin,
                           float alpha, float tolerance, float preheating_time,
                           int min_detect_ms, float coeff_a, float coeff_b)
        : m_model(model), m_name(name), m_pin(pin), m_alpha(alpha), m_tolerance(tolerance), m_preheating_time(preheating_time), m_min_detect_ms(min_detect_ms), m_coeff_a(coeff_a), m_coeff_b(coeff_b)
    {
    }

    void BaseSensor::init()
    {
        ESP_LOGI(TAG, "Initializing %s sensor...", m_name);
        pinMode(m_pin, INPUT);

        // Wait for sensor to warm up
        delay(static_cast<unsigned long>(m_preheating_time * 1000.0F));
        m_needs_calibration = true;
        m_alerts_enabled = true;
    }

    void BaseSensor::read()
    {
        if (m_low_power_mode)
        {
            return;
        }

        const float raw_value = read_raw();
        if (!validate_reading(raw_value))
        {
            ESP_LOGW(TAG, "Invalid reading from %s sensor: %.2f", m_name, raw_value);
            return;
        }

        const float ppm = calculate_ppm(raw_value);
        if (!is_valid_ppm(ppm))
        {
            ESP_LOGW(TAG, "Invalid PPM from %s sensor: %.2f", m_name, ppm);
            return;
        }

        // Update baseline using EMA
        if (m_first_reading)
        {
            m_baseline_ema = ppm;
            m_first_reading = false;
        }
        else
        {
            m_baseline_ema = (m_alpha * ppm) + ((1.0F - m_alpha) * m_baseline_ema);
        }
        m_value = ppm;
    }

    float BaseSensor::read_raw() const
    {
        const float raw_value = static_cast<float>(analogRead(m_pin));
        ESP_LOGD(TAG, "Raw value from %s sensor: %.2f", m_name, raw_value);
        return raw_value;
    }

    void BaseSensor::calibrate()
    {
        ESP_LOGI(TAG, "Starting calibration for %s sensor...", m_name);

        float r0 = CalibrationService::calibrate_sensor(*this, CALIBRATION_SAMPLES);
        if (validate_r0(r0))
        {
            set_r0(r0);
            m_needs_calibration = false;
            ESP_LOGI(TAG, "Calibration complete for %s. R0=%.1f", m_name, m_r0);
        }
        else
        {
            ESP_LOGE(TAG, "Calibration failed for %s. Invalid R0=%.1f", m_name, r0);
        }
    }

    bool BaseSensor::check_alert() const
    {
        if (!m_alerts_enabled || m_first_reading)
        {
            return false;
        }

        const float deviation = std::abs(m_value - m_baseline_ema);
        const bool threshold_exceeded = (deviation > m_tolerance * m_baseline_ema);

        if (threshold_exceeded)
        {
            if (m_detect_start == 0)
            {
                m_detect_start = millis();
            }
            return (millis() - m_detect_start) >= static_cast<unsigned long>(m_min_detect_ms);
        }

        m_detect_start = 0;
        return false;
    }

    void BaseSensor::enter_low_power()
    {
        if (!m_low_power_mode)
        {
            ESP_LOGI(TAG, "Entering low power mode for %s sensor", m_name);
            m_low_power_mode = true;
        }
    }

    void BaseSensor::exit_low_power()
    {
        if (m_low_power_mode)
        {
            ESP_LOGI(TAG, "Exiting low power mode for %s sensor", m_name);
            m_low_power_mode = false;
            // Reset readings
            m_first_reading = true;
            m_baseline_ema = 0.0F;
        }
    }

    void BaseSensor::run_self_test()
    {
        ESP_LOGI(TAG, "Running self-test for %s sensor...", m_name);

        const float raw_value = read_raw();
        if (!validate_reading(raw_value))
        {
            ESP_LOGE(TAG, "Self-test failed for %s: Invalid reading %.2f", m_name, raw_value);
            return;
        }

        const float ppm = calculate_ppm(raw_value);
        if (!is_valid_ppm(ppm))
        {
            ESP_LOGE(TAG, "Self-test failed for %s: Invalid PPM %.2f", m_name, ppm);
            return;
        }

        ESP_LOGI(TAG, "Self-test passed for %s. Raw: %.2f, PPM: %.2f",
                 m_name, raw_value, ppm);
    }

} // namespace pooaway::sensors