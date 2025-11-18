#include "stm32f10x.h"
#include "Motor.h"

//定义全局电机状态变量
int32_t last_position1 = 0;
int32_t target_position2 = 0;
extern uint8_t current_mode;  // 当前控制模式：1-速度模式，2-位置模式等

// =====================================================
// 函数名称：PWM_Init
// 功能描述：初始化TIM2通道3/4 PWM输出以及电机方向控制GPIO
// 参数说明：无
// 返回值：无
// =====================================================
void PWM_Init(void)
{
    //使能GPIOA/B及AFIO时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure;

    //配置PA2/PA3为复用推挽输出（PWM输出）
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;  // 复用推挽
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    //配置PB12-PB15为普通推挽输出（电机方向控制）
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    //配置TIM2内部时钟
    TIM_InternalClockConfig(TIM2);

    //定时器基础配置
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseStructure.TIM_Period = 1000 - 1;    // PWM周期
    TIM_TimeBaseStructure.TIM_Prescaler = 72 - 1;   // 1MHz计数频率
    TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

    // ⑥ 配置PWM输出通道
    TIM_OCInitTypeDef TIM_OCInitStructure;
    TIM_OCStructInit(&TIM_OCInitStructure);
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = 0;  // 初始占空比为0

    TIM_OC3Init(TIM2, &TIM_OCInitStructure);  // 电机1
    TIM_OC4Init(TIM2, &TIM_OCInitStructure);  // 电机2

    //启动定时器
    TIM_Cmd(TIM2, ENABLE);
}


// 函数名称：Motor_Set_Speed
// 功能描述：设置指定电机的转速与方向
// 参数说明：motor_num - 电机编号（1或2）
//           speed - 速度值，正数正转，负数反转，0停止
// 返回值：无

void Motor_Set_Speed(uint8_t motor_num, int16_t speed)
{
    // 限幅处理 
    if(speed > 1000) speed = 1000;
    if(speed < -1000) speed = -1000;

    // 位置模式特殊限制
    if(current_mode == 2)  // 位置模式
    {
        // 限制最大速度
        if(speed > 200) speed = 200;
        if(speed < -200) speed = -200;

        // 死区处理：避免微小振荡
        if(speed > 0 && speed < 120) speed = 0;
        else if(speed < 0 && speed > -120) speed = 0;
    }
    else  // 速度模式
    {
        if(speed > 0 && speed < 30) speed = 0;
        else if(speed < 0 && speed > -30) speed = 0;
    }

    // 设置电机1
    if(motor_num == 1)
    {
        if(speed == 0)
        {
            GPIO_ResetBits(GPIOB, GPIO_Pin_12 | GPIO_Pin_13);  // 停止
        }
        else if(speed > 0)
        {
            GPIO_SetBits(GPIOB, GPIO_Pin_12);
            GPIO_ResetBits(GPIOB, GPIO_Pin_13);  // 正转
        }
        else
        {
            GPIO_ResetBits(GPIOB, GPIO_Pin_12);
            GPIO_SetBits(GPIOB, GPIO_Pin_13);    // 反转
            speed = -speed;                       // 占空比取正
        }
        TIM_SetCompare3(TIM2, speed);  // 设置占空比
    }
    // 设置电机2
    else if(motor_num == 2)
    {
        if(speed == 0)
        {
            GPIO_ResetBits(GPIOB, GPIO_Pin_14 | GPIO_Pin_15);
        }
        else if(speed > 0)
        {
            GPIO_SetBits(GPIOB, GPIO_Pin_14);
            GPIO_ResetBits(GPIOB, GPIO_Pin_15);
        }
        else
        {
            GPIO_ResetBits(GPIOB, GPIO_Pin_14);
            GPIO_SetBits(GPIOB, GPIO_Pin_15);
            speed = -speed;
        }
        TIM_SetCompare4(TIM2, speed);
    }
}
