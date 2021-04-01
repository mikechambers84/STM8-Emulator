/*
	Casil 1610 16-character LCD display device
*/

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "../config.h"
#include "../display.h"

uint8_t rs, rw, en, latch, dataval = 0, flipflop = 0, prevcmd = 0;
uint8_t disp[16];
uint8_t casilmem[256], casiladdr = 0;

void casil_init() {
	flipflop = 0;
	dataval = 0;
	prevcmd = 0;
	memset(casilmem, 0, 256);
}

void casil_print() {
	uint8_t i;
	printf("\rCASIL: \"");
	for (i = 0; i < 16; i++) {
		printf("%c", disp[i]);
	}
	printf("\"");
}

void casil_checkifupdated() {
	static uint8_t lastdisp[16];
	uint8_t i;

	for (i = 0; i < 16; i++) {
		if (disp[i] != lastdisp[i]) {
			product_markforupdate();
			break;
		}
	}

	memcpy(lastdisp, disp, 16);
}

void casil_rs(uint8_t val) {
	rs = val;
}

void casil_rw(uint8_t val) {
	rw = val;
}

void casil_en(uint8_t val) {
	uint8_t olden;
	olden = en;
	en = val;
	if (en == olden) return;
	if (en == 1) return;

	if (flipflop == 0) { //high nibble first
		dataval = (dataval & 0x0F) | (latch << 4);
	}
	else { //low nibble
		dataval = (dataval & 0xF0) | (latch & 0x0F);
		//printf("Casil command: %02X\n", dataval);
		if (!rs && !rw) {
			casiladdr = dataval;
		}
		else if (!rw) {
			casilmem[casiladdr] = dataval;
			if ((casiladdr >= 0x80) && (casiladdr <= 0x87)) {
				disp[casiladdr - 0x80] = dataval;
				casil_checkifupdated();
			}
			else if ((casiladdr >= 0xC0) && (casiladdr <= 0xC7)) {
				disp[casiladdr - 0xB8] = dataval;
				casil_checkifupdated();
			}
			casiladdr++;
		}
		prevcmd = dataval;
	}

	flipflop ^= 1;
}

void casil_latchwrite(uint8_t val) {
	latch = val & 0x0F;
}

uint8_t casil_latchread() {
	return 0;
}
