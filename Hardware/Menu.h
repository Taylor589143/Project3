#ifndef __MENU_H
#define __MENU_H

void Show_Main_Menu(void);
void Show_PID_Menu(void);
void Handle_Key(uint8_t key);

extern uint8_t menu_state;
extern uint8_t edit_mode;

#endif
