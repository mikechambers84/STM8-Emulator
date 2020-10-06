#include <stdio.h>
#include <conio.h>
#include <stdint.h>
#include <stdlib.h>
#include "../args.h"
#include "../cpu.h"
#include "../peripherals/uart1.h"
#include "../peripherals/uart3.h"

void null_register() {
	if (overridecpu == 0) {
		printf("CPU must be explicitly specified if using null product. Use -h for help.\r\n");
		exit(-1);
	}
}

void null_init() {
}

void null_update() {
}

void null_portwrite(uint32_t addr, uint8_t val) {
}

uint8_t null_portread(uint32_t addr) {
	return 0;
}

void null_markforupdate() {
}

void null_loop() {
	uint8_t cc, gotkey;
	gotkey = 0;
	if (_kbhit()) {
		cc = _getch();
		if (cc == 27) {
			running = 0;
		}
		else {
			gotkey = 1;
		}
	}

	if ((uart1_redirect == UART1_REDIRECT_CONSOLE) && gotkey) {
		uart1_rxBufAdd(cc);
	}

	if ((uart3_redirect == UART3_REDIRECT_CONSOLE) && gotkey) {
		uart3_rxBufAdd(cc);
	}
}
