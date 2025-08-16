#include <BLEDevice.h> // initializes BLE general function
#include <BLEServer.h> // Create peripheral/role funciton
#include <BLEUtils.h> // BLE helpers such as UUID handling
#include <BLE2902.h> // Descriptor class for Client Characteristic Configuration Descriptor (CCCD).
#include "IMU.h"
#include "IMU_STRUCT.h"
#include "IMU_REGISTER_MAP.h"
#include <Wire.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LDR_PIN 4

#define SERVICE_UUID "275dc6e0-dff5-4b56-9af0-584a5768a02a" //service UUID
#define TX_CHAR_UUID "b51bd845-2910-4f84-b062-d297ed286b1f" // char serial monitor
#define IMU_CHAR_UUID "0679c389-0d92-4604-aac4-664c43a51934" // char LDR

// Global Objexts
BLECharacteristic* txChar; // Pointer to characteristic objects (UUID, value, properties, descriptor)
BLECharacteristic* imuChar; // Pointer to characteristic objects (UUID, value, properties, descriptor)
BLEServer* server; // pointer to profile/server (multiple services, connection handling)
volatile bool deviceConnected = false; // track whether the central is connected

// this is an override of a virtual function BLEServerCallBacks from the BLE Library that looks like this,
// virtual void onConnect(BLEServer* pServer) {}
// virtual void onDisconnect(BLEServer* pServer) {}
void IMUTask(void *pvParameters);

// CONNECT / DISCONNECT HANDLING
// When there is a central connected, it will automatically call onConnect, and the other way
class ServerCallbacks : public BLEServerCallbacks { 
  // When a central connect, set the deviceConnected to true
  void onConnect(BLEServer* pServer) override { 
    deviceConnected = true; 
    pServer->getAdvertising()->stop(); // prevent additional connections
  }
  // When a central disconnect, set the deviceConnected to false, 
  void onDisconnect(BLEServer* pServer) override {
    deviceConnected = false;
    pServer->getAdvertising()->start();
  }
};

void setup() {
  Serial.begin(115200);
  delay(2000);

  Wire.begin();

  // 1. INITIALIZE BLUETOOTH
  // Initialize BLE Device with the name of ESP32 Server, that is what the central will see.
  BLEDevice::init("ESP32 Server");
  // default is 23 bytes, but requesting for 185 bytes to central for better transmission
  BLEDevice::setMTU(185);
  // Creates the BLE server (peripheral role) and attaches your custom connection callbacks.

  // 2. CREATE GATT STRUCTURE (Server --> Service-->Characteristic + Descriptor)
  server = BLEDevice::createServer();
  server->setCallbacks(new ServerCallbacks());
  // Creates a BLE service with UUID.
  BLEService* service = server->createService(SERVICE_UUID);
  // Create the characteristic with the characteristic UUID and notify property
  txChar = service->createCharacteristic(
    TX_CHAR_UUID,
    BLECharacteristic::PROPERTY_NOTIFY
  );
  // Add the descriptor for CCCD to be able to subscribe/unsubscribe to notification
  txChar->addDescriptor(new BLE2902());
  // add photoresistor char
  imuChar = service->createCharacteristic(
    IMU_CHAR_UUID,
    BLECharacteristic::PROPERTY_NOTIFY
  );
  // Add the descriptor for CCCD to be able to subscribe/unsubscribe to notification
  imuChar->addDescriptor(new BLE2902());

  // 3. Advertise
  // Enable the service (giving the all the data to the server)
  service->start();
  // Get the advertising object from the lirbary
  BLEAdvertising* adv = server->getAdvertising();
  // add the service UUID into the 
  adv->addServiceUUID(SERVICE_UUID);
  // If the central scan, then it will see more data
  adv->setScanResponse(true);
  // start the advertising (broadcasting information)
  adv->start();

  Serial.println("Peripheral ready. Type here to send to central.");

  xTaskCreate(IMUTask, "IMUTask", 4096, NULL, 5, NULL);
  xTaskCreate()
}

// void notifyChunks(uint8_t* data, size_t len) {
//   // Initialize payload to 180, make it conservative to be less than 185 slightly
//   const size_t kChunk = 180;
//   size_t offset = 0;
//   // As long as 
//   while (offset < len) {
//     size_t n = min(kChunk, len - offset);
//     txChar->setValue(data + offset, n);
//     txChar->notify();
//     offset += n;
//     delay(5); // brief pacing so central isn’t overwhelmed
//   }
// }

