#pragma once
#include <string>
#include <WiFi.h>

// Try to include private configuration if available
#if __has_include("private.h")
#include "private.h"
#endif

namespace config
{
    namespace wifi
    {
#ifndef WIFI_SSID
        constexpr char const *SSID = "your_ssid";
#else
        constexpr char const *SSID = WIFI_SSID;
#endif

#ifndef WIFI_PASS
        constexpr char const *PASSWORD = "your_password";
#else
        constexpr char const *PASSWORD = WIFI_PASS;
#endif
    }

    namespace mqtt
    {
#ifndef MQTT_USERNAME
        constexpr char const *USERNAME = "anonymous";
#else
        constexpr char const *USERNAME = MQTT_USERNAME;
#endif

#ifndef MQTT_PASSWORD
        constexpr char const *PASSWORD = "anonymous";
#else
        constexpr char const *PASSWORD = MQTT_PASSWORD;
#endif

#ifndef MQTT_CLIENT_ID
        constexpr char const *CLIENT_ID = "anonymous";
#else
        constexpr char const *CLIENT_ID = MQTT_CLIENT_ID;
#endif

#ifndef MQTT_FEED_PREFIX
        constexpr char const *FEED_PREFIX = "anonymous";
#else
        constexpr char const *FEED_PREFIX = MQTT_FEED_PREFIX;
#endif

        constexpr char const *BROKER = "io.adafruit.com";
        constexpr int PORT = 1883;
        constexpr unsigned long RATE_LIMIT_MS = 5000; // 5 seconds between MQTT publishes
    }

    namespace adafruit_io
    {
#ifndef AIO_USERNAME
        constexpr char const *USERNAME = "anonymous";
#else
        constexpr char const *USERNAME = AIO_USERNAME;
#endif

#ifndef AIO_KEY
        constexpr char const *KEY = "your_key";
#else
        constexpr char const *KEY = AIO_KEY;
#endif
    }

    namespace api
    {
#ifndef API_PATH
        constexpr char const *ENDPOINT = "https://io.adafruit.com/api/v2/anonymous/feeds";
#else
        constexpr char const *ENDPOINT = API_PATH;
#endif

        constexpr int TIMEOUT_MS = 5000;
        constexpr unsigned long RATE_LIMIT_MS = 30000; // 30 seconds between API requests
    }

    namespace hardware
    {
        // Pin Definitions
        constexpr int LED_PIN = 15;
        constexpr int PEE_SENSOR_PIN = 4;
        constexpr int POO_SENSOR_PIN = 5;
        constexpr int BUZZER_PIN = 6;
        constexpr int CALIBRATION_BTN_PIN = 7;
        constexpr int CALIBRATION_LED_PIN = LED_PIN;
    }

    namespace sensors
    {
        // ADC Configuration
        constexpr float VCC = 3.3F;          // Operating voltage
        constexpr int ADC_RESOLUTION = 4096; // 12-bit ADC
        constexpr float RL = 10000.0F;       // Load resistance in kΩ

        // Thresholds and Calibration
        // ... Add sensor-specific thresholds here if needed
    }

    namespace alerts
    {
        // Rate Limiting
        // Alert Rate Limiting Configuration
        constexpr unsigned long API_RATE_LIMIT_MS = 30000;   // 30 seconds between API requests
        constexpr unsigned long MQTT_RATE_LIMIT_MS = 5000;   // 5 seconds between MQTT publishes
        constexpr unsigned long LED_RATE_LIMIT_MS = 500;     // 500ms between LED state changes
        constexpr unsigned long BUZZER_RATE_LIMIT_MS = 1000; // 1 second between buzzer alerts

        // Alert Configuration
        constexpr unsigned long ALERT_INTERVAL = 1000; // milliseconds
    }

    namespace input
    {
        constexpr unsigned long DEBOUNCE_DELAY = 50; // Button debounce delay in milliseconds
    }

    namespace system
    {
        // System-wide configurations
        constexpr unsigned long TASK_DELAY = 100; // Default task delay in milliseconds
    }

    namespace ntp
    {
        constexpr char const *SERVER = "pool.ntp.org";
        constexpr char const *TIMEZONE = "CET-1CEST,M3.5.0,M10.5.0/3"; // Central European Time (Bratislava)
        constexpr long GMT_OFFSET_SEC = 3600;                          // GMT+1 for Bratislava
        constexpr long DAYLIGHT_OFFSET_SEC = 3600;                     // +1 hour for daylight saving
        constexpr unsigned long SYNC_INTERVAL = 3600000;               // Resync every hour
    }

    namespace thingspeak
    {
        constexpr char const *ENDPOINT = "https://api.thingspeak.com/update";

#ifndef THINGSPEAK_USER_API_KEY
        constexpr char const *USER_API_KEY = "your_user_api_key";
#else
        constexpr char const *USER_API_KEY = THINGSPEAK_USER_API_KEY;
#endif

#ifndef THINGSPEAK_ALERTS_API_KEY
        constexpr char const *ALERTS_API_KEY = "your_alerts_api_key";
#else
        constexpr char const *ALERTS_API_KEY = THINGSPEAK_ALERTS_API_KEY;
#endif

#ifndef THINGSPEAK_NH3_API_KEY
        constexpr char const *NH3_API_KEY = "your_nh3_api_key";
#else
        constexpr char const *NH3_API_KEY = THINGSPEAK_NH3_API_KEY;
#endif

#ifndef THINGSPEAK_CH4_API_KEY
        constexpr char const *CH4_API_KEY = "your_ch4_api_key";
#else
        constexpr char const *CH4_API_KEY = THINGSPEAK_CH4_API_KEY;
#endif

        constexpr unsigned long UPDATE_INTERVAL_MS = 15000;
        constexpr int MAX_FIELDS = 8;
        constexpr bool PUBLIC_FLAG = false;

#ifndef THINGSPEAK_NH3_CHANNEL_ID
        constexpr char const *NH3_CHANNEL_ID = "your_nh3_channel_id";
#else
        constexpr char const *NH3_CHANNEL_ID = THINGSPEAK_NH3_CHANNEL_ID;
#endif

#ifndef THINGSPEAK_CH4_CHANNEL_ID
        constexpr char const *CH4_CHANNEL_ID = "your_ch4_channel_id";
#else
        constexpr char const *CH4_CHANNEL_ID = THINGSPEAK_CH4_CHANNEL_ID;
#endif
    }
}
