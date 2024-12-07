#pragma once
#include "base_sensor.h"

namespace pooaway::sensors
{
    class CH4Sensor : public BaseSensor
    {
    private:
        static constexpr float MIN_VALID_VOLTAGE = 0.0F;
        static constexpr float MAX_VALID_VOLTAGE = 3.3F;
        static constexpr float MIN_VALID_PPM = 0.0F;
        static constexpr float MAX_VALID_PPM = 1000.0F;
        static constexpr float RL = 10000.0F; // Load resistance in ohms

    protected:
        bool validate_reading(float raw_value) const override;
        float calculate_ppm(float raw_value) const override;
        bool is_valid_ppm(float ppm) const override;
        float calculate_rs(float voltage) const override;

    public:
        CH4Sensor(int pin)
            : BaseSensor("GM-402B", "POO", pin,
                         0.005F,  // alpha
                         0.3F,    // tolerance
                         180.0F,  // preheating time
                         5000,    // min detect ms
                         26.572F, // coeff a
                         1.2894F) // coeff b
        {
        }
    };

} // namespace pooaway::sensors