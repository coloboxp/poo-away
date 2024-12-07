#pragma once
#include "alert_handler.h"

namespace pooaway::alert
{

    class LedHandler : public AlertHandler
    {
    public:
        void init() override;
        void handle_alert(const bool alerts[SENSOR_COUNT]) override;

    private:
        static constexpr char const *TAG = "LedHandler";
        bool m_led_state{false};
    };

} // namespace pooaway::alert