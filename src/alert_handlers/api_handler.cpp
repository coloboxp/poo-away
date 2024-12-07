#include "alert_handlers/api_handler.h"
#include "esp_log.h"
#include "config.h"
#include "private.h"
#include <Arduino.h>

namespace pooaway::alert
{
    void ApiHandler::init()
    {
        ESP_LOGI(TAG, "Initializing API handler");
        if (WiFi.status() == WL_CONNECTED)
        {
            m_available = true;
            ESP_LOGI(TAG, "API handler initialized");
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

        if (WiFi.status() != WL_CONNECTED)
        {
            m_last_error = "WiFi connection lost";
            ESP_LOGE(TAG, "%s", m_last_error.c_str());
            return;
        }

        // Create the JSON payload in Adafruit IO format
        JsonDocument payload_doc;
        auto feeds = payload_doc["feeds"].to<JsonArray>();

        // Add each sensor reading and threshold as separate feeds
        auto sensors = alert_data["sensors"].as<JsonArray>();
        for (JsonObject sensor : sensors)
        {
            // Add sensor reading
            {
                auto feed = feeds.add<JsonObject>();
                switch (sensor["index"].as<int>())
                {
                case 0:
                    feed["key"] = "pooaway.pee";
                    break;
                case 1:
                    feed["key"] = "pooaway.poo";
                    break;
                case 2:
                    feed["key"] = "pooaway.pee2";
                    break;
                case 3:
                    feed["key"] = "pooaway.poo2";
                    break;
                }
                feed["value"] = sensor["readings"]["value"].as<int>();
            }

            // Add threshold value
            {
                auto feed = feeds.add<JsonObject>();
                switch (sensor["index"].as<int>())
                {
                case 0:
                    feed["key"] = "pooaway.peeth";
                    break;
                case 1:
                    feed["key"] = "pooaway.pooth";
                    break;
                case 2:
                    feed["key"] = "pooaway.peeth2";
                    break;
                case 3:
                    feed["key"] = "pooaway.pooth2";
                    break;
                }
                feed["value"] = sensor["readings"]["threshold"].as<int>();
            }
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
                // Parse and log the response
                String response = m_http_client.getString();
                ESP_LOGI(TAG, "API Response: %s", response.c_str());

                JsonDocument response_doc;
                DeserializationError error = deserializeJson(response_doc, response);

                if (error)
                {
                    ESP_LOGW(TAG, "Failed to parse API response: %s", error.c_str());
                }
                else
                {
                    ESP_LOGI(TAG, "Data posted successfully");
                }

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