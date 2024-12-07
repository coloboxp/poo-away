#pragma once
#include "sensors.h"

class AlertHandler
{
public:
    virtual ~AlertHandler() = default;
    virtual void handle_alert(const bool alerts[SENSOR_COUNT]) = 0;
    virtual void init() = 0;
};