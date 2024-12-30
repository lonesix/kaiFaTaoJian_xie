#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "driver/gpio.h" 
#include <esp_log.h>
#include "esp_timer.h"
#include "esp_rom_sys.h"
 
#include "DHT11.h"
const static char *TAG = "DHT11";
 
// 温度 湿度buffer
uint8_t buffer[5];
int64_t phase_duration[3]={0};
int64_t bit_duration_low[40]={0};
int64_t bit_duration_high[40]={0};
DHT11Dev dht11;
// DHT11 初始化引脚，等待1s上电时间
void DHT11_Init()
{
    dht11.isReg = true;
    gpio_config_t cnf={
    .mode = GPIO_MODE_OUTPUT_OD,
    .pin_bit_mask =  1ULL<<DHT11_GPIO,
    .pull_up_en=1,
    };
    gpio_config(&cnf);
    vTaskDelay(1200/portTICK_PERIOD_MS);
}
 
/*
timeout单位是us
*/
esp_err_t wait_pin_state(uint32_t timeout, int expected_pin_state)
{
    /*用这段固定时间的代码也可以*/
    // esp_rom_delay_us(timeout);
    // if(gpio_get_level(DHT11_GPIO)==expected_pin_state)
    //     return ESP_OK;
    // else
    //     return ESP_FAIL;
 
    /*建议用这段代码*/
    int64_t start_time;
    start_time=esp_timer_get_time();
    while(esp_timer_get_time()-start_time<=timeout)
    {
        if(gpio_get_level(DHT11_GPIO)==expected_pin_state)
            return ESP_OK;
        esp_rom_delay_us(1);
    }
    return ESP_FAIL;
}
 
esp_err_t DataRead()
{
    int64_t time_since_waiting_start;
    esp_err_t result=ESP_FAIL;
    memset(buffer,0,sizeof(buffer));
    
    gpio_set_direction(DHT11_GPIO,GPIO_MODE_OUTPUT_OD);
    gpio_set_level(DHT11_GPIO,1);
    vTaskDelay(25/portTICK_PERIOD_MS);
    gpio_set_level(DHT11_GPIO,0);
    vTaskDelay(25/portTICK_PERIOD_MS);
    gpio_set_level(DHT11_GPIO,1);
    time_since_waiting_start=esp_timer_get_time();
    gpio_set_direction(DHT11_GPIO,GPIO_MODE_INPUT);
    
    result=wait_pin_state(19,0);  //
    if(result == ESP_FAIL)
    {
        ESP_LOGE(TAG, "Phase A Fail, slave not set LOW.");
        return ESP_FAIL;
    }
    phase_duration[0]=esp_timer_get_time()-time_since_waiting_start;
    time_since_waiting_start=esp_timer_get_time();
    /*等从机拉低总线83us，再拉高87us*/
    result=wait_pin_state(85,1);
        if (result == ESP_FAIL)
        {
            ESP_LOGE(TAG, "Phase B Fail, slave not set HIGH.");
            return ESP_FAIL;
        }
    phase_duration[1]=esp_timer_get_time()-time_since_waiting_start;
    time_since_waiting_start=esp_timer_get_time();
    result = wait_pin_state(90, 0);
        if (result == ESP_FAIL)
        {
            ESP_LOGE(TAG, "Phase C Fail, slave not set LOW to start sending.");
            return ESP_FAIL;
        }
    phase_duration[2]=esp_timer_get_time()-time_since_waiting_start;
    time_since_waiting_start=esp_timer_get_time();
 
    for(int j=0;j<5;j++)
    {
        for(int i =0;i<8;i++)
        {
            /*数位低电平时间*/
            while(gpio_get_level(DHT11_GPIO)==0)
            {
                esp_rom_delay_us(1);
            }
            bit_duration_low[j*8+i]=esp_timer_get_time()-time_since_waiting_start;
 
            /*检测数字高位的时间长度来判断是1或0*/
            time_since_waiting_start=esp_timer_get_time();
            do
            {
                if (gpio_get_level(DHT11_GPIO) == 0)
                {
                    if (esp_timer_get_time() - time_since_waiting_start > 40)
                    {
                        /*数字1*/
                        buffer[j] = buffer[j] | (1U << (7 - i));
                        /*数字0不用处理*/
                    }
                    
                    bit_duration_high[j*8+i]=esp_timer_get_time()-                    
                    time_since_waiting_start;
                    time_since_waiting_start=esp_timer_get_time();
                    break;
                }
            } while (esp_timer_get_time() - time_since_waiting_start < 74); 
        }
    }
 
    result=wait_pin_state(56,1);
    if (result == ESP_FAIL)
    {
        ESP_LOGE(TAG, "Data is all read.But CAN not set high.");
        return ESP_FAIL;
    }
    return ESP_OK;
}
//:8bit湿度整数数据+8bit湿度小数数据 +8bi温度整数数据+8bit温度小数数据
float combine_to_float(uint8_t integer_part, uint8_t fractional_part) {
    // 假设小数部分表示的是 0 到 100 之间的值，对应于 0.0 到 1.0 之间的浮点数
    float fraction;
    if (fractional_part<10)
    {
        fraction = (float)fractional_part / 10.0;
    }else
    {
        fraction = (float)fractional_part / 100.0;
    }
    
    float result = integer_part + fraction;
    return result;
}
uint8_t* DHT11()
{
    esp_err_t result;
    uint8_t i,j;
    static uint8_t DHT11_Data[4];
    memset(DHT11_Data,0,sizeof(DHT11_Data));
    memset(phase_duration,0,sizeof(phase_duration));
    memset(bit_duration_low,0,sizeof(bit_duration_low));
    memset(bit_duration_high,0,sizeof(bit_duration_high));
    result = DataRead();
    if (result==ESP_OK)
    {
        ESP_LOGI(TAG,"Reading data succeed.");
        if(((buffer[0]+buffer[1]+buffer[2]+buffer[3])&0xFF) != buffer[4])
        {
            ESP_LOGE(TAG, "But checksum error.");
            return DHT11_Data;
        }
            
        ESP_LOGI(TAG, "Temperature is:%d.%d, Humidity is:%d.%d", buffer[2], buffer[3], buffer[0],buffer[1]);
        dht11.Value.Hum=combine_to_float(buffer[0],buffer[1]);
        dht11.Value.Temp=combine_to_float(buffer[2],buffer[3]);
        for(j=0;j<4;j++)
        {
            DHT11_Data[j]=buffer[j+1];
            dht11.rawVal[j]=buffer[j+1];
        }
    }else
    {
        ESP_LOGI(TAG,"Reading data failed.");
        ESP_LOGI(TAG,"PhaseA duration is:%lld",phase_duration[0]);
        ESP_LOGI(TAG,"PhaseB duration is:%lld",phase_duration[1]);
        ESP_LOGI(TAG,"PhaseC duration is:%lld",phase_duration[2]);
        ESP_LOGI(TAG,"Bit duration is as follows:");
        for(j=0;j<5;j++)
        {
            for(i=0;i<8;i++)
            {
                printf("%lld,%lld-",bit_duration_low[j*8+i],bit_duration_high[j*8+i]);
            }
            printf("\n");
        }
    }
    
    return DHT11_Data;



}
// // 主函数
// void app_main(void)
// {
// 	esp_err_t result;
//     uint8_t i,j;
//     DHT11_Init();
//     while(1)
//     {
//         memset(phase_duration,0,sizeof(phase_duration));
//         memset(bit_duration_low,0,sizeof(bit_duration_low));
//         memset(bit_duration_high,0,sizeof(bit_duration_high));
//         result = DataRead();
//         if (result==ESP_OK)
//         {
//             ESP_LOGI(TAG,"Reading data succeed.");
//             if(((buffer[0]+buffer[1]+buffer[2]+buffer[3])&0xFF) != buffer[4])
//                 ESP_LOGE(TAG, "But checksum error.");
//             ESP_LOGI(TAG, "Temperature is:%d.%d, Humidity is:%d.%d", buffer[2], buffer[3], buffer[0],buffer[1]);
//         }
//         ESP_LOGI(TAG,"PhaseA duration is:%lld",phase_duration[0]);
//         ESP_LOGI(TAG,"PhaseB duration is:%lld",phase_duration[1]);
//         ESP_LOGI(TAG,"PhaseC duration is:%lld",phase_duration[2]);
//         ESP_LOGI(TAG,"Bit duration is as follows:");
//         for(j=0;j<5;j++)
//         {
//             for(i=0;i<8;i++)
//             {
//                 printf("%lld,%lld-",bit_duration_low[j*8+i],bit_duration_high[j*8+i]);
//             }
//             printf("\n");
//         }
//     }
// }

