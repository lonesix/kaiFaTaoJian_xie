#ifndef _GPIODEV_H
#define _GPIODEV_H

#ifdef __cplusplus
extern "C" {
#endif
#include "config.h"
//人体感应器:人进入其感应范围则输出高电平，人离开感应范围则自动延时关闭高电平，输出低电平。

void Body_Init();
int Body_read();
//霍尔磁性传感器:当磁铁接近其冲刺表面时，开关输出低电平; 当磁铁移开时，开关输出高电平。

void Hall_Init();
int Hall_read();
#ifdef __cplusplus
} /*extern "C"*/
#endif
#endif

