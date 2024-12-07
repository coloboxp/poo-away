#include "debug_manager.h"
#include "esp_log.h"
#include "config.h"

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
    ESP_LOGI(TAG, "Sensor Data:");
    for (size_t i = 0; i < pooaway::sensors::SENSOR_COUNT; i++)
    {
        ESP_LOGI(TAG, "%s: Value=%.2f, Baseline=%.2f, R0=%.2f",
                 sensors[i].name,
                 sensors[i].value,
                 sensors[i].baselineEMA,
                 sensors[i].cal.r0);
    }
}