// void DHT11(void)   //温湿传感启动
// {
//     OutputLow();
//     Delay_ms(19);  //>18MS
//     OutputHigh();
//     InputInitial(); //输入
//     ets_delay_us(30);llllllllll
//     if(!getData())//表示传感器拉低总线
//     {
//         ucharFLAG=2;
//         //等待总线被传感器拉高
//         while((!getData())&&ucharFLAG++)
//           ets_delay_us(10);
//         //等待总线被传感器拉低
//         while((getData())&&ucharFLAG++)
//           ets_delay_us(10);
//         COM();//读取第1字节，
//         ucharRH_data_H_temp=ucharcomdata;
//         COM();//读取第2字节，
//         ucharRH_data_L_temp=ucharcomdata;
//         COM();//读取第3字节，
//         ucharT_data_H_temp=ucharcomdata;
//         COM();//读取第4字节，
//         ucharT_data_L_temp=ucharcomdata;
//         COM();//读取第5字节，
//         ucharcheckdata_temp=ucharcomdata;
//         OutputHigh();
//         //判断校验和是否一致
//         uchartemp=(ucharT_data_H_temp+ucharT_data_L_temp+ucharRH_data_H_temp+ucharRH_data_L_temp);
//         if(uchartemp==ucharcheckdata_temp)
//         {
//             //校验和一致，
//             ucharRH_data_H=ucharRH_data_H_temp;
//             ucharRH_data_L=ucharRH_data_L_temp;
//             ucharT_data_H=ucharT_data_H_temp;
//             ucharT_data_L=ucharT_data_L_temp;
//             ucharcheckdata=ucharcheckdata_temp;
//             //保存温度和湿度
//             Humi=ucharRH_data_H;
//             Humi=((uint16)Humi<<8|ucharRH_data_L)/10;
 
//             Temp=ucharT_data_H;
//             Temp=((uint16)Temp<<8|ucharT_data_L)/10;
//         }
//         else
//         {
//           Humi=100;
//           Temp=100;
//         }
//     }
//     else //没用成功读取，返回0
//     {
//     	Humi=0,
//     	Temp=0;
//     }
 
//     OutputHigh(); //输出
// }
 
 
// void app_main()
// {
//     char dht11_buff[50]={0};
 
//     while(1)
//     {
//       DHT11(); //读取温湿度
//       printf("Temp=%.2f--Humi=%.2f%%RH \r\n", Temp,Humi);
//       vTaskDelay(300);  //延时300毫秒
//     }
// }
