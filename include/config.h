#pragma once

// Try to include private configuration if available
#if __has_include("private.h")
#include "private.h"
#endif

// Default values if not defined in private.h
#ifndef WIFI_SSID
#define WIFI_SSID "PooAway_Default"
#endif

#ifndef WIFI_PASS
#define WIFI_PASS "defaultpass"
#endif

#ifndef MQTT_USERNAME
#define MQTT_USERNAME "default_user"
#endif

#ifndef MQTT_PASSWORD
#define MQTT_PASSWORD "default_pass"
#endif

#ifndef MQTT_CLIENT_ID
#define MQTT_CLIENT_ID "pooaway_device"
#endif

#ifndef MQTT_FEED_PREFIX
#define MQTT_FEED_PREFIX "pooaway"
#endif

#ifndef AIO_USERNAME
#define AIO_USERNAME "usernamehere"
#endif

#ifndef MQTT_BROKER
#define MQTT_BROKER "mqtt.example.com"
#endif

#ifndef MQTT_PORT
#define MQTT_PORT 1883
#endif

#ifndef AIO_KEY
#define AIO_KEY "tokenhere"
#endif

#ifndef API_PATH
#define API_PATH "https://io.adafruit.com/api/v2/usernamehere/groups/groupnamehere"
#endif

// Pin definitions
constexpr int LED_PIN = 15;
constexpr int PEE_SENSOR_PIN = 4; // NH3 sensor
constexpr int POO_SENSOR_PIN = 5; // CH4 sensor
constexpr int BUZZER_PIN = 6;
constexpr int CALIBRATION_BTN_PIN = 7;
constexpr int CALIBRATION_LED_PIN = LED_PIN;

// ADC and voltage configuration
constexpr float VCC = 3.3F;
constexpr int ADC_RESOLUTION = 4095;
constexpr float RL = 10000.0F; // Load resistor (10kΩ) - renamed from LOAD_RESISTOR

// Debug configuration
constexpr bool DEBUG_SENSORS = true;

// Timing configurations
constexpr unsigned long PRINT_INTERVAL = 1000UL;
constexpr unsigned long PUBLISH_INTERVAL = 30000UL;
constexpr unsigned long DEBOUNCE_DELAY = 50UL;

constexpr char const *API_ENDPOINT = API_PATH;
constexpr int API_TIMEOUT = 5000; // milliseconds

// Alert settings
constexpr unsigned long ALERT_INTERVAL = 1000; // Alert check interval in milliseconds

// MQTT Configuration
