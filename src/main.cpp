/**
 * @file main.cpp
 * @brief Main program for the PooAway sensor
 *
 * @author: @coloboxp
 * @date: 2024-11-28
 */

#include <Arduino.h>
#include <cmath>
#include "esp_log.h"
#include "sensors.h"
#include <Preferences.h>

// Try to include private configuration if available
#if __has_include("private.h")
#include "private.h"
#endif
#include "config.h"

namespace
{
    constexpr char const *TAG = "PooAway";
    Preferences preferences;

    // Function declarations
    float calculateRs(float voltage_v);
    float readSensor(SensorData &sensor_data);
    float convertToPPM(const SensorData &sensor_data, float rs_r0_ratio);
    void updateEMA(SensorData &sensor_data);
    bool checkThreshold(SensorData &sensor_data);
    void handleAlerts(bool alerts_array[]);
    void playTone(int frequency_hz, int duration_ms);
    void printSensorData();
    void calibrationTask(void *task_parameters);
    void performCleanAirCalibration();
    void calibrateSensor(SensorType sensor_type);
    void setSensorAlerts(SensorType sensor_type, bool enable_state);

    float calculateRs(float voltage_v)
    {
        if (voltage_v < 0.001F)
        {
            voltage_v = 0.001F; // Prevent division by zero
        }
        return RL * ((VCC / voltage_v) - 1.0F);
    }

    float readSensor(SensorData &sensor_data)
    {
        const float adc_value = static_cast<float>(analogRead(sensor_data.pin));
        const float vout = adc_value * (VCC / static_cast<float>(ADC_RESOLUTION));
        const float rs = vout > 0.0F ? calculateRs(vout) : sensor_data.value;
        const float rs_r0 = sensor_data.cal.r0 > 0.0F ? rs / sensor_data.cal.r0 : 1.0F;
        const float ppm = convertToPPM(sensor_data, rs_r0);

        if (DEBUG_SENSORS)
        {
            ESP_LOGI(TAG, "%s: ADC=%.0f, Vout=%.3f, Rs=%.3f, Rs/R0=%.3f, PPM=%.1f",
                     sensor_data.name, adc_value, vout, rs, rs_r0, ppm);
        }

        return ppm;
    }

    float convertToPPM(const SensorData &sensor_data, float rs_r0_ratio)
    {
        return sensor_data.cal.a * std::exp(sensor_data.cal.b * rs_r0_ratio);
    }

