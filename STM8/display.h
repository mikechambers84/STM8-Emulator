#ifndef _DISPLAY_H_
#define _DISPLAY_H_

#include <Windows.h>

extern HANDLE hConsole;
extern uint8_t doupdate;

void cls(void);
void gotoxy(int x, int y);
void display_str(int y, int x, char* str);
void display_init(void);
void display_shutdown(void);

#endif
