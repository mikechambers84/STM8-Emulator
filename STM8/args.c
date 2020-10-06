#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "config.h"
#include "timing.h"
#include "serial.h"
#include "tcpconsole.h"
#include "products/null.h"
#include "peripherals/uart1.h"
#include "peripherals/uart3.h"
#include "devices/stm8s207r8.h"
#include "devices/stm8s207s6.h"
#include "devices/stm8s003f3.h"

double speedarg = -1;
uint8_t showclock = 0, showdisplay = 1, overridecpu = 0;
char* elffile = NULL;
char* ramfile = NULL;
char* eepromfile = NULL;
char* product_name = "null";

void (*product_register)(void) = (void*)null_register;
void (*product_init)(void) = (void*)null_init;
void (*product_update)(void) = (void*)null_update;
void (*product_portwrite)(uint32_t addr, uint8_t val) = (void*)null_portwrite;
uint8_t (*product_portread)(uint32_t addr, uint8_t* dst) = (void*)null_portread;
void (*product_markforupdate)(void) = (void*)null_markforupdate;
void (*product_loop)(void) = (void*)null_loop;

int args_isMatch(char* s1, char* s2) {
	int i = 0, match = 1;

	while (1) {
		char c1, c2;
		c1 = s1[i];
		c2 = s2[i++];
		if ((c1 >= 'A') && (c1 <= 'Z')) {
			c1 -= 'A' - 'a';
		}
		if ((c2 >= 'A') && (c2 <= 'Z')) {
			c2 -= 'A' - 'a';
		}
		if (c1 != c2) {
			match = 0;
			break;
		}
		if (!c1 || !c2) {
			break;
		}
	}

	return match;
}

void args_showHelp() {
	printf("Command line parameters:\r\n\r\n");

	printf("Emulation options:\r\n");
	printf("  -cpu <model>               Specify CPU model. Use \"-cpu list\" to show available options.\r\n");
	printf("  -elf <input file>          Specify ELF file to load and execute.\r\n");
	printf("  -eeprom <input file>       Specify EEPROM file to load and save.\r\n");
	printf("  -ramdump <input file>      Specify file to dump RAM contents to on exit.\r\n");
	printf("  -osc <mhz>                 Specify external oscillator clock speed to emulate in Hz. Default is 24 MHz.\r\n");
	printf("  -product <product>         Specify product hardware design to simulate.\r\n");
	printf("                             Use \"-product list\" to show available options.\r\n\r\n");

	printf("UART1 options:\r\n");
	printf("  -uart1console              Redirect UART1 through the stdio console.\r\n");
	printf("  -uart1serial <n> <baud>    Redirect UART1 through COM port <n> at <baud>.\r\n");
	printf("  -uart1sock <port>          Redirect UART1 through TCP socket, listen on <port>.\r\n\r\n");

	printf("UART3 options:\r\n");
	printf("  -uart3serial <n> <baud>    Redirect UART3 through COM port <n> at <baud>.\r\n");
	printf("  -uart3console              Redirect UART3 through the stdio console.\r\n");
	printf("  -uart3sock <port>          Redirect UART3 through TCP socket, listen on <port>.\r\n\r\n");

	printf("Display options:\r\n");
	printf("  -nodisplay                 Don't display a product representation.\r\n\r\n");

	printf("Miscellaneous options:\r\n");
	printf("  -showclock                 Display actual clock speed.\r\n");
	printf("  -h                         Show this help screen.\r\n");
}

void args_listcpu() {
	printf("Available CPU options:\r\n\r\n");
	
	printf("STM8S003F3 (Flash: 8 KB, RAM: 1 KB, EEPROM: 128 bytes)\r\n");
	printf("STM8S207S6 (Flash: 32 KB, RAM: 6 KB, EEPROM: 1 KB)\r\n");
	printf("STM8S207R8 (Flash: 64 KB, RAM: 6 KB, EEPROM: 2 KB)\r\n");

	exit(0);
}

void args_listproduct() {
	printf("Available product options:\r\n\r\n");

	printf("     null: Null product. No interface, the CPU runs in isolation.\r\n");

	exit(0);
}

