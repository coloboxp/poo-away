#pragma once
#include "sensors.h"

class DebugManager
{
public:
    static DebugManager &instance();

    // Delete copy constructor and assignment operator
    DebugManager(const DebugManager &) = delete;
    DebugManager &operator=(const DebugManager &) = delete;

    void init();
    void print_sensor_data();

private:
    DebugManager(); // Private constructor for singleton

    static constexpr char const *TAG = "DebugManager";
    unsigned long m_last_print{0};
};