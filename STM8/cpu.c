/*
	STM8 CPU emulator core

	NOTE: I should rewrite this to take advantage of the many bit patterns in the opcodes.
		  Then I can get rid of the huge switch statement that has a case for each one.
		  That should also make debugging this code easier.
*/

#include <stdio.h>
#include <stdint.h>
#include "memory.h"
#include "peripherals/uart1.h"
#include "peripherals/uart3.h"
#include "peripherals/tim2.h"
#include "peripherals/adc.h"
#include "peripherals/clk.h"
#include "peripherals/iwdg.h"
#include "cpu.h"

char* cpu_name = "";

//CPU registers
uint32_t pc;
uint16_t sp, x, y;
uint8_t a, cc;


//helper variables
uint8_t opcode, valid, result, prefix, running, halt;
uint16_t result16;
uint32_t addr, origpc, oppc;
uint8_t irq_req[32], anyirq = 0;

void cpu_reset() {
	uint8_t i;

	pc = 0x8000; //reset vector
	sp = ram_start + ram_size - 1;
	cc = COND_I0 | COND_I1;
	halt = 0;

	for (i = 0; i < 32; i++) {
		irq_req[i] = 0;
	}

	uart1_init();
	uart3_init();
	tim2_init();
	adc_init();
	clk_init();
	iwdg_init();
}

FUNC_INLINE void cpu_push(uint8_t val) {
#ifdef DEBUG_OUTPUT
	printf("Push %08X: %02X\n", sp, val);
#endif
	memory_write(sp, val);
	if (sp == 0)
		sp = ram_size - 1;
	else
		sp--;
}

FUNC_INLINE uint8_t cpu_pop() {
	sp++;
#ifdef DEBUG_OUTPUT
	printf("Pop %08X: %02X\n", sp, memory_read(sp));
#endif
	return memory_read(sp);
}

FUNC_INLINE uint8_t cpu_imm8() {
	uint8_t ret;
	ret = memory_read(++pc);
#ifdef DEBUG_OUTPUT
	printf("cpu_imm8() = %02X\n", ret);
#endif
	return ret;
}

FUNC_INLINE uint32_t cpu_soff8() {
	uint32_t ret;
	ret = memory_read(++pc);
	if (ret & 0x80) ret |= 0xFFFFFF00;
#ifdef DEBUG_OUTPUT
	printf("cpu_soff8() = %08X\n", ret);
#endif
	return ret;
}

FUNC_INLINE uint16_t cpu_imm16() {
	uint16_t ret;
	ret = memory_read(pc + 2) | ((uint16_t)memory_read(pc + 1) << 8);
	pc += 2;
#ifdef DEBUG_OUTPUT
	printf("cpu_imm16() = %04X\n", ret);
#endif
	return ret;
}

FUNC_INLINE uint32_t cpu_imm24() {
	uint32_t ret;
	ret = memory_read(pc + 3) | ((uint32_t)memory_read(pc + 2) << 8) | ((uint32_t)memory_read(pc + 1) << 16);
	pc += 3;
#ifdef DEBUG_OUTPUT
	printf("cpu_imm24() = %06X\n", ret);
#endif
	return ret;
}

FUNC_INLINE void cpu_call(uint16_t dest) {
	cpu_push((uint8_t)pc);
	cpu_push((uint8_t)(pc >> 8));
	pc = (pc & 0xFFFF0000) | dest;
}

FUNC_INLINE void cpu_flags_arith_add(uint8_t dest, uint8_t src) {
	uint16_t ctest;
	/*if ((dest ^ result) & (src ^ result) & 0x80)
		SET_COND(COND_V);
	else
		CLEAR_COND(COND_V);*/

	if (result & 0x80)
		SET_COND(COND_N);
	else
		CLEAR_COND(COND_N);

	if (result == 0x00)
		SET_COND(COND_Z);
	else
		CLEAR_COND(COND_Z);

	ctest = (uint16_t)dest + (uint16_t)src;
	if (ctest & 0xFF00) //(result < dest)
		SET_COND(COND_C);
	else
		CLEAR_COND(COND_C);

	if (GET_COND(COND_C) ^ (BIT_D6 & BIT_S6 | BIT_S6 & BIT_R6 | BIT_R6 & BIT_D6))
		SET_COND(COND_V);
	else
		CLEAR_COND(COND_V);
}

FUNC_INLINE void cpu_flags_arith_add16(uint16_t dest, uint16_t src) {
	uint32_t ctest;
	/*if ((dest ^ result16) & (src ^ result16) & 0x8000)
		SET_COND(COND_V);
	else
		CLEAR_COND(COND_V);*/

	if (result16 & 0x8000)
		SET_COND(COND_N);
	else
		CLEAR_COND(COND_N);

	if (result16 == 0x0000)
		SET_COND(COND_Z);
	else
		CLEAR_COND(COND_Z);

	ctest = (uint32_t)dest + (uint32_t)src;
	if (ctest & 0xFFFF0000) //(result16 < dest)
		SET_COND(COND_C);
	else
		CLEAR_COND(COND_C);

	if (GET_COND(COND_C) ^ (BIT_D14 & BIT_S14 | BIT_S14 & BIT_RW14 | BIT_RW14 & BIT_D14))
		SET_COND(COND_V);
	else
		CLEAR_COND(COND_V);
}

FUNC_INLINE void cpu_flags_arith_sub(uint8_t dest, uint8_t src) {
	uint16_t ctest;
	//if ((dest ^ result) & (src ^ result) & 0x80)
	if ((BIT_D7 & BIT_S7 | BIT_D7 & BIT_R7 | BIT_D7 & BIT_S7 & BIT_R7) ^ (BIT_D6 & BIT_S6 | BIT_D6 & BIT_R6 | BIT_D6 & BIT_S6 & BIT_R6))
		SET_COND(COND_V);
	else
		CLEAR_COND(COND_V);

	if (result & 0x80)
		SET_COND(COND_N);
	else
		CLEAR_COND(COND_N);

	if (result == 0x00)
		SET_COND(COND_Z);
	else
		CLEAR_COND(COND_Z);

	ctest = (uint16_t)dest - (uint16_t)src;
	if (ctest & 0xFF00) //(result > dest)
		SET_COND(COND_C);
	else
		CLEAR_COND(COND_C);
}

FUNC_INLINE void cpu_flags_arith_sub16(uint16_t dest, uint16_t src) {
	uint32_t ctest;

	//if ((dest ^ result16) & (src ^ result16) & 0x8000)
	if ((BIT_D15 & BIT_S15 | BIT_D15 & BIT_RW15 | BIT_D15 & BIT_S15 & BIT_RW15) ^ (BIT_D14 & BIT_S14 | BIT_D14 & BIT_RW14 | BIT_D14 & BIT_S14 & BIT_RW14))
		SET_COND(COND_V);
	else
		CLEAR_COND(COND_V);

	if (result16 & 0x8000)
		SET_COND(COND_N);
	else
		CLEAR_COND(COND_N);

	if (result16 == 0x0000)
		SET_COND(COND_Z);
	else
		CLEAR_COND(COND_Z);

	ctest = (uint32_t)dest - (uint32_t)src;
	if (ctest & 0xFFFF0000) //(result16 > dest)
		SET_COND(COND_C);
	else
		CLEAR_COND(COND_C);
}

FUNC_INLINE void cpu_flags_logic(uint8_t val) {
	if (val & 0x80)
		SET_COND(COND_N);
	else
		CLEAR_COND(COND_N);

	if (val == 0x00)
		SET_COND(COND_Z);
	else
		CLEAR_COND(COND_Z);
}

FUNC_INLINE void cpu_flags_logic16(uint16_t val) {
	if (val & 0x8000)
		SET_COND(COND_N);
	else
		CLEAR_COND(COND_N);

	if (val == 0x0000)
		SET_COND(COND_Z);
	else
		CLEAR_COND(COND_Z);
}

//logical operations
FUNC_INLINE void cpu_op_and(uint8_t dest, uint8_t src) {
	result = dest & src;
	cpu_flags_logic(result);
}

FUNC_INLINE void cpu_op_or(uint8_t dest, uint8_t src) {
	result = dest | src;
	cpu_flags_logic(result);
}

FUNC_INLINE void cpu_op_xor(uint8_t dest, uint8_t src) {
	result = dest ^ src;
	cpu_flags_logic(result);
}

//arithmetic operations
FUNC_INLINE void cpu_op_add(uint8_t dest, uint8_t src) {
	result = dest + src;
	cpu_flags_arith_add(dest, src);
}

FUNC_INLINE void cpu_op_add16(uint16_t dest, uint16_t src) {
	result16 = dest + src;
	cpu_flags_arith_add16(dest, src);
}

FUNC_INLINE void cpu_op_inc(uint8_t dest) {
	uint8_t oldcc;
	oldcc = cc;
	result = dest + 1;
	cpu_flags_arith_add(dest, 1);
	cc = (cc & ~COND_C) | (oldcc & COND_C);
}

FUNC_INLINE void cpu_op_inc16(uint16_t dest) {
	uint8_t oldcc;
	oldcc = cc;
	result16 = dest + 1;
	cpu_flags_arith_add16(dest, 1);
	cc = (cc & ~COND_C) | (oldcc & COND_C);
}

FUNC_INLINE void cpu_op_sub(uint8_t dest, uint8_t src) {
	result = dest - src;
	cpu_flags_arith_sub(dest, src);
}

FUNC_INLINE void cpu_op_sub16(uint16_t dest, uint16_t src) {
	result16 = dest - src;
	cpu_flags_arith_sub16(dest, src);
}

FUNC_INLINE void cpu_op_dec(uint8_t dest) {
	uint8_t oldcc;
	oldcc = cc;
	result = dest - 1;
	cpu_flags_arith_sub(dest, 1);
	cc = (cc & ~COND_C) | (oldcc & COND_C);
}

FUNC_INLINE void cpu_op_dec16(uint16_t dest) {
	uint8_t oldcc;
	oldcc = cc;
	result16 = dest - 1;
	cpu_flags_arith_sub16(dest, 1);
	cc = (cc & ~COND_C) | (oldcc & COND_C);
}

//rotate operations
FUNC_INLINE void cpu_op_rlc(uint8_t dest) {
	uint8_t oldc;
	oldc = GET_COND(COND_C);
	result = (dest << 1) | oldc;

	if (dest & 0x80)
		SET_COND(COND_C);
	else
		CLEAR_COND(COND_C);

	cpu_flags_logic(result);
}

FUNC_INLINE void cpu_op_rlcw(uint16_t dest) {
	uint8_t oldc;
	oldc = GET_COND(COND_C);
	result16 = (dest << 1) | oldc;

	if (dest & 0x8000)
		SET_COND(COND_C);
	else
		CLEAR_COND(COND_C);

	cpu_flags_logic16(result16);
}

FUNC_INLINE void cpu_op_rrc(uint8_t dest) { //TODO: is this right?
	uint8_t oldc;
	oldc = GET_COND(COND_C);
	if (dest & 1)
		SET_COND(COND_C);
	else
		CLEAR_COND(COND_C);
	result = (dest >> 1) | (oldc << 7);
	cpu_flags_logic(result);
}

FUNC_INLINE void cpu_op_rrcw(uint16_t dest) { //TODO: is this right?
	uint8_t oldc;
	oldc = GET_COND(COND_C);
	if (dest & 1)
		SET_COND(COND_C);
	else
		CLEAR_COND(COND_C);
	result16 = (dest >> 1) | (oldc << 15);
	cpu_flags_logic16(result);
}

FUNC_INLINE void cpu_op_srl(uint8_t dest) {
	result = dest >> 1;

	if (dest & 1)
		SET_COND(COND_C);
	else
		CLEAR_COND(COND_C);

	cpu_flags_logic(result);
}

FUNC_INLINE void cpu_op_srlw(uint16_t dest) {
	result16 = dest >> 1;

	if (dest & 1)
		SET_COND(COND_C);
	else
		CLEAR_COND(COND_C);

	cpu_flags_logic16(result16);
}

FUNC_INLINE void cpu_op_sra(uint8_t dest) {
	uint8_t sign;
	sign = dest & 0x80;
	if (dest & 1)
		SET_COND(COND_C);
	else
		CLEAR_COND(COND_C);
	result = (dest >> 1) | sign;

	cpu_flags_logic(result);
}

FUNC_INLINE void cpu_op_sraw(uint16_t dest) {
	uint16_t sign;
	sign = dest & 0x8000;
	if (dest & 1)
		SET_COND(COND_C);
	else
		CLEAR_COND(COND_C);
	result16 = (dest >> 1) | sign;

	cpu_flags_logic16(result16);
}

FUNC_INLINE void cpu_op_sll(uint8_t dest) {
	result = dest << 1;

	if (dest & 0x80)
		SET_COND(COND_C);
	else
		CLEAR_COND(COND_C);

	cpu_flags_logic(result);
}

FUNC_INLINE void cpu_op_sllw(uint16_t dest) {
	result16 = dest << 1;

	if (dest & 0x8000)
		SET_COND(COND_C);
	else
		CLEAR_COND(COND_C);

	cpu_flags_logic16(result16);
}

FUNC_INLINE void cpu_op_rlwa(uint16_t dest) {
	uint8_t olda;
	olda = a;
	a = (uint8_t)(dest >> 8);
	result16 = (dest << 8) | (uint16_t)olda;
	cpu_flags_logic16(result16);
}

FUNC_INLINE void cpu_op_rrwa(uint16_t dest) {
	uint8_t olda;
	olda = a;
	a = (uint8_t)dest;
	result16 = (dest >> 8) | (uint16_t)olda << 8;
	cpu_flags_logic16(result16);
}

//misc operations
FUNC_INLINE void cpu_op_swap(uint8_t dest) {
	result = (dest >> 4) | (dest << 4);
	cpu_flags_logic(result);
}

FUNC_INLINE void cpu_op_swapw(uint16_t dest) {
	result16 = (dest >> 8) | (dest << 8);
	cpu_flags_logic16(result16);
}

//addressing modes
FUNC_INLINE uint32_t cpu_addr_indexed_sp() {
	return (uint32_t)cpu_imm8() + (uint32_t)sp;
}

FUNC_INLINE uint32_t cpu_addr_indexed_x() {
	return x; // &0xFF;
}

FUNC_INLINE uint32_t cpu_addr_indexed_y() {
	return y; // &0xFF;
}

FUNC_INLINE uint32_t cpu_addr_shortptr_indexed_x() {
	return (uint32_t)memory_read16(cpu_imm8()) + (uint32_t)x;
}

FUNC_INLINE uint32_t cpu_addr_shortptr_indexed_y() {
	return (uint32_t)memory_read16(cpu_imm8()) + (uint32_t)y;
}

FUNC_INLINE uint32_t cpu_addr_longptr_indexed_x() {
	return (uint32_t)memory_read16(cpu_imm16()) + (uint32_t)x;
}

FUNC_INLINE uint32_t cpu_addr_longptr_indexed_y() {
	return (uint32_t)memory_read16(cpu_imm16()) + (uint32_t)y;
}

FUNC_INLINE uint32_t cpu_addr_short_offset_indexed_x() {
	return (uint32_t)cpu_imm8() + (uint32_t)x; // ((uint32_t)x & 0xFF);
}

