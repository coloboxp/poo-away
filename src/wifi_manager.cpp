#include "wifi_manager.h"
#include "config.h"

namespace pooaway
{
    WiFiManager &WiFiManager::instance()
    {
        static WiFiManager instance;
        return instance;
    }

    bool WiFiManager::init()
    {
        ESP_LOGI(TAG, "Initializing WiFi connection...");
        WiFi.mode(WIFI_STA);
        return ensure_connected();
    }

    bool WiFiManager::ensure_connected()
    {
        if (WiFi.status() == WL_CONNECTED)
        {
            m_is_connected = true;
            return true;
        }

        ESP_LOGI(TAG, "Connecting to WiFi network: %s", WIFI_SSID);
        WiFi.begin(WIFI_SSID, WIFI_PASS);

        int retries = 0;
        while (WiFi.status() != WL_CONNECTED && retries < MAX_RETRIES)
        {
            delay(RETRY_DELAY_MS);
            ESP_LOGD(TAG, "Waiting for WiFi connection... (%d/%d)", retries + 1, MAX_RETRIES);
            ESP_LOGD(TAG, "MAC: %s", WiFi.macAddress().c_str());
            retries++;
        }

        if (WiFi.status() == WL_CONNECTED)
        {
            m_is_connected = true;
            ESP_LOGI(TAG, "Connected to WiFi. IP: %s", WiFi.localIP().toString().c_str());
            return true;
        }

        m_is_connected = false;
        m_last_error = "Failed to connect to WiFi";
        ESP_LOGE(TAG, "%s", m_last_error.c_str());
        return false;
    }
}