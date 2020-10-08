#ifndef _UART3_H_
#define _UART3_H_

#include <stdint.h>

#define UART3_REDIRECT_NULL			0
#define UART3_REDIRECT_CONSOLE		1
#define UART3_REDIRECT_SERIAL		2
#define UART3_REDIRECT_TCP			3

extern uint8_t uart3_redirect;

uint8_t uart3_read(uint32_t addr);
void uart3_write(uint32_t addr, uint8_t val);
void uart3_rxBufAdd(uint8_t val);
uint8_t uart3_rxBufGet(void);
void uart3_txCallback(void);
void uart3_rxCallback(void);
void uart3_clockrun(int32_t clocks);
void uart3_init();

#endif

