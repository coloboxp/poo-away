#pragma once
#include "alert_handler.h"
#include <HTTPClient.h>
#include <WiFi.h>

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
        unsigned long m_last_request{0};
        unsigned long m_rate_limit_ms;
        static constexpr int MAX_RETRIES = 3;
        static constexpr int RETRY_DELAY_MS = 1000;
        static constexpr char const *TAG = "ApiHandler";
    };
} // namespace pooaway::alert
