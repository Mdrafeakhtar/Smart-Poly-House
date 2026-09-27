# 🌱 Smart Poly House — ESP32 Agri-IoT Automation System

An ESP32-based **Smart Poly House** that combines environmental monitoring, crop-specific threshold profiles, automatic actuator control, an embedded web dashboard, LCD status display, LEDs/buzzer, and SIM800 GSM SMS alerts.

The system is designed to help monitor and automate a protected cultivation environment using temperature, humidity, soil, light, and rain conditions.

## ✨ Key Features

- 🌡️ **Temperature & humidity monitoring** using DHT11
- 🌱 **Soil condition monitoring**
- ☀️ **Light monitoring** using LDR
- 🌧️ **Rain detection**
- 🌀 **Automatic fan/relay control** based on crop thresholds
- 💡 **Automatic lighting control** based on ambient light
- 🎛️ **Manual fan and light control** from the web dashboard
- 🌾 **Crop + season profiles** for Tomato, Wheat, Cucumber, Potato, Apple, Mango, and Grapes
- 📱 **SIM800 GSM SMS alerts** for rain, soil, temperature, humidity, and system events
- 🔔 **Buzzer alerts** for selected abnormal conditions
- 🚦 **Three-status LED indication**: normal, warning, danger
- 📟 **16×2 I2C LCD** for local status and ESP32 IP address
- 🌐 **ESP32-hosted web interface** using `WebServer`
- 👤 **Login and user registration** in the embedded web interface
- 📊 **Live sensor data endpoint** for dashboard updates
- ⚙️ **Automatic/manual operating modes** for actuators

## 🧠 System Workflow

```text
Sensors
  │
  ├── DHT11 ─────────────┐
  ├── Soil Sensor ───────┤
  ├── LDR ────────────────┤
  └── Rain Sensor ────────┤
                          ▼
                       ESP32
                          │
          ┌───────────────┼────────────────┐
          ▼               ▼                ▼
    Web Dashboard      LCD + LEDs      GSM / SIM800
          │               │                │
          ▼               ▼                ▼
   Manual Control     Local Status      SMS Alerts
          │
          ▼
   Relay / Fan / Light
```

## 🔧 Hardware

| Component | Purpose | ESP32 Pin |
|---|---|---:|
| ESP32 | Main controller + Wi-Fi | — |
| DHT11 | Temperature & humidity | GPIO 4 |
| Soil sensor | Soil wet/dry state | GPIO 35 |
| LDR module | Light state | GPIO 27 |
| Rain sensor | Rain detection | GPIO 26 |
| Fan relay | Fan/ventilation control | GPIO 25 |
| Light relay | Poly-house lighting | GPIO 32 |
| Buzzer | Audible alert | GPIO 33 |
| Green LED | Normal state | GPIO 14 |
| Yellow LED | Warning state | GPIO 12 |
| Red LED | Danger state | GPIO 13 |
| 16×2 I2C LCD | Local display | SDA 21 / SCL 22 |
| SIM800 | SMS alerts | RX 16 / TX 17 |

> **Relay logic:** the sketch uses active-LOW relays (`LOW = ON`, `HIGH = OFF`). Verify your relay module before connecting mains-powered equipment.

## 🌾 Crop Profiles

The firmware includes configurable threshold profiles for:

- Tomato — Summer / Monsoon / Winter
- Wheat — Summer / Monsoon / Winter
- Cucumber — Summer / Monsoon / Winter
- Potato — Summer / Monsoon / Winter
- Apple — Summer / Monsoon / Winter
- Mango — Summer / Monsoon / Winter
- Grapes — Summer / Monsoon / Winter

Each profile contains temperature, humidity, and soil thresholds used by the automation logic.

## 🌐 Web Dashboard

The ESP32 hosts the interface locally over Wi-Fi. The firmware provides these routes:

| Route | Purpose |
|---|---|
| `/` | Login page |
| `/registerPage` | Registration page |
| `/dashboard` | Main dashboard |
| `/login` | Login endpoint |
| `/registerUser` | User registration endpoint |
| `/data` | Sensor JSON data |
| `/control` | Manual actuator control |
| `/config` | Crop/season configuration |

After the ESP32 connects to Wi-Fi, its local IP address is shown on the LCD and Serial Monitor. Open that IP address from a device on the same network.

