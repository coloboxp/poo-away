#include <Arduino.h>

#define LED_PIN 15
#define SENSOR_PIN 5

float baselineEMA = 0.0;
float alpha = 0.01; // Smoothing factor for EMA, adjust between 0 and 1
bool firstReading = true;

void setup() {
    Serial.begin(115200);
    pinMode(LED_PIN, OUTPUT);
}

void loop() {
    float sensorValue = analogRead(SENSOR_PIN) * (3.3 / 4095.0);

    // Update EMA baseline
    if (firstReading) {
        baselineEMA = sensorValue; // Initialize EMA with first reading
        firstReading = false;
    } else {
        baselineEMA = (alpha * sensorValue) + ((1 - alpha) * baselineEMA);
    }

    // Define threshold as EMA baseline plus a tolerance
    float threshold = baselineEMA + 0.2; // Adjust tolerance as needed

    if (sensorValue > threshold) {
        // Trigger alert
        Serial.println("Alert! Sensor value exceeded threshold.");
        digitalWrite(LED_PIN, HIGH); // Turn on LED
    } else {
        // Reset alert
        digitalWrite(LED_PIN, LOW); // Turn off LED
    }

    Serial.print("Sensor Value: ");
    Serial.print(sensorValue);
    Serial.print(" | Baseline EMA: ");
    Serial.print(baselineEMA);
    Serial.print(" | Threshold: ");
    Serial.println(threshold);

    delay(100);
}