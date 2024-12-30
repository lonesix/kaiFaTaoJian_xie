/*********************************************************************
 * INCLUDES
 */
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "nvs_flash.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_timer.h"
#include "esp_log.h" 

#include "board_mfrc522.h"
#include "board_spi.h"

#include "sg90.h"
#include <ws2812_control.h>

#include "DHT11.h"
#include "iicDev.h"
#include "gpioDev.h"
#include "adcDev.h"
#include "rc522.h"
#include "ws2812.h"
#include "JsonPact.h"

/*********************************************************************
 * LOCAL VARIABLES
 */ 
static const char *TAG = "USER_MAIN";
/******************************************************************
 * EXAMPLE
 */
void DEMO();
void All_EXAMPLE();
void mpu6050_example();
void rc522_example();
void DHT11_example();
void Body_Hall_example();
void sg90_example();
void Light_Soil_Flame_WaterLevel_example();
void ws2812_example();
// 定义传感器类型枚举
typedef enum {
    DEMO_EXAMPLE,
    ALL_EXAMPLE,
    MPU6050_EXAMPLE,
    RC522_EXAMPLE,
    DHT11_EXAMPLE,
    BODY_HALL_EXAMPLE,
    SG90_EXAMPLE,
    LIGHT_SOIL_FLAME_WATERLEVEL_EXAMPLE,
    WS2812_EXAMPLE,
   
} SensorExample;
// 主函数
void app_main(void)
{
    SensorExample ExampleIndex = DEMO_EXAMPLE;
    switch (ExampleIndex)
    {
    case DEMO_EXAMPLE:
        DEMO();
    break;    
    case ALL_EXAMPLE:
        All_EXAMPLE();
    break;
    case MPU6050_EXAMPLE:
        mpu6050_example();
    break;
    case RC522_EXAMPLE:
        rc522_example();
    break;
    case DHT11_EXAMPLE:
        DHT11_example();
    break;
    case BODY_HALL_EXAMPLE:
        Body_Hall_example();
    break;
    case SG90_EXAMPLE:
        sg90_example();
    break;
    case LIGHT_SOIL_FLAME_WATERLEVEL_EXAMPLE:
        Light_Soil_Flame_WaterLevel_example();
    break;
    case WS2812_EXAMPLE:
        ws2812_example();
    break;
    
    default:
        break;
    }

}


void DEMO()
{
    sg90_init();
    sg90_SetAngle(0);

    ws2812_Init();

    mpu6050_init();

    DHT11_Init();

    Body_Init();
    Hall_Init();
    
    set_ADC_Init();//可自行设置
    nADC_init();

    rc522_Init();
    /*-------------------------- 创建线程 ---------------------------*/
    Create_Demo_Task();
}
/****************************************** */
void All_EXAMPLE()
{
    sg90_init();
    sg90_SetAngle(0);

    ws2812_Init();

    mpu6050_init();

    DHT11_Init();

    Body_Init();
    Hall_Init();
    
    set_ADC_Init();//可自行设置
    nADC_init();

    rc522_Init();
    while (1)
    {
        rc522_read_cardid();
        if (rc522.value == ACTIVE)
        {
            ESP_LOGI(TAG, "card: %02x%02x%02x%02x", rc522.card[0], rc522.card[1], rc522.card[2], rc522.card[3]);
            mpu6050_read();
            DHT11();
            Body_read();
            Hall_read();
            printf("Body:%d,Hall:%d\r\n",Body.rawVal,Hall.rawVal);
            print_adc();
            ws2812_led_set_on( COLOR_GREEN);//单灯绿色
            sg90_SetAngle(180);

        }else
        {
            sg90_SetAngle(0);
            ws2812_led_set_on( COLOR_RED);//单灯红色
        }
        
        vTaskDelay(1000/portTICK_PERIOD_MS);
    }
}
/**************************单例*****************************/
//mpu6050——初始化，每2秒读取一次数据
void mpu6050_example()
{
    mpu6050_init();
    while(1)
    {
        
        mpu6050_read();
        vTaskDelay(2000/portTICK_PERIOD_MS);
    }
}
//mpu6050

//rc522——初始化，每500ms读取一下卡号，如果卡存在则打印卡号
void rc522_example()
{
    rc522_Init();   
    while(1)
    {
        rc522_read_cardid();
        if (rc522.value == ACTIVE)
        {
            ESP_LOGI(TAG, "card: %02x%02x%02x%02x", rc522.card[0], rc522.card[1], rc522.card[2], rc522.card[3]);
        }

        vTaskDelay(500/portTICK_PERIOD_MS);
    }
}
//rc522

//dht11——初始化，每2秒读取一次传感器数据并打印
void DHT11_example()
{
    DHT11_Init();
    while(1)
    {
        
        DHT11();
        vTaskDelay(2000/portTICK_PERIOD_MS);
    }
}
//dht11

//Body和Hall
void Body_Hall_example()
{
    
    Body_Init();
    Hall_Init();
    while(1)
    {
        //法1
        int bodyVal,hallVal;
        bodyVal = Body_read();
        hallVal = Hall_read();
        printf("Body:%d,Hall:%d\r\n",bodyVal,hallVal);
        //法2
        Body_read();
        Hall_read();
        printf("Body:%d,Hall:%d\r\n",Body.rawVal,Hall.rawVal);
        vTaskDelay(2000/portTICK_PERIOD_MS);
    }
}
//Body和Hall

//sg90
void sg90_example()
{
    
    sg90_init();
    sg90_SetAngle(0);
    while (1)
    {

        for (uint8_t angle = 0; angle < 180; angle++)
        {
            sg90_SetAngle(angle);
            vTaskDelay(100/portTICK_PERIOD_MS);
        }
        
    }
    
}
//sg90

//Light,Soil,Flame,WaterLevel
void Light_Soil_Flame_WaterLevel_example()
{
    
    set_ADC_Init();//可自行设置
    nADC_init();
    while (1)
    {

        print_adc();
        vTaskDelay(1000/portTICK_PERIOD_MS);
       
        
    }
    
}
//Light,Soil,Flame,WaterLevel

//ws2812
void ws2812_example()
{
    ws2812_Init();
    
    while (1)
    {
        ws2812_led_set_on(COLOR_RGB(255,0,0));//全部红色亮
        vTaskDelay(2000/portTICK_PERIOD_MS);
        ws2812_led_set_pixel(0, COLOR_GREEN);//单灯绿色
        vTaskDelay(2000/portTICK_PERIOD_MS);
        ws2812_led_set_off();//全灭
        vTaskDelay(2000/portTICK_PERIOD_MS);
        ws2812_set_effect(COLOR_PINK, LED_EFFECT_BREATH);//粉色呼吸——阻塞式函数
        vTaskDelay(1000/portTICK_PERIOD_MS);
       
        
    }
}
//ws2812
