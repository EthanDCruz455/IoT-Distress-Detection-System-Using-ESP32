# VitalGuard — IoT Patient Distress Detection System (ESP32)

Real-time vitals monitoring and automated emergency alerting using an ESP32, biometric sensors, Firebase Realtime Database, and Twilio SMS.

## Overview

This project continuously monitors a patient's **heart rate**, **SpO2 (blood oxygen)**, and **body temperature**, and automatically triggers SMS alerts to a designated user and doctor if vitals cross critical thresholds — or instantly, if a manual panic button is pressed. All sensor data is streamed live to Firebase Realtime Database for remote dashboards/monitoring apps.

## Features

- **Continuous vitals monitoring** — heart rate & SpO2 via MAX30102, body temperature via DS18B20
- **Finger-presence detection** — avoids false readings when the sensor isn't in use
- **Three-stage state machine** — `NORMAL → PRE_ALERT → ALERT`, with a configurable grace period (default 20s) before an alert fires, reducing false positives from transient noise
- **Automatic recovery** — returns to `NORMAL` and re-arms alerting if vitals stabilize
- **Manual panic button** — interrupt-driven, fires an immediate SMS regardless of sensor/alert state
- **Dual SMS notification** — alerts sent to both patient/caregiver and doctor via Twilio
- **Live cloud sync** — all readings + system state pushed to Firebase RTDB every cycle
- **One-shot alert lock** — prevents SMS spam by sending only once per distress event

## Hardware

| Component | Purpose |
|---|---|
| ESP32 Dev Board | Main microcontroller, WiFi |
| MAX30102 | Heart rate + SpO2 (PPG) sensor |
| DS18B20 | Digital temperature sensor |
| Push button | Manual panic/SOS trigger |

### Wiring

| Signal | ESP32 Pin |
|---|---|
| DS18B20 Data | GPIO 4 |
| Panic Button | GPIO 13 (active LOW, internal pull-up) |
| MAX30102 | I2C (SDA/SCL default pins) |

> DS18B20 requires a 4.7kΩ pull-up resistor between data and 3.3V.

## How It Works

```
                 ┌──────────┐  distress sustained ≥20s   ┌────────┐
   vitals OK ──▶ │  NORMAL  │ ───────────────────────────▶│ ALERT  │──▶ SMS to User + Doctor
                 └──────────┘                              └────────┘
                       ▲          ┌────────────┐                │
                       │          │ PRE_ALERT  │◀───distress────┘ (recovers)
                       └──recover─┤  (20s grace)│
                                  └────────────┘
```

Distress is flagged when **any** of the following are true:
- Heart rate > 120 bpm or < 40 bpm
- SpO2 < 90%
- Temperature > 39.0°C

Thresholds and the grace period are configurable constants at the top of the sketch.

## Required Libraries (Arduino IDE)

Install via Library Manager:
- `Firebase ESP Client` (mobizt)
- `MAX30105lib` (SparkFun)
- `OneWire`
- `DallasTemperature`

Board support: install **ESP32 by Espressif Systems** via Boards Manager in Arduino IDE.


## Project Structure

```
distress-detection-esp32/
├── src/
│   ├── distress_detection_system.ino   # Main firmware
│   ├── secrets.h.example               # Credential template (copy → secrets.h)
│   └── secrets.h                       # Your real credentials (git-ignored, not in repo)
├── docs/                               # Wiring diagrams / circuit notes
├── .gitignore
└── README.md
```

## Security Notes

- **Never commit `secrets.h`.** It's already in `.gitignore`, but double-check before every push.
- Treat your Twilio Auth Token like a password — anyone with it can send SMS and incur charges on your account. Rotate it immediately if it's ever exposed.
- Lock down Firebase Realtime Database rules before any real-world/production use; test-mode rules are open to anyone with your database URL.
- This is a prototype/educational project, **not a certified medical device**. Do not rely on it as a sole means of emergency detection in a real clinical setting.

## Conclusion

This project demonstrates a practical, low-cost approach to remote patient monitoring by combining commodity biometric sensors with cloud connectivity and automated emergency alerting. The ESP32's WiFi capability, paired with Firebase for live data sync and Twilio for SMS notifications, makes it possible to bridge the gap between a patient's vitals and the people who need to respond — without requiring constant manual supervision.
The staged NORMAL → PRE_ALERT → ALERT state machine strikes a balance between responsiveness and reliability, filtering out transient sensor noise while still escalating quickly when distress is sustained. The independent panic-button interrupt ensures the patient always has a direct, immediate line to help, regardless of what the automated system is doing.
That said, this remains a prototype built for learning and experimentation, not a certified medical device. Real-world deployment would need additional work: sensor calibration and validation against clinical-grade equipment, more robust error handling and retry logic for network/SMS failures, encrypted credential storage, and rigorous testing across edge cases (e.g. intermittent WiFi, sensor disconnection mid-reading). Contributions, suggestions, and forks aimed at hardening the system for more serious use cases are welcome.

## References
1)Datasheets:
- MAX30102 Datasheet — Analog Devices / Maxim Integrated
- DS18B20 Datasheet — Analog Devices / Maxim Integrated
2)Libraries:
- SparkFun MAX3010x Sensor Library — Arduino driver for the MAX30102/MAX30105; also the source of the spo2_algorithm heart-rate/SpO2 calculation used in this  project
- Firebase-ESP-Client — Mobizt's Firebase Arduino client library for ESP32/ESP8266
- OneWire — Arduino library for the 1-Wire protocol used by the DS18B20
- DallasTemperature — Arduino library built on OneWire for DS18B20-family sensors
3)Platform / API Docs:
- Arduino-ESP32 Core — Espressif's Arduino core for ESP32
- Firebase Realtime Database Docs
- Twilio Programmable Messaging API — Send SMS
