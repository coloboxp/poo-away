/**
 * @file main.cpp
 * @brief Main program for the PooAway sensor
 */

#include <Arduino.h>
#include "sensor_manager.h"
#include "alert_manager.h"
#include "debug_manager.h"
#include "config.h"
#include "alert_handlers/api_handler.h"
#include "alert_handlers/buzzer_handler.h"
#include "alert_handlers/led_handler.h"
#include "alert_handlers/mqtt_handler.h"
#include "wifi_manager.h"

using namespace pooaway::alert;
using namespace pooaway::sensors;
using namespace pooaway;

static constexpr char const *TAG = "Main";

// Initialize alert handlers with rate limits
static ApiHandler api_handler(config::alerts::API_RATE_LIMIT_MS);
static MqttHandler mqtt_handler(config::alerts::MQTT_RATE_LIMIT_MS);
static LedHandler led_handler(config::alerts::LED_RATE_LIMIT_MS);
static BuzzerHandler buzzer_handler(config::alerts::BUZZER_RATE_LIMIT_MS);

void setup()
{
    // Initialize serial communication
    Serial.begin(115200);
    ESP_LOGI(TAG, "Starting PooAway sensor system...");

    // Initialize GPIO pins
    pinMode(config::hardware::LED_PIN, OUTPUT);
    pinMode(config::hardware::BUZZER_PIN, OUTPUT);
    pinMode(config::hardware::CALIBRATION_BTN_PIN, INPUT_PULLUP);
    pinMode(config::hardware::CALIBRATION_LED_PIN, OUTPUT);

    // Initialize managers
    SensorManager::instance().init();
    WiFiManager::instance().init();
    AlertManager::instance().init();
    DebugManager::instance().init();

    // Initialize alert handlers
    auto &alert_manager = AlertManager::instance();

    static BuzzerHandler buzzer_handler;
    static LedHandler led_handler;
    static MqttHandler mqtt_handler;

    alert_manager.add_handler(&buzzer_handler);
    alert_manager.add_handler(&led_handler);
    // alert_manager.add_handler(&mqtt_handler);
    alert_manager.add_handler(&api_handler);

    // Initial calibration if needed
    if (SensorManager::instance().needs_calibration())
    {
        ESP_LOGI(TAG, "No calibration values found, performing initial calibration...");
        SensorManager::instance().perform_clean_air_calibration();
    }

    // Run initial diagnostics
    SensorManager::instance().run_diagnostics();

    ESP_LOGI(TAG, "Setup complete!");
}

void loop()
{
    using pooaway::sensors::SensorType;

    static bool alerts[2] = {false};
    static bool last_btn_state = true;
    static unsigned long last_debounce = 0;

    // Handle calibration button
    const bool current_btn_state = digitalRead(config::hardware::CALIBRATION_BTN_PIN);
    if (current_btn_state != last_btn_state)
    {
        last_debounce = millis();
    }

    if ((millis() - last_debounce) > config::input::DEBOUNCE_DELAY)
    {
        if (current_btn_state == LOW)
        { // Button pressed (active LOW)
            ESP_LOGI(TAG, "Calibration button pressed");
            SensorManager::instance().perform_clean_air_calibration();
        }
    }
    last_btn_state = current_btn_state;

    // Update sensors
    auto &sensor_manager = SensorManager::instance();
    sensor_manager.update();

    // Check for alerts
    for (size_t i = 0; i < SENSOR_COUNT; i++)
    {
        const auto sensor_type = static_cast<SensorType>(i);
        alerts[i] = sensor_manager.get_alert_status(sensor_type);

        if (alerts[i])
        {
            const auto *sensor = sensor_manager.get_sensor(sensor_type);
            if (sensor)
            {
                ESP_LOGW(TAG, "Alert from %s! Value: %.2f",
                         sensor->get_name(),
                         sensor_manager.get_sensor_value(sensor_type));
            }
        }
    }

    // Update alert system
    AlertManager::instance().update(alerts);

    // Small delay to prevent tight looping
    delay(10);
}
