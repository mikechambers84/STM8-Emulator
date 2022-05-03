#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "memory.h"
#include "elf.h"

// TODO: clean up this code.
// - Don't dynamically malloc() using size values straight from external data! Sanity-check sizes first.
// - Use endian.h macros to handle endianness of values (and get rid of swap16(), swap32()).
// - Define readable constants for ELF header offsets.

//#define ELF_DETAIL

uint32_t elf_entrypoint = 0;
uint8_t endianness;

int elf_load(const char* path) {
	FILE* elf;
	long fsz;
	uint8_t hdr[52], * phdr = NULL, * shdr = NULL, * chunk = NULL, * strtab = NULL;
	uint16_t phdrnum, phdrsize, shdrnum, shdrsize;
	uint32_t i, j, p_offset, p_vaddr, p_paddr, p_filesz, p_memsz, p_flags, p_align;

	elf = fopen(path, "rb");
	if (elf == NULL) {
		return -1;
	}
	fseek(elf, 0, SEEK_END);
	fsz = ftell(elf);
	fseek(elf, 0, SEEK_SET);

	//load ELF header
	fread(hdr, 1, 52, elf);
	if ((hdr[0x00] != 0x7F)
		|| (hdr[0x01] != 'E')
		|| (hdr[0x02] != 'L')
		|| (hdr[0x03] != 'F')) {
		printf("Not an ELF file!\r\n");
		return -1;
	}
	if (hdr[0x04] != 1) {
		printf("Not a 32-bit binary!\r\n");
		return -1;
	}
	if (hdr[0x05] == 1) endianness = 0; else endianness = 1;
	elf_entrypoint = swap32(*(uint32_t*)(&hdr[0x18]));
	phdrsize = swap16(*(uint16_t*)(&hdr[0x2A]));
	phdrnum = swap16(*(uint16_t*)(&hdr[0x2C]));
	shdrsize = swap16(*(uint16_t*)(&hdr[0x2E]));
	shdrnum = swap16(*(uint16_t*)(&hdr[0x30]));

	//load program header
	phdr = (uint8_t*)malloc((size_t)phdrsize);
	if (phdr == NULL) {
		printf("Could not allocate phdr\r\n");
		return -1;
	}
	for (i = 0; i < phdrnum; i++) {
		fseek(elf, swap32(*(uint32_t*)(&hdr[0x1C])) + (uint32_t)phdrsize * i, SEEK_SET);
		if (fread(phdr, 1, (size_t)phdrsize, elf) < (size_t)phdrsize) {
			printf("Could not read phdr %lu\r\n", i);
			return -1;
		}
		p_offset = swap32(*(uint32_t*)(&phdr[0x04]));
		p_vaddr = swap32(*(uint32_t*)(&phdr[0x08]));
		p_paddr = swap32(*(uint32_t*)(&phdr[0x0C]));
		p_filesz = swap32(*(uint32_t*)(&phdr[0x10]));
		p_memsz = swap32(*(uint32_t*)(&phdr[0x14]));
		p_flags = swap32(*(uint32_t*)(&phdr[0x18]));
		p_align = swap32(*(uint32_t*)(&phdr[0x1C]));
#ifdef ELF_DETAIL
		printf("Program header %lu:\r\n", i);
		printf("\tFile image offset: 0x%08X\r\n", p_offset);
		printf("\t  Virtual address: 0x%08X\r\n", p_vaddr);
		printf("\t Physical address: 0x%08X\r\n", p_paddr);
		printf("\t     Size in file: 0x%08X\r\n", p_filesz);
		printf("\t   Size in memory: 0x%08X\r\n", p_memsz);
		printf("\t            Flags: 0x%08X\r\n", p_flags);
		printf("\t        Alignment: 0x%08X\r\n", p_align);
#endif
		chunk = (uint8_t*)malloc((size_t)p_filesz);
		if (chunk == NULL) {
			return -1;
		}
		fseek(elf, p_offset, SEEK_SET);
		if (fread(chunk, 1, (size_t)p_filesz, elf) < (size_t)p_filesz) {
			free(chunk);
			free(phdr);
			printf("Failed to load %lu bytes for this program section!\r\n", p_filesz);
			return -1;
		}
		for (j = 0; j < p_filesz; j++) {
			memory_write(p_paddr + j, chunk[j]);
		}
		free(chunk);
#ifdef ELF_DETAIL
		printf("Loaded %lu bytes for this program section at 0x%08X\r\n\r\n", p_filesz, p_paddr);
#endif
	}
	free(phdr);

	fclose(elf);

	return 0;
}
