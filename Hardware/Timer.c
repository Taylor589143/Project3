#include "stm32f10x.h"
#include "Encoder.h"
#include "Serial.h"
#include "PID.h"
#include "Motor.h"
#include <stdlib.h>

extern uint8_t current_mode;     // 当前控制模式：1-速度，2-位置
extern int16_t target_speed;     // 电机目标速度~~


void Timer_Init(void)
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    TIM_InternalClockConfig(TIM2);

    TIM_TimeBaseInitTypeDef TIM_BaseInitStruct;
    TIM_BaseInitStruct.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_BaseInitStruct.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_BaseInitStruct.TIM_Period = 1000 - 1;   // 10ms中断
    TIM_BaseInitStruct.TIM_Prescaler = 720 - 1; // 计数频率 10kHz
    TIM_BaseInitStruct.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM2, &TIM_BaseInitStruct);

    TIM_ClearFlag(TIM2, TIM_FLAG_Update);
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

    // 配置NVIC优先级
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = TIM2_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 2; // ★修改：降低抢占优先级
    NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0;
    NVIC_Init(&NVIC_InitStruct);

    TIM_Cmd(TIM2, ENABLE);
}


void TIM2_IRQHandler(void)
{
    if(TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)
    {
        //读取编码器数据
        int16_t speed1 = Encoder_Get_Speed(1);
        int16_t speed2 = Encoder_Get_Speed(2);
        int32_t pos1 = Encoder_Get_Position(1);
        int32_t pos2 = Encoder_Get_Position(2);

        // 模式1：速度控制
        if(current_mode == 1)
        {
            int16_t adjusted_target = target_speed; // 目标速度补偿

            // 静摩擦补偿
            if(abs(target_speed) > 5)
            {
                adjusted_target += (target_speed > 0) ? 2 : -2;
            }

            int16_t pwm1 = Speed_PID_Compute(adjusted_target, speed1); // PID计算
            Motor_Set_Speed(1, pwm1);
        }
        //模式2：位置跟随
        else
        {
            static uint8_t pulse_active = 0;  // 脉冲激活标志
            static uint8_t pulse_count = 0;   // 脉冲计数器

            int32_t pos_error = pos1 - pos2;

            if(!pulse_active && abs(pos_error) > 100)
            {
                int16_t correction_pwm = (pos_error > 0) ? 120 : -120;
                Motor_Set_Speed(2, correction_pwm); // 电机2脉冲修正
                pulse_active = 1;
                pulse_count = 0;
            }
            else if(pulse_active)
            {
                pulse_count++;
                if(pulse_count > 3) // 30ms后停止
                {
                    Motor_Set_Speed(2, 0);
                    pulse_active = 0;
                }
            }
            else
            {
                Motor_Set_Speed(2, 0); // 微小误差保持停止
            }

            last_position1 = pos1;  // 更新上次位置
            Motor_Set_Speed(1, 0);  // 位置模式下电机1自由转动
        }

        //发送数据到上位机
        static uint8_t send_counter = 0;
        if(send_counter++ >= 2) // 每20ms发送一次
        {
            if(USART_GetFlagStatus(USART1, USART_FLAG_TC))
            {
                USART_Send_Data(speed1, target_speed);
            }
            send_counter = 0;
        }

        TIM_ClearITPendingBit(TIM2, TIM_IT_Update); // 清除中断标志
    }
}
