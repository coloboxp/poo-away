# 🐾 PooAway - Smart Pet Waste Detection System

## Overview

PooAway is an ESP32-based intelligent pet waste detection system using dual gas sensors. Built on the DFRobot Beetle ESP32-C6 board, it provides real-time monitoring and alerts for both liquid waste (NH3/Ammonia) and solid waste (CH4/Methane).

## 🔧 Hardware Requirements

- DFRobot Beetle ESP32-C6 Board
- Gas Sensors:
  - NH3 Sensor (MQ137) for urine detection (PIN 4)
  - CH4 Sensor (GM-402B) for solid waste detection (PIN 5)
- Status LED (PIN 15)
- Buzzer (PIN 6)
- Calibration Button (PIN 7)
- Power: 3.3V via USB-C or LiPo battery

## ⚡ Core Features

### Sensor System

- Dual independent gas sensors with real-time monitoring
- Exponential Moving Average (EMA) filtering for stable readings
  - NH3 Sensor: α=0.1 (slower response)
  - CH4 Sensor: α=0.05 (faster response)
- Automatic baseline tracking
- Configurable detection thresholds
  - NH3: 30% deviation tolerance
  - CH4: 40% deviation tolerance
- Minimum detection times to prevent false positives
  - NH3: 5000ms
  - CH4: 3000ms

### Alert System

- Multi-channel notifications with rate limiting:
  - LED indicators (500ms rate limit)
  - Frequency-based buzzer alerts (1s rate limit)
  - MQTT publishing (5s rate limit)
  - REST API endpoints (30s rate limit)

### Calibration

- One-button calibration system
- Clean air baseline establishment
- Automatic R0 resistance calculation
- Persistent calibration storage
- Pre-heating cycle management (180s)

### Power Management

- Low power mode support
- Sensor power state management
- Configurable warm-up cycles

## 🛠️ Technical Details

### Sensor Specifications

#### NH3 Sensor (MQ137)

- Operating voltage: 3.3V
- Load resistance: 47kΩ
- Detection range: 0-500 PPM
- Calibration coefficients: a=102.2, b=-2.473
- Valid R0 range: 1kΩ-100kΩ

#### CH4 Sensor (GM-402B)

- Operating voltage: 3.3V
- Load resistance: 4.7kΩ
- Detection range: 0-10000 PPM
- Calibration coefficients: a=26.572, b=1.2894
- Valid R0 range: 1kΩ-100kΩ

## 📝 Configuration

System configuration via `config.h`:

- Hardware pin assignments
- Network credentials
- MQTT broker settings
- Alert rate limits
- Detection thresholds

Private settings via `private.h` (gitignored):

- WiFi credentials
- MQTT authentication
- API keys and endpoints

## 🔍 Debugging

- ESP logging system integration
- Real-time sensor diagnostics
- Alert handler status monitoring
- Calibration verification tools

## 🛠️ Installation

1. Clone the repository
2. Copy `private.h.example` to `private.h` and configure credentials
3. Build using PlatformIO
4. Upload to ESP32-C6

## 📫 Usage

1. Power up the device
2. Allow 180-second initial sensor warm-up
3. Press calibration button in clean air if needed
4. System will automatically monitor and alert

## 🔧 Maintenance

- Periodic clean air calibration recommended
- Monitor sensor diagnostics
- Verify alert system connectivity
- Check baseline readings stability

## 📊 Monitoring

Available monitoring channels:

- Serial output (115200 baud)
- MQTT topics
- REST API endpoints
- LED status indicators

## 🤝 Contributing

Contributions welcome! Please check issues or submit PRs.

## 📄 License

MIT License - See LICENSE file

## 👥 Authors

- @coloboxp
