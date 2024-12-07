#pragma once
#include "base_sensor.h"

namespace pooaway::sensors
{
    class NH3Sensor : public BaseSensor
    {
    private:
        static constexpr float MIN_VALID_VOLTAGE = 0.0F;
        static constexpr float MAX_VALID_VOLTAGE = 4.0F;
        static constexpr float MIN_VALID_PPM = 0.0F;
        static constexpr float MAX_VALID_PPM = 500.0F;
        static constexpr float RL = 47000.0F;
        static constexpr float MIN_VALID_R0 = 1000.0F;
        static constexpr float MAX_VALID_R0 = 100000.0F;

    protected:
        bool validate_reading(float raw_value) const override;
        float calculate_ppm(float raw_value) const override;
        bool is_valid_ppm(float ppm) const override;
        float calculate_rs(float voltage) const override;
        bool validate_r0(float r0) const override;
        void set_r0(float r0) override;

    public:
        NH3Sensor(int pin)
            : BaseSensor("GM-802B", "PEE", pin,
                         0.1F,    // alpha (slower response)
                         0.3F,    // tolerance
                         30.0F,   // preheating time (30s)
                         5000,    // min detect ms
                         102.2F,  // coeff a (calibrated for NH3 curve)
                         -2.473F) // coeff b (calibrated for NH3 curve)
        {
        }
    };
} // namespace pooaway::sensors