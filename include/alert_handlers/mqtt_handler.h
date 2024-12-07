#pragma once
#include "alert_handler.h"
#include "sensors.h"
#include <WiFi.h>
#include <PubSubClient.h>

namespace pooaway::alert
{

    class MqttHandler : public AlertHandler
    {
    public:
        MqttHandler();
        void init() override;
        void handle_alert(JsonDocument &alert_data) override;

    private:
        bool connect();
        bool publish_alert(int sensor_index, bool alert_state);

        WiFiClient m_wifi_client;
        PubSubClient m_mqtt_client;
        bool m_mqtt_connected{false};
        unsigned long m_last_alert{0};
        static constexpr int MAX_RETRIES = 5;
        static constexpr int RETRY_DELAY_MS = 1000;
    };

} // namespace pooaway::alert
