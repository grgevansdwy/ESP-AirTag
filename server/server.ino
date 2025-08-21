/**
 * @file server.ino
 * @brief ESP32 BLE Server with IMU integration and buzzer control.
 * @details
 * Implements a BLE GATT server with two characteristics:
 * - Button characteristic (read/write)
 * - IMU characteristic (notify for movement detection)
 * 
 * The system uses FreeRTOS tasks:
 * - IMUTask: Reads IMU sensor data and detects movement
 * - ButtonRelayTask: Handles BLE write events and signals via semaphore
 * - BuzzerSetTask: Toggles a buzzer based on BLE commands
 * 
 * Includes orientation computation, linear acceleration processing,
 * and movement detection with a smoothing filter.
 * 
 * @note Requires BLE libraries and FreeRTOS on ESP32.
 * @author
 * George Evans Daenuwy & Rasya Fawwaz
 * @date 2025-08-21
 */

#include <BLEDevice.h>   /**< Initializes BLE general functionality */
#include <BLEServer.h>   /**< Provides BLE server (peripheral role) functionality */
#include <BLEUtils.h>    /**< BLE helper utilities such as UUID handling */
#include <BLE2902.h>     /**< Descriptor class for Client Characteristic Configuration Descriptor (CCCD) */
#include "IMU.h"         /**< Custom IMU driver */
#include "IMU_STRUCT.h"  /**< IMU data structure definition */
#include "IMU_REGISTER_MAP.h" /**< IMU register map */
#include <Wire.h>        /**< I2C communication library */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/** @brief GPIO pin for buzzer output */
#define BUZZER_PIN 2

/** @brief BLE Service UUID */
#define SERVICE_UUID "275dc6e0-dff5-4b56-9af0-584a5768a02a"
/** @brief Button characteristic UUID */
#define BUTTON_CHAR_UUID "b51bd845-2910-4f84-b062-d297ed286b1f"
/** @brief IMU characteristic UUID */
#define IMU_CHAR_UUID "0679c389-0d92-4604-aac4-664c43a51934"

// ---------------------------------------------------------------------------
// Global Objects
// ---------------------------------------------------------------------------

/** @brief Pointer to Button characteristic */
BLECharacteristic* buttonChar;
/** @brief Pointer to IMU characteristic */
BLECharacteristic* imuChar;
/** @brief Pointer to BLE server object */
BLEServer* server;
/** @brief Flag indicating central connection status */
volatile bool deviceConnected = false;
/** @brief Flag indicating BLE message received */
volatile bool bleMessageReceived = false;

// ---------------------------------------------------------------------------
// Forward Declarations
// ---------------------------------------------------------------------------
void IMUTask(void *pvParameters);
void ButtonRelayTask(void *pvParameters);
void BuzzerSetTask(void *pvParameters);

// ---------------------------------------------------------------------------
// BLE Server Callbacks
// ---------------------------------------------------------------------------

/**
 * @class ServerCallbacks
 * @brief Handles BLE server connect/disconnect events.
 */
class ServerCallbacks : public BLEServerCallbacks { 
  void onConnect(BLEServer* pServer) override { 
    deviceConnected = true; 
    pServer->getAdvertising()->stop(); /**< Stop advertising when connected */
  }
  void onDisconnect(BLEServer* pServer) override {
    deviceConnected = false;
    pServer->getAdvertising()->start(); /**< Restart advertising when disconnected */
  }
};

/**
 * @class ButtonCallbacks
 * @brief Handles write events to the Button characteristic.
 */
class ButtonCallbacks: public BLECharacteristicCallbacks {
   void onWrite(BLECharacteristic *pCharacteristic) override {
     bleMessageReceived = true; /**< Set flag when BLE write occurs */
    }
};

// ---------------------------------------------------------------------------
// FreeRTOS Synchronization
// ---------------------------------------------------------------------------

