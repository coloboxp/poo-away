#pragma once
#include "sensors/nh3_sensor.h"
#include "sensors/ch4_sensor.h"
#include "sensors/sensor_types.h"
#include <Preferences.h>
#include <memory>
#include <array>

namespace pooaway::sensors
{

    class SensorManager
    {
    public:
        static SensorManager &instance();

        // Delete copy constructor and assignment operator
        SensorManager(const SensorManager &) = delete;
        SensorManager &operator=(const SensorManager &) = delete;

        // Core functionality
        void init();
        void update();
        bool needs_calibration() const;
        void perform_clean_air_calibration();
        void set_alerts_enabled(SensorType sensor_type, bool enable_state);
        void enter_low_power_mode();
        void exit_low_power_mode();
        void run_diagnostics();

        // Getters
        bool get_alert_status(SensorType sensor_type) const;
        float get_sensor_value(SensorType sensor_type) const;
        BaseSensor *get_sensor(SensorType sensor_type);

    private:
        SensorManager(); // Private constructor for singleton
        static constexpr char const *TAG = "SensorManager";
        Preferences m_preferences;

        // Sensor instances
        std::unique_ptr<NH3Sensor> m_nh3_sensor;
        std::unique_ptr<CH4Sensor> m_ch4_sensor;
        std::array<BaseSensor *, SENSOR_COUNT> m_sensors;

        bool m_calibration_in_progress{false};
        static void calibration_task(void *task_parameters);
        TaskHandle_t m_calibration_tasks[SENSOR_COUNT]{nullptr};
    };

} // namespace pooaway::sensors