#include "sensors/base_sensor.h"
#include "esp_log.h"
#include <cmath>

namespace pooaway::sensors
{

    static constexpr char const *TAG = "BaseSensor";

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
            const float raw_value = read_raw();
            if (validate_reading(raw_value))
            {
                const float rs = calculate_rs(raw_value);
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
            log_error("Failed to calibrate sensor");
        }
    }

    float BaseSensor::read()
    {
        if (m_low_power_mode)
        {
            exit_low_power();
        }

        const float raw_value = read_raw();
        if (!validate_reading(raw_value))
        {
            log_error("Invalid reading detected");
            return m_value;
        }

        const float ppm = calculate_ppm(raw_value);
        if (is_valid_ppm(ppm))
        {
            update_baseline(ppm);
            update_diagnostics(raw_value, ppm);
            m_value = ppm;
        }

        return m_value;
    }

    float BaseSensor::read_raw()
    {
        const float adc_value = static_cast<float>(analogRead(m_pin));
        ESP_LOGV(TAG, "[%s] ADC value: %f", m_name, adc_value);
        return adc_value;
    }

    bool BaseSensor::check_alert() const
    {
        if (!m_alerts_enabled || m_first_reading)
        {
            return false;
        }

        const float delta = std::abs(m_value - m_baseline_ema);
        const float threshold = m_baseline_ema * m_tolerance;

        if (delta > threshold)
        {
            if (m_detect_start == 0)
            {
                m_detect_start = millis();
            }
            else if ((millis() - m_detect_start) >= m_min_detect_ms)
            {
                return true;
            }
        }
        else
        {
            m_detect_start = 0;
        }

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
        // Protect against invalid values
        if (std::isinf(new_value) || std::isnan(new_value))
        {
            ESP_LOGW(TAG, "Invalid value for baseline update: %f", m_name, new_value);
            return;
        }

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
        ESP_LOGE(TAG, "[%s] %s (errors: %lu)", m_name, message, m_diagnostics.error_count);
    }

    bool BaseSensor::validate_reading(float value) const
    {
        // For pee sensor, we expect very low voltages (0.0-0.1V) in normal conditions
        // Adjust validation thresholds accordingly
        constexpr float MIN_VALID_VOLTAGE = 0.0F;
        constexpr float MAX_VALID_VOLTAGE = 2.0F; // Increased from VCC to handle the ~1.9V readings we're seeing

        if (value < MIN_VALID_VOLTAGE || value > MAX_VALID_VOLTAGE)
        {
            ESP_LOGW(TAG, "Voltage out of range: %f", m_name, value);
            return false;
        }

        // Only check voltage delta if we have a previous reading and it's not the first reading
        if (!m_first_reading && m_diagnostics.last_voltage > 0.001F)
        {
            const float delta = std::abs(value - m_diagnostics.last_voltage);
            if (delta > MAX_VOLTAGE_DELTA)
            {
                ESP_LOGW(TAG, "Voltage delta too high: %f (previous: %f, current: %f)",
                         m_name, delta, m_diagnostics.last_voltage, value);
                return false;
            }
        }

        return true;
    }

    float BaseSensor::calculate_ppm(float voltage) const
    {
        const float rs = calculate_rs(voltage);
        const float rs_r0_ratio = rs / m_r0;

        // Prevent infinite/invalid PPM values
        if (rs_r0_ratio <= 0.0F)
        {
            return m_value; // Return last valid reading
        }

        return m_coeff_a * std::exp(m_coeff_b * rs_r0_ratio);
    }

    void BaseSensor::update_diagnostics(float raw_value, float processed_value)
    {
        // Only update diagnostics with valid values
        if (!std::isinf(processed_value) && !std::isnan(processed_value))
        {
            m_diagnostics.read_count++;
            m_diagnostics.last_voltage = raw_value;
            m_diagnostics.last_resistance = calculate_rs(raw_value);
            m_diagnostics.last_read_time = millis();

            m_diagnostics.min_value = std::min(m_diagnostics.min_value, processed_value);
            m_diagnostics.max_value = std::max(m_diagnostics.max_value, processed_value);

            // Update average only with valid readings
            m_diagnostics.avg_value = ((m_diagnostics.avg_value *
                                        (m_diagnostics.read_count - 1)) +
                                       processed_value) /
                                      m_diagnostics.read_count;
        }

        ESP_LOGV(TAG, "Diagnostics: %d Last: %f %f Time: %lu Min: %f Max: %f Avg: %f Errors: %lu",
                 m_name,
                 m_diagnostics.read_count,
                 m_diagnostics.last_voltage,
                 m_diagnostics.last_resistance,
                 m_diagnostics.last_read_time,
                 m_diagnostics.min_value,
                 m_diagnostics.max_value,
                 m_diagnostics.avg_value,
                 m_diagnostics.error_count);
    }

} // namespace pooaway::sensors