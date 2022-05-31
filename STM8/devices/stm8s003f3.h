#ifndef _STM8S003F3_H_
#define _STM8S003F3_H_

#include <stdint.h>

void stm8s003f3_init(void);

#define STM8S003F3_FLASH_START		0x8000
#define STM8S003F3_FLASH_SIZE		0x2000

#define STM8S003F3_RAM_START		0x0000
#define STM8S003F3_RAM_SIZE			0x400

#define STM8S003F3_STACK_SIZE		0x200

#define STM8S003F3_EEPROM_START		0x4000
#define STM8S003F3_EEPROM_SIZE		0x80

#define STM8S003F3_IO_START			0x5000
#define STM8S003F3_IO_SIZE			0x800

#define STM8S003F3_CPU_REG_START	0x7F00
#define STM8S003F3_CPU_REG_SIZE		0x100

#define STM8S003F3_CPUR_A			0x7F00
#define STM8S003F3_CPUR_PCE			0x7F01
#define STM8S003F3_CPUR_PCH			0x7F02
#define STM8S003F3_CPUR_PCL			0x7F03
#define STM8S003F3_CPUR_XH			0x7F04
#define STM8S003F3_CPUR_XL			0x7F05
#define STM8S003F3_CPUR_YH			0x7F06
#define STM8S003F3_CPUR_YL			0x7F07
#define STM8S003F3_CPUR_SPH			0x7F08
#define STM8S003F3_CPUR_SPL			0x7F09
#define STM8S003F3_CPUR_CCR			0x7F0A

#define STM8S003F3_ITC_SPR1			0x7F70
#define STM8S003F3_ITC_SPR2			0x7F71
#define STM8S003F3_ITC_SPR3			0x7F72
#define STM8S003F3_ITC_SPR4			0x7F73
#define STM8S003F3_ITC_SPR5			0x7F74
#define STM8S003F3_ITC_SPR6			0x7F75
#define STM8S003F3_ITC_SPR7			0x7F76
#define STM8S003F3_ITC_SPR8			0x7F77

#define STM8S003F3_UART1_SR			0x5230
#define STM8S003F3_UART1_DR			0x5231
#define STM8S003F3_UART1_BRR1		0x5232
#define STM8S003F3_UART1_BRR2		0x5233
#define STM8S003F3_UART1_CR1		0x5234
#define STM8S003F3_UART1_CR2		0x5235
#define STM8S003F3_UART1_CR3		0x5236
#define STM8S003F3_UART1_CR4		0x5237
#define STM8S003F3_UART1_CR5		0x5238
#define STM8S003F3_UART1_GTR		0x5239
#define STM8S003F3_UART1_PSCR		0x523A

#define STM8S003F3_UART1_START		STM8S003F3_UART1_SR
#define STM8S003F3_UART1_END		STM8S003F3_UART1_PSCR

#define STM8S003F3_EXTI_CR1			0x50A0
#define STM8S003F3_EXTI_CR2			0x50A1

#define STM8S003F3_PA_ODR			0x5000
#define STM8S003F3_PA_IDR			0x5001
#define STM8S003F3_PA_DDR			0x5002
#define STM8S003F3_PA_CR1			0x5003
#define STM8S003F3_PA_CR2			0x5004

#define STM8S003F3_PB_ODR			0x5005
#define STM8S003F3_PB_IDR			0x5006
#define STM8S003F3_PB_DDR			0x5007
#define STM8S003F3_PB_CR1			0x5008
#define STM8S003F3_PB_CR2			0x5009

#define STM8S003F3_PC_ODR			0x500A
#define STM8S003F3_PC_IDR			0x500B
#define STM8S003F3_PC_DDR			0x500C
#define STM8S003F3_PC_CR1			0x500D
#define STM8S003F3_PC_CR2			0x500E

#define STM8S003F3_PD_ODR			0x500F
#define STM8S003F3_PD_IDR			0x5010
#define STM8S003F3_PD_DDR			0x5011
#define STM8S003F3_PD_CR1			0x5012
#define STM8S003F3_PD_CR2			0x5013

#define STM8S003F3_PE_ODR			0x5014
#define STM8S003F3_PE_IDR			0x5015
#define STM8S003F3_PE_DDR			0x5016
#define STM8S003F3_PE_CR1			0x5017
#define STM8S003F3_PE_CR2			0x5018

#define STM8S003F3_PF_ODR			0x5019
#define STM8S003F3_PF_IDR			0x501A
#define STM8S003F3_PF_DDR			0x501B
#define STM8S003F3_PF_CR1			0x501C
#define STM8S003F3_PF_CR2			0x501D

#define STM8S003F3_PORTS_START		STM8S003F3_PA_ODR
#define STM8S003F3_PORTS_END		STM8S003F3_PF_CR2

