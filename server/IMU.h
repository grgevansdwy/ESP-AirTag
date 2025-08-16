struct imu* imu_init(void);

void imu_read_accel(float* xAccel, float xErr, float* yAccel, float yErr, float* zAccel, float zErr);

void imu_read_gyro(float* xGyro, float xErr, float* yGyro, float yErr, float* zGyro, float zErr);

void calibrateGyro(float* xErr, float* yErr, float* zErr);

void calibrateAccel(float* xErr, float* yErr, float* zErr);

void imu_i2c(void);