int args_parse(int argc, char* argv[]) {
	int i, choseproduct = 0;

	if (argc < 2) {
		printf("Use -h for help.\n");
		return -1;
	}

	for (i = 1; i < argc; i++) {
		if (args_isMatch(argv[i], "-h")) {
			args_showHelp();
			return -1;
		}
		else if (args_isMatch(argv[i], "-elf")) {
			if ((i + 1) == argc) {
				printf("Parameter required for -elf. Use -h for help.\r\n");
				return -1;
			}
			elffile = argv[++i];
		}
		else if (args_isMatch(argv[i], "-eeprom")) {
			if ((i + 1) == argc) {
				printf("Parameter required for -eeprom. Use -h for help.\r\n");
				return -1;
			}
			eepromfile = argv[++i];
		}
		else if (args_isMatch(argv[i], "-ramdump")) {
			if ((i + 1) == argc) {
				printf("Parameter required for -ramdump. Use -h for help.\r\n");
				return -1;
			}
			ramfile = argv[++i];
		}
		else if (args_isMatch(argv[i], "-osc")) {
			if ((i + 1) == argc) {
				printf("Parameter required for -osc. Use -h for help.\r\n");
				return -1;
			}
			speedarg = atof(argv[++i]);
			if (speedarg < 1000) {
				printf("Invalid oscilator speed. Must be at least 1000 Hz.\r\n");
				return -1;
			}
		}
		else if (args_isMatch(argv[i], "-product")) {
			if ((i + 1) == argc) {
				printf("Parameter required for -product. Use -h for help.\r\n");
				return -1;
			}
			if (args_isMatch(argv[i + 1], "list")) args_listproduct();
			else if (args_isMatch(argv[i + 1], "null")) product_register = (void*)null_register;
			else {
				printf("%s is an invalid product option\r\n", argv[i + 1]);
				return -1;
			}
			i++;
			choseproduct = 1;
		}
		else if (args_isMatch(argv[i], "-cpu")) {
			if ((i + 1) == argc) {
				printf("Parameter required for -cpu. Use -h for help.\r\n");
				return -1;
			}
			if (args_isMatch(argv[i + 1], "list")) args_listcpu();
			else if (args_isMatch(argv[i + 1], "stm8s207r8")) stm8s207r8_init();
			else if (args_isMatch(argv[i + 1], "stm8s207s6")) stm8s207s6_init();
			else if (args_isMatch(argv[i + 1], "stm8s003f3")) stm8s003f3_init();
			else {
				printf("%s is an invalid CPU option\r\n", argv[i + 1]);
				return -1;
			}
			i++;
			overridecpu = 1;
		}
		else if (args_isMatch(argv[i], "-showclock")) {
			showclock = 1;
		}
		else if (args_isMatch(argv[i], "-nodisplay")) {
			showdisplay = 0;
		}
		else if (args_isMatch(argv[i], "-uart1serial")) {
			int comnum, baud;
			if ((i + 2) >= argc) {
				printf("Parameters required for -uart1serial. Use -h for help.\r\n");
				return -1;
			}
			comnum = atoi(argv[i + 1]);
			baud = atoi(argv[i + 2]);
			if (!comnum || !baud) {
				printf("Invalid parameter(s) for -uart1serial. Use -h for help.\r\n");
			}
			if (serial_init(0, comnum, baud)) {
				printf("Error initializing serial port for -uart1serial.\r\n");
				return -1;
			}
			uart1_redirect = UART1_REDIRECT_SERIAL;
			i += 2;
		}
		else if (args_isMatch(argv[i], "-uart3serial")) {
			int comnum, baud;
			if ((i + 2) >= argc) {
				printf("Parameters required for -uart3serial. Use -h for help.\r\n");
				return -1;
			}
			comnum = atoi(argv[i + 1]);
			baud = atoi(argv[i + 2]);
			if (!comnum || !baud) {
				printf("Invalid parameter(s) for -uart3serial. Use -h for help.\r\n");
			}
			if (serial_init(2, comnum, baud)) {
				printf("Error initializing serial port for -uart3serial.\r\n");
				return -1;
			}
			uart3_redirect = UART3_REDIRECT_SERIAL;
			i += 2;
		}
		else if (args_isMatch(argv[i], "-uart1console")) {
			uart1_redirect = UART1_REDIRECT_CONSOLE;
		}
		else if (args_isMatch(argv[i], "-uart3console")) {
			uart3_redirect = UART3_REDIRECT_CONSOLE;
		}
		else if (args_isMatch(argv[i], "-uart1sock")) {
			uint16_t port;
			if ((i + 1) >= argc) {
				printf("Parameter required for -uart1sock. Use -h for help.\r\n");
				return -1;
			}
			port = (uint16_t)atol(argv[i + 1]);
			if (!port) {
				printf("Invalid parameter for -uart1sock. Use -h for help.\r\n");
			}
			if (tcpconsole_init(0, port)) {
				printf("Error initializing listening socket for -uart1sock.\r\n");
				return -1;
			}
			uart1_redirect = UART1_REDIRECT_TCP;
			i++;
		}
		else if (args_isMatch(argv[i], "-uart3sock")) {
			uint16_t port;
			if ((i + 1) >= argc) {
				printf("Parameter required for -uart3sock. Use -h for help.\r\n");
				return -1;
			}
			port = (uint16_t)atol(argv[i + 1]);
			if (!port) {
				printf("Invalid parameter for -uart3sock. Use -h for help.\r\n");
			}
			if (tcpconsole_init(2, port)) {
				printf("Error initializing listening socket for -uart3sock.\r\n");
				return -1;
			}
			uart3_redirect = UART3_REDIRECT_TCP;
			i++;
		}
		else if (args_isMatch(argv[i], "-consolemode")) {
			//This mode is not meant to be called by the user, but by the emulator
			//to spawn new console processes for UARTs and communicate via IPC.
		}
		else {
			printf("%s is not a valid parameter. Use -h for help.\r\n", argv[i]);
			return -1;
		}
	}

	if (choseproduct == 0) {
		printf("Product selection is required. Use -h for help.\r\n");
		exit(-1);
	}

	product_register();

	return 0;
}
