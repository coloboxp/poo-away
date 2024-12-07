#pragma once
#include "sensors.h"
#include "error_handler.h"

namespace pooaway::alert
{

    class AlertHandler
    {
    public:
        virtual ~AlertHandler() = default;
        virtual void init() = 0;
        virtual void handle_alert(const bool alerts[SENSOR_COUNT]) = 0;
        [[nodiscard]] bool is_available() const { return m_available; }
        [[nodiscard]] const std::string &get_last_error() const { return m_last_error; }

    protected:
        bool m_available{false};
        std::string m_last_error;
    };

} // namespace pooaway::alert