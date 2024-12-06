/**
 * @file main.cpp
 * @brief Main program for the PooAway sensor
 *
 * @author: @coloboxp
 * @date: 2024-11-28
 */

#include <Arduino.h>
#include "esp_log.h"
#include <Preferences.h>

Preferences preferences;

// Pin and voltage definitions
const int LED_PIN = 15;
const int PEE_SENSOR_PIN = 4; // NH3 sensor
const int POO_SENSOR_PIN = 5; // CH4 sensor
const int BUZZER_PIN = 6;
const float VCC = 5.0;           // Supply voltage
const int ADC_RESOLUTION = 4095; // 12-bit ADC
const float RL = 4700.0f;        // Load resistor (4.7kΩ) for both sensors

// Add debug flag
#define DEBUG_SENSORS true

// Add pin for calibration button
const int CALIBRATION_BTN_PIN = 7;       // Choose an available GPIO pin
const int CALIBRATION_LED_PIN = LED_PIN; // Built-in LED to show calibration status

// Sensor types enum
enum SensorType
{
    PEE,         // NH3 sensor
    POO,         // CH4 sensor
    SENSOR_COUNT // Total number of sensors
};

TaskHandle_t calibrationTasks[SensorType::SENSOR_COUNT] = {NULL};

// Calibration data structure
struct SensorCalibration
{
    float r0;                   // Base resistance in clean air
    const int numReadingsForR0; // Number of readings to establish R0
    const float preheatingTime; // Time in seconds needed for preheating
    const float a;              // Exponential coefficient a in: ppm = a * e^(b * rs_r0)
    const float b;              // Exponential coefficient b
};

// Sensor data structure
struct SensorData
{
    const int pin;             // ADC pin number
    const char *model;         // Sensor model
    const char *name;          // Sensor name for logging
    const float alpha;         // EMA filter coefficient
    const float tolerance;     // Threshold for alerts
    float baselineEMA;         // Baseline for comparison
    bool firstReading;         // First reading flag
    float value;               // Current sensor value
    bool alertsEnabled;        // Alert state
    SensorCalibration cal;     // Calibration data
    const int minDetectMs;     // Minimum detection time before alert
    unsigned long detectStart; // Detection start timestamp
};

// Initialize sensors
SensorData sensors[SENSOR_COUNT] = {
    // NH3 sensor (GM-802B)
    {
        .pin = PEE_SENSOR_PIN,
        .model = "GM-802B",
        .name = "\033[33mPEE\033[0m",
        .alpha = 0.01,
        .tolerance = 0.2,
        .baselineEMA = 1.0,
        .firstReading = true,
        .value = 1.0,
        .alertsEnabled = true,
        .cal = {
            .r0 = 1.0,
            .numReadingsForR0 = 100,
            .preheatingTime = 180.0,
            .a = 10.938, // NH3 specific coefficient
            .b = 1.7742  // NH3 specific coefficient
        },
        .minDetectMs = 5000,
        .detectStart = 0},
    // CH4 sensor (GM-402B)
    {.pin = POO_SENSOR_PIN, .model = "GM-402B", .name = "\033[38;5;130mPOO\033[0m", .alpha = 0.005, .tolerance = 0.3, .baselineEMA = 1.0, .firstReading = true, .value = 1.0, .alertsEnabled = false, .cal = {
                                                                                                                                                                                                          .r0 = 1.0, .numReadingsForR0 = 100, .preheatingTime = 180.0,
                                                                                                                                                                                                          .a = 26.572, // CH4 specific coefficient
                                                                                                                                                                                                          .b = 1.2894  // CH4 specific coefficient
                                                                                                                                                                                                      },
     .minDetectMs = 5000,
     .detectStart = 0}};

static const char *TAG = "\033[34mPooAway\033[0m";

// Function declarations
void playTone(int frequency, int duration);
void setSensorAlerts(SensorType sensor, bool enabled);
void calibrateSensor(SensorData &sensor);

