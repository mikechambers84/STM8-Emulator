#include <stdio.h>
#include <stdint.h>
#include "memory.h"
#include "peripherals/uart1.h"
#include "peripherals/uart3.h"
#include "peripherals/tim2.h"
#include "peripherals/adc.h"
#include "peripherals/clk.h"
#include "peripherals/iwdg.h"
#include "ports.h"
#include "cpu.h"

uint8_t* flash = NULL;
uint8_t* RAM = NULL;
uint8_t* EEPROM = NULL;
uint8_t* IO = NULL;
uint8_t* CPUREG = NULL;

uint32_t regaddr[REGISTERS_COUNT];

uint32_t flash_size = 0, ram_size = 0, eeprom_size = 0, io_size = 0, cpureg_size = 0;
uint32_t flash_start = 0, ram_start = 0, eeprom_start = 0, io_start = 0, cpureg_start = 0;

uint32_t ports_start = 0, uart1_start = 0, uart3_start = 0, tim2_start = 0, adc_start = 0, clk_start = 0;
uint32_t ports_end = 0, uart1_end = 0, uart3_end = 0, tim2_end = 0, adc_end = 0, clk_end = 0;

uint32_t iwdg_start = 0;
uint32_t iwdg_end = 0;

//MEMDEBUG_t mem_watch[32];
//uint8_t mem_watchnum = 0;

void memory_cpuregwrite(uint32_t addr, uint8_t val) {
	if (addr == regaddr[CPUR_A])
		a = val;
	else if (addr == regaddr[CPUR_PCE])
		pc = (pc & 0x0000FFFF) | ((uint32_t)val << 16);
	else if (addr == regaddr[CPUR_PCH])
		pc = (pc & 0x00FF00FF) | ((uint32_t)val << 8);
	else if (addr == regaddr[CPUR_PCL])
		pc = (pc & 0x00FFFF00) | (uint32_t)val;
	else if (addr == regaddr[CPUR_XH])
		x = (x & 0x00FF) | ((uint16_t)val << 8);
	else if (addr == regaddr[CPUR_XL])
		x = (x & 0xFF00) | (uint16_t)val;
	else if (addr == regaddr[CPUR_YH])
		y = (y & 0x00FF) | ((uint16_t)val << 8);
	else if (addr == regaddr[CPUR_YL])
		y = (y & 0xFF00) | (uint16_t)val;
	else if (addr == regaddr[CPUR_SPH])
		sp = (sp & 0x00FF) | ((uint16_t)val << 8);
	else if (addr == regaddr[CPUR_SPL])
		sp = (sp & 0xFF00) | (uint16_t)val;
	else if (addr == regaddr[CPUR_CCR])
		cc = val;
	else
		CPUREG[addr - cpureg_start] = val;
}

uint8_t memory_cpuregread(uint32_t addr) {
	uint8_t ret;
	if (addr == regaddr[CPUR_A])
		ret = a;
	else if (addr == regaddr[CPUR_PCE])
		ret = (uint8_t)(pc >> 16);
	else if (addr == regaddr[CPUR_PCH])
		ret = (uint8_t)(pc >> 8);
	else if (addr == regaddr[CPUR_PCL])
		ret = (uint8_t)pc;
	else if (addr == regaddr[CPUR_XH])
		ret = (uint8_t)(x >> 8);
	else if (addr == regaddr[CPUR_XL])
		ret = (uint8_t)x;
	else if (addr == regaddr[CPUR_YH])
		ret = (uint8_t)(y >> 8);
	else if (addr == regaddr[CPUR_YL])
		ret = (uint8_t)y;
	else if (addr == regaddr[CPUR_SPH])
		ret = (uint8_t)(sp >> 8);
	else if (addr == regaddr[CPUR_SPL])
		ret = (uint8_t)sp;
	else if (addr == regaddr[CPUR_CCR])
		ret = cc;
	else
		ret = CPUREG[addr - cpureg_start];
	return ret;
}

