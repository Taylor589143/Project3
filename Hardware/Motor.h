#ifndef __MOTOR_H
#define __MOTOR_H

#include "stm32f10x.h"

#define LEFT_MOTOR_AIN1_PIN      GPIO_Pin_12
#define LEFT_MOTOR_AIN1_PORT     GPIOB

#define LEFT_MOTOR_AIN2_PIN      GPIO_Pin_13
#define LEFT_MOTOR_AIN2_PORT     GPIOB

#define LEFT_MOTOR_PWM_PIN       GPIO_Pin_2
#define LEFT_MOTOR_PWM_PORT      GPIOA


#define RIGHT_MOTOR_BIN1_PIN     GPIO_Pin_14
#define RIGHT_MOTOR_BIN1_PORT    GPIOB

#define RIGHT_MOTOR_BIN2_PIN     GPIO_Pin_15
#define RIGHT_MOTOR_BIN2_PORT    GPIOB

#define RIGHT_MOTOR_PWM_PIN      GPIO_Pin_3
#define RIGHT_MOTOR_PWM_PORT     GPIOA


void PWM_Init(void);
#define Motor_Init   PWM_Init   // 兼容别人的 Motor_Init 写法

void Motor_Set_Speed(uint8_t motor_num, int16_t speed);

void motor(int16_t left_speed, int16_t right_speed);
void Motor_Stop(void);
void Motor_Process(void);

extern int32_t last_position1;

/* 电机2 目标位置（位置模式时使用） */
extern int32_t target_position2;

#endif 
