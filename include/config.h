#pragma once
#include <string>
#include <WiFi.h>

// Try to include private configuration if available
#if __has_include("private.h")
#include "private.h"
#else
// Default anonymous configurations
#define WIFI_SSID "your_ssid"
#define WIFI_PASS "your_password"

#define MQTT_USERNAME "anonymous"
#define MQTT_PASSWORD "anonymous"
#define MQTT_CLIENT_ID "anonymous"
#define MQTT_FEED_PREFIX "anonymous"

#define AIO_USERNAME "anonymous"
#define AIO_KEY "your_key"
#define API_PATH "https://io.adafruit.com/api/v2/anonymous/feeds"
#endif

// Hardware Configuration
constexpr int LED_PIN = 15;
constexpr int PEE_SENSOR_PIN = 4;
constexpr int POO_SENSOR_PIN = 5;
constexpr int BUZZER_PIN = 6;
constexpr int CALIBRATION_BTN_PIN = 7;
constexpr int CALIBRATION_LED_PIN = LED_PIN;

// ADC Configuration
constexpr float VCC = 3.3F;          // Operating voltage
constexpr int ADC_RESOLUTION = 4096; // 12-bit ADC
constexpr float RL = 10000.0F;      // Load resistance in kΩ

// Button Configuration
constexpr unsigned long DEBOUNCE_DELAY = 50;

// API Configuration
constexpr char const *API_ENDPOINT = API_PATH "/feeds/data/data";
constexpr int API_TIMEOUT = 5000; // milliseconds

// MQTT Configuration
constexpr char const *MQTT_BROKER = "io.adafruit.com";
constexpr int MQTT_PORT = 1883;

// Alert Rate Limiting Configuration
constexpr unsigned long API_RATE_LIMIT_MS = 30000;   // 30 seconds between API requests
constexpr unsigned long MQTT_RATE_LIMIT_MS = 5000;   // 5 seconds between MQTT publishes
constexpr unsigned long LED_RATE_LIMIT_MS = 500;     // 500ms between LED state changes
constexpr unsigned long BUZZER_RATE_LIMIT_MS = 1000; // 1 second between buzzer alerts

// Alert Configuration
constexpr unsigned long ALERT_INTERVAL = 1000; // milliseconds
