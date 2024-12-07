#include "alert_handlers/led_handler.h"
#include "esp_log.h"
#include "config.h"

void LedHandler::init()
{
    ESP_LOGI(TAG, "Initializing LED handler");
    pinMode(LED_PIN, OUTPUT);
}

void LedHandler::handle_alert(const bool alerts[SENSOR_COUNT])
{
    bool any_alert = false;
    for (int i = 0; i < SENSOR_COUNT; i++)
    {
        if (alerts[i])
        {
            any_alert = true;
            break;
        }
    }

    if (any_alert)
    {
        m_led_state = !m_led_state;
        digitalWrite(LED_PIN, m_led_state);
    }
    else
    {
        digitalWrite(LED_PIN, LOW);
    }
}