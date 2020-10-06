#ifndef _PORTS_H_
#define _PORTS_H_

#include <stdint.h>

extern uint8_t use_PA;
extern uint8_t use_PB;
extern uint8_t use_PC;
extern uint8_t use_PD;
extern uint8_t use_PE;
extern uint8_t use_PF;
extern uint8_t use_PG;
extern uint8_t use_PH;
extern uint8_t use_PI;

void ports_init();
uint8_t ports_read(uint32_t addr);
void ports_write(uint32_t addr, uint8_t val);
void ports_extmodify(uint32_t addr, uint8_t val);

#endif
