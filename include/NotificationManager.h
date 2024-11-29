#pragma once
#include <Arduino.h>
#include <vector>
#include "esp_log.h"

// Forward declaration
class NotificationDevice;

class NotificationManager {
private:
    std::vector<NotificationDevice*> devices;
    static const char* TAG;

public:
    ~NotificationManager();
    void addDevice(NotificationDevice* device);
    void notify(const std::vector<const char*>& alertTypes);
    void reset();
};

// Abstract base class for notification devices
class NotificationDevice {
public:
    virtual ~NotificationDevice() = default;
    virtual void notify(const std::vector<const char*>& alertTypes) = 0;
    virtual void reset() = 0;
};

// Concrete notification devices
class LED : public NotificationDevice {
private:
    const int pin;
public:
    LED(int pin);
    void notify(const std::vector<const char*>& alertTypes) override;
    void reset() override;
};

class Buzzer : public NotificationDevice {
private:
    const int pin;
public:
    Buzzer(int pin);
    void notify(const std::vector<const char*>& alertTypes) override;
    void reset() override;
};

class BluetoothNotifier : public NotificationDevice {
public:
    void notify(const std::vector<const char*>& alertTypes) override;
    void reset() override;
};

class WiFiNotifier : public NotificationDevice {
private:
    const char* serverUrl;
public:
    WiFiNotifier(const char* url);
    void notify(const std::vector<const char*>& alertTypes) override;
    void reset() override;
}; 