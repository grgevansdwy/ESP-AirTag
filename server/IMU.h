/**
 * @file IMU.h
 * @brief Header file for IMU initialization, calibration, and sensor data reading.
 *
 * This file provides function declarations for initializing the IMU structure,
 * reading accelerometer and gyroscope data, and performing calibration routines.
 */

/**
 * @brief Initialize the IMU structure.
 *
 * Allocates memory for the IMU structure and initializes all sensor values to zero.
 *
 * @return Pointer to the allocated and initialized IMU structure.
 */
struct imu* imu_init(void);

/**
 * @brief Read accelerometer data from the IMU.
 *
 * Reads raw accelerometer values from the IMU over I2C, applies scaling, and compensates
 * for calibration error offsets.
 *
 * @param[out] xAccel Pointer to store the X-axis acceleration value.
 * @param[in]  xErr   Error offset for X-axis acceleration.
 * @param[out] yAccel Pointer to store the Y-axis acceleration value.
 * @param[in]  yErr   Error offset for Y-axis acceleration.
 * @param[out] zAccel Pointer to store the Z-axis acceleration value.
 * @param[in]  zErr   Error offset for Z-axis acceleration.
 */
void imu_read_accel(float* xAccel, float xErr, float* yAccel, float yErr, float* zAccel, float zErr);

/**
 * @brief Read gyroscope data from the IMU.
 *
 * Reads raw gyroscope values from the IMU over I2C, applies scaling, and compensates
 * for calibration error offsets.
 *
 * @param[out] xGyro Pointer to store the X-axis gyroscope value.
 * @param[in]  xErr  Error offset for X-axis gyroscope.
 * @param[out] yGyro Pointer to store the Y-axis gyroscope value.
 * @param[in]  yErr  Error offset for Y-axis gyroscope.
 * @param[out] zGyro Pointer to store the Z-axis gyroscope value.
 * @param[in]  zErr  Error offset for Z-axis gyroscope.
 */
void imu_read_gyro(float* xGyro, float xErr, float* yGyro, float yErr, float* zGyro, float zErr);

/**
 * @brief Calibrate the gyroscope by computing offsets.
 *
 * Reads multiple samples from the gyroscope to estimate and compute the error offsets
 * for each axis. These offsets can later be used for compensation during sensor readings.
 *
 * @param[out] xErr Pointer to store the X-axis gyroscope offset.
 * @param[out] yErr Pointer to store the Y-axis gyroscope offset.
 * @param[out] zErr Pointer to store the Z-axis gyroscope offset.
 */
void calibrateGyro(float* xErr, float* yErr, float* zErr);

/**
 * @brief Calibrate the accelerometer by computing offsets.
 *
 * Reads multiple samples from the accelerometer to estimate and compute the error offsets
 * for each axis. These offsets can later be used for compensation during sensor readings.
 *
 * @param[out] xErr Pointer to store the X-axis accelerometer offset.
 * @param[out] yErr Pointer to store the Y-axis accelerometer offset.
 * @param[out] zErr Pointer to store the Z-axis accelerometer offset.
 */
void calibrateAccel(float* xErr, float* yErr, float* zErr);

/**
 * @brief Initialize I2C communication for the IMU.
 *
 * Configures and sets up the I2C interface to enable communication with the IMU sensor.
 */
void imu_i2c(void);
