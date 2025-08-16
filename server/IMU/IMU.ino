#include <Wire.h>
#include "IMU_REGISTER_MAP.h"
#include "IMU_STRUCT.h"
#include "IMU.h"

#define PERIOD 3000

struct imu* imu_data = imu_init();
float previousTime;
float currentTime;
float elapsedTime;
float printTime;
bool movement = false;

float xErrGy, yErrGy, zErrGy;
float xErrAc, yErrAc, zErrAc;

float accAngleX, accAngleY, gyroAngleX, gyroAngleY, gyroAngleZ;
float roll, yaw, pitch;

float roll_rad, pitch_rad;
float gX, gY, gZ;
float linAccX, linAccY, linAccZ;

float linMag;

float buffer[32] = {0};
float sum = 0;
float avg = 0;
int i = 0;

void setup() {  
  Serial.begin(115200);
  Wire.begin();
  imu_i2c();
  calibrateGyro(&xErrGy, &yErrGy, &zErrGy);
  calibrateAccel(&xErrAc, &yErrAc, &zErrAc);
  printTime = millis();
  currentTime = millis();
  previousTime = currentTime;
  

}

void loop() { 
  currentTime = millis();            // Current time actual time read
  elapsedTime = (currentTime - previousTime) / 1000.0;
  previousTime = currentTime;

  imu_read_accel(&(imu_data->AccX), 0, &(imu_data->AccY), 0, &(imu_data->AccZ), 0);
  accAngleX = atan2(imu_data->AccY, imu_data->AccZ) * 180 / PI - xErrAc;
  accAngleY = atan2(-imu_data->AccX, sqrt(imu_data->AccY*imu_data->AccY + imu_data->AccZ*imu_data->AccZ)) * 180 / PI + yErrAc;
  roll = accAngleX;
  pitch = accAngleY;

  imu_read_gyro(&(imu_data->GyroX), xErrGy, &(imu_data->GyroY), yErrGy, &(imu_data->GyroZ), zErrGy);

  yaw += imu_data->GyroZ * elapsedTime;
  // Complementary filter - combine acceleromter and gyro angle values
  roll  = 0.98*(roll + imu_data->GyroX * elapsedTime) + (1-0.98)*accAngleX;
  pitch = 0.98*(pitch + imu_data->GyroY * elapsedTime) + (1-0.98)*accAngleY;

  roll_rad = roll * PI / 180.0;
  pitch_rad = pitch * PI / 180.0;

  gX = -sin(pitch_rad);
  gY = sin(roll_rad) * cos(pitch_rad);
  gZ = cos(roll_rad) * cos(pitch_rad);

  linAccX = imu_data->AccX - gX;
  linAccY = imu_data->AccY - gY;
  linAccZ = imu_data->AccZ - gZ;

  linMag = sqrt(linAccX*linAccX + linAccY*linAccY + linAccZ*linAccZ);

  if (buffer[i+1] != 0)
    sum -= buffer[i];
  
  buffer[i] = linMag;
  sum += buffer[i];
  
  // Move to next position in circular buffer (wraps around at 5)
  i = (i+1) % 32;
  
  // Calculate simple moving average of last 5 readings
  avg = sum / 32;
    // Serial.print("X-Acceleration: ");
    // Serial.println(imu_data->AccX);
    // Serial.print("X-Gyroscope: ");
    // Serial.println(imu_data->GyroX);
    // Serial.print("Y-Acceleration: ");
    // Serial.println(imu_data->AccY);
    // Serial.print("Y-Gyroscope: ");
    // Serial.println(imu_data->GyroY);
    // Serial.println();

    // Serial.print("roll: ");
    // Serial.println(roll);
    // Serial.print("pitch: ");
    // Serial.println(pitch);
    // Serial.print("yaw: ");
    // Serial.println(yaw);
    // Serial.println();

    // Serial.print("linear magnitude acceleration: ");
    // Serial.println(linMag);
    // Serial.print("linear magnitude acceleration moving average: ");
    // Serial.println(avg);
    // Serial.println();

  if (avg >= 0.5 && !movement) {
    Serial.println("movement detected!");
    movement = true;
  }

  if (avg <= 0.05) {
    movement = false;
  }

}

