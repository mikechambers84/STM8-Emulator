#ifndef _NULL_H_
#define _NULL_H_

#include <stdint.h>

void null_register(void);
void null_init(void);
void null_update(void);
void null_portwrite(uint32_t addr, uint8_t val);
uint8_t null_portread(uint32_t addr);
void null_markforupdate(void);
void null_loop(void);

#endif