/** @brief Semaphore used to signal button events between tasks */
SemaphoreHandle_t xButtonSignalSemaphore;

// ---------------------------------------------------------------------------
// Setup
// ---------------------------------------------------------------------------

/**
 * @brief Arduino setup function.
 * Initializes Serial, I2C, BLE, creates GATT server and tasks.
 */
void setup() {
  Serial.begin(115200);
  delay(2000);
  pinMode(BUZZER_PIN, OUTPUT);
  ledcAttach(BUZZER_PIN, 1000, 11); /**< Configure buzzer PWM */

  Wire.begin();

  // Initialize BLE Device
  BLEDevice::init("ESP32 Server");
  BLEDevice::setMTU(185); /**< Increase MTU size for better throughput */

  // Create server and attach callbacks
  server = BLEDevice::createServer();
  server->setCallbacks(new ServerCallbacks());

  // Create BLE service
  BLEService* service = server->createService(SERVICE_UUID);

  // Create button characteristic (read/write)
  buttonChar = service->createCharacteristic(
    BUTTON_CHAR_UUID,
    BLECharacteristic::PROPERTY_READ |
    BLECharacteristic::PROPERTY_WRITE
  );

  // Create IMU characteristic (notify)
  imuChar = service->createCharacteristic(
    IMU_CHAR_UUID,
    BLECharacteristic::PROPERTY_NOTIFY
  );
  imuChar->addDescriptor(new BLE2902());
  buttonChar->setCallbacks(new ButtonCallbacks());

  // Start service & advertising
  service->start();
  BLEAdvertising* adv = server->getAdvertising();
  adv->addServiceUUID(SERVICE_UUID);
  adv->setScanResponse(true);
  adv->start();

  Serial.println("Peripheral ready. Type here to send to central.");

  // Create tasks
  xTaskCreate(IMUTask, "IMUTask", 4096, NULL, 5, NULL);

  xButtonSignalSemaphore = xSemaphoreCreateBinary();

  if (xButtonSignalSemaphore != NULL) {
    xTaskCreate(ButtonRelayTask, "ButtonRelayTask", 4096, NULL, 5, NULL);
    xTaskCreate(BuzzerSetTask, "BuzzerSetTask", 4096, NULL, 5, NULL);
  }
}

// ---------------------------------------------------------------------------
// Tasks
// ---------------------------------------------------------------------------

/**
 * @brief Task that relays button press signals from BLE writes.
 * @param pvParameters FreeRTOS task parameter (unused).
 */
