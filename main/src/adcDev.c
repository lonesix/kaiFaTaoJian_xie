
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
 
#include "hal/adc_types.h"
#include "esp_adc/adc_continuous.h"
 
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "adcDev.h"
 
ADC_MANAGER ADC;

adc_cali_handle_t cali_handle;
 
adc_continuous_handle_t conti_handle;
 
int adc_num;                    // 缓冲区大小
// int xVal = 0, yVal = 0;   
int lightVal = 0,soilVal = 0,flameVal = 0,waterLevelVal = 0     ;  
uint8_t* adc_val;               // 缓冲区
extern uint8_t InitNum;
bool adc_callback(adc_continuous_handle_t handle,const adc_continuous_evt_data_t* edata,void* user_data) {
    adc_num = edata->size;                                          // 获取缓冲区的大小
    adc_val = edata->conv_frame_buffer;                             // 获取转换结果
    if (adc_num == InitNum*4) {                                             // 将转换结果(4个byte)合成一个int
        lightVal = (((uint16_t)adc_val[1] & 0x0F) << 8) | adc_val[0];   //因为我们设置的是输出12bit,因此需要把16bit的高4位去掉,所以需要&0x0F
        soilVal = (((uint16_t)adc_val[5] & 0x0F) << 8) | adc_val[4];
        flameVal = (((uint16_t)adc_val[9] & 0x0F) << 8) | adc_val[8];
        waterLevelVal = (((uint16_t)adc_val[13] & 0x0F) << 8) | adc_val[12];
        ADC.Light.rawVal = lightVal;
        ADC.Soil.rawVal = soilVal;
        ADC.Flame.rawVal = flameVal;
        ADC.WaterLevel.rawVal = waterLevelVal;
        return true;
    }
    return false;
}
void adc_cali_init(void){
    adc_cali_curve_fitting_config_t cali_initer = {
        .atten = ADC_ATTEN_DB_11,
        .bitwidth = ADC_BITWIDTH_12,
        .chan = LIGHT_ADC_CHANNEL,
        .unit_id = ADC_UNIT_1
    };
    adc_cali_create_scheme_curve_fitting(&cali_initer, &cali_handle);
}
void set_ADC_Init(void)
{
    ADC.isReg.Flame = 1;
    ADC.isReg.Light = 1;
    ADC.isReg.Soil = 1 ;
    ADC.isReg.WaterLevel = 1;
}
uint8_t InitNum = 0;
void nADC_init(void) {
    // uint8_t InitNum = 0;
    if (ADC.isReg.Flame)
    {
        ADC.Flame.isReg = ADC.isReg.Flame;
        InitNum++;
    }
    if (ADC.isReg.Light)
    {
        ADC.Light.isReg = ADC.isReg.Light;
        InitNum++;
    }
    if (ADC.isReg.Soil)
    {
        ADC.Soil.isReg = ADC.isReg.Soil;
        InitNum++;
    }
    if (ADC.isReg.WaterLevel)
    {
        ADC.WaterLevel.isReg = ADC.isReg.WaterLevel;
        InitNum++;
    }
    
    adc_continuous_handle_cfg_t conti_initer = {
        .conv_frame_size = InitNum*4,               // 2 * 4 两个通道,每个4byte
        .max_store_buf_size = 1024          // 随便填,比2*4大就行
    };
    adc_continuous_new_handle(&conti_initer, &conti_handle);
    //创建动态数组
    // adc_digi_pattern_config_t *adc_digi_arr = NULL;
    // adc_digi_arr = (adc_digi_pattern_config_t *)malloc(InitNum * sizeof(adc_digi_pattern_config_t));
    //     if (adc_digi_arr == NULL) {
    //     ESP_LOGE("nADC_Init", "Failed to allocate initial memory");
    //     ADC.Flame.isReg = 0;
    //     ADC.Light.isReg = 0;
    //     ADC.Soil.isReg = 0 ;
    //     ADC.WaterLevel.isReg = 0;
    //     return;
    // }
    // for (size_t i = 0; i < InitNum; i++) {

    //     adc_digi_arr[i] = {
    //     .atten = ADC_ATTEN_DB_11,       // 11dB衰减
    //     .bit_width = ADC_BITWIDTH_12,   // 输出12bit
    //     .channel = LIGHT_ADC_CHANNEL,       // 通道0
    //     .unit = ADC_UNIT_1              // ADC1
    // };
    // }
    adc_digi_pattern_config_t adc_digi_arr[] = {
        {
            .atten = ADC_ATTEN_DB_11,       // 11dB衰减
            .bit_width = ADC_BITWIDTH_12,   // 输出12bit
            .channel = LIGHT_ADC_CHANNEL,       // 通道0
            .unit = ADC_UNIT_1              // ADC1
        },{
            .atten = ADC_ATTEN_DB_11,   
            .bit_width = ADC_BITWIDTH_12,
            .channel = SOIL_ADC_CHANNEL,       // 通道1
            .unit = ADC_UNIT_1              
        },{
            .atten = ADC_ATTEN_DB_11,   
            .bit_width = ADC_BITWIDTH_12,
            .channel = FLAME_ADC_CHANNEL,       // 通道1
            .unit = ADC_UNIT_1              
        },{
            .atten = ADC_ATTEN_DB_11,   
            .bit_width = ADC_BITWIDTH_12,
            .channel = WATER_ADC_CHANNEL,       // 通道1
            .unit = ADC_UNIT_1              
        }
    };
    uint8_t arr_index = 0;
    if (ADC.Flame.isReg)
    {
        adc_digi_arr[arr_index].channel = FLAME_ADC_CHANNEL;
        arr_index++;
    }
    if (ADC.Light.isReg)
    {
        adc_digi_arr[arr_index].channel = LIGHT_ADC_CHANNEL;
        arr_index++;
    }
    if (ADC.Soil.isReg)
    {
        adc_digi_arr[arr_index].channel = SOIL_ADC_CHANNEL;
        arr_index++;
    }
    if (ADC.WaterLevel.isReg)
    {
        adc_digi_arr[arr_index].channel = WATER_ADC_CHANNEL;
        arr_index++;
    }
    adc_continuous_config_t conti_config = {
        .adc_pattern = adc_digi_arr,                // 配置的通道数组
        .conv_mode = ADC_CONV_SINGLE_UNIT_1,        // 只使用ADC1
        .format = ADC_DIGI_OUTPUT_FORMAT_TYPE2,     // 没得选,只能选type2
        .pattern_num = InitNum,                           // 使用的通道数
        .sample_freq_hz = 20000                     // 采样频率,官方demo用的20 * 1000
    };
    adc_continuous_config(conti_handle, &conti_config);
 
    adc_continuous_evt_cbs_t conti_evt = {
        .on_conv_done = adc_callback,               // 绑定转换完毕后的回调函数
    };
    adc_continuous_register_event_callbacks(conti_handle,&conti_evt,NULL);
    
    adc_continuous_start(conti_handle);             // 开启连续转换
    adc_cali_init();
}
 
