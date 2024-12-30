#ifndef _IICDEV_H
#define _IICDEV_H

#ifdef __cplusplus
extern "C" {
#endif
#include "config.h"
//mpu6050

#define I2C_MASTER_NUM I2C_NUM_0  /*!< I2C port number for master dev */
#define I2C_MASTER_FREQ_HZ 100000 /*!< I2C master clock frequency */
void mpu6050_test();
void mpu6050_init();
void mpu6050_read();
#ifdef __cplusplus
} /*extern "C"*/
#endif
#endif

