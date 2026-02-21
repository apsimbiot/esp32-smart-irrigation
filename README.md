# 🌱 ESP32 Smart Drip Irrigation System

Remote-controlled dual-pump irrigation system with MQTT, scheduling, and web dashboard.

## Features

- **Dual Pump Control** — Independent control of 2 pumps with different schedules
- **MQTT Remote Access** — Control from anywhere via HiveMQ Cloud broker
- **Web Dashboard** — Real-time status, manual controls, and schedule management
- **Flexible Scheduling** — Daily watering or every-N-days for different plants
- **TLS Security** — Encrypted MQTT communication

## Architecture

```
┌─────────────────┐     MQTT/TLS      ┌──────────────────┐
│  Web Dashboard  │◄────────────────►│  HiveMQ Cloud    │
│  (Browser)      │                   │  (MQTT Broker)   │
└─────────────────┘                   └────────┬─────────┘
                                               │
                                          MQTT/TLS
                                               │
                                      ┌────────▼─────────┐
                                      │     ESP32        │
                                      │  (WiFi + MQTT)   │
                                      └────────┬─────────┘
                                               │
                                          GPIO Control
                                               │
                              ┌────────────────┼────────────────┐
                              │                │                │
                       ┌──────▼──────┐  ┌──────▼──────┐        │
                       │  Relay CH1  │  │  Relay CH2  │        │
                       └──────┬──────┘  └──────┬──────┘        │
                              │                │               │
                       ┌──────▼──────┐  ┌──────▼──────┐  ┌─────▼─────┐
                       │   Pump 1    │  │   Pump 2    │  │ Buck Conv │
                       │  (Daily)    │  │ (Every 3d)  │  │ 9V → 5V   │
                       └─────────────┘  └─────────────┘  └───────────┘
```

## Hardware Requirements

| Component | Specs | Purpose |
|-----------|-------|---------|
| ESP32 DevKit | Any variant | Main controller |
| 2-Channel Relay | 5V, optocoupler | Pump switching |
| 9V Submersible Pumps | x2 | Water delivery |
| 9.5V 1A DC Adapter | Barrel jack | Power supply |
| LM2596 Buck Converter | Adjustable | 9V → 5V for ESP32 |
| Drip Irrigation Kit | Tubes, emitters | Water distribution |

## MQTT Topics

| Topic | Direction | Payload | Description |
|-------|-----------|---------|-------------|
| `plant/pump1/set` | → ESP32 | `on`/`off` | Control pump 1 |
| `plant/pump2/set` | → ESP32 | `on`/`off` | Control pump 2 |
| `plant/pump1/status` | ← ESP32 | `on`/`off` | Pump 1 state |
| `plant/pump2/status` | ← ESP32 | `on`/`off` | Pump 2 state |
| `plant/pump1/schedule` | ↔ | JSON | Pump 1 schedule |
| `plant/pump2/schedule` | ↔ | JSON | Pump 2 schedule |
| `plant/status` | ← ESP32 | `online`/`offline` | ESP32 status (LWT) |

## Project Structure

```
esp32-smart-irrigation/
├── firmware/           # ESP32 Arduino code
│   ├── src/
│   │   └── main.cpp
│   ├── include/
│   │   └── config.h
│   └── platformio.ini
├── dashboard/          # Web dashboard
│   ├── index.html
│   ├── app.js
│   └── styles.css
├── docs/
│   └── wiring.md
└── README.md
```

## Quick Start

### 1. Configure Firmware

Edit `firmware/include/config.h`:
```cpp
#define WIFI_SSID "your-wifi"
#define WIFI_PASS "your-password"
#define MQTT_HOST "xxx.s1.eu.hivemq.cloud"
#define MQTT_USER "your-mqtt-user"
#define MQTT_PASS "your-mqtt-pass"
```

### 2. Flash ESP32

```bash
cd firmware
pio run --target upload
```

### 3. Open Dashboard

Open `dashboard/index.html` in browser or deploy to GitHub Pages.

## Wiring Diagram

See [docs/wiring.md](docs/wiring.md) for detailed wiring instructions.

## License

MIT

---

Built with ⚡ by Minato & Anurag
