#include "stm32f10x.h"
#include "stdlib.h"


static float speed_output = 0.0f;        
static float speed_kp = 2.0f;
static float speed_ki = 0.5f;
static float speed_kd = 0.1f;

// 误差队列：error[0] 当前误差, error[1] 上次误差, error[2] 上上次误差
static float speed_err[3] = {0, 0, 0};   

/* 位置环 PID 参数  */
// 位置环使用纯比例控制（无积分与微分项，减少机械振荡）
static float position_kp = 0.15f;       

/**
 * @brief 增量式速度 PID 计算函数
 * @param target  目标速度值
 * @param actual  实际速度值（编码器反馈）
 * @return PWM 输出增量（int16_t）
 * 
 * 公式说明：
 * Δu = Kp*(e(k)-e(k-1)) + Ki*e(k) + Kd*(e(k)-2e(k-1)+e(k-2))
 * 
 * 增量式 PID 仅输出“变化量”，适合电机速度闭环控制，
 * 能有效抑制积分饱和与突变。
 */
int16_t Speed_PID_Compute(int16_t target, int16_t actual)  // ★修改：原函数名 Speed_PID_Calculate
{
    /* 1️更新误差序列 */
    speed_err[2] = speed_err[1];
    speed_err[1] = speed_err[0];
    speed_err[0] = target - actual;   // 当前误差 e(k)
    
    /* 2️增量式 PID 计算公式 */
    speed_output += speed_kp * (speed_err[0] - speed_err[1]) +
                    speed_ki * speed_err[0] +
                    speed_kd * (speed_err[0] - 2 * speed_err[1] + speed_err[2]);
    
    /* 3️输出限幅，防止PWM过大损坏电机 */
    if (speed_output > 800)  speed_output = 800;
    if (speed_output < -800) speed_output = -800;
    
    /* 4️返回控制量（PWM值） */
    return (int16_t)speed_output;
}

/**
 * @brief 位置控制器（简化为比例环）
 * @param target  目标位置
 * @param actual  实际位置
 * @return PWM 输出值（int16_t）
 * 
 * 使用纯比例控制：u = Kp * e
 * 
 * 由于位置控制容易产生振荡，因此本环路不使用积分项。
 */
int16_t Position_PID_Compute(int32_t target, int32_t actual)  
{
    int32_t pos_err = target - actual;  
    float output_val = position_kp * pos_err;

    // 严格限幅：位置控制仅需较小力矩输出
    if (output_val > 50)  output_val = 50;
    if (output_val < -50) output_val = -50;

    return (int16_t)output_val;
}

/**
 * @brief 设置速度环 PID 参数
 * @param p 比例系数
 * @param i 积分系数
 * @param d 微分系数
 */
void Speed_PID_SetParams(float p, float i, float d)
{
    speed_kp = p;
    speed_ki = i;
    speed_kd = d;
    // ★新增注释：允许上位机动态调参以优化响应
}

/**
 * @brief 设置位置环 PID 参数
 * @param p 比例系数
 * @param i 积分系数（保留接口，不使用）
 * @param d 微分系数（保留接口，不使用）
 */
void Position_PID_SetParams(float p, float i, float d)
{
    position_kp = p;
    // i、d参数被忽略，保留接口以保持兼容性
}

/**
 * @brief 重置速度 PID 内部状态
 * 
 * 在模式切换或目标速度突变时调用，
 * 清空积分累积与历史误差，防止积分饱和。
 */
void Speed_PID_Reset(void)
{
    speed_err[0] = speed_err[1] = speed_err[2] = 0;
    speed_output = 0.0f;  
}
