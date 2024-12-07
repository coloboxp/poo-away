#pragma once
#include <Arduino.h>
#include "esp_log.h"
#include "config.h"
#include "sensor_types.h"

namespace pooaway::sensors
{
    struct SensorDiagnostics
    {
        bool is_healthy{true};
        unsigned long error_count{0};
        unsigned long read_count{0};
        unsigned long calibration_count{0};
        float last_voltage{0.0F};
        float last_resistance{0.0F};
        unsigned long last_read_time{0};
        float min_value{99999.0F};
        float max_value{0.0F};
        float avg_value{0.0F};
        unsigned long total_active_time{0};
    };

    class BaseSensor
    {
    protected:
        const char *m_model;
        const char *m_name;
        const int m_pin;
        const float m_alpha;           // EMA filter coefficient
        const float m_tolerance;       // Alert threshold
        const float m_preheating_time; // Seconds
        const int m_min_detect_ms;     // Minimum detection time
        const float m_coeff_a;         // PPM calculation coefficient a
        const float m_coeff_b;         // PPM calculation coefficient b

        float m_r0{1.0F};           // Base resistance in clean air
        float m_value{0.0F};        // Current value
        float m_baseline_ema{1.0F}; // Baseline for comparison
        bool m_first_reading{true};
        bool m_needs_calibration{true};
        bool m_low_power_mode{false};
        bool m_alerts_enabled{false};
        mutable unsigned long m_detect_start{0};

        SensorDiagnostics m_diagnostics;

        // Pure virtual methods for sensor-specific implementations
        virtual bool validate_reading(float raw_value) const = 0;
        virtual float calculate_ppm(float raw_value) const = 0;
        virtual bool is_valid_ppm(float ppm) const = 0;
        virtual float calculate_rs(float voltage) const = 0;

        void log_error(const char *message);
        void update_diagnostics(float raw_value, float processed_value);
        void update_baseline(float new_value);
        float read_raw();

    public:
        BaseSensor(const char *model, const char *name, int pin,
                   float alpha, float tolerance, float preheating_time,
                   int min_detect_ms, float coeff_a, float coeff_b);
        virtual ~BaseSensor() = default;

        virtual void init();
        virtual void calibrate();
        virtual float read();
        virtual bool check_alert() const;
        virtual void run_self_test();

        // Power management
        virtual void enter_low_power();
        virtual void exit_low_power();

        // Getters
        const char *get_name() const { return m_name; }
        const char *get_model() const { return m_model; }
        float get_value() const { return m_value; }
        float get_baseline() const { return m_baseline_ema; }
        float get_r0() const { return m_r0; }
        bool needs_calibration() const { return m_needs_calibration; }
        const SensorDiagnostics &get_diagnostics() const { return m_diagnostics; }
        bool alerts_enabled() const { return m_alerts_enabled; }
        void set_alerts_enabled(bool enabled) { m_alerts_enabled = enabled; }
    };

} // namespace pooaway::sensors