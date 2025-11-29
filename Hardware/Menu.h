#ifndef __MENU_H
#define __MENU_H

#include "stm32f10x.h"

typedef enum
{
    SYSTEM_MENU    = 0,   // 处于菜单界面
    SYSTEM_RUNNING = 1,   // 小车正在循迹
    SYSTEM_STOP    = 2    // 已停止
} SystemState_t;

/* 这些变量在 Menu.c 里定义，在其他文件里用 extern 引用 */
extern SystemState_t system_state;      // 当前系统状态
extern uint8_t       car_started;       // 小车是否已经启动
extern uint8_t       launch_confirmed;  // 是否已经发过车（1 表示发过了）

/* 菜单内部状态变量 */
extern uint8_t menu_state;   // 0-主菜单, 2-PID 菜单
extern uint8_t cursor_pos;   // 光标所在行
extern uint8_t edit_mode;    // 0-浏览, 1-编辑

/* 菜单相关接口函数 */
void Show_Main_Menu(void);          // 画主菜单
void Show_PID_Menu(void);           // 画 PID 参数菜单
void Handle_Key(uint8_t key);       // 在主循环里调用，用来响应按键

/* 下面两个函数在 main.c 里实现，这里只做声明，方便菜单调用 */
void    Start_Line_Tracking(void);  // 真正的发车初始化（切换到运行状态）
uint8_t Can_Launch(void);           // 返回 1 表示还没发过车，可以发车

#endif
