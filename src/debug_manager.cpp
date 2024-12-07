#include "debug_manager.h"
#include "esp_log.h"
#include "config.h"
#include "sensor_manager.h"

DebugManager &DebugManager::instance()
{
    static DebugManager instance;
    return instance;
}

DebugManager::DebugManager() = default;

void DebugManager::init()
{
    ESP_LOGI(TAG, "Initializing debug manager");
}

void DebugManager::print_sensor_data()
{
    const unsigned long now = millis();

    if (now - m_last_print < PRINT_INTERVAL)
    {
        return;
    }

    m_last_print = now;

    for (int i = 0; i < SENSOR_COUNT; i++)
    {
        const auto &sensor = sensors[i];
        ESP_LOGI(TAG, "%s: Rs/R0=%.3f (R0=%.1f) Threshold=%.3f %s",
                 sensor.name,
                 sensor.value / sensor.cal.r0,
                 sensor.cal.r0,
                 sensor.tolerance,
                 sensor.alertsEnabled ? "ENABLED" : "DISABLED");
    }
}