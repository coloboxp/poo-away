#include "alert_manager.h"
#include "esp_log.h"
#include "config.h"

AlertManager &AlertManager::instance()
{
    static AlertManager instance;
    return instance;
}

AlertManager::AlertManager() = default;

void AlertManager::init()
{
    ESP_LOGI(TAG, "Initializing alert manager");
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(LED_PIN, OUTPUT);
}

void AlertManager::update(const bool alerts[SENSOR_COUNT])
{
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
            digitalWrite(LED_PIN, !digitalRead(LED_PIN));
            return;
        }
    }
    digitalWrite(LED_PIN, LOW);
}

void AlertManager::play_tone(int frequency_hz, int duration_ms)
{
    tone(BUZZER_PIN, frequency_hz, duration_ms);
}