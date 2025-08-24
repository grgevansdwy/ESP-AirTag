# ESP-AirTag

An **ESP32-S3 based AirTag-like tracking system** featuring **BLE communication, FreeRTOS task management, IMU-based movement detection, and RFID authentication**. The system enables real-time distance estimation, secure access control, and lost-mode alerts via a buzzer.

---

## 📌 Features
- **BLE Communication**
  - Transmitter (AirTag) advertises data including distance and movement status
  - Receiver (Tracker) scans, connects, and sends lost mode commands
- **RFID Authentication**
  - Unlocks access to distance data
  - Hardware button to re-lock RFID
- **Movement Detection**
  - IMU on AirTag detects motion and sends updates to Tracker
- **Distance Estimation**
  - Using RSSI (Received Signal Strength Indicator) or Time-of-Flight methods
- **Lost Mode**
  - Trigger buzzer on AirTag remotely from Tracker
- **LCD User Interface**
  - 1st line: Distance
  - 2nd line: Motion status or "Access Denied"

---
## 🛠 Hardware Requirements
- **ESP32-S3** (2 units: transmitter & receiver)
- **RFID module** (e.g., MFRC522)
- **IMU sensor** (e.g., MPU6050, MPU9250)
- **I2C LCD display**
- **Buzzer**
- Push button for RFID re-lock

---

## ⚙️ Software Requirements
- **Arduino IDE**
- **ESP-IDF** or Arduino core for ESP32
- **FreeRTOS**
- BLE libraries (`ESP32 BLE Arduino`)
- RFID library for chosen module
- IMU library for chosen sensor

---

## 🚀 How It Works
1. **Authentication** — RFID card unlocks access to distance data.  
2. **Scanning** — Tracker connects to AirTag via BLE to read IMU status & RSSI.  
3. **Display** — Distance and motion status shown on LCD.  
4. **Lost Mode** — Tracker sends command to AirTag to trigger buzzer.  
5. **Re-lock** — Button resets RFID lock.


📄 [Documentation](https://grgevansdwy.github.io/ESP-AirTag/)
