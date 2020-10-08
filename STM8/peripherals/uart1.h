#ifndef _UART1_H_
#define _UART1_H_

#include <stdint.h>

#define UART1_REDIRECT_NULL			0
#define UART1_REDIRECT_CONSOLE		1
#define UART1_REDIRECT_SERIAL		2
#define UART1_REDIRECT_TCP			3

extern uint8_t uart1_redirect;

uint8_t uart1_read(uint32_t addr);
void uart1_write(uint32_t addr, uint8_t val);
void uart1_rxBufAdd(uint8_t val);
uint8_t uart1_rxBufGet(void);
void uart1_txCallback(void);
void uart1_rxCallback(void);
void uart1_clockrun(int32_t clocks);
void uart1_init();

#endif

