#ifndef _CPU_H_
#define _CPU_H_

#include <stdint.h>
#include "memory.h"

#define COND_V		0x80
#define COND_I1		0x20
#define COND_H		0x10
#define COND_I0		0x08
#define COND_N		0x04
#define COND_Z		0x02
#define COND_C		0x01

#define SET_COND(c)		cc |= c
#define CLEAR_COND(c)	cc &= ~c
#define GET_COND(c)		((cc & c) ? 1 : 0)

#define STACK_TOP	(RAM_START + RAM_SIZE - 1)

#define CPU_IRQ_TLI				0
#define CPU_IRQ_AWU				1
#define CPU_IRQ_CLK				2
#define CPU_IRQ_EXTI0			3
#define CPU_IRQ_EXTI1			4
#define CPU_IRQ_EXTI2			5
#define CPU_IRQ_EXTI3			6
#define CPU_IRQ_EXTI4			7
#define CPU_IRQ_BECAN_RX		8
#define CPU_IRQ_BECAN_TX		9
#define CPU_IRQ_SPI				10
#define CPU_IRQ_TIM1_UPDATE		11
#define CPU_IRQ_TIM1_COMPARE	12
#define CPU_IRQ_TIM2_UPDATE		13
#define CPU_IRQ_TIM2_COMPARE	14
#define CPU_IRQ_TIM3_UPDATE		15
#define CPU_IRQ_TIM3_COMPARE	16
#define CPU_IRQ_UART1_TX		17
#define CPU_IRQ_UART1_RX		18
#define CPU_IRQ_I2C				19
#define CPU_IRQ_UART3_TX		20
#define CPU_IRQ_UART3_RX		21
#define CPU_IRQ_ADC				22
#define CPU_IRQ_TIM4_UPDATE		23
#define CPU_IRQ_FLASH			24

#define D0  (dest & 0x01) 
#define D1  ((dest & 0x02) >> 1)
#define D2  ((dest & 0x04) >> 2)
#define D3  ((dest & 0x08) >> 3)
#define D4  ((dest & 0x10) >> 4)
#define D5  ((dest & 0x20) >> 5)
#define D6  ((dest & 0x40) >> 6)
#define D7  ((dest & 0x80) >> 7)
#define D8  ((dest & 0x100) >> 8)
#define D9  ((dest & 0x200) >> 9)
#define D10 ((dest & 0x400) >> 10)
#define D11 ((dest & 0x800) >> 11)
#define D12 ((dest & 0x1000) >> 12)
#define D13 ((dest & 0x2000) >> 13)
#define D14 ((dest & 0x4000) >> 14)
#define D15 ((dest & 0x8000) >> 15)

#define S0  (src & 0x01) 
#define S1  ((src & 0x02) >> 1)
#define S2  ((src & 0x04) >> 2)
#define S3  ((src & 0x08) >> 3)
#define S4  ((src & 0x10) >> 4)
#define S5  ((src & 0x20) >> 5)
#define S6  ((src & 0x40) >> 6)
#define S7  ((src & 0x80) >> 7)
#define S8  ((src & 0x100) >> 8)
#define S9  ((src & 0x200) >> 9)
#define S10 ((src & 0x400) >> 10)
#define S11 ((src & 0x800) >> 11)
#define S12 ((src & 0x1000) >> 12)
#define S13 ((src & 0x2000) >> 13)
#define S14 ((src & 0x4000) >> 14)
#define S15 ((src & 0x8000) >> 15)

#define R0  (result & 0x01) 
#define R1  ((result & 0x02) >> 1)
#define R2  ((result & 0x04) >> 2)
#define R3  ((result & 0x08) >> 3)
#define R4  ((result & 0x10) >> 4)
#define R5  ((result & 0x20) >> 5)
#define R6  ((result & 0x40) >> 6)
#define R7  ((result & 0x80) >> 7)

#define RW0  (result16 & 0x01) 
#define RW1  ((result16 & 0x02) >> 1)
#define RW2  ((result16 & 0x04) >> 2)
#define RW3  ((result16 & 0x08) >> 3)
#define RW4  ((result16 & 0x10) >> 4)
#define RW5  ((result16 & 0x20) >> 5)
#define RW6  ((result16 & 0x40) >> 6)
#define RW7  ((result16 & 0x80) >> 7)
#define RW8  ((result16 & 0x100) >> 8)
#define RW9  ((result16 & 0x200) >> 9)
#define RW10 ((result16 & 0x400) >> 10)
#define RW11 ((result16 & 0x800) >> 11)
#define RW12 ((result16 & 0x1000) >> 12)
#define RW13 ((result16 & 0x2000) >> 13)
#define RW14 ((result16 & 0x4000) >> 14)
#define RW15 ((result16 & 0x8000) >> 15)

extern char* cpu_name;
extern uint32_t pc;
extern uint16_t sp, x, y;
extern uint8_t a, cc;
extern uint8_t running;

void cpu_reset();
int32_t cpu_run(int32_t clocks);
void cpu_irq(uint8_t irq);

#endif