void IMUTask(void *pvParameters) {
  // Create IMU struct to hold IMU Data
  struct imu* imu_data = imu_init();

  // Timing for processing linear acceleration
  float previousTime;
  float currentTime;
  float elapsedTime;
  float printTime;

  // boolean to set when device is moving.
  bool movement = false;

  // error correction variables.
  float xErrGy, yErrGy, zErrGy;
  float xErrAc, yErrAc, zErrAc;

  // angular acceleration variables.
  float accAngleX, accAngleY, gyroAngleX, gyroAngleY, gyroAngleZ;

  // Orientation variables.
  float roll, pitch;

  // change from degrees to radians.
  float roll_rad, pitch_rad;

  // gravitational bias.
  float gX, gY, gZ;

  // linear acceleration adjusted for graviational bias.
  float linAccX, linAccY, linAccZ;
  
  // vector for magnitude of linear aceleration.
  float linMag;

  // SMA buffer.
  float buffer[32] = {0};
  float sum = 0;
  float avg = 0;
  int i = 0;

  // variable being sent through BLE
  uint8_t moveSignal = 0;
  char str[6];


  // IMU and I2C setups.
  Wire.begin();
  imu_i2c();
  calibrateGyro(&xErrGy, &yErrGy, &zErrGy);
  calibrateAccel(&xErrAc, &yErrAc, &zErrAc);
  printTime = millis();
  currentTime = millis();
  previousTime = currentTime;

  for (;;) {
    currentTime = millis();            // Current time actual time read
    elapsedTime = (currentTime - previousTime) / 1000.0;
    previousTime = currentTime;

    imu_read_accel(&(imu_data->AccX), 0, &(imu_data->AccY), 0, &(imu_data->AccZ), 0);
    accAngleX = atan2(imu_data->AccY, imu_data->AccZ) * 180 / PI - xErrAc;
    accAngleY = atan2(-imu_data->AccX, sqrt(imu_data->AccY*imu_data->AccY + imu_data->AccZ*imu_data->AccZ)) * 180 / PI + yErrAc;
    roll = accAngleX;
    pitch = accAngleY;

    imu_read_gyro(&(imu_data->GyroX), xErrGy, &(imu_data->GyroY), yErrGy, &(imu_data->GyroZ), zErrGy);

    // Complementary filter - combine acceleromter and gyro angle values
    roll  = 0.98*(roll + imu_data->GyroX * elapsedTime) + (1-0.98)*accAngleX;
    pitch = 0.98*(pitch + imu_data->GyroY * elapsedTime) + (1-0.98)*accAngleY;

    // changing pitch and roll into radian values.
    roll_rad = roll * PI / 180.0;
    pitch_rad = pitch * PI / 180.0;

    // gravitational bias based on orientation.
    gX = -sin(pitch_rad);
    gY = sin(roll_rad) * cos(pitch_rad);
    gZ = cos(roll_rad) * cos(pitch_rad);

    // acceleration adjusted for gravitational bias.
    linAccX = imu_data->AccX - gX;
    linAccY = imu_data->AccY - gY;
    linAccZ = imu_data->AccZ - gZ;

    // normalized vector for non-directional. (scalar)
    linMag = sqrt(linAccX*linAccX + linAccY*linAccY + linAccZ*linAccZ);

    // SMA logic for smoothing (32 so that impulses are smoothed out.)
    if (buffer[i+1] != 0)
      sum -= buffer[i];
  
    buffer[i] = linMag;
    sum += buffer[i]; 
    i = (i+1) % 32;
    avg = sum / 32;

    // notifying logic. 
    if (avg >= 0.5 && !movement) {
      moveSignal = 1;
      Serial.println("movement detected!");
      sprintf(str, "%d", moveSignal);
      imuChar->setValue((uint8_t*)&str, strlen(str));
      imuChar->notify();
      movement = true;
    }

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

void loop() {
  // You could periodically update the value here
  // the line string
  // static String line;
  // // when in the serial input > 0, then it will keep checking to the line inside
  // while (Serial.available()) {
  //   // read by char to determine enter
  //   char c = (char)Serial.read();
  //   if (c == '\r') continue;
  //   if (c == '\n') {
  //     // if it is connected and have a length more than 0, then notify
  //     if (deviceConnected && line.length() > 0) {
  //       notifyChunks((uint8_t*)line.c_str(), line.length());
  //       // enter
  //       const char nl = '\n';
  //       notifyChunks((uint8_t*)&nl, 1);
  //     }
  //     line = "";
  //   } else {
  //     // accumulate the character
  //     line += c;
  //     if (line.length() >= 1000) {
  //       if (deviceConnected) notifyChunks((uint8_t*)line.c_str(), line.length());
  //       line = "";
  //     }
  //   }
  // }
}