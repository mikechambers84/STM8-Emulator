#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "../config.h"
#include "../cpu.h"
#include "../memory.h"
#include "clk.h"

//TODO: Implement analog watchdog

int32_t adc_clocksTotal, adc_clocksRemain;
uint16_t adc_analogVal[16];
uint8_t adc_curChannel = 0, adc_EOCIE = 0, adc_EOC = 1, adc_continuous = 0, adc_pending = 0;

void adc_setAnalogVal(uint8_t channel, uint16_t val) {
	adc_analogVal[channel & 0x0F] = val & 0x3FF; //10-bit limit
	if (channel > 0x0F) {
		printf("DEBUG: Attempted to set a value on an ADC channel higher than 15!\n");
		exit(-1);
	}
}

void adc_timerCallback(void) { //This function emulates completion of ADC sampling
	uint16_t adcval;
	if (IO[regaddr[ADC_CR2] - io_start] & 0x08) //check ADC data alignment bit
		adcval = adc_analogVal[adc_curChannel]; //right aligned
	else
		adcval = adc_analogVal[adc_curChannel] << 6; //left aligned
	IO[regaddr[ADC_DRL] - io_start] = (uint8_t)adcval;
	IO[regaddr[ADC_DRH] - io_start] = (uint8_t)(adcval >> 8);
	adc_EOC = 1;
	if (adc_EOCIE) {
		cpu_irq(CPU_IRQ_ADC);
	}
	if (!adc_continuous) { //disable the timer to prevent more conversions if not in continuous mode
		adc_pending = 0;
	}
}

void adc_init() {
	uint8_t i;
	adc_clocksTotal = 28;
	adc_pending = 0;
	for (i = 0; i < 16; i++) {
		adc_analogVal[i] = 0;
	}
}

void adc_clockrun(int32_t clocks) {
	if (adc_pending == 0) return;
	adc_clocksRemain -= clocks;
	if (adc_clocksRemain <= 0) {
		adc_timerCallback();
		adc_clocksRemain += adc_clocksTotal;
	}
}

int32_t adc_prescale(void) {
	uint8_t pscr;
	pscr = (IO[regaddr[ADC_CR1] - io_start] >> 4) & 7;
	switch (pscr) {
	case 0: return 2;
	case 1: return 3;
	case 2: return 4;
	case 3: return 6;
	case 4: return 8;
	case 5: return 10;
	case 6: return 12;
	case 7: return 18;
	}
	return 2;
}

void adc_write(uint32_t addr, uint8_t val) {
	static uint8_t lastCR1 = 0;

	if (addr == regaddr[ADC_CSR]) {
		adc_EOCIE = (val >> 5) & 1; //interrupt enable on EOC
		adc_EOC = val >> 7;
	}
	else if (addr == regaddr[ADC_CR1]) {
		if ((val & 0xFE) == (lastCR1 & 0xFE)) { //conversions are only init'd if all highest 7 bits stayed the same
			if ((val & 0x01) && !(lastCR1 & 0x01)) { //ADON, initiate a new ADC conversion if set to 1 and was previously 0
				adc_curChannel = IO[regaddr[ADC_CSR] - io_start] & 0x0F;
				adc_clocksTotal = 14 * adc_prescale();
				adc_clocksRemain = adc_clocksTotal;
				adc_pending = 1;
			}
		}
		adc_continuous = (val >> 1) & 1;
		lastCR1 = val;
	}
}

uint8_t adc_read(uint32_t addr) {
	if (addr == regaddr[ADC_CSR])
		return (IO[regaddr[ADC_CSR] - io_start] & 0x3F) | (adc_EOC << 7);
	else
		return IO[addr - io_start];
}

void adc_loop() {

}
