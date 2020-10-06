#ifndef _SERIAL_H_
#define _SERIAL_H_

#include <stdint.h>

int serial_init(int uart, int comnum, int baud);
int serial_read(int uart, char* dst);
void serial_write(int uart, char* src);
void serial_loop(void);

#endif
