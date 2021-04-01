#ifndef _CPU_H_
#define _CPU_H_

#include <stdint.h>

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

#define CPUR_A			0
#define CPUR_PCE		1
#define CPUR_PCH		2
#define CPUR_PCL		3
#define CPUR_XH			4
#define CPUR_XL			5
#define CPUR_YH			6
#define CPUR_YL			7
#define CPUR_SPH		8
#define CPUR_SPL		9
#define CPUR_CCR		10

#define ITC_SPR1		11
#define ITC_SPR2		12
#define ITC_SPR3		13
#define ITC_SPR4		14
#define ITC_SPR5		15
#define ITC_SPR6		16
#define ITC_SPR7		17
#define ITC_SPR8		18

#define UART1_SR		19
#define UART1_DR		20
#define UART1_BRR1		21
#define UART1_BRR2		22
#define UART1_CR1		23
#define UART1_CR2		24
#define UART1_CR3		25
#define UART1_CR4		26
#define UART1_CR5		27
#define UART1_GTR		28
#define UART1_PSCR		29

#define EXTI_CR1		30
#define EXTI_CR2		31

#define PA_ODR			32
#define PA_IDR			33
#define PA_DDR			34
#define PA_CR1			35
#define PA_CR2			36

#define PB_ODR			37
#define PB_IDR			38
#define PB_DDR			39
#define PB_CR1			40
#define PB_CR2			41

#define PC_ODR			42
#define PC_IDR			43
#define PC_DDR			44
#define PC_CR1			45
#define PC_CR2			46

#define PD_ODR			47
#define PD_IDR			48
#define PD_DDR			49
#define PD_CR1			50
#define PD_CR2			51

#define PE_ODR			52
#define PE_IDR			53
#define PE_DDR			54
#define PE_CR1			55
#define PE_CR2			56

#define PF_ODR			57
#define PF_IDR			58
#define PF_DDR			59
#define PF_CR1			60
#define PF_CR2			61

#define PG_ODR			62
#define PG_IDR			63
#define PG_DDR			64
#define PG_CR1			65
#define PG_CR2			66

#define PH_ODR			67
#define PH_IDR			68
#define PH_DDR			69
#define PH_CR1			70
#define PH_CR2			71

#define PI_ODR			72
#define PI_IDR			73
#define PI_DDR			74
#define PI_CR1			75
#define PI_CR2			76

#define TIM2_CR1		77
#define TIM2_IER		78
#define TIM2_SR1		79
#define TIM2_SR2		80
#define TIM2_EGR		81
#define TIM2_CCMR1		82
#define TIM2_CCMR2		83
#define TIM2_CCMR3		84
#define TIM2_CCER1		85
#define TIM2_CCER2		86
#define TIM2_CNTRH		87
#define TIM2_CNTRL		88
#define TIM2_PSCR		89
#define TIM2_ARRH		90
#define TIM2_ARRL		91
#define TIM2_CCR1H		92
#define TIM2_CCR1L		93
#define TIM2_CCR2H		94
#define TIM2_CCR2L		95
#define TIM2_CCR3H		96
#define TIM2_CCR3L		97

#define ADC_CSR			98
#define ADC_CR1			99
#define ADC_CR2			100
#define ADC_CR3			101
#define ADC_DRH			102
#define ADC_DRL			103
#define ADC_TDRH		104
#define ADC_TDRL		105

#define UART3_SR		106
#define UART3_DR		107
#define UART3_BRR1		108
#define UART3_BRR2		109
#define UART3_CR1		110
#define UART3_CR2		111
#define UART3_CR3		112
#define UART3_CR4		113
#define UART3_CR6		114

#define CLK_ICKR		115 //Internal clock control register
#define CLK_ECKR		116 //External clock control register
#define CLK_CMSR		117 //Clock master status register
#define CLK_SWR			118 //Clock master switch register
#define CLK_SWCR		119 //Clock switch control register
#define CLK_CKDIVR		120 //Clock divider register
#define CLK_PCKENR1		121 //Peripheral clock gating register 1
#define CLK_CSSR		122 //Clock security system register
#define CLK_CCOR		123 //Configurable clock control register
#define CLK_PCKENR2		124 //Peripheral clock gating register 2
#define CLK_CANCCR		125 //CAN clock control register
#define CLK_HSITRIMR	126 //HSI clock calibration trimming register
#define CLK_SWIMCCR		127 //SWIM clock control register

