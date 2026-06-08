# SISEM Tarefa B - Grupo 3

**Board:** Infineon XMC4200 Platform2Go  
**Course:** LETI-SISEM, ISEP  
**Group:** 3 (Diogo Nogueira / Vasco Magolo - 1241692 / 1231562)

## Overview

Event-driven bare-metal firmware for the XMC4200. A 10 ms system tick drives all periodic tasks concurrently:

- **LED** blink rate controlled via UART CLI or onboard button
- **Potentiometer** value read via ADC, applied to PWM duty cycle
- **AHT10** temperature and humidity sensor read over I2C
- **CAN** periodic broadcast of sensor data every 500 ms; receive and parse frames from other groups
- **LCD** 16x2 display over I2C, mode selectable via CLI (off / last received CAN frame / potentiometer / GPS)
- **GPS** u-blox NEO-6M reads NMEA `$GPGGA` frames

## Feature flags

Hardware modules can be individually enabled or disabled in [`config.h`](config.h):

```c
#define FEATURE_AHT10  // I2C temperature and humidity sensor
#define FEATURE_CAN    // CAN bus transmit and receive
#define FEATURE_LCD    // 16x2 LCD display over I2C
#define FEATURE_GPS    // GPS module via UART
```

Commenting out a define removes all related code at compile time - includes, CLI menu options, periodic tasks, and ISR handlers - without touching any other file.

## UART CLI

Connect at **9600 8N1**. Open [`apps/docklight.ptp`](apps/docklight.ptp) for pre-configured send sequences.

| Option         | Action                                  |
| -------------- | --------------------------------------- |
| `↵`            | Utility carriage return (<CR>)          |
| `0`            | Exit                                    |
| `1`            | Read potentiometer ADC value            |
| `2`            | Set LED blink period (ms, 100-10000)    |
| `2a` → `2000↵` | Preset: 2 s period                      |
| `2b` → `1000↵` | Preset: 1 s period                      |
| `2c` → `500↵`  | Preset: 500 ms period                   |
| `3`            | Read current blink period               |
| `4`            | Read AHT10 temperature and humidity     |
| `5`            | Set CAN RX filter (hex CAN ID)          |
| `5a` → `4c0↵`  | Preset: filter Group 3 (`0x4C0`)        |
| `5b` → `4c3↵`  | Preset: filter Group 5 (`0x4C3`)        |
| `5c` → `7ff↵`  | Preset: filter other (change ID)        |
| `6`            | Change LCD Mode                         |
| `6a` → `0`     | Preset: OFF                             |
| `6b` → `1`     | Preset: CAN RX (filtered ID + temp/hum) |
| `6c` → `2`     | Preset: Potentiometer info              |
| `6d` → `3`     | Preset: GPS coordinates                 |

## CAN Bus

### Sensor frame (Tx)

| Field     | Value                                          |
| --------- | ---------------------------------------------- |
| CAN ID    | `0x4C0` (1216) - `Msg_Group_3`                 |
| DLC       | 8 bytes                                        |
| Period    | 500 ms                                         |
| Bytes 0-3 | Temperature - `physical = raw * 0.1 - 55` (°C) |
| Bytes 4-7 | Humidity - `physical = raw * 0.1` (%)          |

### GPS frame (Tx)

| Field     | Value                                         |
| --------- | --------------------------------------------- |
| CAN ID    | `0x4C0` (1216) - `Msg_Group_3`                |
| DLC       | 8 bytes                                       |
| Period    | On fix update (~1 Hz), only when fix is valid |
| Bytes 0-3 | Latitude - `physical = raw * 0.000001`        |
| Bytes 4-7 | Longitude - `physical = raw * 0.000001`       |

DBC definition: [`docs/DBC_TarefaB.dbc`](docs/DBC_TarefaB.dbc)  
Cangaroo workspace: [`apps/CAN.cangaroo`](apps/CAN.cangaroo)

## Pins used:

### Led

- P2.3
- GND

### Potentiometer

- 3.3 V
- P14.6
- GND

### I2C Bus (SCL/P3.0, SDA/P2.5)

The following devices share the same bus and can be connected in parallel:

- **AHT10** temperature and humidity sensor
- **LCD** 16x2 display

Power each device independently (5 V / GND).

### GPS

- 5 V
- GND
- P2.14 TX → GPS RX
- P2.15 RX ← GPS TX

### Other connections:

#### CANBUS USB transceiver

- DB-9

## Software used:

### Development

- [DAVE IDE (Infineon Technologies)](https://www.infineon.com/design-resources/development-tools/sdk/dave)

### UART

- [Docklight](apps/docklight.ptp)

### Variables Watcher

- [Micrium](apps/micrium.wspx)

### CAN

- [Cangaroo](apps/CAN.cangaroo)
  - uses [DBC file](docs/DBC_TarefaB.dbc)
