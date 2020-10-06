#include <stdio.h>
#include <stdint.h>
#include "../config.h"
#include "../cpu.h"
#include "../memory.h"
#include "../args.h"
#include "../timing.h"
#include "../serial.h"
#include "../tcpconsole.h"
#include "clk.h"
#include "uart1.h"

uint32_t uart1_txTimer, uart1_rxTimer;
uint16_t uart1_div = 0, uart1_rxRead = 0, uart1_rxWrite = 0;
uint8_t uart1_rxBuf[1024];
uint8_t uart1_redirect = UART1_REDIRECT_NULL;
double uart1_baud = 0;

void uart1_rxBufAdd(uint8_t val) {
	if (((uart1_rxWrite + 1) & 1023) == uart1_rxRead) return; //buffer full, discard byte
	uart1_rxBuf[uart1_rxWrite++] = val;
	uart1_rxWrite &= 1023;
	uart1_rxCallback(NULL);
}

uint8_t uart1_rxBufGet() {
	uint8_t ret;
	if (uart1_rxRead == uart1_rxWrite) return 0; //nothing new in the buffer
	ret = uart1_rxBuf[uart1_rxRead++];
	uart1_rxRead &= 1023;
	return ret;
}

uint8_t uart1_read(uint32_t addr) {
	if (addr == regaddr[UART1_DR]) {
		IO[regaddr[UART1_SR] - io_start] &= ~0x20; //clear RXNE bit
		return IO[addr - io_start];
	}
	else {
		return IO[addr - io_start];
	}
}

void uart1_write(uint32_t addr, uint8_t val) {
	uint8_t brr1, brr2;
	double speed;

	if (addr == regaddr[UART1_SR]) {
		//ignore writes to status register
	}
	else if (addr == regaddr[UART1_DR]) {
		IO[regaddr[UART1_SR] - io_start] &= 0x3F; //TODO: should bit 6 (transmission complete) be cleared here, or only bit 7? (TX data reg empty)
		timing_timerEnable(uart1_txTimer);
	}
	else if (addr == regaddr[UART1_BRR1]) { //divider is only changed on writes to BRR1, so BRR2 should be written to first
		brr1 = IO[regaddr[UART1_BRR1] - io_start];
		brr2 = IO[regaddr[UART1_BRR2] - io_start];
		speed = (double)clocksperloop * 100;
		uart1_div = ((uint16_t)(brr2 & 0xF0) << 8) | ((uint16_t)brr1 << 4) | ((uint16_t)brr2 & 0x0F);
		if (uart1_div > 0) {
			uart1_baud = speed / (double)uart1_div;
			timing_updateIntervalFreq(uart1_txTimer, uart1_baud / 10.0);
			timing_updateIntervalFreq(uart1_rxTimer, uart1_baud / 10.0);
		}
		else {
			uart1_baud = 0;
		}
		//printf("UART1 baud rate: %f\n", uart1_baud);
	}
	else {
		IO[addr - io_start] = val;
	}
}

void uart1_txCallback(void* dummy) {
	switch (uart1_redirect) {
	case UART1_REDIRECT_NULL:
		break;
	case UART1_REDIRECT_CONSOLE:
		printf("%c", IO[regaddr[UART1_DR] - io_start]);
		fflush(stdout);
		break;
	case UART1_REDIRECT_SERIAL:
		serial_write(0, (char*)(&IO[regaddr[UART1_DR] - io_start]));
		break;
	case UART1_REDIRECT_TCP:
		tcpconsole_send(0, IO[regaddr[UART1_DR] - io_start]);
		break;
	}

	IO[regaddr[UART1_SR] - io_start] |= 0xC0; //set TXE and TC bits to indicate transmit "circuit" is ready for more data
	if (IO[regaddr[UART1_CR2] - io_start] & 0xC0) //TIEN or TCIEN set
		cpu_irq(CPU_IRQ_UART1_TX);
	timing_timerDisable(uart1_txTimer);
}

void uart1_rxCallback(void* dummy) {
	double speed;

	//Recalculate timing interval again, in case our master clock source or prescaler has changed
	speed = (double)clocksperloop * 100;
	if (uart1_div > 0) {
		uart1_baud = speed / (double)uart1_div;
		timing_updateIntervalFreq(uart1_txTimer, uart1_baud / 10.0);
		timing_updateIntervalFreq(uart1_rxTimer, uart1_baud / 10.0);
	}

	if (uart1_rxRead == uart1_rxWrite) return; //nothing new in the buffer

	IO[regaddr[UART1_DR] - io_start] = uart1_rxBufGet();
	IO[regaddr[UART1_SR] - io_start] |= 0x20; //set RXNE bit
	if (IO[regaddr[UART1_CR2] - io_start] & 0x20) //RIEN set
		cpu_irq(CPU_IRQ_UART1_RX);
}

void uart1_init() {
	IO[regaddr[UART1_SR] - io_start] = 0xC0;
	IO[regaddr[UART1_DR] - io_start] = 0x00;
	IO[regaddr[UART1_BRR1] - io_start] = 0x00;
	IO[regaddr[UART1_BRR2] - io_start] = 0x00;
	IO[regaddr[UART1_CR1] - io_start] = 0x00;
	IO[regaddr[UART1_CR2] - io_start] = 0x00;
	IO[regaddr[UART1_CR3] - io_start] = 0x00;
	IO[regaddr[UART1_CR4] - io_start] = 0x00;
	IO[regaddr[UART1_CR5] - io_start] = 0x00;
	IO[regaddr[UART1_GTR] - io_start] = 0x00;
	IO[regaddr[UART1_PSCR] - io_start] = 0x00;

	uart1_txTimer = timing_addTimer((void*)uart1_txCallback, NULL, 9600.0 / 10.0, TIMING_DISABLED);
	uart1_rxTimer = timing_addTimer((void*)uart1_rxCallback, NULL, 9600.0 / 10.0, TIMING_ENABLED);
}
