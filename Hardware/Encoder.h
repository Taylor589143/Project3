#ifndef __ENCODER_H
#define __ENCODER_H

#include "stm32f10x.h"


#define ENCODER_M1   1U   // 左电机
#define ENCODER_M2   2U   // 右电机


/* 左编码器 A/B 相 */
#define LEFT_ENCODER_A_PIN     GPIO_Pin_6
#define LEFT_ENCODER_A_PORT    GPIOA
#define LEFT_ENCODER_B_PIN     GPIO_Pin_7
#define LEFT_ENCODER_B_PORT    GPIOA

/* 右编码器 A/B 相 */
#define RIGHT_ENCODER_A_PIN    GPIO_Pin_8    // A 相
#define RIGHT_ENCODER_A_PORT   GPIOA
#define RIGHT_ENCODER_B_PIN    GPIO_Pin_9    // B 相（从 PA11 改到 PA9）
#define RIGHT_ENCODER_B_PORT   GPIOA

typedef struct
{
    int32_t count;   // 计数器当前值（或累计值）
    int16_t speed;   // 最近一次计算出来的速度
} Encoder_TypeDef;


extern Encoder_TypeDef left_encoder;
extern Encoder_TypeDef right_encoder;



void    Encoder_Init(void);
int16_t Encoder_Get_Speed(uint8_t num);
int32_t Encoder_Get_Position(uint8_t num);
void    Encoder_Clear_TotalCount(uint8_t num);

int32_t Encoder_Get_Left_Count(void);
int32_t Encoder_Get_Right_Count(void);
void    Encoder_Reset_Both(void);

#endif
