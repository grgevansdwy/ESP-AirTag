#include "IMU_STRUCT.h"
#include "IMU.h"
#include "IMU_REGISTER_MAP.h"
#include <Wire.h>

/**
 * @brief Initializes a new IMU structure.
 *
 * Allocates memory for an IMU structure and sets all accelerometer
 * and gyroscope values to zero.
 *
 * @return Pointer to the initialized imu structure.
 */
struct imu* imu_init(void) {
  struct imu* imu = (struct imu*) malloc(sizeof(struct imu));
  imu->AccX = imu->AccY = imu->AccZ = 0.0;
  imu->GyroX = imu->GyroY = imu->GyroZ = 0.0;
  return imu;
}

/**
 * @brief Reads accelerometer data from the IMU.
 *
 * Reads raw accelerometer values via I2C, converts them into g-units,
 * and applies error offsets.
 *
 * @param[out] xAccel Pointer to store X-axis acceleration.
 * @param[in]  xErr   X-axis calibration error.
 * @param[out] yAccel Pointer to store Y-axis acceleration.
 * @param[in]  yErr   Y-axis calibration error.
 * @param[out] zAccel Pointer to store Z-axis acceleration.
 * @param[in]  zErr   Z-axis calibration error.
 */
void imu_read_accel(float* xAccel, float xErr, float* yAccel, float yErr, float* zAccel, float zErr) {
    Wire.beginTransmission(0x68);
    Wire.write(REG_ACCEL_XOUT_H);
    Wire.endTransmission(false);
    Wire.requestFrom(0x68, 6, true);
    *xAccel = (int16_t)(Wire.read() << 8 | Wire.read())/16384.0 - xErr;
    *yAccel = (int16_t)(Wire.read() << 8 | Wire.read())/16384.0 - yErr;
    *zAccel = (int16_t)(Wire.read() << 8 | Wire.read())/16384.0 - zErr;
}

/**
 * @brief Reads gyroscope data from the IMU.
 *
 * Reads raw gyroscope values via I2C, converts them into degrees/sec,
 * and applies error offsets.
 *
 * @param[out] xGyro Pointer to store X-axis gyroscope value.
 * @param[in]  xErr  X-axis calibration error.
 * @param[out] yGyro Pointer to store Y-axis gyroscope value.
 * @param[in]  yErr  Y-axis calibration error.
 * @param[out] zGyro Pointer to store Z-axis gyroscope value.
 * @param[in]  zErr  Z-axis calibration error.
 */
void imu_read_gyro(float* xGyro,float xErr, float* yGyro, float yErr, float* zGyro, float zErr) {
    Wire.beginTransmission(0x68);
    Wire.write(REG_GYRO_XOUT_H);
    Wire.endTransmission(false);
    Wire.requestFrom(0x68, 6, true);
    *xGyro = (int16_t)(Wire.read() << 8 | Wire.read())/131.0 - xErr;
    *yGyro = (int16_t)(Wire.read() << 8 | Wire.read())/131.0 - yErr;
    *zGyro = (int16_t)(Wire.read() << 8 | Wire.read())/131.0 - zErr;
}

/**
 * @brief Calibrates the gyroscope by averaging multiple samples.
 *
 * Collects 1000 gyroscope readings and computes the average error
 * for each axis, which can be subtracted in later readings.
 *
 * @param[out] xErr Pointer to store average X-axis error.
 * @param[out] yErr Pointer to store average Y-axis error.
 * @param[out] zErr Pointer to store average Z-axis error.
 */
void calibrateGyro(float* xErr, float* yErr, float* zErr) {
  float val1, val2, val3;
  for (int i = 0; i < 1000; i ++) {
    imu_read_gyro(&val1, 0.0, &val2, 0.0, &val3, 0.0);
    *xErr += val1;
    *yErr += val2;
    *zErr += val3;
  }
  *xErr = *xErr/1000;
  *yErr = *yErr/1000;
  *zErr = *zErr/1000;
}

/**
 * @brief Calibrates the accelerometer by averaging multiple samples.
 *
 * Collects 1000 accelerometer readings and computes the average error
 * for each axis, which can be subtracted in later readings.
 *
 * @param[out] xErr Pointer to store average X-axis error.
 * @param[out] yErr Pointer to store average Y-axis error.
 * @param[out] zErr Pointer to store average Z-axis error.
 */
void calibrateAccel(float* xErr, float* yErr, float* zErr) {
  float val1, val2, val3;
  for (int i = 0; i < 1000; i ++) {
    imu_read_accel(&val1, 0.0, &val2, 0.0, &val3, 0.0);
    *xErr += val1;
    *yErr += val2;
    *zErr += val3;
  }
  *xErr = *xErr/1000;
  *yErr = *yErr/1000;
  *zErr = *zErr/1000;
}

/**
 * @brief Initializes the IMU over I2C.
 *
 * Wakes up the IMU by writing to the power management register.
 * This must be called before reading accelerometer or gyroscope data.
 */
void imu_i2c(void) {
  Wire.beginTransmission(0x68);
  Wire.write(REG_PWR_MGMT_1);
  Wire.write(0x00);
  Wire.endTransmission(true);
}