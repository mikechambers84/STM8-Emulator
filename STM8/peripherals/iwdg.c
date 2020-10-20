#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "../config.h"
#include "../cpu.h"
#include "../memory.h"
#include "../timing.h"
#include "../args.h"
#include "clk.h"
#include "iwdg.h"

uint32_t iwdg_timer = 0xFFFFFFFF;
uint8_t iwdg_countdown;
uint8_t iwdg_reload;
uint8_t iwdg_enabled;

void iwdg_write(uint32_t addr, uint8_t val) {
	double pscr = 4;
	if (addr == regaddr[IWDG_KR]) {
		switch (val) {
		case 0xCC: //enable IWDG
			if (!disableIWDG)
				timing_timerEnable(iwdg_timer);
			break;
		case 0xAA: //refresh IWDG
			iwdg_countdown = iwdg_reload;
			break;
		}
		//printf("IWDG KR <- %02X\n", val);
	}
	else if (addr == regaddr[IWDG_PR]) {
		if (IO[regaddr[IWDG_KR] - io_start] == 0x55) {
			switch (val & 7) {
			case 0: pscr = 4; break;
			case 1: pscr = 8; break;
			case 2: pscr = 16; break;
			case 3: pscr = 32; break;
			case 4: pscr = 64; break;
			case 5: pscr = 128; break;
			case 6: pscr = 256; break;
			case 7: pscr = 256; break; //this one is actually "reserved" in the datasheet
			}
			timing_updateIntervalFreq(iwdg_timer, 128000 / pscr);
		}
		//printf("IWDG PR <- %02X\n", val);
	}
	else if (addr == regaddr[IWDG_RLR]) {
		if (IO[regaddr[IWDG_KR] - io_start] == 0x55) {
			iwdg_reload = val;
		}
		//printf("IWDG RLR <- %02X\n", val);
	}
}

uint8_t iwdg_read(uint32_t addr) {
	return IO[addr - io_start];
}

void iwdg_timerCallback(void* dummy) {
	iwdg_countdown--;
	//printf("%02X\n", iwdg_countdown);
	if (iwdg_countdown == 0) {
		if (exitonIWDG) {
			printf("\n\n\n\nIndependent watchdog timer (IWDG) reached terminal count. Aborting emulation!\n");
			exit(0);
		}
		else {
			cpu_reset();
		}
	}
}

void iwdg_init() {
	iwdg_enabled = 0;
	iwdg_countdown = 0xFF;
	iwdg_reload = 0xFF;
	if (iwdg_timer == 0xFFFFFFFF) {
		iwdg_timer = timing_addTimer((void*)iwdg_timerCallback, NULL, 32000, TIMING_DISABLED);
	}
	else {
		timing_updateIntervalFreq(iwdg_timer, 32000);
		timing_timerDisable(iwdg_timer);
	}
}
