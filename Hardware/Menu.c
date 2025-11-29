#include "stm32f10x.h"   // Device header
#include "OLED.h"
#include "PID.h"
#include "Key.h"
#include "Menu.h"
#include "line.h"        // 为了访问 STRAIGHT_SPEED / CORNER_SPEED_MIN / SPEED_K

/*------------------------------------------------
 *  全局状态变量（在 Menu.c 里真正定义）
 *  其他文件通过 Menu.h 里的 extern 使用
 *------------------------------------------------*/
SystemState_t system_state    = SYSTEM_MENU;   // 系统当前大状态
uint8_t       car_started     = 0;            // 小车是否已经启动
uint8_t       launch_confirmed = 0;           // 这个变量现在不再用来锁死菜单

uint8_t menu_state  = 0;    // 0: 主菜单, 2: PID 菜单, 3: SPEED 菜单
uint8_t cursor_pos  = 1;    // 光标所在的行
uint8_t edit_mode   = 0;    // 0: 浏览模式, 1: 编辑模式

/*------------------------------------------------
 *  内部函数声明（只在本文件用，static）
 *------------------------------------------------*/
static void Menu_DrawMain(void);
static void Menu_DrawPID(void);
static void Menu_DrawSpeed(void);

/*------------------------------------------------
 *  主菜单绘制（内部函数）
 *------------------------------------------------*/
static void Menu_DrawMain(void)
{
    OLED_Clear();

    // 主菜单三项
    OLED_ShowString(1, 2, "LAUNCH");
    OLED_ShowString(2, 2, "PID");
    OLED_ShowString(3, 2, "SPEED");

    // 先清光标列
    OLED_ShowChar(1, 1, ' ');
    OLED_ShowChar(2, 1, ' ');
    OLED_ShowChar(3, 1, ' ');

    // 根据 cursor_pos 画光标
    if (cursor_pos == 1)
    {
        OLED_ShowChar(1, 1, '>');
    }
    else if (cursor_pos == 2)
    {
        OLED_ShowChar(2, 1, '>');
    }
    else if (cursor_pos == 3)
    {
        OLED_ShowChar(3, 1, '>');
    }
}

/*------------------------------------------------
 *  PID 参数菜单绘制（内部函数）
 *------------------------------------------------*/
static void Menu_DrawPID(void)
{
    OLED_Clear();

    OLED_ShowString(1, 1, " PID Setting ");
    OLED_ShowString(2, 2, "Kp:");
    OLED_ShowString(3, 2, "Ki:");
    OLED_ShowString(4, 2, "Kd:");

    // 显示当前 PID 参数
    OLED_ShowFloat(2, 8, line_pid.kp, 2, 1);   // xx.x
    OLED_ShowFloat(3, 8, line_pid.ki, 1, 2);   // 0.xx
    OLED_ShowFloat(4, 8, line_pid.kd, 2, 1);

    /* 如果正在编辑，右上角显示一个 'E' 提示 */
    if (edit_mode)
    {
        OLED_ShowChar(1, 15, 'E');
    }
    else
    {
        OLED_ShowChar(1, 15, ' ');
    }

    // 清空光标列
    OLED_ShowChar(2, 1, ' ');
    OLED_ShowChar(3, 1, ' ');
    OLED_ShowChar(4, 1, ' ');

    /* 根据当前光标位置，在对应行前画 '>' */
    if (cursor_pos == 2)
    {
        OLED_ShowChar(2, 1, '>');
    }
    else if (cursor_pos == 3)
    {
        OLED_ShowChar(3, 1, '>');
    }
    else if (cursor_pos == 4)
    {
        OLED_ShowChar(4, 1, '>');
    }
}

/*------------------------------------------------
 *  SPEED 菜单绘制：直线速度 + 弯道最低速度 + SPEED_K
 *------------------------------------------------*/
