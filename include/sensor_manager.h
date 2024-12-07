#pragma once
#include "sensors.h"
#include <Preferences.h>

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
    void calibrate_sensor(SensorType sensor_type);
    void set_alerts_enabled(SensorType sensor_type, bool enable_state);

    // Getters
    bool get_alert_status(SensorType sensor_type) const;
    float get_sensor_value(SensorType sensor_type) const;

private:
    SensorManager(); // Private constructor for singleton

    static constexpr char const *TAG = "SensorManager";
    Preferences m_preferences;

    // Helper methods
    float calculate_rs(float voltage_v) const;
    float convert_to_ppm(const SensorData &sensor_data, float rs_r0_ratio) const;
    float read_sensor(SensorData &sensor_data);
    void update_ema(SensorData &sensor_data);
    bool check_threshold(SensorData &sensor_data) const;
    static void calibration_task(void *task_parameters);

    // Task handles for calibration
    TaskHandle_t m_calibration_tasks[SENSOR_COUNT]{nullptr};
    bool m_calibration_in_progress{false};
};