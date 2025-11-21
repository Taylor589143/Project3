#ifndef __LINE_H
#define __LINE_H

#include "stm32f10x.h"
#include "PID.h"      //用到Line_pid 的kp/ki/kd


//循迹参数配置
#define STRAIGHT_SPEED    60      // 直道速度
#define CROSS_DELAY       300     // 十字直行时间 ms
#define SHARP_TURN_DELAY  150     // 急转弯时间 ms

//全局变量声明
extern unsigned char lukou_num;         // 路口计数
extern unsigned char last_line_status;  // 上一次循迹状态
extern unsigned int  straight_count;    // 连续直行计数


//函数声明
void Track_Init(void);
void Handle_Crossroad(void);
void Handle_Sharp_Turn(void);
void Track_Straight_Line(void);
void Track_With_PID(int base_speed, float kp, float ki, float kd);
void Advanced_Tracking(void);


//辅助接口（调试时可用）
unsigned char Get_Crossroad_Count(void);
void Track_Reset(void);
void Track_Debug_Output(void);

#endif
