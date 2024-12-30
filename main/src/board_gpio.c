/*********************************************************************
 * INCLUDES
 */
#include "driver/gpio.h"

#include "board_gpio.h"

/*********************************************************************
 * PUBLIC FUNCTIONS
 */
/**
 @brief NFC复位引脚初始化
 @param 无
 @return 无
*/
void NFC_GPIO_Init(void)
{
        gpio_config_t usb_phy_conf = {
    .pin_bit_mask = (1ULL << NFC_RST_GPIO_PIN) ,
    .mode = GPIO_MODE_OUTPUT,
    .pull_up_en = 0,
    .pull_down_en = 0,
    .intr_type = GPIO_INTR_DISABLE,
};
gpio_config(&usb_phy_conf);
    // gpio_pad_select_gpio(NFC_RST_GPIO_PIN);                 // 选择一个GPIO
    // gpio_set_direction(NFC_RST_GPIO_PIN, GPIO_MODE_OUTPUT); // 把这个GPIO作为输出

    NFC_GPIO_Write(NFC_RST_HIGH);
}

/**
 @brief 配置NFC复位引脚工作模式
 @param mode -[in] 工作模式
 @return 无
*/
void NFC_GPIO_Write(uint8_t mode)
{
    gpio_set_level(NFC_RST_GPIO_PIN, mode);
}

/****************************************************END OF FILE****************************************************/