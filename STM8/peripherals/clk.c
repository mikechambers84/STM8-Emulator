#include <stdio.h>
#include <stdint.h>
#include "../memory.h"
#include "../args.h"
#include "../cpu.h"
#include "adc.h"
#include "tim2.h"
#include "uart1.h"
#include "uart3.h"
#include "clk.h"

int32_t clocksperloop = 10000;
int32_t clk_pscr = 8;
uint8_t clk_cmsr = CLK_CMSR_HSI;
uint8_t clk_gateADC = 0;
uint8_t clk_gateTIM1 = 0;
uint8_t clk_gateTIM2 = 0;
uint8_t clk_gateTIM3 = 0;
uint8_t clk_gateTIM4 = 0;
uint8_t clk_gateTIM5 = 0;
uint8_t clk_gateUART1 = 0;
uint8_t clk_gateUART2 = 0;
uint8_t clk_gateUART3 = 0;
uint8_t clk_gateUART4 = 0;

uint8_t clk_read(uint32_t addr) {
	uint8_t ret;
	if (addr == regaddr[CLK_ICKR])
		ret = IO[regaddr[CLK_ICKR] - io_start] | (1 << 4) | (1 << 1); //Always report HSI and LSI as ready
	if (addr == regaddr[CLK_ECKR])
		ret = IO[regaddr[CLK_ECKR] - io_start] | (1 << 1); //Always report HSE as ready
	if (addr == regaddr[CLK_CMSR])
		ret = clk_cmsr;
	else
		ret = IO[addr - io_start];
	return ret;
}

void clk_write(uint32_t addr, uint8_t val) {
	static uint8_t lastCLK_SWCR = 0;
	if (addr == regaddr[CLK_SWR]) {
		if (IO[regaddr[CLK_SWCR] - io_start] & (1 << 1)) { //only change source if SWEN bit set
			clk_switchSource(val);
		}
	}
	else if (addr == regaddr[CLK_SWCR]) {
		if ((val & (1 << 1)) && !(lastCLK_SWCR & (1 << 1))) { //change source if SWEN bit set and was clear before
			clk_switchSource(IO[regaddr[CLK_SWR] - io_start]);
		}
		lastCLK_SWCR = val;
	}
	else if (addr == regaddr[CLK_CKDIVR]) {
		clk_switchSource(clk_cmsr);
	}
}

void clk_loop(int32_t clocks) {
	if (IO[regaddr[CLK_PCKENR1] - io_start] & clk_gateTIM2)
		tim2_clockrun(clocks);
	if (IO[regaddr[CLK_PCKENR1] - io_start] & clk_gateUART1)
		uart1_clockrun(clocks);
	if (IO[regaddr[CLK_PCKENR1] - io_start] & clk_gateUART3)
		uart3_clockrun(clocks);
	if (IO[regaddr[CLK_PCKENR2] - io_start] & clk_gateADC)
		adc_clockrun(clocks);
}

int32_t clk_prescale(uint8_t pscr) {
	uint8_t i;
	int32_t prescale;
	prescale = 1;
	for (i = 0; i < pscr; i++) {
		prescale *= 2;
	}
	return prescale;
}

void clk_switchSource(uint8_t source) {
	int32_t speed;
	uint8_t pscrreg;
	clk_cmsr = source;
	switch (source) {
	case CLK_CMSR_HSE:
		speed = (int32_t)speedarg;
		pscrreg = IO[regaddr[CLK_CKDIVR] - io_start] & 7;
		//printf("Clock switch to HSE\n");
		break;
	case CLK_CMSR_HSI:
		speed = 16000000;
		pscrreg = (IO[regaddr[CLK_CKDIVR] - io_start] >> 3) & 3;
		//printf("Clock switch to HSI\n");
		break;
	case CLK_CMSR_LSI:
		speed = 128000;
		pscrreg = 0;
		//printf("Clock switch to LSI\n");
		break;
	default:
		//printf("DEBUG: Invalid clock source 0x%02X!\n", source);
		return;
	}

	speed /= clk_prescale(pscrreg);
	//printf("Speed: %ld Hz\n", speed);
	clocksperloop = speed / 100;

	IO[regaddr[CLK_SWCR] - io_start] |= (1 << 3); //set SWIF bit
	if (IO[regaddr[CLK_SWCR] - io_start] & (1 << 2)) //if SWIEN bit set
		cpu_irq(CPU_IRQ_CLK); //then fire IRQ
}

void clk_init() {
	IO[regaddr[CLK_ICKR] - io_start] = 0x01;
	IO[regaddr[CLK_ECKR] - io_start] = 0x00;
	IO[regaddr[CLK_CKDIVR] - io_start] = 0x18;
	IO[regaddr[CLK_PCKENR1] - io_start] = 0xFF;
	IO[regaddr[CLK_CSSR] - io_start] = 0x00;
	IO[regaddr[CLK_CCOR] - io_start] = 0x00;
	IO[regaddr[CLK_PCKENR2] - io_start] = 0xFF;
	IO[regaddr[CLK_CANCCR] - io_start] = 0x00;
	IO[regaddr[CLK_HSITRIMR] - io_start] = 0x00;
	clk_switchSource(CLK_CMSR_HSI);
}
