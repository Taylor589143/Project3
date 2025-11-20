#ifndef __PID_H
#define __PID_H

#include "stm32f10x.h"


typedef struct
{
    float kp;          // 比例系数
    float ki;          // 积分系数
    float kd;          // 微分系数

    float integral;    // 积分累计
    float last_error;  // 上一次误差 e(k-1)
} PID_TypeDef;

extern PID_TypeDef line_pid;
extern PID_TypeDef position_pid;


/* -------- 通用 PID 函数（老师模板） -------- */
/**
 * @brief 通用 PID 计算函数（位置式 PID）
 * @param pid           PID 结构体指针
 * @param setpoint      目标值
 * @param current_value 当前值
 * @return PID 输出（float）
 *
 * 算法：
 *   error      = setpoint - current
 *   integral  += error （带积分限幅）
 *   derivative = error - last_error
 *   output     = Kp*error + Ki*integral + Kd*derivative
 *   对 output 再做限幅
 */
float PID_Calculate(PID_TypeDef *pid, float setpoint, float current_value);

/**
 * @brief 重置指定 PID 控制器
 * @param pid PID 结构体指针
 */

void PID_Reset(PID_TypeDef *pid);
void Speed_PID_SetParams(float p, float i, float d);              /* 设置速度环 PID 参数（内部实际就是改 line_pid 的 kp/ki/kd） */
void Position_PID_SetParams(float p, float i, float d);           /* 设置位置环 PID 参数（目前只使用 Kp，Ki / Kd 预留） */
int16_t Speed_PID_Compute(int16_t target, int16_t actual);        /* 速度环：根据目标速度和实际速度，算出 PWM 控制量 */
int16_t Position_PID_Compute(int32_t target, int32_t actual);     /* 位置环：简化为比例控制，输出较小 PWM */

void Speed_PID_Reset(void);

#endif
