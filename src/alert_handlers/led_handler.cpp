#include "alert_handlers/led_handler.h"
#include "esp_log.h"
#include "config.h"
#include <Arduino.h>

namespace pooaway::alert
{

    void LedHandler::init()
    {
        ESP_LOGI(TAG, "Initializing LED handler");
        pinMode(LED_PIN, OUTPUT);
        m_available = true;
    }

    void LedHandler::handle_alert(const bool alerts[pooaway::sensors::SENSOR_COUNT])
    {
        if (!m_available)
            return;

        bool any_alert = false;
        for (size_t i = 0; i < pooaway::sensors::SENSOR_COUNT; i++)
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

} // namespace pooaway::alert