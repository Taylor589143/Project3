#include "stm32f10x.h"
#include "PID.h"

/* ==========================================================
 *  线控 / 速度环 PID（line_pid）
 * ----------------------------------------------------------
 *  用途：
 *    - 作为循迹偏差控制器（常见用法：根据偏差调整左右轮差速）
 *    - 也可以作为简单速度环使用
 * ========================================================== */
PID_TypeDef line_pid =
{
    .kp         = 8.0f,   // 这里略小一点，避免一上来太凶
    .ki         = 0.02f,   // 积分稍弱一点，主要还是靠 P+D
    .kd         = 6.8f,   // 略小于 3.5f，减轻毛刺和噪声对控制的影响
    .integral   = 0.0f,
    .last_error = 0.0f
};

/* ==========================================================
 *  位置环 PID（position_pid）
 * ----------------------------------------------------------
 *  当前仍然只用比例项，ki / kd 预留接口给以后扩展。
 * ========================================================== */
PID_TypeDef position_pid =
{
    .kp         = 0.15f,  // 位置环一般不需要太大，容易抖
    .ki         = 0.0f,
    .kd         = 0.0f,
    .integral   = 0.0f,
    .last_error = 0.0f
};

/* 为了看得更清楚，把一些“常量”独立出来 */
#define LINE_PID_I_LIMIT   20.0f   // 线控 PID 积分限幅（绝对值），比你原来 100 小很多
#define LINE_PID_OUT_LIMIT 50.0f   // 输出限幅（绝对值），对应 PWM 微调量，大概是 ±50

/* ==========================================================
 *  通用 PID 计算函数
 * ----------------------------------------------------------
 *  参数：
 *    pid          - 传入要使用的 PID 实例（如 &line_pid）
 *    setpoint     - 目标值（期望）
 *    current_val  - 当前测量值
 *
 *  返回：
 *    PID 输出（float），由外层根据需要转成 PWM / 速度等
 *
 *  算法特点：
 *   1）比例项：主导响应快慢
 *   2）积分项：只用来消除静差，并且积分限幅比较严格，防止“积分风up”
 *   3）微分项：抑制误差变化过快，减小超调
 *   4）输出再做一次限幅，保护电机 / 避免过猛
 * ========================================================== */
float PID_Calculate(PID_TypeDef *pid, float setpoint, float current_value)
{
    /* 1. 本次误差 */
    float error = setpoint - current_value;

    /* 2. 比例项（P）*/
    float proportional = pid->kp * error;

    /* 3. 积分项（I）：累加误差，带限幅 */
    pid->integral += error;

    /* 积分限幅：只允许在 [-LINE_PID_I_LIMIT, LINE_PID_I_LIMIT] 之间 */
    if (pid->integral > LINE_PID_I_LIMIT)
    {
        pid->integral = LINE_PID_I_LIMIT;
    }
    else if (pid->integral < -LINE_PID_I_LIMIT)
    {
        pid->integral = -LINE_PID_I_LIMIT;
    }
    float integral = pid->ki * pid->integral;

    /* 4. 微分项（D）：误差变化率 */
    float derivative = pid->kd * (error - pid->last_error);

    /* 5. 原始输出 = P + I + D */
    float output = proportional + integral + derivative;

    /* 6. 输出限幅
     *    这里的思路是：PID 的输出用作“微调量”，
     *    而不是直接把整个 PWM 全交给 PID 控。
     *
     *    比如：基础前进速度 300，然后左右轮 +output/-output。
     */
    if (output > LINE_PID_OUT_LIMIT)
    {
        output = LINE_PID_OUT_LIMIT;
    }
    else if (output < -LINE_PID_OUT_LIMIT)
    {
        output = -LINE_PID_OUT_LIMIT;
    }

    /* 7. 记录本次误差，用于下次 D 项计算 */
    pid->last_error = error;

    return output;
}

/* ==========================================================
 *  PID 状态清零
 * ----------------------------------------------------------
 *  把积分项和 last_error 清零，常用于：
 *    - 换模式时
 *    - 停车 / 重新发车前
 * ========================================================== */
void PID_Reset(PID_TypeDef *pid)
{
    pid->integral   = 0.0f;
    pid->last_error = 0.0f;
}

/* ==========================================================
 *  速度 / 线控 PID 对外接口（封装一层方便调用）
 * ========================================================== */

/**
 * @brief  通过菜单 / 串口等修改 line_pid 参数
 * @param  p,i,d 新的 PID 参数
 *
 * 说明：
 *   - 你在菜单里调节 kp/ki/kd，其实就是在改 line_pid 的这三个字段。
 *   - 这里不改 integral/last_error，避免调参时状态被打断。
 */
void Speed_PID_SetParams(float p, float i, float d)
{
    line_pid.kp = p;
    line_pid.ki = i;
    line_pid.kd = d;
}

/**
 * @brief  速度/偏差 PID 计算封装
 * @param  target  目标值（编码器速度/偏差等）
 * @param  actual  实际值
 * @return int16_t 格式的 PID 输出，用于直接叠加到 PWM 上
 */
int16_t Speed_PID_Compute(int16_t target, int16_t actual)
{
    float out = PID_Calculate(&line_pid,
                              (float)target,
                              (float)actual);

    return (int16_t)out;
}

/* 方便在切换模式 / 重新发车时，快速清空 line_pid 内部状态 */
void Speed_PID_Reset(void)
{
    PID_Reset(&line_pid);
}

/* ==========================================================
 *  位置环接口（Position PID）
 * ----------------------------------------------------------
 *  当前版本仍然只使用 Kp * 误差，作为简单的位置控制器；
 *  如果后续你需要更稳的“停车到某个点”，可以在这里加上 I / D。
 * ========================================================== */

/**
 * @brief 设置位置环 PID 参数（目前只用 kp）
 */
void Position_PID_SetParams(float p, float i, float d)
{
    position_pid.kp = p;
    (void)i;  // 先不用，防止未使用警告
    (void)d;
}

/**
 * @brief 位置 PID 计算（当前只做比例控制）
 * @param target 目标位置（编码器脉冲）
 * @param actual 当前实际位置（编码器脉冲）
 * @return int16_t 位置环输出，用作速度指令或 PWM 指令
 */
int16_t Position_PID_Compute(int32_t target, int32_t actual)
{
    int32_t error = target - actual;          // 位置误差
    float   out   = position_pid.kp * error;  // 只用比例项

    /* 位置环输出限幅：一般不希望位置环太暴力 */
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
