#include "stm32f10x.h"

/* ==========================================================
 * 编码器驱动模块（Encoder.c）
 * 功能：
 *  - 配置 TIM3 / TIM4 为编码器接口模式
 *  - 提供速度与位置的读取接口
 * 
 * 保持原有逻辑完全一致，仅调整结构与注释。
 * ========================================================== */

// 修改：命名微调，语义更明确
static int32_t encoder_pos1 = 0, encoder_pos2 = 0;      // 累计位置（脉冲总数）
static int16_t prev_count1 = 0, prev_count2 = 0;        // 上次计数值（用于计算速度）


/**
 * @brief 初始化两个编码器接口（TIM3 & TIM4）
 * 
 * - TIM3 → 读取电机1
 * - TIM4 → 读取电机2
 * 
 * 编码器模式：TIMx_SMCR.SMS = 011（Encoder mode TI12）
 * 捕获信号：A相与B相上升沿均计数
 */
void Encoder_Init(void)
{
    // ★修改：合并时钟配置，提高可读性
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3 | RCC_APB1Periph_TIM4, ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;           // 上拉输入
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    // 编码器1 - TIM3 (PA6, PA7)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // 编码器2 - TIM4 (PB6, PB7)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_TimeBaseStructure.TIM_Prescaler = 0;
    TIM_TimeBaseStructure.TIM_Period = 0xFFFF;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;

    // 配置编码器1（TIM3）
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);
    TIM_EncoderInterfaceConfig(TIM3, TIM_EncoderMode_TI12, TIM_ICPolarity_Rising, TIM_ICPolarity_Rising);
    TIM_SetCounter(TIM3, 0);
    TIM_Cmd(TIM3, ENABLE);

    // 配置编码器2（TIM4）
    TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);
    TIM_EncoderInterfaceConfig(TIM4, TIM_EncoderMode_TI12, TIM_ICPolarity_Rising, TIM_ICPolarity_Rising);
    TIM_SetCounter(TIM4, 0);
    TIM_Cmd(TIM4, ENABLE);

    // 初始化上次计数值
    prev_count1 = TIM_GetCounter(TIM3);
    prev_count2 = TIM_GetCounter(TIM4);
}


/**
 * @brief 获取编码器转速（单位：脉冲数 / 10ms）
 * @param num 编码器编号（1：电机1，2：电机2）
 * @return 编码器在10ms内的脉冲变化量（正反区分方向）
 * 
 * 原理：
 * - 速度 = 当前计数 - 上次计数
 * - 若溢出，则进行±65536补偿
 */
int16_t Encoder_Get_Speed(uint8_t num)
{
    int16_t current_count = 0;
    int16_t delta = 0;

    if (num == 1)
    {
        current_count = TIM_GetCounter(TIM3);
        delta = current_count - prev_count1;
        prev_count1 = current_count;

        // 溢出修正（16位补偿）
        if (delta > 32768) delta -= 65536;
        if (delta < -32768) delta += 65536;

        encoder_pos1 += delta;
    }
    else if (num == 2)
    {
        current_count = TIM_GetCounter(TIM4);
        delta = current_count - prev_count2;
        prev_count2 = current_count;

        if (delta > 32768) delta -= 65536;
        if (delta < -32768) delta += 65536;

        encoder_pos2 += delta;
    }

    return (int16_t)(delta * 1.85f);  // 修改：添加f后缀，防止隐式类型警告
}


/**
 * @brief 获取编码器累计位置（相对值）
 * @param num 编码器编号（1或2）
 * @return 从清零以来的累计脉冲数
 */
int32_t Encoder_Get_Position(uint8_t num)
{
    if (num == 1)
        return encoder_pos1;
    else
        return encoder_pos2;
}


/**
 * @brief 清零编码器累计位置（重置基准）
 * @param num 编码器编号（1或2）
 * 
 * 在切换位置控制模式时调用。
 */
void Encoder_Clear_TotalCount(uint8_t num)
{
    if (num == 1)
    {
        encoder_pos1 = 0;
        prev_count1 = TIM_GetCounter(TIM3);
    }
    else
    {
        encoder_pos2 = 0;
        prev_count2 = TIM_GetCounter(TIM4);
    }
}
