#include "alert_handlers/buzzer_handler.h"
#include "esp_log.h"
#include "config.h"
#include <Arduino.h>

namespace pooaway::alert
{

    void BuzzerHandler::init()
    {
        ESP_LOGI(TAG, "Initializing buzzer handler");
        pinMode(BUZZER_PIN, OUTPUT);
        m_available = true;
    }

    void BuzzerHandler::handle_alert(JsonDocument &alert_data)
    {
        if (!m_available)
            return;

        const unsigned long now = millis();

        if (now - m_last_alert < ALERT_INTERVAL)
        {
            return;
        }

        JsonArray sensors = alert_data["sensors"].as<JsonArray>();
        for (const JsonObject &sensor : sensors)
        {
            if (sensor["alert"].as<bool>())
            {
                int base_freq = 1000;
                int freq_offset = sensor["index"].as<int>() * 500;
                play_tone(base_freq + freq_offset, 100);
                m_last_alert = now;
                return;
            }
        }
    }

    void BuzzerHandler::play_tone(int frequency_hz, int duration_ms)
    {
        tone(BUZZER_PIN, frequency_hz, duration_ms);
    }

} // namespace pooaway::alert