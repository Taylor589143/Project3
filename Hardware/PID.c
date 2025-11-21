#include "stm32f10x.h"
#include "PID.h"

PID_TypeDef line_pid =
{
    .kp         = 2.0f,
    .ki         = 0.5f,
    .kd         = 0.1f,
    .integral   = 0.0f,
    .last_error = 0.0f
};

/* 位置环 PID，目前只用 kp，其它参数预留 */
PID_TypeDef position_pid =
{
    .kp         = 0.15f,
    .ki         = 0.0f,
    .kd         = 0.0f,
    .integral   = 0.0f,
    .last_error = 0.0f
};

/* -------- 通用 PID 实现（参考老师模板） -------- */

float PID_Calculate(PID_TypeDef *pid, float setpoint, float current_value)
{
    /* 1. 计算本次误差 */
    float error = setpoint - current_value;

    /* 2. 比例项 */
    float proportional = pid->kp * error;

    /* 3. 积分项：累加误差，并做限幅防止积分“爆掉” */
    pid->integral += error;
    if (pid->integral > 1000.0f)
    {
        pid->integral = 1000.0f;
    }
    else if (pid->integral < -1000.0f)
    {
        pid->integral = -1000.0f;
    }
    float integral = pid->ki * pid->integral;

    /* 4. 微分项：当前误差 - 上一次误差 */
    float derivative = pid->kd * (error - pid->last_error);

    /* 5. 三项相加得到原始输出 */
    float output = proportional + integral + derivative;

    /* 6. 输出限幅
     *    这里按速度环原来 PWM 的范围设置为 ±800，
     *    可以根据自己 PWM 最大值再调整。
     */
    if (output > 800.0f)
    {
        output = 800.0f;
    }
    else if (output < -800.0f)
    {
        output = -800.0f;
    }

    /* 7. 保存本次误差，用于下次微分计算 */
    pid->last_error = error;

    return output;
}

void PID_Reset(PID_TypeDef *pid)
{
    pid->integral   = 0.0f;
    pid->last_error = 0.0f;
}

/* -------- 速度环对外接口 -------- */

void Speed_PID_SetParams(float p, float i, float d)
{
    line_pid.kp = p;
    line_pid.ki = i;
    line_pid.kd = d;
}

/**
 * @brief 速度环 PID 计算
 * @param target 目标速度（编码器单位）
 * @param actual 实际速度（编码器单位）
 * @return PWM 输出（int16_t，有正负）
 *
 * 内部直接调用通用 PID_Calculate，对象是全局 line_pid。
 * 这样：
 *   - 菜单里改 line_pid.kp/ki/kd 直接生效；
 *   - 串口里用 Speed_PID_SetParams 也只是改 line_pid。
 */
int16_t Speed_PID_Compute(int16_t target, int16_t actual)
{
    float out = PID_Calculate(&line_pid,
                              (float)target,
                              (float)actual);

    return (int16_t)out;
}

void Speed_PID_Reset(void)
{
    PID_Reset(&line_pid);
}

/* -------- 位置环对外接口 -------- */

/**
 * @brief 位置环比例控制（简化版）
 *
 * 位置控制容易振荡，这里按你原来的思路，
 * 只做一个简单的 Kp * 误差，再做一个比较小的限幅。
 */
void Position_PID_SetParams(float p, float i, float d)
{
    position_pid.kp = p;
    (void)i;    // 先不用，防止未使用警告
    (void)d;
}

int16_t Position_PID_Compute(int32_t target, int32_t actual)
{
    int32_t error = target - actual;          // 位置误差
    float   out   = position_pid.kp * error;  // 只用比例项

    /* 位置环输出限幅，一般不需要太大扭矩 */
    if (out > 50.0f)
    {
        out = 50.0f;
    }
    else if (out < -50.0f)
    {
        out = -50.0f;
    }

    return (int16_t)out;
}
