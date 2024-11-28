# 🐾 PooAway - Pet Waste Detection System

## Overview

PooAway is an ESP32-based smart detection system, implemented on the Seeed XIAO ESP32C6 board, that helps prevent our dog from engaging with its waste by providing immediate alerts.

Using dual moisture sensors and real-time monitoring, it detects both liquid (NH3 - Ammonia 💦) and solid waste (CH4 - Methane 💩), alerting through LED and buzzer notifications.

## 🔧 Hardware Requirements

- ESP32 Development Board
- 2x Sensors:
  - PEE Sensor (PIN 5)
  - POO Sensor (PIN 6)
- LED Indicator (PIN 15)
- Buzzer (PIN 16)
- Power Supply (3.3V)

## ⚡ Features

- Dual sensor detection (PEE/POO)
- Configurable sensitivity settings
- Real-time alerts:
  - Visual (LED)
  - Audio (Buzzer patterns)
- Smart baseline tracking using EMA (Exponential Moving Average)
- ESP logging for debugging
- Different alert patterns for:
  - Single detection: Two long beeps
  - Multiple detections: Four short beeps

## 🛠️ Installation

1. Clone this repository
2. Open in PlatformIO/Arduino IDE
3. Configure board settings
4. Upload to ESP32

## 📝 Usage

1. Power up the device
2. System auto-calibrates on first reading
3. Monitors continuously for waste detection
4. Alerts trigger automatically when detection occurs

## ⚙️ Configuration

Adjust sensor settings in `main.cpp`:

```cpp
struct SensorData {
    const int pin;         // Sensor pin
    const char name;       // Identifier
    const float alpha;     // Response speed (0-1)
    const float tolerance; // Sensitivity
    float baselineEMA;     // Baseline
    bool firstReading;     // First reading flag
    float value;           // Current value
};
```

### Sensitivity Presets

- **Fast response, medium tolerance**: Good balance for pee detection
- **Medium-fast response, medium tolerance**: Balanced detection for poo

## 🔍 Debugging

- Monitor serial output (115200 baud)
- ESP logging enabled
- Real-time sensor values and thresholds
- Alert status messages

## 📫 Support

For issues and feature requests, please open an issue on GitHub.

## 👥 Contributors

- @coloboxp

## 📄 License

MIT License

## About EMA

EMA is a type of filter that smooths out data by giving more weight to recent values. It is useful for removing noise from sensor readings and providing a more stable baseline.
