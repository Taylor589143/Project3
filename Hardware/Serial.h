#ifndef __SERIAL_H
#define __SERIAL_H

#include "stm32f10x.h"

void USART1_Init(void);
void Serial_Init(void);
void USART_Send_Data(int16_t speed_now, int16_t target_speed);
void USART_SendByte(uint8_t byte);
void USART_SendStrring(char *str);

#endif
