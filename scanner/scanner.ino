// ==============================================
// Includes
// ==============================================
// FREERTOS
#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

//BLE CLIENT
#include <BLEDevice.h>    // BLE core (ESP32 Arduino core 3.x uses NimBLE backend)
#include <BLEUtils.h>
#include <BLEClient.h>
#include <BLEScan.h>

// ---- HELPER FUNCTIONS -----
#include "BLEScanner.h"

// ---- LCD DISPLAY
#include <LiquidCrystal_I2C.h>

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
#define BUTTON_PIN 42 // GPIO pin for button (not connected to peripheral)

// ==============================================
// Function Prototypes (separate prototypes vs. impls)
// ==============================================
void BLEScannerTask(void *pvParameters);
// void distanceTask(void *pvParameters);
// void UITask(void *pvParameters);
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
//QueueHandle_t RSSIQ;
// QueueHandle_t disQ;
// ==============================================
// Semaphore
// ==============================================
SemaphoreHandle_t btnSemaphore;

// ==============================================
// Global Variable
// ==============================================
// BLEScanner
// BLEClient* client = nullptr;
// BLERemoteCharacteristic* btnRemoteChar = nullptr;
// BLERemoteCharacteristic* imuRemoteChar = nullptr;
//bool connected = false;
uint32_t startTime;
LiquidCrystal_I2C lcd(0x27, 16, 2); 

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
    if(xSemaphoreTake(btnSemaphore, portMAX_DELAY) == pdTRUE) {
      writeBtnState(true); // Write button state if available
      Serial.println("YES SEMAPHORE GOT IT");
    }
    vTaskDelay(pdMS_TO_TICKS(50)); // Avoid busy-waiting
  }
}

void buttonTask(void *pvParameters) {
  btnSemaphore = xSemaphoreCreateBinary();
  pinMode(BUTTON_PIN, INPUT_PULLUP); // Button pin as input with pull-up
  bool lastButton = HIGH;   // Input pull-up → HIGH when unpressed
  if(btnSemaphore == NULL){
    Serial.println("semaphore failed");
  }
  while (1) {
    bool currentButton = digitalRead(BUTTON_PIN);
    // Edge detection: detect falling edge (pressed)
    if (lastButton == HIGH && currentButton == LOW) {
      Serial.println("pressed");
      xSemaphoreGive(btnSemaphore);
    }

    // Update last button states for edge detection
    lastButton = currentButton;

    vTaskDelay(pdMS_TO_TICKS(100)); // Debounce
  }
}

void LCDTask(void *pvParameters) {
  lcd.init();
  lcd.backlight();
  lcd.clear();
  uint8_t movingFlag;
  for (;;) {
    if (xQueueReceive())
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
  // RSSIQ = xQueueCreate(1, sizeof(int));
  // disQ = xQueueCreate(1, sizeof(int));


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

  // xTaskCreatePinnedToCore(
  //   distanceTask,
  //   "distance Task",
  //   4096,
  //   nullptr,
  //   1,
  //   nullptr,
  //   0
  // );

  // // TASK ON PIN 1 - All peripherals / UI
  // xTaskCreatePinnedToCore(
  //   UITask,
  //   "UI Task",
  //   4096,
  //   nullptr,
  //   1,
  //   nullptr,
  //   1
  // );

  xTaskCreatePinnedToCore(
    buttonTask,
    "Button Task",
    4096,
    nullptr,
    1,
    nullptr,
    1
  );

  xTaskCreatePinnedToCore(
    LCDTask,
    "LCD task",
    4096,
    NULL,
    1,
    NULL,
    1
  );

  // xTaskCreatePinnedToCore(
  //   RFIDTask,
  //   "RFID Task",
  //   4096,
  //   nullptr,
  //   1,
  //   nullptr,
  //   1
  // );

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
