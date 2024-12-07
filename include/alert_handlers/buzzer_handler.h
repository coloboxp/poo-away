#pragma once
#include "alert_handler.h"

namespace pooaway::alert
{

    class BuzzerHandler : public AlertHandler
    {
    public:
        explicit BuzzerHandler(unsigned long rate_limit_ms = 0);
        void init() override;
        void handle_alert(JsonDocument &alert_data) override;

    private:
        void play_tone(int frequency_hz, int duration_ms);
        unsigned long m_last_request{0};
        unsigned long m_rate_limit_ms;
        static constexpr char const *TAG = "BuzzerHandler";
    };

} // namespace pooaway::alert
