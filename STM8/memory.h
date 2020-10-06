#ifndef _MEMORY_H_
#define _MEMORY_H

#include <stdint.h>
#include "config.h"

#define MEMORY_ACCESS_READ		0
#define MEMORY_ACCESS_WRITE		1
#define MEMORY_ACCESS_BOTH		2

/*typedef struct {
	uint32_t addr;
	uint8_t accesstype;
} MEMDEBUG_t;*/

extern uint8_t *flash;
extern uint8_t *RAM;
extern uint8_t *EEPROM;
extern uint8_t *IO;
extern uint8_t *CPUREG;

extern uint32_t flash_size, ram_size, eeprom_size, io_size, cpureg_size;
extern uint32_t flash_start, ram_start, eeprom_start, io_start, cpureg_start;

extern uint32_t ports_start, uart1_start, uart3_start, tim2_start, adc_start, clk_start;
extern uint32_t ports_end, uart1_end, uart3_end, tim2_end, adc_end, clk_end;

extern uint32_t regaddr[REGISTERS_COUNT];

uint8_t memory_read(uint32_t addr);
uint16_t memory_read16(uint32_t addr);
void memory_write(uint32_t addr, uint8_t val);
void memory_write16(uint32_t addr, uint16_t val);
//int memory_register_watch(uint32_t addr, uint8_t accesstype);

#endif
