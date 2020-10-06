#ifndef _TCPCONSOLE_H_
#define _TCPCONSOLE_H_

#include <stdint.h>

int tcpconsole_init(uint8_t uart, uint16_t port);
void tcpconsole_loop(void);
void tcpconsole_send(uint8_t uart, uint8_t val);

#endif
