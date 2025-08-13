struct imu{
    float AccX, AccY, AccZ; 
    float GyroX, GyroY, GyroZ;
    bool isMoving;
};

struct imu* imu_init(void);