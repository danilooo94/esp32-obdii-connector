# Esp32ELMReader

ESP32-C3 firmware that acts as a **Bluetooth LE → USB serial bridge** between a car's ELM327 OBD-II adapter and the [CarDataReader](../../CarDataReader) dashboard.

This is **Part 1** of the R36S Car Dashboard system. See [CarDataReader/README.md](../../CarDataReader/README.md) for the full system overview.

---

## Role in the system

```
Car OBD-II port
      │ (Bluetooth LE)
      ▼
  ELM327 adapter
      │
      │ BLE (GATT service FFF0 / chars FFF1+FFF2)
      ▼
 ┌─────────────────────────────┐
 │      Esp32ELMReader         │  ← you are here
 │  Queries PIDs via ELMduino  │
 │  Outputs JSON over USB      │
 └────────────┬────────────────┘
              │ USB serial  115200 baud
              ▼
       CarDataReader (pygame dashboard on R36S)
```

---

## Serial output

One JSON line per complete reading cycle:

```json
{"coolant_temp": 87.0, "oil_temp": 92.5, "intake_temp": 29.0, "fuel_pct": 68.0, "battery_v": 13.8}
```

Status messages during startup / reconnection:

```json
{"status": "connecting"}
{"status": "connected"}
{"status": "disconnected"}
```

---

## Project structure

```
Esp32ELMReader/
├── platformio.ini                    # Board: esp32-c3-devkitc-02
├── src/
│   └── main.cpp                      # Firmware: BLE connect + PID loop
└── lib/
    └── obd-ble-serial/src/
        ├── BLEClientSerial.h         # BLE-to-Stream adapter
        └── BLEClientSerial.cpp
```

---

## OBD PIDs queried

| Field | PID | Unit |
|-------|-----|------|
| `coolant_temp` | 0x05 | °C |
| `oil_temp` | 0x5C | °C |
| `intake_temp` | 0x0F | °C |
| `fuel_pct` | 0x2F | % |
| `battery_v` | AT RV | V |

---

## Build & Flash

**Requirements:** VS Code + PlatformIO extension.

1. Open this folder in VS Code.
2. Set your OBD adapter's BLE name in `src/main.cpp`:
   ```cpp
   #define OBD_DEVICE_NAME "OBDII"   // e.g. "OBDLink CX", "V-LINK", "VEEPEAK"
   ```
3. Connect the ESP32-C3 via USB and click **Upload**.
4. Open the serial monitor at **115200 baud** to verify JSON output.

**Simulation mode** (no OBD adapter needed):

```cpp
#define SIMULATE_OBD 1   // in src/main.cpp
```

---

## Dependencies

| Library | Source |
|---------|--------|
| `PowerBroker2/ELMduino` | PlatformIO registry |
| `bblanchon/ArduinoJson` | PlatformIO registry |
| `obd-ble-serial` | Vendored in `lib/` (no remote fetch) |
