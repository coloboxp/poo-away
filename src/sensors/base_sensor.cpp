#include "sensors/base_sensor.h"
#include "esp_log.h"
#include <cmath>

namespace pooaway::sensors
{

    static constexpr char const *TAG = "BaseSensor";
    static constexpr int ERROR_THRESHOLD = 10;
    static constexpr float MAX_VOLTAGE_DELTA = 0.5F;

    BaseSensor::BaseSensor(const char *model, const char *name, int pin,
                           float alpha, float tolerance, float preheating_time,
                           int min_detect_ms, float coeff_a, float coeff_b)
        : m_model(model), m_name(name), m_pin(pin), m_alpha(alpha), m_tolerance(tolerance), m_preheating_time(preheating_time), m_min_detect_ms(min_detect_ms), m_coeff_a(coeff_a), m_coeff_b(coeff_b)
    {
    }

    void BaseSensor::init()
    {
        pinMode(m_pin, INPUT);
        m_needs_calibration = true;
    }

    void BaseSensor::calibrate()
    {
        ESP_LOGI(TAG, "Starting calibration for %s sensor...", m_name);

        // Take multiple readings to establish R0
        constexpr int NUM_READINGS = 100;
        float sum = 0.0F;
        int valid_readings = 0;

        for (int i = 0; i < NUM_READINGS; i++)
        {
            const float adc_value = static_cast<float>(analogRead(m_pin));
            const float voltage = adc_value * (VCC / static_cast<float>(ADC_RESOLUTION));

            if (voltage > 0.001F)
            {
                const float rs = calculate_rs(voltage);
                sum += rs;
                valid_readings++;
            }
            delay(100);
        }

        if (valid_readings > 0)
        {
            m_r0 = sum / static_cast<float>(valid_readings);
            m_needs_calibration = false;
            m_diagnostics.calibration_count++;
            ESP_LOGI(TAG, "Calibration complete for %s. R0=%.1f", m_name, m_r0);
        }
        else
        {
            ESP_LOGE(TAG, "Calibration failed for %s", m_name);
            m_r0 = RL;
        }
    }

    float BaseSensor::read()
    {
        if (m_low_power_mode)
        {
            exit_low_power();
        }

        const float adc_value = static_cast<float>(analogRead(m_pin));
        const float voltage = adc_value * (VCC / static_cast<float>(ADC_RESOLUTION));

        if (!validate_reading(voltage))
        {
            log_error("Invalid reading detected");
            return m_value;
        }

        const float ppm = calculate_ppm(voltage);
        update_baseline(ppm);
        update_diagnostics(voltage, ppm);
        m_value = ppm;

        return ppm;
    }

    bool BaseSensor::check_alert()
    {
        if (m_first_reading || !m_value || !m_alerts_enabled)
        {
            return false;
        }

        const float ratio = m_value / m_baseline_ema;
        const bool threshold_exceeded = ratio > (1.0F + m_tolerance);

        if (threshold_exceeded)
        {
            if (m_detect_start == 0UL)
            {
                m_detect_start = millis();
                return false;
            }
            const bool alert_triggered = (millis() - m_detect_start) >= m_min_detect_ms;
            if (alert_triggered)
            {
                m_diagnostics.alert_count++;
            }
            return alert_triggered;
        }

        m_detect_start = 0UL;
        return false;
    }

    float BaseSensor::calculate_rs(float voltage) const
    {
        if (voltage < 0.001F)
        {
            voltage = 0.001F;
        }
        return RL * ((VCC / voltage) - 1.0F);
    }

    void BaseSensor::update_baseline(float new_value)
    {
        if (m_first_reading)
        {
            m_baseline_ema = new_value;
            m_first_reading = false;
        }
        else
        {
            m_baseline_ema = (m_alpha * new_value) + ((1.0F - m_alpha) * m_baseline_ema);
        }
    }

    void BaseSensor::run_self_test()
    {
        ESP_LOGI(TAG, "Running self-test for %s sensor...", m_name);

        // Test ADC reading
        const float test_value = analogRead(m_pin);
        if (test_value <= 0 || test_value >= ADC_RESOLUTION)
        {
            log_error("ADC reading out of range");
            return;
        }

        // Test voltage calculation
        const float voltage = test_value * (VCC / static_cast<float>(ADC_RESOLUTION));
        if (voltage <= 0.0F || voltage >= VCC)
        {
            log_error("Voltage calculation error");
            return;
        }

        // Test sensor resistance
        const float rs = calculate_rs(voltage);
        if (rs <= 0.0F || rs >= 1000000.0F)
        { // 1MΩ max
            log_error("Sensor resistance out of range");
            return;
        }

        m_diagnostics.is_healthy = true;
        ESP_LOGI(TAG, "Self-test passed for %s sensor", m_name);
    }

    void BaseSensor::enter_low_power()
    {
        if (!m_low_power_mode)
        {
            ESP_LOGI(TAG, "Entering low power mode for %s sensor", m_name);
            m_low_power_mode = true;

            // Configure ADC for low power
            analogSetPinAttenuation(m_pin, ADC_11db);

            // Record timing for power management
            m_diagnostics.total_active_time +=
                (millis() - m_diagnostics.last_read_time);
        }
    }

    void BaseSensor::exit_low_power()
    {
        if (m_low_power_mode)
        {
            ESP_LOGI(TAG, "Exiting low power mode for %s sensor", m_name);
            m_low_power_mode = false;

            // Restore normal ADC configuration
            analogSetPinAttenuation(m_pin, ADC_0db);
            m_diagnostics.last_read_time = millis();
        }
    }

    void BaseSensor::log_error(const char *message)
    {
        m_diagnostics.error_count++;
        m_diagnostics.is_healthy = (m_diagnostics.error_count < ERROR_THRESHOLD);
        ESP_LOGE(TAG, "[%s] %s (errors: %lu)", m_name, message,
                 m_diagnostics.error_count);
    }

    bool BaseSensor::validate_reading(float value) const
    {
        if (value <= 0.0F || value >= VCC)
        {
            return false;
        }

        if (!m_first_reading &&
            std::abs(value - m_diagnostics.last_voltage) > MAX_VOLTAGE_DELTA)
        {
            return false;
        }

        return true;
    }

    void BaseSensor::update_diagnostics(float raw_value, float processed_value)
    {
        m_diagnostics.read_count++;
        m_diagnostics.last_voltage = raw_value;
        m_diagnostics.last_resistance = calculate_rs(raw_value);
        m_diagnostics.last_read_time = millis();

        m_diagnostics.min_value = std::min(m_diagnostics.min_value, processed_value);
        m_diagnostics.max_value = std::max(m_diagnostics.max_value, processed_value);
        m_diagnostics.avg_value = ((m_diagnostics.avg_value *
                                    (m_diagnostics.read_count - 1)) +
                                   processed_value) /
                                  m_diagnostics.read_count;
    }

} // namespace pooaway::sensors