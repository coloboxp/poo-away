#pragma once
#include <Arduino.h>
#include "sensors/sensor_types.h"

namespace pooaway::sensors
{
    class ISensor
    {
    public:
        virtual ~ISensor() = default;
        virtual void init() = 0;
        virtual void read() = 0;
        virtual float get_value() const = 0;
        virtual bool check_alert() const = 0;
        virtual const char* get_name() const = 0;
    };

    class ICalibration
    {
    public:
        virtual ~ICalibration() = default;
        virtual void calibrate() = 0;
        virtual float get_r0() const = 0;
        virtual bool validate_r0(float r0) const = 0;
        virtual void set_r0(float r0) = 0;
        virtual void run_self_test() = 0;
    };

    class ISensorReading
    {
    public:
        virtual ~ISensorReading() = default;
        virtual float read_raw() const = 0;
    };

    class IPowerManagement
    {
    public:
        virtual ~IPowerManagement() = default;
        virtual void enter_low_power() = 0;
        virtual void exit_low_power() = 0;
    };
}