#pragma once
#include "alert_handler.h"
#include <WiFiClient.h>
#include <PubSubClient.h>

namespace pooaway::alert
{

    class MqttHandler : public AlertHandler
    {
    public:
        explicit MqttHandler(unsigned long rate_limit_ms = 0);
        void init() override;
        void handle_alert(JsonDocument &alert_data) override;

    private:
        bool connect();

        WiFiClient m_wifi_client;
        PubSubClient m_mqtt_client;
        unsigned long m_last_request{0};
        unsigned long m_rate_limit_ms;
        static constexpr int MAX_RETRIES = 3;
        static constexpr int RETRY_DELAY_MS = 1000;
        static constexpr char const *TAG = "MqttHandler";
    };

} // namespace pooaway::alert