FUNC_INLINE uint32_t cpu_addr_short_offset_indexed_y() {
	return (uint32_t)cpu_imm8() + (uint32_t)y; // ((uint32_t)y & 0xFF);
}

FUNC_INLINE uint32_t cpu_addr_long_offset_indexed_x() {
	return (uint32_t)cpu_imm16() + (uint32_t)x;
}

FUNC_INLINE uint32_t cpu_addr_long_offset_indexed_y() {
	return (uint32_t)cpu_imm16() + (uint32_t)y;
}

FUNC_INLINE uint32_t cpu_addr_longptr() {
	return memory_read16(cpu_imm16());
}

FUNC_INLINE uint32_t cpu_addr_shortptr() {
	return memory_read16(cpu_imm8());
}

int32_t cpu_run(int32_t clocks) {
	uint8_t val;
	uint16_t val16;
	int32_t startclocks; //temporary thing until i finish adding all proper clock timings...

	while ((clocks > 0) && running) {
		if (anyirq == 1) {
			if (!(GET_COND(COND_I0) && GET_COND(COND_I1))) { //if interrupts are enabled, check for any in the queue
				uint8_t i;
				for (i = 0; i < 32; i++) {
					if (irq_req[i] == 1) {
						uint8_t spr;

						//Push current register context to stack
						cpu_push((uint8_t)pc);
						cpu_push((uint8_t)(pc >> 8));
						cpu_push((uint8_t)(pc >> 16));
						cpu_push((uint8_t)y);
						cpu_push((uint8_t)(y >> 8));
						cpu_push((uint8_t)x);
						cpu_push((uint8_t)(x >> 8));
						cpu_push(a);
						cpu_push(cc);


						//Get interrupt bits from SPR table and set them in CC register before transferring control to interrupt vector
						spr = memory_read(ITC_SPR1 + (i >> 2));
						spr = (spr >> ((i & 3) << 1)) & 3;
						if (spr != 2) {
							if (spr & 1) SET_COND(COND_I0); else CLEAR_COND(COND_I0);
							if (spr & 2) SET_COND(COND_I1); else CLEAR_COND(COND_I1);
						}

						//Transfer control to interrupt vector
						pc = 0x8000 + (uint32_t)(2 + i) * 4;
						irq_req[i] = 0; //mark the IRQ as serviced
						break; //don't do another IRQ at the same time...
					}
				}
				if (i == 32) anyirq = 0;
			}
		}

		startclocks = clocks;
		prefix = 0;
		valid = 1;
		origpc = pc;
	start:
		pc &= 0x00FFFFFF;
		opcode = memory_read(pc);
#ifdef DEBUG_OUTPUT
		printf("Op @ %08X = %02X\n", pc, opcode);
#endif
		switch (opcode) {
			//prefixes
		case 0x72:
		case 0x90:
		case 0x91:
		case 0x92:
			prefix = opcode;
			pc++;
			goto start;
		}

		oppc = pc;

		if (((opcode & 0xF0) == 0x10) && ((prefix == 0x72) || (prefix == 0x90))) { //bit operations
			uint8_t bnum;
			bnum = (opcode & 0x0F) >> 1;
			addr = cpu_imm16();
			val = memory_read(addr);
			if (prefix == 0x72) {
				if (opcode & 1) { //BRES
					val &= ~(1 << bnum);
				}
				else { //BSET
					val |= 1 << bnum;
				}
			}
			else { //prefix == 0x90
				if (opcode & 1) { //BCCM
					val &= ~(1 << bnum);
					val |= (GET_COND(COND_C) << bnum);
				}
				else { //BCPL
					val ^= 1 << bnum;
				}
			}
			memory_write(addr, val);
			clocks -= 1;
			pc++;
		}
		else if (((opcode & 0xF0) == 0x00) && (prefix == 0x72)) { //bit test and jump operations
			uint8_t bnum;
			bnum = (opcode & 0x0F) >> 1;
			addr = cpu_imm16();
			val = memory_read(addr);
			addr = cpu_soff8();
			if (val & (1 << bnum))
				SET_COND(COND_C);
			else
				CLEAR_COND(COND_C);
			pc++;
			if (opcode & 1) { //BTJF
				if (!GET_COND(COND_C)) {
					pc += addr;
					clocks -= 1;
				}
			}
			else { //BTJT
				if (GET_COND(COND_C)) {
					pc += addr;
					clocks -= 1;
				}
			}
			clocks -= 2;
		}
		else switch (opcode) { //other opcodes
		case 0x00: //NEG (addr8,SP)
			addr = cpu_addr_indexed_sp();
			cpu_op_add(memory_read(addr) ^ 0xFF, 1);
			memory_write(addr, result);
			pc++;
			break;
		case 0x01:
			if (prefix == 0x90) { //RRWA Y
				cpu_op_rrwa(y);
				y = result16;
			}
			else { //RRWA X
				cpu_op_rrwa(x);
				x = result16;
			}
			pc++;
			break;
		case 0x02:
			if (prefix == 0x90) { //RLWA Y
				cpu_op_rlwa(y);
				y = result16;
			}
			else { //RLWA X
				cpu_op_rlwa(x);
				x = result16;
			}
			pc++;
			break;
		case 0x03: //CPL (addr8,SP)
			addr = cpu_addr_indexed_sp();
			result = memory_read(addr) ^ 0xFF;
			memory_write(addr, result);
			cpu_flags_logic(result);
			SET_COND(COND_C);
			pc++;
			break;
		case 0x04: //SRL (addr8,SP)
			addr = cpu_addr_indexed_sp();
			cpu_op_srl(memory_read(addr));
			memory_write(addr, result);
			pc++;
			break;
		case 0x06: //RRC (addr8,SP)
			addr = cpu_addr_indexed_sp();
			cpu_op_rrc(memory_read(addr));
			memory_write(addr, result);
			pc++;
			break;
		case 0x07: //SRA (addr8,SP)
			addr = cpu_addr_indexed_sp();
			cpu_op_sra(memory_read(addr));
			memory_write(addr, result);
			pc++;
			break;
		case 0x08: //SLL (addr8,SP)
			addr = cpu_addr_indexed_sp();
			cpu_op_sll(memory_read(addr));
			memory_write(addr, result);
			pc++;
			break;
		case 0x09: //RLC (addr8,SP)
			addr = cpu_addr_indexed_sp();
			cpu_op_rlc(memory_read(addr));
			memory_write(addr, result);
			pc++;
			break;
		case 0x0A: //DEC (addr8,SP)
			addr = cpu_addr_indexed_sp();
			cpu_op_dec(memory_read(addr));
			memory_write(addr, result);
			pc++;
			break;

		case 0x0C: //INC (addr8,SP)
			addr = cpu_addr_indexed_sp();
			cpu_op_inc(memory_read(addr));
			memory_write(addr, result);
			pc++;
			break;
		case 0x0D: //TNZ (addr8,SP)
			addr = cpu_addr_indexed_sp();
			cpu_flags_logic(memory_read(addr));
			pc++;
			break;
		case 0x0E: //SWAP (addr8,SP)
			addr = cpu_addr_indexed_sp();
			cpu_op_swap(memory_read(addr));
			memory_write(addr, result);
			pc++;
			break;
		case 0x0F: //CLR (addr8,SP)
			addr = cpu_addr_indexed_sp();
			memory_write(addr, 0);
			CLEAR_COND(COND_N);
			SET_COND(COND_Z);
			clocks -= 1;
			pc++;
			break;
		case 0x10: //SUB A,(addr8,SP)
			addr = cpu_addr_indexed_sp();
			cpu_op_sub(a, memory_read(addr));
			a = result;
			pc++;
			break;
		case 0x11: //CP A,(addr8,SP)
			addr = cpu_addr_indexed_sp();
			cpu_op_sub(a, memory_read(addr));
			pc++;
			break;
		case 0x12: //SBC A,(addr8,SP)
			addr = cpu_addr_indexed_sp();
			cpu_op_sub(a, memory_read(addr) + GET_COND(COND_C));
			a = result;
			pc++;
			break;
		case 0x13: //CPW X,(addr8,SP)
			addr = cpu_addr_indexed_sp();
			cpu_op_sub16(x, memory_read16(addr));
			clocks -= 2;
			pc++;
			break;
		case 0x14: //AND A,(addr8,SP)
			addr = cpu_addr_indexed_sp();
			a &= memory_read(addr);
			cpu_flags_logic16(a);
			clocks -= 1;
			pc++;
			break;
		case 0x15: //BCP A,(addr8,SP)
			addr = cpu_addr_indexed_sp();
			cpu_op_and(a, memory_read(addr));
			clocks -= 1;
			pc++;
			break;
		case 0x16: //LDW Y,(addr8,SP)
			addr = cpu_addr_indexed_sp();
			y = memory_read16(addr);
			cpu_flags_logic16(y);
			clocks -= 2;
			pc++;
			break;
		case 0x17: //LDW (addr8,SP),Y
			addr = cpu_addr_indexed_sp();
			memory_write16(addr, y);
			cpu_flags_logic16(y);
			clocks -= 2;
			pc++;
			break;
		case 0x18: //XOR A,(addr8,SP)
			addr = cpu_addr_indexed_sp();
			cpu_op_xor(a, memory_read(addr));
			a = result;
			clocks -= 1;
			pc++;
			break;
		case 0x19: //ADC A,(addr8,SP)
			addr = cpu_addr_indexed_sp();
			cpu_op_add(a, memory_read(addr) + GET_COND(COND_C));
			a = result;
			clocks -= 1;
			pc++;
			break;
		case 0x1A: //OR A,(addr8,SP)
			addr = cpu_addr_indexed_sp();
			cpu_op_or(a, memory_read(addr));
			a = result;
			clocks -= 1;
			pc++;
			break;
		case 0x1B: //ADD A,(addr8,SP)
			addr = cpu_addr_indexed_sp();
			cpu_op_add(a, memory_read(addr));
			a = result;
			clocks -= 1;
			pc++;
			break;
		case 0x1C: //ADDW X,#imm16
			cpu_op_add16(x, cpu_imm16());
			x = result16;
			clocks -= 2;
			pc++;
			break;
		case 0x1D: //SUBW X,#imm16
			cpu_op_sub16(x, cpu_imm16());
			x = result16;
			clocks -= 2;
			pc++;
			break;
		case 0x1E: //LDW X,(addr8,SP)
			addr = cpu_addr_indexed_sp();
			x = memory_read16(addr);
			cpu_flags_logic16(x);
			clocks -= 2;
			pc++;
			break;
		case 0x1F: //LDW (addr8,SP),X
			addr = cpu_addr_indexed_sp();
			memory_write16(addr, x);
			cpu_flags_logic16(x);
			clocks -= 2;
			pc++;
			break;
		case 0x20: //JRA/JRT
			addr = cpu_soff8();
			clocks -= 2;
			pc = pc + addr + 1;
			break;
		case 0x21: //JRF
			clocks -= 1;
			pc += 2; //branch never, but still need to increment PC
			break;
		case 0x22: //JRUGT
			addr = cpu_soff8();
			clocks -= 1;
			pc++;
			if(!GET_COND(COND_C) && !GET_COND(COND_Z)) {
				clocks -= 1;
				pc = pc + addr;
			}
			break;
		case 0x23: //JRULE
			addr = cpu_soff8();
			clocks -= 1;
			pc++;
			if(GET_COND(COND_C) || GET_COND(COND_Z)) {
				clocks -= 1;
				pc = pc + addr;
			}
			break;
		case 0x24: //JRNC/JRUGE
			addr = cpu_soff8();
			clocks -= 1;
			pc++;
			if(!GET_COND(COND_C)) {
				clocks -= 1;
				pc = pc + addr;
			}
			break;
		case 0x25: //JRULT/JRC
			addr = cpu_soff8();
			clocks -= 1;
			pc++;
			if(GET_COND(COND_C)) {
				clocks -= 1;
				pc = pc + addr;
			}
			break;
		case 0x26: //JRNE
			addr = cpu_soff8();
			clocks -= 1;
			pc++;
			if(!GET_COND(COND_Z)) {
				clocks -= 1;
				pc = pc + addr;
			}
			break;
		case 0x27: //JREQ
			addr = cpu_soff8();
			clocks -= 1;
			pc++;
			if(GET_COND(COND_Z)) {
				clocks -= 1;
				pc = pc + addr;
			}
			break;
		case 0x28:
			addr = cpu_soff8();
			clocks -= 1;
			pc++;
			if (prefix == 90) { //JRNH
				if(!GET_COND(COND_H)) {
					clocks -= 1;
					pc = pc + addr;
				}
			}
			else { //JRNV
				if(!GET_COND(COND_V)) {
					clocks -= 1;
					pc = pc + addr;
				}
			}
			break;
		case 0x29:
			addr = cpu_soff8();
			clocks -= 1;
			pc++;
			if (prefix == 90) { //JRH
				if(GET_COND(COND_H)) {
					clocks -= 1;
					pc = pc + addr;
				}
			}
			else { //JRV
				if(GET_COND(COND_V)) {
					clocks -= 1;
					pc = pc + addr;
				}
			}
			break;
		case 0x2A: //JRPL
			addr = cpu_soff8();
			clocks -= 1;
			pc++;
			if(!GET_COND(COND_N)) {
				clocks -= 1;
				pc = pc + addr;
			}
			break;
		case 0x2B: //JRMI
			addr = cpu_soff8();
			clocks -= 1;
			pc++;
			if(GET_COND(COND_N)) {
				clocks -= 1;
				pc = pc + addr;
			}
			break;
		case 0x2C:
			addr = cpu_soff8();
			clocks -= 1;
			pc++;
			if (prefix == 90) { //JRNM
				if(!(GET_COND(COND_I0) || GET_COND(COND_I1))) {
					clocks -= 1;
					pc = pc + addr;
				}
			}
			else { //JRSGT
				// TODO: check, condition doesn't match datasheet
				if(!GET_COND(COND_N) && (GET_COND(COND_N) == GET_COND(COND_V))) {
					clocks -= 1;
					pc = pc + addr;
				}
			}
			break;
		case 0x2D:
			addr = cpu_soff8();
			clocks -= 1;
			pc++;
			if (prefix == 90) { //JRM
				if(GET_COND(COND_I0) || GET_COND(COND_I1)) {
					clocks -= 1;
					pc = pc + addr;
				}
			}
			else { //JRSLE
				// TODO: check, condition doesn't match datasheet
				if(GET_COND(COND_N) || (GET_COND(COND_N) ^ GET_COND(COND_V))) {
					clocks -= 1;
					pc = pc + addr;
				}
			}
			break;
		case 0x2E:
			addr = cpu_soff8();
			clocks -= 1;
			pc++;
			if (prefix == 90) { //JRIL
				// This instruction doesn't actually work according to errata.
				// JRIL is equivalent to an unconditional jump.
				// See ES0110, ES0102, or ES036 documents section 2.1.2.
				clocks -= 1;
				pc = pc + addr;
			}
			else { //JRSGE
				if((GET_COND(COND_N) ^ GET_COND(COND_V)) == 0) {
					clocks -= 1;
					pc = pc + addr;
				}
			}
			break;
		case 0x2F:
			addr = cpu_soff8();
			clocks -= 1;
			pc++;
			if (prefix == 90) { //JRIH
				// This instruction doesn't actually work according to errata.
				// JRIH is equivalent to NOP.
				// See ES0110, ES0102, or ES036 documents section 2.1.2.
			}
			else { //JRSLT
				if(GET_COND(COND_N) ^ GET_COND(COND_V)) {
					clocks -= 1;
					pc = pc + addr;
				}
			}
			break;
		case 0x30:
			if(prefix == 0x72) { //NEG [longptr.w]
				addr = cpu_addr_longptr();
				clocks -= 4;
			}
			else if(prefix == 0x92) { //NEG [shortptr.w]
				addr = cpu_addr_shortptr();
				clocks -= 4;
			}
			else { //NEG addr8
				addr = cpu_imm8();
				clocks -= 1;
			}
			val = memory_read(addr);
			cpu_op_add(val ^ 0xFF, 1);
			memory_write(addr, result);
			pc++;
			break;
		case 0x31: //EXG A,addr16
			addr = cpu_imm16();
			val = a;
			a = memory_read(addr);
			memory_write(addr, a);
			clocks -= 3;
			pc++;
			break;
		case 0x32: //POP addr16
			addr = cpu_imm16();
			result = cpu_pop();
			memory_write(addr, result);
			pc++;
			break;
		case 0x33:
			if(prefix == 0x72) { //CPL [longptr.w]
				addr = cpu_addr_longptr();
				clocks -= 4;
			}
			else if(prefix == 0x92) { //CPL [shortptr.w]
				addr = cpu_addr_shortptr();
				clocks -= 4;
			}
			else { //CPL addr8
				addr = cpu_imm8();
				clocks -= 1;
			}
			result = memory_read(addr) ^ 0xFF;
			cpu_flags_logic(result);
			memory_write(addr, result);
			SET_COND(COND_C);
			pc++;
			break;
		case 0x34:
			if(prefix == 0x72) { //SRL [longptr.w]
				addr = cpu_addr_longptr();
				clocks -= 4;
			}
			else if(prefix == 0x92) { //SRL [shortptr.w]
				addr = cpu_addr_shortptr();
				clocks -= 4;
			}
			else { //SRL addr8
				addr = cpu_imm8();
				clocks -= 1;
			}
			cpu_op_srl(memory_read(addr));
			memory_write(addr, result);
			pc++;
			break;
		case 0x35: //MOV addr16,#imm8
			val = cpu_imm8();
			addr = cpu_imm16();
			memory_write(addr, val);
			pc++;
			break;
		case 0x36:
			if(prefix == 0x72) { //RRC [longptr.w]
				addr = cpu_addr_longptr();
				clocks -= 4;
			}
			else if(prefix == 0x92) { //RRC [shortptr.w]
				addr = cpu_addr_shortptr();
				clocks -= 4;
			}
			else { //RRC addr8
				addr = cpu_imm8();
				clocks -= 1;
			}
			cpu_op_rrc(memory_read(addr));
			memory_write(addr, result);
			pc++;
			break;
		case 0x37:
			if(prefix == 0x72) { //SRA [longptr.w]
				addr = cpu_addr_longptr();
				clocks -= 4;
			}
			else if(prefix == 0x92) { //SRA [shortptr.w]
				addr = cpu_addr_shortptr();
				clocks -= 4;
			}
			else { //SRA addr8
				addr = cpu_imm8();
				clocks -= 1;
			}
			cpu_op_sra(memory_read(addr));
			memory_write(addr, result);
			pc++;
			break;
		case 0x38:
			if(prefix == 0x72) { //SLL [longptr.w]
				addr = cpu_addr_longptr();
				clocks -= 4;
			}
			else if(prefix == 0x92) { //SLL [shortptr.w]
				addr = cpu_addr_shortptr();
				clocks -= 4;
			}
			else { //SLL addr8
				addr = cpu_imm8();
				clocks -= 1;
			}
			cpu_op_sll(memory_read(addr));
			memory_write(addr, result);
			pc++;
			break;
		case 0x39:
			if(prefix == 0x72) { //RLC [longptr.w]
				addr = cpu_addr_longptr();
				clocks -= 4;
			}
			else if(prefix == 0x92) { //RLC [shortptr.w]
				addr = cpu_addr_shortptr();
				clocks -= 4;
			}
			else { //RLC addr8
				addr = cpu_imm8();
				clocks -= 1;
			}
			cpu_op_rlc(memory_read(addr));
			memory_write(addr, result);
			pc++;
			break;
		case 0x3A:
			if(prefix == 0x72) { //DEC [longptr.w]
				addr = cpu_addr_longptr();
				clocks -= 4;
			}
			else if(prefix == 0x92) { //DEC [shortptr.w]
				addr = cpu_addr_shortptr();
				clocks -= 4;
			}
			else { //DEC addr8
				addr = cpu_imm8();
				clocks -= 1;
			}
			cpu_op_dec(memory_read(addr));
			memory_write(addr, result);
			pc++;
			break;
		case 0x3B: //PUSH addr16
			addr = cpu_imm16();
			cpu_push(memory_read(addr));
			pc++;
			break;
		case 0x3C:
			if(prefix == 0x72) { //INC [longptr.w]
				addr = cpu_addr_longptr();
				clocks -= 4;
			}
			else if(prefix == 0x92) { //INC [shortptr.w]
				addr = cpu_addr_shortptr();
				clocks -= 4;
			}
			else { //INC addr8
				addr = cpu_imm8();
				clocks -= 1;
			}
			cpu_op_inc(memory_read(addr));
			memory_write(addr, result);
			pc++;
			break;
		case 0x3D:
			if(prefix == 0x72) { //TNZ [longptr.w]
				addr = cpu_addr_longptr();
				clocks -= 4;
			}
			else if(prefix == 0x92) { //TNZ [shortptr.w]
				addr = cpu_addr_shortptr();
				clocks -= 4;
			}
			else { //TNZ addr8
				addr = cpu_imm8();
				clocks -= 1;
			}
			cpu_flags_logic(memory_read(addr));
			pc++;
			break;
		case 0x3E:
			if(prefix == 0x72) { //SWAP [longptr.w]
				addr = cpu_addr_longptr();
				clocks -= 4;
			}
			else if(prefix == 0x92) { //SWAP [shortptr.w]
				addr = cpu_addr_shortptr();
				clocks -= 4;
			}
			else { //SWAP addr8
				addr = cpu_imm8();
				clocks -= 1;
			}
			cpu_op_swap(memory_read(addr));
			memory_write(addr, result);
			pc++;
			break;
		case 0x3F:
			if (prefix == 0x72) { //CLR [longptr.w]
				addr = cpu_addr_longptr();
				clocks -= 4;
			}
			else if (prefix == 0x92) { //CLR [shortptr.w]
				addr = cpu_addr_shortptr();
				clocks -= 4;
			}
			else { //CLR addr8
				addr = cpu_imm8();
				clocks -= 1;
			}
			memory_write(addr, 0);
			CLEAR_COND(COND_N);
			SET_COND(COND_Z);
			pc++;
			break;
		case 0x40:
			if (prefix == 0x90) { //NEG (addr16,Y)
				addr = (uint32_t)cpu_imm16() + (uint32_t)y;
				cpu_op_add(memory_read(addr) ^ 0xFF, 1);
				memory_write(addr, result);
			}
			else if (prefix == 0x72) { //NEG (addr16,X)
				addr = (uint32_t)cpu_imm16() + (uint32_t)x;
				cpu_op_add(memory_read(addr) ^ 0xFF, 1);
				memory_write(addr, result);
			}
			else { //NEG A
				cpu_op_add(a ^ 0xFF, 1);
				a = result;
			}
			pc++;
			break;
		case 0x41: //EXG A,XL
			val = a;
			a = (uint8_t)x;
			x = (x & 0xFF00) | val;
			pc++;
			break;
		case 0x42:
			if (prefix == 0x90) //MUL Y,A
				y = (y & 0x00FF) * (uint16_t)a;
			else //MUL X,A
				x = (x & 0x00FF) * (uint16_t)a;
			CLEAR_COND(COND_H); //TODO: Verify this...
			CLEAR_COND(COND_C);
			clocks -= 4;
			pc++;
			break;
		case 0x43:
			if (prefix == 0x90) { //CPL (addr16,Y)
				addr = (uint32_t)cpu_imm16() + (uint32_t)y;
				result = memory_read(addr) ^ 0xFF;
				memory_write(addr, result);
				cpu_flags_logic(result);
			}
			else if (prefix == 0x72) { //CPL (addr16,X)
				addr = (uint32_t)cpu_imm16() + (uint32_t)x;
				result = memory_read(addr) ^ 0xFF;
				memory_write(addr, result);
				cpu_flags_logic(result);
			}
			else { //CPL A
				a ^= 0xFF;
				cpu_flags_logic(a);
			}
			SET_COND(COND_C);
			pc++;
			break;
		case 0x44:
			if (prefix == 0x90) { //SRL (addr16,Y)
				addr = (uint32_t)cpu_imm16() + (uint32_t)y;
				val = memory_read(addr);
				cpu_op_srl(val);
				memory_write(addr, result);
			}
			else if (prefix == 0x72) { //SRL (addr16,X)
				addr = (uint32_t)cpu_imm16() + (uint32_t)x;
				val = memory_read(addr);
				cpu_op_srl(val);
				memory_write(addr, result);
			}
			else { //SRL A
				cpu_op_srl(a);
				a = result;
			}
			pc++;
			break;
		case 0x45: //MOV addr8,addr8
			addr = cpu_imm8();
			result = memory_read(addr);
			addr = cpu_imm8();
			memory_write(addr, result);
			pc++;
			break;
		case 0x46:
			if (prefix == 0x90) { //RRC (addr16,Y)
				addr = (uint32_t)cpu_imm16() + (uint32_t)y;
				cpu_op_rrc(memory_read(addr));
				memory_write(addr, result);
			}
			else if (prefix == 0x72) { //RRC (addr16,X)
				addr = (uint32_t)cpu_imm16() + (uint32_t)x;
				cpu_op_rrc(memory_read(addr));
				memory_write(addr, result);
			}
			else { //RRC A
				cpu_op_rrc(a);
				a = result;
			}
			pc++;
			break;
		case 0x47:
			if (prefix == 0x90) { //SRA (addr16,Y)
				addr = (uint32_t)cpu_imm16() + (uint32_t)y;
				cpu_op_sra(memory_read(addr));
				memory_write(addr, result);
			}
			else if (prefix == 0x72) { //SRA (addr16,X)
				addr = (uint32_t)cpu_imm16() + (uint32_t)x;
				cpu_op_sra(memory_read(addr));
				memory_write(addr, result);
			}
			else { //SRA A
				cpu_op_sra(a);
				a = result;
			}
			pc++;
			break;
		case 0x48:
			if (prefix == 0x90) { //SLL (addr16,Y)
				addr = (uint32_t)cpu_imm16() + (uint32_t)y;
				cpu_op_sll(memory_read(addr));
				memory_write(addr, result);
			}
			else if (prefix == 0x72) { //SLL (addr16,X)
				addr = (uint32_t)cpu_imm16() + (uint32_t)x;
				cpu_op_sll(memory_read(addr));
				memory_write(addr, result);
			}
			else { //SLL A
				cpu_op_sll(a);
				a = result;
			}
			pc++;
			break;
		case 0x49:
			if (prefix == 0x90) { //RLC (addr16,Y)
				addr = (uint32_t)cpu_imm16() + (uint32_t)y;
				cpu_op_rlc(memory_read(addr));
				memory_write(addr, result);
			}
			else if (prefix == 0x72) { //RLC (addr16,X)
				addr = (uint32_t)cpu_imm16() + (uint32_t)x;
				cpu_op_rlc(memory_read(addr));
				memory_write(addr, result);
			}
			else { //RLC A
				cpu_op_rlc(a);
				a = result;
			}
			pc++;
			break;
		case 0x4A:
			if (prefix == 0x90) { //DEC (addr16,Y)
				addr = (uint32_t)cpu_imm16() + (uint32_t)y;
				cpu_op_dec(memory_read(addr));
				memory_write(addr, result);
			}
			else if (prefix == 0x72) { //DEC (addr16,X)
				addr = (uint32_t)cpu_imm16() + (uint32_t)x;
				cpu_op_dec(memory_read(addr));
				memory_write(addr, result);
			}
			else { //DEC A
				cpu_op_dec(a);
				a = result;
			}
			clocks -= 1;
			pc++;
			break;
		case 0x4B: //PUSH #imm8
			cpu_push(cpu_imm8());
			pc++;
			break;
		case 0x4C:
			if (prefix == 0x90) { //INC (addr16,Y)
				addr = (uint32_t)cpu_imm16() + (uint32_t)y;
				cpu_op_inc(memory_read(addr));
				memory_write(addr, result);
			}
			else if (prefix == 0x72) { //INC (addr16,X)
				addr = (uint32_t)cpu_imm16() + (uint32_t)x;
				cpu_op_inc(memory_read(addr));
				memory_write(addr, result);
			}
			else { //INC A
				cpu_op_inc(a);
				a = result;
			}
			clocks -= 1;
			pc++;
			break;
		case 0x4D:
			if (prefix == 0x90) { //TNZ (addr16,Y)
				addr = (uint32_t)cpu_imm16() + (uint32_t)y;
				val = memory_read(addr);
			}
			else if (prefix == 0x72) { //TNZ (addr16,X)
				addr = (uint32_t)cpu_imm16() + (uint32_t)x;
				val = memory_read(addr);
			}
			else //TNZ A
				val = a;
			cpu_flags_logic(val);
			pc++;
			break;
		case 0x4E:
			if (prefix == 0x90) { //SWAP (addr16,Y)
				addr = (uint32_t)cpu_imm16() + (uint32_t)y;
				cpu_op_swap(memory_read(addr));
				memory_write(addr, result);
			}
			else if (prefix == 0x72) { //SWAP (addr16,X)
				addr = (uint32_t)cpu_imm16() + (uint32_t)x;
				cpu_op_swap(memory_read(addr));
				memory_write(addr, result);
			}
			else { //SWAP A
				cpu_op_swap(a);
				a = result;
			}
			pc++;
			break;
		case 0x4F:
			if (prefix == 0x90) { //CLR (addr16,Y)
				addr = (uint32_t)cpu_imm16() + (uint32_t)y;
				memory_write(addr, 0);
			}
			else if (prefix == 0x72) { //CLR (addr16,X)
				addr = (uint32_t)cpu_imm16() + (uint32_t)x;
				memory_write(addr, 0);
			}
			else //CLR A
				a = 0;
			CLEAR_COND(COND_N);
			SET_COND(COND_Z);
			clocks -= 1;
			pc++;
			break;
		case 0x50:
			if (prefix == 0x72) { //NEG addr16
				addr = cpu_imm16();
				val = memory_read(addr);
				cpu_op_add(val ^ 0xFF, 1);
				memory_write(addr, result);
				clocks -= 1;
			}
			else if (prefix == 0x90) { //NEGW Y
				cpu_op_add16(y ^ 0xFFFF, 1);
				y = result16;
				clocks -= 2;
			}
			else { //NEGW X
				cpu_op_add16(x ^ 0xFFFF, 1);
				x = result16;
				clocks -= 2;
			}
			pc++;
			break;
		case 0x51: //EXGW X,Y
		{
			uint16_t tx;
			tx = x;
			x = y;
			y = tx;
			pc++;
			break;
		}
		case 0x52: //SUB SP,#imm8
			sp -= (uint16_t)cpu_imm8();
			pc++;
			break;
		case 0x53:
			if (prefix == 0x72) { //CPL addr16
				addr = cpu_imm16();
				result = memory_read(addr) ^ 0xFF;
				memory_write(addr, result);
				cpu_flags_logic(result);
				clocks -= 1;
			}
			else if (prefix == 0x90) { //CPLW Y
				y ^= 0xFFFF;
				cpu_flags_logic16(y);
				clocks -= 2;
			}
			else { //CPLW X
				x ^= 0xFFFF;
				cpu_flags_logic16(x);
				clocks -= 2;
			}
			SET_COND(COND_C);
			pc++;
			break;
		case 0x54:
			if (prefix == 0x72) { //SRL addr16
				addr = cpu_imm16();
				cpu_op_srl(memory_read(addr));
				memory_write(addr, result);
				clocks -= 1;
			}
			else if (prefix == 0x90) { //SRLW Y
				cpu_op_srlw(y);
				y = result16;
				clocks -= 2;
			}
			else { //SRLW X
				cpu_op_srlw(x);
				x = result16;
				clocks -= 2;
			}
			pc++;
			break;
		case 0x55: //MOV addr16,addr16
			addr = cpu_imm16();
			result = memory_read(addr);
			addr = cpu_imm16();
			memory_write(addr, result);
			pc++;
			break;
		case 0x56:
			if (prefix == 0x72) { //RRC addr16
				addr = cpu_imm16();
				val = memory_read(addr);
				cpu_op_rrc(val);
				memory_write(addr, result);
				clocks -= 1;
			}
			else if (prefix == 0x90) { //RRCW Y
				cpu_op_rrcw(y);
				y = result16;
				clocks -= 2;
			}
			else { //RRCW X
				cpu_op_rrcw(x);
				x = result16;
				clocks -= 2;
			}
			pc++;
			break;
		case 0x57:
			if (prefix == 0x72) { //SRA addr16
				addr = cpu_imm16();
				val = memory_read(addr);
				cpu_op_sra(val);
				memory_write(addr, result);
				clocks -= 1;
			}
			else if (prefix == 0x90) { //SRAW Y
				cpu_op_sraw(y);
				y = result16;
				clocks -= 2;
			}
			else { //SRAW X
				cpu_op_sraw(x);
				x = result16;
				clocks -= 2;
			}
			pc++;
			break;
		case 0x58:
			if (prefix == 0x72) { //SLL addr16
				addr = cpu_imm16();
				val = memory_read(addr);
				cpu_op_sll(val);
				memory_write(addr, result);
				clocks -= 1;
			}
			else if (prefix == 0x90) { //SLLW Y
				cpu_op_sllw(y);
				y = result16;
				clocks -= 2;
			}
			else { //SLLW X
				cpu_op_sllw(x);
				x = result16;
				clocks -= 2;
			}
			pc++;
			break;
		case 0x59:
			if (prefix == 0x72) { //RLC addr16
				addr = cpu_imm16();
				val = memory_read(addr);
				cpu_op_rlc(val);
				memory_write(addr, result);
				clocks -= 1;
			}
			else if (prefix == 0x90) { //RLCW Y
				cpu_op_rlcw(y);
				y = result16;
				clocks -= 2;
			}
			else { //RLCW X
				cpu_op_rlcw(x);
				x = result16;
				clocks -= 2;
			}
			pc++;
			break;
		case 0x5A:
			if (prefix == 0x72) { //DEC addr16
				addr = cpu_imm16();
				val = memory_read(addr);
				cpu_op_dec(val);
				memory_write(addr, result);
			}
			else if (prefix == 0x90) { //DECW Y
				cpu_op_dec16(y);
				y = result16;
			}
			else { //DECW X
				cpu_op_dec16(x);
				x = result16;
			}
			pc++;
			break;
		case 0x5B: //ADDW SP,#imm8
			sp += (uint16_t)cpu_imm8();
			clocks -= 2;
			pc++;
			break;
		case 0x5C:
			if (prefix == 0x72) { //INC addr16
				addr = cpu_imm16();
				val = memory_read(addr);
				cpu_op_inc(val);
				memory_write(addr, result);
			}
			else if (prefix == 0x90) { //INCW Y
				cpu_op_inc16(y);
				y = result16;
			}
			else { //INCW X
				cpu_op_inc16(x);
				x = result16;
			}
			pc++;
			break;
		case 0x5D:
			if (prefix == 0x72) { //TNZ addr16
				addr = cpu_imm16();
				val = memory_read(addr);
				cpu_flags_logic(val);
				clocks -= 1;
			}
			else if (prefix == 0x90) { //TNZW Y
				cpu_flags_logic16(y);
				clocks -= 2;
			}
			else { //TNZW X
				cpu_flags_logic16(x);
				clocks -= 2;
			}
			pc++;
			break;
		case 0x5E:
			if (prefix == 0x72) { //SWAP addr16
				addr = cpu_imm16();
				cpu_op_swap(memory_read(addr));
				memory_write(addr, result);
			}
			else if (prefix == 0x90) { //SWAPW Y
				cpu_op_swapw(y);
				y = result16;
			}
			else { //SWAPW X
				cpu_op_swapw(x);
				x = result16;
			}
			pc++;
			break;
		case 0x5F:
			if (prefix == 0x72) //CLR addr16
				memory_write(cpu_imm16(), 0);
			else if (prefix == 0x90) //CLRW Y
				y = 0;
			else //CLRW X
				x = 0;
			CLEAR_COND(COND_N);
			SET_COND(COND_Z);
			clocks -= 1;
			pc++;
			break;
		case 0x60:
			if(prefix == 0x91) { //NEG ([shortptr.w],Y)
				addr = cpu_addr_shortptr_indexed_y();
				clocks -= 4;
			}
			else if(prefix == 0x92) { //NEG ([shortptr.w],X)
				addr = cpu_addr_shortptr_indexed_x();
				clocks -= 4;
			}
			else if(prefix == 0x72) { //NEG ([longptr.w],X)
				addr = cpu_addr_longptr_indexed_x();
				clocks -= 4;
			}
			else if(prefix == 0x90) { //NEG (addr8,Y)
				addr = (uint32_t)cpu_imm8() + (uint32_t)y;
				clocks -= 1;
			}
			else { //NEG (addr8,X)
				addr = (uint32_t)cpu_imm8() + (uint32_t)x;
				clocks -= 1;
			}
			val = memory_read(addr);
			cpu_op_add(val ^ 0xFF, 1);
			memory_write(addr, result);
			pc++;
			break;
		case 0x61: //EXG A,YL
			val = a;
			a = (uint8_t)y;
			y = (y & 0xFF00) | val;
			pc++;
			break;
		case 0x62: //DIV X/Y,A
		{
			uint16_t quotient, remainder;
			CLEAR_COND(COND_V);
			CLEAR_COND(COND_H);
			CLEAR_COND(COND_N);
			if (a == 0) SET_COND(COND_C); else CLEAR_COND(COND_C);
			if (a != 0) {
				quotient = ((prefix == 0x90) ? y : x) / (uint16_t)a;
				remainder = ((prefix == 0x90) ? y : x) % (uint16_t)a;
				if (quotient == 0) SET_COND(COND_Z); else CLEAR_COND(COND_Z);
				if (prefix == 0x90)
					y = quotient;
				else
					x = quotient;
				a = (uint8_t)remainder;
			}
			// Instruction actually takes variable number of cycles, 2-17,
			// depending on values of operands. Until we know exactly how that
			// works, just use somewhere in the middle as a rough estimate.
			clocks -= 8;
			pc++;
			break;
		}
		case 0x63:
			if(prefix == 0x91) { //CPL ([shortptr.w],Y)
				addr = cpu_addr_shortptr_indexed_y();
				clocks -= 4;
			}
			else if(prefix == 0x72) { //CPL ([longptr.w],X)
				addr = cpu_addr_longptr_indexed_x();
				clocks -= 4;
			}
			else if(prefix == 0x92) { //CPL ([shortptr.w],X)
				addr = cpu_addr_shortptr_indexed_x();
				clocks -= 4;
			}
			else if(prefix == 0x90) { //CPL (addr8,Y)
				addr = (uint32_t)cpu_imm8() + (uint32_t)y;
				clocks -= 1;
			}
			else { //CPL (addr8,X)
				addr = (uint32_t)cpu_imm8() + (uint32_t)x;
				clocks -= 1;
			}
			result = memory_read(addr) ^ 0xFF;
			cpu_flags_logic(result);
			memory_write(addr, result);
			SET_COND(COND_C);
			pc++;
			break;
		case 0x64:
			if(prefix == 0x91) { //SRL ([shortptr.w],Y)
				addr = cpu_addr_shortptr_indexed_y();
				clocks -= 4;
			}
			else if(prefix == 0x72) { //SRL ([longptr.w],X)
				addr = cpu_addr_longptr_indexed_x();
				clocks -= 4;
			}
			else if(prefix == 0x92) { //SRL ([shortptr.w],X)
				addr = cpu_addr_shortptr_indexed_x();
				clocks -= 4;
			}
			else if(prefix == 0x90) { //SRL (addr8,Y)
				addr = (uint32_t)cpu_imm8() + (uint32_t)y;
				clocks -= 1;
			}
			else { //SRL (addr8,X)
				addr = (uint32_t)cpu_imm8() + (uint32_t)x;
				clocks -= 1;
			}
			cpu_op_srl(memory_read(addr));
			memory_write(addr, result);
			pc++;
			break;
		case 0x65: //DIVW
		{
			uint16_t tx, ty;
			CLEAR_COND(COND_V);
			CLEAR_COND(COND_H);
			CLEAR_COND(COND_N);
			if (y == 0) SET_COND(COND_C); else CLEAR_COND(COND_C);
			if (y != 0) {
				tx = x / y;
				ty = x % y;
				if (tx == 0) SET_COND(COND_Z); else CLEAR_COND(COND_Z);
				x = tx;
				y = ty;
			}
			// Instruction actually takes variable number of cycles, 2-17,
			// depending on values of operands. Until we know exactly how that
			// works, just use somewhere in the middle as a rough estimate.
			clocks -= 8;
			pc++;
			break;
		}
		case 0x66:
			if(prefix == 0x91) { //RRC ([shortptr.w],Y)
				addr = cpu_addr_shortptr_indexed_y();
				clocks -= 4;
			}
			else if(prefix == 0x72) { //RRC ([longptr.w],X)
				addr = cpu_addr_longptr_indexed_x();
				clocks -= 4;
			}
			else if(prefix == 0x92) { //RRC ([shortptr.w],X)
				addr = cpu_addr_shortptr_indexed_x();
				clocks -= 4;
			}
			else if(prefix == 0x90) { //RRC (addr8,Y)
				addr = (uint32_t)cpu_imm8() + (uint32_t)y;
				clocks -= 1;
			}
			else { //RRC (addr8,X)
				addr = (uint32_t)cpu_imm8() + (uint32_t)x;
				clocks -= 1;
			}
			cpu_op_rrc(memory_read(addr));
			memory_write(addr, result);
			pc++;
			break;
		case 0x67:
			if(prefix == 0x91) { //SRA ([shortptr.w],Y)
				addr = cpu_addr_shortptr_indexed_y();
				clocks -= 4;
			}
			else if(prefix == 0x72) { //SRA ([longptr.w],X)
				addr = cpu_addr_longptr_indexed_x();
				clocks -= 4;
			}
			else if(prefix == 0x92) { //SRA ([shortptr.w],X)
				addr = cpu_addr_shortptr_indexed_x();
				clocks -= 4;
			}
			else if(prefix == 0x90) { //SRA (addr8,Y)
				addr = (uint32_t)cpu_imm8() + (uint32_t)y;
				clocks -= 1;
			}
			else { //SRA (addr8,X)
				addr = (uint32_t)cpu_imm8() + (uint32_t)x;
				clocks -= 1;
			}
			cpu_op_sra(memory_read(addr));
			memory_write(addr, result);
			pc++;
			break;
		case 0x68:
			if(prefix == 0x91) { //SLL ([shortptr.w],Y)
				addr = cpu_addr_shortptr_indexed_y();
				clocks -= 4;
			}
			else if(prefix == 0x72) { //SLL ([longptr.w],X)
				addr = cpu_addr_longptr_indexed_x();
				clocks -= 4;
			}
			else if(prefix == 0x92) { //SLL ([shortptr.w],X)
				addr = cpu_addr_shortptr_indexed_x();
				clocks -= 4;
			}
			else if(prefix == 0x90) { //SLL (addr8,Y)
				addr = (uint32_t)cpu_imm8() + (uint32_t)y;
				clocks -= 1;
			}
			else { //SLL (addr8,X)
				addr = (uint32_t)cpu_imm8() + (uint32_t)x;
				clocks -= 1;
			}
			cpu_op_sll(memory_read(addr));
			memory_write(addr, result);
			pc++;
			break;
		case 0x69:
			if(prefix == 0x91) { //RLC ([shortptr.w],Y)
				addr = cpu_addr_shortptr_indexed_y();
				clocks -= 4;
			}
			else if(prefix == 0x72) { //RLC ([longptr.w],X)
				addr = cpu_addr_longptr_indexed_x();
				clocks -= 4;
			}
			else if(prefix == 0x92) { //RLC ([shortptr.w],X)
				addr = cpu_addr_shortptr_indexed_x();
				clocks -= 4;
			}
			else if(prefix == 0x90) { //RLC (addr8,Y)
				addr = (uint32_t)cpu_imm8() + (uint32_t)y;
				clocks -= 1;
			}
			else { //RLC (addr8,X)
				addr = (uint32_t)cpu_imm8() + (uint32_t)x;
				clocks -= 1;
			}
			cpu_op_rlc(memory_read(addr));
			memory_write(addr, result);
			pc++;
			break;
		case 0x6A:
			if(prefix == 0x91) { //DEC ([shortptr.w],Y)
				addr = cpu_addr_shortptr_indexed_y();
				clocks -= 4;
			}
			else if(prefix == 0x72) { //DEC ([longptr.w],X)
				addr = cpu_addr_longptr_indexed_x();
				clocks -= 4;
			}
			else if(prefix == 0x92) { //DEC ([shortptr.w],X)
				addr = cpu_addr_shortptr_indexed_x();
				clocks -= 4;
			}
			else if(prefix == 0x90) { //DEC (addr8,Y)
				addr = (uint32_t)cpu_imm8() + (uint32_t)y;
				clocks -= 1;
			}
			else { //DEC (addr8,X)
				addr = (uint32_t)cpu_imm8() + (uint32_t)x;
				clocks -= 1;
			}
			cpu_op_dec(memory_read(addr));
			memory_write(addr, result);
			pc++;
			break;
		case 0x6B: //LD (addr8,SP),A
			addr = (uint32_t)cpu_imm8() + (uint32_t)sp;
			memory_write(addr, a);
			cpu_flags_logic(a);
			pc++;
			break;
		case 0x6C:
			if(prefix == 0x91) { //INC ([shortptr.w],Y)
				addr = cpu_addr_shortptr_indexed_y();
				clocks -= 4;
			}
			else if(prefix == 0x72) { //INC ([longptr.w],X)
				addr = cpu_addr_longptr_indexed_x();
				clocks -= 4;
			}
			else if(prefix == 0x92) { //INC ([shortptr.w],X)
				addr = cpu_addr_shortptr_indexed_x();
				clocks -= 4;
			}
			else if(prefix == 0x90) { //INC (addr8,Y)
				addr = (uint32_t)cpu_imm8() + (uint32_t)y;
				clocks -= 1;
			}
			else { //INC (addr8,X)
				addr = (uint32_t)cpu_imm8() + (uint32_t)x;
				clocks -= 1;
			}
			cpu_op_inc(memory_read(addr));
			memory_write(addr, result);
			pc++;
			break;
		case 0x6D:
			if(prefix == 0x91) { //TNZ ([shortptr.w],Y)
				addr = cpu_addr_shortptr_indexed_y();
				clocks -= 4;
			}
			else if(prefix == 0x72) { //TNZ ([longptr.w],X)
				addr = cpu_addr_longptr_indexed_x();
				clocks -= 4;
			}
			else if(prefix == 0x92) { //TNZ ([shortptr.w],X)
				addr = cpu_addr_shortptr_indexed_x();
				clocks -= 4;
			}
			else if(prefix == 0x90) { //TNZ (addr8,Y)
				addr = (uint32_t)cpu_imm8() + (uint32_t)y;
				clocks -= 1;
			}
			else { //TNZ (addr8,X)
				addr = (uint32_t)cpu_imm8() + (uint32_t)x;
				clocks -= 1;
			}
			cpu_flags_logic(memory_read(addr));
			pc++;
			break;
		case 0x6E:
			if(prefix == 0x91) { //SWAP ([shortptr.w],Y)
				addr = cpu_addr_shortptr_indexed_y();
				clocks -= 4;
			}
			else if(prefix == 0x72) { //SWAP ([longptr.w],X)
				addr = cpu_addr_longptr_indexed_x();
				clocks -= 4;
			}
			else if(prefix == 0x92) { //SWAP ([shortptr.w],X)
				addr = cpu_addr_shortptr_indexed_x();
				clocks -= 4;
			}
			else if(prefix == 0x90) { //SWAP (addr8,Y)
				addr = (uint32_t)cpu_imm8() + (uint32_t)y;
				clocks -= 1;
			}
			else { //SWAP (addr8,X)
				addr = (uint32_t)cpu_imm8() + (uint32_t)x;
				clocks -= 1;
			}
			cpu_op_swap(memory_read(addr));
			memory_write(addr, result);
			pc++;
			break;
		case 0x6F:
			if (prefix == 0x91) { //CLR ([shortptr.w],Y)
				addr = cpu_addr_shortptr_indexed_y();
				clocks -= 4;
			}
			else if (prefix == 0x72) { //CLR ([longptr.w],X)
				addr = cpu_addr_longptr_indexed_x();
				clocks -= 4;
			}
			else if (prefix == 0x92) { //CLR ([shortptr.w],X)
				addr = cpu_addr_shortptr_indexed_x();
				clocks -= 4;
			}
			else if (prefix == 0x90) { //CLR (addr8,Y)
				addr = (uint32_t)cpu_imm8() + (uint32_t)y;
				clocks -= 1;
			}
			else { //CLR (addr8,X)
				addr = (uint32_t)cpu_imm8() + (uint32_t)x;
				clocks -= 1;
			}
			memory_write(addr, 0);
			CLEAR_COND(COND_N);
			SET_COND(COND_Z);
			pc++;
			break;
		case 0x70:
			if (prefix == 0x90) //NEG (Y)
				addr = cpu_addr_indexed_y();
			else //NEG (X)
				addr = cpu_addr_indexed_x();
			val = memory_read(addr);
			cpu_op_add(val ^ 0xFF, 1);
			memory_write(addr, result);
			pc++;
			break;

			//0x72 is a prefix
		case 0x73:
			if (prefix == 0x90) //CPL (Y)
				addr = cpu_addr_indexed_y();
			else //CPL (X)
				addr = cpu_addr_indexed_x();
			result = memory_read(addr) ^ 0xFF;
			cpu_flags_logic(result);
			memory_write(addr, result);
			SET_COND(COND_C);
			pc++;
			break;
		case 0x74:
			if (prefix == 0x90) //SRL (Y)
				addr = cpu_addr_indexed_y();
			else //SRL (X)
				addr = cpu_addr_indexed_x();
			cpu_op_srl(memory_read(addr));
			memory_write(addr, result);
			pc++;
			break;

		case 0x76:
			if (prefix == 0x90) //RRC (Y)
				addr = cpu_addr_indexed_y();
			else //RRC (X)
				addr = cpu_addr_indexed_x();
			cpu_op_rrc(memory_read(addr));
			memory_write(addr, result);
			pc++;
			break;
		case 0x77:
			if (prefix == 0x90) //SRA (Y)
				addr = cpu_addr_indexed_y();
			else //SRA (X)
				addr = cpu_addr_indexed_x();
			cpu_op_sra(memory_read(addr));
			memory_write(addr, result);
			pc++;
			break;
		case 0x78:
			if (prefix == 0x90) //SLL (Y)
				addr = cpu_addr_indexed_y();
			else //SLL (X)
				addr = cpu_addr_indexed_x();
			cpu_op_sll(memory_read(addr));
			memory_write(addr, result);
			pc++;
			break;
		case 0x79:
			if (prefix == 0x90) //RLC (Y)
				addr = cpu_addr_indexed_y();
			else //RLC (X)
				addr = cpu_addr_indexed_x();
			cpu_op_rlc(memory_read(addr));
			memory_write(addr, result);
			pc++;
			break;
		case 0x7A:
			if (prefix == 0x90) //DEC (Y)
				addr = cpu_addr_indexed_y();
			else //DEC (X)
				addr = cpu_addr_indexed_x();
			cpu_op_dec(memory_read(addr));
			memory_write(addr, result);
			pc++;
			break;
		case 0x7B: //LD A,(addr8,SP)
			addr = (uint32_t)cpu_imm8() + (uint32_t)sp;
			a = memory_read(addr);
			cpu_flags_logic(a);
			pc++;
			break;
		case 0x7C:
			if (prefix == 0x90) //INC (Y)
				addr = cpu_addr_indexed_y();
			else //INC (X)
				addr = cpu_addr_indexed_x();
			cpu_op_inc(memory_read(addr));
			memory_write(addr, result);
			pc++;
			break;
		case 0x7D:
			if (prefix == 0x90) //TNZ (Y)
				addr = cpu_addr_indexed_y();
			else //TNZ (X)
				addr = cpu_addr_indexed_x();
			val = memory_read(addr);
			cpu_flags_logic(val);
			pc++;
			break;
		case 0x7E:
			if (prefix == 0x90) //SWAP (Y)
				addr = cpu_addr_indexed_y();
			else //SWAP (X)
				addr = cpu_addr_indexed_x();
			cpu_op_swap(memory_read(addr));
			memory_write(addr, result);
			pc++;
			break;
		case 0x7F:
			if (prefix == 0x90) //CLR (Y)
				addr = cpu_addr_indexed_y();
			else //CLR (X)
				addr = cpu_addr_indexed_x();
			memory_write(addr, 0);
			CLEAR_COND(COND_N);
			SET_COND(COND_Z);
			clocks -= 1;
			pc++;
			break;
		case 0x80: //IRET
			cc = cpu_pop();
			a = cpu_pop();
			x = cpu_pop();
			x <<= 8;
			x |= cpu_pop();
			y = cpu_pop();
			y <<= 8;
			y |= cpu_pop();
			pc = cpu_pop();
			pc <<= 8;
			pc = cpu_pop();
			pc <<= 8;
			pc |= cpu_pop();
			clocks -= 11;
			break;
		case 0x81: //RET
			pc = cpu_pop();
			pc <<= 8;
			pc |= cpu_pop();
			clocks -= 4;
			break;
		case 0x82: //INT
			// TODO: change to use cpu_imm24()?
			addr = memory_read(pc + 1);
			addr <<= 8;
			addr |= memory_read(pc + 2);
			addr <<= 8;
			addr |= memory_read(pc + 3);
			clocks -= 2;
			pc = addr;
			break;
		case 0x83: //TRAP
			pc++;
			cpu_push((uint8_t)pc);
			cpu_push((uint8_t)(pc >> 8));
			cpu_push((uint8_t)(pc >> 16));
			cpu_push((uint8_t)y);
			cpu_push((uint8_t)(y >> 8));
			cpu_push((uint8_t)x);
			cpu_push((uint8_t)(x >> 8));
			cpu_push(a);
			cpu_push(cc);

			SET_COND(COND_I0);
			SET_COND(COND_I1);

			//Transfer control to trap vector
			clocks -= 9;
			pc = 0x8004;
			break;
		case 0x84: //POP A
			a = cpu_pop();
			pc++;
			break;
		case 0x85:
			if (prefix == 0x90) { //POPW Y
				y = cpu_pop();
				y <<= 8;
				y |= cpu_pop();
			}
			else { //POPW X
				x = cpu_pop();
				x <<= 8;
				x |= cpu_pop();
			}
			clocks -= 2;
			pc++;
			break;
		case 0x86: //POP CC
			cc = cpu_pop();
			pc++;
			break;
		case 0x87: //RETF
			pc = cpu_pop();
			pc <<= 8;
			pc |= cpu_pop();
			pc <<= 8;
			pc |= cpu_pop();
			clocks -= 5;
			break;
		case 0x88: //PUSH A
			cpu_push(a);
			pc++;
			break;
		case 0x89:
			if (prefix == 0x90) { //PUSHW Y
				cpu_push((uint8_t)y);
				cpu_push((uint8_t)(y >> 8));
			}
			else { //PUSHW X
				cpu_push((uint8_t)x);
				cpu_push((uint8_t)(x >> 8));
			}
			clocks -= 2;
			pc++;
			break;
		case 0x8A: //PUSH CC
			cpu_push(cc);
			pc++;
			break;
		case 0x8B: //BREAK (only useful with a debugger connected, otherwise like a NOP)
			clocks -= 1;
			pc++;
			break;
		case 0x8C: //CCF
			cc ^= COND_C;
			pc++;
			break;
		case 0x8D:
			// TODO: change to use cpu_imm24()?
			if (prefix == 0x92) { //CALLF [longptr.e]
				val16 = cpu_imm16();
				addr = memory_read(val16++);
				addr <<= 8;
				addr |= memory_read(val16++);
				addr <<= 8;
				addr |= memory_read(val16);
				clocks -= 8;
			}
			else { //CALLF addr24
				addr = memory_read(++pc);
				addr <<= 8;
				addr |= memory_read(++pc);
				addr <<= 8;
				addr |= memory_read(++pc);
				clocks -= 5;
			}
			pc++;
			cpu_push((uint8_t)pc);
			cpu_push((uint8_t)(pc >> 8));
			cpu_push((uint8_t)(pc >> 16));
			pc = addr;
			break;
		case 0x8E: //HALT
			halt = 1;
			CLEAR_COND(COND_I0);
			SET_COND(COND_I1);
			clocks -= 10;
			pc++;
			break;
		case 0x8F:
			if (prefix == 0x72) { //WFE
				// No condition flags affected.
			}
			else { //WFI
				CLEAR_COND(COND_I0);
				SET_COND(COND_I1);
			}
			halt = 1;
			pc++;
			break;
			//0x90 to 0x92 are prefixes
		case 0x93:
			if (prefix == 0x90) //LDW Y,X
				y = x;
			else //LDW X,Y
				x = y;
			pc++;
			break;
		case 0x94:
			if (prefix == 0x90) //LDW SP,Y
				sp = y;
			else //LDW SP,X
				sp = x;
			pc++;
			break;
		case 0x95:
			if (prefix == 0x90) //LD YH,A
				y = (y & 0x00FF) | ((uint16_t)a << 8);
			else //LD XH,A
				x = (x & 0x00FF) | ((uint16_t)a << 8);
			pc++;
			break;
		case 0x96:
			if (prefix == 0x90) //LDW Y,SP
				y = sp;
			else //LDW X,SP
				x = sp;
			pc++;
			break;
		case 0x97:
			if (prefix == 0x90) //LD YL,A
				y = (y & 0xFF00) | a;
			else //LD XL,A
				x = (x & 0xFF00) | a;
			pc++;
			break;
		case 0x98: //RCF
			CLEAR_COND(COND_C);
			pc++;
			break;
		case 0x99: //SCF
			SET_COND(COND_C);
			pc++;
			break;
		case 0x9A: //RIM
			CLEAR_COND(COND_I0);
			SET_COND(COND_I1);
			pc++;
			break;
		case 0x9B: //SIM
			SET_COND(COND_I0);
			SET_COND(COND_I1);
			pc++;
			break;
		case 0x9C: //RVF
			CLEAR_COND(COND_V);
			pc++;
			break;
		case 0x9D: //NOP
			pc++;
			break;
		case 0x9E:
			if (prefix == 0x90) //LD A,YH
				a = (uint8_t)(y >> 8);
			else //LD A,XH
				a = (uint8_t)(x >> 8);
			pc++;
			break;
		case 0x9F:
			if (prefix == 0x90) //LD A,YL
				a = (uint8_t)y;
			else //LD A,XL
				a = (uint8_t)x;
			pc++;
			break;
		case 0xA0: //SUB A,#imm8
			cpu_op_sub(a, cpu_imm8());
			a = result;
			pc++;
			break;
		case 0xA1: //CP A,#imm8
			val = cpu_imm8();
			cpu_op_sub(a, val);
			pc++;
			break;
		case 0xA2:
			if (prefix == 0x72) { //SUBW Y,#imm16
				cpu_op_sub16(y, cpu_imm16());
				y = result16;
				clocks -= 2;
			}
			else { //SBC A,#imm8
				cpu_op_sub(a, cpu_imm8() + GET_COND(COND_C));
				a = result;
				clocks -= 1;
			}
			pc++;
			break;
		case 0xA3:
			if (prefix == 0x90) //CPW Y,#imm16
				cpu_op_sub16(y, cpu_imm16());
			else //CPW X,#imm16
				cpu_op_sub16(x, cpu_imm16());
			clocks -= 2;
			pc++;
			break;
		case 0xA4: //AND A,#imm8
			a &= cpu_imm8();
			cpu_flags_logic(a);
			clocks -= 1;
			pc++;
			break;
		case 0xA5: //BCP A,#imm8
			cpu_flags_logic(a & cpu_imm8());
			clocks -= 1;
			pc++;
			break;
		case 0xA6: //LD A,#imm8
			a = cpu_imm8();
			cpu_flags_logic(a);
			pc++;
			break;
		case 0xA7:
			if (prefix == 0x91) { //LDF ([longptr.e],Y),A
				addr = cpu_imm16();
				addr = ((uint32_t)memory_read(addr) << 16) | ((uint32_t)memory_read(addr + 1) << 8) | (uint32_t)memory_read(addr);
				addr += (uint32_t)y;
				clocks -= 4;
			}
			else if (prefix == 0x92) { //LDF ([longptr.e],X),A
				addr = cpu_imm16();
				addr = ((uint32_t)memory_read(addr) << 16) | ((uint32_t)memory_read(addr + 1) << 8) | (uint32_t)memory_read(addr);
				addr += (uint32_t)x;
				clocks -= 4;
			}
			else if (prefix == 0x90) { //LDF (addr24,Y),A
				addr = cpu_imm24() + (uint32_t)y;
				clocks -= 1;
			}
			else { //LDF (addr24,X),A
				addr = cpu_imm24() + (uint32_t)x;
				clocks -= 1;
			}
			memory_write(addr, a);
			cpu_flags_logic(a);
			pc++;
			break;
		case 0xA8: //XOR A,#imm8
			cpu_op_xor(a, cpu_imm8());
			a = result;
			pc++;
			break;
		case 0xA9:
			if(prefix == 0x72) { //ADDW Y,#imm16
				cpu_op_add16(y, cpu_imm16());
				y = result16;
				clocks -= 2;
			}
			else { //ADC A,#imm8
				cpu_op_add(a, cpu_imm8() + GET_COND(COND_C));
				a = result;
				clocks -= 1;
			}
			pc++;
			break;
		case 0xAA: //OR A,#imm8
			cpu_op_or(a, cpu_imm8());
			a = result;
			pc++;
			break;
		case 0xAB: //ADD A,#imm8
			cpu_op_add(a, cpu_imm8());
			a = result;
			clocks -= 1;
			pc++;
			break;
		case 0xAC:
			// TODO: change to use cpu_imm24()?
			if (prefix == 0x92) {  //JPF [longptr.e]
				addr = cpu_imm16();
				pc = memory_read(addr);
				pc <<= 8;
				pc |= (uint32_t)memory_read(++addr);
				pc <<= 8;
				pc |= (uint32_t)memory_read(++addr);
				clocks -= 6;
			}
			else { //JPF addr24
				addr = cpu_imm8();
				addr <<= 8;
				addr |= (uint32_t)cpu_imm8();
				addr <<= 8;
				addr |= (uint32_t)cpu_imm8();
				pc = addr;
				clocks -= 2;
			}
			break;
		case 0xAD: //CALLR
			addr = cpu_soff8();
			pc++;
			cpu_call(pc + addr);
			clocks -= 4;
			break;
		case 0xAE:
			if (prefix == 0x90) { //LDW Y,#imm16
				y = cpu_imm16();
				cpu_flags_logic16(y);
			}
			else { //LDW X,#imm16
				x = cpu_imm16();
				cpu_flags_logic16(x);
			}
			clocks -= 2;
			pc++;
			break;
		case 0xAF:
			if (prefix == 0x91) { //LDF A,([longptr.e],Y)
				addr = cpu_imm16();
				addr = ((uint32_t)memory_read(addr) << 16) | ((uint32_t)memory_read(addr + 1) << 8) | (uint32_t)memory_read(addr);
				addr += (uint32_t)y;
				clocks -= 5;
			}
			else if (prefix == 0x92) { //LDF A,([longptr.e],X)
				addr = cpu_imm16();
				addr = ((uint32_t)memory_read(addr) << 16) | ((uint32_t)memory_read(addr + 1) << 8) | (uint32_t)memory_read(addr);
				addr += (uint32_t)x;
				clocks -= 5;
			}
			else if (prefix == 0x90) { //LDF A,(addr24,Y)
				addr = cpu_imm24() + (uint32_t)y;
				clocks -= 1;
			}
			else { //LDF A,(addr24,X)
				addr = cpu_imm24() + (uint32_t)x;
				clocks -= 1;
			}
			a = memory_read(addr);
			cpu_flags_logic(a);
			pc++;
			break;
		case 0xB0:
			if (prefix == 0x72) { //SUBW X,addr16
				addr = cpu_imm16();
				cpu_op_sub16(x, memory_read16(addr));
				x = result16;
				clocks -= 2;
			}
			else { //SUB A,addr8
				addr = cpu_imm8();
				cpu_op_sub(a, memory_read(addr));
				a = result;
				clocks -= 1;
			}
			pc++;
			break;
		case 0xB1: //CP A,addr8
			val = memory_read(cpu_imm8());
			cpu_op_sub(a, val);
			pc++;
			break;
		case 0xB2:
			if (prefix == 0x72) { //SUBW Y,addr16
				addr = cpu_imm16();
				cpu_op_sub16(y, memory_read16(addr));
				y = result16;
				clocks -= 2;
			}
			else { //SBC A,addr8
				addr = cpu_imm8();
				cpu_op_sub(a, memory_read(addr) + GET_COND(COND_C));
				a = result;
				clocks -= 1;
			}
			pc++;
			break;
		case 0xB3:
			if (prefix == 0x90) //CPW Y,addr8
				cpu_op_sub16(y, memory_read16(cpu_imm8()));
			else //CPW X,addr8
				cpu_op_sub16(x, memory_read16(cpu_imm8()));
			clocks -= 2;
			pc++;
			break;
		case 0xB4: //AND A,addr8
			a &= memory_read(cpu_imm8());
			cpu_flags_logic(a);
			clocks -= 1;
			pc++;
			break;
		case 0xB5: //BCP A,addr8
			cpu_flags_logic(a & memory_read(cpu_imm8()));
			clocks -= 1;
			pc++;
			break;
		case 0xB6: //LD A,addr8
			a = memory_read(cpu_imm8());
			cpu_flags_logic(a);
			pc++;
			break;
		case 0xB7: //LD addr8,A
			memory_write(cpu_imm8(), a);
			cpu_flags_logic(a);
			pc++;
			break;
		case 0xB8: //XOR A,(addr8)
			cpu_op_xor(a, memory_read(cpu_imm8()));
			a = result;
			pc++;
			break;
		case 0xB9:
			if (prefix == 0x72) { //ADDW Y,addr16
				cpu_op_add16(y, memory_read16(cpu_imm16()));
				y = result16;
				clocks -= 2;
			}
			else { //ADC A,addr8
				cpu_op_add(a, memory_read(cpu_imm8()) + GET_COND(COND_C));
				a = result;
				clocks -= 1;
			}
			pc++;
			break;
		case 0xBA: //OR A,addr8
			cpu_op_or(a, memory_read(cpu_imm8()));
			a = result;
			pc++;
			break;
		case 0xBB:
			if (prefix == 0x72) { //ADDW X,addr16
				val16 = memory_read16(cpu_imm16());
				cpu_op_add16(x, val16);
				x = result16;
				clocks -= 2;
			}
			else { //ADD A,addr8
				val = memory_read(cpu_imm8());
				cpu_op_add(a, val);
				a = result;
				clocks -= 1;
			}
			pc++;
			break;
		case 0xBC:
			if (prefix == 0x92) { //LDF A,[longptr.e]
				addr = cpu_imm16();
				addr = ((uint32_t)memory_read(addr) << 16) | ((uint32_t)memory_read(addr + 1) << 8) | (uint32_t)memory_read(addr);
				clocks -= 5;
			}
			else { //LDF A,addr24
				addr = cpu_imm24();
				clocks -= 1;
			}
			a = memory_read(addr);
			cpu_flags_logic(a);
			pc++;
			break;
		case 0xBD:
			if (prefix == 0x92) { //LDF [longptr.e],A
				addr = cpu_imm16();
				addr = ((uint32_t)memory_read(addr) << 16) | ((uint32_t)memory_read(addr + 1) << 8) | (uint32_t)memory_read(addr);
				clocks -= 4;
			}
			else { //LDF addr24,A
				addr = cpu_imm24();
				clocks -= 1;
			}
			memory_write(addr, a);
			cpu_flags_logic(a);
			pc++;
			break;
		case 0xBE:
			if (prefix == 0x90) { //LDW Y,addr8
				y = memory_read16(cpu_imm8());
				cpu_flags_logic16(y);
			}
			else { //LDW X,addr8
				x = memory_read16(cpu_imm8());
				cpu_flags_logic16(x);
			}
			clocks -= 2;
			pc++;
			break;
		case 0xBF:
			addr = cpu_imm8();
			if (prefix == 0x90) //LDW addr8,Y
				val16 = y;
			else //LDW addr8,X
				val16 = x;
			memory_write16(addr, val16);
			cpu_flags_logic16(val16);
			clocks -= 2;
			pc++;
			break;
		case 0xC0:
			if(prefix == 0x72) { //SUB A,[longptr.w]
				addr = cpu_addr_longptr();
				clocks -= 4;
			}
			else if(prefix == 0x92) { //SUB A,[shortptr.w]
				addr = cpu_addr_shortptr();
				clocks -= 4;
			}
			else { //SUB A,addr16
				addr = cpu_imm16();
				clocks -= 1;
			}
			cpu_op_sub(a, memory_read(addr));
			a = result;
			pc++;
			break;
		case 0xC1:
			if (prefix == 0x72) { //CP A,[longptr.w]
				addr = cpu_addr_longptr();
				val = memory_read(addr);
				clocks -= 4;
			}
			else if (prefix == 0x92) { //CP A,[shortptr.w]
				addr = cpu_addr_shortptr();
				val = memory_read(addr);
				clocks -= 4;
			}
			else { //CP A,addr16
				val = memory_read(cpu_imm16());
				clocks -= 1;
			}
			cpu_op_sub(a, val);
			pc++;
			break;
		case 0xC2:
			if(prefix == 0x72) { //SBC A,[longptr.w]
				addr = cpu_addr_longptr();
				clocks -= 4;
			}
			else if(prefix == 0x92) { //SBC A,[shortptr.w]
				addr = cpu_addr_shortptr();
				clocks -= 4;
			}
			else { //SBC A,addr16
				addr = cpu_imm16();
				clocks -= 1;
			}
			cpu_op_sub(a, memory_read(addr) + GET_COND(COND_C));
			a = result;
			pc++;
			break;
		case 0xC3:
			if (prefix == 0x91) { //CPW Y,[shortptr.w]
				addr = cpu_addr_shortptr();
				cpu_op_sub16(y, memory_read16(addr));
				clocks -= 5;
			}
			else if (prefix == 0x90) { //CPW Y,addr16
				addr = cpu_imm16();
				cpu_op_sub16(y, memory_read16(addr));
				clocks -= 2;
			}
			else if (prefix == 0x72) { //CPW X,[longptr.w]
				addr = cpu_addr_longptr();
				cpu_op_sub16(x, memory_read16(addr));
				clocks -= 5;
			}
			else if (prefix == 0x92) { //CPW X,[shortptr.w]
				addr = cpu_addr_shortptr();
				cpu_op_sub16(x, memory_read16(addr));
				clocks -= 5;
			}
			else { //CPW X,addr16
				addr = cpu_imm16();
				cpu_op_sub16(x, memory_read16(addr));
				clocks -= 2;
			}
			pc++;
			break;
		case 0xC4:
			if (prefix == 0x72) { //AND A,[longptr.w]
				addr = cpu_addr_longptr();
				clocks -= 4;
			}
			else if (prefix == 0x92) { //AND A,[shortptr.w]
				addr = cpu_addr_shortptr();
				clocks -= 4;
			}
			else { //AND A,addr16
				addr = cpu_imm16();
				clocks -= 1;
			}
			a &= memory_read(addr);
			cpu_flags_logic(a);
			pc++;
			break;
		case 0xC5:
			if (prefix == 0x72) { //BCP A,[longptr.w]
				addr = cpu_addr_longptr();
				clocks -= 4;
			}
			else if (prefix == 0x92) { //BCP A,[shortptr.w]
				addr = cpu_addr_shortptr();
				clocks -= 4;
			}
			else { //BCP A,addr16
				addr = cpu_imm16();
				clocks -= 1;
			}
			cpu_flags_logic(a & memory_read(addr));
			pc++;
			break;
		case 0xC6:
			if(prefix == 0x72) { //LD A,[longptr.w]
				addr = cpu_addr_longptr();
				clocks -= 4;
			}
			else if(prefix == 0x92) { // LD A,[shortptr.w]
				addr = cpu_addr_shortptr();
				clocks -= 4;
			}
			else { //LD A,addr16
				addr = cpu_imm16();
				clocks -= 1;
			}
			a = memory_read(addr);
			cpu_flags_logic(a);
			pc++;
			break;
		case 0xC7:
			if(prefix == 0x72) { //LD [longptr.w],A
				addr = cpu_addr_longptr();
				clocks -= 4;
			}
			else if(prefix == 0x92) { //LD [shortptr.w],A
				addr = cpu_addr_shortptr();
				clocks -= 4;
			}
			else { //LD addr16,A
				addr = cpu_imm16();
				clocks -= 1;
			}
			memory_write(addr, a);
			cpu_flags_logic(a);
			pc++;
			break;
		case 0xC8:
			if(prefix == 0x72) { //XOR A,[longptr.w]
				addr = cpu_addr_longptr();
				clocks -= 4;
			}
			else if(prefix == 0x92) { //XOR A,[shortptr.w]
				addr = cpu_addr_shortptr();
				clocks -= 4;
			}
			else { //XOR A,addr16
				addr = cpu_imm16();
				clocks -= 1;
			}
			cpu_op_xor(a, memory_read(addr));
			a = result;
			pc++;
			break;
		case 0xC9:
			if (prefix == 0x92) { //ADC A,[shortptr.w]
				addr = memory_read16(cpu_imm8());
				cpu_op_add(a, memory_read(addr) + GET_COND(COND_C));
				clocks -= 4;
			}
			else if (prefix == 0x72) { //ADC A,[longptr.w]
				addr = memory_read16(cpu_imm16());
				cpu_op_add(a, memory_read(addr) + GET_COND(COND_C));
				clocks -= 4;
			}
			else { //ADC A,addr16
				cpu_op_add(a, memory_read(cpu_imm16()) + GET_COND(COND_C));
				clocks -= 1;
			}
			a = result;
			pc++;
			break;
		case 0xCA:
			if(prefix == 0x72) { //OR A,[longptr.w]
				addr = cpu_addr_longptr();
				clocks -= 4;
			}
			else if(prefix == 0x92) { //OR A,[shortptr.w]
				addr = cpu_addr_shortptr();
				clocks -= 4;
			}
			else { //OR A,addr16
				addr = cpu_imm16();
				clocks -= 1;
			}
			val = memory_read(addr);
			cpu_op_or(a, val);
			a = result;
			pc++;
			break;
		case 0xCB:
			if (prefix == 0x72) { //ADD A,[longptr.w]
				addr = cpu_addr_longptr();
				clocks -= 4;
			}
			else if (prefix == 0x92) { //ADD A,[shortptr.w]
				addr = cpu_addr_shortptr();
				clocks -= 4;
			}
			else { //ADD A,addr16
				addr = cpu_imm16();
				clocks -= 1;
			}
			pc++;
			break;
		case 0xCC:
			if(prefix == 0x72) { //JP [longptr.w]
				addr = cpu_addr_longptr();
				clocks -= 5;
			}
			else if(prefix == 0x92) { //JP [shortptr.w]
				addr = cpu_addr_shortptr();
				clocks -= 5;
			}
			else { //JP addr16
				addr = cpu_imm16();
				clocks -= 1;
			}
			pc = addr;
			break;
		case 0xCD:
			if (prefix == 0x72) { //CALL [longptr.w]
				addr = cpu_addr_longptr();
				clocks -= 6;
			}
			else if (prefix == 0x92) { //CALL [shortptr.w]
				addr = cpu_addr_shortptr();
				clocks -= 6;
			}
			else { //CALL addr16
				addr = cpu_imm16();
				clocks -= 4;
			}
			pc++;
			cpu_call(addr);
			break;
		case 0xCE:
			if (prefix == 0x90) { //LDW Y,addr16
				addr = cpu_imm16();
				y = memory_read16(addr);
				cpu_flags_logic16(y);
				clocks -= 2;
			}
			else if (prefix == 0x91) { //LDW Y,[shortptr.w]
				addr = cpu_addr_shortptr();
				y = memory_read16(addr);
				cpu_flags_logic16(y);
				clocks -= 5;
			}
			else if (prefix == 0x72) { //LDW X,[longptr.w]
				addr = cpu_addr_longptr();
				x = memory_read16(addr);
				cpu_flags_logic16(x);
				clocks -= 5;
			}
			else if (prefix == 0x92) { //LDW X,[shortptr.w]
				addr = cpu_addr_shortptr();
				x = memory_read16(addr);
				cpu_flags_logic16(x);
				clocks -= 5;
			}
			else { //LDW X,addr16
				addr = cpu_imm16();
				x = memory_read16(addr);
				cpu_flags_logic16(x);
				clocks -= 2;
			}
			pc++;
			break;
		case 0xCF:
			if (prefix == 0x90) { //LDW addr16,Y
				addr = cpu_imm16();
				val16 = y;
				clocks -= 2;
			}
			else if (prefix == 0x91) { //LDW [shortptr.w],Y
				addr = cpu_addr_shortptr();
				val16 = y;
				clocks -= 5;
			}
			else if (prefix == 0x72) { //LDW [longptr.w],X
				addr = cpu_addr_longptr();
				val16 = x;
				clocks -= 5;
			}
			else if (prefix == 0x92) { //LDW [shortptr.w],X
				addr = cpu_addr_shortptr();
				val16 = x;
				clocks -= 5;
			}
			else { //LDW addr16,X
				addr = cpu_imm16();
				val16 = x;
				clocks -= 2;
			}
			memory_write16(addr, val16);
			cpu_flags_logic16(val16);
			pc++;
			break;
		case 0xD0:
			if(prefix == 0x91) { //SUB A,([shortptr.w],Y)
				addr = cpu_addr_shortptr_indexed_y();
				clocks -= 4;
			}
			else if(prefix == 0x72) { //SUB A,([longptr.w],X)
				addr = cpu_addr_longptr_indexed_x();
				clocks -= 4;
			}
			else if(prefix == 0x92) { //SUB A,([shortptr.w],X)
				addr = cpu_addr_shortptr_indexed_x();
				clocks -= 4;
			}
			else if(prefix == 0x90) { //SUB A,(addr16,Y)
				addr = (uint32_t)cpu_imm16() + (uint32_t)y;
				clocks -= 1;
			}
			else { //SUB A,(addr16,X)
				addr = (uint32_t)cpu_imm16() + (uint32_t)x;
				clocks -= 1;
			}
			cpu_op_sub(a, memory_read(addr));
			a = result;
			pc++;
			break;
		case 0xD1:
			if (prefix == 0x91) { //CP A,([shortptr.w],Y)
				addr = cpu_addr_shortptr_indexed_y();
				clocks -= 4;
			}
			else if (prefix == 0x72) { //CP A,([longptr.w],X)
				addr = cpu_addr_longptr_indexed_x();
				clocks -= 4;
			}
			else if (prefix == 0x92) { //CP A,([shortptr.w],X)
				addr = cpu_addr_shortptr_indexed_x();
				clocks -= 4;
			}
			else if (prefix == 0x90) { //CP A,(addr16,Y)
				addr = (uint32_t)cpu_imm16() + (uint32_t)y;
				clocks -= 1;
			}
			else { //CP A,(addr16,X)
				addr = (uint32_t)cpu_imm16() + (uint32_t)x;
				clocks -= 1;
			}
			val = memory_read(addr);
			cpu_op_sub(a, val);
			pc++;
			break;
		case 0xD2:
			if(prefix == 0x91) { //SBC A,([shortptr.w],Y)
				addr = cpu_addr_shortptr_indexed_y();
				clocks -= 4;
			}
			else if(prefix == 0x72) { //SBC A,([longptr.w],X)
				addr = cpu_addr_longptr_indexed_x();
				clocks -= 4;
			}
			else if(prefix == 0x92) { //SBC A,([shortptr.w],X)
				addr = cpu_addr_shortptr_indexed_x();
				clocks -= 4;
			}
			else if(prefix == 0x90) { //SBC A,(addr16,Y)
				addr = (uint32_t)cpu_imm16() + (uint32_t)y;
				clocks -= 1;
			}
			else { //SBC A,(addr16,X)
				addr = (uint32_t)cpu_imm16() + (uint32_t)x;
				clocks -= 1;
			}
			cpu_op_sub(a, memory_read(addr) + GET_COND(COND_C));
			a = result;
			pc++;
			break;
		case 0xD3:
			if (prefix == 0x72) { //CPW Y,([longptr.w],X)
				addr = cpu_addr_longptr_indexed_x();
				cpu_op_sub16(y, memory_read16(addr));
				clocks -= 5;
			}
			else if (prefix == 0x92) { //CPW Y,([shortptr.w],X)
				addr = cpu_addr_shortptr_indexed_x();
				cpu_op_sub16(y, memory_read16(addr));
				clocks -= 5;
			}
			else if (prefix == 0x91) { //CPW X,([shortptr.w],Y)
				addr = cpu_addr_shortptr_indexed_y();
				cpu_op_sub16(x, memory_read16(addr));
				clocks -= 5;
			}
			else if (prefix == 0x90) { //CPW X,(addr16,Y)
				addr = (uint32_t)cpu_imm16() + (uint32_t)y;
				cpu_op_sub16(x, memory_read16(addr));
				clocks -= 2;
			}
			else { //CPW Y,(addr16,X)
				addr = (uint32_t)cpu_imm16() + (uint32_t)x;
				cpu_op_sub16(y, memory_read16(addr));
				clocks -= 2;
			}
			pc++;
			break;
		case 0xD4:
			if (prefix == 0x72) { //AND A,([longptr.w],X)
				addr = cpu_addr_longptr_indexed_x();
				clocks -= 4;
			}
			else if (prefix == 0x91) { //AND A,([shortptr.w],Y)
				addr = cpu_addr_shortptr_indexed_y();
				clocks -= 4;
			}
			else if (prefix == 0x92) { //AND A,([shortptr.w],X)
				addr = cpu_addr_shortptr_indexed_x();
				clocks -= 4;
			}
			else if (prefix == 0x90) { //AND A,(addr16,Y)
				addr = (uint32_t)cpu_imm16() + (uint32_t)y;
				clocks -= 1;
			}
			else { //AND A,(addr16,X)
				addr = (uint32_t)cpu_imm16() + (uint32_t)x;
				clocks -= 1;
			}
			a &= memory_read(addr);
			cpu_flags_logic(a);
			pc++;
			break;
		case 0xD5:
			if (prefix == 0x72) { //BCP A,([longptr.w],X)
				addr = cpu_addr_longptr_indexed_x();
				clocks -= 4;
			}
			else if (prefix == 0x91) { //BCP A,([shortptr.w],Y)
				addr = cpu_addr_shortptr_indexed_y();
				clocks -= 4;
			}
			else if (prefix == 0x92) { //BCP A,([shortptr.w],X)
				addr = cpu_addr_shortptr_indexed_x();
				clocks -= 4;
			}
			else if (prefix == 0x90) { //BCP A,(addr16,Y)
				addr = (uint32_t)cpu_imm16() + (uint32_t)y;
				clocks -= 1;
			}
			else { //BCP A,(addr16,X)
				addr = (uint32_t)cpu_imm16() + (uint32_t)x;
				clocks -= 1;
			}
			cpu_flags_logic(a & memory_read(addr));
			pc++;
			break;
		case 0xD6:
			if(prefix == 0x72) { //LD A,([longptr.w],X)
				addr = cpu_addr_longptr_indexed_x();
				clocks -= 4;
			}
			else if(prefix == 0x91) { //LD A,([shortptr.w],Y)
				addr = cpu_addr_shortptr_indexed_y();
				clocks -= 4;
			}
			else if(prefix == 0x92) { //LD A,([shortptr.w],X)
				addr = cpu_addr_shortptr_indexed_x();
				clocks -= 4;
			}
			else if(prefix == 0x90) { //LD A,(addr16,Y)
				addr = (uint32_t)cpu_imm16() + (uint32_t)y;
				clocks -= 1;
			}
			else { //LD A,(addr16,X)
				addr = (uint32_t)cpu_imm16() + (uint32_t)x;
				clocks -= 1;
			}
			a = memory_read(addr);
			cpu_flags_logic(a);
			pc++;
			break;
		case 0xD7:
			if(prefix == 0x72) { //LD ([longptr.w],X),A
				addr = cpu_addr_longptr_indexed_x();
				clocks -= 4;
			}
			else if(prefix == 0x91) { //LD ([shortptr.w],Y),A
				addr = cpu_addr_shortptr_indexed_y();
				clocks -= 4;
			}
			else if(prefix == 0x92) { //LD ([shortptr.w],X),A
				addr = cpu_addr_shortptr_indexed_x();
				clocks -= 4;
			}
			else if(prefix == 0x90) { //LD (addr16,Y),A
				addr = (uint32_t)cpu_imm16() + (uint32_t)y;
				clocks -= 1;
			}
			else { //LD (addr16,X),A
				addr = (uint32_t)cpu_imm16() + (uint32_t)x;
				clocks -= 1;
			}
			memory_write(addr, a);
			cpu_flags_logic(a);
			pc++;
			break;
		case 0xD8:
			if(prefix == 0x72) { //XOR A,([longptr.w],X)
				addr = cpu_addr_longptr_indexed_x();
				clocks -= 4;
			}
			else if(prefix == 0x91) { //XOR A,([shortptr.w],Y)
				addr = cpu_addr_shortptr_indexed_y();
				clocks -= 4;
			}
			else if(prefix == 0x92) { //XOR A,([shortptr.w],X)
				addr = cpu_addr_shortptr_indexed_x();
				clocks -= 4;
			}
			else if(prefix == 0x90) { //XOR A,(addr16,Y)
				addr = (uint32_t)cpu_imm16() + (uint32_t)y;
				clocks -= 1;
			}
			else { //XOR A,(addr16,X)
				addr = (uint32_t)cpu_imm16() + (uint32_t)x;
				clocks -= 1;
			}
			cpu_op_xor(a, memory_read(addr));
			a = result;
			pc++;
			break;
		case 0xD9:
			if (prefix == 0x90) { //ADC A,(addr16,Y)
				addr = (uint32_t)cpu_imm16() + (uint32_t)y;
				clocks -= 1;
			}
			else if (prefix == 0x91) { //ADC A,([shortptr.w],Y)
				addr = cpu_addr_shortptr_indexed_y();
				clocks -= 4;
			}
			else if (prefix == 0x92) { //ADC A,([shortptr.w],X)
				addr = cpu_addr_shortptr_indexed_x();
				clocks -= 4;
			}
			else if (prefix == 0x72) { //ADC A,([longptr.w],X)
				addr = cpu_addr_longptr_indexed_x();
				clocks -= 4;
			}
			else { //ADC A,(addr16,X)
				addr = (uint32_t)cpu_imm16() + (uint32_t)x;
				clocks -= 1;
			}
			cpu_op_add(a, memory_read(addr) + GET_COND(COND_C));
			a = result;
			pc++;
			break;
		case 0xDA:
			if (prefix == 0x90) { //OR A,(addr16,Y)
				addr = (uint32_t)cpu_imm16() + (uint32_t)y;
				clocks -= 1;
			}
			else if (prefix == 0x91) { //OR A,([shortptr.w],Y)
				addr = cpu_addr_shortptr_indexed_y();
				clocks -= 4;
			}
			else if (prefix == 0x92) { //OR A,([shortptr.w],X)
				addr = cpu_addr_shortptr_indexed_x();
				clocks -= 4;
			}
			else if (prefix == 0x72) { //OR A,([longptr.w],X)
				addr = cpu_addr_longptr_indexed_x();
				clocks -= 4;
			}
			else { //OR A,(addr16,X)
				addr = (uint32_t)cpu_imm16() + (uint32_t)x;
				clocks -= 1;
			}
			cpu_op_or(a, memory_read(addr));
			a = result;
			pc++;
			break;
		case 0xDB:
			if (prefix == 0x90) { //ADD A,(addr16,Y)
				addr = (uint32_t)cpu_imm16() + (uint32_t)y;
				clocks -= 1;
			}
			else if (prefix == 0x91) { //ADD A,([shortptr.w],Y)
				addr = cpu_addr_shortptr_indexed_y();
				clocks -= 4;
			}
			else if (prefix == 0x92) { //ADD A,([shortptr.w],X)
				addr = cpu_addr_shortptr_indexed_x();
				clocks -= 4;
			}
			else if (prefix == 0x72) { //ADD A,([longptr.w],X)
				addr = cpu_addr_longptr_indexed_x();
				clocks -= 4;
			}
			else { //ADD A,(addr16,X)
				addr = (uint32_t)cpu_imm16() + (uint32_t)x;
				clocks -= 1;
			}
			cpu_op_add(a, memory_read(addr));
			a = result;
			pc++;
			break;
		case 0xDC:
			if (prefix == 0x90) { //JP (addr16,Y)
				addr = (uint32_t)cpu_imm16() + (uint32_t)y;
				clocks -= 2;
			}
			else if (prefix == 0x91) { //JP ([shortptr.w],Y)
				addr = cpu_addr_shortptr_indexed_y();
				clocks -= 5;
			}
			else if (prefix == 0x92) { //JP ([shortptr.w],X)
				addr = cpu_addr_shortptr_indexed_x();
				clocks -= 5;
			}
			else if (prefix == 0x72) { //JP ([longptr.w],X)
				addr = cpu_addr_longptr_indexed_x();
				clocks -= 5;
			}
			else { //JP (addr16,X)
				addr = (uint32_t)cpu_imm16() + (uint32_t)x;
				clocks -= 1;
			}
			pc = addr;
			break;
		case 0xDD:
			if (prefix == 0x72) { //CALL ([longptr.w],X)
				addr = cpu_addr_longptr_indexed_x();
				clocks -= 6;
			}
			else if (prefix == 0x92) { //CALL ([shortptr.w],X)
				addr = cpu_addr_shortptr_indexed_x();
				clocks -= 6;
			}
			else if (prefix == 0x91) { //CALL ([shortptr.w],Y)
				addr = cpu_addr_shortptr_indexed_y();
				clocks -= 6;
			}
			else if (prefix == 0x90) { //CALL (addr16,Y)
				addr = (uint32_t)cpu_imm16() + (uint32_t)y;
				clocks -= 4;
			}
			else { //CALL (addr16,X)
				addr = (uint32_t)cpu_imm16() + (uint32_t)x;
				clocks -= 4;
			}
			pc++;
			cpu_call(addr);
			break;
		case 0xDE:
			if (prefix == 0x91) { //LDW Y,([shortptr.w],Y)
				addr = cpu_addr_shortptr_indexed_y();
				y = memory_read16(addr);
				cpu_flags_logic16(y);
				clocks -= 5;
			}
			else if (prefix == 0x90) { //LDW Y,(addr16,Y)
				addr = (uint32_t)cpu_imm16() + (uint32_t)y;
				y = memory_read16(addr);
				cpu_flags_logic16(y);
				clocks -= 2;
			}
			else if (prefix == 0x72) { //LDW X,([longptr.w],X)
				addr = cpu_addr_longptr_indexed_x();
				x = memory_read16(addr);
				cpu_flags_logic16(x);
				clocks -= 5;
			}
			else if (prefix == 0x92) { //LDW X,([shortptr.w],X)
				addr = cpu_addr_shortptr_indexed_x();
				x = memory_read16(addr);
				cpu_flags_logic16(x);
				clocks -= 5;
			}
			else { //LDW X,(addr16,X)
				addr = (uint32_t)cpu_imm16() + (uint32_t)x;
				x = memory_read16(addr);
				cpu_flags_logic16(x);
				clocks -= 2;
			}
			pc++;
			break;
		case 0xDF:
			if (prefix == 0x91) { //LDW ([shortptr.w],Y),X
				addr = cpu_addr_shortptr_indexed_y();
				memory_write16(addr, x);
				cpu_flags_logic16(x);
				clocks -= 5;
			}
			else if (prefix == 0x90) { //LDW (addr16,Y),X
				addr = (uint32_t)cpu_imm16() + (uint32_t)y;
				memory_write16(addr, x);
				cpu_flags_logic16(x);
				clocks -= 2;
			}
			else if (prefix == 0x72) { //LDW ([longptr.w],X),Y
				addr = cpu_addr_longptr_indexed_x();
				memory_write16(addr, y);
				cpu_flags_logic16(y);
				clocks -= 5;
			}
			else if (prefix == 0x92) { //LDW ([shortptr.w],X),Y
				addr = cpu_addr_shortptr_indexed_x();
				memory_write16(addr, y);
				cpu_flags_logic16(y);
				clocks -= 5;
			}
			else { //LDW (addr16,X),Y
				addr = (uint32_t)cpu_imm16() + (uint32_t)x;
				memory_write16(addr, y);
				cpu_flags_logic16(y);
				clocks -= 2;
			}
			pc++;
			break;
		case 0xE0:
			if (prefix == 0x90) //SUB A,(addr8,Y)
				addr = (uint32_t)cpu_imm8() + (uint32_t)y;
			else //SUB A,(addr8,X)
				addr = (uint32_t)cpu_imm8() + (uint32_t)x;
			cpu_op_sub(a, memory_read(addr));
			a = result;
			pc++;
			break;
		case 0xE1:
			if (prefix == 0x90) //CP A,(addr8,Y)
				addr = (uint32_t)cpu_imm8() + (uint32_t)y;
			else //CP A,(addr8,X)
				addr = (uint32_t)cpu_imm8() + (uint32_t)x;
			val = memory_read(addr);
			cpu_op_sub(a, val);
			pc++;
			break;
		case 0xE2:
			if (prefix == 0x90) //SBC A,(addr8,Y)
				addr = (uint32_t)cpu_imm8() + (uint32_t)y;
			else //SBC A,(addr8,X)
				addr = (uint32_t)cpu_imm8() + (uint32_t)x;
			cpu_op_sub(a, memory_read(addr) + GET_COND(COND_C));
			a = result;
			pc++;
			break;
		case 0xE3:
			if (prefix == 0x90) { //CPW X,(addr8,Y)
				addr = (uint32_t)cpu_imm8() + (uint32_t)y;
				cpu_op_sub16(x, memory_read16(addr));
			}
			else { //CPW Y,(addr8,X)
				addr = (uint32_t)cpu_imm8() + (uint32_t)x;
				cpu_op_sub16(y, memory_read16(addr));
			}
			clocks -= 2;
			pc++;
			break;
		case 0xE4:
			if (prefix == 0x90) //AND A,(addr8,Y)
				addr = (uint32_t)cpu_imm8() + (uint32_t)y;
			else //AND A,(addr8,X)
				addr = (uint32_t)cpu_imm8() + (uint32_t)x;
			a &= memory_read(addr);
			cpu_flags_logic(a);
			clocks -= 1;
			pc++;
			break;
		case 0xE5:
			if (prefix == 0x90) //BCP A,(addr8,Y)
				addr = (uint32_t)cpu_imm8() + (uint32_t)y;
			else //BCP A,(addr8,X)
				addr = (uint32_t)cpu_imm8() + (uint32_t)x;
			cpu_flags_logic(a & memory_read(addr));
			clocks -= 1;
			pc++;
			break;
		case 0xE6:
			if (prefix == 0x90) //LD A,(addr8,Y)
				addr = (uint32_t)cpu_imm8() + (uint32_t)y;
			else //LD A,(addr8,X)
				addr = (uint32_t)cpu_imm8() + (uint32_t)x;
			a = memory_read(addr);
			cpu_flags_logic(a);
			pc++;
			break;
		case 0xE7:
			if (prefix == 0x90) //LD (addr8,Y),A
				addr = (uint32_t)cpu_imm8() + (uint32_t)y;
			else //LD (addr8,X),A
				addr = (uint32_t)cpu_imm8() + (uint32_t)x;
			memory_write(addr, a);
			cpu_flags_logic(a);
			pc++;
			break;
		case 0xE8:
			if (prefix == 0x90) //XOR A,(addr8,Y)
				addr = (uint32_t)cpu_imm8() + (uint32_t)y;
			else //XOR A,(addr8,X)
				addr = (uint32_t)cpu_imm8() + (uint32_t)x;
			val = memory_read(addr);
			cpu_op_xor(a, val);
			a = result;
			pc++;
			break;
		case 0xE9:
			if (prefix == 0x90) { //ADC A,(addr8,Y)
				addr = (uint32_t)cpu_imm8() + (uint32_t)y;
				cpu_op_add(a, memory_read(addr) + GET_COND(COND_C));
			}
			else { //ADC A,(addr8,X)
				addr = (uint32_t)cpu_imm8() + (uint32_t)x;
				cpu_op_add(a, memory_read(addr) + GET_COND(COND_C));
			}
			a = result;
			clocks -= 1;
			pc++;
			break;
		case 0xEA:
			if (prefix == 0x90) //OR A,(addr8,Y)
				addr = (uint32_t)cpu_imm8() + (uint32_t)y;
			else //OR A,(addr8,X)
				addr = (uint32_t)cpu_imm8() + (uint32_t)x;
			val = memory_read(addr);
			cpu_op_or(a, val);
			a = result;
			pc++;
			break;
		case 0xEB:
			if (prefix == 0x90) //ADD A,(addr8,Y)
				addr = (uint32_t)cpu_imm8() + (uint32_t)y;
			else //ADD A,(addr8,X)
				addr = (uint32_t)cpu_imm8() + (uint32_t)x;
			cpu_op_add(a, memory_read(addr));
			a = result;
			clocks -= 1;
			pc++;
			break;
		case 0xEC:
			if(prefix == 0x90) { //JP (addr8,Y)
				addr = (uint32_t)cpu_imm8() + (uint32_t)y;
				clocks -= 2;
			}
			else { //JP (addr8,X)
				addr = (uint32_t)cpu_imm8() + (uint32_t)x;
				clocks -= 1;
			}
			pc = addr;
			break;
		case 0xED:
			if (prefix == 0x90) //CALL (addr8,Y)
				addr = (uint32_t)cpu_imm8() + (uint32_t)y;
			else //CALL (addr8,X)
				addr = (uint32_t)cpu_imm8() + (uint32_t)x;
			pc++;
			cpu_call(addr);
			clocks -= 4;
			break;
		case 0xEE:
			if (prefix == 0x90) { //LDW Y,(addr8,Y)
				addr = (uint32_t)cpu_imm8() + (uint32_t)y;
				y = memory_read16(addr);
				cpu_flags_logic16(y);
			}
			else { //LDW X,(addr8,X)
				addr = (uint32_t)cpu_imm8() + (uint32_t)x;
				x = memory_read16(addr);
				cpu_flags_logic16(x);
			}
			clocks -= 2;
			pc++;
			break;
		case 0xEF:
			if (prefix == 0x90) { //LDW (addr8,Y),X
				addr = (uint32_t)cpu_imm8() + (uint32_t)y;
				memory_write16(addr, x);
				cpu_flags_logic16(x);
			}
			else { //LDW (addr8,X),Y
				addr = (uint32_t)cpu_imm8() + (uint32_t)x;
				memory_write16(addr, y);
				cpu_flags_logic16(y);
			}
			clocks -= 2;
			pc++;
			break;
		case 0xF0:
			if (prefix == 0x72) { //SUBW X,(addr8,SP)
				addr = (uint32_t)cpu_imm8() + (uint32_t)sp;
				cpu_op_sub16(x, memory_read16(addr));
				x = result16;
				clocks -= 2;
			}
			else if (prefix == 0x90) { //SUB A,(Y)
				addr = cpu_addr_indexed_y();
				cpu_op_sub(a, memory_read(addr));
				a = result;
				clocks -= 1;
			}
			else { //SUB A,(X)
				addr = cpu_addr_indexed_x();
				cpu_op_sub(a, memory_read(addr));
				a = result;
				clocks -= 1;
			}
			pc++;
			break;
		case 0xF1:
			if (prefix == 0x90) //CP A,(Y)
				addr = y;
			else //CP A,(X)
				addr = x;
			cpu_op_sub(a, memory_read(addr));
			pc++;
			break;
		case 0xF2:
			if (prefix == 0x72) { //SUBW Y,(addr8,SP)
				addr = (uint32_t)cpu_imm8() + (uint32_t)sp;
				cpu_op_sub16(y, memory_read(addr));
				y = result16;
				clocks -= 2;
			}
			else if (prefix == 0x90) { //SBC A,(Y)
				addr = cpu_addr_indexed_y();
				cpu_op_sub(a, memory_read(addr) + GET_COND(COND_C));
				a = result;
				clocks -= 1;
			}
			else { //SBC A,(X)
				addr = cpu_addr_indexed_x();
				cpu_op_sub(a, memory_read(addr) + GET_COND(COND_C));
				a = result;
				clocks -= 1;
			}
			pc++;
			break;
		case 0xF3:
			if (prefix == 0x90) //CPW X,(Y)
				cpu_op_sub16(x, memory_read16(cpu_addr_indexed_y()));
			else //CPW Y,(X)
				cpu_op_sub16(y, memory_read16(cpu_addr_indexed_x()));
			clocks -= 2;
			pc++;
			break;
		case 0xF4:
			if (prefix == 0x90) //AND A,(Y)
				a &= memory_read(cpu_addr_indexed_y());
			else //AND A,(X)
				a &= memory_read(cpu_addr_indexed_x());
			cpu_flags_logic(a);
			clocks -= 1;
			pc++;
			break;
		case 0xF5:
			if (prefix == 0x90) //BCP A,(Y)
				cpu_flags_logic(a & memory_read(cpu_addr_indexed_y()));
			else //BCP A,(X)
				cpu_flags_logic(a & memory_read(cpu_addr_indexed_x()));
			clocks -= 1;
			pc++;
			break;
		case 0xF6:
			if (prefix == 0x90) //LD A,(Y)
				a = memory_read(cpu_addr_indexed_y());
			else //LD A,(X)
				a = memory_read(cpu_addr_indexed_x());
			cpu_flags_logic(a);
			pc++;
			break;
		case 0xF7:
			if (prefix == 0x90) //LD (Y),A
				addr = cpu_addr_indexed_y();
			else //LD (X),A
				addr = cpu_addr_indexed_x();
			memory_write(addr, a);
			cpu_flags_logic(a);
			pc++;
			break;
		case 0xF8:
			if (prefix == 0x90) //XOR A,(Y)
				cpu_op_xor(a, memory_read(y));
			else //XOR A,(X)
				cpu_op_xor(a, memory_read(x));
			a = result;
			clocks -= 1;
			pc++;
			break;
		case 0xF9:
			if (prefix == 0x72) { //ADDW Y,(addr8,SP)
				addr = (uint32_t)cpu_imm8() + (uint32_t)sp;
				cpu_op_add16(y, memory_read16(addr));
				y = result16;
				clocks -= 2;
			}
			else if (prefix == 0x90) { //ADC A,(Y)
				cpu_op_add(a, memory_read(y) + GET_COND(COND_C));
				a = result;
				clocks -= 1;
			}
			else { //ADC A,(X)
				cpu_op_add(a, memory_read(x) + GET_COND(COND_C));
				a = result;
				clocks -= 1;
			}
			pc++;
			break;
		case 0xFA:
			if (prefix == 0x90) //OR A,(Y)
				addr = cpu_addr_indexed_y();
			else //OR A,(X)
				addr = cpu_addr_indexed_x();
			val = memory_read(addr);
			cpu_op_or(a, addr);
			a = result;
			pc++;
			break;
		case 0xFB:
			if (prefix == 0x72) { //ADDW X,(addr8,SP)
				addr = (uint32_t)cpu_imm8() + (uint32_t)sp;
				cpu_op_add16(x, memory_read16(addr));
				x = result16;
				clocks -= 2;
			}
			else if (prefix == 0x90) { //ADD A,(Y)
				cpu_op_add(a, memory_read(cpu_addr_indexed_y()));
				a = result;
				clocks -= 1;
			}
			else { //ADD A,(X)
				cpu_op_add(a, memory_read(cpu_addr_indexed_x()));
				a = result;
				clocks -= 1;
			}
			pc++;
			break;
		case 0xFC:
			if (prefix == 0x90) //JP (Y)
				pc = y;
			else //JP (X)
				pc = x;
			break;
		case 0xFD:
			pc++;
			if (prefix == 0x90) //CALL (Y)
				addr = cpu_addr_indexed_y();
			else //CALL (X)
				addr = cpu_addr_indexed_x();
			cpu_call(addr);
			clocks -= 4;
			break;
		case 0xFE:
			if (prefix == 0x90) { //LDW Y,(Y)
				val16 = memory_read16(cpu_addr_indexed_y());
				y = val16;
				cpu_flags_logic16(y);
			}
			else { //LDW X,(X)
				val16 = memory_read16(cpu_addr_indexed_x());
				x = val16;
				cpu_flags_logic16(x);
			}
			clocks -= 2;
			pc++;
			break;
		case 0xFF:
			if (prefix == 0x90) { //LDW (Y),X
				memory_write16(cpu_addr_indexed_y(), x);
				cpu_flags_logic16(x);
			}
			else { //LDW (X),Y
				memory_write16(cpu_addr_indexed_x(), y);
				cpu_flags_logic16(y);
			}
			clocks -= 2;
			pc++;
			break;
		default:
			valid = 0;
		}

		if (halt) {
			clocks = 0;
			break;
		}

#ifdef DEBUG_OUTPUT
		printf("A = %02X\tX = %04X\tY = %04X\tSP = %08X\tCC = %02X\n", a, x, y, sp, cc);
#endif
		if (valid == 0) {
			printf("INVALID OPCODE @ %08X: %02X prefix %02X\n", pc, opcode, prefix);
			running = 0;
		}

		// By default, assume unless otherwise already decremented, instructions take 1 clock cycle.
		if (startclocks == clocks) clocks--; //TODO: Actual instruction timing...
	}

	return clocks;
}

void cpu_irq(uint8_t irq) {
	irq_req[irq & 0x1F] = 1;
	anyirq = 1;
	halt = 0;
}
