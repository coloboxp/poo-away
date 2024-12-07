#pragma once
#include "base_sensor.h"

namespace pooaway::sensors
{

    class NH3Sensor : public BaseSensor
    {
    public:
        explicit NH3Sensor(int pin)
            : BaseSensor("GM-802B", "\033[33mPEE\033[0m", pin,
                         0.01F,   // alpha - fast response for pee
                         0.2F,    // tolerance
                         180.0F,  // preheating time
                         5000,    // min detect ms
                         10.938F, // coeff a
                         1.7742F) // coeff b
        {
        }

    protected:
        float calculate_ppm(float raw_value) const override
        {
            // NH3-specific PPM calculation
            const float rs = calculate_rs(raw_value);
            const float rs_r0_ratio = rs / m_r0;
            return m_coeff_a * std::exp(m_coeff_b * rs_r0_ratio);
        }
    };

} // namespace pooaway::sensors