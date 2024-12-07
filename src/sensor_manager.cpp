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
        : m_nh3_sensor(std::make_unique<NH3Sensor>(PEE_SENSOR_PIN)), m_ch4_sensor(std::make_unique<CH4Sensor>(POO_SENSOR_PIN))
    {
        m_preferences.begin("pooaway", false);

        // Initialize sensor array
        m_sensors[static_cast<size_t>(SensorType::PEE)] = m_nh3_sensor.get();
        m_sensors[static_cast<size_t>(SensorType::POO)] = m_ch4_sensor.get();
    }

    void SensorManager::init()
    {
        ESP_LOGI(TAG, "Initializing sensors...");

        for (auto *sensor : m_sensors)
        {
            if (sensor == nullptr)
                continue;

            sensor->init();

            // Load calibration from preferences if available
            const float saved_r0 = m_preferences.getFloat(sensor->get_name(), 0.0F);
            if (saved_r0 > 0.0F)
            {
                ESP_LOGI(TAG, "Loaded calibration for %s: R0=%.1f",
                         sensor->get_name(), saved_r0);
                sensor->set_r0(saved_r0);
            }
            else
            {
                sensor->calibrate();
            }
        }
    }

    void SensorManager::update()
    {
        for (auto *sensor : m_sensors)
        {
            if (sensor != nullptr)
            {
                sensor->read();
            }
        }
    }

    void SensorManager::perform_clean_air_calibration()
    {
        ESP_LOGI(TAG, "Starting clean air calibration...");

        for (auto *sensor : m_sensors)
        {
            if (sensor == nullptr)
                continue;

            sensor->calibrate();

            // Save calibration to preferences
            m_preferences.putFloat(sensor->get_name(), sensor->get_r0());
        }

        ESP_LOGI(TAG, "Calibration complete");
        run_diagnostics();
    }

    float SensorManager::get_sensor_value(SensorType type) const
    {
        const auto *sensor = m_sensors[static_cast<size_t>(type)];
        return sensor ? sensor->get_value() : 0.0F;
    }

    bool SensorManager::get_alert_status(SensorType type) const
    {
        const auto *sensor = m_sensors[static_cast<size_t>(type)];
        return sensor ? sensor->check_alert() : false;
    }

    BaseSensor *SensorManager::get_sensor(SensorType type)
    {
        return m_sensors[static_cast<size_t>(type)];
    }

    void SensorManager::run_diagnostics()
    {
        ESP_LOGI(TAG, "Running diagnostics for all sensors");

        for (auto *sensor : m_sensors)
        {
            if (sensor == nullptr)
                continue;

            const float value = sensor->get_value();
            const float r0 = sensor->get_r0();

            ESP_LOGI(TAG, "[%s] Value: %.2f, R0: %.1f",
                     sensor->get_name(), value, r0);
        }
    }

    bool SensorManager::needs_calibration() const
    {
        for (const auto *sensor : m_sensors)
        {
            if (sensor && sensor->needs_calibration())
            {
                return true;
            }
        }
        return false;
    }

} // namespace pooaway::sensors