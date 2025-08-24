/**
 * @file BLEScanner.h
 * @brief BLE client interface for connecting to a peripheral, handling button writes,
 *        and subscribing to IMU notifications.
 *
 * This module provides the necessary global variables and function declarations
 * to manage BLE client connections, discover services and characteristics, and
 * handle communication with a remote BLE peripheral.
 */

#pragma once
#include <Arduino.h>
#include <BLEDevice.h>
#include "freertos/FreeRTOS.h"   
#include "freertos/queue.h"     

// ============================================================================
// Global Variables
// ============================================================================

/**
 * @brief Indicates whether the BLE client is currently connected to a peripheral.
 */
extern volatile bool connected;

/**
 * @brief Pointer to the active BLE client instance.
 */
extern BLEClient* client;

/**
 * @brief Remote characteristic pointer for the button service.
 */
extern BLERemoteCharacteristic* btnRemoteChar;

/**
 * @brief Remote characteristic pointer for the IMU service.
 */
extern BLERemoteCharacteristic* imuRemoteChar;

/**
 * @brief UUID for the target BLE service.
 *
 * Must be set from the main application.
 */
extern BLEUUID svcUUID;

/**
 * @brief UUID for the button characteristic.
 *
 * Must be set from the main application.
 */
extern BLEUUID btnUUID;

/**
 * @brief UUID for the IMU characteristic.
 *
 * Must be set from the main application.
 */
extern BLEUUID imuUUID;

// ============================================================================
// Functions
// ============================================================================

/**
 * @brief Provide a FreeRTOS queue for IMU notifications.
 *
 * This queue is used to receive IMU status updates (e.g., motion detected as 0/1).
 *
 * @param[in] q Queue handle to store IMU notification values.
 */
void setIMUQueue(QueueHandle_t q);

/**
 * @brief Write a button state (pressed or released) to the remote button characteristic.
 *
 * Sends a single byte (0 or 1) to the peripheral.
 *
 * @param[in] pressed Boolean indicating button state (true = pressed, false = released).
 * @return True if the write was successful, false otherwise.
 */
bool writeBtnState(bool pressed);

/**
 * @brief Connect to a BLE peripheral, discover services/characteristics,
 *        and subscribe to IMU notifications.
 *
 * Handles the connection process including validation of the required
 * service UUIDs and subscription setup.
 *
 * @param[in] addr BLE address of the peripheral to connect to.
 * @return True if connection and subscription succeed, false otherwise.
 */
bool connectToPeripheral(BLEAddress addr); 
