/**
 * @file main.cpp
 * @brief Main program for the PooAway sensor
 */

#include <Arduino.h>
#include "sensor_manager.h"
#include "alert_manager.h"
#include "debug_manager.h"
#include "config.h"

static constexpr char const *TAG = "Main";

void setup()
{
    // Initialize serial communication
    Serial.begin(115200);
    ESP_LOGI(TAG, "Starting PooAway sensor system...");

    // Initialize managers
    SensorManager::instance().init();
    AlertManager::instance().init();
    DebugManager::instance().init();

    // Initialize GPIO pins
    pinMode(LED_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(CALIBRATION_BTN_PIN, INPUT_PULLUP);

    // Initial calibration if needed
    if (SensorManager::instance().needs_calibration())
    {
        ESP_LOGI(TAG, "No calibration values found, performing initial calibration...");
        SensorManager::instance().perform_clean_air_calibration();
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
            SensorManager::instance().perform_clean_air_calibration();
        }
    }

    last_btn_state = current_btn_state;

    // Update sensors and get alerts
    SensorManager::instance().update();

    // Handle alerts and debug output
    AlertManager::instance().update(alerts);
    DebugManager::instance().print_sensor_data();

    // Small delay to prevent overwhelming the system
    vTaskDelay(pdMS_TO_TICKS(10));
}
