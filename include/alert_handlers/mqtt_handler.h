#pragma once
#include "alert_handler.h"
#include <PubSubClient.h>
#include <WiFi.h>

class MqttHandler : public AlertHandler
{
public:
    MqttHandler();
    ~MqttHandler() override = default;

    void init() override;
    void handle_alert(const bool alerts[SENSOR_COUNT]) override;

private:
    static constexpr char const *TAG = "MqttHandler";
    WiFiClient m_wifi_client;
    PubSubClient m_mqtt_client;
    bool connect();
    void publish_alert(int sensor_index, bool alert_state);
};