#pragma once
#include "alert_handler.h"

namespace pooaway::alert
{

    class BuzzerHandler : public AlertHandler
    {
    public:
        void init() override;
        void handle_alert(const bool alerts[SENSOR_COUNT]) override;
        void play_tone(int frequency_hz, int duration_ms);

    private:
        static constexpr char const *TAG = "BuzzerHandler";
        unsigned long m_last_alert{0};
    };

} // namespace pooaway::alert
