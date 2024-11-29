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
#include "Sensor.h"
#include "NotificationManager.h"

// Create sensors
Sensor peeSensor(4, "\033[33mPEE\033[0m", 0.01, 0.2);
Sensor pooSensor(6, "\033[38;5;130mPOO\033[0m", 0.01, 0.3);

// Create notification manager
NotificationManager notificationManager;

void setup() {
    // Add notification devices
    notificationManager.addDevice(new LED(15));
    notificationManager.addDevice(new Buzzer(16));
    notificationManager.addDevice(new BluetoothNotifier());
    notificationManager.addDevice(new WiFiNotifier("http://your-server.com/api/alerts"));
}

void loop() {
    // Update sensor readings
    peeSensor.update();
    pooSensor.update();

    // Check for alerts
    std::vector<const char*> activeAlerts;
    if (peeSensor.isAlertTriggered()) {
        activeAlerts.push_back(peeSensor.getName());
    }
    if (pooSensor.isAlertTriggered()) {
        activeAlerts.push_back(pooSensor.getName());
    }

    // Handle notifications
    notificationManager.notify(activeAlerts);

    delay(100);
}
