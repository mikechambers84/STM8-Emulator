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
#include "uart3.h"

uint32_t uart3_txTimer, uart3_rxTimer;
uint16_t uart3_div = 0, uart3_rxRead = 0, uart3_rxWrite = 0;
uint8_t uart3_rxBuf[1024];
uint8_t uart3_redirect = UART3_REDIRECT_NULL;
double uart3_baud = 0;

void uart3_rxBufAdd(uint8_t val) {
	if (((uart3_rxWrite + 1) & 1023) == uart3_rxRead) return; //buffer full, discard byte
	uart3_rxBuf[uart3_rxWrite++] = val;
	uart3_rxWrite &= 1023;
	uart3_rxCallback(NULL);
}

uint8_t uart3_rxBufGet() {
	uint8_t ret;
	if (uart3_rxRead == uart3_rxWrite) return 0; //nothing new in the buffer
	ret = uart3_rxBuf[uart3_rxRead++];
	uart3_rxRead &= 1023;
	return ret;
}

uint8_t uart3_read(uint32_t addr) {
	if (addr == regaddr[UART3_DR]) {
		IO[regaddr[UART3_SR] - io_start] &= ~0x20; //clear RXNE bit
		return IO[addr - io_start];
	}
	else {
		return IO[addr - io_start];
	}
}

void uart3_write(uint32_t addr, uint8_t val) {
	uint8_t brr1, brr2;
	double speed;

	if (addr == regaddr[UART3_SR]) {
		//ignore writes to status register
	}
	else if (addr == regaddr[UART3_DR]) {
		IO[regaddr[UART3_SR] - io_start] &= 0x3F; //TODO: should bit 6 (transmission complete) be cleared here, or only bit 7? (TX data reg empty)
		timing_timerEnable(uart3_txTimer);
	}
	else if (addr == regaddr[UART3_BRR1]) { //divider is only changed on writes to BRR1, so BRR2 should be written to first
		brr1 = IO[regaddr[UART3_BRR1] - io_start];
		brr2 = IO[regaddr[UART3_BRR2] - io_start];
		speed = (double)clocksperloop * 100;
		uart3_div = ((uint16_t)(brr2 & 0xF0) << 8) | ((uint16_t)brr1 << 4) | ((uint16_t)brr2 & 0x0F);
		if (uart3_div > 0) {
			uart3_baud = speed / (double)uart3_div;
			timing_updateIntervalFreq(uart3_txTimer, uart3_baud / 10.0);
			timing_updateIntervalFreq(uart3_rxTimer, uart3_baud / 10.0);
		}
		else {
			uart3_baud = 0;
		}
		//printf("UART3 baud rate: %f\n", uart3_baud);
	}
	else {
		IO[addr - io_start] = val;
	}
}

void uart3_txCallback(void* dummy) {
	switch (uart3_redirect) {
	case UART3_REDIRECT_NULL:
		break;
	case UART3_REDIRECT_CONSOLE:
		printf("%c", IO[regaddr[UART3_DR] - io_start]);
		fflush(stdout);
		break;
	case UART3_REDIRECT_SERIAL:
		serial_write(2, (char*)(&IO[regaddr[UART3_DR] - io_start]));
		break;
	case UART3_REDIRECT_TCP:
		tcpconsole_send(2, IO[regaddr[UART3_DR] - io_start]);
		break;
	}

	IO[regaddr[UART3_SR] - io_start] |= 0xC0; //set TXE and TC bits to indicate transmit "circuit" is ready for more data
	if (IO[regaddr[UART3_CR2] - io_start] & 0xC0) //TIEN or TCIEN set
		cpu_irq(CPU_IRQ_UART3_TX);
	timing_timerDisable(uart3_txTimer);
}

void uart3_rxCallback(void* dummy) {
	double speed;

	//Recalculate timing interval again, in case our master clock source or prescaler has changed
	speed = (double)clocksperloop * 100;
	if (uart3_div > 0) {
		uart3_baud = speed / (double)uart3_div;
		timing_updateIntervalFreq(uart3_txTimer, uart3_baud / 10.0);
		timing_updateIntervalFreq(uart3_rxTimer, uart3_baud / 10.0);
	}

	if (uart3_rxRead == uart3_rxWrite) return; //nothing new in the buffer

	IO[regaddr[UART3_DR] - io_start] = uart3_rxBufGet();
	IO[regaddr[UART3_SR] - io_start] |= 0x20; //set RXNE bit
	if (IO[regaddr[UART3_CR2] - io_start] & 0x20) //RIEN set
		cpu_irq(CPU_IRQ_UART3_RX);
}

void uart3_init() {
	IO[regaddr[UART3_SR] - io_start] = 0xC0;
	IO[regaddr[UART3_DR] - io_start] = 0x00;
	IO[regaddr[UART3_BRR1] - io_start] = 0x00;
	IO[regaddr[UART3_BRR2] - io_start] = 0x00;
	IO[regaddr[UART3_CR1] - io_start] = 0x00;
	IO[regaddr[UART3_CR2] - io_start] = 0x00;
	IO[regaddr[UART3_CR3] - io_start] = 0x00;
	IO[regaddr[UART3_CR4] - io_start] = 0x00;
	IO[regaddr[UART3_CR6] - io_start] = 0x00;

	uart3_txTimer = timing_addTimer((void*)uart3_txCallback, NULL, 9600.0 / 10.0, TIMING_DISABLED);
	uart3_rxTimer = timing_addTimer((void*)uart3_rxCallback, NULL, 9600.0 / 10.0, TIMING_ENABLED);
}
