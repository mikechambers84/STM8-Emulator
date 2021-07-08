#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include "stm8s207r8.h"
#include "../peripherals/clk.h"
#include "../memory.h"
#include "../cpu.h"
#include "../ports.h"

void stm8s207r8_init() {
	uint32_t i;

	cpu_name = "STM8S207R8";

	flash_start = STM8S207R8_FLASH_START;
	flash_size = STM8S207R8_FLASH_SIZE;

	ram_start = STM8S207R8_RAM_START;
	ram_size = STM8S207R8_RAM_SIZE;

	eeprom_start = STM8S207R8_EEPROM_START;
	eeprom_size = STM8S207R8_EEPROM_SIZE;

	io_start = STM8S207R8_IO_START;
	io_size = STM8S207R8_IO_SIZE;

	cpureg_start = STM8S207R8_CPU_REG_START;
	cpureg_size = STM8S207R8_CPU_REG_SIZE;

	flash_mem = (uint8_t*)malloc(STM8S207R8_FLASH_SIZE);
	RAM = (uint8_t*)malloc(STM8S207R8_RAM_SIZE);
	EEPROM = (uint8_t*)malloc(STM8S207R8_EEPROM_SIZE);
	IO = (uint8_t*)malloc(STM8S207R8_IO_SIZE);
	CPUREG = (uint8_t*)malloc(STM8S207R8_CPU_REG_SIZE);
	if ((flash_mem == NULL) || (RAM == NULL) || (EEPROM == NULL) || (IO == NULL) || (CPUREG == NULL)) {
		printf("Memory allocation error!\n");
		exit(-1);
	}

	memset(flash_mem, 0, STM8S207R8_FLASH_SIZE);
	memset(RAM, 0, STM8S207R8_RAM_SIZE);
	memset(EEPROM, 0, STM8S207R8_EEPROM_SIZE);
	memset(IO, 0, STM8S207R8_IO_SIZE);
	memset(CPUREG, 0, STM8S207R8_CPU_REG_SIZE);

	for (i = 0; i < REGISTERS_COUNT; i++) {
		regaddr[i] = 0xFFFFFFFF;
	}

	regaddr[CPUR_A] = STM8S207R8_CPUR_A;
	regaddr[CPUR_PCE] = STM8S207R8_CPUR_PCE;
	regaddr[CPUR_PCH] = STM8S207R8_CPUR_PCH;
	regaddr[CPUR_PCL] = STM8S207R8_CPUR_PCL;
	regaddr[CPUR_XH] = STM8S207R8_CPUR_XH;
	regaddr[CPUR_XL] = STM8S207R8_CPUR_XL;
	regaddr[CPUR_YH] = STM8S207R8_CPUR_YH;
	regaddr[CPUR_YL] = STM8S207R8_CPUR_YL;
	regaddr[CPUR_SPH] = STM8S207R8_CPUR_SPH;
	regaddr[CPUR_SPL] = STM8S207R8_CPUR_SPL;
	regaddr[CPUR_CCR] = STM8S207R8_CPUR_CCR;
	regaddr[ITC_SPR1] = STM8S207R8_ITC_SPR1;
	regaddr[ITC_SPR2] = STM8S207R8_ITC_SPR2;
	regaddr[ITC_SPR3] = STM8S207R8_ITC_SPR3;
	regaddr[ITC_SPR4] = STM8S207R8_ITC_SPR4;
	regaddr[ITC_SPR5] = STM8S207R8_ITC_SPR5;
	regaddr[ITC_SPR6] = STM8S207R8_ITC_SPR6;
	regaddr[ITC_SPR7] = STM8S207R8_ITC_SPR7;
	regaddr[ITC_SPR8] = STM8S207R8_ITC_SPR8;
	regaddr[UART1_SR] = STM8S207R8_UART1_SR;
	regaddr[UART1_DR] = STM8S207R8_UART1_DR;
	regaddr[UART1_BRR1] = STM8S207R8_UART1_BRR1;
	regaddr[UART1_BRR2] = STM8S207R8_UART1_BRR2;
	regaddr[UART1_CR1] = STM8S207R8_UART1_CR1;
	regaddr[UART1_CR2] = STM8S207R8_UART1_CR2;
	regaddr[UART1_CR3] = STM8S207R8_UART1_CR3;
	regaddr[UART1_CR4] = STM8S207R8_UART1_CR4;
	regaddr[UART1_CR5] = STM8S207R8_UART1_CR5;
	regaddr[UART1_GTR] = STM8S207R8_UART1_GTR;
	regaddr[UART1_PSCR] = STM8S207R8_UART1_PSCR;
	regaddr[UART3_SR] = STM8S207R8_UART3_SR;
	regaddr[UART3_DR] = STM8S207R8_UART3_DR;
	regaddr[UART3_BRR1] = STM8S207R8_UART3_BRR1;
	regaddr[UART3_BRR2] = STM8S207R8_UART3_BRR2;
	regaddr[UART3_CR1] = STM8S207R8_UART3_CR1;
	regaddr[UART3_CR2] = STM8S207R8_UART3_CR2;
	regaddr[UART3_CR3] = STM8S207R8_UART3_CR3;
	regaddr[UART3_CR4] = STM8S207R8_UART3_CR4;
	regaddr[UART3_CR6] = STM8S207R8_UART3_CR6;
	regaddr[EXTI_CR1] = STM8S207R8_EXTI_CR1;
	regaddr[EXTI_CR2] = STM8S207R8_EXTI_CR2;
	regaddr[PA_ODR] = STM8S207R8_PA_ODR;
	regaddr[PA_IDR] = STM8S207R8_PA_IDR;
	regaddr[PA_DDR] = STM8S207R8_PA_DDR;
	regaddr[PA_CR1] = STM8S207R8_PA_CR1;
	regaddr[PA_CR2] = STM8S207R8_PA_CR2;
	regaddr[PB_ODR] = STM8S207R8_PB_ODR;
	regaddr[PB_IDR] = STM8S207R8_PB_IDR;
	regaddr[PB_DDR] = STM8S207R8_PB_DDR;
	regaddr[PB_CR1] = STM8S207R8_PB_CR1;
	regaddr[PB_CR2] = STM8S207R8_PB_CR2;
	regaddr[PC_ODR] = STM8S207R8_PC_ODR;
	regaddr[PC_IDR] = STM8S207R8_PC_IDR;
	regaddr[PC_DDR] = STM8S207R8_PC_DDR;
	regaddr[PC_CR1] = STM8S207R8_PC_CR1;
	regaddr[PC_CR2] = STM8S207R8_PC_CR2;
	regaddr[PD_ODR] = STM8S207R8_PD_ODR;
	regaddr[PD_IDR] = STM8S207R8_PD_IDR;
	regaddr[PD_DDR] = STM8S207R8_PD_DDR;
	regaddr[PD_CR1] = STM8S207R8_PD_CR1;
	regaddr[PD_CR2] = STM8S207R8_PD_CR2;
	regaddr[PE_ODR] = STM8S207R8_PE_ODR;
	regaddr[PE_IDR] = STM8S207R8_PE_IDR;
	regaddr[PE_DDR] = STM8S207R8_PE_DDR;
	regaddr[PE_CR1] = STM8S207R8_PE_CR1;
	regaddr[PE_CR2] = STM8S207R8_PE_CR2;
	regaddr[PF_ODR] = STM8S207R8_PF_ODR;
	regaddr[PF_IDR] = STM8S207R8_PF_IDR;
	regaddr[PF_DDR] = STM8S207R8_PF_DDR;
	regaddr[PF_CR1] = STM8S207R8_PF_CR1;
	regaddr[PF_CR2] = STM8S207R8_PF_CR2;
	regaddr[PG_ODR] = STM8S207R8_PG_ODR;
	regaddr[PG_IDR] = STM8S207R8_PG_IDR;
	regaddr[PG_DDR] = STM8S207R8_PG_DDR;
	regaddr[PG_CR1] = STM8S207R8_PG_CR1;
	regaddr[PG_CR2] = STM8S207R8_PG_CR2;
	regaddr[PH_ODR] = STM8S207R8_PH_ODR;
	regaddr[PH_IDR] = STM8S207R8_PH_IDR;
	regaddr[PH_DDR] = STM8S207R8_PH_DDR;
	regaddr[PH_CR1] = STM8S207R8_PH_CR1;
	regaddr[PH_CR2] = STM8S207R8_PH_CR2;
	regaddr[PI_ODR] = STM8S207R8_PI_ODR;
	regaddr[PI_IDR] = STM8S207R8_PI_IDR;
	regaddr[PI_DDR] = STM8S207R8_PI_DDR;
	regaddr[PI_CR1] = STM8S207R8_PI_CR1;
	regaddr[PI_CR2] = STM8S207R8_PI_CR2;
	regaddr[TIM2_CR1] = STM8S207R8_TIM2_CR1;
	regaddr[TIM2_IER] = STM8S207R8_TIM2_IER;
	regaddr[TIM2_SR1] = STM8S207R8_TIM2_SR1;
	regaddr[TIM2_SR2] = STM8S207R8_TIM2_SR2;
	regaddr[TIM2_EGR] = STM8S207R8_TIM2_EGR;
	regaddr[TIM2_CCMR1] = STM8S207R8_TIM2_CCMR1;
	regaddr[TIM2_CCMR2] = STM8S207R8_TIM2_CCMR2;
	regaddr[TIM2_CCMR3] = STM8S207R8_TIM2_CCMR3;
	regaddr[TIM2_CCER1] = STM8S207R8_TIM2_CCER1;
	regaddr[TIM2_CCER2] = STM8S207R8_TIM2_CCER2;
	regaddr[TIM2_CNTRH] = STM8S207R8_TIM2_CNTRH;
	regaddr[TIM2_CNTRL] = STM8S207R8_TIM2_CNTRL;
	regaddr[TIM2_PSCR] = STM8S207R8_TIM2_PSCR;
	regaddr[TIM2_ARRH] = STM8S207R8_TIM2_ARRH;
	regaddr[TIM2_ARRL] = STM8S207R8_TIM2_ARRL;
	regaddr[TIM2_CCR1H] = STM8S207R8_TIM2_CCR1H;
	regaddr[TIM2_CCR1L] = STM8S207R8_TIM2_CCR1L;
	regaddr[TIM2_CCR2H] = STM8S207R8_TIM2_CCR2H;
	regaddr[TIM2_CCR2L] = STM8S207R8_TIM2_CCR2L;
	regaddr[TIM2_CCR3H] = STM8S207R8_TIM2_CCR3H;
	regaddr[TIM2_CCR3L] = STM8S207R8_TIM2_CCR3L;
	regaddr[ADC_CSR] = STM8S207R8_ADC_CSR;
	regaddr[ADC_CR1] = STM8S207R8_ADC_CR1;
	regaddr[ADC_CR2] = STM8S207R8_ADC_CR2;
	regaddr[ADC_CR3] = STM8S207R8_ADC_CR3;
	regaddr[ADC_DRH] = STM8S207R8_ADC_DRH;
	regaddr[ADC_DRL] = STM8S207R8_ADC_DRL;
	regaddr[ADC_TDRH] = STM8S207R8_ADC_TDRH;
	regaddr[ADC_TDRL] = STM8S207R8_ADC_TDRL;
	regaddr[CLK_ICKR] = STM8S207R8_CLK_ICKR;
	regaddr[CLK_ECKR] = STM8S207R8_CLK_ECKR;
	regaddr[CLK_CMSR] = STM8S207R8_CLK_CMSR;
	regaddr[CLK_SWR] = STM8S207R8_CLK_SWR;
	regaddr[CLK_SWCR] = STM8S207R8_CLK_SWCR;
	regaddr[CLK_CKDIVR] = STM8S207R8_CLK_CKDIVR;
	regaddr[CLK_PCKENR1] = STM8S207R8_CLK_PCKENR1;
	regaddr[CLK_CSSR] = STM8S207R8_CLK_CSSR;
	regaddr[CLK_CCOR] = STM8S207R8_CLK_CCOR;
	regaddr[CLK_PCKENR2] = STM8S207R8_CLK_PCKENR2;
	regaddr[CLK_CANCCR] = STM8S207R8_CLK_CANCCR;
	regaddr[CLK_HSITRIMR] = STM8S207R8_CLK_HSITRIMR;
	regaddr[CLK_SWIMCCR] = STM8S207R8_CLK_SWIMCCR;
	regaddr[IWDG_KR] = STM8S207R8_IWDG_KR;
	regaddr[IWDG_PR] = STM8S207R8_IWDG_PR;
	regaddr[IWDG_RLR] = STM8S207R8_IWDG_RLR;
	regaddr[FLASH_CR1] = STM8S207R8_FLASH_CR1;
	regaddr[FLASH_CR2] = STM8S207R8_FLASH_CR2;
	regaddr[FLASH_NCR2] = STM8S207R8_FLASH_NCR2;
	regaddr[FLASH_FPR] = STM8S207R8_FLASH_FPR;
	regaddr[FLASH_NFPR] = STM8S207R8_FLASH_NFPR;
	regaddr[FLASH_IAPSR] = STM8S207R8_FLASH_IAPSR;
	regaddr[FLASH_PUKR] = STM8S207R8_FLASH_PUKR;
	regaddr[FLASH_DUKR] = STM8S207R8_FLASH_DUKR;

	ports_start = STM8S207R8_PORTS_START;
	ports_end = STM8S207R8_PORTS_END;
	uart1_start = STM8S207R8_UART1_START;
	uart1_end = STM8S207R8_UART1_END;
	uart3_start = STM8S207R8_UART3_START;
	uart3_end = STM8S207R8_UART3_END;
	tim2_start = STM8S207R8_TIM2_START;
	tim2_end = STM8S207R8_TIM2_END;
	adc_start = STM8S207R8_ADC_START;
	adc_end = STM8S207R8_ADC_END;
	clk_start = STM8S207R8_CLK_START;
	clk_end = STM8S207R8_CLK_END;
	iwdg_start = STM8S207R8_IWDG_START;
	iwdg_end = STM8S207R8_IWDG_END;
	flash_reg_start = STM8S207R8_FLASH_REG_START;
	flash_reg_end = STM8S207R8_FLASH_REG_END;

	use_PA = 1;
	use_PB = 1;
	use_PC = 1;
	use_PD = 1;
	use_PE = 1;
	use_PF = 1;
	use_PG = 1;
	use_PH = 1;
	use_PI = 1;

	clk_gateADC = 1 << 3;
	clk_gateTIM1 = 1 << 7;
	clk_gateTIM2 = 1 << 5;
	clk_gateTIM3 = 1 << 6;
	clk_gateTIM4 = 1 << 4;
	clk_gateUART1 = 1 << 2;
	clk_gateUART2 = 0;
	clk_gateUART3 = 1 << 3;
	clk_gateUART4 = 0;
}
