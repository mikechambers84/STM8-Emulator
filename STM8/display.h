#ifndef _DISPLAY_H_
#define _DISPLAY_H_
#include <stdint.h>

#ifdef __WIN32__
#include <Windows.h>
extern HANDLE hConsole;
extern uint8_t doupdate;
#elif __unix__
#endif


void cls(void);
void gotoxy(int x, int y);
void display_str(int y, int x, char* str);
int display_init(void);
void display_shutdown(void);

#endif
