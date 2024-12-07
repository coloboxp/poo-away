#pragma once
#include <vector>
#include "sensors.h"
#include "alert_handler.h"

namespace pooaway::alert
{

    class AlertManager
    {
    public:
        static AlertManager &instance();

        AlertManager(const AlertManager &) = delete;
        AlertManager &operator=(const AlertManager &) = delete;

        void init();
        void update(const bool alerts[SENSOR_COUNT]);
        void add_handler(AlertHandler *handler);
        void remove_handler(AlertHandler *handler);
        [[nodiscard]] std::vector<std::string> get_handler_errors() const;

    private:
        AlertManager();
        static constexpr char const *TAG = "AlertManager";
        unsigned long m_last_alert{0};
        std::vector<AlertHandler *> m_handlers;
    };

} // namespace pooaway::alert
