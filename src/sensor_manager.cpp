#include "sensor_manager.h"
#include "esp_log.h"
#include "config.h"

namespace pooaway::sensors
{

    SensorManager &SensorManager::instance()
    {
        static SensorManager instance;
        return instance;
    }

    SensorManager::SensorManager()
    {
        m_preferences.begin("pooaway", false);

        // Create sensor instances
        m_nh3_sensor = std::make_unique<NH3Sensor>(PEE_SENSOR_PIN);
        m_ch4_sensor = std::make_unique<CH4Sensor>(POO_SENSOR_PIN);

        // Setup sensor array for easy access
        m_sensors[static_cast<size_t>(SensorType::PEE)] = m_nh3_sensor.get();
        m_sensors[static_cast<size_t>(SensorType::POO)] = m_ch4_sensor.get();
    }

    void SensorManager::init()
    {
        ESP_LOGI(TAG, "Initializing sensors...");

        // Initialize each sensor
        for (auto *sensor : m_sensors)
        {
            sensor->init();

            // Load calibration from preferences if available
            const float saved_r0 = m_preferences.getFloat(sensor->get_name(), 0.0F);
            if (saved_r0 > 0.0F)
            {
                sensor->set_r0(saved_r0);
                ESP_LOGI(TAG, "Loaded calibration for %s: R0=%.1f",
                         sensor->get_name(), saved_r0);
            }
        }
    }

    void SensorManager::update()
    {
        if (m_calibration_in_progress)
        {
            return;
        }

        for (auto *sensor : m_sensors)
        {
            sensor->read();
        }
    }

    bool SensorManager::needs_calibration() const
    {
        for (const auto *sensor : m_sensors)
        {
            if (sensor->needs_calibration())
            {
                return true;
            }
        }
        return false;
    }

    void SensorManager::perform_clean_air_calibration()
    {
        if (m_calibration_in_progress)
        {
            ESP_LOGI(TAG, "Calibration already in progress");
            return;
        }

        m_calibration_in_progress = true;
        digitalWrite(CALIBRATION_LED_PIN, HIGH);

        ESP_LOGI(TAG, "Starting clean air calibration...");

        for (auto *sensor : m_sensors)
        {
            sensor->calibrate();
            // Save calibration to preferences
            m_preferences.putFloat(sensor->get_name(), sensor->get_r0());
        }

        m_calibration_in_progress = false;
        digitalWrite(CALIBRATION_LED_PIN, LOW);
        ESP_LOGI(TAG, "Calibration complete");
    }

    void SensorManager::set_alerts_enabled(SensorType sensor_type, bool enable_state)
    {
        if (auto *sensor = get_sensor(sensor_type))
        {
            sensor->set_alerts_enabled(enable_state);
            ESP_LOGI(TAG, "%s alerts %s",
                     sensor->get_name(),
                     enable_state ? "enabled" : "disabled");
        }
    }

    bool SensorManager::get_alert_status(SensorType sensor_type) const
    {
        if (const auto *sensor = m_sensors[static_cast<size_t>(sensor_type)])
        {
            // Cast away const since check_alert() is no longer const
            return const_cast<BaseSensor *>(sensor)->check_alert();
        }
        return false;
    }

    float SensorManager::get_sensor_value(SensorType sensor_type) const
    {
        if (const auto *sensor = m_sensors[static_cast<size_t>(sensor_type)])
        {
            return sensor->get_value();
        }
        return 0.0F;
    }

    BaseSensor *SensorManager::get_sensor(SensorType sensor_type)
    {
        const auto index = static_cast<size_t>(sensor_type);
        if (index >= SENSOR_COUNT)
        {
            ESP_LOGE(TAG, "Invalid sensor type: %d", static_cast<int>(sensor_type));
            return nullptr;
        }
        return m_sensors[index];
    }

    void SensorManager::enter_low_power_mode()
    {
        ESP_LOGI(TAG, "Entering low power mode for all sensors");
        for (auto *sensor : m_sensors)
        {
            sensor->enter_low_power();
        }
    }

    void SensorManager::exit_low_power_mode()
    {
        ESP_LOGI(TAG, "Exiting low power mode for all sensors");
        for (auto *sensor : m_sensors)
        {
            sensor->exit_low_power();
        }
    }

    void SensorManager::run_diagnostics()
    {
        ESP_LOGI(TAG, "Running diagnostics for all sensors");
        for (auto *sensor : m_sensors)
        {
            sensor->run_self_test();
            const auto &diag = sensor->get_diagnostics();

            ESP_LOGI(TAG, "Sensor %s diagnostics:", sensor->get_name());
            ESP_LOGI(TAG, "  Health: %s", diag.is_healthy ? "OK" : "FAIL");
            ESP_LOGI(TAG, "  Readings: %lu (Errors: %lu)",
                     diag.read_count, diag.error_count);
            ESP_LOGI(TAG, "  Values - Min: %.2f, Max: %.2f, Avg: %.2f",
                     diag.min_value, diag.max_value, diag.avg_value);
            ESP_LOGI(TAG, "  Active time: %lu ms", diag.total_active_time);
        }
    }

} // namespace pooaway::sensors