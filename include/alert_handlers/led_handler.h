#pragma once
#include "alert_handler.h"

namespace pooaway::alert
{

    class LedHandler : public AlertHandler
    {
    public:
        void init() override;
        void handle_alert(JsonDocument &alert_data) override;

    private:
        bool m_led_state{false};
    };

} // namespace pooaway::alert