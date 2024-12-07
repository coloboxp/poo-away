#pragma once
#include "base_sensor.h"

namespace pooaway::sensors
{
    class CH4Sensor : public BaseSensor
    {
    private:
        static constexpr float MIN_VALID_VOLTAGE = 0.4F;
        static constexpr float MAX_VALID_VOLTAGE = 4.0F;
        static constexpr float MIN_VALID_PPM = 0.0F;
        static constexpr float MAX_VALID_PPM = 10000.0F;
        static constexpr float RL = 4700.0F;
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
        CH4Sensor(int pin)
            : BaseSensor("GM-402B", "POO", pin,
                         0.05F,   // alpha (faster response)
                         0.4F,    // tolerance
                         30.0F,   // preheating time (30s)
                         3000,    // min detect ms
                         26.572F, // coeff a
                         1.2894F) // coeff b
        {
        }
    };
}