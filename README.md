# Cloud-Centric Distress Detection System

![Platform](https://img.shields.io/badge/platform-ESP32-blue)
![Language](https://img.shields.io/badge/language-C%2B%2B%20(Arduino)-orange)
![Cloud](https://img.shields.io/badge/cloud-Firebase-yellow)
![Alerts](https://img.shields.io/badge/alerts-Twilio%20SMS-red)
![Status](https://img.shields.io/badge/status-completed-brightgreen)

An IoT-based wearable/portable system that continuously monitors heart rate, SpO2, and body temperature, detects abnormal (distress) conditions using a state-based model, stores data on the cloud, and automatically sends SMS alerts to predefined contacts during emergencies.

Developed as part of the IoT Lab at N.M.A.M. Institute of Technology, Nitte — 6th Semester ECE, 2025–2026.

---

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [System Architecture](#system-architecture)
- [Working Principle](#working-principle)
- [Hardware Requirements](#hardware-requirements)
- [Software Requirements](#software-requirements)
- [Getting Started](#getting-started)
- [Results](#results)
- [Limitations & Future Work](#limitations--future-work)
- [Team](#team)
- [References](#references)
- [License](#license)

---

## Overview

Emergency situations — especially for elderly individuals or people with chronic health conditions — often go undetected in time because traditional monitoring relies on manual intervention. This project automates that process: an ESP32 microcontroller continuously acquires physiological data, evaluates it against predefined thresholds and a state machine, pushes readings to the cloud for remote monitoring, and triggers real-time SMS alerts the moment a distress condition is detected.

**Objectives:**
1. Continuously acquire heart rate, SpO2, and panic-button input via sensors interfaced with an ESP32.
2. Transmit raw sensor data to a cloud platform and perform storage, analytics, and distress detection at the cloud layer.
3. Implement cloud-controlled pre-alert and emergency notification mechanisms for timely assistance.

**Scope:** The system targets basic, real-time health monitoring for at-risk individuals (elderly, patients with medical conditions). Advanced features like ML-based prediction, GPS tracking, and a companion mobile app are considered future enhancements and are not part of this implementation.

---

## Features

- 📈 Real-time heart rate & SpO2 monitoring (MAX30102)
- 🌡️ Body temperature monitoring (DS18B20)
- 🆘 Manual panic/snooze button for user-triggered alerts
- 🧠 State-based distress detection (`NORMAL → PRE-ALERT → ALERT`) to minimize false alarms
- ☁️ Cloud data storage and remote monitoring via Firebase
- 📲 Automated SMS emergency alerts via Twilio API
- 🔄 Automatic recovery handling when vitals return to normal

---

## System Architecture

```
   ┌───────────────┐      ┌─────────────────┐      ┌────────────────┐
   │   Sensors     │      |      ESP32      │      │  Cloud Layer   │
   │ MAX30102      │─────>|  - Data read    │─────>│  Firebase DB   │
   │ DS18B20       │      │  - Wi-Fi comms  │      │  (storage +    │
   │ Panic Button  │─────>│  - Threshold /  │<─────│   monitoring)  │
   └───────────────┘      │    state logic  │      └────────────────┘
                          └────────┬────────┘
                                   │ Distress detected
                                   |
                                   ▼
                          ┌─────────────────┐
                          │   Twilio API    │
                          │  SMS Alert to   │
                          │ predefined      │
                          │ contacts        │
                          └─────────────────┘
```

*(Replace this ASCII diagram with `assets/block_diagram.png` from your report's Figure 3.1 once added to the repo.)*

---

## Working Principle

1. **Initialization** — The ESP32 initializes sensors, Wi-Fi connection, and cloud services.
2. **Data Acquisition** — Continuously collects heart rate, SpO2, and body temperature.
3. **Data Processing** — Analyzes readings using predefined thresholds and a state-based model.
4. **State Transition** — Moves between `NORMAL`, `PRE-ALERT`, and `ALERT` states to ensure reliable detection and reduce false alarms.
5. **Cloud Communication** — Sensor data is transmitted to Firebase for storage and remote monitoring.
6. **Alert Generation** — On a critical condition, the system sends an emergency SMS via Twilio to predefined contacts.
7. **Recovery Handling** — If vitals return to normal, the system resets to `NORMAL` and resumes monitoring.

---

## 🔧 Hardware Requirements

| Component | Purpose |
|---|---|
| ESP32 Development Board | Main microcontroller — data processing & communication |
| MAX30102 Sensor | Heart rate and SpO2 measurement |
| DS18B20 Temperature Sensor | Body temperature measurement |
| Push Button | Manual panic / snooze input |
| Connecting Wires | Circuit connections |
| Breadboard | Circuit assembly platform |
| Power Supply | USB or battery power |

## 🔌 Pin Connections
Pin Connection Reference — ESP32 to Sensors

MAX30102 (Heart Rate / SpO2 Sensor)
| MAX30102 Pin | Function        | ESP32 Pin |
|--------------|------------------|-----------|
| VIN          | Power (3.3V)     | 3V3       |
| GND          | Ground           | GND       |
| SDA          | I2C Data         | GPIO 21   |
| SCL          | I2C Clock        | GPIO 22   |
| INT          | Interrupt (opt.) | GPIO 4    |

DS18B20 (Temperature Sensor)
| DS18B20 Pin | Function              | ESP32 Pin |
|-------------|------------------------|-----------|
| VDD         | Power (3.3V)           | 3V3       |
| GND         | Ground                 | GND       |
| DQ          | 1-Wire Data            | GPIO 15   |
| —           | 4.7kΩ pull-up (DQ→VDD) | —         |

Push Button (Panic/Snooze)
| Button Pin | Function          | ESP32 Pin |
|------------|-------------------|-----------|
| Terminal 1 | Digital Input     | GPIO 5    |
| Terminal 2 | Ground            | GND       |

Notes:
- MAX30102 uses I2C — default address 0x57.
- DS18B20 requires a 4.7kΩ pull-up resistor between DQ and VDD.
- Push button can use ESP32's internal pull-up (INPUT_PULLUP) to avoid an external resistor.

## 💻 Software Requirements

| Tool | Purpose |
|---|---|
| Arduino IDE | Programming and uploading firmware to the ESP32 |
| Firebase (Cloud Database) | Real-time data storage and monitoring |
| Twilio API | Sending SMS alerts during emergencies |
| ESP32 Wi-Fi Libraries | Wireless communication between device and cloud |

---

## Getting Started

### Prerequisites
- Arduino IDE with the ESP32 board package installed
- A Firebase project (Realtime Database or Firestore) with credentials
- A Twilio account with an active phone number and API credentials

### Setup
1. Clone this repository:
   ```bash
   git clone https://github.com/<your-username>/cloud-centric-distress-detection.git
   cd cloud-centric-distress-detection
   ```
2. Open the firmware sketch in Arduino IDE (e.g. `firmware/distress_detection.ino`).
3. Install required libraries via the Library Manager:
   - `MAX30105`/`SparkFun MAX3010x` (for MAX30102)
   - `OneWire` and `DallasTemperature` (for DS18B20)
   - `Firebase_ESP_Client`
   - Wi-Fi / HTTPClient (bundled with ESP32 core)
4. Create a `config.h` (not committed to source control) with your credentials:
   ```cpp
   #define WIFI_SSID "your_wifi_ssid"
   #define WIFI_PASSWORD "your_wifi_password"
   #define FIREBASE_HOST "your_firebase_host"
   #define FIREBASE_AUTH "your_firebase_auth_key"
   #define TWILIO_SID "your_twilio_sid"
   #define TWILIO_AUTH_TOKEN "your_twilio_auth_token"
   #define TWILIO_FROM_NUMBER "+1XXXXXXXXXX"
   #define ALERT_TO_NUMBER "+91XXXXXXXXXX"
   ```
5. Wire the components per the pin mapping in `docs/wiring.md` (add this based on your build).
6. Flash the ESP32 and open the Serial Monitor at 115200 baud to view live readings and state transitions.

> ⚠️ Never commit real API keys, Wi-Fi credentials, or phone numbers. Add `config.h` to `.gitignore`.

---

## Results

The system was assembled and tested successfully:

- Real-time heart rate, SpO2, temperature, and system state were displayed on the serial monitor.
- Heart rate and temperature readings were stable across test runs.
- SpO2 values mostly ranged between 98–100%, reflecting the practical sensitivity limits of the MAX30102 in this setup.
- The state-based approach (`NORMAL → PRE-ALERT → ALERT`) reliably reduced false alarms by requiring abnormal conditions to persist before triggering an alert.
- SMS alerts were sent correctly and promptly upon distress detection, with minor delays attributable to sensor sampling and communication intervals.

*(Add your Figures 5.1–5.3 — project setup, message output, console output — to `assets/` and reference them here.)*

---

## Limitations & Future Work

- SpO2 sensor sensitivity is limited at the margins, reducing precision.
- Response time is affected by sensor sampling and delay functions.
- Planned enhancements: ML-based distress prediction, GPS-based location tagging in alerts, and a dedicated mobile app interface.

---

## Team

- Diya Surendra Shetty — NNM23EC059
- Ethan Dcruz — NNM23EC060
- G Devadarshan — NNM23EC061
- Gagan Naik — NNM23EC062

*N.M.A.M. Institute of Technology, Nitte — Electronics and Communication Engineering, 2025–2026*

---

## References

1. "IoT-Based Health Monitoring System," *International Journal of Engineering Research and Technology (IJERT)*, 2021.
2. Arduino IDE Documentation, Arduino, 2023. https://www.arduino.cc/en/software
3. "DS18B20 Programmable Resolution 1-Wire Digital Thermometer Datasheet," Maxim Integrated, 2018.


