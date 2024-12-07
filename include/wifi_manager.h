#pragma once
#include <WiFi.h>
#include "esp_log.h"

namespace pooaway
{
    class WiFiManager
    {
    private:
        static constexpr char const *TAG = "WiFiManager";
        static constexpr int MAX_RETRIES = 5;
        static constexpr int RETRY_DELAY_MS = 1000;

        bool m_is_connected = false;
        String m_last_error;

        WiFiManager() = default;

    public:
        static WiFiManager &instance();

        bool init();
        bool ensure_connected();
        bool is_connected() const { return m_is_connected; }
        const String &get_last_error() const { return m_last_error; }
    };
}