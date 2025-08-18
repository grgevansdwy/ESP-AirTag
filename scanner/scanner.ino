// ==============================================
// Includes
// ==============================================
// FREERTOS
#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"   

//BLE CLIENT
#include <BLEDevice.h>    // BLE core (ESP32 Arduino core 3.x uses NimBLE backend)
#include <BLEUtils.h>
#include <BLEClient.h>
#include <BLEScan.h>

#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <SPI.h>
#include <MFRC522.h>

// ---- HELPER FUNCTIONS -----
#include "BLEScanner.h"
#include "distance.h"

// ==============================================
// UUID
// ==============================================
// Your custom UUIDs — must match the peripheral
#define SERVICE_UUID "275dc6e0-dff5-4b56-9af0-584a5768a02a"
#define BUTTON_CHAR_UUID "b51bd845-2910-4f84-b062-d297ed286b1f"
#define IMU_CHAR_UUID "0679c389-0d92-4604-aac4-664c43a51934"

// ==============================================
// Macros / Pin Definitions
// ==============================================
#define BUZZZER_PIN 42 // GPIO pin for button (not connected to peripheral)
#define RESET_PIN 41
// ==============================================
// Function Prototypes (separate prototypes vs. impls)
// ==============================================
void BLEScannerTask(void *pvParameters);
void distanceTask(void *pvParameters);
void UITask(void *pvParameters);
void buttonTask(void *pvParameters);
// void RFIDTask(void *pvParameters);
// void accessTask(void *pvParameters);

// ==============================================
// Global Task Handles
// ==============================================
static TaskHandle_t TaskHandle_BLEScanner    = nullptr;
// static TaskHandle_t TaskHandle_Access = nullptr;

// ==============================================
// Global Queue Handles
// ==============================================
QueueHandle_t IMUQ;
QueueHandle_t RSSIQ;
QueueHandle_t disQ;
TaskHandle_t LCDTaskHandle;
TaskHandle_t ButtonTaskHandle;
TaskHandle_t RFIDTaskHandle;
QueueSetHandle_t uiSet = nullptr;


// ==============================================
// Semaphore
// ==============================================

// ==============================================
// Global Variable
// ==============================================
// BLEScanner
// BLEClient* client = nullptr;
// BLERemoteCharacteristic* btnRemoteChar = nullptr;
// BLERemoteCharacteristic* imuRemoteChar = nullptr;
//bool connected = false;
uint32_t startTime;

// Peripheral Objects
LiquidCrystal_I2C lcd(0x27, 16, 2);
MFRC522 rfid(PIN_SS, PIN_RST);

byte AUTH_UID[] = { 0xBF, 0xFB, 0x22, 0x43};

