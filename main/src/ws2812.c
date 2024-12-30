#include <ws2812_control.h>
#include "ws2812.h"

WS2812Dev WS2812;
// ### 2. 设置 LED 效果

// 可以通过调用 `ws2812_set` 函数来设置 LED 的效果。例如，设置所有 LED 为常亮状态，颜色为红色：

// ```c
// //方式1
// ws2812_set(WS2812, COLOR_RED, LED_EFFECT_ON);
// //方式2
// led_set_on(WS2812,COLOR_RED);
// //也可使用COLOR_RGB(255,0,0)来设置红色或自定义颜色
// led_set_on(WS2812,COLOR_RGB(255,0,0));
// ```

// ### 3. 常用功能

// - **设置单个 LED 颜色**：

//   ```c
//   led_set_pixel(WS2812, 0, COLOR_GREEN); // 设置第0个LED为绿色
//   ```

// - **关闭所有 LED**：

//   ```c
//   led_set_off(WS2812);
//   ```

// - **呼吸效果**：

//   ```c
//   //方式1
//   ws2812_set(WS2812, COLOR_BLUE, LED_EFFECT_BREATH);
//   //方式2
//   led_set_breath(WS2812, COLOR_BLUE);
//   ```

// - **彩虹效果**：

//   ```c
//   //方式1
//   ws2812_set(WS2812, COLOR_BLUE, LED_EFFECT_RAINBOW);
//   //方式2
//   led_set_rainbow(WS2812);
//   ```


// #### `ws2812_set()`第三个参数 模式可设置以下效果
// >
//     LED_EFFECT_ON             // 使LED灯常亮效果
//     LED_EFFECT_BREATH,        // 使LED灯呼吸效果
//     LED_EFFECT_FADE_IN,       // 使LED灯淡入效果
//     LED_EFFECT_BLINK_SLOW,    // 使LED灯慢闪烁效果
//     LED_EFFECT_BLINK_FAST,    // 使LED灯快闪烁效果
//     LED_EFFECT_RAINBOW        // 使LED灯彩虹效果

// 需注意`ws2812_set()`的配置应用于所有灯珠
void ws2812_Init()
{
        // 创建一个WS2812灯带
    WS2812.led = ws2812_create();
    
    WS2812.isReg = true;

}
//index从0开始计数
void ws2812_led_set_pixel(int index, led_color_t color)
{
    led_set_pixel(WS2812.led, index, color); 
}

void ws2812_led_set_on(led_color_t color)
{
    led_set_on(WS2812.led,color);
}

void ws2812_led_set_off()
{
    led_set_off(WS2812.led);
}

void ws2812_set_effect(led_color_t color, led_effect_t effect)
{
    ws2812_set(WS2812.led,color,effect);
}