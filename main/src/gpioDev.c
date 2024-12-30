#include "driver/gpio.h"
#include "gpioDev.h"
gpioDev Body;
gpioDev Hall;
void GPIOAPI(gpio_num_t gpio_num)
{
    gpio_config_t gpioGonfig = {
    .pin_bit_mask = (1ULL << gpio_num) ,
    .mode = GPIO_MODE_INPUT,
    .pull_up_en = 0,
    .pull_down_en = 0,
    .intr_type = GPIO_INTR_DISABLE,
};
    gpio_config(&gpioGonfig);
    gpio_get_level( gpio_num);

}

void Body_Init()
{
    Body.isReg = true;
        gpio_config_t gpioGonfig = {
    .pin_bit_mask = (1ULL << BODY_GPIO_PIN) ,
    .mode = GPIO_MODE_INPUT,
    .pull_up_en = 0,
    .pull_down_en = 0,
    .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&gpioGonfig);
}

int Body_read()
{
    Body.rawVal = gpio_get_level( BODY_GPIO_PIN);
    return Body.rawVal;
}

void Hall_Init()
{
    Hall.isReg =true;
    gpio_config_t gpioGonfig = {
    .pin_bit_mask = (1ULL << HALL_GPIO_PIN) ,
    .mode = GPIO_MODE_INPUT,
    .pull_up_en = 0,
    .pull_down_en = 0,
    .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&gpioGonfig);
}
int Hall_read()
{
    Hall.rawVal = gpio_get_level( HALL_GPIO_PIN);
    return Hall.rawVal;
}