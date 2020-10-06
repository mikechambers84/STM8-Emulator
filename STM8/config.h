#ifndef _CONFIG_H_
#define _CONFIG_H_

//#define DEBUG_OUTPUT

void (*product_init)(void);
void (*product_update)(void);
void (*product_portwrite)(uint32_t addr, uint8_t val);
uint8_t (*product_portread)(uint32_t addr, uint8_t* dst);
void (*product_markforupdate)(void);
void (*product_loop)(void);

#ifdef _WIN32
#define FUNC_INLINE __forceinline
#else
#define FUNC_INLINE __attribute__((always_inline))
#endif

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

#define REGISTERS_COUNT	128

#endif