static void Menu_DrawSpeed(void)
{
    OLED_Clear();

    OLED_ShowString(1, 1, " Speed Setting ");
    OLED_ShowString(2, 2, "Str:");   // STRAIGHT_SPEED
    OLED_ShowString(3, 2, "Cor:");   // CORNER_SPEED_MIN
    OLED_ShowString(4, 2, " K :");   // SPEED_K

    // 显示当前速度参数
    OLED_ShowNum  (2, 8, STRAIGHT_SPEED,    3);  // 3 位数字
    OLED_ShowNum  (3, 8, CORNER_SPEED_MIN,  3);
    OLED_ShowFloat(4, 8, SPEED_K, 2, 1);         // xx.x

    if (edit_mode)
    {
        OLED_ShowChar(1, 15, 'E');
    }
    else
    {
        OLED_ShowChar(1, 15, ' ');
    }

    // 清光标列
    OLED_ShowChar(2, 1, ' ');
    OLED_ShowChar(3, 1, ' ');
    OLED_ShowChar(4, 1, ' ');

    if (cursor_pos == 2)
    {
        OLED_ShowChar(2, 1, '>');
    }
    else if (cursor_pos == 3)
    {
        OLED_ShowChar(3, 1, '>');
    }
    else if (cursor_pos == 4)
    {
        OLED_ShowChar(4, 1, '>');
    }
}

/*------------------------------------------------
 *  对外接口：显示主菜单
 *------------------------------------------------*/
void Show_Main_Menu(void)
{
    menu_state = 0;      // 切回主菜单状态
    edit_mode  = 0;      // 确保不在编辑模式

    if (cursor_pos < 1 || cursor_pos > 3)
    {
        cursor_pos = 1;
    }

    Menu_DrawMain();
}

/*------------------------------------------------
 *  对外接口：显示 PID 菜单
 *------------------------------------------------*/
void Show_PID_Menu(void)
{
    menu_state = 2;      // PID 菜单状态

    /* 在 PID 菜单里，光标有效行是 2~4 */
    if (cursor_pos < 2 || cursor_pos > 4)
    {
        cursor_pos = 2;
    }

    Menu_DrawPID();
}

/*------------------------------------------------
 *  对外接口：显示 SPEED 菜单
 *------------------------------------------------*/
void Show_Speed_Menu(void)
{
    menu_state = 3;      // SPEED 菜单

    if (cursor_pos < 2 || cursor_pos > 4)
    {
        cursor_pos = 2;
    }

    Menu_DrawSpeed();
}

/*------------------------------------------------
 *  核心函数：按键处理
 *------------------------------------------------*/
