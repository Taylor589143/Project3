#include "stm32f10x.h"
#include "Sensor.h"
#include "Delay.h"
#include "Serial.h"  // 用于调试输出

// 传感器状态全局变量
int L2 = 0, L1 = 0, M = 0, R1 = 0, R2 = 0;

/**
  * @brief  传感器初始化
  */
void Sensor_Init(void)
{
    // 开启GPIOA时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    
    GPIO_InitTypeDef GPIO_InitStructure;
    
    // 配置传感器引脚为输入模式
    GPIO_InitStructure.GPIO_Pin = L2_PIN | L1_PIN | M_PIN | R1_PIN | R2_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; // 上拉输入
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    printf("Infrared Sensors Initialized (PA0,PA1,PA4,PA5,PA8)\r\n");
}

/**
  * @brief  读取传感器状态
  * 说明：传感器遇到白色时亮灯输出低电平(0)，遇到黑色时灭灯输出高电平(1)
  *       所以：0=检测到白色，1=检测到黑色
  */
void Sensor_Read(void)
{
    // 读取传感器状态（0:检测到白色，1:检测到黑色）
    L2 = GPIO_ReadInputDataBit(L2_PORT, L2_PIN);  // PA0
    L1 = GPIO_ReadInputDataBit(L1_PORT, L1_PIN);  // PA1
    M  = GPIO_ReadInputDataBit(M_PORT, M_PIN);    // PA4
    R1 = GPIO_ReadInputDataBit(R1_PORT, R1_PIN);  // PA5
    R2 = GPIO_ReadInputDataBit(R2_PORT, R2_PIN);  // PA8
}

/**
  * @brief  获取循线状态
  * @return 循线状态码 (1-11)
  * 逻辑：传感器在黑色线上输出1，在白色背景上输出0
  * 传感器排列：L2(最左), L1(左), M(中), R1(右), R2(最右)
  */
uint8_t Get_Line_Status(void)
{
    // 先读取最新传感器数据
    Sensor_Read();
    
    // 传感器状态：1=在黑线上，0=在白色背景上
    // 从左到右：L2, L1, M, R1, R2
    
    // 1. 首先检查特殊状态
    if(L2 == 1 && L1 == 1 && M == 1 && R1 == 1 && R2 == 1) 
        return 11;     // 11111 - 全黑（十字路口或停车线）
    
    if(L2 == 0 && L1 == 0 && M == 0 && R1 == 0 && R2 == 0) 
        return 10;     // 00000 - 丢失路线（全白）
    
    // 2. 检查十字路口（中间3个传感器同时检测到黑线）
    if(Detect_Crossroad())
        return 12;     // 新增：明确标识十字路口
    
    // 3. 正常循迹状态
    if(L2 == 1 && L1 == 0 && M == 0 && R1 == 0 && R2 == 0) 
        return 1;      // 10000 - 严重偏左
    else if(L2 == 1 && L1 == 1 && M == 0 && R1 == 0 && R2 == 0) 
        return 2;      // 11000 - 偏左
    else if(L2 == 0 && L1 == 1 && M == 0 && R1 == 0 && R2 == 0) 
        return 3;      // 01000 - 轻微偏左
    else if(L2 == 0 && L1 == 1 && M == 1 && R1 == 0 && R2 == 0) 
        return 4;      // 01100 - 正常偏左
    else if(L2 == 0 && L1 == 0 && M == 1 && R1 == 0 && R2 == 0) 
        return 5;      // 00100 - 居中
    else if(L2 == 0 && L1 == 0 && M == 1 && R1 == 1 && R2 == 0) 
        return 6;      // 00110 - 正常偏右
    else if(L2 == 0 && L1 == 0 && M == 0 && R1 == 1 && R2 == 0) 
        return 7;      // 00010 - 轻微偏右
    else if(L2 == 0 && L1 == 0 && M == 0 && R1 == 1 && R2 == 1) 
        return 8;      // 00011 - 偏右
    else if(L2 == 0 && L1 == 0 && M == 0 && R1 == 0 && R2 == 1) 
        return 9;      // 00001 - 严重偏右
    else 
        return 0;      // 未知状态或其他组合
}

/**
  * @brief  检测十字路口（针对18mm窄线优化）
  * @return 1-十字路口, 0-非十字路口
  * 说明：对于18mm窄赛道，当多个传感器同时检测到黑线时判定为十字路口
  */
uint8_t Detect_Crossroad(void)
{
    uint8_t sensor_count = 0;
    
    // 计算检测到黑线的传感器数量
    if(L2 == 1) sensor_count++;
    if(L1 == 1) sensor_count++;
    if(M == 1) sensor_count++;
    if(R1 == 1) sensor_count++;
    if(R2 == 1) sensor_count++;
    
    // 对于18mm窄线，3个或以上传感器同时检测到黑线即为十字路口
    if(sensor_count >= 3) {
        return 1;
    }
    
    // 特殊十字路口模式检测
    if((L1 == 1 && M == 1 && R1 == 1) ||  // 01110 中间三个
       (L2 == 1 && L1 == 1 && M == 1) ||  // 11100 左三个
       (M == 1 && R1 == 1 && R2 == 1)) {  // 00111 右三个
        return 1;
    }

    return 0;
}

/**
  * @brief  传感器调试输出
  * 用于串口调试，显示传感器状态
  */
void Sensor_Debug_Output(void)
{
    Sensor_Read();
    
    printf("Sensors: %d%d%d%d%d (L2,L1,M,R1,R2)\r\n", 
           L2, L1, M, R1, R2);
    
    uint8_t status = Get_Line_Status();
    switch(status) {
        case 1: printf("Status: Severe Left\r\n"); break;
        case 2: printf("Status: Left\r\n"); break;
        case 3: printf("Status: Slight Left\r\n"); break;
        case 4: printf("Status: Normal Left\r\n"); break;
        case 5: printf("Status: Center\r\n"); break;
        case 6: printf("Status: Normal Right\r\n"); break;
        case 7: printf("Status: Slight Right\r\n"); break;
        case 8: printf("Status: Right\r\n"); break;
        case 9: printf("Status: Severe Right\r\n"); break;
        case 10: printf("Status: Line Lost\r\n"); break;
        case 11: printf("Status: Stop Line\r\n"); break;
        case 12: printf("Status: Crossroad\r\n"); break;
        default: printf("Status: Unknown\r\n"); break;
    }
}

/**
  * @brief  获取传感器原始数据
  * @return 5位原始数据 (bit4: L2, bit3: L1, bit2: M, bit1: R1, bit0: R2)
  */
uint8_t Sensor_Get_Raw_Data(void)
{
    Sensor_Read();
    return (L2 << 4) | (L1 << 3) | (M << 2) | (R1 << 1) | R2;
}

/**
  * @brief  检查是否在线上
  * @return 1-至少一个传感器检测到线, 0-完全丢线
  */
uint8_t Sensor_Is_On_Line(void)
{
    Sensor_Read();
    return (L2 == 1 || L1 == 1 || M == 1 || R1 == 1 || R2 == 1);
}





