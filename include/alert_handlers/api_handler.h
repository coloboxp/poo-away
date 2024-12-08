#pragma once
#include "alert_handler.h"
#include <HTTPClient.h>
#include <WiFi.h>
#include <string>
#include <map>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>

namespace pooaway::alert
{
    class ApiHandler : public AlertHandler
    {
    public:
        explicit ApiHandler(unsigned long rate_limit_ms = 0);
        void init() override;
        void handle_alert(JsonDocument &alert_data) override;

    private:
        HTTPClient m_http_client;
        WiFiClientSecure m_secure_client;
        unsigned long m_last_request{0};
        unsigned long m_rate_limit_ms{0};
        static constexpr int MAX_RETRIES = 3;
        static constexpr int RETRY_DELAY_MS = 1000;
        static constexpr char const *TAG = "ApiHandler";

        struct ChannelInfo
        {
            String channel_id;
            String write_api_key;
            String read_api_key;
        };
        std::map<String, ChannelInfo> m_channel_info{};
        bool ensure_channel_exists(const char *name);
        bool store_channel_info(const char *name, JsonDocument &response);
        void send_sensor_data(JsonObjectConst sensor);
    };
} // namespace pooaway::alert
