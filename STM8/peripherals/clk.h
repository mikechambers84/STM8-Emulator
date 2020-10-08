#ifndef _CLK_H_
#define _CLK_H_

#include <stdint.h>

#define CLK_CMSR_HSE		0xB4
#define CLK_CMSR_HSI		0xE1
#define CLK_CMSR_LSI		0xD2

extern int32_t clocksperloop;
extern uint8_t clk_gateADC;
extern uint8_t clk_gateTIM1;
extern uint8_t clk_gateTIM2;
extern uint8_t clk_gateTIM3;
extern uint8_t clk_gateTIM4;
extern uint8_t clk_gateTIM5;
extern uint8_t clk_gateUART1;
extern uint8_t clk_gateUART2;
extern uint8_t clk_gateUART3;
extern uint8_t clk_gateUART4;

void clk_init(void);
void clk_loop(int32_t clocks);
int32_t clk_prescale(uint8_t pscr);
uint8_t clk_read(uint32_t addr);
void clk_write(uint32_t addr, uint8_t val);
void clk_switchSource(uint8_t source);

#endif
