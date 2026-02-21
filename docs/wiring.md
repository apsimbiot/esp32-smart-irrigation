# 🔌 Wiring Guide

## Components Overview

| Component | Purpose |
|-----------|---------|
| ESP32 DevKit | Main controller (WiFi + MQTT) |
| 2-Channel Relay | Switches pump power |
| LM2596 Buck Converter | Converts 9V → 5V for ESP32 |
| 9.5V 1A DC Adapter | Powers pumps + ESP32 |
| 9V Submersible Pumps (x2) | Water delivery |

## Wiring Diagram

```
                    ┌─────────────────────────────────────────┐
                    │            9.5V DC Adapter              │
                    │              (1A output)                │
                    └─────────────┬───────────────────────────┘
                                  │
                                  │ 9V
                    ┌─────────────┼─────────────┐
                    │             │             │
              ┌─────▼─────┐ ┌─────▼─────┐ ┌─────▼─────┐
              │  Pump 1   │ │  Pump 2   │ │   Buck    │
              │ (via      │ │ (via      │ │ Converter │
              │  relay)   │ │  relay)   │ │ 9V → 5V   │
              └───────────┘ └───────────┘ └─────┬─────┘
                    ▲             ▲             │
                    │             │             │ 5V
              ┌─────┴─────────────┴─────┐ ┌─────▼─────┐
              │    2-Channel Relay      │ │   ESP32   │
              │                         │ │  DevKit   │
              │  IN1 ← GPIO26          │ │           │
              │  IN2 ← GPIO27          │ │           │
              │  VCC ← 5V              │ │           │
              │  GND ← GND             │ │           │
              └─────────────────────────┘ └───────────┘
```

## Detailed Connections

### ESP32 Pinout

| ESP32 Pin | Connected To | Purpose |
|-----------|--------------|---------|
| GPIO26 | Relay IN1 | Pump 1 control |
| GPIO27 | Relay IN2 | Pump 2 control |
| 5V / VIN | Buck OUT+ | Power input |
| GND | Buck OUT- / Relay GND | Common ground |

### Relay Module

| Relay Pin | Connected To |
|-----------|--------------|
| VCC | ESP32 5V (or Buck OUT+) |
| GND | Common GND |
| IN1 | ESP32 GPIO26 |
| IN2 | ESP32 GPIO27 |
| COM1 | 9V Adapter + |
| NO1 | Pump 1 + |
| COM2 | 9V Adapter + |
| NO2 | Pump 2 + |

### Buck Converter (LM2596)

| Buck Pin | Connected To |
|----------|--------------|
| IN+ | 9V Adapter + |
| IN- | 9V Adapter - (GND) |
| OUT+ | ESP32 5V/VIN |
| OUT- | Common GND |

> ⚠️ **Important:** Before connecting ESP32, adjust the buck converter output to exactly 5V using the potentiometer!

### Pumps

| Pump Wire | Connected To |
|-----------|--------------|
| Pump 1 + | Relay NO1 |
| Pump 1 - | Common GND |
| Pump 2 + | Relay NO2 |
| Pump 2 - | Common GND |

## Step-by-Step Assembly

### 1. Prepare Buck Converter
1. Connect 9V adapter to buck converter IN+ and IN-
2. **Before connecting anything else**, use multimeter to measure OUT+/OUT-
3. Adjust potentiometer until output reads exactly 5.0V
4. Disconnect adapter after adjustment

### 2. Wire ESP32 Power
1. Connect buck converter OUT+ to ESP32 VIN (or 5V pin)
2. Connect buck converter OUT- to ESP32 GND

### 3. Wire Relay Module
1. Connect relay VCC to ESP32 5V
2. Connect relay GND to ESP32 GND
3. Connect relay IN1 to ESP32 GPIO26
4. Connect relay IN2 to ESP32 GPIO27

### 4. Wire Pumps
1. Connect 9V adapter + to both relay COM terminals
2. Connect relay NO1 to Pump 1 +
3. Connect relay NO2 to Pump 2 +
4. Connect both Pump - wires to common GND

### 5. Final Check
- [ ] Buck converter outputs exactly 5V
- [ ] All GND connections share common ground
- [ ] Relay logic matches code (active-LOW by default)
- [ ] Pumps are connected to NO (normally open) terminals

## Safety Notes

⚡ **Power Safety**
- Never exceed pump voltage rating (9V)
- Don't run pumps dry for extended periods
- Keep electronics away from water

🔌 **Relay Notes**
- Most relay modules are active-LOW (GPIO LOW = relay ON)
- If your relay is active-HIGH, change `RELAY_ACTIVE_LOW` to `false` in config.h

💧 **Pump Notes**
- Submersible pumps should always be submerged when running
- Consider adding check valves to prevent backflow

## Breadboard Layout

```
    ┌──────────────────────────────────────────────────┐
    │ + + + + + + + + + + + + + + + + + + + + + + + +  │ ← 5V rail
    │ - - - - - - - - - - - - - - - - - - - - - - - -  │ ← GND rail
    │                                                  │
    │   ┌─────┐        ┌─────────┐                    │
    │   │Buck │        │  Relay  │                    │
    │   │Conv │        │ Module  │                    │
    │   └─────┘        └─────────┘                    │
    │                                                  │
    │            ┌────────────────┐                   │
    │            │     ESP32      │                   │
    │            │    DevKit      │                   │
    │            └────────────────┘                   │
    │                                                  │
    │ - - - - - - - - - - - - - - - - - - - - - - - -  │ ← GND rail
    │ + + + + + + + + + + + + + + + + + + + + + + + +  │ ← 9V rail (for pumps)
    └──────────────────────────────────────────────────┘
```

## Troubleshooting

| Problem | Solution |
|---------|----------|
| ESP32 not powering on | Check buck converter output is 5V |
| Relay not clicking | Verify GPIO connections and active-LOW logic |
| Pump not running | Check relay NO terminal connections |
| ESP32 resets when pump starts | Add capacitor to power rails or use separate power |
| WiFi keeps disconnecting | Move ESP32 closer to router, check antenna |
