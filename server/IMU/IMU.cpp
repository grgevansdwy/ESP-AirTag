#include "IMU_STRUCT.h"
#include "IMU.h"
#include "IMU_REGISTER_MAP.h"
#include <Wire.h>

struct imu* imu_init(void) {
  struct imu* imu = (struct imu*) malloc(sizeof(struct imu));
  imu->AccX = imu->AccY = imu->AccZ = 0.0;
  imu->GyroX = imu->GyroY = imu->GyroZ = 0.0;
  return imu;
}

void imu_read_accel(float* xAccel, float xErr, float* yAccel, float yErr, float* zAccel, float zErr) {
    Wire.beginTransmission(0x68);
    Wire.write(REG_ACCEL_XOUT_H);
    Wire.endTransmission(false);
    Wire.requestFrom(0x68, 6, true);
    *xAccel = (int16_t)(Wire.read() << 8 | Wire.read())/16384.0 - xErr;
    *yAccel = (int16_t)(Wire.read() << 8 | Wire.read())/16384.0 - yErr;
    *zAccel = (int16_t)(Wire.read() << 8 | Wire.read())/16384.0 - zErr;
}

void imu_read_mag(float* xMag, float* yMag, float* zMag) {
    Wire.beginTransmission(0x0C);
    Wire.write(0x03);
    Wire.endTransmission(false);
    Wire.requestFrom(0x0C, 6, true);
    int16_t x = (int16_t)(Wire.read() | (Wire.read() << 8));
    int16_t y = (int16_t)(Wire.read() | (Wire.read() << 8));
    int16_t z = (int16_t)(Wire.read() | (Wire.read() << 8));

    *xMag = x * 0.6f;
    *yMag = y * 0.6f;
    *zMag = z * 0.6f;
}

void imu_read_gyro(float* xGyro,float xErr, float* yGyro, float yErr, float* zGyro, float zErr) {
    Wire.beginTransmission(0x68);
    Wire.write(REG_GYRO_XOUT_H);
    Wire.endTransmission(false);
    Wire.requestFrom(0x68, 6, true);
    *xGyro = (int16_t)(Wire.read() << 8 | Wire.read())/131.0 - xErr;
    *yGyro = (int16_t)(Wire.read() << 8 | Wire.read())/131.0 - yErr;
    *zGyro = (int16_t)(Wire.read() << 8 | Wire.read())/131.0 - zErr;
}

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

void imu_i2c(void) {
  Wire.beginTransmission(0x68);
  Wire.write(REG_PWR_MGMT_1);
  Wire.write(0x00);
  Wire.endTransmission(true);
}

void imu_config_magno(void) {
Wire.beginTransmission(0x68);
  Wire.write(0x37);
  Wire.write(0x02);
  Wire.endTransmission(true);

  Wire.beginTransmission(0x0C);                       // Open session with AK8963
  Wire.write(0x0A);                                   // CNTL[3:0] mode bits
  Wire.write(0b00011111);                                           // Output data=16-bits; Access fuse ROM
  Wire.endTransmission();

  Wire.beginTransmission(0x0C);                       // Open session with AK8963
  Wire.write(0x10);                                      // Point to AK8963 fuse ROM
  Wire.endTransmission();
}