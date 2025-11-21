#include "Encoder.h"

/* ==========================================================
 * 编码器驱动模块（Encoder.c）
 *
 * 功能：
 *  - 使用 TIM3 / TIM4 的编码器接口模式读取两路电机编码器
 *  - 提供“速度”（每调用一次的脉冲增量）和“累计位置”接口
 *
 * 约定：
 *  - num = 1 -> 使用 TIM3（一般接电机1，PA6/PA7）
 *  - num = 2 -> 使用 TIM4（一般接电机2，PB6/PB7）
 * ========================================================== */

/* 累计位置（从清零开始，累加每次的脉冲增量） */
static int32_t encoder_pos1 = 0;
static int32_t encoder_pos2 = 0;

/* 上一次读取到的定时器计数值，用于做差计算“增量” */
static int32_t prev_count1 = 0;
static int32_t prev_count2 = 0;

/**
 * @brief 编码器初始化函数
 *
 * 配置内容：
 *  - GPIOA6 / GPIOA7 用作 TIM3 编码器输入（电机1）
 *  - GPIOB6 / GPIOB7 用作 TIM4 编码器输入（电机2）
 *  - TIM3 / TIM4 工作在 Encoder Interface Mode TI12
 *  - 计数器范围 0~0xFFFF，向上计数
 */
void Encoder_Init(void)
{
    GPIO_InitTypeDef        GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;

    /* 1. 打开 GPIOA / GPIOB 和 TIM3 / TIM4 的时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3 | RCC_APB1Periph_TIM4, ENABLE);

    /* 2. 配置编码器引脚为上拉输入
     *    理论上也可以配置为浮空输入，根据具体硬件决定。
     */
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;      // 上拉输入
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    /* 电机1：TIM3 的 A/B 相：PA6 / PA7 */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* 电机2：TIM4 的 A/B 相：PB6 / PB7 */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* 3. 配置 TIM3 / TIM4 的基本计数参数 */
    TIM_TimeBaseStructure.TIM_Prescaler     = 0;               // 不分频，直接计数
    TIM_TimeBaseStructure.TIM_Period        = 0xFFFF;          // 16 位计数器最大值
    TIM_TimeBaseStructure.TIM_CounterMode   = TIM_CounterMode_Up;
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;

    /* 配置 TIM3 为编码器模式（电机1） */
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);
    /* TI12 模式：使用 TI1 和 TI2 两路输入，A/B 相共同决定计数方向 */
    TIM_EncoderInterfaceConfig(TIM3,
                               TIM_EncoderMode_TI12,
                               TIM_ICPolarity_Rising,   // A 相上升沿
                               TIM_ICPolarity_Rising);  // B 相上升沿
    TIM_SetCounter(TIM3, 0);
    TIM_Cmd(TIM3, ENABLE);

    /* 配置 TIM4 为编码器模式（电机2） */
    TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);
    TIM_EncoderInterfaceConfig(TIM4,
                               TIM_EncoderMode_TI12,
                               TIM_ICPolarity_Rising,
                               TIM_ICPolarity_Rising);
    TIM_SetCounter(TIM4, 0);
    TIM_Cmd(TIM4, ENABLE);

    /* 4. 初始化“上一次计数”的起点 */
    prev_count1 = TIM_GetCounter(TIM3);
    prev_count2 = TIM_GetCounter(TIM4);

    /* 同时把累计位置清零 */
    encoder_pos1 = 0;
    encoder_pos2 = 0;
}

/**
 * @brief 获取编码器速度（每次调用间隔的脉冲数）
 *
 * @param num 编码器编号：
 *            1 -> TIM3 对应的编码器
 *            2 -> TIM4 对应的编码器
 *
 * @return 速度值（单位：脉冲数/调用间隔），带符号，用于区分正反转方向
 *
 * 原理说明：
 *  1. 读取当前 TIMx 的计数值 current_count
 *  2. 和上一次的 prev_count 做差 delta = current_count - prev_count
 *  3. 若中间发生 16 位溢出，会出现“差值特别大”，
 *     - 当 delta >  32768 时，认为是向负方向溢出一次，做 delta -= 65536
 *     - 当 delta < -32768 时，认为是向正方向溢出一次，做 delta += 65536
 *  4. 把 delta 累加到 encoder_posX 中，得到“累计位置”
 *  5. 返回 delta * 1.85f 作为速度值
 */
int16_t Encoder_Get_Speed(uint8_t num)
{
    int32_t current_count = 0;     //用32位做中间计算
    int32_t delta         = 0;

    if (num == 1)
    {
        /* 读取 TIM3 当前计数并与上次值做差 */
        current_count = (int16_t)TIM_GetCounter(TIM3);   //TIM_GetCounter  返回本来就是16
        delta         = current_count - prev_count1;
        prev_count1   = current_count;

        /* 溢出修正：
         *  - 计数器是 0~65535
         *  - 正常情况下，两次调用间隔内的增量不会非常大
         *  - 当 delta >  32768，说明可能从低位绕回高位（负方向溢出）
         *    -> 减去 65536，相当于 delta = delta - 2^16
         *  - 当 delta < -32768，说明可能从高位绕回低位（正方向溢出）
         *    -> 加上 65536，相当于 delta = delta + 2^16
         */
        if (delta > 32768)  delta -= 65536;
        if (delta < -32768) delta += 65536;

        /* 累加到电机1的“总位置” */
        encoder_pos1 += delta;
    }
    else if (num == 2)
    {
        /* 读取 TIM4 当前计数并与上次值做差 */
        current_count = (int16_t)TIM_GetCounter(TIM4);
        delta         = current_count - prev_count2;
        prev_count2   = current_count;

        if (delta > 32768)  delta -= 65536;
        if (delta < -32768) delta += 65536;

        encoder_pos2 += delta;
    }
    else
    {
        /* num 传错时，直接返回 0 */
        delta = 0;
    }


    return (int16_t)(delta * 1.85f);
}

/**
 * @brief 获取编码器累计位置（相对值）
 *
 * @param num 编码器编号（1 或 2）
 * @return 从 Encoder_Init 或 Encoder_Clear_TotalCount 以来的总脉冲数
 *
 * 说明：
 *  - 这个值等于每次 Encoder_Get_Speed() 返回的“delta”不断累加的结果。
 *  - 可以用于位置环控制、转过多少圈等计算。
 */
int32_t Encoder_Get_Position(uint8_t num)
{
    if (num == 1)
    {
        return encoder_pos1;
    }
    else if (num == 2)
    {
        return encoder_pos2;
    }
    else
    {
        return 0;
    }
}

/**
 * @brief 清零某一路编码器的累计位置
 *
 * @param num 编码器编号（1 或 2）
 *
 * 使用场景：
 *  - 例如小车刚对好起跑线，需要把当前位置当作“0点”；
 *  - 切换位置控制的目标点时，也可以重置累计位置。
 *
 * 实现说明：
 *  - 把 encoder_posX 置 0；
 *  - 同时把 prev_countX 更新为当前 TIM 计数值，
 *    这样下一次调用 Encoder_Get_Speed() 时不会出现一个很大的“跳变”。
 */
void Encoder_Clear_TotalCount(uint8_t num)
{
    if (num == 1)
    {
        encoder_pos1 = 0;
        prev_count1  = (int16_t)TIM_GetCounter(TIM3);
    }
    else if (num == 2)
    {
        encoder_pos2 = 0;
        prev_count2  = (int16_t)TIM_GetCounter(TIM4);
    }
}
