#include <BLEDevice.h> // initializes BLE general function
#include <BLEServer.h> // Create peripheral/role funciton
#include <BLEUtils.h> // BLE helpers such as UUID handling
#include <BLE2902.h> // Descriptor class for Client Characteristic Configuration Descriptor (CCCD).

#define SERVICE_UUID "275dc6e0-dff5-4b56-9af0-584a5768a02a" //service UUID
#define TX_CHAR_UUID "b51bd845-2910-4f84-b062-d297ed286b1f" // server to client notification

// Global Objexts
BLECharacteristic* txChar; // Pointer to characteristic objects (UUID, value, properties, descriptor)
BLEServer* server; // pointer to profile/server (multiple services, connection handling)
volatile bool deviceConnected = false; // track whether the central is connected

// this is an override of a virtual function BLEServerCallBacks from the BLE Library that looks like this,
// virtual void onConnect(BLEServer* pServer) {}
// virtual void onDisconnect(BLEServer* pServer) {}

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
}

void notifyChunks(uint8_t* data, size_t len) {
  // Initialize payload to 180, make it conservative to be less than 185 slightly
  const size_t kChunk = 180;
  size_t offset = 0;
  // As long as 
  while (offset < len) {
    size_t n = min(kChunk, len - offset);
    txChar->setValue(data + offset, n);
    txChar->notify();
    offset += n;
    delay(5); // brief pacing so central isn’t overwhelmed
  }
}

void loop() {
  // You could periodically update the value here
  // the line string
  static String line;
  // when in the serial input > 0, then it will keep checking to the line inside
  while (Serial.available()) {
    // read by char to determine enter
    char c = (char)Serial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      // if it is connected and have a length more than 0, then notify
      if (deviceConnected && line.length() > 0) {
        notifyChunks((uint8_t*)line.c_str(), line.length());
        // enter
        const char nl = '\n';
        notifyChunks((uint8_t*)&nl, 1);
      }
      line = "";
    } else {
      // accumulate the character
      line += c;
      if (line.length() >= 1000) {
        if (deviceConnected) notifyChunks((uint8_t*)line.c_str(), line.length());
        line = "";
      }
    }
  }
}
