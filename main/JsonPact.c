#include <stdio.h>
#include <stdlib.h>
#include "JsonPact.h"
#include "driver/uart.h"
#include "cJSON.h"
#include "config.h"
#include <esp_log.h>
#include "string.h"

#include "ws2812.h"
#include "sg90.h"
#include "DHT11.h"
#include "iicDev.h"
#include "gpioDev.h"
#include "adcDev.h"
#include "rc522.h"
extern ADC_MANAGER ADC;
extern DHT11Dev dht11;
extern IICDev MPU6050;
extern SG90Dev sg90;
extern gpioDev Body;
extern gpioDev Hall;
extern WS2812Dev WS2812;
extern SPIDev rc522;
#define UART_NUM    UART_NUM_1        // 使用UART1
#define TX_PIN      10                // TX引脚
#define RX_PIN      11                // RX引脚
#define BUF_SIZE    1024              // 缓冲区大小
#define BAUD_RATE   115200            // 波特率
const char* TAG = "UART_Comm";
uart_port_t uart_num = UART_NUM;
int tx_pin = TX_PIN;
int rx_pin = RX_PIN;
int baud_rate = BAUD_RATE;
int buf_size = BUF_SIZE;
uint8_t* rx_buffer;                      // 接收缓冲区
void ProcessReceivedJson(cJSON *root);
void sensorMapRange_Init();
void uart_init()
{
        uart_config_t uart_config = {
        .baud_rate = baud_rate,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    ESP_ERROR_CHECK(uart_driver_install(uart_num, buf_size * 2, buf_size * 2, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(uart_num, tx_pin, rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    // ESP_ERROR_CHECK(uart_set_pin(uart_num, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    rx_buffer = (uint8_t*) malloc(buf_size);
    sensorMapRange_Init();
    ESP_LOGI(TAG, "UART%d initialized with baud rate %d", uart_num, baud_rate);
}

// 发送数据
void sendData(const char* data)
{
    uart_write_bytes(uart_num, data, strlen(data));
    ESP_LOGI(TAG, "Sent data: %s", data);
}
// 发送数据
void sendJsonData(cJSON* json)
{
   // 将 cJSON 对象转换为字符串
    char* json_str = cJSON_PrintUnformatted(json);  // 不格式化 JSON，节省空间
    
    if (json_str != NULL)
    {
        // 发送 JSON 字符串到串口
        uart_write_bytes(uart_num, json_str, strlen(json_str));
        // uart_write_bytes(uart_num, "++", strlen("++"));
        // 打印日志
        ESP_LOGI(TAG, "Sent JSON: %s", json_str);
        
        // 释放动态分配的内存
        free(json_str);
    }
    else
    {
        ESP_LOGE(TAG, "Failed to convert cJSON to string");
    }
}
//发送错误json报文
void int_to_string(int number,char *str);
typedef enum{
    NAMEISERROR = 1,
    TYPEISERROR,
    PROPERTYERROR,
    SENSORISREG,
    JSONKEYISMISS

}JsonError;
void sendErrorJson(JsonError error)
{
    // 创建一个cJSON对象
    cJSON *err = cJSON_CreateObject();
    if (err == NULL) {
        ESP_LOGE(TAG, "Failed to create JSON object");
        return;
    }
    char str[10] ={0}; 
    memset(str, 0, 10 * sizeof(char));


    // 使用sprintf将整数转换为字符串
    sprintf(str, "%d", error);
    // 向对象中添加键值对
    cJSON_AddStringToObject(err, "error", str);
    sendJsonData(err);
    cJSON_Delete(err); 

}
// 接收数据
void receiveData()
{
    int length = uart_read_bytes(uart_num, rx_buffer, buf_size, pdMS_TO_TICKS(1000));  // 超时1秒
    if (length > 0) {
        rx_buffer[length] = '\0';  // 确保字符串结尾
        ESP_LOGI(TAG, "Received data: %s", rx_buffer);
    }
}

void receiveDataCjson() {
    // 清空接收缓冲区以避免读取旧数据
    memset(rx_buffer, 0, buf_size);
    int length = uart_read_bytes(uart_num, rx_buffer, buf_size, pdMS_TO_TICKS(10));  // 超时
    if (length > 0) {
        rx_buffer[length] = '\0';  // 确保字符串结尾
        ESP_LOGI(TAG, "Received data: %s", rx_buffer);

        // 尝试解析收到的数据为 cJSON 对象
        cJSON* root = cJSON_Parse((const char*)rx_buffer);
        if (root == NULL) {
            ESP_LOGE(TAG, "Failed to parse received data as JSON");
            return;
        }

        // 调用外部接口处理解析后的 JSON
        ProcessReceivedJson(root);
        cJSON_Delete(root);  // 在任务执行完后删除 JSON 对象;
    }
}


#include <stdio.h>
#include <string.h>

// 定义传感器类型枚举
typedef enum {
    SENSOR_MPU6050,
    SENSOR_RC522,
    SENSOR_DHT11,
    SENSOR_BODY,
    SENSOR_HALL,
    SENSOR_LIGHT,
    SENSOR_SOIL,
    SENSOR_FLAME,
    SENSOR_WATERLEVEL,
    SENSOR_WS2812,
    SENSOR_SG90,
    SENSOR_UNKNOWN  // 用于处理未知传感器
} SensorType;
// 定义传感器名称的字符串数组和对应的枚举值映射表
typedef struct {
    const char* name;
    SensorType type;
} SensorMapping;
const SensorMapping sensorMappings[] = {
    {"MPU6050", SENSOR_MPU6050},
    {"RC522", SENSOR_RC522},
    {"DHT11", SENSOR_DHT11},
    {"Body", SENSOR_BODY},
    {"Hall", SENSOR_HALL},
    {"Light", SENSOR_LIGHT},
    {"Soil", SENSOR_SOIL},
    {"Flame", SENSOR_FLAME},
    {"WaterLevel", SENSOR_WATERLEVEL},
    {"WS2812", SENSOR_WS2812},
    {"SG90", SENSOR_SG90}
};
#define SENSOR_COUNT (sizeof(sensorMappings) / sizeof(sensorMappings[0]))
 
// 根据输入字符串返回传感器枚举值的函数
SensorType getSensorTypeFromString(const char* input) {
    for (int i = 0; i < SENSOR_COUNT; i++) {
        if (strcmp(input, sensorMappings[i].name) == 0) {
            return sensorMappings[i].type;
        }
    }
    return SENSOR_UNKNOWN; // 若未找到匹配的字符串，则返回未知枚举值
}
// 定义传感器属性枚举
typedef enum {
    MPU6050_DATA_ACCE_X,
    MPU6050_DATA_ACCE_Y,
    MPU6050_DATA_ACCE_Z,
    MPU6050_DATA_GYRO_X,
    MPU6050_DATA_GYRO_Y,
    MPU6050_DATA_GYRO_Z,
    MPU6050_DATA_TEMP,
    MPU6050_DATA_CUSTOM,
    MPU6050_DATA_UNKNOWN,

    RC522_DATA_CARD_ID,
    RC522_DATA_CARD_STATE,
    RC522_DATA_CUSTOM,
    RC522_DATA_UNKNOWN,

    DHT11_DATA_TEMP,
    DHT11_DATA_HUM,
    DHT11_DATA_UNKNOWN,

    BODY_DATA_RAW,
    BODY_DATA_VALUE,
    BODY_DATA_UNKNOWN,

    HALL_DATA_RAW,
    HALL_DATA_VALUE,
    HALL_DATA_UNKNOWN,

    LIGHT_DATA_RAW,
    LIGHT_DATA_CUSTOM,
    LIGHT_DATA_UNKNOWN,

    SOIL_DATA_RAW,
    SOIL_DATA_CUSTOM,
    SOIL_DATA_UNKNOWN,

    FLAME_DATA_RAW,
    FLAME_DATA_CUSTOM,
    FLAME_DATA_UNKNOWN,

    WATERLEVEL_DATA_RAW,
    WATERLEVEL_DATA_CUSTOM,
    WATERLEVEL_DATA_UNKNOWN,

    WS2812_DATA_RGB,
    WS2812_DATA_CUSTOM,
    WS2812_DATA_UNKNOWN,

    SG90_DATA_ANGLE,
    SG90_DATA_CUSTOM,
    SG90_DATA_UNKNOWN,  // 用于处理未知属性
} SensorData;

// 定义结构体来包含传感器名称和属性枚举
typedef struct {
    const char* name;
    SensorType sensorType;
    SensorData sensorData;
} SensorDataMapping;

// 创建包含所有传感器数据的结构体数组
const SensorDataMapping sensorDataMappings[] = {
    {"acce_x", SENSOR_MPU6050, MPU6050_DATA_ACCE_X},
    {"acce_y", SENSOR_MPU6050, MPU6050_DATA_ACCE_Y},
    {"acce_z", SENSOR_MPU6050, MPU6050_DATA_ACCE_Z},
    {"gyro_x", SENSOR_MPU6050, MPU6050_DATA_GYRO_X},
    {"gyro_y", SENSOR_MPU6050, MPU6050_DATA_GYRO_Y},
    {"gyro_z", SENSOR_MPU6050, MPU6050_DATA_GYRO_Z},
    {"temp", SENSOR_MPU6050, MPU6050_DATA_TEMP},
    {"custom", SENSOR_MPU6050, MPU6050_DATA_CUSTOM},

    {"card_id", SENSOR_RC522, RC522_DATA_CARD_ID},
    {"card_state", SENSOR_RC522, RC522_DATA_CARD_STATE},
    {"custom", SENSOR_RC522, RC522_DATA_CUSTOM},

    {"temp", SENSOR_DHT11, DHT11_DATA_TEMP},
    {"hum", SENSOR_DHT11, DHT11_DATA_HUM},

    {"raw", SENSOR_BODY, BODY_DATA_RAW},
    {"value", SENSOR_BODY, BODY_DATA_VALUE},

    {"raw", SENSOR_HALL, HALL_DATA_RAW},
    {"value", SENSOR_HALL, HALL_DATA_VALUE},

    {"raw", SENSOR_LIGHT, LIGHT_DATA_RAW},
    {"custom", SENSOR_LIGHT, LIGHT_DATA_CUSTOM},

    {"raw", SENSOR_SOIL, SOIL_DATA_RAW},
    {"custom", SENSOR_SOIL, SOIL_DATA_CUSTOM},

    {"raw", SENSOR_FLAME, FLAME_DATA_RAW},
    {"custom", SENSOR_FLAME, FLAME_DATA_CUSTOM},

    {"raw", SENSOR_WATERLEVEL, WATERLEVEL_DATA_RAW},
    {"custom", SENSOR_WATERLEVEL, WATERLEVEL_DATA_CUSTOM},

    {"rgb", SENSOR_WS2812, WS2812_DATA_RGB},
    {"custom", SENSOR_WS2812, WS2812_DATA_CUSTOM},

    {"angle", SENSOR_SG90, SG90_DATA_ANGLE},
    {"custom", SENSOR_SG90, SG90_DATA_CUSTOM},

    // 可以在这里添加更多传感器和属性
};

#define SENSOR_DATA_MAPPINGS_COUNT (sizeof(sensorDataMappings) / sizeof(sensorDataMappings[0]))
//搜索算法前置数据类型
typedef struct 
{
    uint8_t count;//范围数组的大小
    uint8_t *range;//范围数组指针
    SensorType sensorType;//传感器类型
    SensorData sensorData;
}SensorDataMappingRange;
SensorDataMappingRange sensorMapRange[]={
    {0,NULL,SENSOR_MPU6050,MPU6050_DATA_UNKNOWN},
    {0,NULL,SENSOR_RC522,RC522_DATA_UNKNOWN},
    {0,NULL,SENSOR_DHT11,DHT11_DATA_UNKNOWN},
    {0,NULL,SENSOR_BODY,BODY_DATA_UNKNOWN},
    {0,NULL,SENSOR_HALL ,HALL_DATA_UNKNOWN},
    {0,NULL,SENSOR_LIGHT,LIGHT_DATA_UNKNOWN },
    {0,NULL,SENSOR_SOIL,SOIL_DATA_UNKNOWN },
    {0,NULL,SENSOR_FLAME,FLAME_DATA_UNKNOWN },
    {0,NULL,SENSOR_WATERLEVEL ,WATERLEVEL_DATA_UNKNOWN},
    {0,NULL,SENSOR_WS2812,WS2812_DATA_UNKNOWN },
    {0,NULL,SENSOR_SG90,SG90_DATA_UNKNOWN },
};
#define SENSOR_DATA_MAPPINGS_RANGE_COUNT (sizeof(SensorDataMappingRange) / sizeof(sensorMapRange[0]))

void find_matching_indices_corrected(SensorDataMapping* arr, int size, uint8_t target, uint8_t* count, uint8_t** indices_ptr) {
    *count = 0;
    *indices_ptr = NULL; // 初始化为NULL
 
    for (int i = 0; i < size; i++) {
        if (arr[i].sensorType == target) {
            // 动态分配或重新分配内存
            *indices_ptr = realloc(*indices_ptr, (*count + 1) * sizeof(int));
            if (*indices_ptr == NULL) {
                fprintf(stderr, "Memory allocation failed\n");
                exit(EXIT_FAILURE);
            }
            // 存储当前下标
            (*indices_ptr)[*count] = i;
            (*count)++;
        }
    }
 
    
}
 
   

void sensorMapRange_Init()
{
    for (size_t i = 0; i < SENSOR_DATA_MAPPINGS_RANGE_COUNT; i++)
    {
        find_matching_indices_corrected(sensorDataMappings,SENSOR_DATA_MAPPINGS_COUNT,sensorMapRange[i].sensorType,&sensorMapRange[i].count,&sensorMapRange[i].range);
    }
    
}
// 函数：根据输入的传感器枚举和代表传感器属性的字符串，在数组某一范围进行检索输出枚举
SensorData findSensorData(SensorType sensorType, const char* name) {

    // for (int i = 0; i < sensorMapRange[sensorType].count; i++) {
    //     int num = sensorMapRange[sensorType].range[i];
    //     ESP_LOGI("findSensorData","%s",sensorDataMappings[num].name);
    //     if ( strcmp(sensorDataMappings[num].name, name) == 0) {
    //         return sensorDataMappings[num].sensorData;
    //     }
    // }

    for (int i = 0; i < SENSOR_DATA_MAPPINGS_COUNT; i++) {
        if (sensorDataMappings[i].sensorType == sensorType)
        {
            if ( strcmp(sensorDataMappings[i].name, name) == 0) {
            return sensorDataMappings[i].sensorData;
            }
        }
        

        
    }
    return sensorMapRange[sensorType].sensorData;
}

typedef enum{
    CMD_READ,
    CMD_WRITE,
    CMD_UNKNOW
}CmdType;
typedef struct 
{
    const char* str;
    CmdType cmdType;
}JsonCmd;

const JsonCmd cmdStrType[] =
{
    {"read",CMD_READ},
    {"write",CMD_WRITE }
};
#define CMD_COUNT 2
// 根据输入字符串返回传感器枚举值的函数
CmdType getCmdTypeFromString(const char* input) {
    for (int i = 0; i < CMD_COUNT; i++) {
        if (strcmp(input, cmdStrType[i].str) == 0) {
            return cmdStrType[i].cmdType;
        }
    }
    return CMD_UNKNOW; // 若未找到匹配的字符串，则返回未知枚举值
}

typedef struct{
    const char *id;
    const char *name;
    const char *type;
    const char *property;
    const char *value;
    int Command_value;
    SensorType sensorType;
    CmdType    CmdType;
    SensorData sensorData;
}JsonData;



// 一个通用的传感器命令处理函数，根据传感器类型调用相应的处理函数
void ReturnJson(JsonData *data);
extern bool *IsReg[];
void ProcessReceivedJson(cJSON *root)
{
    JsonData jsonData;
    //获取json中的id
    cJSON *id_item = cJSON_GetObjectItem(root, "session_id");
    if (id_item == NULL)
    {
        sendErrorJson(JSONKEYISMISS);
        ESP_LOGE(TAG, "Missing 'id' in received JSON");
        return;
    }
    // 根据 name 字段的值进行不同的处理
    jsonData.id = id_item->valuestring;
    // 获取 JSON 中的 name 字段
    cJSON *name_item = cJSON_GetObjectItem(root, "name");
    if (name_item == NULL)
    {
        sendErrorJson(JSONKEYISMISS);
        ESP_LOGE(TAG, "Missing 'name' in received JSON");
        return;
    }
    // 根据 name 字段的值进行不同的处理
    jsonData.name = name_item->valuestring;
    jsonData.sensorType = getSensorTypeFromString(jsonData.name);
    if (jsonData.sensorType == SENSOR_UNKNOWN)
    {
        //json error1 未知传感器
        sendErrorJson(NAMEISERROR);
        return;
    }
    //检索相应传感器是否注册
    if (*IsReg[jsonData.sensorType] == false)
    {
        /* code */
        ESP_LOGW("IsReg","%s",sensorMappings[jsonData.sensorType].name);
        sendErrorJson(SENSORISREG);
        return;
    }
    

    // 获取 JSON 中的 type 字段
    cJSON *type_item = cJSON_GetObjectItem(root, "type");
    if (type_item == NULL)
    {
        sendErrorJson(JSONKEYISMISS);
        ESP_LOGE(TAG, "Missing 'type' in received JSON");
        return;
    }
    jsonData.type = type_item->valuestring;
    jsonData.CmdType = getCmdTypeFromString(jsonData.type);
    if (((jsonData.sensorType < SENSOR_WS2812)&&(jsonData.CmdType != CMD_READ))\
    || ((jsonData.sensorType > SENSOR_WS2812 && jsonData.sensorType<SENSOR_UNKNOWN)&&(jsonData.CmdType != CMD_WRITE))\
    )
    {
        //json error2 未知命令/命令无效
        sendErrorJson(TYPEISERROR);
        return;
    }
    
    //如果是合规的写命令，获取value值
    if (jsonData.CmdType == CMD_WRITE)
    {
        // 获取 JSON 中的 value 字段
        cJSON *value_item = cJSON_GetObjectItem(root, "value");
        if (value_item == NULL)
        {
            sendErrorJson(JSONKEYISMISS);
            ESP_LOGE(TAG, "Missing 'value' in received JSON");
            return;
        }
        jsonData.value = value_item->valuestring;
        jsonData.Command_value = atoi(jsonData.value);

    }
    
    // 获取 JSON 中的 property 字段
    cJSON *property_item = cJSON_GetObjectItem(root, "property");
    if (property_item == NULL)
    {
        sendErrorJson(JSONKEYISMISS);
        ESP_LOGE(TAG, "Missing 'property' in received JSON");
        return;
    }
    jsonData.property = property_item->valuestring;
    jsonData.sensorData = findSensorData(jsonData.sensorType,jsonData.property);
    ESP_LOGW("jsonData.sensorData","jsonData.sensorData:%d",jsonData.sensorData);
    SensorData sensorData_error[]={
    MPU6050_DATA_UNKNOWN,
    RC522_DATA_UNKNOWN,
    DHT11_DATA_UNKNOWN,
    BODY_DATA_UNKNOWN,
    HALL_DATA_UNKNOWN,
    LIGHT_DATA_UNKNOWN,
    SOIL_DATA_UNKNOWN,
    FLAME_DATA_UNKNOWN,
    WATERLEVEL_DATA_UNKNOWN,
    WS2812_DATA_UNKNOWN,
    SG90_DATA_UNKNOWN,  // 用于处理未知属性
};
    bool found = false;  // 标记是否找到
    int size = sizeof(sensorData_error) / sizeof(sensorData_error[0]);  // 集合大小
    for (int i = 0; i < size; i++) {
        if (jsonData.sensorData == sensorData_error[i]) {
            ESP_LOGW("property","sensorData_error:%d",sensorData_error[i]);
            found = true;
            break;
        }
    }
    if (found)
    {
        //json error3 属性值无效
        sendErrorJson(PROPERTYERROR);
        return;
    }
    
    //处理函数
    switch (jsonData.sensorType)
    {
    case SENSOR_MPU6050:
        mpu6050_read();
        break;
    case SENSOR_RC522:
        rc522_read_cardid();
        break;
    case SENSOR_DHT11:
        DHT11();
        break;
    case SENSOR_BODY:
        Body_read();
        break;      
    case SENSOR_HALL:
        Hall_read();
        break;  
    case SENSOR_LIGHT:
    case SENSOR_SOIL:
    case SENSOR_FLAME:
    case SENSOR_WATERLEVEL:   
        print_adc();
        break;   

    default:
        break;
    }
    ReturnJson(&jsonData);
    // sensor_cmd_deal(&jsonData);
    



            // if (cJSON_IsString(text_item))

}
static void Updata_Demo();
extern bool Timer_flag;
void trig_Timer(const int ms);
/**
 @brief 事件处理业务
 @param 无
 @return 无
*/
bool IsTimer = false ;//是否开启定时任务
bool Istrig = true ;//是否开启触发任务
static void Demo_task(void *arg)
{

    uart_init();
    uint8_t InitTime = 1;
    vTaskDelay(3000*InitTime/portTICK_PERIOD_MS);
    ESP_LOGI("Demo","Init finished");
    
    if(IsTimer)Updata_Demo();//开启定时上传
    sendData("{\"Init\":1}\r\n");
    while (1)
    {
        receiveDataCjson();
        vTaskDelay(10/portTICK_PERIOD_MS);
        //定时上传具体任务
        if (Timer_flag && IsTimer)
        {
            //发送根据协议主动上报温度
            DHT11();
            JsonData DHT11_hum ={
                .id = 0,
                .sensorType = SENSOR_DHT11,
                .name = "DHT11",
                .property = "hum",
                .sensorData = DHT11_DATA_HUM
            };
            ReturnJson(&DHT11_hum);
            Timer_flag = false;
        }
        //触发任务
        if (Istrig)
        {
        rc522_read_cardid();
            if (rc522.value == ACTIVE)
            {
                ESP_LOGI(TAG, "card: %02x%02x%02x%02x", rc522.card[0], rc522.card[1], rc522.card[2], rc522.card[3]);
                JsonData RC522_cardid ={
                .id = 0,
                .sensorType = SENSOR_RC522,
                .name = "RC522",
                .property = "card_id",
                .sensorData = RC522_DATA_CARD_ID
            };
            ReturnJson(&RC522_cardid);
            Istrig = false;//防止短时间内重复触发
            trig_Timer(3000);//定时开启触发任务
            }else
            {

            }
        }
    }
    
    vTaskDelay(1000/portTICK_PERIOD_MS);
    vTaskDelete(NULL);
}

//定时上报任务demo
TimerHandle_t xTimerHandle = NULL;
bool Timer_flag =false;
void TimerCallback(TimerHandle_t xTimer) {
    // 获取传递的参数（示例）
    void *userParam = pvTimerGetTimerID(xTimer);
    if (userParam != NULL) {
        *(bool *)userParam = true;
    }

    
}
static void Updata_Demo()
{
    xTimerHandle = xTimerCreate(
        "MyTimer", 
        (2000/portTICK_PERIOD_MS), 
        pdTRUE, 
        (void*)&Timer_flag, // 传递参数
        TimerCallback
    );

    if (xTimerHandle == NULL) {
        printf("Timer creation failed!\n");
        return;
    }

    // 启动定时器
    if (xTimerStart(xTimerHandle, 0) != pdPASS) {
        printf("Failed to start timer!\n");
        return;
    }
}
TimerHandle_t trigTimerHandle = NULL;
void trig_Timer(const int ms)
{
        xTimerHandle = xTimerCreate(
        "trigTimer", 
        (ms/portTICK_PERIOD_MS), 
        pdTRUE, 
        (void*)&Istrig, // 传递参数
        TimerCallback
    );

    if (xTimerHandle == NULL) {
        printf("Timer creation failed!\n");
        return;
    }

    // 启动定时器
    if (xTimerStart(xTimerHandle, 0) != pdPASS) {
        printf("Failed to start timer!\n");
        return;
    }
}
void Create_Demo_Task()
{
    xTaskCreate(Demo_task, "Demo_task", 2048*4, NULL, 5, NULL);
}
bool *IsReg[] = {
    &MPU6050.isReg,
    &rc522.isReg,
    &dht11.isReg,
    &Body.isReg,
    &Hall.isReg,
    &ADC.Light.isReg,
    &ADC.Soil.isReg,
    &ADC.WaterLevel.isReg,
    &ADC.Flame.isReg,
    &WS2812.isReg,
    &sg90.isReg,
};
typedef enum{
    Sensor,//传感器
    Command//执行器
}JsonType;

// 定义一个足够大的缓冲区来存储转换后的字符串
#define BUFFER_SIZE 64
 
void int_to_string(int number,char *str) {
    
    if (str ==NULL )
    {
        ESP_LOGE("INT_TO_STRING","str == NULL");
        return;
    }
    memset(str, 0, BUFFER_SIZE * sizeof(char));


    // 使用sprintf将整数转换为字符串
    sprintf(str, "%d", number);
 
    // 打印转换后的字符串
    ESP_LOGI("INT_TO_STRING", "Converted integer: %s", str);
}

void float_to_string(float number,char *str) {
   
    if (str ==NULL )
    {
        ESP_LOGE("FLOAT_TO_STRING","str == NULL");
        return;
    }
    memset(str, 0, BUFFER_SIZE * sizeof(char));
    // 使用sprintf将浮点数转换为字符串，并限制小数点后的位数
    sprintf(str, "%.2f", number);
 
    // 打印转换后的字符串
    ESP_LOGI("FLOAT_TO_STRING", "Converted float: %s", str);
}
void cardid_to_string(char* cardid,char*str)
{
    if (str ==NULL )
    {
        ESP_LOGE("CARDID_TO_STRING","str == NULL");
        return;
    }
    memset(str, 0, BUFFER_SIZE * sizeof(char));
    sprintf(str, "%02x%02x%02x%02x", cardid[0],cardid[1],cardid[2],cardid[3]);
    // 打印转换后的字符串
    ESP_LOGI("CARDID_TO_STRING", "CARDID: %s", str);
}

void int_to_RGB(uint32_t number,led_color_t* color)
{
    if (color ==NULL )
    {
        ESP_LOGE("INT_TO_RGB","color == NULL");
        return;
    }
    if (number > 0xFFFFFF)
    {
        number = 0xFFFFFF;
    }
    // 提取或计算RGB分量，确保它们不超过0xFF
    color->red   = (uint8_t)((number >> 16) & 0xFF);      // 取高8位作为红色分量
    color->green = (uint8_t)((number >> 8)  & 0xFF);      // 取中间8位作为绿色分量
    color->blue  = (uint8_t)(number & 0xFF);              // 取低8位作为蓝色分量

}
void ReturnJson(JsonData *data)
{
    JsonType jsonType;
    //检查传感器是否注册
    // if (*IsReg[data->sensorType] == false)
    // {
    //     //错误json——未注册

    //     return;
    // }
    //判断是传感器还是执行器
    if (data->sensorType < SENSOR_WS2812)
    {
        //判断是传感器
        jsonType = Sensor;
    }else
    {
        //判断是执行器
        jsonType = Command;   
    }
        // 创建根对象
    cJSON *root = cJSON_CreateObject();
 
    // 添加键值对到根对象
    cJSON_AddStringToObject(root, "session_id", data->id);
    cJSON_AddStringToObject(root, "name", data->name);
    if (jsonType == Sensor)cJSON_AddStringToObject(root, "type", "sensor");
    else if (jsonType == Command)cJSON_AddStringToObject(root, "type", "command");
    cJSON_AddStringToObject(root, "property", data->property);
    char buffer[BUFFER_SIZE];
    if (jsonType == Sensor){
        
        switch (data->sensorData)
        {
            case MPU6050_DATA_ACCE_X:
                printf("MPU6050传感器数据类型名称是：acce_x\n");
                // 这里可以添加处理MPU6050加速度X轴数据的代码
                float_to_string(MPU6050.acce.acce_x,buffer);
                
                break;
            case MPU6050_DATA_ACCE_Y:
                printf("MPU6050传感器数据类型名称是：acce_y\n");
                // 处理Y轴数据
                float_to_string(MPU6050.acce.acce_y,buffer);
                
                break;
            case MPU6050_DATA_ACCE_Z:
                printf("MPU6050传感器数据类型名称是：acce_z\n");
                // 处理Z轴数据
                float_to_string(MPU6050.acce.acce_z,buffer);
                
                break;
            case MPU6050_DATA_GYRO_X:
                printf("MPU6050传感器数据类型名称是：gyro_x\n");
                // 处理陀螺仪X轴数据
                float_to_string(MPU6050.gyro.gyro_x,buffer);
                
                break;
            case MPU6050_DATA_GYRO_Y:
                printf("MPU6050传感器数据类型名称是：gyro_y\n");
                // 处理Y轴数据
                float_to_string(MPU6050.gyro.gyro_y,buffer);
                
                break;
            case MPU6050_DATA_GYRO_Z:
                printf("MPU6050传感器数据类型名称是：gyro_z\n");
                // 处理Z轴数据
                float_to_string(MPU6050.gyro.gyro_z,buffer);
                
                break;
            case MPU6050_DATA_TEMP:
                printf("MPU6050传感器数据类型名称是：temp\n");
                // 处理温度数据
                float_to_string(MPU6050.temp.temp,buffer);
                
                break;
            case MPU6050_DATA_CUSTOM:
                printf("MPU6050传感器数据类型名称是：custom\n");
                // 处理自定义数据
                //使用方法：CustomValue数据类型为void*，在程序中赋值时给的什么指针类型，调用时就可以直接强转，再加*
                //下面是float*的例子
                if (MPU6050.CustomValue == NULL)
                {
                     sprintf(buffer, "NULL");
                }else float_to_string(*(float*)MPU6050.CustomValue,buffer);
                
                
                
                break;
            case MPU6050_DATA_UNKNOWN:
                printf("MPU6050传感器数据类型未知\n");
                break;
    
            case RC522_DATA_CARD_ID:
                printf("RC522传感器数据类型名称是：card_id\n");
                 // 处理卡片ID数据
                cardid_to_string((char *)rc522.card,buffer);
                

                break;
            case RC522_DATA_CARD_STATE:
                printf("RC522传感器数据类型名称是：card_state\n");
                // 处理卡片状态数据
                int_to_string(rc522.value,buffer);
                
                break;
            case RC522_DATA_CUSTOM:
                printf("RC522传感器数据类型名称是：custom\n");
                // 处理自定义数据
                //使用方法：CustomValue数据类型为void*，在程序中赋值时给的什么指针类型，调用时就可以直接强转，再加*
                //int*的例子
                if (rc522.CustomValue == NULL)
                {
                     sprintf(buffer, "NULL");
                }else int_to_string(*(int*)rc522.CustomValue,buffer);
                
                break;
            case RC522_DATA_UNKNOWN:
                printf("RC522传感器数据类型未知\n");
                break;
    
            case DHT11_DATA_TEMP:
                printf("DHT11传感器数据类型名称是：temp:%.2f\n",dht11.Value.Temp);
                float_to_string(dht11.Value.Temp,buffer);
                
                break;
            case DHT11_DATA_HUM:
                printf("DHT11传感器数据类型名称是：humidity\n");
                float_to_string(dht11.Value.Hum,buffer);
                
                break;
            case DHT11_DATA_UNKNOWN:
                printf("DHT11传感器数据类型未知\n");
                break;
    
            case BODY_DATA_RAW:
                printf("BODY传感器数据类型名称是：raw\n");
                int_to_string(Body.rawVal,buffer);
                
                break;
            case BODY_DATA_VALUE:
                printf("BODY传感器数据类型名称是：value\n");
                int_to_string(Body.Value,buffer);
                
                break;
            case BODY_DATA_UNKNOWN:
                printf("BODY传感器数据类型未知\n");
                break;
    
            case HALL_DATA_RAW:
                printf("HALL传感器数据类型名称是：raw\n");
                int_to_string(Hall.rawVal,buffer);
                
                break;
            case HALL_DATA_VALUE:
                printf("HALL传感器数据类型名称是：value\n");
                int_to_string(Hall.Value,buffer);
                
                break;
            case HALL_DATA_UNKNOWN:
                printf("HALL传感器数据类型未知\n");
                break;
    
            case LIGHT_DATA_RAW:
                printf("LIGHT传感器数据类型名称是：raw\n");
                int_to_string(ADC.Light.rawVal,buffer);
                
                break;
            case LIGHT_DATA_CUSTOM:
                printf("LIGHT传感器数据类型名称是：custom\n");
                int_to_string(ADC.Light.Value,buffer);
                
                break;
            case LIGHT_DATA_UNKNOWN:
                printf("LIGHT传感器数据类型未知\n");
                break;
    
            case SOIL_DATA_RAW:
                printf("SOIL传感器数据类型名称是：raw\n");
                int_to_string(ADC.Soil.rawVal,buffer);
                break;
            case SOIL_DATA_CUSTOM:
                printf("SOIL传感器数据类型名称是：custom\n");
                int_to_string(ADC.Soil.Value,buffer);
                break;
            case SOIL_DATA_UNKNOWN:
                printf("SOIL传感器数据类型未知\n");
                
                break;
    
            case FLAME_DATA_RAW:
                printf("FLAME传感器数据类型名称是：raw\n");
                int_to_string(ADC.Flame.rawVal,buffer);
                break;
            case FLAME_DATA_CUSTOM:
                printf("FLAME传感器数据类型名称是：custom\n");
                int_to_string(ADC.Flame.Value,buffer);
                break;
            case FLAME_DATA_UNKNOWN:
                printf("FLAME传感器数据类型未知\n");
                break;
    
            case WATERLEVEL_DATA_RAW:
                printf("WATERLEVEL传感器数据类型名称是：raw\n");
                int_to_string(ADC.WaterLevel.rawVal,buffer);
                break;
            case WATERLEVEL_DATA_CUSTOM:
                printf("WATERLEVEL传感器数据类型名称是：custom\n");
                int_to_string(ADC.WaterLevel.Value,buffer);
                break;
            case WATERLEVEL_DATA_UNKNOWN:
                printf("WATERLEVEL传感器数据类型未知\n");
                break;

            // case WS2812_DATA_RGB:
            //     printf("WS2812传感器数据类型名称是：rgb\n");
            //     int_to_string(WS2812.RGB,buffer);
            //     break;
            // case WS2812_DATA_CUSTOM:
            //     printf("WS2812传感器数据类型名称是：custom\n");
            //     int_to_string(WS2812.custmMode,buffer);
            //     break;
            // case WS2812_DATA_UNKNOWN:
            //     printf("WS2812传感器数据类型未知\n");
            //     break;
    
            // case SG90_DATA_ANGLE:
            //     printf("SG90传感器数据类型名称是：angle\n");
            //     int_to_string(sg90.angle,buffer);
            //     // 处理角度数据
            //     break;
            // case SG90_DATA_CUSTOM:
            //     printf("SG90传感器数据类型名称是：custom\n");
            //     int_to_string(sg90.custmMode,buffer);
            //     // 处理自定义数据
            //     break;
            // case SG90_DATA_UNKNOWN:
            //     printf("SG90传感器数据类型未知\n");
            //     break;
            default:
                printf("未知的传感器数据类型\n");
                break;
        }
        cJSON_AddStringToObject(root, "value", buffer);
    }else if (jsonType == Command){

        switch (data->sensorData)
        {
            case WS2812_DATA_RGB:
                printf("WS2812传感器数据类型名称是：rgb\n");
                if (WS2812.custmMode == COLOR_MODE)
                {
                    led_color_t color;
                    if (data->Command_value > 0xFFFFFF)
                    {
                        data->Command_value = 0xFFFFFF;
                    }
                    
                    int_to_RGB(data->Command_value,&color);
                    ws2812_led_set_on(color);
                    WS2812.RGB = data->Command_value;
                    int_to_string(WS2812.RGB,buffer);
                }

                

                break;
            case WS2812_DATA_CUSTOM:
                printf("WS2812传感器数据类型名称是：custom\n");
                WS2812.custmMode = data->Command_value;
                int_to_string(WS2812.custmMode,buffer);
                break;
            case WS2812_DATA_UNKNOWN:
                printf("WS2812传感器数据类型未知\n");
                break;
    
            case SG90_DATA_ANGLE:
                printf("SG90传感器数据类型名称是：angle\n");
                sg90.angle = data->Command_value;
                if (sg90.angle >180 )
                {
                    sg90.angle = 180;
                }
                sg90_SetAngle(sg90.angle);
                int_to_string(sg90.angle,buffer);
                // 处理角度数据
                break;
            case SG90_DATA_CUSTOM:
                printf("SG90传感器数据类型名称是：custom\n");
                // 处理自定义数据
                sg90.custmMode = data->Command_value;
                int_to_string(sg90.custmMode,buffer);
                break;
            case SG90_DATA_UNKNOWN:
                printf("SG90传感器数据类型未知\n");
                break;
    
            default:
                printf("未知的传感器数据类型\n");
                break;
        }
        cJSON_AddStringToObject(root, "value", buffer);
    }
    
 
    //发送json数据
    sendJsonData(root);

 
    // 释放内存
    cJSON_Delete(root);

    
    
    
}

int BuildJson() {
    // 创建一个空的 JSON 对象
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        printf("Failed to create JSON object\n");
        return -1;
    }
    /************************************************************ */
    if (MPU6050.isReg)
    {
        // 创建并添加json对象
        cJSON *MPU6050_obj = cJSON_CreateObject();
        if (MPU6050_obj == NULL) {
            printf("Failed to create MPU6050 object\n");
            cJSON_Delete(root);
            return -1;
        }
        cJSON_AddItemToObject(MPU6050_obj, "acce_x", cJSON_CreateNumber(MPU6050.acce.acce_x));
        cJSON_AddItemToObject(MPU6050_obj, "acce_y", cJSON_CreateNumber(MPU6050.acce.acce_y));
        cJSON_AddItemToObject(MPU6050_obj, "acce_z", cJSON_CreateNumber(MPU6050.acce.acce_z));
        cJSON_AddItemToObject(MPU6050_obj, "gyro_x", cJSON_CreateNumber(MPU6050.gyro.gyro_x));
        cJSON_AddItemToObject(MPU6050_obj, "gyro_y", cJSON_CreateNumber(MPU6050.gyro.gyro_y));
        cJSON_AddItemToObject(MPU6050_obj, "gyro_z", cJSON_CreateNumber(MPU6050.gyro.gyro_z));
        cJSON_AddItemToObject(MPU6050_obj, "temp", cJSON_CreateNumber(MPU6050.temp.temp));
        cJSON_AddItemToObject(root, "MPU6050", MPU6050_obj);
    }
    /************************************************************ */
    if (rc522.isReg)
    {
        // 创建并添加json对象
        cJSON *rc522_obj = cJSON_CreateObject();
        if (rc522_obj == NULL) {
            printf("Failed to create rc522 object\n");
            cJSON_Delete(root);
            return -1;
        }
        cJSON *card_id = cJSON_CreateArray();
    if (card_id == NULL) {
        fprintf(stderr, "Failed to create card_id array\n");
        cJSON_Delete(rc522_obj);
        cJSON_Delete(root);
        return -1;
    }
    cJSON_AddItemToArray(card_id, cJSON_CreateNumber(rc522.card[0]));
    cJSON_AddItemToArray(card_id, cJSON_CreateNumber(rc522.card[1]));
    cJSON_AddItemToArray(card_id, cJSON_CreateNumber(rc522.card[2]));
    cJSON_AddItemToArray(card_id, cJSON_CreateNumber(rc522.card[3]));
    cJSON_AddItemToObject(rc522_obj, "card_id", card_id);
    cJSON_AddItemToObject(rc522_obj, "value", cJSON_CreateNumber(rc522.value));
    // cJSON_AddItemToObject(rc522_obj, "CustomValue", cJSON_CreateNumber(rc522.CustomValue));
    cJSON_AddItemToObject(root, "rc522", rc522_obj);
    }
    /************************************************************ */
    if (dht11.isReg)
    {
        // 创建并添加json对象
        cJSON *dht11_obj = cJSON_CreateObject();
        if (dht11_obj == NULL) {
            printf("Failed to create dht11 object\n");
            cJSON_Delete(root);
            return -1;
        }
        cJSON *arrayraw = cJSON_CreateArray();
    if (arrayraw == NULL) {
        fprintf(stderr, "Failed to create arrayraw array\n");
        cJSON_Delete(dht11_obj);
        cJSON_Delete(root);
        return -1;
    }
    cJSON_AddItemToArray(arrayraw, cJSON_CreateNumber(dht11.rawVal[0]));
    cJSON_AddItemToArray(arrayraw, cJSON_CreateNumber(dht11.rawVal[1]));
    cJSON_AddItemToArray(arrayraw, cJSON_CreateNumber(dht11.rawVal[2]));
    cJSON_AddItemToArray(arrayraw, cJSON_CreateNumber(dht11.rawVal[3]));
    cJSON_AddItemToObject(dht11_obj, "raw", arrayraw);
    cJSON_AddItemToObject(dht11_obj, "Temp", cJSON_CreateNumber(dht11.Value.Temp));
    cJSON_AddItemToObject(dht11_obj, "Hum", cJSON_CreateNumber(dht11.Value.Hum));
    cJSON_AddItemToObject(root, "DHT11", dht11_obj);
    }
    /************************************************************ */
    if (Body.isReg)
    {
        // 创建并添加json对象
        cJSON *Body_obj = cJSON_CreateObject();
        if (Body_obj == NULL) {
            printf("Failed to create Body object\n");
            cJSON_Delete(root);
            return -1;
        }

    
    cJSON_AddItemToObject(Body_obj, "rawVal", cJSON_CreateNumber(Body.rawVal));
    cJSON_AddItemToObject(Body_obj, "Value", cJSON_CreateNumber(Body.Value));
    cJSON_AddItemToObject(root, "Body", Body_obj);
    }
    /************************************************************ */
    if (Hall.isReg)
    {
        // 创建并添加json对象
        cJSON *Hall_obj = cJSON_CreateObject();
        if (Hall_obj == NULL) {
            printf("Failed to create Hall object\n");
            cJSON_Delete(root);
            return -1;
        }

    
    cJSON_AddItemToObject(Hall_obj, "rawVal", cJSON_CreateNumber(Hall.rawVal));
    cJSON_AddItemToObject(Hall_obj, "Value", cJSON_CreateNumber(Hall.Value));
    cJSON_AddItemToObject(root, "Hall", Hall_obj);
    }
    /************************************************************ */
    if (ADC.Flame.isReg)
    {
        // 创建并添加json对象
        cJSON *Flame_obj = cJSON_CreateObject();
        if (Flame_obj == NULL) {
            printf("Failed to create Flame object\n");
            cJSON_Delete(root);
            return -1;
        }

    
    cJSON_AddItemToObject(Flame_obj, "rawVal", cJSON_CreateNumber(ADC.Flame.rawVal));
    cJSON_AddItemToObject(Flame_obj, "Value", cJSON_CreateNumber(ADC.Flame.Value));
    cJSON_AddItemToObject(root, "Flame", Flame_obj);
    }
    /************************************************************ */
    if (ADC.Light.isReg)
    {
        // 创建并添加json对象
        cJSON *Light_obj = cJSON_CreateObject();
        if (Light_obj == NULL) {
            printf("Failed to create Light object\n");
            cJSON_Delete(root);
            return -1;
        }

    
    cJSON_AddItemToObject(Light_obj, "rawVal", cJSON_CreateNumber(ADC.Light.rawVal));
    cJSON_AddItemToObject(Light_obj, "Value", cJSON_CreateNumber(ADC.Light.Value));
    cJSON_AddItemToObject(root, "Light", Light_obj);
    }
    /************************************************************ */
    if (ADC.Soil.isReg)
    {
        // 创建并添加json对象
        cJSON *Soil_obj = cJSON_CreateObject();
        if (Soil_obj == NULL) {
            printf("Failed to create Soil object\n");
            cJSON_Delete(root);
            return -1;
        }

    
    cJSON_AddItemToObject(Soil_obj, "rawVal", cJSON_CreateNumber(ADC.Soil.rawVal));
    cJSON_AddItemToObject(Soil_obj, "Value", cJSON_CreateNumber(ADC.Soil.Value));
    cJSON_AddItemToObject(root, "Flame", Soil_obj);
    }
    /************************************************************ */
    if (ADC.WaterLevel.isReg)
    {
        // 创建并添加json对象
        cJSON *WaterLevel_obj = cJSON_CreateObject();
        if (WaterLevel_obj == NULL) {
            printf("Failed to create WaterLevel object\n");
            cJSON_Delete(root);
            return -1;
        }

    
    cJSON_AddItemToObject(WaterLevel_obj, "rawVal", cJSON_CreateNumber(ADC.WaterLevel.rawVal));
    cJSON_AddItemToObject(WaterLevel_obj, "Value", cJSON_CreateNumber(ADC.WaterLevel.Value));
    cJSON_AddItemToObject(root, "WaterLevel", WaterLevel_obj);
    }
    /************************************************************ */
    if (sg90.isReg)
    {
        // 创建并添加json对象
        cJSON *sg90_obj = cJSON_CreateObject();
        if (sg90_obj == NULL) {
            printf("Failed to create sg90 object\n");
            cJSON_Delete(root);
            return -1;
        }

    
    cJSON_AddItemToObject(sg90_obj, "angle", cJSON_CreateNumber(sg90.angle));
    cJSON_AddItemToObject(sg90_obj, "custmMode", cJSON_CreateNumber(sg90.custmMode));
    cJSON_AddItemToObject(root, "sg90", sg90_obj);
    }
    /************************************************************ */
    if (WS2812.isReg)
    {
        // 创建并添加json对象
        cJSON *WS2812_obj = cJSON_CreateObject();
        if (WS2812_obj == NULL) {
            printf("Failed to create WS2812 object\n");
            cJSON_Delete(root);
            return -1;
        }

    
    cJSON_AddItemToObject(WS2812_obj, "RGB", cJSON_CreateNumber(WS2812.RGB));
    cJSON_AddItemToObject(WS2812_obj, "custmMode", cJSON_CreateNumber(WS2812.custmMode));
    cJSON_AddItemToObject(root, "WS2812", WS2812_obj);
    }
    // 将 JSON 对象转换为字符串并打印
    char *json_string = cJSON_Print(root);
    if (json_string == NULL) {
        printf("Failed to print JSON string\n");
        cJSON_Delete(root);
        return -1;
    }
    printf("%s\n", json_string);

    // 释放内存
    cJSON_Delete(root);
    free(json_string);

    return 0;
}
