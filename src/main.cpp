/**
 * @file main.cpp
 * @brief Main program for the PooAway sensor
 *
 * @author: @coloboxp
 * @date: 2024-11-28
 *
 * This sketch is designed to detect when the pet does its business
 * and alert the user with a buzzer and LED to scare the pet away and
 * discourage the behavior, likewise the user react quickly and clean
 * the mess before the pet can play with it or eat it.
 */

#include <Arduino.h>
#include "esp_log.h"

const int LED_PIN = 15;
const int PEE_SENSOR_PIN = 4;
const int POO_SENSOR_PIN = 5;
const int BUZZER_PIN = 6;
const float VCC = 3.3;
const int ADC_RESOLUTION = 4095;

// Add enum for sensor types
enum SensorType
{
    PEE,         // Pee sensor
    POO,         // Poo sensor
    SENSOR_COUNT // Total number of sensors (must be last enum value)
};

// Sensor data structure, to hold the sensor configuration and values.
// TODO: Handle via BLE
struct SensorData
{
    const int pin;         // Pin number where the sensor is connected
    const char *name;      // Name for better messaging
    const float alpha;     // Response speed
    const float tolerance; // Sensitivity
    float baselineEMA;     // Baseline EMA
    bool firstReading;     // First reading flag
    float value;           // Sensor value
    bool alertsEnabled;    // Flag to enable/disable alerts
};

// Sensor configurations
SensorData sensors[SENSOR_COUNT] = {
    // Yellow for pee detection (ANSI escape code colors, for terminal)
    {PEE_SENSOR_PIN, "\033[33mPEE\033[0m", 0.01, 0.2, 0.0, true, 0.0, true},
    // Brown for poo detection (ANSI escape code colors, for terminal)
    {POO_SENSOR_PIN, "\033[38;5;130mPOO\033[0m", 0.01, 0.2, 0.0, true, 0.0, true}};

// Tag for logging (blue text, ANSI escape code)
static const char *TAG = "\033[34mPooAway\033[0m";

// Function declaration before loop()
void playTone(int frequency, int duration);
void setSensorAlerts(SensorType sensor, bool enabled);

// Alert limiting constants (5 alerts max, within 5 minutes)
const unsigned int MAX_ALERTS = 5;
// Alert cooldown period (5 minutes)
const unsigned long ALERT_COOLDOWN_MS = 5 * 60 * 1000; // 5 minutes in milliseconds
// Alert counter and last alert time
unsigned int alertCount = 0;
unsigned long lastAlertTime = 0;

/**
 * @brief Setup function to initialize the sensors and pins
 */
