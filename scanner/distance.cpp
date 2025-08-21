/**
 * @file distance.cpp
 * @brief Implementation of RSSI averaging and distance estimation utilities.
 *
 * This file provides definitions for global parameters and functions
 * that help smooth RSSI (Received Signal Strength Indicator) values
 * and estimate distance based on the log-distance path loss model.
 *
 * The algorithm uses exponential smoothing for RSSI and applies
 * the following formula for distance estimation:
 *
 * \f[
 *   d = 10^{\frac{(txPower - rssi)}{10 \cdot n}}
 * \f]
 *
 * where:
 * - \f$d\f$ is the estimated distance in meters,
 * - \f$rssi\f$ is the measured signal strength in dBm,
 * - \f$txPower\f$ is the RSSI at 1 meter reference distance,
 * - \f$n\f$ is the path-loss exponent.
 */

#include <math.h>
#include "distance.h"

// ============================================================================
// Global Variables
// ============================================================================

/**
 * @brief Flag to indicate whether the RSSI average has been initialized.
 */
bool hasAvg = false;

/**
 * @brief Stores the current smoothed RSSI average.
 */
float rssiAvg = 0.0f;

/**
 * @brief Transmitter power at 1 meter (in dBm).
 *
 * This value should be tuned based on calibration measurements.
 */
float txPower = -52.0f;   // tune later

/**
 * @brief Path-loss exponent (environmental factor).
 *
 * Typical values:
 * - ~2.0 in free space
 * - 2.7–4.0 in indoor/obstructed environments
 *
 * Should be tuned experimentally for best accuracy.
 */
float nFactor = 2.5f;     // tune later

// ============================================================================
// Functions
// ============================================================================

/**
 * @brief Updates the running average of the received signal strength indicator (RSSI).
 *
 * Applies exponential smoothing with a fixed factor \f$\alpha = 0.2\f$.
 * This reduces noise in instantaneous RSSI readings.
 *
 * @param[in] rssi Current measured RSSI value (in dBm).
 */
void updateRssiAvg(int rssi) {
  const float alpha = 0.2f;
  if (!hasAvg) {
    rssiAvg = rssi;
    hasAvg = true;
  } else {
    rssiAvg = alpha * rssi + (1.0f - alpha) * rssiAvg;
  }
}

/**
 * @brief Estimates the distance (in meters) from RSSI using the log-distance path loss model.
 *
 * Computes an approximate distance to the transmitter based on the difference
 * between the measured RSSI and the known transmit power at 1 meter.
 *
 * @param[in] rssi     Received signal strength indicator (in dBm).
 * @param[in] txPower  Transmitter power at 1 meter (in dBm).
 * @param[in] n        Path-loss exponent (environmental factor).
 * @return Estimated distance in meters.
 */
float estimateDistanceMeters(float rssi, float txPower, float n) {
  return powf(10.0f, (txPower - rssi) / (10.0f * n));
}
