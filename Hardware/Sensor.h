#ifndef __SENSOR_H
#define __SENSOR_H

#include "stm32f10x.h"

// 传感器引脚定义
#define L2_PIN    GPIO_Pin_0   // PA0 - 最左侧传感器
#define L1_PIN    GPIO_Pin_1   // PA1 - 左侧传感器
#define M_PIN     GPIO_Pin_4   // PA4 - 中间传感器
#define R1_PIN    GPIO_Pin_5   // PA5 - 右侧传感器
#define R2_PIN    GPIO_Pin_8   // PA8 - 最右侧传感器

#define L2_PORT   GPIOA
#define L1_PORT   GPIOA
#define M_PORT    GPIOA
#define R1_PORT   GPIOA
#define R2_PORT   GPIOA


#define SENSOR_NUM 5

// 传感器状态结构体
typedef struct {
    uint8_t L2;     // 最左侧传感器
    uint8_t L1;     // 左侧传感器  
    uint8_t M;      // 中间传感器
    uint8_t R1;     // 右侧传感器
    uint8_t R2;     // 最右侧传感器
    uint8_t raw;    // 原始5位数据
} Sensor_State;

// 函数声明
void Sensor_Init(void);
void Sensor_Read(void);

unsigned char Get_Line_Status(void);
unsigned char Detect_Crossroad(void);

void Sensor_Debug_Output(void);

// 全局变量声明
extern int L2, L1, M, R1, R2;

#endif