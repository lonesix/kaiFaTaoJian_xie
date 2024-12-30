/*********************************************************************
 * INCLUDES
 */
#include <string.h>
#include "driver/spi_master.h"

#include "board_spi.h"

/*********************************************************************
 * LOCAL VARIABLES
 */
static spi_device_handle_t s_spiHandle;

/*********************************************************************
 * PUBLIC FUNCTIONS
 */
/**
 @brief NFC SPI驱动初始化
 @param 无
 @return 无
*/
void NFC_SPI_Init(void)
{
    esp_err_t ret;
    
    spi_bus_config_t spiBusConfig =
    {
        .miso_io_num = NFC_SPI_MISO_PIN,                    // MISO信号线
        .mosi_io_num = NFC_SPI_MOSI_PIN,                    // MOSI信号线
        .sclk_io_num = NFC_SPI_SCLK_PIN,                    // SCLK信号线
        .quadwp_io_num = -1,                                // WP信号线，专用于QSPI的D2
        .quadhd_io_num = -1,                                // HD信号线，专用于QSPI的D3
        .max_transfer_sz = 64 * 8,                          // 最大传输数据大小
    };

    spi_device_interface_config_t spiDeviceConfig =
    {
        .clock_speed_hz = SPI_MASTER_FREQ_10M,              // Clock out at 10 MHz,
        .mode = 0,                                          // SPI mode 0
        /*
         * The timing requirements to read the busy signal from the EEPROM cannot be easily emulated
         * by SPI transactions. We need to control CS pin by SW to check the busy signal manually.
         */
        .spics_io_num = -1,
        .queue_size = 7,                                    // 传输队列大小，决定了等待传输数据的数量
    };

    //Initialize the SPI bus
    ret = spi_bus_initialize(SPI3_HOST, &spiBusConfig, DMA_CHAN);
    ESP_ERROR_CHECK(ret);
    ret = spi_bus_add_device(SPI3_HOST, &spiDeviceConfig, &s_spiHandle);
    ESP_ERROR_CHECK(ret);

    // 配置CS引脚
    gpio_config_t usb_phy_conf = {
    .pin_bit_mask = (1ULL << NFC_SPI_CS_PIN) ,
    .mode = GPIO_MODE_OUTPUT,
    .pull_up_en = 0,
    .pull_down_en = 0,
    .intr_type = GPIO_INTR_DISABLE,
};
gpio_config(&usb_phy_conf);
    // gpio_pad_select_gpio(NFC_SPI_CS_PIN);                   // 选择一个GPIO
    // gpio_set_direction(NFC_SPI_CS_PIN, GPIO_MODE_OUTPUT);   // 把这个GPIO作为输出
}

/**
 @brief NFC SPI写入数据
 @param pData -[in] 写入数据
 @param dataLen -[in] 写入数据长度
 @return 无
*/
void NFC_SPI_Write(uint8_t *pData, uint32_t dataLen)
{
    esp_err_t ret;
    spi_transaction_t t;
    if(0 == dataLen)                                        // no need to send anything
    {
        return;
    }

    memset(&t, 0, sizeof(t));                               // Zero out the transaction
    t.length = dataLen * 8;                                 // Len is in bytes, transaction length is in bits.
    t.tx_buffer = pData;                                    // Data
    ret = spi_device_polling_transmit(s_spiHandle, &t);     // Transmit!
    assert(ret == ESP_OK);                                  // Should have had no issues.
}

/**
 @brief NFC SPI读取数据
 @param pData -[out] 读取数据
 @param dataLen -[in] 读取数据长度
 @return 无
*/
void NFC_SPI_Read(uint8_t *pData, uint32_t dataLen)
{
    spi_transaction_t t;
    if(0 == dataLen)                                        // no need to receivce anything
    {
        return;
    }

    memset(&t, 0, sizeof(t));                               // Zero out the transaction
    t.length = dataLen * 8;                                 // Len is in bytes, transaction length is in bits.
    t.rx_buffer = pData;
    esp_err_t ret = spi_device_polling_transmit(s_spiHandle, &t);
    assert(ret == ESP_OK);
}

/****************************************************END OF FILE****************************************************/