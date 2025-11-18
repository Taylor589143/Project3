#include "stm32f10x.h"
#include "OLED.h"
#include "Serial.h"
#include "PID.h"
#include "Key.h"
#include "Timer.h"
#include "Encoder.h"
#include "Motor.h"

// =====================================================
// 全局变量定义
// =====================================================
uint8_t current_mode = 1;     // 当前控制模式：1-速度控制，2-位置跟随
int16_t target_speed = 0;     // 电机目标速度，通过串口设置

int main(void)
{
    // -------------------- 外设初始化 --------------------
    Key_Init();      // 按键初始化
    OLED_Init();     // OLED显示初始化
    Serial_Init();   // 串口初始化
    Timer_Init();    // 定时器初始化（10ms周期）
    Encoder_Init();  // 编码器初始化
    PWM_Init();      // PWM初始化，用于电机控制

    // -------------------- PID参数设置 --------------------
    Speed_PID_SetParams(5.0f, 1.5f, 0.5f);         // 电机速度PID
    Position_PID_SetParams(0.15f, 0.01f, 0.03f);   // 位置PID，低增益减少振动

    // -------------------- 电机停止初始化 --------------------
    GPIO_ResetBits(GPIOB, GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15);
    TIM_SetCompare3(TIM2, 0);  // 电机1PWM置0
    TIM_SetCompare4(TIM2, 0);  // 电机2PWM置0

    // -------------------- OLED显示初始状态 --------------------
    OLED_ShowString(1, 1, "Mode:");
    OLED_ShowNum(1, 6, current_mode, 1);
    OLED_ShowString(2, 1, "Speed Control");

    // =====================================================
    // 主循环
    // =====================================================
    while(1)
    {
        // ---------- 按键检测与模式切换 ----------
        uint8_t key_num = Key_GetNum();
        if(key_num != 0)  // 如果有按键按下
        {
            current_mode = (current_mode == 1) ? 2 : 1;   // 切换模式
            OLED_ShowNum(1, 6, current_mode, 1);         // 更新OLED显示模式

            if(current_mode == 1)
            {
                // 速度控制模式显示
                OLED_ShowString(2, 1, "Speed Control");

                // 停止两个电机
                GPIO_ResetBits(GPIOB, GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15);
                TIM_SetCompare3(TIM2, 0);
                TIM_SetCompare4(TIM2, 0);

                // 重置速度PID
                Speed_PID_Reset();
            }
            else
            {
                // 位置跟随模式显示
                OLED_ShowString(2, 1, "Pos Following");

                // 重置编码器位置
                Encoder_Clear_TotalCount(1);
                Encoder_Clear_TotalCount(2);

                // 重置目标位置变量
                target_position2 = 0;

                // 记录电机1当前位置作为参考
                last_position1 = Encoder_Get_Position(1);
            }
        }
    }
}