void ADC_raw_to_voltage()
{
    int newlightVal ,newsoilVal,newflameVal ,newwaterLevelVal      ;  
            
        
        
        
    if (ADC.Flame.isReg)
    {
        adc_cali_raw_to_voltage(cali_handle,flameVal,&newflameVal);
    }
    if (ADC.Light.isReg)
    {
        adc_cali_raw_to_voltage(cali_handle,lightVal,&newlightVal);
    }
    if (ADC.Soil.isReg)
    {
        adc_cali_raw_to_voltage(cali_handle,soilVal,&newsoilVal);
    }
    if (ADC.WaterLevel.isReg)
    {
        adc_cali_raw_to_voltage(cali_handle,waterLevelVal,&newwaterLevelVal);
    }    
}

 void ADC_raw_to_actualVal()
{
    //水位传感器：检测水越深，水深传感器模拟值越大；反之，水深传感器模拟值越小
    //环境亮度传感器：当手遮住光敏传感器时，显示数值将变小；当光线变强时，显示数值将变大；
    //火焰传感器：原理红外线，
    //土壤湿度传感器：当插到土壤或者水中时，DO输出处于低电平。 如果感应板干燥时，输出将返回高水平状态。
}

void print_adc(void) {
    // nADC_init();
    // adc_cali_init();
    int newlightVal ,newsoilVal,newflameVal ,newwaterLevelVal      ;  
    // int lightVal = 0,soilVal = 0,flameVal = 0,waterLevelVal = 0     ;  
        // for(int i = 0; i < adc_num; ++i){                   //打印看看缓冲区
        //     printf("%d\t",adc_val[i]);
        // }
        
            /* code */
            // printf("%p,%p",cali_handle,&newlightVal);
        
        
        adc_cali_raw_to_voltage(cali_handle,lightVal,&newlightVal);
        adc_cali_raw_to_voltage(cali_handle,soilVal,&newsoilVal);
        adc_cali_raw_to_voltage(cali_handle,flameVal,&newflameVal);
        adc_cali_raw_to_voltage(cali_handle,waterLevelVal,&newwaterLevelVal);
        printf("lightVal is %d soilVal is %d newlightVal is %dmV newsoilVal is %dmV\r\n", lightVal, soilVal,newlightVal,newsoilVal);
        printf("flameVal is %d waterLevelVal is %d newflameVal is %dmV newwaterLevelVal is %dmV\r\n", flameVal, waterLevelVal,newflameVal,newwaterLevelVal);
        // vTaskDelay(1000 / portTICK_PERIOD_MS);
    
}





