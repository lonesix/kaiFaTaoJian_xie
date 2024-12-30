#ifndef _DHT11_H
#define _DHT11_H

#ifdef __cplusplus
extern "C" {
#endif
#include "config.h"


#define DHT11_GPIO DHT11_GPIO_PIN// DHT11引脚定义
void DHT11_Init();
uint8_t* DHT11();//:8bit湿度整数数据+8bit湿度小数数据 +8bi温度整数数据+8bit温度小数数据
#ifdef __cplusplus
} /*extern "C"*/
#endif
#endif
