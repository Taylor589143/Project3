#ifndef __MOTOR_H
#define __MOTOR_H

#include "stm32f10x.h"


// 模块名称：Motor（电机驱动模块）
// 功能说明：提供电机PWM初始化和速度控制接口

// 初始化PWM及电机相关GPIO
void PWM_Init(void);

// 设置电机速度
// 参数 motor_num : 电机编号，1-电机1，2-电机2
// 参数 speed     : 速度值，正数正转，负数反转，0停止
void Motor_Set_Speed(uint8_t motor_num, int16_t speed);

// 电机处理函数（可扩展为闭环控制等）
void Motor_Process(void);

// 外部可访问的电机位置相关变量
extern int32_t last_position1;  // 电机1上次位置
extern int32_t target_position2; // 电机2目标位置

#endif
