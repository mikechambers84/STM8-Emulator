#include <stdio.h>
#include <conio.h>
#include <stdint.h>
#include <Windows.h>
#include "console.h"

void console_init(char* title) {
	SetConsoleTitleA(title);

}
