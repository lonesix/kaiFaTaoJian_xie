#ifndef _WS2812_H
#define _WS2812_H

#ifdef __cplusplus
extern "C" {
#endif
#include "config.h"
void ws2812_Init();

//index从0开始计数
void ws2812_led_set_pixel(int index, led_color_t color);
void ws2812_led_set_on(led_color_t color);
void ws2812_led_set_off();

void ws2812_set_effect(led_color_t color, led_effect_t effect);
#ifdef __cplusplus
} /*extern "C"*/
#endif
#endif

