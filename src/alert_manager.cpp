#include "alert_manager.h"
#include "esp_log.h"
#include "config.h"
#include "sensors.h"
#include "sensor_manager.h"
#include <algorithm>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>

namespace pooaway::alert
{

    AlertManager &AlertManager::instance()
    {
        static AlertManager instance;
        return instance;
    }

    AlertManager::AlertManager() = default;

    void AlertManager::init()
    {
        ESP_LOGI(TAG, "Initializing alert manager");
        for (auto handler : m_handlers)
        {
            try
            {
                handler->init();
                if (!handler->is_available())
                {
                    ESP_LOGW(TAG, "Handler initialization failed: %s",
                             handler->get_last_error().c_str());
                }
            }
            catch (const std::exception &e)
            {
                ESP_LOGE(TAG, "Handler initialization error: %s", e.what());
            }
        }
    }

    void AlertManager::update(const bool alerts[pooaway::sensors::SENSOR_COUNT])
    {
        const unsigned long now = millis();

        if (now - m_last_alert < config::alerts::ALERT_INTERVAL)
        {
            return;
        }

        m_last_alert = now;
        ESP_LOGV(TAG, "Creating alert data document");

        JsonDocument doc;
        doc["device_id"] = WiFi.macAddress();
        doc["timestamp"] = now;

        auto sensors_array = doc["sensors"].to<JsonArray>();
        ESP_LOGV(TAG, "Processing sensors data");

        // Add all sensors regardless of alert status
        for (size_t i = 0; i < pooaway::sensors::SENSOR_COUNT; i++)
        {
            auto sensor = sensors_array.add<JsonObject>();
            sensor["index"] = i;
            sensor["name"] = ::sensors[i].name;
            sensor["model"] = ::sensors[i].model;
            sensor["alert"] = alerts[i];

            // Get sensor instance from SensorManager
            auto *sensor_ptr = pooaway::sensors::SensorManager::instance().get_sensor(static_cast<pooaway::sensors::SensorType>(i));
            if (!sensor_ptr)
                continue;

            auto readings = sensor["readings"].to<JsonObject>();
            readings["value"] = ::sensors[i].value;                          // PPM value from global array
            readings["baseline"] = ::sensors[i].baselineEMA;                 // Baseline from global array
            readings["voltage"] = sensor_ptr->get_voltage();                 // Get voltage from sensor instance
            readings["rs"] = sensor_ptr->get_rs();                           // Get Rs from sensor instance
            readings["r0"] = sensor_ptr->get_r0();                           // Get R0 from sensor instance
            readings["ratio"] = sensor_ptr->get_rs() / sensor_ptr->get_r0(); // Calculate ratio

            auto calibration = sensor["calibration"].to<JsonObject>();
            calibration["preheating_time"] = ::sensors[i].cal.preheatingTime;
            calibration["a"] = ::sensors[i].cal.a;
            calibration["b"] = ::sensors[i].cal.b;
        }

        ESP_LOGV(TAG, "Sending to %d handlers", m_handlers.size());
        for (auto handler : m_handlers)
        {
            try
            {
                if (handler->is_available())
                {
                    ESP_LOGV(TAG, "Calling handler type: %d", static_cast<int>(handler->get_type()));
                    handler->handle_alert(doc);
                }
                else
                {
                    ESP_LOGW(TAG, "Handler not available, type: %d", static_cast<int>(handler->get_type()));
                }
            }
            catch (const std::exception &e)
            {
                ESP_LOGE(TAG, "Handler update error: %s", e.what());
            }
        }
    }

    std::vector<std::string> AlertManager::get_handler_errors() const
    {
        std::vector<std::string> errors;
        for (const auto handler : m_handlers)
        {
            if (!handler->is_available())
            {
                errors.push_back(handler->get_last_error());
            }
        }
        return errors;
    }

    void AlertManager::add_handler(AlertHandler *handler)
    {
        m_handlers.push_back(handler);
        handler->init();
    }

    void AlertManager::remove_handler(AlertHandler *handler)
    {
        auto it = std::find(m_handlers.begin(), m_handlers.end(), handler);
        if (it != m_handlers.end())
        {
            m_handlers.erase(it);
        }
    }
} // namespace pooaway::alert