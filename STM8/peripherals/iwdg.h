#ifndef _IWDG_H_
#define _IWDG_H_

#include <stdint.h>

void iwdg_write(uint32_t addr, uint8_t val);
uint8_t iwdg_read(uint32_t addr);
void iwdg_init();

#endif