#define STM8S003F3_TIM2_CR1			0x5300 //TIM2 control register 1 0x00
#define STM8S003F3_TIM2_IER			0x5303 //TIM2 interrupt enable register 0x00
#define STM8S003F3_TIM2_SR1			0x5304 //TIM2 status register 1 0x00
#define STM8S003F3_TIM2_SR2			0x5305 //TIM2 status register 2 0x00
#define STM8S003F3_TIM2_EGR			0x5306 //TIM2 event generation register 0x00
#define STM8S003F3_TIM2_CCMR1		0x5307 //TIM2 capture/compare mode register 1 0x00
#define STM8S003F3_TIM2_CCMR2		0x5308 //TIM2 capture/compare mode register 2 0x00
#define STM8S003F3_TIM2_CCMR3		0x5309 //TIM2 capture/compare mode register 3 0x00
#define STM8S003F3_TIM2_CCER1		0x530A //TIM2 capture/compare enable register 1 0x00
#define STM8S003F3_TIM2_CCER2		0x530B //TIM2 capture/compare enable register 2 0x00
#define STM8S003F3_TIM2_CNTRH		0x530C //TIM2 counter high 0x00
#define STM8S003F3_TIM2_CNTRL		0x530D //TIM2 counter low 0x00
#define STM8S003F3_TIM2_PSCR		0x530E //TIM2 prescaler register 0x00
#define STM8S003F3_TIM2_ARRH		0x530F //TIM2 auto-reload register high 0xFF
#define STM8S003F3_TIM2_ARRL		0x5310 //TIM2 auto-reload register low 0xFF
#define STM8S003F3_TIM2_CCR1H		0x5311 //TIM2 capture/compare register 1 high 0x00
#define STM8S003F3_TIM2_CCR1L		0x5312 //TIM2 capture/compare register 1 low 0x00
#define STM8S003F3_TIM2_CCR2H		0x5313 //TIM2 capture/compare reg. 2 high 0x00
#define STM8S003F3_TIM2_CCR2L		0x5314 //TIM2 capture/compare register 2 low 0x00
#define STM8S003F3_TIM2_CCR3H		0x5315 //TIM2 capture/compare register 3 high 0x00
#define STM8S003F3_TIM2_CCR3L		0x5316 //TIM2 capture/compare register 3 low 0x00

#define STM8S003F3_TIM2_START		STM8S003F3_TIM2_CR1
#define STM8S003F3_TIM2_END			STM8S003F3_TIM2_CCR3L

#define STM8S003F3_ADC_CSR			0x5400 //ADC control/status register
#define STM8S003F3_ADC_CR1			0x5401 //ADC configuration register 1
#define STM8S003F3_ADC_CR2			0x5402 //ADC configuration register 2
#define STM8S003F3_ADC_CR3			0x5403 //ADC configuration register 3
#define STM8S003F3_ADC_DRH			0x5404 //ADC data register high
#define STM8S003F3_ADC_DRL			0x5405 //ADC data register low
#define STM8S003F3_ADC_TDRH			0x5406 //ADC Schmitt trigger disable register high
#define STM8S003F3_ADC_TDRL			0x5407 //ADC Schmitt trigger disable register low
#define STM8S003F3_ADC_HTRH			0x5408 //ADC high threshold register high
#define STM8S003F3_ADC_HTRL			0x5409 //ADC high threshold register low
#define STM8S003F3_ADC_LTRH			0x540A //ADC low threshold register high
#define STM8S003F3_ADC_LTRL			0x540B //ADC low threshold register low
#define STM8S003F3_ADC_AWSRH		0x540C //ADC analog watchdog status register high
#define STM8S003F3_ADC_AWSRL		0x540D //ADC analog watchdog status register low
#define STM8S003F3_ADC_AWCRH		0x540E //ADC analog watchdog control register high
#define STM8S003F3_ADC_AWCRL		0x540F //ADC analog watchdog control register low

#define STM8S003F3_ADC_START		STM8S003F3_ADC_CSR
#define STM8S003F3_ADC_END			STM8S003F3_ADC_AWCRL

#define STM8S003F3_CLK_ICKR			0x50C0 //Internal clock control register
#define STM8S003F3_CLK_ECKR			0x50C1 //External clock control register
#define STM8S003F3_CLK_CMSR			0x50C3 //Clock master status register
#define STM8S003F3_CLK_SWR			0x50C4 //Clock master switch register
#define STM8S003F3_CLK_SWCR			0x50C5 //Clock switch control register
#define STM8S003F3_CLK_CKDIVR		0x50C6 //Clock divider register
#define STM8S003F3_CLK_PCKENR1		0x50C7 //Peripheral clock gating register 1
#define STM8S003F3_CLK_CSSR			0x50C8 //Clock security system register
#define STM8S003F3_CLK_CCOR			0x50C9 //Configurable clock control register
#define STM8S003F3_CLK_PCKENR2		0x50CA //Peripheral clock gating register 2
#define STM8S003F3_CLK_CANCCR		0x50CB //CAN clock control register
#define STM8S003F3_CLK_HSITRIMR		0x50CC //HSI clock calibration trimming register
#define STM8S003F3_CLK_SWIMCCR		0x50CD //SWIM clock control register

#define STM8S003F3_CLK_START		STM8S003F3_CLK_ICKR
#define STM8S003F3_CLK_END			STM8S003F3_CLK_SWIMCCR

#define STM8S003F3_IWDG_KR			0x50E0 //IWDG key register
#define STM8S003F3_IWDG_PR			0x50E1 //IWDG prescaler register
#define STM8S003F3_IWDG_RLR			0x50E2 //IWDG reload register

#define STM8S003F3_IWDG_START		STM8S003F3_IWDG_KR
#define STM8S003F3_IWDG_END			STM8S003F3_IWDG_RLR

#define STM8S003F3_FLASH_CR1		0x505A //Flash control register 1
#define STM8S003F3_FLASH_CR2		0x505B //Flash control register 2
#define STM8S003F3_FLASH_NCR2		0x505C //Flash complimentary control register 2
#define STM8S003F3_FLASH_FPR		0x505D //Flash protection register
#define STM8S003F3_FLASH_NFPR		0x505E //Flash complimentary protection register
#define STM8S003F3_FLASH_IAPSR		0x505F //Flash in-application programming status register
#define STM8S003F3_FLASH_PUKR		0x5062 //Flash Program memory unprotection register
#define STM8S003F3_FLASH_DUKR		0x5064 //Data EEPROM unprotection register

#define STM8S003F3_FLASH_REG_START	STM8S003F3_FLASH_CR1
#define STM8S003F3_FLASH_REG_END	STM8S003F3_FLASH_DUKR

#endif
