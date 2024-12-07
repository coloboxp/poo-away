#include "alert_handlers/buzzer_handler.h"
#include "esp_log.h"
#include "config.h"
#include <Arduino.h>

namespace pooaway::alert
{

    BuzzerHandler::BuzzerHandler(unsigned long rate_limit_ms)
        : m_rate_limit_ms(rate_limit_ms) {}

    void BuzzerHandler::init()
    {
        ESP_LOGI(TAG, "Initializing buzzer handler");
        pinMode(BUZZER_PIN, OUTPUT);
        m_available = true;
        if (m_rate_limit_ms > 0)
        {
            ESP_LOGI(TAG, "Rate limiting enabled: %lu ms", m_rate_limit_ms);
        }
    }

    void BuzzerHandler::handle_alert(JsonDocument &alert_data)
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

        auto sensors = alert_data["sensors"].as<JsonArray>();

        for (JsonObject sensor : sensors)
        {
            if (sensor["alert"].as<bool>())
            {
                // Different tones for different sensors
                int base_freq = 2000;
                int freq_offset = sensor["index"].as<int>() * 200;
                play_tone(base_freq + freq_offset, 100);
                return; // Play only one tone even if multiple alerts
            }
        }
    }

    void BuzzerHandler::play_tone(int frequency_hz, int duration_ms)
    {
        tone(BUZZER_PIN, frequency_hz, duration_ms);
    }

} // namespace pooaway::alert