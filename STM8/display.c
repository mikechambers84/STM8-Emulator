#include <stdio.h>
#include <conio.h>
#include <stdint.h>
#include <windows.h> 
#include "memory.h"
#include "hardware/casil.h"

HANDLE hConsole;
uint8_t doupdate = 1;

void cls() {
    COORD coordScreen = { 0, 0 };    // home for the cursor 
    DWORD cCharsWritten;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    DWORD dwConSize;

    // Get the number of character cells in the current buffer. 

    if (!GetConsoleScreenBufferInfo(hConsole, &csbi))
    {
        return;
    }

    dwConSize = csbi.dwSize.X * csbi.dwSize.Y;

    // Fill the entire screen with blanks.

    if (!FillConsoleOutputCharacter(hConsole,        // Handle to console screen buffer 
        (TCHAR)' ',     // Character to write to the buffer
        dwConSize,       // Number of cells to write 
        coordScreen,     // Coordinates of first cell 
        &cCharsWritten))// Receive number of characters written
    {
        return;
    }

    // Get the current text attribute.

    if (!GetConsoleScreenBufferInfo(hConsole, &csbi))
    {
        return;
    }

    // Set the buffer's attributes accordingly.

    if (!FillConsoleOutputAttribute(hConsole,         // Handle to console screen buffer 
        csbi.wAttributes, // Character attributes to use
        dwConSize,        // Number of cells to set attribute 
        coordScreen,      // Coordinates of first cell 
        &cCharsWritten)) // Receive number of characters written
    {
        return;
    }

    // Put the cursor at its home coordinates.

    SetConsoleCursorPosition(hConsole, coordScreen);
}

void gotoxy(int x, int y) {
	COORD coord;
	coord.X = x;
	coord.Y = y;
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

void display_str(int x, int y, char* str) {
    SetConsoleTextAttribute(hConsole, 0x07);
    gotoxy(x, y);
    printf(str);
}

void display_init() {
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, 7);
    cls();
}

void display_shutdown() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    SHORT lastrow;
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    lastrow = csbi.srWindow.Bottom - csbi.srWindow.Top;
    SetConsoleTextAttribute(hConsole, 7);
    gotoxy(0, lastrow - 1);
}
