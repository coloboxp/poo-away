#include "alert_handlers/mqtt_handler.h"
#include "wifi_manager.h"
#include "esp_log.h"
#include "config.h"
#include <Arduino.h>

namespace pooaway::alert
{

    MqttHandler::MqttHandler(unsigned long rate_limit_ms)
        : m_rate_limit_ms(rate_limit_ms), m_mqtt_client(m_wifi_client)
    {
        m_type = HandlerType::DATA_PUBLISHER; // MQTT publishes all data
        m_mqtt_client.setBufferSize(512);
    }

    void MqttHandler::init()
    {
        ESP_LOGI(TAG, "Initializing MQTT handler");

        if (!WiFiManager::instance().ensure_connected())
        {
            m_last_error = "WiFi not connected";
            ESP_LOGE(TAG, "%s", m_last_error.c_str());
            return;
        }

        m_mqtt_client.setServer(config::mqtt::BROKER, config::mqtt::PORT);

        if (connect())
        {
            m_available = true;
            ESP_LOGI(TAG, "MQTT handler initialized");
            if (m_rate_limit_ms > 0)
            {
                ESP_LOGI(TAG, "Rate limiting enabled: %lu ms", m_rate_limit_ms);
            }
        }
    }

    void MqttHandler::handle_alert(JsonDocument &alert_data)
    {
        if (!m_available)
            return;

        // Check rate limiting
        if (m_rate_limit_ms > 0)
        {
            unsigned long now = millis();
            if (now - m_last_request < m_rate_limit_ms)
            {
                ESP_LOGD(TAG, "Rate limited, skipping publish");
                return;
            }
            m_last_request = now;
        }

        // First ensure WiFi is connected
        if (!WiFiManager::instance().ensure_connected())
        {
            m_last_error = "WiFi connection lost";
            ESP_LOGE(TAG, "%s", m_last_error.c_str());
            return;
        }

        // Then check MQTT connection
        if (!m_mqtt_client.connected() && !connect())
        {
            return;
        }

        // Process all sensors regardless of alert status
        auto sensors = alert_data["sensors"].as<JsonArray>();
        for (JsonObject sensor : sensors)
        {
            String sensor_name = sensor["name"].as<const char *>();
            sensor_name.toLowerCase(); // Convert to lowercase for consistent naming

            String topic = String(MQTT_FEED_PREFIX) + "/sensors/" + sensor_name;

            // Create a new payload document for each sensor
            JsonDocument payload_doc;

            // Basic info
            payload_doc["sensor"] = sensor["name"].as<const char *>();
            payload_doc["model"] = sensor["model"].as<const char *>();

            // Readings
            const auto readings = sensor["readings"].as<JsonObject>();
            payload_doc["ppm"] = readings["value"].as<float>();
            payload_doc["baseline_ppm"] = readings["baseline"].as<float>();
            payload_doc["voltage"] = readings["voltage"].as<float>();
            payload_doc["rs"] = readings["rs"].as<float>();
            payload_doc["r0"] = readings["r0"].as<float>();
            payload_doc["ratio"] = readings["ratio"].as<float>();
            payload_doc["alert"] = sensor["alert"].as<bool>();

            // Calibration data
            const auto cal = sensor["calibration"].as<JsonObject>();
            payload_doc["preheating_time"] = cal["preheating_time"].as<int>();
            payload_doc["cal_a"] = cal["a"].as<float>();
            payload_doc["cal_b"] = cal["b"].as<float>();

            // Use buffer for more efficient publishing
            char buffer[1024];
            size_t n = serializeJson(payload_doc, buffer);

            if (m_mqtt_client.publish(topic.c_str(), buffer, n))
            {
                ESP_LOGI(TAG, "Published to %s: %s", topic.c_str(), buffer);
            }
            else
            {
                ESP_LOGE(TAG, "Failed to publish to %s", topic.c_str());
            }
        }

        m_mqtt_client.loop();
    }

    bool MqttHandler::connect()
    {
        int retries = 0;
        while (!m_mqtt_client.connected() && retries < MAX_RETRIES)
        {
            ESP_LOGI(TAG, "Attempting MQTT connection...");

            if (m_mqtt_client.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD))
            {
                ESP_LOGI(TAG, "Connected to MQTT broker");
                return true;
            }

            ESP_LOGW(TAG, "Failed to connect to MQTT, rc=%d", m_mqtt_client.state());
            delay(RETRY_DELAY_MS);
            retries++;
        }

        m_last_error = "Failed to connect to MQTT broker";
        ESP_LOGE(TAG, "%s", m_last_error.c_str());
        return false;
    }

} // namespace pooaway::alert