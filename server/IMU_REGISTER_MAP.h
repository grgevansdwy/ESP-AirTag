/**
 * @file MPU6500Regs.h
 * @brief Register definitions for the MPU-6500 IMU.
 * @details
 * Based on the MPU-9250 Register Map (Gyro/Accel section),
 * compatible with MPU-6500 devices.
 * 
 * Derived from: RM-MPU-9250A-00, Rev. 1.6, Jan 7, 2015.
 *
 * This header provides named macros for register addresses
 * and common values, allowing clean and readable access to
 * the MPU-6500 via I2C or SPI.
 *
 * @note Magnetometer registers are also included.
 * @date 2025-08-21
 */

#ifndef MPU6500REGS_H_
#define MPU6500REGS_H_

/** @defgroup MPU6500_Registers MPU-6500 Register Map
 *  @brief Register addresses for the MPU-6500 IMU.
 *  @{
 */

/** @name Gyroscope Self-Test Registers */
///@{
#define REG_SELF_TEST_X_GYRO      0x00 /**< Self-test register for X gyro */
#define REG_SELF_TEST_Y_GYRO      0x01 /**< Self-test register for Y gyro */
#define REG_SELF_TEST_Z_GYRO      0x02 /**< Self-test register for Z gyro */
///@}

/** @name Accelerometer Self-Test Registers */
///@{
#define REG_SELF_TEST_X_ACCEL     0x0D /**< Self-test register for X accel */
#define REG_SELF_TEST_Y_ACCEL     0x0E /**< Self-test register for Y accel */
#define REG_SELF_TEST_Z_ACCEL     0x0F /**< Self-test register for Z accel */
///@}

/** @name Gyroscope Offsets */
///@{
#define REG_XG_OFFSET_H           0x13 /**< Gyro X offset high byte */
#define REG_XG_OFFSET_L           0x14 /**< Gyro X offset low byte */
#define REG_YG_OFFSET_H           0x15 /**< Gyro Y offset high byte */
#define REG_YG_OFFSET_L           0x16 /**< Gyro Y offset low byte */
#define REG_ZG_OFFSET_H           0x17 /**< Gyro Z offset high byte */
#define REG_ZG_OFFSET_L           0x18 /**< Gyro Z offset low byte */
///@}

/** @name Sample Rate & Config */
///@{
#define REG_SMPLRT_DIV            0x19 /**< Sample rate divider */
#define REG_CONFIG                0x1A /**< General configuration */
#define REG_GYRO_CONFIG           0x1B /**< Gyroscope configuration */
#define REG_ACCEL_CONFIG          0x1C /**< Accelerometer configuration */
#define REG_ACCEL_CONFIG2         0x1D /**< Secondary accel config */
#define REG_LP_ACCEL_ODR          0x1E /**< Low power accel output data rate */
#define REG_WOM_THR               0x1F /**< Wake-on-motion threshold */
///@}

/** @name FIFO & I2C Master */
///@{
#define REG_FIFO_EN               0x23 /**< FIFO enable */
#define REG_I2C_MST_CTRL          0x24 /**< I2C master control */
#define REG_I2C_SLV0_ADDR         0x25 /**< I2C slave 0 address */
#define REG_I2C_SLV0_REG          0x26 /**< I2C slave 0 register */
#define REG_I2C_SLV0_CTRL         0x27 /**< I2C slave 0 control */
#define REG_I2C_SLV1_ADDR         0x28 /**< I2C slave 1 address */
#define REG_I2C_SLV1_REG          0x29 /**< I2C slave 1 register */
#define REG_I2C_SLV1_CTRL         0x2A /**< I2C slave 1 control */
#define REG_I2C_SLV2_ADDR         0x2B /**< I2C slave 2 address */
#define REG_I2C_SLV2_REG          0x2C /**< I2C slave 2 register */
#define REG_I2C_SLV2_CTRL         0x2D /**< I2C slave 2 control */
#define REG_I2C_SLV3_ADDR         0x2E /**< I2C slave 3 address */
#define REG_I2C_SLV3_REG          0x2F /**< I2C slave 3 register */
#define REG_I2C_SLV3_CTRL         0x30 /**< I2C slave 3 control */
#define REG_I2C_SLV4_ADDR         0x31 /**< I2C slave 4 address */
#define REG_I2C_SLV4_REG          0x32 /**< I2C slave 4 register */
#define REG_I2C_SLV4_DO           0x33 /**< I2C slave 4 data out */
#define REG_I2C_SLV4_CTRL         0x34 /**< I2C slave 4 control */
#define REG_I2C_SLV4_DI           0x35 /**< I2C slave 4 data in */
#define REG_I2C_MST_STATUS        0x36 /**< I2C master status */
///@}

/** @name Interrupts */
///@{
#define REG_INT_PIN_CFG           0x37 /**< Interrupt pin configuration */
#define REG_INT_ENABLE            0x38 /**< Interrupt enable */
#define REG_INT_STATUS            0x3A /**< Interrupt status */
///@}

