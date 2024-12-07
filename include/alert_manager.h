#pragma once
#include "sensors.h"

class AlertManager
{
public:
    static AlertManager &instance();

    // Delete copy constructor and assignment operator
    AlertManager(const AlertManager &) = delete;
    AlertManager &operator=(const AlertManager &) = delete;

    void init();
    void update(const bool alerts[SENSOR_COUNT]);
    void play_tone(int frequency_hz, int duration_ms);

private:
    AlertManager(); // Private constructor for singleton

    static constexpr char const *TAG = "AlertManager";
    unsigned long m_last_alert{0};
};