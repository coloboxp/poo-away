#pragma once
#include <string>
#include <ArduinoJson.h>
#include "sensors/sensor_types.h"

namespace pooaway::alert
{
    class AlertHandler
    {
    public:
        virtual ~AlertHandler() = default;
        virtual void init() = 0;
        virtual void handle_alert(JsonDocument &alert_data) = 0;
        virtual bool is_available() const { return m_available; }
        virtual std::string get_last_error() const { return m_last_error; }

    protected:
        bool m_available{false};
        std::string m_last_error;
        static constexpr char const *TAG = "AlertHandler";
    };

} // namespace pooaway::alert