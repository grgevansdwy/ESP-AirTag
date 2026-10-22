# ESP-AirTag

An AirTag-style tracker built from two ESP32-S3 boards. The **tag** advertises over Bluetooth Low Energy and reports its motion. The **tracker** scans for it, estimates the distance, shows it on an LCD once you authenticate with an RFID card, and can trigger a "lost mode" buzzer on the tag.

**Docs:** <https://grgevansdwy.github.io/ESP-AirTag/> (Doxygen API reference generated from the source)

## How it works

```
        TAG  (server/)                                   TRACKER  (scanner/)
 ┌───────────────────────────┐   BLE advertise      ┌───────────────────────────────┐
 │ ESP32-S3                  │ ───────────────────▶ │ ESP32-S3                      │
 │  IMU task  ─ motion flag  │   RSSI               │  BLE scan ─▶ RSSI filter ─▶   │
 │  Buzzer task ◀────────────│◀──── write "lost" ── │            distance estimate  │
 │  MPU6500 IMU, buzzer      │   (GATT characteristics)│  RFID (RC522) gate         │
 └───────────────────────────┘                      │  16x2 I2C LCD, buttons        │
                                                    └───────────────────────────────┘
```

1. **Authenticate.** Tap an RFID card on the tracker to unlock the distance display. A hardware button locks it again.
2. **Scan.** The tracker finds the tag by its service UUID, connects, and reads the motion state.
3. **Estimate distance.** Each RSSI reading is smoothed with an exponential moving average (alpha = 0.2), then converted with the log-distance path-loss model:

   `d = 10 ^ ((txPower - RSSI) / (10 * n))`

   `txPower` is the RSSI at 1 m and `n` is the path-loss exponent. Both are calibration constants in [`scanner/distance.cpp`](scanner/distance.cpp) (defaults `-52 dBm` and `2.5`). Tune them for your room; indoor RSSI distance is only a rough estimate.
4. **Display.** Line 1 shows the distance. Line 2 shows the motion status, or `Access Denied` when locked.
5. **Lost mode.** The lost-mode button on the tracker writes a BLE characteristic that makes the tag sound its buzzer.

FreeRTOS tasks keep the IMU polling, buzzer control and button relay independent of the BLE stack.

## Repository layout

| Path | What it is |
|---|---|
| `server/` | Tag firmware: BLE peripheral, IMU driver (`IMU.cpp`), buzzer |
| `scanner/` | Tracker firmware: BLE scanner, RSSI-to-distance, RFID, LCD |
| `docs/` | Generated Doxygen HTML, served by GitHub Pages |
| `Doxyfile` | Doxygen config (`doxygen Doxyfile` regenerates the docs) |

## Hardware

| Part | Tag | Tracker |
|---|---|---|
| ESP32-S3 dev board | 1 | 1 |
| MPU6500 IMU (I2C, address `0x68`) | yes | |
| Buzzer | GPIO 2 (lost-mode alarm) | GPIO 40 (RFID feedback tone) |
| Lost-mode push button | | GPIO 42 |
| Re-lock push button | | GPIO 41 |
| MFRC522 RFID reader (SPI) | | SCK 13, MISO 11, MOSI 12, SS 14, RST 10 |
| 16x2 I2C LCD (address `0x27`) | | yes |

Pin numbers come from the `#define`s at the top of each `.ino`. Check them against your wiring.

## Build and flash

1. Install the Arduino IDE (or Arduino CLI) with the **esp32** board package (Arduino core 3.x).
2. Install these libraries: `MFRC522`, `LiquidCrystal_I2C`. The BLE library ships with the ESP32 core.
3. Open `server/server.ino`, select your ESP32-S3 board, and flash the first board (the tag).
4. Open `scanner/scanner.ino` and flash the second board (the tracker).

The two sketches must share the same `SERVICE_UUID`, `BUTTON_CHAR_UUID` and `IMU_CHAR_UUID`. They already do, so change all three together if you change any.

## Known limitations

- RSSI distance is noisy. Walls, bodies and antenna orientation shift it by meters.
- The RFID check only gates the display. It is not link-layer security, and the BLE link itself is not encrypted or bonded.
- Single tag, single tracker.
