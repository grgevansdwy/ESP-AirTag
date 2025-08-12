#include <BLEDevice.h>    // BLE core (ESP32 Arduino core 3.x uses NimBLE backend)
#include <BLEUtils.h>
#include <BLEClient.h>
#include <BLEScan.h>

// Your custom UUIDs — must match the peripheral
#define SERVICE_UUID "275dc6e0-dff5-4b56-9af0-584a5768a02a"
#define TX_CHAR_UUID "b51bd845-2910-4f84-b062-d297ed286b1f"

// Convert to BLEUUID so comparisons are easy
static BLEUUID svcUUID(SERVICE_UUID);
static BLEUUID txUUID(TX_CHAR_UUID);

volatile bool connected = false;

BLEClient* client = nullptr;
BLERemoteCharacteristic* txRemoteChar = nullptr;

// Notification callback: prints incoming bytes
void onNotify(BLERemoteCharacteristic* /*pRemoteCharacteristic*/,
              uint8_t* pData,
              size_t length,
              bool /*isNotify*/) {
  Serial.write(pData, length);
}

bool connectToPeripheral(BLEAddress addr) {
  client = BLEDevice::createClient();
  BLEDevice::setMTU(185); // request larger MTU (peer may accept/deny)

  Serial.print("Connecting to: ");
  Serial.println(addr.toString().c_str());

  // 1) CONNECT FIRST
  if (!client->connect(addr)) {
    Serial.println("Connection failed.");
    return false;
  }

  // 2) DISCOVER SERVICE & CHAR
  BLERemoteService* service = client->getService(svcUUID);
  if (!service) {
    Serial.println("Service not found.");
    client->disconnect();
    return false;
  }

  txRemoteChar = service->getCharacteristic(txUUID);
  if (!txRemoteChar) {
    Serial.println("TX characteristic not found.");
    client->disconnect();
    return false;
  }

  if (!txRemoteChar->canNotify()) {
    Serial.println("TX char does not support notify.");
    client->disconnect();
    return false;
  }

  // 3) SUBSCRIBE TO NOTIFICATIONS
  // ESP32 core 3.x: registerForNotify returns void (not bool)
  txRemoteChar->registerForNotify(onNotify); // enables CCCD + stores callback

  connected = true;
  Serial.println("Connected and subscribed. Waiting for messages...");
  return true;
}

void setup() {
  Serial.begin(115200);
  delay(200);

  BLEDevice::init("ESP32-UART-Central");

  // Configure scanner
  BLEScan* scan = BLEDevice::getScan();
  scan->setActiveScan(true);
  scan->setInterval(80); // 80 * 0.625ms = 50ms period
  scan->setWindow(40);   // 40 * 0.625ms = 25ms window (50% duty)

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
}

void loop() {
  // Auto-reconnect if disconnected
  if (connected && client && !client->isConnected()) {
    Serial.println("Disconnected. Rescanning...");
    connected = false;
    txRemoteChar = nullptr;
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

  delay(50);
}
