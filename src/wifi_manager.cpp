#include "wifi_manager.h"
#include "config.h"
#include <time.h>
#include "esp_sntp.h"

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
        return ensure_connected() && sync_time();
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

    bool WiFiManager::sync_time()
    {
        ESP_LOGI(TAG, "Synchronizing time with NTP server...");

        // Configure NTP
        configTzTime(config::ntp::TIMEZONE, config::ntp::SERVER);

        // Wait for time to be set
        int retry = 0;
        const int max_retry = 10;
        while (time(nullptr) < 1000000000 && retry < max_retry)
        {
            ESP_LOGD(TAG, "Waiting for NTP time sync... (%d/%d)", retry + 1, max_retry);
            delay(500);
            retry++;
        }

        if (time(nullptr) < 1000000000)
        {
            m_last_error = "Failed to sync time with NTP server";
            ESP_LOGE(TAG, "%s", m_last_error.c_str());
            return false;
        }

        // Log current time
        struct tm timeinfo;
        if (getLocalTime(&timeinfo))
        {
            char time_str[64];
            strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S %Z", &timeinfo);
            ESP_LOGI(TAG, "Time synchronized: %s", time_str);
        }

        return true;
    }
}