#define IWDG_KR			128 //IWDG key register
#define IWDG_PR			129 //IWDG prescaler register
#define IWDG_RLR		130 //IWDG reload register

#define FLASH_CR1		131
#define FLASH_CR2		132
#define FLASH_NCR2		133
#define FLASH_FPR		134
#define FLASH_NFPR		135
#define FLASH_IAPSR		136
#define FLASH_PUKR		137
#define FLASH_DUKR		138

#define REGISTERS_COUNT	139


#define BIT_D0  (dest & 0x01) 
#define BIT_D1  ((dest & 0x02) >> 1)
#define BIT_D2  ((dest & 0x04) >> 2)
#define BIT_D3  ((dest & 0x08) >> 3)
#define BIT_D4  ((dest & 0x10) >> 4)
#define BIT_D5  ((dest & 0x20) >> 5)
#define BIT_D6  ((dest & 0x40) >> 6)
#define BIT_D7  ((dest & 0x80) >> 7)
#define BIT_D8  ((dest & 0x100) >> 8)
#define BIT_D9  ((dest & 0x200) >> 9)
#define BIT_D10 ((dest & 0x400) >> 10)
#define BIT_D11 ((dest & 0x800) >> 11)
#define BIT_D12 ((dest & 0x1000) >> 12)
#define BIT_D13 ((dest & 0x2000) >> 13)
#define BIT_D14 ((dest & 0x4000) >> 14)
#define BIT_D15 ((dest & 0x8000) >> 15)

#define BIT_S0  (src & 0x01) 
#define BIT_S1  ((src & 0x02) >> 1)
#define BIT_S2  ((src & 0x04) >> 2)
#define BIT_S3  ((src & 0x08) >> 3)
#define BIT_S4  ((src & 0x10) >> 4)
#define BIT_S5  ((src & 0x20) >> 5)
#define BIT_S6  ((src & 0x40) >> 6)
#define BIT_S7  ((src & 0x80) >> 7)
#define BIT_S8  ((src & 0x100) >> 8)
#define BIT_S9  ((src & 0x200) >> 9)
#define BIT_S10 ((src & 0x400) >> 10)
#define BIT_S11 ((src & 0x800) >> 11)
#define BIT_S12 ((src & 0x1000) >> 12)
#define BIT_S13 ((src & 0x2000) >> 13)
#define BIT_S14 ((src & 0x4000) >> 14)
#define BIT_S15 ((src & 0x8000) >> 15)

#define BIT_R0  (result & 0x01) 
#define BIT_R1  ((result & 0x02) >> 1)
#define BIT_R2  ((result & 0x04) >> 2)
#define BIT_R3  ((result & 0x08) >> 3)
#define BIT_R4  ((result & 0x10) >> 4)
#define BIT_R5  ((result & 0x20) >> 5)
#define BIT_R6  ((result & 0x40) >> 6)
#define BIT_R7  ((result & 0x80) >> 7)

#define BIT_RW0  (result16 & 0x01) 
#define BIT_RW1  ((result16 & 0x02) >> 1)
#define BIT_RW2  ((result16 & 0x04) >> 2)
#define BIT_RW3  ((result16 & 0x08) >> 3)
#define BIT_RW4  ((result16 & 0x10) >> 4)
#define BIT_RW5  ((result16 & 0x20) >> 5)
#define BIT_RW6  ((result16 & 0x40) >> 6)
#define BIT_RW7  ((result16 & 0x80) >> 7)
#define BIT_RW8  ((result16 & 0x100) >> 8)
#define BIT_RW9  ((result16 & 0x200) >> 9)
#define BIT_RW10 ((result16 & 0x400) >> 10)
#define BIT_RW11 ((result16 & 0x800) >> 11)
#define BIT_RW12 ((result16 & 0x1000) >> 12)
#define BIT_RW13 ((result16 & 0x2000) >> 13)
#define BIT_RW14 ((result16 & 0x4000) >> 14)
#define BIT_RW15 ((result16 & 0x8000) >> 15)

extern char* cpu_name;
extern uint32_t pc;
extern uint16_t sp, x, y;
extern uint8_t a, cc;
extern uint8_t running;

void cpu_reset();
int32_t cpu_run(int32_t clocks);
void cpu_irq(uint8_t irq);

#endif
