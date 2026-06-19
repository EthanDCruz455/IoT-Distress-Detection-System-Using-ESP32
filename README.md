# VitalGuard — IoT Patient Distress Detection System (ESP32)

Real-time vitals monitoring and automated emergency alerting using an ESP32, biometric sensors, Firebase Realtime Database, and Twilio SMS.

> *"Distress Detection System using ESP32" works too — but VitalGuard is the name I'd give it if you want something more memorable for a portfolio, paper, or app store listing. Easy to rename — see [Project Name](#project-name) below.*

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

Board support: install **ESP32 by Espressif Systems** via Boards Manager.

## Setup

1. **Clone the repo**
   ```bash
   git clone https://github.com/<your-username>/distress-detection-esp32.git
   cd distress-detection-esp32
   ```

2. **Configure credentials**
   ```bash
   cp src/secrets.h.example src/secrets.h
   ```
   Edit `src/secrets.h` and fill in your own:
   - WiFi SSID/password
   - Firebase API key & Realtime Database URL
   - Twilio Account SID, Auth Token, sender number, and recipient numbers (user + doctor)

   `secrets.h` is git-ignored — it will never be committed.

3. **Get a Firebase project**
   - Create a project at [firebase.google.com](https://firebase.google.com/)
   - Enable **Realtime Database** (start in test mode for development, then lock down rules for production)
   - Enable **Anonymous Authentication** under Authentication → Sign-in method

4. **Get Twilio credentials**
   - Sign up at [twilio.com](https://www.twilio.com/)
   - Grab your Account SID and Auth Token from the console
   - Buy/use a Twilio phone number as the `FROM` number

5. **Open in Arduino IDE**
   - Open `src/distress_detection_system.ino`
   - Select board: **ESP32 Dev Module**
   - Select the correct COM port
   - Upload

6. **Monitor**
   - Open Serial Monitor at `115200` baud to view live vitals, state transitions, and SMS/Firebase logs

## Project Structure

```
distress-detection-esp32/
├── src/
│   ├── distress_detection_system.ino   # Main firmware
│   ├── secrets.h.example               # Credential template (copy → secrets.h)
│   └── secrets.h                       # Your real credentials (git-ignored, not in repo)
├── docs/                               # Wiring diagrams / circuit notes (optional, add your own)
├── .gitignore
└── README.md
```

## Security Notes

- **Never commit `secrets.h`.** It's already in `.gitignore`, but double-check before every push.
- Treat your Twilio Auth Token like a password — anyone with it can send SMS and incur charges on your account. Rotate it immediately if it's ever exposed.
- Lock down Firebase Realtime Database rules before any real-world/production use; test-mode rules are open to anyone with your database URL.
- This is a prototype/educational project, **not a certified medical device**. Do not rely on it as a sole means of emergency detection in a real clinical setting.

## Project Name

Feel free to rename this however fits your use case — a few options:
- **VitalGuard** — IoT Patient Distress Detection System
- **PulseAlert** — ESP32 Wearable Distress Monitor
- **SentinelVitals** — Real-Time Health Distress Detection & Alert System
- Or keep it simple: **Distress Detection System using ESP32**

## License

Add a license of your choice (MIT is a common permissive default for hardware/firmware projects like this).
