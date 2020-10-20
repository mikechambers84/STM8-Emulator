#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include "../config.h"
#include "../memory.h"
#include "../cpu.h"
#include "clk.h"

int32_t prescaleratio = 1, prescaleticks = 0;

void tim2_init() {
	uint32_t i;
	for (i = tim2_start; i < tim2_end; i++) {
		IO[i - io_start] = 0x00;
	}
	IO[regaddr[TIM2_ARRH] - io_start] = 0xFF;
	IO[regaddr[TIM2_ARRL] - io_start] = 0xFF;
	prescaleratio = 1;
	prescaleticks = 0;
}

void tim2_overflow() {
	//printf("TIM2 overflow\n");

	if (IO[regaddr[TIM2_CR1] - io_start] & 0x08) { //One-pulse mode, disable counter after overflow
		IO[regaddr[TIM2_CR1] - io_start] &= 0xFE;
	}
	if ((IO[regaddr[TIM2_IER] - io_start] & 0x01)) { //if update interrupt enable, do interrupt...
		cpu_irq(CPU_IRQ_TIM2_UPDATE);
	}
	IO[regaddr[TIM2_SR1] - io_start] |= 0x01; //set UIF flag in status register 1
	//counter auto-reload
	//IO[regaddr[TIM2_CNTRL] - io_start] = IO[regaddr[TIM2_ARRL] - io_start];
	//IO[regaddr[TIM2_CNTRH] - io_start] = IO[regaddr[TIM2_ARRH] - io_start];
}

void tim2_clockrun(int32_t clocks) {
	int32_t i;
	if ((IO[regaddr[TIM2_CR1] - io_start] & 0x01) == 0x00) return; //don't do anything if counter isn't enabled

	for (i = 0; i < clocks; i++) {
		if (++prescaleticks >= prescaleratio) {
			if (++IO[regaddr[TIM2_CNTRL] - io_start] == 0) {
				/*if (++IO[regaddr[TIM2_CNTRH] - io_start] == 0) {
					tim2_overflow();
				}*/
				IO[regaddr[TIM2_CNTRH] - io_start]++;
			}
			if ((((uint16_t)IO[regaddr[TIM2_CNTRH] - io_start] << 8) | (uint16_t)IO[regaddr[TIM2_CNTRL] - io_start]) == (((uint16_t)IO[regaddr[TIM2_ARRH] - io_start] << 8) | (uint16_t)IO[regaddr[TIM2_ARRL] - io_start])) {
				tim2_overflow();
				IO[regaddr[TIM2_CNTRL] - io_start] = 0;
				IO[regaddr[TIM2_CNTRH] - io_start] = 0;
			}
			//printf("%02X%02X\n", IO[regaddr[TIM2_CNTRH] - IO_START], IO[regaddr[TIM2_CNTRL] - IO_START]);
			prescaleticks -= prescaleratio;
		}
	}
}

void tim2_write(uint32_t addr, uint8_t val) {
	//printf("TIM2 write: %08X <- %02X\n", addr, val);
	if (addr == regaddr[TIM2_PSCR]) {
		val &= 0x0F;
		IO[regaddr[TIM2_PSCR] - io_start] = val;
		prescaleratio = (int32_t)clk_prescale(val);
		//printf("TIM2 prescale: %lu\n", prescaleratio);
	}
}

uint8_t tim2_read(uint32_t addr) {
	return IO[addr - io_start];
}