uint8_t memory_read(uint32_t addr) {
	uint8_t ret = 0xFF;

	if ((addr >= flash_start) && (addr < (flash_start + flash_size))) {
		ret = flash[addr - flash_start];
	}
	else if ((addr >= ram_start) && (addr < (ram_start + ram_size))) {
		ret = RAM[addr - ram_start];
	}
	else if ((addr >= eeprom_start) && (addr < (eeprom_start + eeprom_size))) {
		ret = EEPROM[addr - eeprom_start];
		//fprintf(stderr, "%04X = %02X\n", addr, ret);
	}
	else if ((addr >= io_start) && (addr < (io_start + io_size))) {
		//printf("Port read 0x%04X\n", addr);
		if ((addr >= ports_start) && (addr <= ports_end)) {
			if (product_portread(addr, &ret) == 0) {
				ret = ports_read(addr); //only do this if the product's read function didn't give us an overriding value
			}
		}
		else if ((addr >= uart1_start) && (addr <= uart1_end)) {
			ret = uart1_read(addr);
		}
		else if ((addr >= uart3_start) && (addr <= uart3_end)) {
			ret = uart3_read(addr);
		}
		else if ((addr >= tim2_start) && (addr <= tim2_end)) {
			ret = tim2_read(addr);
		}
		else if ((addr >= adc_start) && (addr <= adc_end)) {
			ret = adc_read(addr);
		}
		else if ((addr >= clk_start) && (addr <= clk_end)) {
			ret = clk_read(addr);
		}
		else if ((addr >= iwdg_start) && (addr <= iwdg_end)) {
			ret = iwdg_read(addr);
		}
		else
			ret = IO[addr - io_start];
	}
	else if ((addr >= cpureg_start) && (addr < (cpureg_start + cpureg_size))) {
		ret = memory_cpuregread(addr);
	}
	/*#ifdef DEBUG_OUTPUT
		printf("memory_read(0x%08X) = %02X\n", addr, ret);
	#endif*/
	return ret;
}

uint16_t memory_read16(uint32_t addr) {
	uint16_t ret;
	ret = ((uint16_t)memory_read(addr) << 8) | memory_read(addr + 1);
	//printf("memory_read16(0x%08X) = %04X\n", addr, ret);
	return ret;
}

void memory_write(uint32_t addr, uint8_t val) {
	/*#ifdef DEBUG_OUTPUT
		printf("memory_write(0x%08X, 0x%02X)\n", addr, val);
	#endif*/

	/*if ((addr >= 0x14F) && (addr < 0x184)) {
		fprintf(stderr, "Addr %08X write <- %02X (%u) @ PC %08X\n", addr, val, val, pc);
	}*/
	//ports and registers

	if ((addr >= flash_start) && (addr < (flash_start + flash_size))) {
		flash[addr - flash_start] = val;
		return;
	}
	if ((addr >= ram_start) && (addr < (ram_start + ram_size))) {
		RAM[addr - ram_start] = val;
		return;
	}
	if ((addr >= eeprom_start) && (addr < (eeprom_start + eeprom_size))) {
		EEPROM[addr - eeprom_start] = val;
		return;
	}
	if ((addr >= io_start) && (addr < (io_start + io_size))) {
		IO[addr - io_start] = val;
		//printf("Port write %04X <- %02X\n", addr, val);
		product_portwrite(addr, val);

		//if ((addr >= ports_start) && (addr <= ports_end)) {
			//ports_write(addr, val);
		//}
		if ((addr >= uart1_start) && (addr <= uart1_end)) {
			uart1_write(addr, val);
		}
		else if ((addr >= uart3_start) && (addr <= uart3_end)) {
			uart3_write(addr, val);
		}
		else if ((addr >= tim2_start) && (addr <= tim2_end)) {
			tim2_write(addr, val);
		}
		else if ((addr >= adc_start) && (addr <= adc_end)) {
			adc_write(addr, val);
		}
		else if ((addr >= clk_start) && (addr <= clk_end)) {
			clk_write(addr, val);
		}
		else if ((addr >= iwdg_start) && (addr <= iwdg_end)) {
			iwdg_write(addr, val);
		}
		return;
	}
	if ((addr >= cpureg_start) && (addr < (cpureg_start + cpureg_size))) {
		memory_cpuregwrite(addr, val);
		return;
	}
}

void memory_write16(uint32_t addr, uint16_t val) {
	memory_write(addr, (uint8_t)(val >> 8));
	memory_write(addr + 1, (uint8_t)val);
}

//TODO: Implement memory watch debugging
/*int memory_register_watch(uint32_t addr, uint8_t accesstype) {
	return 0;
}*/
