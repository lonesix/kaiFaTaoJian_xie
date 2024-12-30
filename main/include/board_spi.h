#ifndef _BOARD_SPI_H_
#define _BOARD_SPI_H_

/*********************************************************************
 * INCLUDES
 */
#include "driver/gpio.h"
#include "config.h"
/*********************************************************************
 * DEFINITIONS
 */
#define NFC_SPI_MISO_PIN        RC522_SPI_MISO_PIN
#define NFC_SPI_MOSI_PIN        RC522_SPI_MOSI_PIN
#define NFC_SPI_SCLK_PIN        RC522_SPI_SCLK_PIN
#define NFC_SPI_CS_PIN          RC522_SPI_CS_PIN

#define DMA_CHAN                SPI_DMA_CH_AUTO 

#define SPI_CS_LOW              gpio_set_level(NFC_SPI_CS_PIN, 0)
#define SPI_CS_HIGH             gpio_set_level(NFC_SPI_CS_PIN, 1)

/*********************************************************************
 * API FUNCTIONS
 */
void NFC_SPI_Init(void);
void NFC_SPI_Write(uint8_t *pData, uint32_t dataLen);
void NFC_SPI_Read(uint8_t *pData, uint32_t dataLen);

#endif /* _BOARD_SPI_H_ */