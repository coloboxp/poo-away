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
const int BUZZER_PIN = 16;
const int PEE_SENSOR_PIN = 5;
const int POO_SENSOR_PIN = 6;
const float VCC = 3.3;
const int ADC_RESOLUTION = 4095;

// Add enum for sensor types
enum SensorType
{
    PEE,         // Pee sensor
    POO,         // Poo sensor
    SENSOR_COUNT // Total number of sensors (must be last)
};

struct SensorData
{
    const int pin;         // Pin number where the sensor is connected
    const char *name;      // Name for better messaging
    const float alpha;     // Response speed
    const float tolerance; // Sensitivity
    float baselineEMA;     // Baseline EMA
    bool firstReading;     // First reading flag
    float value;           // Sensor value
};

// Sensor configurations with different sensitivity/response combinations
// Format: {pin, name, alpha (response speed), tolerance (sensitivity), baseline, firstReading, value}
SensorData sensors[SENSOR_COUNT] = {
    // Very fast response, low tolerance - Quick alerts but may have false positives
    //{PEE_SENSOR_PIN, "PEE", 0.1, 0.1, 0.0, true, 0.0},

    // Fast response, medium tolerance - Good balance for pee detection
    {PEE_SENSOR_PIN, "PEE", 0.01, 0.2, 0.0, true, 0.0},

    // Medium response, high tolerance - More stable, fewer false positives
    //{PEE_SENSOR_PIN, "PEE", 0.005, 0.3, 0.0, true, 0.0},

    // Slow response, very high tolerance - Most stable but slower alerts
    //{POO_SENSOR_PIN, "POO", 0.001, 0.4, 0.0, true, 0.0},

    // Medium-fast response, medium tolerance - Balanced detection
    {POO_SENSOR_PIN, "POO", 0.01, 0.3, 0.0, true, 0.0},

    // Very slow response, extreme tolerance - Minimal false positives
    //{POO_SENSOR_PIN, "POO", 0.0005, 0.5, 0.0, true, 0.0}
};

// Add tag for logging
static const char *TAG = "PooAway";

/**
 * @brief Setup function to initialize the sensors and pins
 */
void setup()
{
    pinMode(LED_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    ESP_LOGI(TAG, "Pee&Poo sensor starting up...");
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
    bool anyAlert = false;
    const char *alertTypes[SENSOR_COUNT] = {nullptr};
    int alertCount = 0;

    for (int i = 0; i < SENSOR_COUNT; i++)
    {
        if (alerts[i])
        {
            anyAlert = true;
            alertTypes[alertCount++] = sensors[i].name;
        }
    }

    if (anyAlert)
    {
        // Build alert message
        char message[64] = "Alert!! ";
        for (int i = 0; i < alertCount; i++)
        {
            strcat(message, alertTypes[i]);
            strcat(message, " ");
        }
        strcat(message, "detected!");

        // Play buzzer with intensity based on number of alerts
        if (alertCount == 1)
        {
            // Single alert - normal warning tone
            digitalWrite(BUZZER_PIN, HIGH);
            delay(500);
            digitalWrite(BUZZER_PIN, LOW);
            delay(200);
            digitalWrite(BUZZER_PIN, HIGH);
            delay(500);
            digitalWrite(BUZZER_PIN, LOW);
        }
        else if (alertCount >= 2)
        {
            // Multiple alerts - urgent warning pattern
            for (int i = 0; i < 4; i++)
            {
                digitalWrite(BUZZER_PIN, HIGH);
                delay(200);
                digitalWrite(BUZZER_PIN, LOW);
                delay(100);
            }
        }

        ESP_LOGI(TAG, "%s", message);
        digitalWrite(LED_PIN, HIGH);
    }
    else
    {
        digitalWrite(LED_PIN, LOW);
    }
}

/**
 * @brief Print sensor data
 */
void printSensorData()
{
    char buffer[128];
    for (int i = 0; i < SENSOR_COUNT; i++)
    {
        snprintf(buffer, sizeof(buffer),
                 "%s - Value: %.3f | EMA: %.3f | Threshold: %.3f",
                 sensors[i].name,
                 sensors[i].value,
                 sensors[i].baselineEMA,
                 sensors[i].baselineEMA + sensors[i].tolerance);

        ESP_LOGI(TAG, "%s", buffer);
    }
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
