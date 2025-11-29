#include "stm32f10x.h"
#include "OLED.h"
#include "Serial.h"
#include "PID.h"
#include "Key.h"
#include "Encoder.h"
#include "Motor.h"
#include "Sensor.h"
#include "Menu.h"
#include "line.h"
#include "Delay.h"
#include <stdio.h>

/*------------------------------------------------
 *  全局变量（给其他模块用）
 *------------------------------------------------*/
uint8_t current_mode  = 1;   // 当前控制模式：1=速度模式（本项目一直用 1）
int16_t target_speed  = 0;   // 预留给串口调试用（比如发速度指令）

/* 供 Menu.c 调用的三个函数，在 Menu.h 里有声明 */
void Start_Line_Tracking(void);
void Stop_Line_Tracking(void);
uint8_t Can_Launch(void);

//主函数

int main(void)
{

    Serial_Init();     // 串口：放最前，后面的 printf 都能用
    OLED_Init();       // OLED 显示
    Key_Init();        // 按键

    PWM_Init();        // 电机 PWM + 方向控制
    Motor_Set_Speed(1, 0);   // 上电先保证电机不转
    Motor_Set_Speed(2, 0);

    Encoder_Init();    // 编码器
    Sensor_Init();     // 红外传感器（里面有 printf 调试信息）

    // 你也可以在这里手动给 line_pid 一个初始值，
    // 也可以用 PID.c 里默认的：
    // line_pid.kp = 6.0f;
    // line_pid.ki = 0.2f;
    // line_pid.kd = 2.0f;

    /*-----------初始化系统状态 -----------*/
    system_state      = SYSTEM_MENU;  // 一开始在菜单界面
    car_started       = 0;
    launch_confirmed  = 0;
    current_mode      = 1;
    target_speed      = 0;

    /* 显示主菜单：LAUNCH / PID */
    Show_Main_Menu();

    /*================================================
     * 3. 主循环
     *================================================*/
    while (1)
    {
        /* 3.1 按键扫描（Key_Scan 已经带消抖 + 长按） */
        uint8_t key = Key_Scan();   // 1=UP, 2=DOWN, 3=OK, 4=BACK

        if (key != 0)
        {
            switch (system_state)
            {
                case SYSTEM_MENU:
                    /* 菜单状态：所有按键交给菜单模块处理
                     * Handle_Key 里面会根据 key：
                     *  - 在主菜单移动光标/选择 LAUNCH 或 PID
                     *  - 在 PID 菜单里编辑 kp/ki/kd
                     *  - 需要发车时会调用 Start_Line_Tracking()
                     */
                    Handle_Key(key);
                    break;

                case SYSTEM_RUNNING:
                    /* 运行状态：目前只用 BACK 键停车 */
                    if (key == KEY_BACK || key == 4)
                    {
                        Stop_Line_Tracking();
                    }
                    break;

                case SYSTEM_STOP:
                    /* 停止状态：按键暂时不再起作用，需要重新上电 */
                    break;
            }
        }

        /* 3.2 根据系统状态执行不同任务 */
        switch (system_state)
        {
            case SYSTEM_MENU:
                /* 菜单界面的 OLED 刷新都在 Menu.c 里做了，
                 * 这里不需要再画东西。*/
                break;

            case SYSTEM_RUNNING:
                if (car_started)
                {
                    /* 真正的循迹逻辑：
                     *  - 内部会调用 Get_Line_Status / Detect_Crossroad
                     *  - 小偏差用 PID 精调，大偏差用状态机调整
                     *  - 最终调用 motor()/Motor_Set_Speed 控制电机
                     */
                    Advanced_Tracking();
                }
                else
                {
                    motor(0, 0);
                }
                break;

            case SYSTEM_STOP:
                /* 停止状态：保险起见，持续置零 */
                Motor_Set_Speed(1, 0);
                Motor_Set_Speed(2, 0);
                break;
        }

        /* 3.3 主循环大约 10ms 一次，对应 Key_Scan / PID 的节奏 */
        Delay_ms(10);
    }
}

/*====================================================
 *  发车 / 停车 / 是否允许发车
 *===================================================*/

/**
 * @brief  开始循迹（在菜单中选择 LAUNCH -> OK 时被调用）
 */
void Start_Line_Tracking(void)
{
    /* 已经发车过就不再做任何事（只允许发一次） */
    if (launch_confirmed)
    {
        return;
    }

    system_state      = SYSTEM_RUNNING;  // 切到运行状态
    car_started       = 1;
    launch_confirmed  = 1;               // 标记“已经发车”

    /* 清屏，给个简单提示 */
    OLED_Clear();
    OLED_ShowString(1, 1, "RUN MODE");

    /* 初始化循迹系统：清路口计数、PID 累积误差等 */
    Track_Init();

    /* 编码器累计位置清零（即使暂时不用，也可以留着） */
    Encoder_Clear_TotalCount(1);         // 左轮
    Encoder_Clear_TotalCount(2);         // 右轮

    printf(">>> Line tracking started.\r\n");
}

/**
 * @brief  停止循迹（运行状态下按 BACK 调用）
 */
void Stop_Line_Tracking(void)
{
    system_state = SYSTEM_STOP;
    car_started  = 0;

    /* 马上停掉两路电机 */
    Motor_Set_Speed(1, 0);
    Motor_Set_Speed(2, 0);

    OLED_Clear();
    OLED_ShowString(1, 1, "STOPPED");

    printf(">>> Line tracking stopped.\r\n");
}

/**
 * @brief  是否允许发车（菜单中用来判断 LAUNCH 是否可用）
 * @return 1: 允许发车；0: 已经发过车
 */
uint8_t Can_Launch(void)
{
    return (launch_confirmed == 0) ? 1 : 0;
}



