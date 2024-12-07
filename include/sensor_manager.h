#pragma once
#include <memory>
#include <array>
#include <Preferences.h>
#include "sensors/base_sensor.h"
#include "sensors/nh3_sensor.h"
#include "sensors/ch4_sensor.h"
#include "sensors/sensor_types.h"

namespace pooaway::sensors
{
    class SensorManager
    {
    private:
        static constexpr char const *TAG = "SensorManager";
        static constexpr size_t MAX_SENSORS = SENSOR_COUNT;

        std::array<BaseSensor *, MAX_SENSORS> m_sensors{};
        std::unique_ptr<NH3Sensor> m_nh3_sensor;
        std::unique_ptr<CH4Sensor> m_ch4_sensor;
        Preferences m_preferences;

        SensorManager();

    public:
        static SensorManager &instance();

        void init();
        void update();
        void perform_clean_air_calibration();
        void run_diagnostics();

        float get_sensor_value(SensorType type) const;
        bool get_alert_status(SensorType type) const;
        BaseSensor *get_sensor(SensorType type);
        bool needs_calibration() const;

        // Prevent copying
        SensorManager(const SensorManager &) = delete;
        SensorManager &operator=(const SensorManager &) = delete;
    };
} // namespace pooaway::sensors