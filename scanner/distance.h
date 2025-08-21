/**
 * @file distance.h
 * @brief Functions and variables for RSSI averaging and distance estimation.
 *
 * Provides global parameters and helper functions for smoothing RSSI values
 * and estimating distance based on the log-distance path loss model.
 */

#pragma once
#include <stdint.h>

/**
 * @brief Flag indicating whether an RSSI average has been initialized.
 */
extern bool hasAvg;

/**
 * @brief Exponential moving average of the RSSI value.
 */
extern float rssiAvg;

/**
 * @brief Reference transmit power (measured RSSI at 1 meter distance).
 *
 * Typically determined through calibration, defaulting around -59 dBm
 * for many BLE devices.
 */
extern float txPower;

/**
 * @brief Path-loss exponent (environmental factor).
 *
 * Represents how quickly the signal attenuates with distance.
 * Common values:
 * - Free space: ~2.0
 * - Indoor: 2.7 – 4.0
 */
extern float nFactor;

/**
 * @brief Update the exponential moving average of the RSSI.
 *
 * Uses an exponential smoothing factor to stabilize the RSSI reading
 * against sudden fluctuations.
 *
 * @param[in] rssi Latest RSSI reading (in dBm).
 */
void updateRssiAvg(int rssi);

/**
 * @brief Estimate distance from RSSI using the log-distance path loss model.
 *
 * Calculates the approximate distance to a transmitter based on
 * received signal strength, reference power, and environmental factor.
 *
 * Formula:
 * \f[
 *   d = 10^{\frac{(txPower - rssi)}{10 \cdot n}}
 * \f]
 *
 * @param[in] rssi   Current RSSI reading (dBm).
 * @param[in] txPower Reference RSSI at 1 meter (dBm).
 * @param[in] n       Path-loss exponent.
 * @return Estimated distance in meters.
 */
float estimateDistanceMeters(float rssi, float txPower, float n);
