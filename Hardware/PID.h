#ifndef __PID_H
#define __PID_H

#include "stm32f10x.h"

/* ==========================================================
 *  PID 控制器结构体定义
 * ----------------------------------------------------------
 *  kp, ki, kd     : 比例 / 积分 / 微分系数
 *  integral       : 误差积分累加
 *  last_error     : 上一次误差，用于计算微分项
 * ========================================================== */
typedef struct
{
    float kp;          // 比例系数
    float ki;          // 积分系数
    float kd;          // 微分系数

    float integral;    // 积分累计量
    float last_error;  // 上一次误差 e(k-1)
} PID_TypeDef;

/* ==========================================================
 *  全局 PID 实例
 * ----------------------------------------------------------
 *  line_pid      : 线控 / 速度环 PID（循迹用的那个）
 *  position_pid  : 位置环 PID（位置控制时用）
 * ========================================================== */
extern PID_TypeDef line_pid;
extern PID_TypeDef position_pid;

/* ==========================================================
 *  通用 PID 相关函数
 * ========================================================== */

/**
 * @brief 通用位置式 PID 计算
 *
 * @param pid           指向要使用的 PID 控制器
 * @param setpoint      目标值（期望）
 * @param current_value 当前测量值
 *
 * @return PID 输出（float，由上层决定怎么映射到 PWM / 速度）
 *
 * 计算步骤：
 *   error      = setpoint - current_value
 *   integral  += error  （内部自带积分限幅）
 *   derivative = error - last_error
 *   output     = Kp*error + Ki*integral + Kd*derivative
 *   最后对 output 再做一次限幅
 */
float PID_Calculate(PID_TypeDef *pid,
                    float setpoint,
                    float current_value);

/**
 * @brief 清零一个 PID 控制器的内部状态
 *
 * @param pid 指向要清零的 PID 结构体
 *
 * 说明：
 *   - 把 integral、last_error 置 0
 *   - 一般在“切换模式 / 重启控制 / 刚发车前”调用
 */
void PID_Reset(PID_TypeDef *pid);

/* ==========================================================
 *  线控 / 速度环 PID 接口（封装 line_pid）
 * ========================================================== */

/* 修改 line_pid 的 kp / ki / kd 参数（菜单或串口调参时调用） */
void    Speed_PID_SetParams(float p, float i, float d);

/* 根据目标速度 / 偏差 和 实际值，计算 PID 输出（用于速度/循迹） */
int16_t Speed_PID_Compute(int16_t target, int16_t actual);

/* 把 line_pid 的积分和 last_error 清零 */
void    Speed_PID_Reset(void);

/* ==========================================================
 *  位置环 PID 接口（封装 position_pid）
 * ========================================================== */

/* 设置位置环 PID 的参数（目前主要用 kp，ki/kd 预留） */
void    Position_PID_SetParams(float p, float i, float d);

/* 根据目标位置 / 当前实际位置，计算位置环的输出 */
int16_t Position_PID_Compute(int32_t target, int32_t actual);

#endif /* __PID_H */
