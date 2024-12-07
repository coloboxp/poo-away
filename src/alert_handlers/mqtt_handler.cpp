#include "alert_handlers/mqtt_handler.h"
#include "esp_log.h"
#include "config.h"

namespace pooaway::alert
{

    MqttHandler::MqttHandler() : m_mqtt_client(m_wifi_client)
    {
        m_available = false;
    }

    void MqttHandler::init()
    {
        ESP_LOGI(TAG, "Initializing MQTT handler");

        // Try to connect to WiFi with timeout
        WiFi.begin(WIFI_SSID, WIFI_PASS);
        unsigned long start = millis();

        while (WiFi.status() != WL_CONNECTED)
        {
            if (millis() - start > 10000)
            { // 10 second timeout
                m_available = false;
                m_last_error = "WiFi connection timeout";
                ESP_LOGW(TAG, "%s", m_last_error.c_str());
                return;
            }
            delay(500);
            ESP_LOGI(TAG, "Connecting to WiFi...");
        }

        ESP_LOGI(TAG, "Connected to WiFi");
        m_mqtt_client.setServer(MQTT_BROKER, MQTT_PORT);
        m_available = true;
        m_last_error.clear();
    }

    void MqttHandler::handle_alert(const bool alerts[SENSOR_COUNT])
    {
        if (!m_available)
        {
            return;
        }

        if (!connect())
        {
            return;
        }

        for (int i = 0; i < SENSOR_COUNT; i++)
        {
            if (!publish_alert(i, alerts[i]))
            {
                m_available = false;
                return;
            }
        }
    }

    bool MqttHandler::connect()
    {
        if (m_mqtt_client.connected())
        {
            return true;
        }

        // Implement reconnection backoff
        unsigned long now = millis();
        if (now - m_last_reconnect_attempt < RECONNECT_INTERVAL)
        {
            return false;
        }
        m_last_reconnect_attempt = now;

        ESP_LOGI(TAG, "Connecting to MQTT broker...");

        if (m_mqtt_client.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD))
        {
            ESP_LOGI(TAG, "Connected to MQTT broker");
            return true;
        }

        m_last_error = "Failed to connect to MQTT broker";
        ESP_LOGW(TAG, "%s", m_last_error.c_str());
        return false;
    }

    bool MqttHandler::publish_alert(int sensor_index, bool alert_state)
    {
        char topic[64];
        snprintf(topic, sizeof(topic), "%s/%s/alert",
                 MQTT_FEED_PREFIX,
                 sensors[sensor_index].name);

        const char *payload = alert_state ? "1" : "0";

        if (!m_mqtt_client.publish(topic, payload))
        {
            m_last_error = "Failed to publish to topic";
            ESP_LOGW(TAG, "%s: %s", m_last_error.c_str(), topic);
            return false;
        }

        return true;
    }

} // namespace pooaway::alert