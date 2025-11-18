#include "stm32f10x.h"                  // Device header
#include "OLED.h"
#include "Encoder.h"
#include "Key.h"


uint8_t menu_state = 0;	//0=主菜单，1=LED控制,2=PID,3=Image，4=Angle
uint8_t cursor_pos = 1;	//光标位置
uint8_t edit_mode = 0;	//0=浏览，1=编辑
uint8_t led_speed = 0;	//0=500ms，1=1000ms，2=200ms
uint8_t led_dir = 0;	//0=右，1=左

extern float pid_param[3];

void Show_Main_Menu(void)
{
	OLED_Clear();
	OLED_ShowString(1,2,"LED Control");
	OLED_ShowString(2,2,"PID");
	OLED_ShowString(3,2,"Image");
	OLED_ShowString(4,2,"Angle");
	
	if(cursor_pos == 1)
		OLED_ShowChar(1,1,'>');
	if(cursor_pos == 2)
		OLED_ShowChar(2,1,'>');
	if(cursor_pos == 3)
		OLED_ShowChar(3,1,'>');
	if(cursor_pos == 4)
		OLED_ShowChar(4,1,'>');
}

void Show_LED_Menu(void)
{
	OLED_Clear();
	OLED_ShowString(1,1,"LED Control");
	OLED_ShowString(2,2,"LED_speed");
	OLED_ShowString(3,2,"LED_dir");
	OLED_ShowNum(2,14,led_speed,1);
	OLED_ShowNum(3,14,led_dir,1);
	
	if(edit_mode)
		OLED_ShowChar(1,15,'E');
	
	if(cursor_pos == 2)
		OLED_ShowChar(2,1,'>');
	if(cursor_pos == 3)
		OLED_ShowChar(3,1,'>');
	
}
void Show_Angle_Menu(void)
{
	OLED_Clear();
	OLED_ShowString(1,1,"Angle");
	OLED_ShowString(2,2,"Angle");
	OLED_ShowChar(2,1,'>');
}
void Show_Image_Menu(void)
{
	OLED_Clear();
	OLED_ShowString(1,1,"Image");
	OLED_ShowString(2,2,"Image");
	OLED_ShowChar(2,1,'>');
}
void Show_PID_Menu(void)
{
	OLED_Clear();
	OLED_ShowString(1,1,"PID");
	OLED_ShowString(2,2,"kp");
	OLED_ShowString(3,2,"ki");
	OLED_ShowString(4,2,"kd");
	OLED_ShowFloat(2,13,pid_param[0],1,1);
	OLED_ShowFloat(3,13,pid_param[1],1,1);
	OLED_ShowFloat(4,13,pid_param[2],1,1);
	
	if(edit_mode)
		OLED_ShowChar(1,15,'E');
	
	if(cursor_pos == 2)
		OLED_ShowChar(2,1,'>');
	if(cursor_pos == 3)
		OLED_ShowChar(3,1,'>');
	if(cursor_pos == 4)
		OLED_ShowChar(4,1,'>');
}

void Handle_Key(uint8_t key)
{
	if(key == 0)return;
	
	if(menu_state == 0)
	{
		if(key == 1)
			cursor_pos = (cursor_pos == 1) ? 4 : (cursor_pos - 1);
		else if(key == 2)
			cursor_pos = cursor_pos % 4 + 1;
		else if(key == 3)
		{
			if(cursor_pos == 1)
			{
				menu_state = 1;
			}
			if(cursor_pos == 2)
			{
				menu_state = 2;
			}
			if(cursor_pos == 3)
			{
				menu_state = 3;
			}
			if(cursor_pos == 4)
			{
				menu_state = 4;
			}
			cursor_pos = 2;
			
		}	
			
	}
	else if(menu_state == 1)
			{
				if(edit_mode == 0)	//浏览模式
				{
					if(key == 1||key == 2)
					{
						cursor_pos = (cursor_pos == 2) ? 3 : 2;
					}
					else if(key == 3)
					{
						edit_mode = 1;
					}
					else if(key == 4)
					{
						menu_state = 0;
						cursor_pos = 1;
					}
					
				}
				else	//编辑模式
				{
					if(key == 3)
					{
						edit_mode = 0;
					}
					else if(key == 2||key == 1)
					{
						if(cursor_pos == 2)
						{
							if(key == 1)
								led_speed = (led_speed+1)%3;
							else
								led_speed = (led_speed == 0) ? 2 : led_speed - 1;
						}
						else led_dir = !led_dir;
					}
				}
			}
		else if(menu_state == 2)
		{
				if(edit_mode == 0)	//浏览模式
				{
					if(key == 1)
						cursor_pos = (cursor_pos == 2) ? 4 : (cursor_pos - 1);
					else if(key == 2)
						cursor_pos = (cursor_pos-1) % 3 + 2;
					else if(key == 3)
					{
						edit_mode = 1;
					}
					else if(key == 4)
					{
						menu_state = 0;
						cursor_pos = 1;
					}
					
				}
				else	//编辑模式
				{
					if(key == 3)
					{
						edit_mode = 0;
					}
					else if(key == 1)
					{
						switch (cursor_pos)
						{
							case 2:pid_param[0]+=0.1;break;
							case 3:pid_param[1]+=0.1;break;
							case 4:pid_param[2]+=0.1;break;
						}
					}
					else if(key == 2)
					{
						switch (cursor_pos)
						{
							case 2:pid_param[0]-=0.1;break;
							case 3:pid_param[1]-=0.1;break;
							case 4:pid_param[2]-=0.1;break;
						}
					}
				}
		}
		else if(menu_state == 3)
		{
			if(key == 4)
				menu_state = 0;
				cursor_pos = 1;
		}
		else if(menu_state == 4)
		{
			if(key == 4)
				menu_state = 0;
				cursor_pos = 1;
		}
	switch (menu_state)
	{
	case 0:
	Show_Main_Menu();
	break;
	case 1:
	Show_LED_Menu();
	break;
	case 2:
	Show_PID_Menu();
	break;
	case 3:
	Show_Image_Menu();
	break;
	case 4:
	Show_Angle_Menu();
	break;
    }

}