    void updateEMA(SensorData &sensor_data)
    {
        const float new_value = readSensor(sensor_data);
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

    bool checkThreshold(SensorData &sensor_data)
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

    void playTone(int frequency_hz, int duration_ms)
    {
        tone(BUZZER_PIN, frequency_hz, duration_ms);
    }

    void handleAlerts(bool alerts_array[])
    {
        static unsigned long last_alert = 0;
        const unsigned long now = millis();

        if (now - last_alert < 1000UL)
        {
            return;
        }

        last_alert = now;

        for (int i = 0; i < SENSOR_COUNT; i++)
        {
            if (alerts_array[i])
            {
                playTone(1000 + (i * 500), 100);
                digitalWrite(LED_PIN, !digitalRead(LED_PIN));
                return;
            }
        }
        digitalWrite(LED_PIN, LOW);
    }

    void printSensorData()
    {
        static unsigned long last_print = 0;
        const unsigned long now = millis();

        if (now - last_print < PRINT_INTERVAL)
        {
            return;
        }

        last_print = now;

        for (int i = 0; i < SENSOR_COUNT; i++)
        {
            ESP_LOGI(TAG, "%s: Rs/R0=%.3f (R0=%.1f) Threshold=%.3f %s",
                     sensors[i].name,
                     sensors[i].value / sensors[i].cal.r0,
                     sensors[i].cal.r0,
                     sensors[i].tolerance,
                     sensors[i].alertsEnabled ? "ENABLED" : "DISABLED");
        }
    }

    void calibrationTask(void *task_parameters)
    {
        SensorData *sensor = static_cast<SensorData *>(task_parameters);
        const int num_readings = sensor->cal.numReadingsForR0;
        float sum = 0.0F;
        int valid_readings = 0;

        ESP_LOGI(TAG, "Starting calibration for %s sensor", sensor->name);
        ESP_LOGI(TAG, "Preheating for %.0f seconds...", sensor->cal.preheatingTime);

        vTaskDelay(pdMS_TO_TICKS(static_cast<uint32_t>(sensor->cal.preheatingTime * 1000.0F)));

        for (int i = 0; i < num_readings && valid_readings < num_readings; i++)
        {
            const float adc_value = static_cast<float>(analogRead(sensor->pin));
            const float vout = adc_value * (VCC / static_cast<float>(ADC_RESOLUTION));

            if (vout > 0.0F)
            {
                const float rs = calculateRs(vout);
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

        vTaskDelete(nullptr);
    }

    void calibrateSensor(SensorType sensor_type)
    {
        static TaskHandle_t calibration_tasks[SENSOR_COUNT] = {nullptr};
        const int sensor_idx = static_cast<int>(sensor_type);

        if (sensor_idx >= SENSOR_COUNT)
        {
            ESP_LOGE(TAG, "Invalid sensor type: %d", sensor_idx);
            return;
        }

        if (calibration_tasks[sensor_idx] != nullptr)
        {
            ESP_LOGI(TAG, "Calibration already in progress for %s", sensors[sensor_idx].name);
            return;
        }

        xTaskCreate(
            calibrationTask,
            "calibration_task",
            2048,
            &sensors[sensor_idx],
            1,
            &calibration_tasks[sensor_idx]);
    }

    void performCleanAirCalibration()
    {
        static bool calibration_in_progress = false;

        if (calibration_in_progress)
        {
            ESP_LOGI(TAG, "Calibration already in progress");
            return;
        }

        calibration_in_progress = true;
        digitalWrite(CALIBRATION_LED_PIN, HIGH);

        ESP_LOGI(TAG, "Starting clean air calibration...");

        // Calibrate all sensors
        for (int i = 0; i < SENSOR_COUNT; i++)
        {
            calibrateSensor(static_cast<SensorType>(i));
        }

        calibration_in_progress = false;
        digitalWrite(CALIBRATION_LED_PIN, LOW);
    }

    void setSensorAlerts(SensorType sensor_type, bool enable_state)
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
}

void setup()
{
    // Initialize serial communication
    Serial.begin(115200);
    ESP_LOGI(TAG, "Starting PooAway sensor system...");

    // Initialize GPIO pins
    pinMode(LED_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(CALIBRATION_BTN_PIN, INPUT_PULLUP);

    // Initialize preferences
    preferences.begin("pooaway", false);

    // Load calibration values from NVS
    for (int i = 0; i < SENSOR_COUNT; i++)
    {
        sensors[i].cal.r0 = preferences.getFloat(
            sensors[i].model,
            sensors[i].cal.r0);
        ESP_LOGI(TAG, "Loaded R0=%.1f for %s sensor",
                 sensors[i].cal.r0,
                 sensors[i].name);
    }

    // Initially disable POO sensor alerts (less reliable)
    setSensorAlerts(POO, false);

    // Initial calibration if no values stored
    bool needs_calibration = false;
    for (const auto &sensor : sensors)
    {
        if (sensor.cal.r0 <= 0.0F)
        {
            needs_calibration = true;
            break;
        }
    }

    if (needs_calibration)
    {
        ESP_LOGI(TAG, "No calibration values found, performing initial calibration...");
        performCleanAirCalibration();
    }

    ESP_LOGI(TAG, "Setup complete!");
}

void loop()
{
    static bool alerts[SENSOR_COUNT] = {false};
    static bool last_btn_state = HIGH;
    static unsigned long last_debounce = 0;

    // Handle calibration button
    const bool current_btn_state = digitalRead(CALIBRATION_BTN_PIN);

    if (current_btn_state != last_btn_state)
    {
        last_debounce = millis();
    }

    if ((millis() - last_debounce) > DEBOUNCE_DELAY)
    {
        if (current_btn_state == LOW)
        { // Button pressed (active LOW)
            performCleanAirCalibration();
        }
    }

    last_btn_state = current_btn_state;

    // Update sensor readings and check thresholds
    for (int i = 0; i < SENSOR_COUNT; i++)
    {
        updateEMA(sensors[i]);
        alerts[i] = checkThreshold(sensors[i]);
    }

    // Handle alerts and display data
    handleAlerts(alerts);
    printSensorData();

    // Small delay to prevent overwhelming the system
    delay(10);
}
