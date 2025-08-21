/**
 * @struct imu
 * @brief Data structure for storing IMU sensor readings.
 *
 * Holds accelerometer, gyroscope, and magnetometer measurements
 * along the X, Y, and Z axes.
 */
struct imu {
    float AccX;  /**< Acceleration along X-axis (g). */
    float AccY;  /**< Acceleration along Y-axis (g). */
    float AccZ;  /**< Acceleration along Z-axis (g). */
    float GyroX; /**< Angular velocity around X-axis (degrees/sec). */
    float GyroY; /**< Angular velocity around Y-axis (degrees/sec). */
    float GyroZ; /**< Angular velocity around Z-axis (degrees/sec). */
    float MagX;  /**< Magnetic field along X-axis (µT). */
    float MagY;  /**< Magnetic field along Y-axis (µT). */
    float MagZ;  /**< Magnetic field along Z-axis (µT). */
};
