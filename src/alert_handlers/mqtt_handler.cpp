#include "alert_handlers/mqtt_handler.h"
#include "esp_log.h"
#include "config.h"
#include "sensors.h"
#include <Arduino.h>

namespace pooaway::alert
{

    MqttHandler::MqttHandler() : m_mqtt_client(m_wifi_client)
    {
        m_mqtt_client.setBufferSize(512);
    }

    void MqttHandler::init()
    {
        ESP_LOGI(TAG, "Initializing MQTT handler");

        // Connect to WiFi
        ESP_LOGI(TAG, "Connecting to WiFi...");
        WiFi.begin(WIFI_SSID, WIFI_PASS);

        int retries = 0;
        while (WiFi.status() != WL_CONNECTED && retries < MAX_RETRIES)
        {
            delay(RETRY_DELAY_MS);
            ESP_LOGI(TAG, ".");
            retries++;
        }

        if (WiFi.status() != WL_CONNECTED)
        {
            m_last_error = "Failed to connect to WiFi";
            ESP_LOGE(TAG, "%s", m_last_error.c_str());
            return;
        }

        ESP_LOGI(TAG, "WiFi connected");

        // Configure MQTT
        m_mqtt_client.setServer(MQTT_BROKER, MQTT_PORT);

        if (connect())
        {
            m_available = true;
            ESP_LOGI(TAG, "MQTT handler initialized");
        }
    }

    void MqttHandler::handle_alert(const bool alerts[pooaway::sensors::SENSOR_COUNT])
    {
        if (!m_available)
            return;

        const unsigned long now = millis();
        if (now - m_last_alert < ALERT_INTERVAL)
        {
            return;
        }

        if (!m_mqtt_client.connected() && !connect())
        {
            return;
        }

        for (size_t i = 0; i < pooaway::sensors::SENSOR_COUNT; i++)
        {
            if (!publish_alert(i, alerts[i]))
            {
                ESP_LOGW(TAG, "Failed to publish alert for sensor %d", i);
            }
        }

        m_last_alert = now;
        m_mqtt_client.loop();
    }

    bool MqttHandler::connect()
    {
        ESP_LOGI(TAG, "Connecting to MQTT broker...");

        int retries = 0;
        while (!m_mqtt_client.connected() && retries < MAX_RETRIES)
        {
            if (m_mqtt_client.connect("PooAway"))
            {
                ESP_LOGI(TAG, "Connected to MQTT broker");
                return true;
            }

            ESP_LOGW(TAG, "Failed to connect to MQTT broker, retrying...");
            delay(RETRY_DELAY_MS);
            retries++;
        }

        m_last_error = "Failed to connect to MQTT broker";
        ESP_LOGE(TAG, "%s", m_last_error.c_str());
        return false;
    }

    bool MqttHandler::publish_alert(int sensor_index, bool alert_state)
    {
        char topic[64];
        char payload[128];

        snprintf(topic, sizeof(topic), "pooaway/alerts/%d", sensor_index);
        snprintf(payload, sizeof(payload), "{\"sensor\":\"%s\",\"alert\":%s}",
                 ::sensors[sensor_index].name,
                 alert_state ? "true" : "false");

        if (!m_mqtt_client.publish(topic, payload))
        {
            m_last_error = "Failed to publish MQTT message";
            ESP_LOGE(TAG, "%s", m_last_error.c_str());
            return false;
        }

        return true;
    }

} // namespace pooaway::alert