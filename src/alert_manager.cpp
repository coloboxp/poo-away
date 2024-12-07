#include "alert_manager.h"
#include "esp_log.h"
#include "config.h"
#include "sensors.h"
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

        if (now - m_last_alert < ALERT_INTERVAL)
        {
            return;
        }

        m_last_alert = now;

        JsonDocument doc;
        doc["device_id"] = WiFi.macAddress();
        doc["timestamp"] = now;

        auto sensors_array = doc["sensors"].to<JsonArray>();

        for (size_t i = 0; i < pooaway::sensors::SENSOR_COUNT; i++)
        {
            if (alerts[i])
            {
                auto sensor = sensors_array.add<JsonObject>();
                sensor["index"] = i;
                sensor["name"] = ::sensors[i].name;
                sensor["model"] = ::sensors[i].model;
                sensor["alert"] = alerts[i];

                auto readings = sensor["readings"].to<JsonObject>();
                readings["value"] = ::sensors[i].value;
                readings["baseline"] = ::sensors[i].baselineEMA;
                readings["r0"] = ::sensors[i].cal.r0;

                auto calibration = sensor["calibration"].to<JsonObject>();
                calibration["preheating_time"] = ::sensors[i].cal.preheatingTime;
                calibration["a"] = ::sensors[i].cal.a;
                calibration["b"] = ::sensors[i].cal.b;
            }
        }

        for (auto handler : m_handlers)
        {
            try
            {
                if (handler->is_available())
                {
                    handler->handle_alert(doc);
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