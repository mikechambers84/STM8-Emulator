#include <stdio.h>
#include <stdint.h>
#include "config.h"
#include "memory.h"
#include "hardware/casil.h"
#include "display.h"
#include "cpu.h"

uint8_t use_PA = 0;
uint8_t use_PB = 0;
uint8_t use_PC = 0;
uint8_t use_PD = 0;
uint8_t use_PE = 0;
uint8_t use_PF = 0;
uint8_t use_PG = 0;
uint8_t use_PH = 0;
uint8_t use_PI = 0;

void ports_init() {
	uint32_t i;
	for (i = ports_start; i <= ports_end; i++) {
		IO[i - io_start] = 0;
	}
}

uint8_t ports_read(uint32_t addr) {
	uint8_t ret;

	if ((addr == regaddr[PA_IDR]) && (use_PA))
		ret = (IO[regaddr[PA_DDR] - io_start] & IO[regaddr[PA_ODR] - io_start]) | (~IO[regaddr[PA_DDR] - io_start] & IO[regaddr[PA_IDR] - io_start]);
	else if ((addr == regaddr[PB_IDR]) && (use_PB))
		ret = (IO[regaddr[PB_DDR] - io_start] & IO[regaddr[PB_ODR] - io_start]) | (~IO[regaddr[PB_DDR] - io_start] & IO[regaddr[PB_IDR] - io_start]);
	else if ((addr == regaddr[PC_IDR]) && (use_PC))
		ret = (IO[regaddr[PC_DDR] - io_start] & IO[regaddr[PC_ODR] - io_start]) | (~IO[regaddr[PC_DDR] - io_start] & IO[regaddr[PC_IDR] - io_start]);
	else if ((addr == regaddr[PD_IDR]) && (use_PD))
		ret = (IO[regaddr[PD_DDR] - io_start] & IO[regaddr[PD_ODR] - io_start]) | (~IO[regaddr[PD_DDR] - io_start] & IO[regaddr[PD_IDR] - io_start]);
	else if ((addr == regaddr[PE_IDR]) && (use_PE))
		ret = (IO[regaddr[PE_DDR] - io_start] & IO[regaddr[PE_ODR] - io_start]) | (~IO[regaddr[PE_DDR] - io_start] & IO[regaddr[PE_IDR] - io_start]);
	else if ((addr == regaddr[PF_IDR]) && (use_PF))
		ret = (IO[regaddr[PF_DDR] - io_start] & IO[regaddr[PF_ODR] - io_start]) | (~IO[regaddr[PF_DDR] - io_start] & IO[regaddr[PF_IDR] - io_start]);
	else if ((addr == regaddr[PG_IDR]) && (use_PG))
		ret = (IO[regaddr[PG_DDR] - io_start] & IO[regaddr[PG_ODR] - io_start]) | (~IO[regaddr[PG_DDR] - io_start] & IO[regaddr[PG_IDR] - io_start]);
	else if ((addr == regaddr[PH_IDR]) && (use_PH))
		ret = (IO[regaddr[PH_DDR] - io_start] & IO[regaddr[PH_ODR] - io_start]) | (~IO[regaddr[PH_DDR] - io_start] & IO[regaddr[PH_IDR] - io_start]);
	else if ((addr == regaddr[PI_IDR]) && (use_PI))
		ret = (IO[regaddr[PI_DDR] - io_start] & IO[regaddr[PI_ODR] - io_start]) | (~IO[regaddr[PI_DDR] - io_start] & IO[regaddr[PI_IDR] - io_start]);
	else
		ret = IO[addr - io_start];

	return ret;
}

/*void ports_write(uint32_t addr, uint8_t val) {
	static uint8_t lastPB_ODR = 0x00;
	IO[addr - IO_START] = val;

	switch (addr) {
	case PB_ODR:
		if (val != lastPB_ODR) {
			PRODUCT_MARKFORUPDATE();
		}
		lastPB_ODR = val;
		break;
	case PE_ODR:
		casil_en(val & 1);
		casil_rs((val >> 1) & 1);
		casil_rw((val >> 2) & 1);
		break;
	case PC_ODR:
		casil_latchwrite(((val >> 1) & 7) | ((val & 0x20) >> 2));
		break;
	}
}*/

void ports_extmodify(uint32_t addr, uint8_t val) {
	uint8_t extisens = 0, i, oldval, newval;

	if (addr == regaddr[PC_IDR]) {
		newval = val & IO[regaddr[PC_DDR] - io_start] & IO[regaddr[PC_CR2] - io_start];
		extisens = (IO[regaddr[EXTI_CR1] - io_start] >> 4) & 3;
		oldval = IO[regaddr[PC_IDR] - io_start] & IO[regaddr[PC_DDR] - io_start] & IO[regaddr[PC_CR2] - io_start];
		for (i = 0; i < 8; i++) {
			if ((newval & (1 << i)) != (oldval & (1 << i))) {
				switch (extisens) {
				case 0: //falling edge and low level
					if ((newval & (1 << i)) == 0)
						cpu_irq(CPU_IRQ_EXTI2);
					break;
				case 1: //rising edge only
					if (newval & (1 << i))
						cpu_irq(CPU_IRQ_EXTI2);
					break;
				case 2: //falling edge only
					if ((newval & (1 << i)) == 0)
						cpu_irq(CPU_IRQ_EXTI2);
					break;
				case 3: //rising and falling edge
					cpu_irq(CPU_IRQ_EXTI2);
					break;
				}
			}
		}
		IO[regaddr[PC_IDR] - io_start] = val;
	}
}
