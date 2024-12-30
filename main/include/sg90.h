#ifndef _SG90__H
#define _SG90__H

#ifdef __cplusplus
extern "C" {
#endif
#include "config.h"
#define sg90_pin SG90_GPIO_PIN//信号线的引脚
void sg90_init();
void sg90_SetAngle(float angle);

#ifdef __cplusplus
} /*extern "C"*/
#endif
#endif

