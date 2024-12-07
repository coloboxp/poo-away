#include "alert_handlers/led_handler.h"
#include "esp_log.h"
#include "config.h"
#include <Arduino.h>

namespace pooaway::alert
{

    LedHandler::LedHandler(unsigned long rate_limit_ms)
        : m_rate_limit_ms(rate_limit_ms) {}

    void LedHandler::init()
    {
        ESP_LOGI(TAG, "Initializing LED handler");
        pinMode(LED_PIN, OUTPUT);
        m_available = true;
        if (m_rate_limit_ms > 0)
        {
            ESP_LOGI(TAG, "Rate limiting enabled: %lu ms", m_rate_limit_ms);
        }
    }

    void LedHandler::handle_alert(JsonDocument &alert_data)
    {
        if (!m_available)
            return;

        // Check rate limiting
        if (m_rate_limit_ms > 0)
        {
            unsigned long now = millis();
            if (now - m_last_request < m_rate_limit_ms)
            {
                return;
            }
            m_last_request = now;
        }

        bool any_alert = false;
        JsonArray sensors = alert_data["sensors"].as<JsonArray>();

        for (const JsonObject &sensor : sensors)
        {
            if (sensor["alert"].as<bool>())
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