#ifndef __SERIAL_H
#define __SERIAL_H

#include "stm32f10x.h"

/* ==========================================================
 * 串口通信模块头文件
 * 
 * 提供功能：
 *  - Serial_Init() : 初始化串口通信
 *  - USART_Send_Data() : 发送实时数据到上位机
 * 
 * 注意：
 *  串口波特率：115200
 *  上位机命令格式：@speed%数值
 * ========================================================== */

void Serial_Init(void);
void USART_Send_Data(int16_t speed1, int16_t target_speed);

#endif
