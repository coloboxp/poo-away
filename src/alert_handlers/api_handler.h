#pragma once
#include "alert_handler.h"
#include <HTTPClient.h>

namespace pooaway::alert
{
    class ApiHandler : public AlertHandler
    {
    public:
        void init() override;
        void handle_alert(const JsonDocument &alert_data) override;

    private:
        HTTPClient m_http_client;
        unsigned long m_last_alert{0};
        static constexpr int MAX_RETRIES = 3;
        static constexpr int RETRY_DELAY_MS = 1000;
    };
} // namespace pooaway::alert