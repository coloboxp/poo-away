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
        bool all_success = true;
        for (const char *sensor : sensors)
        {
            try
            {
                if (!ensure_channel_exists(sensor))
                {
                    ESP_LOGE(TAG, "Failed to initialize channel for %s: %s",
                             sensor, m_last_error.c_str());
                    all_success = false;
                    continue;
                }
            }
            catch (const std::exception &e)
            {
                ESP_LOGE(TAG, "Exception initializing channel for %s: %s",
                         sensor, e.what());
                all_success = false;
                continue;
            }
        }

        // Only mark as available if all channels were initialized successfully
        m_available = all_success;
        ESP_LOGI(TAG, "API handler initialization %s", m_available ? "successful" : "failed");
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
            if (sensor_name == "nh3" || sensor_name == "pee")
            {
                info.write_api_key = config::thingspeak::NH3_API_KEY;
                m_channel_info["pee"] = info; // Store both mappings
                m_channel_info["nh3"] = info;
            }
            else if (sensor_name == "ch4" || sensor_name == "poo")
            {
                info.write_api_key = config::thingspeak::CH4_API_KEY;
                m_channel_info["poo"] = info; // Store both mappings
                m_channel_info["ch4"] = info;
            }
            else
            {
                m_last_error = std::string("Unknown sensor type: ") + name;
                ESP_LOGE(TAG, "%s", m_last_error.c_str());
                return false;
            }

            info.read_api_key = config::thingspeak::USER_API_KEY;

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
            m_last_error = std::string("Channel info not found for ") + std::string(name);
            ESP_LOGE(TAG, "%s", m_last_error.c_str());
            return false;
        }

        ESP_LOGI(TAG, "Channel info already configured for %s", name);
        return true;
    }

    void ApiHandler::handle_alert(JsonDocument &alert_data)
    {
        ESP_LOGV(TAG, "handle_alert called for DATA_PUBLISHER");

        if (!m_available)
        {
            ESP_LOGW(TAG, "Handler not available, skipping");
            return;
        }

        // Check rate limiting
        if (m_rate_limit_ms > 0)
        {
            unsigned long now = millis();
            if (now - m_last_request < m_rate_limit_ms)
            {
                ESP_LOGD(TAG, "Rate limited, skipping request. Time since last: %lu ms",
                         now - m_last_request);
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
        ESP_LOGV(TAG, "Processing %u sensors for data publishing", sensors.size());

        // Process all sensors regardless of alert status
        for (JsonObjectConst sensor : sensors)
        {
            ESP_LOGV(TAG, "Publishing data for sensor %s", sensor["name"].as<const char *>());
            send_sensor_data(sensor);

            // ThingSpeak requires 15 seconds between updates
            delay(15000);
        }
    }

    void ApiHandler::send_sensor_data(JsonObjectConst sensor)
    {
        String sensor_name(sensor["name"].as<const char *>());
        sensor_name.toLowerCase();

        if (m_channel_info.find(sensor_name) == m_channel_info.end())
        {
            ESP_LOGE(TAG, "No channel info found for %s", sensor_name.c_str());
            return;
        }

        const auto &channel_info = m_channel_info[sensor_name];

        ESP_LOGV(TAG, "Sending data for sensor %s with API key %s",
                 sensor_name.c_str(),
                 channel_info.write_api_key.c_str());

        // Create bulk update payload following ThingSpeak format
        JsonDocument payload_doc;
        payload_doc["write_api_key"] = channel_info.write_api_key;
        JsonArray updates = payload_doc["updates"].to<JsonArray>();

        JsonObject update = updates.add<JsonObject>();

        // Get current timestamp
        struct tm timeinfo;
        if (!getLocalTime(&timeinfo))
        {
            ESP_LOGE(TAG, "Failed to obtain time");
            return;
        }

        char timestamp[30];
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &timeinfo); // Remove timezone
        update["created_at"] = timestamp;

        // Add all fields
        update["field1"] = sensor["readings"]["value"].as<float>();
        update["field2"] = sensor["readings"]["baseline"].as<float>();
        update["field3"] = sensor["readings"]["voltage"].as<float>();
        update["field4"] = sensor["readings"]["rs"].as<float>();
        update["field5"] = sensor["readings"]["r0"].as<float>();
        update["field6"] = sensor["readings"]["ratio"].as<float>();
        update["field7"] = sensor["alert"].as<bool>();
        update["field8"] = sensor["calibration"]["preheating_time"].as<int>();

        String payload;
        serializeJson(payload_doc, payload);
        ESP_LOGV(TAG, "Sending payload: %s", payload.c_str());

        // Use ThingSpeak's bulk update endpoint with channel ID
        String url = "https://api.thingspeak.com/channels/";
        url += (sensor_name == "pee" || sensor_name == "nh3") ? config::thingspeak::NH3_CHANNEL_ID : config::thingspeak::CH4_CHANNEL_ID;
        url += "/bulk_update.json";

        // Ensure we end any previous connection
        m_http_client.end();
        
        // Begin new connection
        if (!m_http_client.begin(url)) {
            ESP_LOGE(TAG, "Failed to begin HTTP client");
            return;
        }

        m_http_client.addHeader("Content-Type", "application/json");
        m_http_client.setTimeout(config::api::TIMEOUT_MS);

        int httpCode = m_http_client.POST(payload);

        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_ACCEPTED)
        {
            String response = m_http_client.getString();
            ESP_LOGV(TAG, "ThingSpeak Response: %s", response.c_str());
        }
        else
        {
            ESP_LOGE(TAG, "HTTP POST failed, code: %d", httpCode);
        }

        // Always end the connection
        m_http_client.end();
        delay(100); // Give some time for socket cleanup
    }
} // namespace pooaway::alert