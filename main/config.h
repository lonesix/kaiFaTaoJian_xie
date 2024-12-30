#ifndef _CONFIG_H
#define _CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif
#include "stdbool.h"
#include "stdint.h"
#include <ws2812_control.h>
/*adcSensor*/
#define LIGHT_GPIO_PIN 5//环境亮度传感器
#define SOIL_GPIO_PIN 6//土壤湿度传感器
#define FLAME_GPIO_PIN 7//火焰传感器
#define WATER_LEVER_GPIO_PIN 8//水位传感器
/*ADC结构体*/
typedef struct 
{
    bool isReg;//是否注册
    int rawVal;//原始值
    int Value;//实际物理含义值，单位根据传感器不同而不同
}adcDev;//用于管理基于adc的传感器
typedef struct {
    uint8_t Light:1;
    uint8_t Soil:1;
    uint8_t Flame:1;
    uint8_t WaterLevel:1;
}IsRegADC;
typedef struct {
    IsRegADC isReg;
    adcDev Light;
    adcDev Soil;
    adcDev Flame;
    adcDev WaterLevel

}ADC_MANAGER;
/*ADC结构体*/
/*adcSensor*/

/*gpioSensor*/
#define BODY_GPIO_PIN 40//人体红外传感器

#define HALL_GPIO_PIN 41//霍尔磁性传感器
typedef struct 
{
    bool isReg;//是否注册
    int rawVal;//原始值
    int Value;//实际物理含义值，单位根据传感器不同而不同
}gpioDev;//用于管理基于gpio高低电平的传感器
#define DHT11_GPIO_PIN	39//DHT11温湿度传感器
typedef struct 
{
    float Temp;
    float Hum;
}DHT11Data;//用于管理基于gpio高低电平的传感器
typedef struct 
{
    bool isReg;//是否注册
    uint8_t rawVal[4];//原始值
    DHT11Data Value;//实际物理含义值，单位根据传感器不同而不同
}DHT11Dev;//用于管理基于gpio高低电平的传感器
typedef struct DHT11Dev* DHT11Dev_t;
#define SG90_GPIO_PIN 13
typedef enum {
    ANGLE_MODE,//默认
}SG90CustomMode;
typedef struct 
{
    bool isReg;//是否注册
    int angle;//角度值
    SG90CustomMode custmMode;
}SG90Dev;//用于管理舵机
typedef enum {
    COLOR_MODE,//默认
}WS2812CustomMode;
typedef struct 
{
    bool isReg;//是否注册
    ws2812_strip_t* led;
    int RGB;//RGB值
    WS2812CustomMode custmMode;
}WS2812Dev;//用于管理RGB灯
/*gpioSensor*/

/*iicSensor*/
//Mpu6050
#define I2C_MASTER_SCL_IO 1      /*!< gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO 2      /*!< gpio number for I2C master data  */
#include "mpu6050.h"
typedef struct 
{
    bool isReg;//是否注册
    mpu6050_acce_value_t acce;
    mpu6050_gyro_value_t gyro;
    mpu6050_temp_value_t temp;
    void *CustomValue;
}IICDev;//用于管理mpu6050
/*iicSensor*/

/*spiSensor*/
//rc522
#define RC522_RST_GPIO_PIN  12
#define RC522_SPI_MISO_PIN  48
#define RC522_SPI_MOSI_PIN  45
#define RC522_SPI_SCLK_PIN  47
#define RC522_SPI_CS_PIN    21
typedef enum
{
    NONE=0,
    ACTIVE=1
} card_state_t;
typedef struct 
{
    bool isReg;//是否注册
    uint8_t card[4];
    card_state_t value;
    void *CustomValue;
}SPIDev;//用于管理rf522
/*spiSensor*/
extern ADC_MANAGER ADC;
extern DHT11Dev dht11;
extern IICDev MPU6050;
extern SG90Dev sg90;
extern gpioDev Body;
extern gpioDev Hall;
extern WS2812Dev WS2812;
extern SPIDev rc522;
#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif/*_CONFIG_H*/
