#include <stdio.h>
#include <stdint.h>
#include <Windows.h>
#include "peripherals/uart1.h"
#include "peripherals/uart3.h"

HANDLE serial_hComm[3] = {
    INVALID_HANDLE_VALUE,
    INVALID_HANDLE_VALUE,
    INVALID_HANDLE_VALUE
};

int serial_init(int uart, int comnum, int baud) {
    DCB dcb;
    char dcbstr[16];
    char comport[16];

    sprintf(comport, "COM%d", comnum);
    serial_hComm[uart] = CreateFileA(comport,
        GENERIC_READ | GENERIC_WRITE,
        0,
        0,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,
        0);
    if (serial_hComm[uart] == INVALID_HANDLE_VALUE)
        return -1;

    if (!GetCommState(serial_hComm[uart], &dcb))
        return -1;

    FillMemory(&dcb, sizeof(dcb), 0);
    dcb.DCBlength = sizeof(dcb);
    sprintf(dcbstr, "%d,n,8,1", baud);
    if (!BuildCommDCBA(dcbstr, &dcb))
        return -1;

    if (!SetCommState(serial_hComm[uart], &dcb))
        return -1;

    return 0;
}

int serial_read(int uart, char* dst) {
    DWORD didread = 0;
    OVERLAPPED osReader = { 0 };

    if (serial_hComm[uart] == INVALID_HANDLE_VALUE) return -1;

    if (!ReadFile(serial_hComm[uart], dst, 1, &didread, &osReader))
        return -1;
    if (didread != 1)
        return -1;

    return 0;
}

void serial_write(int uart, char* src) {
    DWORD didwrite = 0;
    OVERLAPPED osWriter = { 0 };

    if (serial_hComm[uart] == INVALID_HANDLE_VALUE) return;

    WriteFile(serial_hComm[uart], src, 1, &didwrite, &osWriter);
}

void serial_loop() {
    char newbuf;

    if (uart1_redirect == UART1_REDIRECT_SERIAL) {
        if (serial_read(0, &newbuf) == 0) {
            uart1_rxBufAdd((uint8_t)newbuf);
        }
    }

    if (uart3_redirect == UART3_REDIRECT_SERIAL) {
        if (serial_read(2, &newbuf) == 0) {
            uart3_rxBufAdd((uint8_t)newbuf);
        }
    }
}