// ==============================================
// Task
// ==============================================
void BLEScannerTask(void *pvParameters) {
  uint8_t btnState = 0;
  BLEDevice::init("ESP32-UART-Central");

  // Configure scanner
  BLEScan* scan = BLEDevice::getScan();
  scan->setActiveScan(true);
  scan->setInterval(80); // 80 * 0.625ms = 50ms period
  scan->setWindow(40);   // 40 * 0.625ms = 25ms window (50% duty)

  setIMUQueue(IMUQ);

  Serial.println("Scanning for peripheral advertising the service...");

  while (!connected) {
    // ESP32 core 3.x: start() returns BLEScanResults*
    BLEScanResults* results = scan->start(5, false); // 5s blocking scan
    for (int i = 0; i < results->getCount() && !connected; i++) {
      // getDevice(i) often returns a pointer in 3.x
      BLEAdvertisedDevice d = results->getDevice(i);
      if (d.haveServiceUUID() && d.isAdvertisingService(svcUUID)) {
        connected = connectToPeripheral(d.getAddress());
      }
    }
    if (!connected) Serial.println("Not found yet. Rescanning...");
  }

  startTime = millis();
  while(1){
    // Auto-reconnect if disconnected
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
      //Serial.printf("RSSI: %d dBm\n", rssi);
      xQueueOverwrite(RSSIQ, &rssi); // Push button state to queue
    } // Avoid busy-waiting
    
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

void distanceTask(void *pvParameters) {
  int rssi = 0;
  while(1){
    if (xQueueReceive(RSSIQ, &rssi, portMAX_DELAY) == pdTRUE) {
      updateRssiAvg(rssi); // smoothen the rssi
      float distance = estimateDistanceMeters(rssiAvg, txPower, nFactor); // convert rssi to distance
      //Serial.printf("Raw RSSI: %d dBm | Smoothed RSSI: %.2f dBm | Distance: %.2f m\n", rssi, rssiAvg, distance);
      xQueueOverwrite(disQ, &distance);
    }
  }
  vTaskDelay(pdMS_TO_TICKS(50));
}

void UITask(void *pvParameters) {
  Wire.begin(8, 9);
  lcd.init();
  lcd.backlight();
  uint8_t movingFlag;
  float distance;
  for (;;) {
     xQueueSetMemberHandle member =
        xQueueSelectFromSet(uiSet, portMAX_DELAY);  // wake on either queue

     if (member == IMUQ) {
      uint8_t movingFlag;
      if (xQueueReceive(IMUQ, &movingFlag, 0) == pdTRUE) {
        lcd.setCursor(0, 1);
        if (movingFlag) {
          lcd.print("moving!     ");   // pad spaces to clear leftovers
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
        lcd.print(distance, 2);        // print float with 2 decimals
        lcd.print(" m   ");            // pad a few spaces
        Serial.printf("distance %.2f\n", distance);
      }
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void buttonTask(void *pvParameters) {
  pinMode(BUZZER_PIN, INPUT_PULLUP); // Button pin as input with pull-up
  bool lastButton = HIGH;   // Input pull-up → HIGH when unpressed
  while (1) {
    bool currentButton = digitalRead(BUZZER_PIN);
    // Edge detection: detect falling edge (pressed)
    if (lastButton == HIGH && currentButton == LOW) {
      Serial.println("pressed");
      writeBtnState(true); 
    }

    // Update last button states for edge detection
    lastButton = currentButton;

    vTaskDelay(pdMS_TO_TICKS(100)); // Debounce
  }
}

void resetButtonTask(void *pvParameters) {
  unsigned long lastBtnChange = 0;
  bool lastBtnState = HIGH;
  pinMode(RESET_PIN, INPUT_PULLUP);
  for (;;) {
    bool now = digitalRead(BUTTON_PIN);
    unsigned long time = millis();
    if(now != lastBtnState && ((time - lastBtnChange) > 40)){ // only change if the last button state is different and only once every 40ms
      lastBtnChange = time;
      lastBtnState = now;
      if(now == LOW){ // reset if the state is low
        rfid.PCD_Reset();
        rfid.PCD_Init();
        Serial.println("RFID reset.");
        Serial.println("RFID Locked.");
        vTaskSuspend(ButtonTaskHandle);
        vTaskSuspend(LCDTaskHandle);
        lcd.clear();
        lcd.print("Locked.");
        vTaskResume(RFIDTaskHandle);
      }
    }
  }
}

bool isAuthorized() {
  if (rfid.uid.size != sizeof(AUTH_UID)) return false;
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] != AUTH_UID[i]) return false;
  }
  return true;
}

void RFIDTask(void *pvParameters) {
  
  for (;;) {
    if (!rfid.PICC_IsNewCardPresent()) return;

    // Check if UID of the card present, if present, then sore it, else return false
    if (!rfid.PICC_ReadCardSerial()) return;

    Serial.print("UID:");
    for (byte i = 0; i < rfid.uid.size; i++) {
      Serial.print(' ');
      if (rfid.uid.uidByte[i] < 0x10) Serial.print('0');
      Serial.print(rfid.uid.uidByte[i], HEX);
    }
    Serial.println();

    // Authorization
    if(isAuthorized()){
      ledcWriteTone(BUZZER_PIN, 1000);            // 2 kHz beep
      vTaskDelay(pdMS_TO_TICKS(500));
      ledcWriteTone(BUZZER_PIN, 0);               // stop tone
      Serial.println("Unlocked");
      vTaskResume(LCDTaskHandle);
      vTaskResume(ButtonTaskHandle);
    }

    // Only able to do one print per tap
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    
    if(isAuthorized()) {
      vTaskSuspend(NULL);
    }
  }
}


// ==============================================
// setup / loop (Arduino entry points)
// ----------------------------------------------
// Note: per your guideline, we mainly document non-obvious parts;
// setup/loop are straightforward.
// ==============================================
void setup() {
  Serial.begin(115200);
  svcUUID = BLEUUID(SERVICE_UUID);
  btnUUID = BLEUUID(BUTTON_CHAR_UUID);
  imuUUID = BLEUUID(IMU_CHAR_UUID); // Uncomment if using IMU

  // Create queues
  IMUQ = xQueueCreate(1, sizeof(uint8_t));
  RSSIQ = xQueueCreate(1, sizeof(int));
  disQ = xQueueCreate(1, sizeof(float));

  uiSet = xQueueCreateSet(/*sum of lengths*/ 1 + 1);
  xQueueAddToSet(IMUQ, uiSet);
  xQueueAddToSet(disQ, uiSet);

  // TASK ON PIN 0 - radio-friendly and are not connected to peripheral
  xTaskCreatePinnedToCore(
    BLEScannerTask,
    "BLE Scanner Task",
    8192,
    nullptr,
    5,
    &TaskHandle_BLEScanner,
    0
  );

  xTaskCreatePinnedToCore(
    distanceTask,
    "distance Task",
    4096,
    nullptr,
    1,
    nullptr,
    0
  );

  // TASK ON PIN 1 - All peripherals / UI
  xTaskCreatePinnedToCore(
    UITask,
    "UI Task",
    4096,
    nullptr,
    1,
    nullptr,
    1
  );

  xTaskCreatePinnedToCore(
    buttonTask,
    "Button Task",
    4096,
    nullptr,
    1,
    &ButtonTaskHandle,
    1
  );

  xTaskCreatePinnedToCore(
    LCDTask,
    "LCD Task",
    4096,
    NULL,
    1,
    &LCDTaskHandle,
    1
  );

  xTaskCreatePinnedToCore(
    RFIDTask,
    "RFID Task",
    4096,
    nullptr,
    1,
    &RFIDTaskHandle,
    1
  );

  // xTaskCreatePinnedToCore(
  //   accessTask,
  //   "Access Task",
  //   2048,
  //   nullptr,
  //   1,
  //   &TaskHandle_Access,
  //   1
  // );

  Serial.println("Scanner started");
}

void loop() {
  // Empty: all work is done by FreeRTOS tasks.
}
