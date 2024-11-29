#include "NotificationManager.h"

const char* NotificationManager::TAG = "\033[34mPooAway\033[0m";

NotificationManager::~NotificationManager() {
    for (auto device : devices) {
        delete device;
    }
}

void NotificationManager::addDevice(NotificationDevice* device) {
    devices.push_back(device);
}

void NotificationManager::notify(const std::vector<const char*>& alertTypes) {
    if (!alertTypes.empty()) {
        char message[128] = "Alert!! ";
        for (const auto& alertType : alertTypes) {
            if (strlen(message) + strlen(alertType) + 2 < sizeof(message)) {
                strcat(message, alertType);
                strcat(message, " ");
            }
        }
        
        if (strlen(message) + 10 < sizeof(message)) {
            strcat(message, "detected!");
        }
        
        ESP_LOGI(TAG, "%s", message);
        
        for (auto device : devices) {
            device->notify(alertTypes);
        }
    } else {
        reset();
    }
}

void NotificationManager::reset() {
    for (auto device : devices) {
        device->reset();
    }
}

// LED implementation
LED::LED(int pin) : pin(pin) {
    pinMode(pin, OUTPUT);
}

void LED::notify(const std::vector<const char*>& alertTypes) {
    digitalWrite(pin, HIGH);
}

void LED::reset() {
    digitalWrite(pin, LOW);
}

// Buzzer implementation
Buzzer::Buzzer(int pin) : pin(pin) {
    pinMode(pin, OUTPUT);
}

void Buzzer::notify(const std::vector<const char*>& alertTypes) {
    if (alertTypes.size() == 1) {
        // Single alert pattern
        digitalWrite(pin, HIGH);
        delay(500);
        digitalWrite(pin, LOW);
        delay(200);
        digitalWrite(pin, HIGH);
        delay(500);
        digitalWrite(pin, LOW);
    } else {
        // Multiple alerts pattern
        for (int i = 0; i < 4; i++) {
            digitalWrite(pin, HIGH);
            delay(200);
            digitalWrite(pin, LOW);
            delay(100);
        }
    }
}

void Buzzer::reset() {
    digitalWrite(pin, LOW);
}

// Bluetooth implementation
void BluetoothNotifier::notify(const std::vector<const char*>& alertTypes) {
    // Implement Bluetooth notification logic
    // This could use ESP32's Bluetooth functionality
}

void BluetoothNotifier::reset() {
    // Implement any necessary cleanup
}

// WiFi implementation
WiFiNotifier::WiFiNotifier(const char* url) : serverUrl(url) {}

void WiFiNotifier::notify(const std::vector<const char*>& alertTypes) {
    // Implement HTTP POST request to server
    // This could use ESP32's WiFi functionality
}

void WiFiNotifier::reset() {
    // Implement any necessary cleanup
} 