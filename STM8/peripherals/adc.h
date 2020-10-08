#ifndef _ADC_H_
#define _ADC_H_

#include <stdint.h>

void adc_setAnalogVal(uint8_t channel, uint16_t val);
void adc_init(void);
void adc_clockrun(int32_t clocks);
void adc_write(uint32_t addr, uint8_t val);
uint8_t adc_read(uint32_t addr);
void adc_loop(void);

#endif
