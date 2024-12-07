#pragma once
#include "base_sensor.h"

namespace pooaway::sensors
{

    class CH4Sensor : public BaseSensor
    {
    public:
        explicit CH4Sensor(int pin)
            : BaseSensor("GM-402B", "\033[38;5;130mPOO\033[0m", pin,
                         0.005F,  // alpha - slower response for poo
                         0.3F,    // tolerance
                         180.0F,  // preheating time
                         5000,    // min detect ms
                         26.572F, // coeff a
                         1.2894F) // coeff b
        {
        }

    protected:
        float calculate_ppm(float raw_value) const override
        {
            // CH4-specific PPM calculation
            const float rs = calculate_rs(raw_value);
            const float rs_r0_ratio = rs / m_r0;
            return m_coeff_a * std::exp(m_coeff_b * rs_r0_ratio);
        }
    };

} // namespace pooaway::sensors