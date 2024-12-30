#ifndef _ADCDEV_H
#define _ADCDEV_H

#ifdef __cplusplus
extern "C" {
#endif
#include "config.h"
#define ADC_NUM 4
//注意：必须使用adc1也就是gpio1-10
#define LIGHT_ADC_CHANNEL LIGHT_GPIO_PIN-1//环境亮度传感器
#define SOIL_ADC_CHANNEL SOIL_GPIO_PIN-1//土壤湿度传感器
#define FLAME_ADC_CHANNEL FLAME_GPIO_PIN-1//火焰传感器
#define WATER_ADC_CHANNEL WATER_LEVER_GPIO_PIN-1//水位传感器
void set_ADC_Init(void);
void nADC_init();
void print_adc(void) ;
#ifdef __cplusplus
} /*extern "C"*/
#endif
#endif

