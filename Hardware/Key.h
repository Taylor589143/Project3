#ifndef __KEY_H
#define __KEY_H

#include "stm32f10x.h"

/*---------------- 按键硬件连接说明 ----------------
 *  UP    : PC13  上拉输入
 *  DOWN  : PC14  上拉输入
 *  OK    : PC15  上拉输入
 *  BACK  : PB0   上拉输入
 *------------------------------------------------*/

/* 端口与引脚定义 */
#define KEY_UP_PORT      GPIOC
#define KEY_UP_PIN       GPIO_Pin_13

#define KEY_DOWN_PORT    GPIOC
#define KEY_DOWN_PIN     GPIO_Pin_14

#define KEY_OK_PORT      GPIOC
#define KEY_OK_PIN       GPIO_Pin_15

#define KEY_BACK_PORT    GPIOB
#define KEY_BACK_PIN     GPIO_Pin_0

/* 逻辑按键值（和 Handle_Key / main.c 一致） */
#define KEY_UP           1
#define KEY_DOWN         2
#define KEY_OK           3
#define KEY_BACK         4

/* 对外函数接口 */
void    Key_Init(void);
uint8_t Key_Scan(void);
uint8_t Key_ScanWithDelay(void);

#endif
