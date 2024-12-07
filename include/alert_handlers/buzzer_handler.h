#pragma once
#include "alert_handler.h"

namespace pooaway::alert
{

    class BuzzerHandler : public AlertHandler
    {
    public:
        void init() override;
        void handle_alert(const bool alerts[pooaway::sensors::SENSOR_COUNT]) override;

    private:
        void play_tone(int frequency_hz, int duration_ms);
        unsigned long m_last_alert{0};
    };

} // namespace pooaway::alert
