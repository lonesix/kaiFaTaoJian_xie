/*********************************************************************
 * INCLUDES
 */
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "board_gpio.h"
#include "board_spi.h"
#include "board_mfrc522.h"

static char pcdRequest(uint8_t reqCode, uint8_t *pTagType);
static char pcdAnticoll(uint8_t *pSnr);
static char pcdSelect(uint8_t *pSnr);
static char pcdAuthState(uint8_t authMode, uint8_t addr, uint8_t *pKey, uint8_t *pSnr);
static char pcdRead(uint8_t addr, uint8_t *pData);
static char pcdWrite(uint8_t addr, uint8_t *pData);
static void pcdReset(void);
static void calulateCRC(uint8_t *pInData, uint8_t len, uint8_t *pOutData);
static char pcdComMF522(uint8_t command, uint8_t *pInData, uint8_t inLenByte, uint8_t *pOutData, uint32_t *pOutLenBit);
static void pcdAntennaOn(void);
static void pcdAntennaOff(void);
static void setBitMask(uint8_t reg, uint8_t mask);
static void clearBitMask(uint8_t reg, uint8_t mask);
static uint8_t readRawRc(uint8_t addr);
static void writeRawRc(uint8_t addr, uint8_t writeData);
static void delayMs(uint8_t time);

/*********************************************************************
 * LOCAL VARIABLES
 */
static uint8_t s_cardType[2];                                                   // 卡类型
static uint8_t s_cardSerialNo[4];                                               // 卡序列号
static uint8_t s_defaultKeyA[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};         // 默认密码A

static const char *TAG = "MFRC522";

/*********************************************************************
 * PUBLIC FUNCTIONS
 */
/**
 @brief MFRC522的初始化函数
 @param 无
 @return 无
*/
void MFRC522_Init(void)
{
    pcdReset();                                 // 复位
    delayMs(5);
        ESP_LOGI(TAG, "reg: %02x" ,readRawRc(Status1Reg));
        ESP_LOGI(TAG, "reg: %02x" ,readRawRc(Status2Reg));
        ESP_LOGI(TAG, "reg: %02x" ,readRawRc(WaterLevelReg));
    pcdAntennaOn();                             // 开启天线发射
}

/**
 @brief MFRC522读取卡片块数据
 @param addr -[in] 块地址
 @return 状态值，0 - 成功；2 - 无卡；3 - 防冲撞失败；4 - 选卡失败；5 - 密码错误
*/
uint8_t MFRC522_ReadCardDataBlock(uint8_t addr)
{
    memset(s_cardSerialNo, 0, 4);

    if(pcdRequest(PICC_REQALL, s_cardType) == MI_OK)
    {
    }
    else
    {
        // ESP_LOGI(TAG, "ERR: 2");
        return 2;                               // 无卡
    }

    if(pcdAnticoll(s_cardSerialNo) == MI_OK)
    {
    }
    else
    {
        // ESP_LOGI(TAG, "ERR: 3");
        return 3;                               // 防冲撞失败
    }

    if(pcdSelect(s_cardSerialNo) == MI_OK)
    {
    }
    else
    {
        // ESP_LOGI(TAG, "ERR: 4");
        return 4;                               // 选卡失败
    }

    if(pcdAuthState(0x60, addr, s_defaultKeyA, s_cardSerialNo) == MI_OK)
    {
        // ESP_LOGI(TAG, "ERR: 0");
        return 0;
    }
    else
    {
        // ESP_LOGI(TAG, "ERR: 5");
        return 5;                               // 密码错误
    }
}

/**
 @brief 读取卡片序列号
 @param pCardSerialNo -[out] 卡片序列号
 @return 0 - 读卡成功；2 - 无卡
*/
uint8_t MFRC522_ReadCardSerialNo(uint8_t *pCardSerialNo)
{
    uint8_t status = MFRC522_ReadCardDataBlock(4);			
    memcpy(pCardSerialNo, s_cardSerialNo, 4);
    return status;
}


/*********************************************************************
 * LOCAL FUNCTIONS
 */