void ButtonRelayTask(void *pvParameters) {
  while(1) {
    if(bleMessageReceived) {
      bleMessageReceived = false;
      xSemaphoreGive(xButtonSignalSemaphore);
      Serial.println("write recieved!");
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

/**
 * @brief Task that toggles buzzer on/off when button events occur.
 * @param pvParameters FreeRTOS task parameter (unused).
 */
void BuzzerSetTask(void *pvParameters) {
  bool buzzerOn = false;
  while(1) {
    if(xSemaphoreTake(xButtonSignalSemaphore, portMAX_DELAY) == pdTRUE) {
      if (!buzzerOn) {
        buzzerOn = true;
        ledcWriteTone(BUZZER_PIN, 1000); /**< Turn buzzer on */
      } else {
        buzzerOn = false;
        ledcWriteTone(BUZZER_PIN, 0); /**< Turn buzzer off */
      }
    }
  }
}

/**
 * @brief Task that reads IMU data, processes orientation, and detects movement.
 * @details
 * - Reads accelerometer and gyroscope data
 * - Applies complementary filter for orientation
 * - Removes gravitational bias
 * - Computes linear acceleration magnitude
 * - Applies SMA filter to detect sustained movement
 * - Notifies central via BLE when movement starts/stops
 * 
 * @param pvParameters FreeRTOS task parameter (unused).
 */
void IMUTask(void *pvParameters) {
  // Create IMU struct to hold IMU Data
  struct imu* imu_data = imu_init();

  // Timing variables
  float previousTime;
  float currentTime;
  float elapsedTime;
  float printTime;

  // Movement detection
  bool movement = false;

  // Error correction
  float xErrGy, yErrGy, zErrGy;
  float xErrAc, yErrAc, zErrAc;

  // Orientation and acceleration variables
  float accAngleX, accAngleY, gyroAngleX, gyroAngleY, gyroAngleZ;
  float roll, pitch;
  float roll_rad, pitch_rad;
  float gX, gY, gZ;
  float linAccX, linAccY, linAccZ;
  float linMag;

  // SMA filter buffer
  float buffer[32] = {0};
  float sum = 0;
  float avg = 0;
  int i = 0;

  // BLE notification variables
  uint8_t moveSignal = 0;
  char str[6];

  // IMU setup and calibration
  Wire.begin();
  imu_i2c();
  calibrateGyro(&xErrGy, &yErrGy, &zErrGy);
  calibrateAccel(&xErrAc, &yErrAc, &zErrAc);
  printTime = millis();
  currentTime = millis();
  previousTime = currentTime;

  for (;;) {
    // Timing
    currentTime = millis();
    elapsedTime = (currentTime - previousTime) / 1000.0;
    previousTime = currentTime;

    // Read accelerometer and compute angles
    imu_read_accel(&(imu_data->AccX), 0, &(imu_data->AccY), 0, &(imu_data->AccZ), 0);
    accAngleX = atan2(imu_data->AccY, imu_data->AccZ) * 180 / PI - xErrAc;
    accAngleY = atan2(-imu_data->AccX, sqrt(imu_data->AccY*imu_data->AccY + imu_data->AccZ*imu_data->AccZ)) * 180 / PI + yErrAc;
    roll = accAngleX;
    pitch = accAngleY;

    // Read gyro
    imu_read_gyro(&(imu_data->GyroX), xErrGy, &(imu_data->GyroY), yErrGy, &(imu_data->GyroZ), zErrGy);

    // Complementary filter
    roll  = 0.98*(roll + imu_data->GyroX * elapsedTime) + (1-0.98)*accAngleX;
    pitch = 0.98*(pitch + imu_data->GyroY * elapsedTime) + (1-0.98)*accAngleY;

    // Convert to radians
    roll_rad = roll * PI / 180.0;
    pitch_rad = pitch * PI / 180.0;

    // Compute gravitational bias
    gX = -sin(pitch_rad);
    gY = sin(roll_rad) * cos(pitch_rad);
    gZ = cos(roll_rad) * cos(pitch_rad);

    // Linear acceleration (gravity-compensated)
    linAccX = imu_data->AccX - gX;
    linAccY = imu_data->AccY - gY;
    linAccZ = imu_data->AccZ - gZ;

    // Compute magnitude
    linMag = sqrt(linAccX*linAccX + linAccY*linAccY + linAccZ*linAccZ);

    // SMA smoothing (buffer length = 32)
    if (buffer[i+1] != 0)
      sum -= buffer[i];
  
    buffer[i] = linMag;
    sum += buffer[i]; 
    i = (i+1) % 32;
    avg = sum / 32;

    // Notify movement start
    if (avg >= 0.25 && !movement) {
      moveSignal = 1;
      Serial.println("movement detected!");
      sprintf(str, "%d", moveSignal);
      imuChar->setValue((uint8_t*)&str, strlen(str));
      imuChar->notify();
      movement = true;
    }

    // Notify movement stop
    if (avg <= 0.05 && movement) {
      movement = false;
      moveSignal = 0;
      Serial.println("stopped moving!");
      sprintf(str, "%d", moveSignal);
      imuChar->setValue((uint8_t*)&str, strlen(str));
      imuChar->notify();
    }
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

/**
 * @brief Arduino loop function (unused, tasks handle logic).
 */
void loop() {
}
