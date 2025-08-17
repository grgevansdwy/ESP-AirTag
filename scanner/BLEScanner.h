#pragma once
#include <Arduino.h>
#include <BLEDevice.h>
#include "freertos/FreeRTOS.h"   
#include "freertos/queue.h"     

// Globals owned by this module (declare extern if you also use them elsewhere)
extern volatile bool connected;
extern BLEClient* client;
extern BLERemoteCharacteristic* btnRemoteChar;
extern BLERemoteCharacteristic* imuRemoteChar;

// Set these UUIDs from your main (or define them here if you prefer)
extern BLEUUID svcUUID;
extern BLEUUID btnUUID;
extern BLEUUID imuUUID;

// Provide your IMU queue so notifications can push 0/1 into it
void setIMUQueue(QueueHandle_t q);



// Write one byte (0/1) to the button characteristic
bool writeBtnState(bool pressed);

// Connect, discover, validate, and subscribe to IMU notifications
bool connectToPeripheral(BLEAddress addr); 
