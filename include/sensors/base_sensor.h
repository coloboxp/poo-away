#pragma once
#include <Arduino.h>
#include "config.h"
#include "sensor_diagnostics.h"

namespace pooaway::sensors
{

    class BaseSensor
    {
    public:
        BaseSensor(const char *model, const char *name, int pin,
                   float alpha, float tolerance, float preheating_time,
                   int min_detect_ms, float coeff_a, float coeff_b);

        virtual ~BaseSensor() = default;

        // Common interface
        virtual void init();
        virtual void calibrate();
        virtual float read();
        virtual bool check_alert();

        // Getters and setters
        [[nodiscard]] const char *get_name() const { return m_name; }
        [[nodiscard]] bool needs_calibration() const { return m_needs_calibration; }
        [[nodiscard]] float get_value() const { return m_value; }
        [[nodiscard]] float get_r0() const { return m_r0; }
        void set_r0(float r0) { m_r0 = r0; }
        void set_alerts_enabled(bool enabled) { m_alerts_enabled = enabled; }

        // Diagnostic methods
        [[nodiscard]] const SensorDiagnostics &get_diagnostics() const { return m_diagnostics; }
        void run_self_test();
        bool is_healthy() const { return m_diagnostics.is_healthy; }
        void enter_low_power();
        void exit_low_power();

    protected:
        // Sensor-specific calculations to be overridden
        virtual float calculate_ppm(float raw_value) const = 0;
        virtual float calculate_rs(float voltage) const;
        virtual void update_baseline(float new_value);

        // Common sensor data
        const char *m_model;
        const char *m_name;
        const int m_pin;
        const float m_alpha;
        const float m_tolerance;
        const float m_preheating_time;
        const int m_min_detect_ms;
        const float m_coeff_a;
        const float m_coeff_b;

        float m_r0{1.0F};
        float m_baseline_ema{1.0F};
        float m_value{1.0F};
        bool m_first_reading{true};
        bool m_needs_calibration{true};
        unsigned long m_detect_start{0UL};

        // New protected members
        SensorDiagnostics m_diagnostics;
        bool m_low_power_mode{false};
        bool m_alerts_enabled{true};

        // Enhanced error handling
        void log_error(const char *message);
        bool validate_reading(float value) const;
        void update_diagnostics(float raw_value, float processed_value);
    };

} // namespace pooaway::sensors