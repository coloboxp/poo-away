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

        m_mqtt_client.setServer(MQTT_BROKER, MQTT_PORT);

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

        // Create payload document
        JsonDocument payload_doc;

        // Copy sensor data from alert_data
        auto sensors = alert_data["sensors"].as<JsonArray>();
        for (JsonObject sensor : sensors)
        {
            const int sensor_index = sensor["index"].as<int>();
            String topic = String(MQTT_FEED_PREFIX) + "/sensors/" + String(sensor_index);

            // Create sensor payload
            JsonObject sensor_payload = payload_doc.to<JsonObject>();
            sensor_payload["sensor"] = sensor["name"];
            sensor_payload["alert"] = sensor["alert"];
            sensor_payload["value"] = sensor["readings"]["value"];
            sensor_payload["baseline"] = sensor["readings"]["baseline"];

            // Serialize and publish
            String payload;
            serializeJson(sensor_payload, payload);
            m_mqtt_client.publish(topic.c_str(), payload.c_str());
            ESP_LOGI(TAG, "Published to %s: %s", topic.c_str(), payload.c_str());
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