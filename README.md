# Rice Farm Water Management System

Embedded system developed as an undergraduate thesis project to monitor water-related variables in a rice field and remotely control a gate. The solution uses ESP32 microcontrollers, LoRa communication, and Firebase Realtime Database to connect the field node to the network-connected gateway.

> **Status:** academic prototype under development. Test directories are kept for experiment traceability. The recommended firmware pair is [`slave.ino`](slave.ino) and [`master.ino`](master.ino).

## Overview

```text
┌──────────────────────┐       LoRa/E32        ┌──────────────────────┐
│ Field node           │ ────────────────────▶ │ Gateway node          │
│ ESP32 + sensors      │ ◀──────────────────── │ ESP32 + Wi-Fi         │
│ flow / level / pH    │                       │ Firebase RTDB         │
│ gate actuator        │                       │ dashboard/control     │
└──────────────────────┘                       └──────────────────────┘
```

### Features

- Flow measurement with a YF-S201/YF201 sensor.
- Water-level reading with a float switch.
- pH reading and calibration.
- Telemetry encoded as JSON messages.
- Serial communication with LoRa E32 modules.
- Data publishing and command reception through Firebase.
- Gate actuation using relays and limit switches.
- Independent experiments for handshakes, frequency, packets, and Wi-Fi.

## Repository structure

| Path | Purpose |
| --- | --- |
| [`master.ino`](master.ino) | Final gateway sketch: receives LoRa telemetry and synchronizes it with Firebase. |
| [`slave.ino`](slave.ino) | Final field-node sketch: reads sensors and controls the gate through LoRa commands. |
| [`espSlave/`](espSlave/) | Earlier field-node implementation. |
| [`lora_e32/v2/espWiFi/`](lora_e32/v2/espWiFi/) | Earlier Wi-Fi gateway implementation. |
| [`lora_e32/v2/espEscravo/`](lora_e32/v2/espEscravo/) | Experimental slave-node variant. |
| [`calibra_sensor_ph/`](calibra_sensor_ph/) | Helper sketch for pH sensor calibration. |
| [`sensor_fluxo_YF201/`](sensor_fluxo_YF201/) | Isolated flow-sensor test. |
| [`sensor_umidade/`](sensor_umidade/) | Isolated analog-input test. |
| [`projetoubiquos/`](projetoubiquos/) | Earlier experiments and proofs of concept. |

## Reference hardware

- ESP32.
- LoRa E32 module (UART on `RX=4` and `TX=5` in these sketches).
- YF-S201/YF201 flow sensor.
- Float switch using `INPUT_PULLUP`.
- Analog pH sensor.
- Two relays, gate motor, and limit switches.

Pin assignments may vary between prototypes. Confirm the electrical schematic before powering the circuit; do not use these examples as a replacement for electrical validation or relay sizing.

## Getting started

### 1. Prepare the environment

1. Install the [Arduino IDE](https://www.arduino.cc/en/software).
2. Add **ESP32 by Espressif Systems** support.
3. Install the libraries used by the selected sketch:
   - `SoftwareSerial` (or a board-compatible implementation).
   - `Firebase ESP Client` for the gateway.
   - Firebase dependencies (`TokenHelper` and `RTDBHelper`) when referenced by a sketch.
   - `painlessMesh` and `ArduinoJson` only for experiments under `projetoubiquos/espmesh/`.
4. Select the ESP32 board, serial port, and serial monitor speed (`115200` or `9600`, as indicated by the sketch).

### 2. Configure credentials

Credentials must never be committed. The final gateway sketch loads them from a local `secrets.h` file, which is ignored by Git. Create `secrets.h` in the repository root before compiling `master.ino`:

```cpp
#define WIFI_SSID "your-wifi-ssid"
#define WIFI_PASSWORD "your-wifi-password"
#define API_KEY "your-firebase-api-key"
#define DATABASE_URL "https://your-project-default-rtdb.firebaseio.com/"
```

Do not paste real values into tracked sketches or documentation. Use a development Firebase project, restricted database rules, and rotated credentials. If a credential was ever committed or shared, revoke it and generate a replacement.

### 3. Upload the firmware

1. Open the desired `.ino` file in the Arduino IDE.
2. Compile before connecting power hardware.
3. Upload the gateway first and validate Wi-Fi/Firebase connectivity in the serial monitor.
4. Upload the field-node firmware and validate the sensors without actuating the motor.
5. Test `OPEN_GATE` and `CLOSE_GATE` with the gate disconnected and the limit switches working.

## Telemetry protocol

The field node sends newline-delimited messages. The final implementation uses this compact structure:

```json
{
  "id": 0,
  "p": 6.8,
  "f": 2.4,
  "l": 1,
  "g": 0
}
```

The fields represent the slave ID, pH, flow rate, water level, and gate state. The gateway and field node must use the same format and units.

## Safety and operation

- Never publish passwords, API keys, or tokens to Git.
- Do not actuate motors until limit switches and electrical protection have been validated.
- Calibrate the pH and flow sensors before interpreting the data.
- Treat values received over LoRa as untrusted input.
- Use Firebase security rules appropriate for the environment; this firmware is a prototype and is not a replacement for production authentication or authorization.
- Stop testing if there is overheating, electrical noise, or unexpected motor behavior.

## Development

This repository has no automated test suite because the sketches depend on physical hardware. Before submitting a change:

1. Compile the affected sketch in the Arduino IDE.
2. Check the serial monitor under normal and disconnected conditions.
3. Test Wi-Fi/LoRa failure paths and the limit switches.
4. Document pin, library, and protocol changes in the pull request.

## Authors

Academic project by **Matheus Costa Amboni**, developed at the Federal University of Santa Catarina (UFSC).

See the associated academic thesis for background, methodology, and experimental results.
