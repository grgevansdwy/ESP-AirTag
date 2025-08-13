#include <Wire.h>
#include "IMU_REGISTER_MAP.h"
#include "IMU_STRUCT.h"

#define PERIOD 3000

const int MPU = 0x68;
struct imu* imu_data = imu_init();
int startTime = 0;
int currentTime = 0;


void setup() {  
  Serial.begin(115200);
  Wire.begin();
  Wire.beginTransmission(MPU);
  Wire.write(REG_PWR_MGMT_1);
  Wire.write(0x00);
  Wire.endTransmission(true);
  startTime = millis();

  Wire.beginTransmission(MPU);
  Wire.write(REG_ACCEL_CONFIG);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU, 1, true);
  uint8_t config = Wire.read();
  Serial.println((config >> 3) & 0x03, BIN);
  

}

void loop() {
  currentTime = millis();
  Wire.beginTransmission(MPU);
  Wire.write(REG_ACCEL_XOUT_H);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU, 6, true);
  imu_data->AccX = (int16_t)(Wire.read() << 8 | Wire.read())/16384.0;
  imu_data->AccY = (int16_t)(Wire.read() << 8 | Wire.read())/16384.0;
  imu_data->AccZ = (int16_t)(Wire.read() << 8 | Wire.read())/16384.0;

  Wire.beginTransmission(MPU);
  Wire.write(REG_GYRO_XOUT_H);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU, 6, true);
  imu_data->GyroX = (int16_t)(Wire.read() << 8 | Wire.read())/16384.0;
  imu_data->GyroY = (int16_t)(Wire.read() << 8 | Wire.read())/16384.0;
  imu_data->GyroZ = (int16_t)(Wire.read() << 8 | Wire.read())/16384.0;

  if (currentTime - startTime >= PERIOD) {
    Serial.print("X-Acceleration: ");
    Serial.println(imu_data->AccX);
    Serial.print("Y-Acceleration: ");
    Serial.println(imu_data->AccY);
    Serial.print("Z-Acceleration: ");
    Serial.println(imu_data->AccZ);
    Serial.println();

    Serial.print("X-Gyro: ");
    Serial.println(imu_data->GyroX);
    Serial.print("Y-Gyro: ");
    Serial.println(imu_data->GyroY);
    Serial.print("Z-Gyro: ");
    Serial.println(imu_data->GyroZ);
    Serial.println();


    startTime = currentTime;
  }

}

struct imu* imu_init(void) {
  struct imu* imu = (struct imu*) malloc(sizeof(struct imu));
  imu->isMoving = false;
  imu->AccX = imu->AccY = imu->AccZ = 0.0;
  imu->GyroX = imu->GyroX = imu->GyroX = 0.0;
  return imu;
}

void AccelError(void) {

}
