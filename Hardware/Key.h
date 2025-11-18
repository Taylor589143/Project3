#ifndef __KEY_H
#define __KEY_H

#include "stm32f10x.h"

#define KEY_UP_PIN GPIO_Pin_0
#define KEY_DOWN_PIN GPIO_Pin_1
#define KEY_OK_PIN GPIO_Pin_10  
#define KEY_BACK_PIN GPIO_Pin_11
#define KEY_PORT GPIOB

#define KEY_UP 1
#define KEY_DOWN 2
#define KEY_OK 3
#define KEY_BACK 4

void Key_Init(void);
uint8_t Key_Scan(void);
uint8_t Key_ScanWithDelay(void);

#endif