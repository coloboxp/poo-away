#include "alert_handlers/buzzer_handler.h"
#include "esp_log.h"
#include "config.h"

namespace pooaway::alert
{

    void BuzzerHandler::init()
    {
        ESP_LOGI(TAG, "Initializing buzzer handler");
        pinMode(BUZZER_PIN, OUTPUT);
        m_available = true;
    }

    void BuzzerHandler::handle_alert(const bool alerts[SENSOR_COUNT])
    {
        if (!m_available)
            return;

        const unsigned long now = millis();

        if (now - m_last_alert < ALERT_INTERVAL)
        {
            return;
        }

        m_last_alert = now;

        for (int i = 0; i < SENSOR_COUNT; i++)
        {
            if (alerts[i])
            {
                play_tone(1000 + (i * 500), 100);
                return;
            }
        }
    }

    void BuzzerHandler::play_tone(int frequency_hz, int duration_ms)
    {
        tone(BUZZER_PIN, frequency_hz, duration_ms);
    }

} // namespace pooaway::alert