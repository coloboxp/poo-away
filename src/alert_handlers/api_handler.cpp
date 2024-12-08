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

        // Initialize HTTP client with SSL

        // Initialize sensors one by one with proper error handling
        const char *sensors[] = {"NH3", "CH4"};
        for (const char *sensor : sensors)
        {
            try
            {
                if (!ensure_channel_exists(sensor))
                {
                    ESP_LOGE(TAG, "Failed to initialize channel for %s: %s",
                             sensor, m_last_error.c_str());
                    continue;
                }
            }
            catch (const std::exception &e)
            {
                ESP_LOGE(TAG, "Exception initializing channel for %s: %s",
                         sensor, e.what());
                continue;
            }
        }
    }

    bool ApiHandler::ensure_channel_exists(const char *name)
    {
        if (!name)
        {
            m_last_error = "Invalid sensor name (null)";
            ESP_LOGE(TAG, "%s", m_last_error.c_str());
            return false;
        }

        ESP_LOGI(TAG, "Verifying/Creating ThingSpeak channel for %s", name);

        // Convert to lowercase for consistent comparison
        String sensor_name(name);
        sensor_name.toLowerCase();

        try
        {
            // Set the write API key based on sensor name
            ChannelInfo info;
            if (sensor_name == "nh3")
            {
                info.write_api_key = config::thingspeak::NH3_API_KEY;
            }
            else if (sensor_name == "ch4")
            {
                info.write_api_key = config::thingspeak::CH4_API_KEY;
            }
            else
            {
                m_last_error = std::string("Unknown sensor type: ") + name;
                ESP_LOGE(TAG, "%s", m_last_error.c_str());
                return false;
            }

            info.read_api_key = config::thingspeak::USER_API_KEY;
            m_channel_info[sensor_name] = info;

            ESP_LOGI(TAG, "Successfully configured channel for %s", name);
            return true;
        }
        catch (const std::exception &e)
        {
            m_last_error = std::string("Exception in ensure_channel_exists: ") + e.what();
            ESP_LOGE(TAG, "%s", m_last_error.c_str());
            return false;
        }
    }

    bool ApiHandler::store_channel_info(const char *name, JsonDocument &response)
    {
        String sensor_name(name);
        sensor_name.toLowerCase();

        // The channel info should already be stored by ensure_channel_exists
        if (m_channel_info.find(sensor_name) == m_channel_info.end())
        {
            m_last_error = std::string("Channel info not found for ") + name;
            ESP_LOGE(TAG, "%s", m_last_error.c_str());
            return false;
        }

        ESP_LOGI(TAG, "Channel info already configured for %s", name);
        return true;
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

        JsonArrayConst sensors = alert_data["sensors"].as<JsonArrayConst>();

        // Debug log the input data
        ESP_LOGD(TAG, "Processing sensor data:");
        for (JsonObjectConst sensor : sensors)
        {
            ESP_LOGD(TAG, "Sensor %s: value=%.2f, baseline=%.2f, alert=%d",
                     sensor["name"].as<const char *>(),
                     sensor["readings"]["value"].as<float>(),
                     sensor["readings"]["baseline"].as<float>(),
                     sensor["alert"].as<bool>());

            send_sensor_data(sensor);

            // ThingSpeak requires 15 seconds between updates
            delay(15000);
        }
    }

    void ApiHandler::send_sensor_data(JsonObjectConst sensor)
    {
        String sensor_name(sensor["name"].as<const char *>());
        sensor_name.toLowerCase();

        // Get channel info
        if (m_channel_info.find(sensor_name) == m_channel_info.end())
        {
            ESP_LOGE(TAG, "No channel info found for %s", sensor_name.c_str());
            return;
        }

        const auto &channel_info = m_channel_info[sensor_name];

        // Create bulk update payload
        JsonDocument payload_doc;
        payload_doc["write_api_key"] = channel_info.write_api_key;
        JsonArray updates = payload_doc["updates"].to<JsonArray>();

        // Get current timestamp
        struct tm timeinfo;
        if (!getLocalTime(&timeinfo))
        {
            ESP_LOGE(TAG, "Failed to get local time");
            return;
        }

        char time_str[32];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S %z", &timeinfo);

        // Create the update entry
        JsonObject update = updates.add<JsonObject>();
        update["created_at"] = time_str;

        // Map sensor data to ThingSpeak fields
        JsonObjectConst readings = sensor["readings"].as<JsonObjectConst>();
        update["field1"] = readings["value"].as<float>();
        update["field2"] = readings["baseline"].as<float>();
        update["field3"] = readings["voltage"].as<float>();
        update["field4"] = readings["rs"].as<float>();
        update["field5"] = readings["r0"].as<float>();
        update["field6"] = readings["ratio"].as<float>();
        update["field7"] = sensor["alert"].as<bool>();
        update["field8"] = sensor["calibration"]["preheating_time"].as<int>();

        // Add status field
        update["status"] = sensor["alert"].as<bool>() ? "alert" : "normal";

        String payload;
        serializeJson(payload_doc, payload);
        ESP_LOGI(TAG, "Sending %s sensor data: %s", sensor_name.c_str(), payload.c_str());

        // Use stored channel ID for URL
        String url = String("https://api.thingspeak.com/channels/") +
                     channel_info.channel_id + "/bulk_update.json";

        m_http_client.begin(url);
        m_http_client.addHeader("Content-Type", "application/json");
        m_http_client.setTimeout(config::api::TIMEOUT_MS);

        int retries = 0;
        int httpCode;

        do
        {
            httpCode = m_http_client.POST(payload);

            if (httpCode == HTTP_CODE_OK)
            {
                String response = m_http_client.getString();
                ESP_LOGI(TAG, "ThingSpeak Response for %s: %s",
                         sensor_name.c_str(), response.c_str());
                m_http_client.end();
                return;
            }

            ESP_LOGW(TAG, "HTTP POST failed for %s, code: %d, retrying...",
                     sensor_name.c_str(), httpCode);
            delay(RETRY_DELAY_MS);
            retries++;
        } while (retries < MAX_RETRIES);

        m_last_error = std::string("Failed to send ") + sensor_name.c_str() +
                       " data: " + std::to_string(httpCode);
        ESP_LOGE(TAG, "%s", m_last_error.c_str());
        m_http_client.end();
    }
} // namespace pooaway::alert