/**
 @brief 寻卡
 @param reqCode -[in] 寻卡方式，0x52 寻感应区内所有符合1443A标准的卡，0x26 寻未进入休眠状态的卡
 @param pTagType -[out] 卡片类型代码
                        0x4400 = Mifare_UltraLight
                        0x0400 = Mifare_One(S50)
                        0x0200 = Mifare_One(S70)
                        0x0800 = Mifare_Pro(X)
                        0x4403 = Mifare_DESFire
 @return 状态值，MI OK - 成功；MI_ERR - 失败
*/
static char pcdRequest(uint8_t reqCode, uint8_t *pTagType)
{
    char status;
    uint32_t len;
    uint8_t comMF522Buf[MAXRLEN];

    clearBitMask(Status2Reg, 0x08);
    writeRawRc(BitFramingReg, 0x07);
    setBitMask(TxControlReg, 0x03);

    comMF522Buf[0] = reqCode;

    status = pcdComMF522(PCD_TRANSCEIVE, comMF522Buf, 1, comMF522Buf, &len);    // 发送并接收数据
    if((status == MI_OK) && (len == 0x10))
    {
        // ESP_LOGI(TAG, "mi_ok");
        *pTagType = comMF522Buf[0];
        *(pTagType+1) = comMF522Buf[1];
    }
    else
    {
        // ESP_LOGI(TAG, "mi_err");
        status = MI_ERR;
    }

    return status;
}

/**
 @brief 防冲撞
 @param pSnr -[out] 卡片序列号，4字节
 @return 状态值，MI OK - 成功；MI_ERR - 失败
*/
static char pcdAnticoll(uint8_t *pSnr)
{
    char status;
    uint8_t i, snrCheck = 0;
    uint32_t len;
    uint8_t comMF522Buf[MAXRLEN];

    clearBitMask(Status2Reg, 0x08);             // 寄存器包含接收器和发送器和数据模式检测器的状态标志
    writeRawRc(BitFramingReg, 0x00);            // 不启动数据发送，接收的LSB位存放在位0，接收到的第二位放在位1，定义发送的最后一个字节位数为8
    clearBitMask(CollReg, 0x80);                // 所有接收的位在冲突后将被清除

    comMF522Buf[0] = PICC_ANTICOLL1;
    comMF522Buf[1] = 0x20;

    status = pcdComMF522(PCD_TRANSCEIVE, comMF522Buf, 2, comMF522Buf, &len);

    if(status == MI_OK)
    {
        for(i = 0; i < 4; i++)
        {
            *(pSnr + i) = comMF522Buf[i];
            snrCheck ^= comMF522Buf[i];
        }
        if(snrCheck != comMF522Buf[i])          // 返回四个字节，最后一个字节为校验位
        {
            status = MI_ERR;
        }
    }

    setBitMask(CollReg, 0x80);

    return status;
}

/**
 @brief 选定卡片
 @param pSnr -[in] 卡片序列号，4字节
 @return 状态值，MI OK - 成功；MI_ERR - 失败
*/
static char pcdSelect(uint8_t *pSnr)
{
    char status;
    uint8_t i;
    uint8_t comMF522Buf[MAXRLEN];
    uint32_t len;

    comMF522Buf[0] = PICC_ANTICOLL1;
    comMF522Buf[1] = 0x70;
    comMF522Buf[6] = 0;

    for(i = 0; i < 4; i++)
    {
        comMF522Buf[i + 2] = *(pSnr + i);
        comMF522Buf[6] ^= *(pSnr + i);
    }

    calulateCRC(comMF522Buf, 7, &comMF522Buf[7]);

    clearBitMask(Status2Reg, 0x08);

    status = pcdComMF522(PCD_TRANSCEIVE, comMF522Buf, 9, comMF522Buf, &len);

    if((status == MI_OK ) && (len == 0x18))
    {
        status = MI_OK;
    }
    else
    {
        status = MI_ERR;
    }

    return status;
}

/**
 @brief 验证卡片密码
 @param authMode -[in] 密码验证模式，0x60 验证A密钥，0x61 验证B密钥
 @param addr -[in] 块地址
 @param pKey -[in] 密码
 @param pSnr -[in] 卡片序列号，4字节
 @return 状态值，MI OK - 成功；MI_ERR - 失败
*/
static char pcdAuthState(uint8_t authMode, uint8_t addr, uint8_t *pKey, uint8_t *pSnr)
{
    char status;
    uint8_t i, comMF522Buf[MAXRLEN];
    uint32_t len;

    comMF522Buf[0] = authMode;
    comMF522Buf[1] = addr;

    for(i = 0; i < 6; i++)
    {
        comMF522Buf[i + 2] = *(pKey + i);
    }

    for(i = 0; i < 6; i++)
    {
        comMF522Buf[i + 8] = *(pSnr + i);
    }

    status = pcdComMF522(PCD_AUTHENT, comMF522Buf, 12, comMF522Buf, &len);

    if((status != MI_OK ) || ( ! (readRawRc(Status2Reg) & 0x08)))
    {
        status = MI_ERR;
    }

    return status;
}

