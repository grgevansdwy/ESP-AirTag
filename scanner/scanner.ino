/**
 * @file scanner.ino
 * @brief ESP32 BLE Central with RFID and LCD UI.
 * @details
 * This sketch implements an ESP32 BLE central device that:
 * - Scans and connects to a BLE peripheral (with Button + IMU characteristics)
 * - Estimates distance from RSSI using a path-loss model
 * - Displays IMU movement state and distance on an I2C LCD
 * - Uses FreeRTOS tasks for concurrency (scanner, distance calc, UI, button, RFID, reset)
 * - Integrates RC522 RFID for access control, requiring authorized UID
 *
 * @note Uses ESP32 Arduino Core 3.x (NimBLE backend).
 * @author
 * George Evans Daenuwy & Rasya Fawwaz
 * @date 2025-08-21
 */

// ==============================================
// Includes
// ==============================================
// FREERTOS
#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"   

// BLE CLIENT
#include <BLEDevice.h>    /**< BLE core (ESP32 Arduino core 3.x uses NimBLE backend) */
#include <BLEUtils.h>
#include <BLEClient.h>
#include <BLEScan.h>

#include <LiquidCrystal_I2C.h> /**< I2C LCD library */
#include <Wire.h>              /**< I2C bus */
#include <SPI.h>               /**< SPI bus */
#include <MFRC522.h>           /**< RFID RC522 driver */

// ---- HELPER FUNCTIONS -----
#include "BLEScanner.h"        /**< Custom BLE scanning logic */
#include "distance.h"          /**< Distance estimation from RSSI */

// ==============================================
// UUIDs (must match peripheral)
// ==============================================
#define SERVICE_UUID "275dc6e0-dff5-4b56-9af0-584a5768a02a"  /**< Service UUID */
#define BUTTON_CHAR_UUID "b51bd845-2910-4f84-b062-d297ed286b1f" /**< Button characteristic UUID */
#define IMU_CHAR_UUID "0679c389-0d92-4604-aac4-664c43a51934"   /**< IMU characteristic UUID */

// ==============================================
// Macros / Pin Definitions
// ==============================================
#define BUZZER_PIN 42 /**< GPIO pin for buzzer/button */
#define RESET_PIN 41  /**< GPIO pin for reset button */
#define RFID_PIN 40   /**< GPIO pin for RFID buzzer */
#define PIN_SCK  13   /**< RC522 SCK pin */
#define PIN_MISO 11   /**< RC522 MISO pin */
#define PIN_MOSI 12   /**< RC522 MOSI pin */
#define PIN_SS   14   /**< RC522 chip select (SDA/SS) */
#define PIN_RST  10   /**< RC522 reset */

// ==============================================
// Function Prototypes
// ==============================================
/** @brief Task to scan for BLE peripherals */
void BLEScannerTask(void *pvParameters);
/** @brief Task to compute distance from RSSI */
void distanceTask(void *pvParameters);
/** @brief Task to update LCD UI */
void UITask(void *pvParameters);
/** @brief Task to handle button input */
void buttonTask(void *pvParameters);
/** @brief Task to handle RFID scanning and authorization */
void RFIDTask(void *pvParameters);
/** @brief Task to reset RFID and lock system */
void resetButtonTask(void *pvParameters);

// ==============================================
// Global Task Handles
// ==============================================
static TaskHandle_t TaskHandle_BLEScanner    = nullptr; /**< Handle for BLE scanner task */
TaskHandle_t UITaskHandle;    /**< Handle for UI task */
TaskHandle_t ButtonTaskHandle;/**< Handle for button task */
TaskHandle_t RFIDTaskHandle;  /**< Handle for RFID task */

// ==============================================
// Global Queue Handles
// ==============================================
QueueHandle_t IMUQ;   /**< Queue for IMU moving flag */
QueueHandle_t RSSIQ;  /**< Queue for RSSI values */
QueueHandle_t disQ;   /**< Queue for distance values */
QueueSetHandle_t uiSet = nullptr; /**< Queue set for UI multiplexing */

// ==============================================
// Global Variables
// ==============================================
uint32_t startTime; /**< Timer for RSSI sampling */