void setup()
{
    pinMode(LED_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    ESP_LOGI(TAG, "Pee&Poo sensor starting up...");

    setSensorAlerts(POO, false); // Disable alerts for poo sensor until it has been burned in (24 to 168 hours)
}

/**
 * @brief Read the sensor value
 * @param sensor The sensor data struct
 * @return The sensor value
 */
float readSensor(SensorData &sensor)
{
    float sensorValue = analogRead(sensor.pin) * (VCC / ADC_RESOLUTION);
    return sensorValue;
}

/**
 * @brief Update the EMA (Exponential Moving Average) for the sensor
 * @param sensor The sensor data struct
 */
void updateEMA(SensorData &sensor)
{
    // If it's the first reading, set the baseline to the current value
    if (sensor.firstReading)
    {
        sensor.baselineEMA = sensor.value;
        sensor.firstReading = false;
    }
    else
    {
        // Update the EMA using the alpha value
        sensor.baselineEMA = (sensor.alpha * sensor.value) + ((1 - sensor.alpha) * sensor.baselineEMA);
    }
}

/**
 * @brief Check if the sensor value exceeds the threshold
 * @param sensor The sensor data struct
 * @return True if the sensor value exceeds the threshold, false otherwise
 */
bool checkThreshold(SensorData &sensor)
{
    // Calculate the threshold as the baseline EMA plus the tolerance
    float threshold = sensor.baselineEMA + sensor.tolerance;
    // Return true if the sensor value exceeds the threshold
    return sensor.value > threshold;
}

/**
 * @brief Handle alerts based on sensor values
 * @param alerts Array of boolean flags indicating sensor alerts
 */
void handleAlerts(const bool alerts[SENSOR_COUNT])
{
    // Check if we're in cooldown period
    if (alertCount >= MAX_ALERTS && (millis() - lastAlertTime) < ALERT_COOLDOWN_MS)
    {
        return;
    }

    // Reset alert count if cooldown period has passed
    if (alertCount >= MAX_ALERTS && (millis() - lastAlertTime) >= ALERT_COOLDOWN_MS)
    {
        alertCount = 0;
    }

    bool anyAlert = false;
    const char *alertTypes[SENSOR_COUNT] = {nullptr};
    int alertCount = 0;

    // Count active alerts, but only for enabled sensors
    for (int i = 0; i < SENSOR_COUNT && i < sizeof(sensors) / sizeof(sensors[0]); i++)
    {
        if (alerts[i] && sensors[i].alertsEnabled)
        {
            anyAlert = true;
            if (alertCount < SENSOR_COUNT)
            {
                alertTypes[alertCount++] = sensors[i].name;
            }
        }
    }

    if (anyAlert)
    {
        alertCount++;
        lastAlertTime = millis();

        char message[128] = "Alert!! ";
        for (int i = 0; i < alertCount; i++)
        {
            if (strlen(message) + strlen(alertTypes[i]) + 2 < sizeof(message))
            {
                strcat(message, alertTypes[i]);
                strcat(message, " ");
            }
        }
        if (strlen(message) + 10 < sizeof(message))
        {
            strcat(message, "detected!");
        }

        // Buzzer phases
        for (int i = 0; i < 2; i++)
        {
            // Phase 1: Human Alert - intense pattern
            if (alertCount == 1)
            {
                // Only one sensor triggered
                for (int k = 0; k < 5; k++)
                {
                    digitalWrite(BUZZER_PIN, HIGH);
                    delay(500);
                    digitalWrite(BUZZER_PIN, LOW);
                    delay(200);
                }
            }
            else
            {
                // Two or more sensors triggered, more noise!
                for (int j = 0; j < 4; j++)
                {
                    digitalWrite(BUZZER_PIN, HIGH);
                    delay(200);
                    digitalWrite(BUZZER_PIN, LOW);
                    delay(100);
                }
            }

            delay(500); // Pause between phases

            // Phase 2: Pet Deterrent
            playTone(25000, 2000); // 2 seconds of high-frequency sound

            ESP_LOGI(TAG, "%s", message);
            digitalWrite(LED_PIN, HIGH);
        }
    }
    digitalWrite(LED_PIN, LOW);
}

/**
 * @brief Play a tone on the buzzer
 * @param frequency The frequency of the tone
 * @param duration The duration of the tone
 */
void playTone(int frequency, int duration)
{
    int period = 1000000 / frequency; // Period in microseconds
    int halfPeriod = period / 2;      // Half-period for HIGH/LOW
    unsigned long endTime = millis() + duration;

    while (millis() < endTime)
    {
        digitalWrite(BUZZER_PIN, HIGH);
        delayMicroseconds(halfPeriod);
        digitalWrite(BUZZER_PIN, LOW);
        delayMicroseconds(halfPeriod);
    }
}

/**
 * @brief Print sensor data
 */
void printSensorData()
{
    char buffer[256] = "Sensors: ";
    char temp[64];

    for (int i = 0; i < SENSOR_COUNT; i++)
    {
        float threshold = sensors[i].baselineEMA + sensors[i].tolerance;
        bool isOverThreshold = sensors[i].value > threshold;

        snprintf(temp, sizeof(temp),
                 "%s [Val: %s%.3f\033[0m/Th: \033[35m%.3f\033[0m] ", // Threshold in magenta
                 sensors[i].name,
                 isOverThreshold ? "\033[31m" : "\033[32m", // Red if over threshold, Green if under
                 sensors[i].value,
                 sensors[i].baselineEMA + sensors[i].tolerance);

        strcat(buffer, temp);
    }

    ESP_LOGI(TAG, "%s", buffer);
}

/**
 * @brief Main loop to read sensor values, update EMA, check thresholds, and handle alerts
 */
void loop()
{
    bool alerts[SENSOR_COUNT];

    // Process all sensors
    for (int i = 0; i < SENSOR_COUNT; i++)
    {
        sensors[i].value = readSensor(sensors[i]);
        updateEMA(sensors[i]);
        alerts[i] = checkThreshold(sensors[i]);
    }

    handleAlerts(alerts);
    printSensorData();

    delay(100);
}

// Enable/disable alerts for a sensor
// TODO: Handle via BLE
void setSensorAlerts(SensorType sensor, bool enabled)
{
    if (sensor < SENSOR_COUNT)
    {
        sensors[sensor].alertsEnabled = enabled;
        ESP_LOGI(TAG, "Alerts for %s sensor: %s",
                 sensors[sensor].name,
                 enabled ? "enabled" : "disabled");
    }
}