/**
 @brief 读取M1卡一块数据
 @param addr -[in] 块地址
 @param pData -[out] 读出的数据，16字节
 @return 状态值，MI OK - 成功；MI_ERR - 失败
*/
static char pcdRead(uint8_t addr, uint8_t *pData)
{
    char status;
    uint8_t i, comMF522Buf[MAXRLEN];
    uint32_t len;

    comMF522Buf[0] = PICC_READ;
    comMF522Buf[1] = addr;

    calulateCRC(comMF522Buf, 2, &comMF522Buf[2]);

    status = pcdComMF522(PCD_TRANSCEIVE, comMF522Buf, 4, comMF522Buf, &len);

    if((status == MI_OK) && (len == 0x90))
    {
        for(i = 0; i < 16; i++)
        {
            *(pData + i) = comMF522Buf[i];
        }
    }
    else
    {
        status = MI_ERR;
    }

    return status;
}

/**
 @brief 写入M1卡一块数据
 @param addr -[in] 块地址
 @param pData -[out] 写入的数据，16字节
 @return 状态值，MI OK - 成功；MI_ERR - 失败
*/
static char pcdWrite(uint8_t addr, uint8_t *pData)
{
    char status;
    uint8_t i, comMF522Buf[MAXRLEN];
    uint32_t len;

    comMF522Buf[0] = PICC_WRITE;
    comMF522Buf[1] = addr;

    calulateCRC(comMF522Buf, 2, &comMF522Buf[2]);

    status = pcdComMF522(PCD_TRANSCEIVE, comMF522Buf, 4, comMF522Buf, &len);
    if((status != MI_OK) || (len != 4) || ((comMF522Buf[0] & 0x0F) != 0x0A))
    {
        status = MI_ERR;
    }

    if(status == MI_OK)
    {
        for(i = 0; i < 16; i++)
        {
            comMF522Buf[i] = *(pData + i);
        }
        calulateCRC(comMF522Buf, 16, &comMF522Buf[16]);

        status = pcdComMF522(PCD_TRANSCEIVE, comMF522Buf, 18, comMF522Buf, &len);
        if((status != MI_OK) || (len != 4) || ((comMF522Buf[0] & 0x0F) != 0x0A))
        {
            status = MI_ERR;
        }
    }

    return status;
}

/**
 @brief 复位RC522
 @return 无
*/
static void pcdReset(void)
{
    // 需先保持高电平，后给个下降沿
    NFC_GPIO_Write(NFC_RST_LOW);
    delayMs(5);
    NFC_GPIO_Write(NFC_RST_HIGH);
    delayMs(10);

    writeRawRc(CommandReg, PCD_RESETPHASE);     // 和MI卡通讯，CRC初始值0x6363
    delayMs(1);
    writeRawRc(ModeReg, 0x3D);
    writeRawRc(TReloadRegL, 30);
    writeRawRc(TReloadRegH, 0);
    writeRawRc(TModeReg, 0x8D);
    writeRawRc(TPrescalerReg, 0x3E);
    writeRawRc(TxASKReg, 0x40);
}

/**
 @brief 用MF522计算CRC16
 @param pInData -[in] 计算CRC16的数组
 @param len -[in] 计算CRC16的数组字节长度
 @param pOutData -[out] 存放计算结果存放的首地址
 @return 无
*/
static void calulateCRC(uint8_t *pInData, uint8_t len, uint8_t *pOutData)
{
    uint8_t i, n;

    clearBitMask(DivIrqReg, 0x04);
    writeRawRc(CommandReg, PCD_IDLE);
    setBitMask(FIFOLevelReg, 0x80);

    for(i = 0; i < len; i++)
    {
        writeRawRc(FIFODataReg, *(pInData + i));
    }

    writeRawRc(CommandReg, PCD_CALCCRC);

    i = 0xFF;

    do
    {
        n = readRawRc(DivIrqReg);
        i--;
    }
    while((i != 0) && ! (n & 0x04));

    pOutData[0] = readRawRc(CRCResultRegL);
    pOutData[1] = readRawRc(CRCResultRegM);
}

