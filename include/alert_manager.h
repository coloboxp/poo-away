#pragma once
#include <vector>
#include "sensors.h"
#include "alert_handler.h"

class AlertManager
{
public:
    static AlertManager &instance();

    // Delete copy constructor and assignment operator
    AlertManager(const AlertManager &) = delete;
    AlertManager &operator=(const AlertManager &) = delete;

    void init();
    void update(const bool alerts[SENSOR_COUNT]);
    void add_handler(AlertHandler *handler);
    void remove_handler(AlertHandler *handler);

private:
    AlertManager(); // Private constructor for singleton

    static constexpr char const *TAG = "AlertManager";
    unsigned long m_last_alert{0};
    std::vector<AlertHandler *> m_handlers;
};