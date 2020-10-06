#ifndef _CASIL_H_
#define _CASIL_H_

#include <stdint.h>

void casil_init();
void casil_rs(uint8_t val);
void casil_rw(uint8_t val);
void casil_en(uint8_t val);
void casil_latchwrite(uint8_t val);
uint8_t casil_latchread();

extern uint8_t disp[16];

#endif
