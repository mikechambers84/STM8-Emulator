/*
	STM8 emulator main
*/

#include <stdio.h>
#include <conio.h>
#include <stdint.h>
#include <Windows.h>
#include "config.h"
#include "elf.h"
#include "intelhex.h"
#include "srec.h"
#include "memory.h"
#include "cpu.h"
#include "peripherals/uart1.h"
#include "peripherals/uart3.h"
#include "peripherals/tim2.h"
#include "peripherals/adc.h"
#include "peripherals/clk.h"
#include "display.h"
#include "timing.h"
#include "args.h"
#include "serial.h"
#include "tcpconsole.h"

uint32_t clocksrun = 0, clocktimer;
uint8_t pause = 0;

void eeprom_load(char* file) {
	FILE* eepfile;
	eepfile = fopen(file, "rb");
	if (eepfile == NULL) {
		printf("Error loading EEPROM file.\n");
		return;
	}
	fread(EEPROM, 1, eeprom_size, eepfile);
	fclose(eepfile);
}

void clocks_callback(void* dummy) {
	char info[64];
	if (showclock) {
		sprintf(info, "Clock speed: %.03f MHz         ", ((double)clocksrun * 2) / 1000000.0);
		if (showdisplay)
			display_str(40, 0, info);
		else
			printf("%s\r", info);
	}
	clocksrun = 0;
}

void limiter_callback(void* dummy) {
	pause = 0;
}

int main(int argc, char* argv[]) {
	FILE* dumpfile;
	int dummy;

	printf("STM8 Emulator v0.13 - 2022/04/20\n\n");

	if (args_parse(argc, argv)) {
		return -1;
	}

	if (elffile == NULL && hexfile == NULL && srecfile == NULL) {
		printf("An ELF, Intel Hex, or SREC file must be specified. Use -h for help.\n");
		return -1;
	}

	if((elffile != NULL && (hexfile != NULL || srecfile != NULL)) || (hexfile != NULL && (elffile != NULL || srecfile != NULL)) || (srecfile != NULL && (elffile != NULL || hexfile != NULL))) {
		printf("Cannot specify more than one program file (ELF, Intel Hex, SREC) simultaneously. Use -h for help.\n");
		return -1;
	}

	if (speedarg <= 0)
		speedarg = 24000000;

	printf("Emulator configuration:\n");
	printf("       Product: %s\n", product_name);
	printf("           CPU: %s (Flash = %lu, RAM = %lu, EEPROM = %lu)\n", cpu_name, flash_size, ram_size, eeprom_size);
	printf("Ext oscillator: %.00f Hz\n", speedarg);
	printf("  Program file: %s\n", (elffile != NULL) ? elffile : ((hexfile != NULL) ? hexfile : srecfile));
	printf("   EEPROM file: %s\n", (eepromfile == NULL) ? "[none]" : eepromfile);
	printf("     Dump file: %s\n", (ramfile == NULL) ? "[none]" : ramfile);
	printf("         UART1: ");
	switch (uart1_redirect) {
	case UART1_REDIRECT_NULL: printf("No connection\n"); break;
	case UART1_REDIRECT_CONSOLE: printf("Console redirection\n"); break;
	case UART1_REDIRECT_SERIAL: printf("Serial port redirection\n"); break;
	case UART1_REDIRECT_TCP: printf("TCP socket redirection (server)\n"); break;
	}
	printf("         UART3: ");
	switch (uart3_redirect) {
	case UART3_REDIRECT_NULL: printf("No connection\n"); break;
	case UART3_REDIRECT_CONSOLE: printf("Console redirection\n"); break;
	case UART3_REDIRECT_SERIAL: printf("Serial port redirection\n"); break;
	case UART3_REDIRECT_TCP: printf("TCP socket redirection (server)\n"); break;
	}

	if(elffile != NULL) {
		if(elf_load(elffile) < 0) {
			printf("There was an error loading the ELF file.\n");
			return -1;
		}
	} else if(hexfile != NULL) {
		if(intelhex_load(hexfile) < 0) {
			printf("There was an error loading the Intel Hex file.\n");
			return -1;
		}
	} else if(srecfile != NULL) {
		if(srec_load(srecfile) < 0) {
			printf("There was an error loading the SREC file.\n");
			return -1;
		}
	}
	if (eepromfile != NULL) {
		eeprom_load(eepromfile);
	}
	//printf("Clocks per loop: %ld\n", clocksperloop);
	printf("\nPress any key to begin emulation...\n");
	dummy = _getch();


	timing_init();
#ifndef DEBUG_OUTPUT
	if (showdisplay) {
		display_init();
		product_init();
	}
#endif
	timing_addTimer((void*)clocks_callback, NULL, 2, TIMING_ENABLED);
	timing_addTimer((void*)limiter_callback, NULL, 100, TIMING_ENABLED);
	cpu_reset();
	running = 1;
	while (running) {
		uint16_t loops;
		while (pause) {
			timing_loop();
			adc_loop();
		}
		for (loops = 0; loops < (clocksperloop / 10); loops++) {
			cpu_run(10);
			clk_loop(10);
			clocksrun += 10UL;
		}
		pause = 1;
		timing_loop();
		adc_loop();
		serial_loop();
		tcpconsole_loop();
		product_loop();
#ifndef DEBUG_OUTPUT
		if (showdisplay) {
			product_update();
		}
#endif
	}
#ifndef DEBUG_OUTPUT
	if (showdisplay) display_shutdown();
#endif
	if (ramfile != NULL) {
		dumpfile = fopen(ramfile, "wb");
		fwrite(RAM, 1, ram_size, dumpfile);
		fclose(dumpfile);
	}
	if (eepromfile != NULL) {
		dumpfile = fopen(eepromfile, "wb");
		fwrite(EEPROM, 1, eeprom_size, dumpfile);
		fclose(dumpfile);
	}

	return 0;
}
