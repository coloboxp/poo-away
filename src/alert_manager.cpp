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
    for (auto handler : m_handlers)
    {
        handler->init();
    }
}

void AlertManager::update(const bool alerts[SENSOR_COUNT])
{
    const unsigned long now = millis();

    if (now - m_last_alert < ALERT_INTERVAL)
    {
        return;
    }

    m_last_alert = now;

    for (auto handler : m_handlers)
    {
        handler->handle_alert(alerts);
    }
}

void AlertManager::add_handler(AlertHandler *handler)
{
    m_handlers.push_back(handler);
    handler->init();
}

void AlertManager::remove_handler(AlertHandler *handler)
{
    auto it = std::find(m_handlers.begin(), m_handlers.end(), handler);
    if (it != m_handlers.end())
    {
        m_handlers.erase(it);
    }
}