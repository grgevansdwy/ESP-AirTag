// ---- FREERTOS ----
#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ---- BLE CLIENT ----
#include <BLEDevice.h>    // ESP32 Arduino core 3.x -> NimBLE backend
#include <BLEUtils.h>
#include <BLEClient.h>
#include <BLEScan.h>

#include "BLEScanner.h"

// ===== Module globals =====
volatile bool connected = false;
BLEClient* client = nullptr;
BLERemoteCharacteristic* btnRemoteChar = nullptr;
BLERemoteCharacteristic* imuRemoteChar = nullptr;

// Set these from your main .ino or define here:
BLEUUID svcUUID;  // e.g., BLEUUID("275dc6e0-dff5-4b56-9af0-584a5768a02a")
BLEUUID btnUUID;  // e.g., BLEUUID("b51b...")  // WRITE / WRITE_NR
BLEUUID imuUUID;  // e.g., BLEUUID("0679...")  // NOTIFY

// IMU queue (mailbox). NOTE: xQueueOverwrite requires queue length == 1.
static QueueHandle_t gIMUQ = nullptr;
void setIMUQueue(QueueHandle_t q) { gIMUQ = q; }

// ---- Notification callback ----
// NimBLE/Arduino expects: (BLERemoteCharacteristic*, uint8_t*, size_t, bool)
static void onImuNotify(BLERemoteCharacteristic* /*rc*/, uint8_t* pData, size_t length, bool /*isNotify*/) {
  if (!gIMUQ || length == 0) return;

  Serial.write(pData, length);
  Serial.println();

  // Find the first '0' or '1' in the payload
  uint8_t imuFlag = 0;
  bool found = false;
  for (size_t i = 0; i < length; ++i) {
    if (pData[i] == '0') { imuFlag = 0; found = true; break; }
    if (pData[i] == '1') { imuFlag = 1; found = true; break; }
  }
  if (!found) return;  // ignore unexpected chars

  // latest wins (queue length must be 1)
  xQueueOverwrite(gIMUQ, &imuFlag);
}


// ---- Write button state (single byte) ----
bool writeBtnState(bool pressed) {
  if (!connected || !client || !btnRemoteChar) return false;

  const uint8_t value = pressed ? 1 : 0;

  // Prefer fast, fire-and-forget if allowed; fallback to with-response.
  if (btnRemoteChar->canWriteNoResponse()) {
    // In this core: third param true => NO RESPONSE, false => WITH RESPONSE
    btnRemoteChar->writeValue((uint8_t*)&value, 1, /*noResp=*/true);
    return true;
  }
  if (btnRemoteChar->canWrite()) {
    btnRemoteChar->writeValue((uint8_t*)&value, 1, /*noResp=*/false);
    return true;
  }
  return false; // not writeable
}

// ---- Connect, discover, validate, subscribe ----
bool connectToPeripheral(BLEAddress addr) {
  client = BLEDevice::createClient();
  BLEDevice::setMTU(185); // request larger MTU; peer may accept/ignore

  Serial.print("Connecting to: ");
  Serial.println(addr.toString().c_str());

  if (!client->connect(addr)) {
    Serial.println("Connection failed.");
    return false;
  }

  BLERemoteService* service = client->getService(svcUUID);
  if (!service) {
    Serial.println("Service not found.");
    client->disconnect();
    return false;
  }

  // ---- Button char: must be WRITE or WRITE_NO_RESPONSE ----
  btnRemoteChar = service->getCharacteristic(btnUUID);
  if (!btnRemoteChar) {
    Serial.println("Button characteristic not found.");
    client->disconnect();
    return false;
  }
  if (!(btnRemoteChar->canWrite() || btnRemoteChar->canWriteNoResponse())) {
    Serial.println("Button characteristic is not writeable.");
    client->disconnect();
    return false;
  }

  // ---- IMU char: must support NOTIFY ----
  imuRemoteChar = service->getCharacteristic(imuUUID);
  if (!imuRemoteChar) {
    Serial.println("IMU characteristic not found.");
    client->disconnect();
    return false;
  }
  if (!imuRemoteChar->canNotify()) {
    Serial.println("IMU characteristic does not support notify.");
    client->disconnect();
    return false;
  }

  // Subscribe to IMU notifications
  imuRemoteChar->registerForNotify(onImuNotify);

  connected = true;
  Serial.printf("Connected. Button write mode: %s. Subscribed to IMU.\n",
                btnRemoteChar->canWriteNoResponse() ? "WriteWithoutResponse" : "WriteWithResponse");
  return true;
}
