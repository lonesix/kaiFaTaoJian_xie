/*********************************************************************
 * INCLUDES
 */
#include <string.h>
#include "esp_timer.h"
#include "esp_log.h"

#include "board_gpio.h"
#include "board_mfrc522.h"
#include "board_spi.h"
#include "user_nfc.h"

static void nfcCooldownTimerCallback(void *arg);
static void handleCardId(uint8_t *pData);

/*********************************************************************
 * LOCAL VARIABLES
 */ 
static volatile uint8_t s_nfcCooldown = 0;
static esp_timer_handle_t s_nfcCooldownTimer = NULL;
static volatile uint8_t s_nfcTriggerEvent = 0;
static uint8_t s_cardId[4] = {0};

/*********************************************************************
 * PUBLIC FUNCTIONS
 */
/**
 @brief NFC模块初始化
 @param 无
 @return 无
*/
void NFC_Init(void)
{
    NFC_GPIO_Init();
    NFC_SPI_Init();
    MFRC522_Init();
    CreateNfcCooldownTimer();
}

/**
 @brief 创建刷卡冷却定时器
 @param 无
 @return 无
*/
void CreateNfcCooldownTimer(void)
{
    esp_timer_create_args_t nfcCooldownTimerArg = 
    { 
        .callback = &nfcCooldownTimerCallback,                      // 设置回调函数
        .arg = NULL,                                                // 不携带参数
        .name = "nfcCooldownTimer"                                  // 定时器名字
    };
    esp_timer_create(&nfcCooldownTimerArg, &s_nfcCooldownTimer);
}

/**
 @brief 开启刷卡冷却定时器
 @param 无
 @return 无
*/
void StartNfcCooldownTimer(void)
{
    s_nfcCooldown = true;
    esp_timer_start_once(s_nfcCooldownTimer, NFC_COOLDOWN_PERIOD * 1000);
}

/**
 @brief 获取卡ID
 @param 无
 @return 无
*/
void GetCardId(uint8_t *pData)
{
    memcpy(pData, s_cardId, 4);
}

/**
 @brief 获取刷卡触发事件
 @param 无
 @return 触发事件类型
*/
uint8_t GetNfcTriggerEvent(void)
{
    return s_nfcTriggerEvent;
}

/**
 @brief 获取按键触发事件
 @param event -[in] 触发事件类型
 @return 无
*/
void SetNfcTriggerEvent(uint8_t event)
{
    s_nfcTriggerEvent = event;
}


/*********************************************************************
 * LOCAL FUNCTIONS
 */
/**
 @brief 刷卡冷却定时器的回调函数
 @param 无
 @return 无
*/
static void nfcCooldownTimerCallback(void *arg)
{
    s_nfcCooldown = false;
}

/**
 @brief 处理刷卡数据
 @param pData -[in] 接收数据
 @return 无
*/
static void handleCardId(uint8_t *pData)
{
    if(true == s_nfcCooldown)
    {
        return;
    }

    s_cardId[0] = pData[6];
    s_cardId[1] = pData[5];
    s_cardId[2] = pData[4];
    s_cardId[3] = pData[3];

    SetNfcTriggerEvent(NFC_EVENT);
    StartNfcCooldownTimer();
}

/****************************************************END OF FILE****************************************************/