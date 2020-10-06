#ifndef _ELF_H_
#define _ELF_H_

#include <stdint.h>

#define swap32(x) (endianness ? (((x & 0xFF000000) >> 24) | ((x & 0x00FF0000) >> 8) | ((x & 0x0000FF00) << 8) | ((x & 0x000000FF) << 24)) : (x))
#define swap16(x) (endianness ? (((x & 0xFF00) >> 8) | ((x & 0x00FF) << 8)) : (x))

extern uint8_t endianness;

int elf_load(char* path);
uint32_t elf_getentry();

#endif
