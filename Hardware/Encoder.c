#include "Encoder.h"

/* ==========================================================
 * 编码器驱动模块（简单版，不用指针数组）
 *
 * 硬件约定：
 *   左轮编码器：TIM3，通道1/2，PA6 / PA7
 *   右轮编码器：TIM1，通道1/2，PA8 / PA9
 * ========================================================== */

/* 兼容你同学写法的结构体实例 */
Encoder_TypeDef left_encoder  = {0, 0};
Encoder_TypeDef right_encoder = {0, 0};

/* 软件累计位置（从上电/清零开始累计的脉冲数） */
static int32_t encoder_pos1 = 0;   // 左轮
static int32_t encoder_pos2 = 0;   // 右轮

/* 上一次读取到的定时器计数值，用来做差得到“增量” */
static int32_t prev_count1 = 0;    // TIM3 上一次计数
static int32_t prev_count2 = 0;    // TIM1 上一次计数

/* 溢出判定相关常量（16 位计数器 0~65535） */
#define ENCODER_OVERFLOW_THRESHOLD  32768   // 超过这个值认为发生溢出
#define ENCODER_MAX_COUNT           65536   // 2^16

/* ==========================================================
 * 编码器初始化
 * ========================================================== */
void Encoder_Init(void)
{
    GPIO_InitTypeDef        GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;

    /* 1. 使能 GPIOA、TIM3、TIM1 时钟
     *    - TIM3 在 APB1 总线
     *    - TIM1 在 APB2 总线
     */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_TIM1, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

    /* 2. 配置 PA6/7/8/9 为上拉输入（编码器 A/B 相） */
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;      // 上拉输入，空闲时为高电平
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    // 左轮：PA6 / PA7 -> TIM3_CH1 / TIM3_CH2
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // 右轮：PA8 / PA9 -> TIM1_CH1 / TIM1_CH2
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_8 | GPIO_Pin_9;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* 3. 配置 TIM3 / TIM1 的计数器参数 */
    TIM_TimeBaseStructure.TIM_Prescaler     = 0;              // 不分频
    TIM_TimeBaseStructure.TIM_Period        = 0xFFFF;         // 16 位最大计数
    TIM_TimeBaseStructure.TIM_CounterMode   = TIM_CounterMode_Up;
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;

    /* ---- 左轮：TIM3 编码器模式 ---- */
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);
    TIM_EncoderInterfaceConfig(TIM3,
                               TIM_EncoderMode_TI12,          // 使用 CH1/CH2 两路
                               TIM_ICPolarity_Rising,
                               TIM_ICPolarity_Rising);
    TIM_SetCounter(TIM3, 0);
    TIM_Cmd(TIM3, ENABLE);

    /* ---- 右轮：TIM1 编码器模式 ---- */
    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);
    TIM_EncoderInterfaceConfig(TIM1,
                               TIM_EncoderMode_TI12,
                               TIM_ICPolarity_Rising,
                               TIM_ICPolarity_Rising);
    TIM_SetCounter(TIM1, 0);
    TIM_Cmd(TIM1, ENABLE);

    /* 4. 软件变量初始化 */
    prev_count1    = (int16_t)TIM_GetCounter(TIM3);
    prev_count2    = (int16_t)TIM_GetCounter(TIM1);
    encoder_pos1   = 0;
    encoder_pos2   = 0;

    left_encoder.count  = 0;
    left_encoder.speed  = 0;
    right_encoder.count = 0;
    right_encoder.speed = 0;
}


int16_t Encoder_Get_Speed(uint8_t num)
{
    int32_t current = 0;
    int32_t delta   = 0;

    /* -------- 左轮：TIM3 -------- */
    if (num == ENCODER_M1 || num == 1)
    {
        /* 1. 当前计数值（强转为有符号，用来做差） */
        current = (int16_t)TIM_GetCounter(TIM3);

        /* 2. 与上一次计数做差得到本次“增量” */
        delta   = current - prev_count1;
        prev_count1 = current;

        /* 3. 处理 16 位溢出 */
        if (delta > ENCODER_OVERFLOW_THRESHOLD)
        {
            delta -= ENCODER_MAX_COUNT;   // 低位绕到高位（负向溢出）
        }
        else if (delta < -ENCODER_OVERFLOW_THRESHOLD)
        {
            delta += ENCODER_MAX_COUNT;   // 高位绕到低位（正向溢出）
        }

        /* 4. 累加到“总位置” */
        encoder_pos1 += delta;

        /* 5. 更新兼容结构体 */
        left_encoder.count = encoder_pos1;
        left_encoder.speed = (int16_t)(delta * 1.85f);

        /* 6. 返回速度值 */
        return left_encoder.speed;
    }

    /* -------- 右轮：TIM1 -------- */
    else if (num == ENCODER_M2 || num == 2)
    {
        current = (int16_t)TIM_GetCounter(TIM1);
        delta   = current - prev_count2;
        prev_count2 = current;

        if (delta > ENCODER_OVERFLOW_THRESHOLD)
        {
            delta -= ENCODER_MAX_COUNT;
        }
        else if (delta < -ENCODER_OVERFLOW_THRESHOLD)
        {
            delta += ENCODER_MAX_COUNT;
        }

        encoder_pos2 += delta;

        right_encoder.count = encoder_pos2;
        right_encoder.speed = (int16_t)(delta * 1.85f);

        return right_encoder.speed;
    }

    /* 编号写错，直接返回 0 */
    return 0;
}

/* ==========================================================
 * 获取累计位置（注意：只有在调用过 Encoder_Get_Speed 后才会更新）
 * ========================================================== */
int32_t Encoder_Get_Position(uint8_t num)
{
    if (num == ENCODER_M1 || num == 1)
    {
        return encoder_pos1;
    }
    else if (num == ENCODER_M2 || num == 2)
    {
        return encoder_pos2;
    }
    else
    {
        return 0;
    }
}

/* ==========================================================
 * 清零某一路编码器的累计位置和“上次计数值”
 * ========================================================== */
void Encoder_Clear_TotalCount(uint8_t num)
{
    if (num == ENCODER_M1 || num == 1)
    {
        encoder_pos1 = 0;
        prev_count1  = (int16_t)TIM_GetCounter(TIM3);

        left_encoder.count = 0;
        left_encoder.speed = 0;
    }
    else if (num == ENCODER_M2 || num == 2)
    {
        encoder_pos2 = 0;
        prev_count2  = (int16_t)TIM_GetCounter(TIM1);

        right_encoder.count = 0;
        right_encoder.speed = 0;
    }
}



int32_t Encoder_Get_Left_Count(void)
{
    return (int32_t)(int16_t)TIM_GetCounter(TIM3);
}

int32_t Encoder_Get_Right_Count(void)
{
    return (int32_t)(int16_t)TIM_GetCounter(TIM1);
}

void Encoder_Reset_Both(void)
{
    /* 清硬件计数器 */
    TIM_SetCounter(TIM3, 0);
    TIM_SetCounter(TIM1, 0);

    /* 清软件变量 */
    encoder_pos1 = 0;
    encoder_pos2 = 0;
    prev_count1  = 0;
    prev_count2  = 0;

    left_encoder.count = 0;
    left_encoder.speed = 0;
    right_encoder.count = 0;
    right_encoder.speed = 0;
}
