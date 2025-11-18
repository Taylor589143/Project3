#ifndef __PID_H
#define __PID_H

#include "stm32f10x.h"

void Speed_PID_SetParams(float p, float i, float d);
void Position_PID_SetParams(float p, float i, float d);
int16_t Speed_PID_Compute(int16_t target, int16_t actual);  

int16_t Position_PID_Compute(int32_t target, int32_t actual);
void Speed_PID_Reset(void);

#endif
