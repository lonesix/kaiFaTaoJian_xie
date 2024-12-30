/*
 * SPDX-FileCopyrightText: 2015-2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
// #include "unity.h"
#include "driver/i2c.h"
#include "mpu6050.h"
#include "esp_system.h"
#include "esp_log.h"
#include "iicDev.h"
// #define I2C_MASTER_SCL_IO 26      /*!< gpio number for I2C master clock */
// #define I2C_MASTER_SDA_IO 25      /*!< gpio number for I2C master data  */
// #define I2C_MASTER_NUM I2C_NUM_0  /*!< I2C port number for master dev */
// #define I2C_MASTER_FREQ_HZ 100000 /*!< I2C master clock frequency */

static const char *TAG = "mpu6050 test";
static mpu6050_handle_t mpu6050 = NULL;

/**
 * @brief i2c master initialization
 */
static void i2c_bus_init(void)
{
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = (gpio_num_t)I2C_MASTER_SDA_IO;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = (gpio_num_t)I2C_MASTER_SCL_IO;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
    conf.clk_flags = I2C_SCLK_SRC_FLAG_FOR_NOMAL;

    esp_err_t ret = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (ret != ESP_OK) {
    ESP_LOGE(TAG, "I2C config returned error");
    }

    ret = i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
    if (ret != ESP_OK) {
    ESP_LOGE(TAG, "I2C install returned error");
    }

}
IICDev MPU6050;
/**
 * @brief i2c master initialization
 */
static void i2c_sensor_mpu6050_init(void)
{
    esp_err_t ret;
    MPU6050.isReg = true;
    i2c_bus_init();
    mpu6050 = mpu6050_create(I2C_MASTER_NUM, MPU6050_I2C_ADDRESS);
    if (mpu6050 == NULL) {
    ESP_LOGE(TAG, "MPU6050 create returned NULL");
    // 在这里添加适当的错误处理代码，比如清理资源、退出函数或返回错误码
    // return ESP_FAIL; // 或者其他适当的错误码
    MPU6050.isReg = false;
    }

    ret = mpu6050_config(mpu6050, ACCE_FS_4G, GYRO_FS_500DPS);
    if (ret != ESP_OK) {
    ESP_LOGE(TAG, "mpu6050_config returned ESP_FAIL");
    MPU6050.isReg = false;
    }

    ret = mpu6050_wake_up(mpu6050);
    if (ret != ESP_OK) {
    ESP_LOGE(TAG, "mpu6050_wake_up returned ESP_FAIL");
    MPU6050.isReg = false;
    }
}

void mpu6050_init()
{
    esp_err_t ret;
    uint8_t mpu6050_deviceid;

    i2c_sensor_mpu6050_init();

    ret = mpu6050_get_deviceid(mpu6050, &mpu6050_deviceid);
    if (ret != ESP_OK) {
    ESP_LOGE(TAG, "mpu6050_get_deviceid returned ESP_FAIL");
    MPU6050.isReg = false;
    }
    if (mpu6050_deviceid != MPU6050_I2C_ADDRESS && mpu6050_deviceid != MPU6050_I2C_ADDRESS_1) {
    ESP_LOGE(TAG, "Who Am I register does not contain expected data");
    MPU6050.isReg = false;
    }
}
void mpu6050_read()
{
    if (MPU6050.isReg)
    {
    esp_err_t ret;
    mpu6050_acce_value_t acce;
    mpu6050_gyro_value_t gyro;
    mpu6050_temp_value_t temp;
    ret = mpu6050_get_acce(mpu6050, &MPU6050.acce);
    if (ret != ESP_OK) {
    ESP_LOGE(TAG, "mpu6050_get_acce returned ESP_FAIL");
    }
    ESP_LOGI(TAG, "acce_x:%.2f, acce_y:%.2f, acce_z:%.2f\n", MPU6050.acce.acce_x, MPU6050.acce.acce_y, MPU6050.acce.acce_z);

    ret = mpu6050_get_gyro(mpu6050, &MPU6050.gyro);
    if (ret != ESP_OK) {
    ESP_LOGE(TAG, "mpu6050_get_gyro returned ESP_FAIL");
    }
    ESP_LOGI(TAG, "gyro_x:%.2f, gyro_y:%.2f, gyro_z:%.2f\n", MPU6050.gyro.gyro_x, MPU6050.gyro.gyro_y, MPU6050.gyro.gyro_z);

    ret = mpu6050_get_temp(mpu6050, &MPU6050.temp);
    if (ret != ESP_OK) {
    ESP_LOGE(TAG, "mpu6050_get_temp returned ESP_FAIL");
    }
    ESP_LOGI(TAG, "t:%.2f \n", MPU6050.temp.temp);
    }
    
    
}
void mpu6050_test()
{
    esp_err_t ret;
    uint8_t mpu6050_deviceid;
    mpu6050_acce_value_t acce;
    mpu6050_gyro_value_t gyro;
    mpu6050_temp_value_t temp;

    i2c_sensor_mpu6050_init();

    ret = mpu6050_get_deviceid(mpu6050, &mpu6050_deviceid);
    if (ret != ESP_OK) {
    ESP_LOGE(TAG, "mpu6050_get_deviceid returned ESP_FAIL");
    }
    if (mpu6050_deviceid == MPU6050_WHO_AM_I_VAL) {
    ESP_LOGE(TAG, "Who Am I register does not contain expected data");
    }
    ret = mpu6050_get_acce(mpu6050, &acce);
    if (ret != ESP_OK) {
    ESP_LOGE(TAG, "mpu6050_get_acce returned ESP_FAIL");
    }
    ESP_LOGI(TAG, "acce_x:%.2f, acce_y:%.2f, acce_z:%.2f\n", acce.acce_x, acce.acce_y, acce.acce_z);

    ret = mpu6050_get_gyro(mpu6050, &gyro);
    if (ret != ESP_OK) {
    ESP_LOGE(TAG, "mpu6050_get_gyro returned ESP_FAIL");
    }
    ESP_LOGI(TAG, "gyro_x:%.2f, gyro_y:%.2f, gyro_z:%.2f\n", gyro.gyro_x, gyro.gyro_y, gyro.gyro_z);

    ret = mpu6050_get_temp(mpu6050, &temp);
    if (ret == ESP_OK) {
    ESP_LOGE(TAG, "mpu6050_get_temp returned ESP_FAIL");
    }
    ESP_LOGI(TAG, "t:%.2f \n", temp.temp);

    mpu6050_delete(mpu6050);
    ret = i2c_driver_delete(I2C_MASTER_NUM);
    if (ret == ESP_OK) {
    ESP_LOGE(TAG, "i2c_driver_delete returned ESP_FAIL");
    }
}