// Task handle array to track calibration tasks

/**
 * @brief Convert sensor reading to PPM
 */
float convertToPPM(const SensorData &sensor, float rs_r0) {
    // Using the formula: PPM = a * e^(b * rs_r0)
    return sensor.cal.a * exp(sensor.cal.b * rs_r0);
}

/**
 * @brief Read sensor value and calculate Rs/R0 ratio
 */
float readSensor(SensorData &sensor) {
    float adc = analogRead(sensor.pin);
    float vout = adc * (VCC / ADC_RESOLUTION);
    
    // Calculate Rs using voltage divider formula
    float rs = vout > 0.0f ? RL * ((VCC / vout) - 1.0f) : sensor.value;
    
    // Calculate Rs/R0 ratio
    float rs_r0 = sensor.cal.r0 > 0.0f ? rs / sensor.cal.r0 : 1.0f;
    
    // Convert to PPM
    float ppm = convertToPPM(sensor, rs_r0);
    
    if (DEBUG_SENSORS) {
        ESP_LOGI(TAG, "%s: ADC=%d, Vout=%.3f, Rs=%.3f, Rs/R0=%.3f, PPM=%.1f",
                 sensor.name, (int)adc, vout, rs, rs_r0, ppm);
    }
    
    return ppm;
}

/**
 * @brief Update EMA filter for sensor
 */
void updateEMA(SensorData &sensor)
{
    float newValue = readSensor(sensor);

    if (sensor.firstReading)
    {
        sensor.baselineEMA = newValue;
        sensor.firstReading = false;
    }
    else
    {
        sensor.baselineEMA = (sensor.alpha * newValue) +
                             ((1.0f - sensor.alpha) * sensor.baselineEMA);
    }
}

/**
 * @brief Check if sensor reading exceeds threshold
 */
bool checkThreshold(SensorData &sensor)
{
    if (!sensor.alertsEnabled)
    {
        return false;
    }

    float ratio = sensor.value / sensor.baselineEMA;
    bool isTriggered = ratio < (1.0f - sensor.tolerance); // Note: < because lower ratio means more gas

    // Start or reset detection timer
    if (isTriggered)
    {
        if (sensor.detectStart == 0)
        {
            sensor.detectStart = millis();
        }
        // Check if enough time has passed
        return (millis() - sensor.detectStart) >= sensor.minDetectMs;
    }
    else
    {
        sensor.detectStart = 0; // Reset timer when not triggered
        return false;
    }
}

/**
 * @brief Handle alerts for triggered sensors
 */
void handleAlerts(bool alerts[])
{
    static bool alerting = false;
    bool shouldAlert = false;

    for (int i = 0; i < SENSOR_COUNT; i++)
    {
        if (alerts[i])
        {
            shouldAlert = true;
            break;
        }
    }

    if (shouldAlert && !alerting)
    {
        digitalWrite(LED_PIN, HIGH);
        playTone(1000, 100);
        alerting = true;
    }
    else if (!shouldAlert && alerting)
    {
        digitalWrite(LED_PIN, LOW);
        alerting = false;
    }
}

/**
 * @brief Play tone on buzzer
 */
void playTone(int frequency, int duration)
{
    tone(BUZZER_PIN, frequency, duration);
}

/**
 * @brief Print sensor data to serial
 */
void printSensorData()
{
    for (int i = 0; i < SENSOR_COUNT; i++)
    {
        ESP_LOGI(TAG, "%s: Rs/R0=%.3f (R0=%.1f) Threshold=%.3f %s",
                 sensors[i].name,
                 sensors[i].value,
                 sensors[i].cal.r0,
                 sensors[i].tolerance,
                 sensors[i].alertsEnabled ? "ALERTS ON" : "alerts off");
    }
}

/**
 * @brief Calibration task for a single sensor
 */
