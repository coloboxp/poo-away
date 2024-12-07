#pragma once
#include "base_sensor.h"

namespace pooaway::sensors
{
    class NH3Sensor : public BaseSensor
    {
    private:
        static constexpr float MIN_VALID_VOLTAGE = 0.0F;
        static constexpr float MAX_VALID_VOLTAGE = 2.0F; // Adjusted for observed readings
        static constexpr float MIN_VALID_PPM = 0.0F;
        static constexpr float MAX_VALID_PPM = 100.0F;
        static constexpr float RL = 10000.0F; // Load resistance in ohms

    protected:
        bool validate_reading(float raw_value) const override;
        float calculate_ppm(float raw_value) const override;
        bool is_valid_ppm(float ppm) const override;
        float calculate_rs(float voltage) const override;

    public:
        NH3Sensor(int pin)
            : BaseSensor("GM-802B", "PEE", pin,
                         0.01F,   // alpha
                         0.2F,    // tolerance
                         180.0F,  // preheating time
                         5000,    // min detect ms
                         10.938F, // coeff a
                         1.7742F) // coeff b
        {
        }
    };

} // namespace pooaway::sensors