// Peripheral Objects
LiquidCrystal_I2C lcd(0x27, 16, 2); /**< I2C LCD (16x2) */
MFRC522 rfid(PIN_SS, PIN_RST);      /**< RFID reader instance */

/** @brief Authorized UID for RFID access */
byte AUTH_UID[] = { 0x04, 0x81, 0x70, 0x0A, 0x9C, 0x14, 0x90 };

// ==============================================
// Tasks
// ==============================================

/**
 * @brief BLE scanner task.
 * - Initializes BLE central
 * - Scans for peripherals advertising the target service UUID
 * - Connects and auto-reconnects if disconnected
 * - Periodically reads RSSI and sends to RSSI queue
 */
void BLEScannerTask(void *pvParameters) {
  uint8_t btnState = 0;
  BLEDevice::init("ESP32-UART-Central");

  // Configure scanner
  BLEScan* scan = BLEDevice::getScan();
  scan->setActiveScan(true);
  scan->setInterval(80);
  scan->setWindow(40);

  setIMUQueue(IMUQ);

  Serial.println("Scanning for peripheral advertising the service...");

  while (!connected) {
    BLEScanResults* results = scan->start(5, false);
    for (int i = 0; i < results->getCount() && !connected; i++) {
      BLEAdvertisedDevice d = results->getDevice(i);
      if (d.haveServiceUUID() && d.isAdvertisingService(svcUUID)) {
        connected = connectToPeripheral(d.getAddress());
      }
    }
    if (!connected) Serial.println("Not found yet. Rescanning...");
  }

  startTime = millis();
  while(1){
    if (connected && client && !client->isConnected()) {
      Serial.println("Disconnected. Rescanning...");
      connected = false;
      btnRemoteChar = nullptr;
      imuRemoteChar = nullptr;
      client->disconnect();

      BLEScan* scan = BLEDevice::getScan();
      while (!connected) {
        BLEScanResults* results = scan->start(5, false);
        for (int i = 0; i < results->getCount() && !connected; i++) {
          BLEAdvertisedDevice d = results->getDevice(i);
          if (d.haveServiceUUID() && d.isAdvertisingService(svcUUID)) {
            connected = connectToPeripheral(d.getAddress());
          }
        }
        if (!connected) Serial.println("Still searching...");
      }
    }
    if (connected && client && client->isConnected() && (millis() - startTime > 200)) {
      startTime = millis();
      int rssi = client->getRssi();
      xQueueOverwrite(RSSIQ, &rssi);
    }
    vTaskDelay(pdMS_TO_TICKS(31));
  }
}

/**
 * @brief Distance estimation task.
 * - Consumes RSSI values
 * - Smooths RSSI
 * - Converts to distance estimate
 * - Sends result to distance queue
 */
void distanceTask(void *pvParameters) {
  int rssi = 0;
  while(1){
    if (xQueueReceive(RSSIQ, &rssi, portMAX_DELAY) == pdTRUE) {
      updateRssiAvg(rssi);
      float distance = estimateDistanceMeters(rssiAvg, txPower, nFactor);
      Serial.printf("Raw RSSI: %d dBm | Smoothed RSSI: %.2f dBm | Distance: %.2f m\n", rssi, rssiAvg, distance);
      xQueueOverwrite(disQ, &distance);
    }
  }
  vTaskDelay(pdMS_TO_TICKS(20));
}

/**
 * @brief UI task.
 * - Displays distance and movement state on LCD
 * - Consumes from IMUQ and distance queue
 */
