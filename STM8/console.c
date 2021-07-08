#include <stdio.h>
#include <stdint.h>
#include "console.h"

#ifdef __WIN32__
#include <conio.h>
#include <Windows.h>
#endif

#ifdef linux

#endif

void console_init(char* title) {
	#ifdef __WIN32__	
	SetConsoleTitleA(title);
	#endif
}
