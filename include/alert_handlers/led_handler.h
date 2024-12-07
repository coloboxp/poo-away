#pragma once
#include "alert_handler.h"

namespace pooaway::alert
{

    class LedHandler : public AlertHandler
    {
    public:
        explicit LedHandler(unsigned long rate_limit_ms = 0);
        void init() override;
        void handle_alert(JsonDocument &alert_data) override;

    private:
        bool m_led_state{false};
        unsigned long m_last_request{0};
        unsigned long m_rate_limit_ms;
        static constexpr char const *TAG = "LedHandler";
    };

} // namespace pooaway::alert