void calibrationTask(void *pvParameters)
{
    SensorData *sensor = (SensorData *)pvParameters;

    ESP_LOGI(TAG, "Starting calibration for %s sensor...", sensor->name);
    pinMode(sensor->pin, INPUT);

    // Preheat period
    ESP_LOGI(TAG, "Preheating %s sensor for %.0f seconds...",
             sensor->name, sensor->cal.preheatingTime);

    unsigned long startTime = millis();
    while (millis() - startTime < (sensor->cal.preheatingTime * 1000))
    {
        if ((millis() - startTime) % 30000 == 0)
        {
            // Read current values during preheat for debugging
            float vout = analogRead(sensor->pin) * (VCC / ADC_RESOLUTION);
            float rs = RL * ((VCC / vout) - 1.0f);
            ESP_LOGI(TAG, "%s preheating: %.0f sec remaining, Rs=%.3f",
                     sensor->name,
                     sensor->cal.preheatingTime - ((millis() - startTime) / 1000.0),
                     rs);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    // Take baseline readings with better error checking
    float sum = 0;
    int validReadings = 0;
    const float MAX_VALID_R0 = 100000.0f; // Maximum reasonable R0 value

    for (int i = 0; i < sensor->cal.numReadingsForR0; i++)
    {
        float vout = analogRead(sensor->pin) * (VCC / ADC_RESOLUTION);
        if (vout > 0.1f && vout < VCC)
        { // Validate voltage reading
            float rs = RL * ((VCC / vout) - 1.0f);
            if (rs > 0.0f && rs < MAX_VALID_R0)
            {
                sum += rs;
                validReadings++;
                if (DEBUG_SENSORS)
                {
                    ESP_LOGI(TAG, "%s calibration reading %d: Rs=%.3f",
                             sensor->name, i, rs);
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    if (validReadings > 0)
    {
        sensor->cal.r0 = sum / validReadings;
        sensor->baselineEMA = 1.0f; // Start with neutral ratio
        ESP_LOGI(TAG, "%s sensor calibrated. R0: %.3f",
                 sensor->name, sensor->cal.r0);
    }
    else
    {
        ESP_LOGE(TAG, "Calibration failed for %s sensor!", sensor->name);
        sensor->cal.r0 = RL; // Use load resistor as fallback
    }

    vTaskDelete(NULL);
}

/**
 * @brief Add function to perform clean air calibration
 */
void performCleanAirCalibration()
{
    ESP_LOGI(TAG, "Starting clean air calibration...");
    digitalWrite(CALIBRATION_LED_PIN, HIGH);

    // Take multiple readings for each sensor
    for (int i = 0; i < SENSOR_COUNT; i++)
    {
        float sum = 0;
        int validReadings = 0;

        ESP_LOGI(TAG, "Calibrating %s sensor in clean air...", sensors[i].name);

        // Take 100 readings over 10 seconds
        for (int j = 0; j < 100; j++)
        {
            float vout = analogRead(sensors[i].pin) * (VCC / ADC_RESOLUTION);
            float rs = vout > 0.0f ? RL * ((VCC / vout) - 1.0f) : 0.0f;

            if (rs > 0.0f)
            {
                sum += rs;
                validReadings++;
            }
            delay(100); // 100ms between readings
        }

        if (validReadings > 0)
        {
            // Store new R0 value
            sensors[i].cal.r0 = sum / validReadings;
            ESP_LOGI(TAG, "%s new R0: %.3f", sensors[i].name, sensors[i].cal.r0);

            // Reset baseline EMA
            sensors[i].baselineEMA = 1.0f;
            sensors[i].firstReading = true;
        }
    }

    digitalWrite(CALIBRATION_LED_PIN, LOW);
    ESP_LOGI(TAG, "Clean air calibration complete!");
}

/**
 * @brief Setup function with parallel calibration
 */
void setup()
{
    Serial.begin(115200);
    delay(100); // Wait for serial to stabilize

    ESP_LOGI(TAG, "=== Setup Starting ===");

    // Initialize pins with explicit states
    digitalWrite(LED_PIN, LOW);
    digitalWrite(CALIBRATION_LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    pinMode(LED_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(CALIBRATION_LED_PIN, OUTPUT);
    
    // Initialize button with pullup and wait for stabilization
    pinMode(CALIBRATION_BTN_PIN, INPUT_PULLUP);
    delay(100);
    
    // Read and verify button state multiple times
    bool buttonState = HIGH;
    for(int i = 0; i < 5; i++) {
        buttonState &= digitalRead(CALIBRATION_BTN_PIN);
        delay(10);
    }
    ESP_LOGI(TAG, "Initial button state: %s", buttonState ? "HIGH" : "LOW");
    
    if (!buttonState) {
        ESP_LOGW(TAG, "Warning: Button reading as LOW at startup - check wiring!");
    }

    // Initial calibration
    ESP_LOGI(TAG, "Starting initial calibration...");
    digitalWrite(CALIBRATION_LED_PIN, HIGH);
    performCleanAirCalibration();
    digitalWrite(CALIBRATION_LED_PIN, LOW);

    setSensorAlerts(POO, false);
    ESP_LOGI(TAG, "=== Setup Complete ===");
}

/**
 * @brief Main loop
 */
void loop()
{
    static unsigned long lastPrint = 0;
    static unsigned long lastButtonCheck = 0;
    static bool lastButtonState = HIGH;
    static bool buttonPressed = false;
    static bool firstLoop = true;
    const unsigned long PRINT_INTERVAL = 1000;
    const unsigned long DEBOUNCE_DELAY = 50;
    bool alerts[SENSOR_COUNT];

    // First loop initialization
    if (firstLoop) {
        ESP_LOGI(TAG, "=== First Loop ===");
        firstLoop = false;
        lastButtonState = digitalRead(CALIBRATION_BTN_PIN);
        lastButtonCheck = millis();
        ESP_LOGI(TAG, "Button state initialized to: %s", lastButtonState ? "HIGH" : "LOW");
        if (!lastButtonState) {
            ESP_LOGW(TAG, "Warning: Button still LOW - possible wiring issue");
        }
        return;
    }

    // Button handling with extra validation
    if (millis() - lastButtonCheck >= DEBOUNCE_DELAY) {
        bool currentButtonState = digitalRead(CALIBRATION_BTN_PIN);
        
        // Only process state changes
        if (currentButtonState != lastButtonState) {
            lastButtonCheck = millis();
            
            // Extra debounce check
            delay(10);
            if (digitalRead(CALIBRATION_BTN_PIN) == currentButtonState) {
                lastButtonState = currentButtonState;
                
                if (currentButtonState == LOW && !buttonPressed) {
                    ESP_LOGI(TAG, "=== Button Press Detected ===");
                    buttonPressed = true;
                    ESP_LOGI(TAG, "Starting manual calibration...");
                    digitalWrite(CALIBRATION_LED_PIN, HIGH);
                    performCleanAirCalibration();
                    digitalWrite(CALIBRATION_LED_PIN, LOW);
                    ESP_LOGI(TAG, "Manual calibration complete");
                }
            }
        }
        
        if (currentButtonState == HIGH) {
            buttonPressed = false;
        }
    }

    // Process all sensors
    for (int i = 0; i < SENSOR_COUNT; i++)
    {
        sensors[i].value = readSensor(sensors[i]);
        updateEMA(sensors[i]);
        alerts[i] = checkThreshold(sensors[i]);
    }

    handleAlerts(alerts);

    if (millis() - lastPrint >= PRINT_INTERVAL)
    {
        printSensorData();
        lastPrint = millis();
    }

    delay(100);
}

/**
 * @brief Enable/disable alerts for a sensor
 */
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
