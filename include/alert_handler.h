#pragma once
#include <string>
#include <ArduinoJson.h>
#include "sensors/sensor_types.h"
#include "wifi_manager.h"

namespace pooaway::alert
{
    enum class HandlerType
    {
        ALERT_ONLY,    // For handlers that should only act on alerts (buzzer)
        DATA_PUBLISHER // For handlers that should always publish data (MQTT, API)
    };

    class AlertHandler
    {
    public:
        virtual ~AlertHandler() = default;
        virtual void init() = 0;
        virtual void handle_alert(JsonDocument &alert_data) = 0;
        virtual bool is_available() const { return m_available; }
        virtual std::string get_last_error() const { return m_last_error; }
        HandlerType get_type() const { return m_type; }

    protected:
        bool m_available{false};
        std::string m_last_error;
        HandlerType m_type{HandlerType::DATA_PUBLISHER}; // Default to data publisher
        static constexpr char const *TAG = "AlertHandler";
    };

} // namespace pooaway::alert