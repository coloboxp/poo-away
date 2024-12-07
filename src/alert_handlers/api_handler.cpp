#include "alert_handlers/api_handler.h"
#include "esp_log.h"
#include "config.h"
#include "private.h"
#include <Arduino.h>

namespace pooaway::alert
{
    ApiHandler::ApiHandler(unsigned long rate_limit_ms)
        : m_rate_limit_ms(rate_limit_ms)
    {
        m_type = HandlerType::DATA_PUBLISHER;
    }

    void ApiHandler::init()
    {
        ESP_LOGI(TAG, "Initializing API handler");
        if (WiFi.status() == WL_CONNECTED)
        {
            m_available = true;
            ESP_LOGI(TAG, "API handler initialized");
            if (m_rate_limit_ms > 0)
            {
                ESP_LOGI(TAG, "Rate limiting enabled: %lu ms", m_rate_limit_ms);
            }
        }
        else
        {
            m_last_error = "WiFi not connected";
            ESP_LOGE(TAG, "%s", m_last_error.c_str());
        }
    }

    void ApiHandler::handle_alert(JsonDocument &alert_data)
    {
        if (!m_available)
            return;

        // Check rate limiting
        if (m_rate_limit_ms > 0)
        {
            unsigned long now = millis();
            if (now - m_last_request < m_rate_limit_ms)
            {
                ESP_LOGD(TAG, "Rate limited, skipping request");
                return;
            }
            m_last_request = now;
        }

        if (!WiFiManager::instance().ensure_connected())
        {
            m_last_error = "WiFi connection lost";
            ESP_LOGE(TAG, "%s", m_last_error.c_str());
            return;
        }

        // Create the JSON payload in Adafruit IO format
        JsonDocument payload_doc;
        auto feeds = payload_doc["feeds"].to<JsonArray>();

        // Process all sensors regardless of alert status
        auto sensors = alert_data["sensors"].as<JsonArray>();

        // Debug log the input data
        ESP_LOGD(TAG, "Processing sensor data:");
        for (JsonObject sensor : sensors)
        {
            ESP_LOGD(TAG, "Sensor %s: value=%.2f, baseline=%.2f, alert=%d",
                     sensor["name"].as<const char*>(),
                     sensor["readings"]["value"].as<float>(),
                     sensor["readings"]["baseline"].as<float>(),
                     sensor["alert"].as<bool>());
        }

        for (JsonObject sensor : sensors)
        {
            String sensor_name = sensor["name"].as<const char*>();
            sensor_name.toLowerCase(); // Convert to lowercase for consistent naming
            
            const auto readings = sensor["readings"].as<JsonObject>();
            
            // Add all sensor readings
            const std::pair<const char*, float> values[] = {
                {"", readings["value"].as<float>()},                    // Main value
                {".baseline", readings["baseline"].as<float>()},        // Baseline
                {".voltage", readings["voltage"].as<float>()},          // Voltage
                {".rs", readings["rs"].as<float>()},                   // Rs value
                {".r0", readings["r0"].as<float>()},                   // R0 value
                {".ratio", readings["ratio"].as<float>()},             // Ratio
                {".alert", sensor["alert"].as<bool>()}                 // Alert status
            };

            // Add each reading type to the feeds
            for (const auto& [suffix, value] : values)
            {
                auto feed = feeds.add<JsonObject>();
                feed["key"] = String("pooaway.") + sensor_name + suffix;
                feed["value"] = value;
            }

            // Add calibration data
            const auto cal = sensor["calibration"].as<JsonObject>();
            const std::pair<const char*, float> cal_values[] = {
                {".cal_a", cal["a"].as<float>()},
                {".cal_b", cal["b"].as<float>()}
            };

            for (const auto& [suffix, value] : cal_values)
            {
                auto feed = feeds.add<JsonObject>();
                feed["key"] = String("pooaway.") + sensor_name + suffix;
                feed["value"] = value;
            }

            // Add preheating time separately as it's an integer
            auto feed = feeds.add<JsonObject>();
            feed["key"] = String("pooaway.") + sensor_name + ".preheating_time";
            feed["value"] = cal["preheating_time"].as<int>();
        }

        // Only proceed if we have feeds to send
        if (feeds.size() == 0)
        {
            ESP_LOGW(TAG, "No valid sensor data to send");
            return;
        }

        String payload;
        serializeJson(payload_doc, payload);
        ESP_LOGI(TAG, "Sending payload: %s", payload.c_str());

        m_http_client.begin(API_ENDPOINT);
        m_http_client.addHeader("Content-Type", "application/json");
        m_http_client.addHeader("X-AIO-Key", AIO_KEY);
        m_http_client.setTimeout(API_TIMEOUT);

        int retries = 0;
        int httpCode;

        do
        {
            httpCode = m_http_client.POST(payload);

            if (httpCode == HTTP_CODE_OK)
            {
                String response = m_http_client.getString();
                ESP_LOGI(TAG, "API Response: %s", response.c_str());
                m_http_client.end();
                return;
            }

            ESP_LOGW(TAG, "HTTP POST failed, code: %d, retrying...", httpCode);
            delay(RETRY_DELAY_MS);
            retries++;
        } while (retries < MAX_RETRIES);

        m_last_error = std::string("Failed to send HTTP request: ") + std::to_string(httpCode);
        ESP_LOGE(TAG, "%s", m_last_error.c_str());
        m_http_client.end();
    }
} // namespace pooaway::alert