void UITask(void *pvParameters) {
  lcd.init();
  lcd.backlight();
  lcd.clear();
  uint8_t movingFlag;
  float distance;
  vTaskSuspend(NULL);
  for (;;) {
     xQueueSetMemberHandle member =
        xQueueSelectFromSet(uiSet, portMAX_DELAY);

     if (member == IMUQ) {
      uint8_t movingFlag;
      if (xQueueReceive(IMUQ, &movingFlag, 0) == pdTRUE) {
        lcd.setCursor(0, 1);
        if (movingFlag) {
          lcd.print("moving!     ");
          Serial.println("device is moving!");
        } else {
          lcd.print("not moving  ");
          Serial.println("Device is not moving");
        }
      }
    } else if (member == disQ) {
      float distance;
      if (xQueueReceive(disQ, &distance, 0) == pdTRUE) {
        lcd.setCursor(0, 0);
        lcd.print("distance: ");
        lcd.print(distance, 2);
        lcd.print(" m   ");
        Serial.printf("distance %.2f\n", distance);
      }
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

/**
 * @brief Button task.
 * - Reads button press with debouncing
 * - Sends state via BLE
 */
void buttonTask(void *pvParameters) {
  pinMode(BUZZER_PIN, INPUT_PULLUP);
  bool lastButton = HIGH;
  vTaskSuspend(NULL);
  while (1) {
    bool currentButton = digitalRead(BUZZER_PIN);
    if (lastButton == HIGH && currentButton == LOW) {
      Serial.println("pressed");
      writeBtnState(true); 
    }
    lastButton = currentButton;
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

/**
 * @brief Reset button task.
 * - Debounced reset button input
 * - Resets RFID and locks system
 * - Suspends UI and button tasks
 */
void resetButtonTask(void *pvParameters) {
  unsigned long lastBtnChange = 0;
  bool lastBtnState = HIGH;
  pinMode(RESET_PIN, INPUT_PULLUP);
  for (;;) {
    bool now = digitalRead(RESET_PIN);
    unsigned long time = millis();
    if(now != lastBtnState && ((time - lastBtnChange) > 40)) {
      lastBtnChange = time;
      lastBtnState = now;
      if(now == LOW){
        rfid.PCD_Reset();
        rfid.PCD_Init();
        Serial.println("RFID reset.");
        Serial.println("RFID Locked.");
        lcd.clear();
        lcd.print("Locked."); 
        vTaskSuspend(ButtonTaskHandle);
        vTaskSuspend(UITaskHandle);
        vTaskResume(RFIDTaskHandle);
      }
    }
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

/**
 * @brief Check if RFID UID is authorized.
 * @retval true if UID matches AUTH_UID
 * @retval false otherwise
 */
bool isAuthorized() {
  if (rfid.uid.size != sizeof(AUTH_UID)) return false;
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] != AUTH_UID[i]) return false;
  }
  return true;
}

/**
 * @brief RFID task.
 * - Waits for RFID card
 * - Prints UID to Serial
 * - Grants access if authorized
 * - Resumes UI and button tasks on success
 */
void RFIDTask(void *pvParameters) {
  rfid.PCD_Init();
  ledcAttach(RFID_PIN, 1000, 11);
  rfid.PCD_DumpVersionToSerial();
  Serial.println("RFID Locked");
  while (1) {
    if (!rfid.PICC_IsNewCardPresent()) {
      continue;
    }
    if (!rfid.PICC_ReadCardSerial()) continue;

    Serial.print("UID:");
    for (byte i = 0; i < rfid.uid.size; i++) {
      Serial.print(' ');
      if (rfid.uid.uidByte[i] < 0x10) Serial.print('0');
      Serial.print(rfid.uid.uidByte[i], HEX);
    }
    Serial.println();

    if(isAuthorized()){
      ledcWriteTone(RFID_PIN, 1000);
      vTaskDelay(pdMS_TO_TICKS(500));
      ledcWriteTone(RFID_PIN, 0);
      vTaskResume(UITaskHandle);
      vTaskResume(ButtonTaskHandle);
      vTaskSuspend(NULL);
    }
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

// ==============================================
// Arduino entry points
// ==============================================

/**
 * @brief Arduino setup function.
 * Initializes queues, peripherals, tasks, and starts FreeRTOS.
 */
void setup() {
  Serial.begin(115200);
  svcUUID = BLEUUID(SERVICE_UUID);
  btnUUID = BLEUUID(BUTTON_CHAR_UUID);
  imuUUID = BLEUUID(IMU_CHAR_UUID);

  IMUQ = xQueueCreate(1, sizeof(uint8_t));
  RSSIQ = xQueueCreate(1, sizeof(int));
  disQ = xQueueCreate(1, sizeof(float));

  uiSet = xQueueCreateSet(1 + 1);
  xQueueAddToSet(IMUQ, uiS
