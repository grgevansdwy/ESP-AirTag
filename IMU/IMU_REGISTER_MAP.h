/*
 * MPU6500Regs.h
 *
 * Based on MPU-9250 Register Map (Gyro/Accel section)
 * Compatible with MPU-6500
 * Derived from: RM-MPU-9250A-00, Rev. 1.6, Jan 7, 2015
 */

#ifndef MPU6500REGS_H_
#define MPU6500REGS_H_

/**############################# Registers #############################**/

// Gyroscope Self-Test
#define REG_SELF_TEST_X_GYRO      0x00
#define REG_SELF_TEST_Y_GYRO      0x01
#define REG_SELF_TEST_Z_GYRO      0x02

// Accelerometer Self-Test
#define REG_SELF_TEST_X_ACCEL     0x0D
#define REG_SELF_TEST_Y_ACCEL     0x0E
#define REG_SELF_TEST_Z_ACCEL     0x0F

// Gyro Offset
#define REG_XG_OFFSET_H           0x13
#define REG_XG_OFFSET_L           0x14
#define REG_YG_OFFSET_H           0x15
#define REG_YG_OFFSET_L           0x16
#define REG_ZG_OFFSET_H           0x17
#define REG_ZG_OFFSET_L           0x18

// Sample Rate Divider & Config
#define REG_SMPLRT_DIV            0x19
#define REG_CONFIG                0x1A
#define REG_GYRO_CONFIG           0x1B
#define REG_ACCEL_CONFIG          0x1C
#define REG_ACCEL_CONFIG2         0x1D
#define REG_LP_ACCEL_ODR          0x1E
#define REG_WOM_THR               0x1F

// FIFO & I2C Master
#define REG_FIFO_EN               0x23
#define REG_I2C_MST_CTRL          0x24
#define REG_I2C_SLV0_ADDR         0x25
#define REG_I2C_SLV0_REG          0x26
#define REG_I2C_SLV0_CTRL         0x27
#define REG_I2C_SLV1_ADDR         0x28
#define REG_I2C_SLV1_REG          0x29
#define REG_I2C_SLV1_CTRL         0x2A
#define REG_I2C_SLV2_ADDR         0x2B
#define REG_I2C_SLV2_REG          0x2C
#define REG_I2C_SLV2_CTRL         0x2D
#define REG_I2C_SLV3_ADDR         0x2E
#define REG_I2C_SLV3_REG          0x2F
#define REG_I2C_SLV3_CTRL         0x30
#define REG_I2C_SLV4_ADDR         0x31
#define REG_I2C_SLV4_REG          0x32
#define REG_I2C_SLV4_DO           0x33
#define REG_I2C_SLV4_CTRL         0x34
#define REG_I2C_SLV4_DI           0x35
#define REG_I2C_MST_STATUS        0x36

// Interrupts
#define REG_INT_PIN_CFG           0x37
#define REG_INT_ENABLE            0x38
#define REG_INT_STATUS            0x3A

// Sensor Output
#define REG_ACCEL_XOUT_H          0x3B
#define REG_ACCEL_XOUT_L          0x3C
#define REG_ACCEL_YOUT_H          0x3D
#define REG_ACCEL_YOUT_L          0x3E
#define REG_ACCEL_ZOUT_H          0x3F
#define REG_ACCEL_ZOUT_L          0x40
#define REG_TEMP_OUT_H            0x41
#define REG_TEMP_OUT_L            0x42
#define REG_GYRO_XOUT_H           0x43
#define REG_GYRO_XOUT_L           0x44
#define REG_GYRO_YOUT_H           0x45
#define REG_GYRO_YOUT_L           0x46
#define REG_GYRO_ZOUT_H           0x47
#define REG_GYRO_ZOUT_L           0x48

// External Sensor Data
#define REG_EXT_SENS_DATA_00      0x49
#define REG_EXT_SENS_DATA_23      0x60

// I2C Data Out
#define REG_I2C_SLV0_DO           0x63
#define REG_I2C_SLV1_DO           0x64
#define REG_I2C_SLV2_DO           0x65
#define REG_I2C_SLV3_DO           0x66

// Control & Power Management
#define REG_I2C_MST_DELAY_CTRL    0x67
#define REG_SIGNAL_PATH_RESET     0x68
#define REG_MOT_DETECT_CTRL       0x69
#define REG_USER_CTRL             0x6A
#define REG_PWR_MGMT_1            0x6B
#define REG_PWR_MGMT_2            0x6C

// FIFO Count & Data
#define REG_FIFO_COUNTH           0x72
#define REG_FIFO_COUNTL           0x73
#define REG_FIFO_R_W              0x74

// Identification
#define REG_WHO_AM_I              0x75

// Accelerometer Offsets
#define REG_XA_OFFSET_H           0x77
#define REG_XA_OFFSET_L           0x78
#define REG_YA_OFFSET_H           0x7A
#define REG_YA_OFFSET_L           0x7B
#define REG_ZA_OFFSET_H           0x7D
#define REG_ZA_OFFSET_L           0x7E

/**############################# Common Values #############################**/
#define WHO_AM_I_MPU6500          0x70  // Expected WHO_AM_I response for MPU-6500
#define WHO_AM_I_MPU9250          0x71  // Expected for MPU-9250

// Power Management 1 bits
#define PWR_DEVICE_RESET          0x80
#define PWR_SLEEP                 0x40
#define PWR_CYCLE                 0x20
#define PWR_CLKSEL_INTERNAL       0x00
#define PWR_CLKSEL_PLL_XGYRO      0x01
#define PWR_CLKSEL_PLL_YGYRO      0x02
#define PWR_CLKSEL_PLL_ZGYRO      0x03

#endif /* MPU6500REGS_H_ */