void Handle_Key(uint8_t key)
{
    if (key == 0)
    {
        return;
    }

    switch (menu_state)
    {
    /* ====================== 主菜单 ====================== */
    case 0:
        if (key == KEY_UP)
        {
            // 在 1~3 之间循环
            if (cursor_pos == 1) cursor_pos = 3;
            else cursor_pos--;
            Menu_DrawMain();
        }
        else if (key == KEY_DOWN)
        {
            if (cursor_pos == 3) cursor_pos = 1;
            else cursor_pos++;
            Menu_DrawMain();
        }
        else if (key == KEY_OK)
        {
            if (cursor_pos == 1)       // LAUNCH
            {
                Start_Line_Tracking();
            }
            else if (cursor_pos == 2)  // PID
            {
                edit_mode  = 0;
                cursor_pos = 2;
                Show_PID_Menu();
            }
            else if (cursor_pos == 3)  // SPEED
            {
                edit_mode  = 0;
                cursor_pos = 2;
                Show_Speed_Menu();
            }
        }
        break;

    /* ==================== PID 参数菜单 =================== */
    case 2:
        if (edit_mode == 0)        // 浏览模式
        {
            if (key == KEY_UP)
            {
                // 2 <-> 4 <-> 3 <-> 2 ...
                if (cursor_pos == 2)      cursor_pos = 4;
                else                      cursor_pos--;
                Menu_DrawPID();
            }
            else if (key == KEY_DOWN)
            {
                if (cursor_pos == 4)      cursor_pos = 2;
                else                      cursor_pos++;
                Menu_DrawPID();
            }
            else if (key == KEY_OK)
            {
                edit_mode = 1;
                Menu_DrawPID();
            }
            else if (key == KEY_BACK)
            {
                menu_state = 0;
                cursor_pos = 1;
                edit_mode  = 0;
                Show_Main_Menu();
            }
        }
        else                        // 编辑模式
        {
            if (key == KEY_OK)
            {
                edit_mode = 0;
                Menu_DrawPID();
            }
            else if (key == KEY_UP || key == KEY_DOWN)
            {
                float dir = (key == KEY_UP) ? (+1.0f) : (-1.0f);

                const float step_kp = 0.1f;
                const float step_ki = 0.01f;
                const float step_kd = 0.1f;

                switch (cursor_pos)
                {
                case 2:  // Kp
                    line_pid.kp += dir * step_kp;
                    if (line_pid.kp < 0.0f)  line_pid.kp = 0.0f;
                    if (line_pid.kp > 20.0f) line_pid.kp = 20.0f;
                    break;

                case 3:  // Ki
                    line_pid.ki += dir * step_ki;
                    if (line_pid.ki < 0.0f)  line_pid.ki = 0.0f;
                    if (line_pid.ki >  2.0f) line_pid.ki =  2.0f;
                    break;

                case 4:  // Kd
                    line_pid.kd += dir * step_kd;
                    if (line_pid.kd < 0.0f)  line_pid.kd = 0.0f;
                    if (line_pid.kd > 20.0f) line_pid.kd = 20.0f;
                    break;

                default:
                    break;
                }

                Menu_DrawPID();
            }
        }
        break;

    /* ==================== SPEED 菜单 =================== */
    case 3:
        if (edit_mode == 0)      // 浏览模式：移动光标或返回
        {
            if (key == KEY_UP)
            {
                // 在 2~4 之间循环：2->4->3->2...
                if (cursor_pos == 2)      cursor_pos = 4;
                else                      cursor_pos--;
                Menu_DrawSpeed();
            }
            else if (key == KEY_DOWN)
            {
                if (cursor_pos == 4)      cursor_pos = 2;
                else                      cursor_pos++;
                Menu_DrawSpeed();
            }
            else if (key == KEY_OK)
            {
                edit_mode = 1;
                Menu_DrawSpeed();
            }
            else if (key == KEY_BACK)
            {
                menu_state = 0;
                cursor_pos = 1;
                edit_mode  = 0;
                Show_Main_Menu();
            }
        }
        else                    // 编辑模式：调速度 & SPEED_K
        {
            if (key == KEY_OK)
            {
                edit_mode = 0;
                Menu_DrawSpeed();
            }
            else if (key == KEY_UP || key == KEY_DOWN)
            {
                float fdir = (key == KEY_UP) ? (+1.0f) : (-1.0f);
                int   idir = (key == KEY_UP) ? (+1)    : (-1);

                switch (cursor_pos)
                {
                case 2:  // 直线速度 STRAIGHT_SPEED
                    STRAIGHT_SPEED += idir * 5;     // 每次调 5
                    if (STRAIGHT_SPEED < 20)  STRAIGHT_SPEED = 20;
                    if (STRAIGHT_SPEED > 150) STRAIGHT_SPEED = 150;
                    // 确保弯道最低速度不大于直线速度
                    if (CORNER_SPEED_MIN > STRAIGHT_SPEED)
                        CORNER_SPEED_MIN = STRAIGHT_SPEED;
                    break;

                case 3:  // 弯道最低速度 CORNER_SPEED_MIN
                    CORNER_SPEED_MIN += idir * 2;   // 每次调 2
                    if (CORNER_SPEED_MIN < 10)  CORNER_SPEED_MIN = 10;
                    if (CORNER_SPEED_MIN > STRAIGHT_SPEED)
                        CORNER_SPEED_MIN = STRAIGHT_SPEED;
                    break;

                case 4:  // SPEED_K （弯道降速系数）
                    SPEED_K += fdir * 0.2f;         // 每次调 0.2
                    if (SPEED_K < 1.0f)  SPEED_K = 1.0f;
                    if (SPEED_K > 15.0f) SPEED_K = 15.0f;
                    break;

                default:
                    break;
                }

                Menu_DrawSpeed();
            }
        }
        break;

    default:
        menu_state = 0;
        Show_Main_Menu();
        break;
    }
}
