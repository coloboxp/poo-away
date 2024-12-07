#include "alert_handlers/mqtt_handler.h"
#include "esp_log.h"
#include "config.h"

MqttHandler::MqttHandler() : m_mqtt_client(m_wifi_client)
{
    // Initialize MQTT client with WiFi client in constructor
}

void MqttHandler::init()
{
    ESP_LOGI(TAG, "Initializing MQTT handler");

    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        ESP_LOGI(TAG, "Connecting to WiFi...");
    }
    ESP_LOGI(TAG, "Connected to WiFi");

    m_mqtt_client.setServer(MQTT_BROKER, MQTT_PORT);
}

void MqttHandler::handle_alert(const bool alerts[SENSOR_COUNT])
{
    if (!connect())
    {
        return;
    }

    for (int i = 0; i < SENSOR_COUNT; i++)
    {
        publish_alert(i, alerts[i]);
    }
}

bool MqttHandler::connect()
{
    if (m_mqtt_client.connected())
    {
        return true;
    }

    ESP_LOGI(TAG, "Connecting to MQTT broker...");

    if (m_mqtt_client.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD))
    {
        ESP_LOGI(TAG, "Connected to MQTT broker");
        return true;
    }

    ESP_LOGE(TAG, "Failed to connect to MQTT broker, rc=%d", m_mqtt_client.state());
    return false;
}

void MqttHandler::publish_alert(int sensor_index, bool alert_state)
{
    char topic[64];
    snprintf(topic, sizeof(topic), "%s/%s/alert",
             MQTT_FEED_PREFIX,
             sensors[sensor_index].name);

    const char *payload = alert_state ? "1" : "0";

    if (!m_mqtt_client.publish(topic, payload))
    {
        ESP_LOGE(TAG, "Failed to publish to %s", topic);
    }
}