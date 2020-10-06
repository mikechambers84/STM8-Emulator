#ifndef _TIM2_H_
#define _TIM2_H_

#include <stdint.h>

void tim2_init();
void tim2_clockrun(uint16_t clocks);
void tim2_write(uint32_t addr, uint8_t val);
uint8_t tim2_read(uint32_t addr);

#endif
