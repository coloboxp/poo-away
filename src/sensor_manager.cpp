#include "sensor_manager.h"
#include "esp_log.h"
#include <cmath>

SensorManager &SensorManager::instance()
{
    static SensorManager instance;
    return instance;
}

SensorManager::SensorManager()
{
    m_preferences.begin("pooaway", false);
}

void SensorManager::init()
{
    // Load calibration values from NVS
    for (int i = 0; i < SENSOR_COUNT; i++)
    {
        sensors[i].cal.r0 = m_preferences.getFloat(
            sensors[i].model,
            sensors[i].cal.r0);
        ESP_LOGI(TAG, "Loaded R0=%.1f for %s sensor",
                 sensors[i].cal.r0,
                 sensors[i].name);
    }
}

void SensorManager::update()
{
    for (int i = 0; i < SENSOR_COUNT; i++)
    {
        update_ema(sensors[i]);
    }
}

bool SensorManager::needs_calibration() const
{
    for (const auto &sensor : sensors)
    {
        if (sensor.cal.r0 <= 0.0F)
        {
            return true;
        }
    }
    return false;
}

void SensorManager::perform_clean_air_calibration()
{
    if (m_calibration_in_progress)
    {
        ESP_LOGI(TAG, "Calibration already in progress");
        return;
    }

    m_calibration_in_progress = true;
    digitalWrite(CALIBRATION_LED_PIN, HIGH);

    ESP_LOGI(TAG, "Starting clean air calibration...");

    for (int i = 0; i < SENSOR_COUNT; i++)
    {
        calibrate_sensor(static_cast<SensorType>(i));
    }

    m_calibration_in_progress = false;
    digitalWrite(CALIBRATION_LED_PIN, LOW);
}

void SensorManager::calibrate_sensor(SensorType sensor_type)
{
    const int sensor_idx = static_cast<int>(sensor_type);

    if (sensor_idx >= SENSOR_COUNT)
    {
        ESP_LOGE(TAG, "Invalid sensor type: %d", sensor_idx);
        return;
    }

    if (m_calibration_tasks[sensor_idx] != nullptr)
    {
        ESP_LOGI(TAG, "Calibration already in progress for %s", sensors[sensor_idx].name);
        return;
    }

    xTaskCreate(
        calibration_task,
        "calibration_task",
        2048,
        &sensors[sensor_idx],
        1,
        &m_calibration_tasks[sensor_idx]);
}

void SensorManager::set_alerts_enabled(SensorType sensor_type, bool enable_state)
{
    const int sensor_idx = static_cast<int>(sensor_type);

    if (sensor_idx >= SENSOR_COUNT)
    {
        ESP_LOGE(TAG, "Invalid sensor type: %d", sensor_idx);
        return;
    }

    sensors[sensor_idx].alertsEnabled = enable_state;
    ESP_LOGI(TAG, "%s alerts %s",
             sensors[sensor_idx].name,
             enable_state ? "enabled" : "disabled");
}

bool SensorManager::get_alert_status(SensorType sensor_type) const
{
    const int sensor_idx = static_cast<int>(sensor_type);
    return (sensor_idx < SENSOR_COUNT) ? check_threshold(sensors[sensor_idx]) : false;
}

float SensorManager::get_sensor_value(SensorType sensor_type) const
{
    const int sensor_idx = static_cast<int>(sensor_type);
    return (sensor_idx < SENSOR_COUNT) ? sensors[sensor_idx].value : 0.0F;
}

// Private methods implementation
float SensorManager::calculate_rs(float voltage_v) const
{
    if (voltage_v < 0.001F)
    {
        voltage_v = 0.001F; // Prevent division by zero
    }
    return RL * ((VCC / voltage_v) - 1.0F);
}

float SensorManager::convert_to_ppm(const SensorData &sensor_data, float rs_r0_ratio) const
{
    return sensor_data.cal.a * std::exp(sensor_data.cal.b * rs_r0_ratio);
}

