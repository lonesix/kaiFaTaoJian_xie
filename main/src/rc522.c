#include "user_nfc.h"
#include "board_gpio.h"
#include "board_mfrc522.h"
#include "board_spi.h"
#include "esp_log.h"
#include "rc522.h"
SPIDev rc522;
void rc522_Init(){
    rc522.isReg = true;
    NFC_GPIO_Init();

    NFC_SPI_Init();
    MFRC522_Init();

}

void rc522_read_cardid(void)
{
    uint8_t ret;
    ret = MFRC522_ReadCardSerialNo(rc522.card);
    
    ESP_LOGI("TAG", "cardret: %02x", ret);
    if (ret != 2)
    {
        /* code */
        rc522.value = ACTIVE;
        
    }else{
        rc522.value = NONE;
        rc522.card[0] = 0;
        rc522.card[1] = 0;
        rc522.card[2] = 0;
        rc522.card[3] = 0;
    }
}