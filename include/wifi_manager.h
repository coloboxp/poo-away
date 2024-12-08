#pragma once
#include <WiFi.h>
#include <string>
#include "esp_log.h"

namespace pooaway
{
    class WiFiManager
    {
    private:
        static constexpr char const *TAG = "WiFiManager";
        static constexpr int MAX_RETRIES = 20;
        static constexpr int RETRY_DELAY_MS = 500;

        bool m_is_connected = false;
        std::string m_last_error;

        WiFiManager() = default;

    public:
        static WiFiManager &instance();

        bool init();
        bool ensure_connected();
        bool is_connected() const { return m_is_connected; }
        const std::string &get_last_error() const { return m_last_error; }

        bool sync_time();
    };
}