/** @name Sensor Outputs */
///@{
#define REG_ACCEL_XOUT_H          0x3B /**< Accel X high byte */
#define REG_ACCEL_XOUT_L          0x3C /**< Accel X low byte */
#define REG_ACCEL_YOUT_H          0x3D /**< Accel Y high byte */
#define REG_ACCEL_YOUT_L          0x3E /**< Accel Y low byte */
#define REG_ACCEL_ZOUT_H          0x3F /**< Accel Z high byte */
#define REG_ACCEL_ZOUT_L          0x40 /**< Accel Z low byte */
#define REG_TEMP_OUT_H            0x41 /**< Temperature high byte */
#define REG_TEMP_OUT_L            0x42 /**< Temperature low byte */
#define REG_GYRO_XOUT_H           0x43 /**< Gyro X high byte */
#define REG_GYRO_XOUT_L           0x44 /**< Gyro X low byte */
#define REG_GYRO_YOUT_H           0x45 /**< Gyro Y high byte */
#define REG_GYRO_YOUT_L           0x46 /**< Gyro Y low byte */
#define REG_GYRO_ZOUT_H           0x47 /**< Gyro Z high byte */
#define REG_GYRO_ZOUT_L           0x48 /**< Gyro Z low byte */
///@}

/** @name External Sensor Data */
///@{
#define REG_EXT_SENS_DATA_00      0x49 /**< External sensor data register start */
#define REG_EXT_SENS_DATA_23      0x60 /**< External sensor data register end */
///@}

/** @name I2C Data Out */
///@{
#define REG_I2C_SLV0_DO           0x63 /**< I2C slave 0 data out */
#define REG_I2C_SLV1_DO           0x64 /**< I2C slave 1 data out */
#define REG_I2C_SLV2_DO           0x65 /**< I2C slave 2 data out */
#define REG_I2C_SLV3_DO           0x66 /**< I2C slave 3 data out */
///@}

/** @name Control & Power Management */
///@{
#define REG_I2C_MST_DELAY_CTRL    0x67 /**< I2C master delay control */
#define REG_SIGNAL_PATH_RESET     0x68 /**< Signal path reset */
#define REG_MOT_DETECT_CTRL       0x69 /**< Motion detection control */
#define REG_USER_CTRL             0x6A /**< User control */
#define REG_PWR_MGMT_1            0x6B /**< Power management 1 */
#define REG_PWR_MGMT_2            0x6C /**< Power management 2 */
///@}

/** @name FIFO */
///@{
#define REG_FIFO_COUNTH           0x72 /**< FIFO count high byte */
#define REG_FIFO_COUNTL           0x73 /**< FIFO count low byte */
#define REG_FIFO_R_W              0x74 /**< FIFO read/write */
///@}

/** @name Identification */
///@{
#define REG_WHO_AM_I              0x75 /**< Device ID register */
///@}

/** @name Accelerometer Offsets */
///@{
#define REG_XA_OFFSET_H           0x77 /**< Accel X offset high */
#define REG_XA_OFFSET_L           0x78 /**< Accel X offset low */
#define REG_YA_OFFSET_H           0x7A /**< Accel Y offset high */
#define REG_YA_OFFSET_L           0x7B /**< Accel Y offset low */
#define REG_ZA_OFFSET_H           0x7D /**< Accel Z offset high */
#define REG_ZA_OFFSET_L           0x7E /**< Accel Z offset low */
///@}

/** @name Magnetometer Outputs */
///@{
#define REG_MAG_XOUT_H            0x03 /**< Mag X high */
#define REG_MAG_XOUT_L            0x04 /**< Mag X low */
#define REG_MAG_YOUT_H            0x05 /**< Mag Y high */
#define REG_MAG_YOUT_L            0x06 /**< Mag Y low */
#define REG_MAG_ZOUT_H            0x07 /**< Mag Z high */
#define REG_MAG_ZOUT_L            0x08 /**< Mag Z low */
///@}

/** @} */ // end of MPU6500_Registers

/** @defgroup MPU6500_Common MPU-6500 Common Values
 *  @brief Useful constants for MPU-6500 operation.
 *  @{
 */

#define WHO_AM_I_MPU6500          0x70  /**< Expected WHO_AM_I response for MPU-6500 */
#define WHO_AM_I_MPU9250          0x71  /**< Expected WHO_AM_I response for MPU-9250 */

/** @name Power Management 1 bits */
///@{
#define PWR_DEVICE_RESET          0x80 /**< Reset entire device */
#define PWR_SLEEP                 0x40 /**< Enter sleep mode */
#define PWR_CYCLE                 0x20 /**< Cycle between sleep/active */
#define PWR_CLKSEL_INTERNAL       0x00 /**< Internal 20 MHz oscillator */
#define PWR_CLKSEL_PLL_XGYRO      0x01 /**< PLL with X gyro reference */
#define PWR_CLKSEL_PLL_YGYRO      0x02 /**< PLL with Y gyro reference */
#define PWR_CLKSEL_PLL_ZGYRO      0x03 /**< PLL with Z gyro reference */
///@}

/** @} */ // end of MPU6500_Common

#endif /* MPU6500REGS_H_ */