float SensorManager::read_sensor(SensorData &sensor_data)
{
    const float adc_value = static_cast<float>(analogRead(sensor_data.pin));
    const float vout = adc_value * (VCC / static_cast<float>(ADC_RESOLUTION));
    const float rs = vout > 0.0F ? calculate_rs(vout) : sensor_data.value;
    const float rs_r0 = sensor_data.cal.r0 > 0.0F ? rs / sensor_data.cal.r0 : 1.0F;
    const float ppm = convert_to_ppm(sensor_data, rs_r0);

    if (DEBUG_SENSORS)
    {
        ESP_LOGI(TAG, "%s: ADC=%.0f, Vout=%.3f, Rs=%.3f, Rs/R0=%.3f, PPM=%.1f",
                 sensor_data.name, adc_value, vout, rs, rs_r0, ppm);
    }

    return ppm;
}

void SensorManager::update_ema(SensorData &sensor_data)
{
    const float new_value = read_sensor(sensor_data);
    if (sensor_data.firstReading)
    {
        sensor_data.baselineEMA = new_value;
        sensor_data.firstReading = false;
    }
    else
    {
        sensor_data.baselineEMA = (sensor_data.alpha * new_value) +
                                  ((1.0F - sensor_data.alpha) * sensor_data.baselineEMA);
    }
    sensor_data.value = new_value;
}

bool SensorManager::check_threshold(SensorData &sensor_data) const
{
    if (!sensor_data.alertsEnabled)
    {
        return false;
    }

    const float ratio = sensor_data.value / sensor_data.baselineEMA;
    const bool is_triggered = ratio < (1.0F - sensor_data.tolerance);

    if (is_triggered)
    {
        if (sensor_data.detectStart == 0)
        {
            sensor_data.detectStart = millis();
        }
        return (millis() - sensor_data.detectStart) >=
               static_cast<unsigned long>(sensor_data.minDetectMs);
    }

    sensor_data.detectStart = 0;
    return false;
}

void SensorManager::calibration_task(void *task_parameters)
{
    SensorData *sensor = static_cast<SensorData *>(task_parameters);
    const int num_readings = sensor->cal.numReadingsForR0;
    float sum = 0.0F;
    int valid_readings = 0;

    // Get instance for preferences access
    static Preferences preferences;
    preferences.begin("pooaway", false);

    ESP_LOGI(TAG, "Starting calibration for %s sensor", sensor->name);
    ESP_LOGI(TAG, "Preheating for %.0f seconds...", sensor->cal.preheatingTime);

    vTaskDelay(pdMS_TO_TICKS(static_cast<uint32_t>(sensor->cal.preheatingTime * 1000.0F)));

    for (int i = 0; i < num_readings && valid_readings < num_readings; i++)
    {
        const float adc_value = static_cast<float>(analogRead(sensor->pin));
        const float vout = adc_value * (VCC / static_cast<float>(ADC_RESOLUTION));

        if (vout > 0.0F)
        {
            const float rs = RL * ((VCC / vout) - 1.0F);
            sum += rs;
            valid_readings++;

            if (DEBUG_SENSORS)
            {
                ESP_LOGI(TAG, "Calibration reading %d/%d: ADC=%.0f, Vout=%.3f, Rs=%.0f",
                         valid_readings, num_readings, adc_value, vout, rs);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100)); // 100ms between readings
    }

    if (valid_readings > 0)
    {
        sensor->cal.r0 = sum / static_cast<float>(valid_readings);
        preferences.putFloat(sensor->model, sensor->cal.r0);
        ESP_LOGI(TAG, "Calibration complete for %s sensor. R0=%.1f",
                 sensor->name, sensor->cal.r0);
    }
    else
    {
        ESP_LOGE(TAG, "Calibration failed for %s sensor", sensor->name);
        sensor->cal.r0 = RL; // Use load resistor as fallback
    }

    preferences.end(); // Close preferences
    vTaskDelete(nullptr);
}