/**
 @brief 通过MFRC522和ISO14443卡通讯
 @param command -[in] RC522命令字
 @param pInData -[in] 通过RC522发送到卡片的数据
 @param inLenByte -[in] 发送数据的字节长度
 @param pOutData -[out] 接收到的卡片返回数据
 @param pOutLenBit -[out] 返回数据的位长度
 @return 状态值，MI OK - 成功；MI_ERR - 失败
*/
static char pcdComMF522(uint8_t command, uint8_t *pInData, uint8_t inLenByte, uint8_t *pOutData, uint32_t *pOutLenBit)
{
    char status = MI_ERR;
    uint8_t irqEn = 0x00;
    uint8_t waitFor = 0x00;
    uint8_t lastBits;
    uint8_t n;
    uint32_t i;
    uint8_t j;

    switch(command)
    {
    case PCD_AUTHENT:
        irqEn = 0x12;
        waitFor = 0x10;
        break;
    case PCD_TRANSCEIVE:
        irqEn = 0x77;
        waitFor = 0x30;
        break;
    default:
        break;
    }

    writeRawRc(ComIEnReg, irqEn | 0x80);
    clearBitMask(ComIrqReg, 0x80);
    writeRawRc(CommandReg, PCD_IDLE);
    setBitMask(FIFOLevelReg, 0x80);             // 清空FIFO

    for(i = 0; i < inLenByte; i++)
    {
        writeRawRc(FIFODataReg, pInData[i]);    // 数据写入FIFO
    }
    writeRawRc(CommandReg, command);            // 命令写入命令寄存器

    if(command == PCD_TRANSCEIVE)
    {
        setBitMask(BitFramingReg, 0x80);        // 开始发送
    }

    i = 6000;                                   // 根据时钟频率调整，操作M1卡最大等待时间25ms 2000?
    do
    {
        n = readRawRc(ComIrqReg);
        i--;
    }
    while((i != 0) && !(n & 0x01) && !(n & waitFor));
    clearBitMask(BitFramingReg, 0x80);

    if(i != 0)
    {
        j = readRawRc(ErrorReg);
        if(!(j & 0x1B))
        {
            status = MI_OK;
            if(n & irqEn & 0x01)
            {
                status = MI_NOTAGERR;
            }
            if(command == PCD_TRANSCEIVE)
            {
                n = readRawRc(FIFOLevelReg);
                lastBits = readRawRc(ControlReg) & 0x07;
                if(lastBits)
                {
                    *pOutLenBit = (n - 1) * 8 + lastBits;
                }
                else
                {
                    *pOutLenBit = n * 8;
                }
                if(n == 0)
                {
                    n = 1;
                }
                if(n > MAXRLEN)
                {
                    n = MAXRLEN;
                }
                for(i = 0; i < n; i++)
                {
                    pOutData[i] = readRawRc(FIFODataReg);
                }
            }
        }
        else
        {
          status = MI_ERR;
        }
    }

    setBitMask(ControlReg, 0x80);               // stop timer now
    writeRawRc(CommandReg, PCD_IDLE);

    return status;
}

/**
 @brief 开启天线【每次启动或关闭天线发射之间至少有1ms的间隔】
 @return 无
*/
static void pcdAntennaOn(void)
{
    uint8_t temp;
    temp = readRawRc(TxControlReg);
    if(!(temp & 0x03))
    {
        setBitMask(TxControlReg, 0x03);
    }
}

/**
 @brief 关闭天线
 @return 无
*/
static void pcdAntennaOff(void)
{
    clearBitMask(TxControlReg, 0x03);
}

/**
 @brief 置RC522寄存器位
 @param reg -[in] 寄存器地址
 @param mask -[in] 置位值
 @return 无
*/
static void setBitMask(uint8_t reg, uint8_t mask)
{
    char temp = 0x00;
    temp = readRawRc(reg) | mask;
    writeRawRc(reg, temp | mask);               // set bit mask
}

/**
 @brief 清RC522寄存器位
 @param reg -[in] 寄存器地址
 @param mask -[in] 清位值
 @return 无
*/
static void clearBitMask(uint8_t reg, uint8_t mask)
{
    char temp = 0x00;
    temp = readRawRc(reg) & (~mask);
    writeRawRc(reg, temp);                      // clear bit mask
}

/**
 @brief 写RC522寄存器
 @param addr -[in] 寄存器地址
 @param writeData -[in] 写入数据
 @return 无
*/
static void writeRawRc(uint8_t addr, uint8_t writeData)
{
    SPI_CS_LOW;

    addr <<= 1;
    addr &= 0x7e;

    NFC_SPI_Write(&addr, 1);
    NFC_SPI_Write(&writeData, 1);

    SPI_CS_HIGH;
}

/**
 @brief 读RC522寄存器
 @param addr -[in] 寄存器地址
 @return 读出一字节数据
*/
static uint8_t readRawRc(uint8_t addr)
{
    uint8_t readData;

    SPI_CS_LOW;

    addr <<= 1;
    addr |= 0x80;

    NFC_SPI_Write(&addr, 1);
    NFC_SPI_Read(&readData, 1);

    SPI_CS_HIGH;

    return readData;
}

/**
 @brief 毫秒级延时函数
 @param time -[in] 延时时间（毫秒）
 @return 无
*/
static void delayMs(uint8_t time)
{
    vTaskDelay(time / portTICK_PERIOD_MS);
}

/****************************************************END OF FILE****************************************************/