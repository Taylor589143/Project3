#ifndef __LINE_H
#define __LINE_H

#include "stm32f10x.h"
#include "PID.h"      // 用到 line_pid 的 kp/ki/kd

/* ----------------- 线位置权重 -----------------
 * 传感器逻辑：在黑线上为 1，在白色背景为 0
 * 从左到右依次是：L2, L1, M, R1, R2
 * 使用权重：-5, -3, 0, +3, +5
 */
#define WEIGHT_L2      (-5.0f)
#define WEIGHT_L1      (-3.0f)
#define WEIGHT_M       ( 0.0f)
#define WEIGHT_R1      ( 3.0f)
#define WEIGHT_R2      ( 5.0f)

/* 线的“零点偏置”
 * 如果实测发现车始终偏一侧，可以把这个值调成 ±0.5 之类的小数
 */
#define POSITION_OFFSET  0.0f

/* ----------------- 速度相关参数 -----------------
 * STRAIGHT_SPEED：直线基础速度（最后你要跑快，可以只改这个）
 * CORNER_SPEED_MIN：弯道时的最低速度（太小会不动）
 * SPEED_K：弯道降速系数，越大转弯越慢
 */
extern int  STRAIGHT_SPEED;            // 后面想提速，就去line.c里面修改，菜单也可以用案件修改，增加了flash闪存函数
extern int  CORNER_SPEED_MIN; 
extern float  SPEED_K;


/* 电机速度限幅（注意 Motor_Set_Speed 内部也有死区判断） */
#define MAX_SPEED          100     // 不要超过你的 PWM 设计上限
#define MIN_SPEED           0      // 允许降到 0，不允许反转

/* ----------------- 其它参数（保留接口用） ----------------- */
#define CROSS_DELAY       300     // 可以不用，保留兼容
#define SHARP_TURN_DELAY  150     // 不再用“延时急转弯”，但宏先留着

/* 全黑保护时间：主循环大约 5ms 一次的话，600*5ms ≈ 3 秒 */
#define ALL_BLACK_LIMIT   600

/* ----------------- 全局变量声明 ----------------- */
extern unsigned char lukou_num;         // 十字路口计数
extern unsigned char last_line_status;  // 上一次状态（调试用，可简单映射）
extern unsigned int  straight_count;    // 连续“贴线”计数（用于直线加速）

/* ----------------- 功能函数接口 ----------------- */
void Track_Init(void);                      // 循迹系统初始化
void Handle_Crossroad(void);                // 十字路口处理（这里只做计数）
void Handle_Sharp_Turn(void);               // 兼容接口，内部可不做额外处理

void Track_Straight_Line(void);             // 兼容接口：内部直接调用 Advanced_Tracking
void Track_With_PID(int base_speed,
                    float kp, float ki, float kd); // 兼容接口：调用高级循迹
void Advanced_Tracking(void);               // ★核心：权重 + PID 高级循迹

/* ----------------- 辅助 / 调试接口 ----------------- */
unsigned char Get_Crossroad_Count(void);    // 获取已通过路口计数
void Track_Reset(void);                     // 复位循迹系统
void Track_Debug_Output(void);              // 串口打印当前循迹状态（调试用）

#endif




