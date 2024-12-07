#pragma once
#include "sensors/interfaces.h"
#include "esp_log.h"
#include "config.h"

namespace pooaway::sensors
{
    // Forward declaration
    class CalibrationService;

    class BaseSensor : public ICalibration,
                       public ISensor,
                       public ISensorReading,
                       public IPowerManagement
    {
        friend class CalibrationService;

    protected:
        static constexpr float VCC = 3.3F;
        static constexpr int ADC_RESOLUTION = 4095;

        const char *m_model;
        const char *m_name;
        const int m_pin;
        const float m_alpha;
        const float m_tolerance;
        const float m_preheating_time;
        const int m_min_detect_ms;
        const float m_coeff_a;
        const float m_coeff_b;

        float m_value{0.0F};
        float m_r0{0.0F};
        bool m_needs_calibration{true};
        bool m_alerts_enabled{false};
        bool m_first_reading{true};
        float m_baseline_ema{0.0F};
        mutable unsigned long m_detect_start{0UL};
        bool m_low_power_mode{false};

        virtual bool validate_reading(float raw_value) const = 0;
        virtual float calculate_ppm(float raw_value) const = 0;
        virtual bool is_valid_ppm(float ppm) const = 0;
        virtual float calculate_rs(float voltage) const = 0;

    public:
        BaseSensor(const char *model, const char *name, int pin,
                   float alpha, float tolerance, float preheating_time,
                   int min_detect_ms, float coeff_a, float coeff_b);

        virtual ~BaseSensor() = default;

        // ISensor interface
        void init() override;
        void read() override;
        float get_value() const override { return m_value; }
        bool check_alert() const override;
        const char *get_name() const override { return m_name; }

        // Sensor reading getters
        virtual float get_voltage() const { return 0.0f; } // Default implementation
        virtual float get_rs() const { return 0.0f; }      // Default implementation
        float get_r0() const { return m_r0; }              // Concrete implementation

        // ICalibration interface
        void calibrate() override;
        void set_r0(float r0) override { m_r0 = r0; }
        bool validate_r0(float r0) const override = 0;
        void run_self_test() override;

        // ISensorReading interface
        float read_raw() const override;

        // IPowerManagement interface
        void enter_low_power() override;
        void exit_low_power() override;

        bool needs_calibration() const { return m_needs_calibration; }
    };
}