## 📱 GSM SMS Alerts

The SIM800 module is used to send notifications for events such as:

- Smart Poly House startup
- New user registration
- Rain detection
- Soil becoming dry/wet
- Temperature threshold violation
- Humidity threshold violation
- Fan automatic/manual state changes
- Light automatic/manual state changes
- Crop/season session configuration

## 🛠️ Software Requirements

- Arduino IDE
- ESP32 board package for Arduino
- ESP32-compatible `WiFi` and `WebServer` libraries
- `DHT sensor library`
- `LiquidCrystal_I2C`
- SIM800-compatible GSM module

### Arduino libraries

Install these libraries through Arduino IDE's Library Manager where applicable:

```text
DHT sensor library
LiquidCrystal I2C
```

`WiFi.h` and `WebServer.h` are provided by the ESP32 Arduino core.

## 🚀 Setup

### 1. Clone the repository

```bash
git clone https://github.com/Mdrafeakhtar/Smart-Poly-House.git
cd Smart-Poly-House
```

### 2. Open the sketch

Open:

```text
Smart_Poly_House_Final.ino
```

in Arduino IDE.

### 3. Configure your credentials

Before uploading, replace these placeholders in the sketch:

```cpp
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* USERNAME = "Rafe";
const char* PASSWORD = "CHANGE_THIS_ADMIN_PASSWORD";
String ALERT_NUMBER = "YOUR_PHONE_NUMBER";
```

**Never commit your real Wi-Fi password, admin password, or phone number to a public repository.**

### 4. Select the ESP32 board

In Arduino IDE, select the ESP32 board that matches your hardware, connect the board over USB, choose the correct port, and upload the sketch.

### 5. Monitor startup

Open Serial Monitor at:

```text
115200 baud
```

The ESP32 prints its local IP address after connecting to Wi-Fi.

## 📟 LCD Startup Sequence

The LCD displays information such as:

1. Company/startup message
2. Smart Poly House starting status
3. Wi-Fi connection status
4. ESP32 IP address
5. Rotating temperature/humidity, soil/light, and rain status pages

## 🔐 Security Notes

This repository intentionally contains **placeholders instead of real credentials**.

For a production deployment, improve the authentication system by:

- Hashing passwords instead of storing plaintext passwords
- Persisting users securely instead of RAM-only storage
- Adding session tokens/cookies
- Adding request authentication/authorization
- Moving credentials into a protected configuration mechanism
- Avoiding SMS messages that expose unnecessary personal information
- Using appropriate electrical isolation and protection for relay-controlled loads

## ⚠️ Hardware Safety

This project can control relays connected to external electrical equipment. Do not connect mains voltage directly to the ESP32 or breadboard. Use properly rated relays, fuses, enclosures, isolation, and qualified electrical installation practices.

## 📁 Repository Structure

```text
Smart-Poly-House/
├── Smart_Poly_House_Final.ino
├── README.md
├── .gitignore
└── docs/
    └── screenshots/
```

## 📸 Demo

Add real project screenshots to:

```text
docs/screenshots/
```

Recommended screenshots:

- Dashboard
- Sensor readings
- Crop/season configuration
- Manual control
- LCD hardware setup
- SMS alert example

## 💼 Skills Demonstrated

This project demonstrates practical experience with:

- Embedded C/C++
- ESP32
- IoT system design
- Wi-Fi networking
- WebServer-based embedded dashboards
- Sensor interfacing
- I2C communication
- UART communication
- GSM/SIM800 integration
- Relay and actuator control
- Real-time monitoring
- Rule-based automation
- Embedded web UI development
- Hardware/software integration
- Debugging and system testing

## 🔮 Future Improvements

- Store user accounts in non-volatile memory or a secure backend
- Add MQTT/cloud monitoring
- Add historical sensor graphs
- Add mobile-responsive PWA/app support
- Add OTA firmware updates
- Add more crop profiles
- Add soil-moisture percentage using an analog capacitive sensor
- Add water-pump automation
- Add data logging and downloadable reports
- Add secure authentication with hashed credentials

## 👨‍💻 Author

**Md Rafe Akhtar**

Embedded Systems • IoT • UAV/Drone Systems • Electronics & Communication Engineering

---

⭐ If this project is useful for your learning, consider starring the repository.
