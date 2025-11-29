#include "stm32f10x.h"
#include "Motor.h"


// 全局变量（给别的文件用的变量，在这里真正“定义”）

int32_t last_position1   = 0;   // 电机1 上一次位置（预留）
int32_t target_position2 = 0;   // 电机2 目标位置（预留）


void PWM_Init(void)
{
    // 1. 开时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA |
                           RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    GPIO_InitTypeDef        GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef       TIM_OCInitStructure;

    // 2. PA2 / PA3 配置为复用推挽输出（TIM2_CH3 / TIM2_CH4）
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_2 | GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;      // 复用推挽
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // 3. PB12~PB15 配置为普通推挽输出（电机方向控制）
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_12 | GPIO_Pin_13 |
                                    GPIO_Pin_14 | GPIO_Pin_15;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;     // 推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    // 4. 定时器基础配置：
    //    72MHz / 72 = 1MHz 计数频率，Period = 100 => PWM 频率约 10kHz
    //    占空比范围 0~100，方便理解和调试
    TIM_TimeBaseStructure.TIM_ClockDivision     = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode       = TIM_CounterMode_Up;
    TIM_TimeBaseStructure.TIM_Prescaler         = 72 - 1;     // 1MHz
    TIM_TimeBaseStructure.TIM_Period            = 100 - 1;    // 0~99
    TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

    // 5. PWM 通道配置（CH3 / CH4）
    TIM_OCStructInit(&TIM_OCInitStructure);
    TIM_OCInitStructure.TIM_OCMode      = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_OCPolarity  = TIM_OCPolarity_High;
    TIM_OCInitStructure.TIM_Pulse       = 0;                // 初始占空比 0%

    // 左电机：CH3
    TIM_OC3Init(TIM2, &TIM_OCInitStructure);
    TIM_OC3PreloadConfig(TIM2, TIM_OCPreload_Enable);

    // 右电机：CH4
    TIM_OC4Init(TIM2, &TIM_OCInitStructure);
    TIM_OC4PreloadConfig(TIM2, TIM_OCPreload_Enable);

    TIM_ARRPreloadConfig(TIM2, ENABLE);
    TIM_Cmd(TIM2, ENABLE);
}

// =====================================================
// Motor_Set_Speed：设置某一路电机的速度和方向
// 参数：motor_num = 1(左电机) / 2(右电机)
//       speed     = -100 ~ +100，正反转，0=停止
//
// 说明：为了“强行转起来”，这里不做任何复杂死区判断，
//       只做一个简单的限幅和绝对值。
// =====================================================
void Motor_Set_Speed(uint8_t motor_num, int16_t speed)
{
    // ---------- 1. 做个简单限幅：-100 ~ +100 ----------
    if (speed > 100)  speed = 100;
    if (speed < -100) speed = -100;

    // 对应的 PWM 占空比：0~99
    uint16_t pwm = (speed >= 0) ? speed : -speed;

    // ---------- 2. 按电机编号分别控制 ----------
    if (motor_num == 1)          // 电机1 -> TIM2_CH3, PB12/PB13
    {
				if (speed == 0)
				{
					// 停止：两路方向脚拉低 + PWM=0
					GPIO_ResetBits(GPIOB, GPIO_Pin_12 | GPIO_Pin_13);
					TIM_SetCompare3(TIM2, 0);
				}
				else if (speed > 0)      // 正转
				{
					GPIO_SetBits  (GPIOB, GPIO_Pin_12);   // AIN1 = 1
					GPIO_ResetBits(GPIOB, GPIO_Pin_13);   // AIN2 = 0
					TIM_SetCompare3(TIM2, pwm);           // 占空比 pwm%
				}
				else                     // 反转
				{
					GPIO_ResetBits(GPIOB, GPIO_Pin_12);   // AIN1 = 0
					GPIO_SetBits  (GPIOB, GPIO_Pin_13);   // AIN2 = 1
					TIM_SetCompare3(TIM2, pwm);
				}
			}
		  

				else if (motor_num == 2)     // 电机2 -> TIM2_CH4, PB14/PB15
				{
				if (speed == 0)
				{
					 // 停车：两个方向引脚都拉低，PWM 置 0
					GPIO_ResetBits(GPIOB, GPIO_Pin_14 | GPIO_Pin_15);
					TIM_SetCompare4(TIM2, 0);
				}
				else if (speed > 0)      // “正转”定义成：和电机1同方向向前走
				{
					// 注意：这里跟你原来相反，把 14 置低、15 置高
					GPIO_ResetBits(GPIOB, GPIO_Pin_14);
					GPIO_SetBits  (GPIOB, GPIO_Pin_15);
					TIM_SetCompare4(TIM2, speed);   // 占空比用正值
				}
				else                     // 反转
				{
					// 反向：14 高、15 低（跟上面正转反过来）
					GPIO_SetBits  (GPIOB, GPIO_Pin_14);
					GPIO_ResetBits(GPIOB, GPIO_Pin_15);
					TIM_SetCompare4(TIM2, -speed);  // 占空比取绝对值
				}
			  }
}


void motor(int16_t left_speed, int16_t right_speed)
{
    Motor_Set_Speed(1, left_speed);
    Motor_Set_Speed(2, right_speed);
}

// 预留的处理函数，方便以后在主循环里做一些电机相关的周期性处理
void Motor_Process(void)
{
    // 暂时不需要做什么
}
