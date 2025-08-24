/**
 * @file BLEScanner.cpp
 * @brief Implementation of BLE client connection, button write, and IMU notification handling.
 *
 * This module provides functionality to:
 * - Connect to a BLE peripheral.
 * - Discover and validate required services and characteristics.
 * - Subscribe to IMU characteristic notifications.
 * - Write button state values to a remote BLE characteristic.
 *
 * It uses FreeRTOS for inter-task communication via queues and the ESP32 Arduino BLE stack.
 */

// ---- FREERTOS ----
#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ---- BLE CLIENT ----
#include <BLEDevice.h>    ///< ESP32 Arduino core 3.x uses NimBLE backend
#include <BLEUtils.h>
#include <BLEClient.h>
#include <BLEScan.h>

#include "BLEScanner.h"

// ============================================================================
// Module Globals
// ============================================================================

/**
 * @brief Indicates whether the client is connected to a BLE peripheral.
 */
volatile bool connected = false;

/**
 * @brief Pointer to the active BLE client instance.
 */
BLEClient* client = nullptr;

/**
 * @brief Remote characteristic pointer for the button characteristic.
 */
BLERemoteCharacteristic* btnRemoteChar = nullptr;

/**
 * @brief Remote characteristic pointer for the IMU characteristic.
 */
BLERemoteCharacteristic* imuRemoteChar = nullptr;

/**
 * @brief UUID for the BLE service (must be set by the application).
 */
BLEUUID svcUUID;

/**
 * @brief UUID for the button characteristic (must be set by the application).
 */
BLEUUID btnUUID;

/**
 * @brief UUID for the IMU characteristic (must be set by the application).
 */
BLEUUID imuUUID;

/**
 * @brief FreeRTOS queue for IMU notifications.
 *
 * The queue length must be 1 because `xQueueOverwrite` is used to always keep
 * the most recent IMU flag (0 or 1).
 */
static QueueHandle_t gIMUQ = nullptr;

/**
 * @brief Set the IMU notification queue.
 *
 * @param[in] q Queue handle that will receive IMU notification flags (0 or 1).
 */
void setIMUQueue(QueueHandle_t q) { gIMUQ = q; }

// ============================================================================
// Notification Callback
// ============================================================================

/**
 * @brief Callback for IMU characteristic notifications.
 *
 * This function processes incoming notification data, looks for a '0' or '1'
 * character in the payload, and writes the corresponding value into the IMU queue.
 *
 * @param[in] rc       Pointer to the remote characteristic (unused).
 * @param[in] pData    Pointer to the notification payload data.
 * @param[in] length   Length of the notification payload.
 * @param[in] isNotify Indicates if this is a notification (unused).
 */
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

  // Always overwrite: queue length must be 1
  xQueueOverwrite(gIMUQ, &imuFlag);
}

// ============================================================================
// Button Write
// ============================================================================

/**
 * @brief Write the button state to the remote BLE characteristic.
 *
 * Attempts to send the button state (pressed or released) as a single byte (0 or 1)
 * to the remote characteristic. Prefers "write without response" if available,
 * otherwise falls back to "write with response".
 *
 * @param[in] pressed True if button is pressed, false if released.
 * @return True if the value was successfully written, false otherwise.
 */
bool writeBtnState(bool pressed) {
  if (!connected || !client || !btnRemoteChar) return false;

  const uint8_t value = pressed ? 1 : 0;

  // Prefer fast, fire-and-forget if allowed; fallback to with-response.
  if (btnRemoteChar->canWriteNoResponse()) {
    btnRemoteChar->writeValue((uint8_t*)&value, 1, /*noResp=*/true);
    return true;
  }
  if (btnRemoteChar->canWrite()) {
    btnRemoteChar->writeValue((uint8_t*)&value, 1, /*noResp=*/false);
    return true;
  }
  return false; // not writeable
}

// ============================================================================
// Connect, Discover, Validate, Subscribe
// ============================================================================

/**
 * @brief Connect to a BLE peripheral and subscribe to IMU notifications.
 *
 * This function performs the following steps:
 * - Creates a BLE client and attempts to connect to the specified peripheral.
 * - Requests a larger MTU (may or may not be honored by peer).
 * - Discovers the configured service, button, and IMU characteristics.
 * - Validates that the button characteristic supports write/write-no-response.
 * - Validates that the IMU characteristic supports notify.
 * - Subscribes to IMU notifications.
 *
 * @param[in] addr BLE address of the peripheral to connect to.
 * @return True if connection and subscription succeed, false otherwise.
 */
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
