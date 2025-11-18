#ifndef __ENCODER_H
#define __ENCODER_H

#include "stm32f10x.h"

/* ==========================================================
 * 编码器模块接口说明
 * 
 * - Encoder_Init()               初始化 TIM3/TIM4 编码器接口
 * - Encoder_Get_Speed(num)       获取当前速度（单位：脉冲/10ms）
 * - Encoder_Get_Position(num)    获取累计位置脉冲
 * - Encoder_Clear_TotalCount(num)清零累计位置
 * ========================================================== */

void Encoder_Init(void);
int16_t Encoder_Get_Speed(uint8_t num);
int32_t Encoder_Get_Position(uint8_t num);
void Encoder_Clear_TotalCount(uint8_t num);

#endif
