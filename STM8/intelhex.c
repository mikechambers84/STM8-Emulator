#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "memory.h"
#include "endian.h"
#include "intelhex.h"

#define IHEX_DETAIL

// Nominal max length in characters of a record is 521 (excluding line breaks
// and terminating null).
#define LINE_MAX_LEN 525

// Absolute minimum line length in characters: start code + 2 digit byte count
// + 4 digit address + 2 digit record type + 2 digit checksum.
#define LINE_MIN_LEN 11

// Maximum specifyable byte count in a record is 255, so max record length in
// bytes is that plus size of other fields (count, addr, type, checksum).
#define REC_MAX_LEN 260

#define REC_OFFSET_BYTE_COUNT 0
#define REC_OFFSET_ADDR 1
#define REC_OFFSET_TYPE 3
#define REC_OFFSET_DATA 4

#define REC_TYPE_DATA 0x0
#define REC_TYPE_EOF 0x1
#define REC_TYPE_EXT_SEG_ADDR 0x2
#define REC_TYPE_START_SEG_ADDR 0x3
#define REC_TYPE_EXT_LIN_ADDR 0x4
#define REC_TYPE_START_LIN_ADDR 0x5

static uint8_t intelhex_convert_hex_char(char c) {
	c = toupper(c);
	if(isdigit(c)) {
		return c - '0';
	} else {
		return c - ('A' - 10);
	}
}

static size_t intelhex_decode_record(const char* line, size_t line_len, uint8_t* buf, size_t buf_len) {
	size_t rec_len = 0;
	uint8_t val;

	// Check first character of line is start code (colon).
	if(line_len > 0 && *line == ':') {
		line++;
		line_len--;
	} else {
		return 0;
	}

	while(line_len > 0) {
		if(isxdigit(*line) && isxdigit(*(line + 1))) {
			// Convert pair of hex digit characters into byte value.
			val = (intelhex_convert_hex_char(*line) << 4) | intelhex_convert_hex_char(*(line + 1));
			line += 2;
			line_len -= 2;

			// Add byte value to output buffer (if room).
			if(buf_len > 0) {
				*buf++ = val;
				buf_len--;
				rec_len++;
			} else {
				break;
			}
		} else if(*line == '\n' || *line == '\r') {
			// Skip any trailing line-break characters.
			line++;
			line_len--;
		} else {
			// Otherwise, an invalid character, exit with failure.
			return 0;
		}
	}

	return rec_len;
}

int intelhex_load(const char* path) {
	FILE* ihex;
	char line[LINE_MAX_LEN];
	unsigned int line_count = 0, mem_addr, mem_offset = 0;
	uint8_t record[REC_MAX_LEN], checksum;
	size_t line_len, record_len;
	bool end = false;

	// Open the file in text mode (so CRLF line endings are automatically converted to LF).
	ihex = fopen(path, "r");
	if(ihex == NULL) {
		return -1;
	}

	do {
		memset(line, '\0', sizeof(line));

		if(fgets(line, sizeof(line) / sizeof(line[0]), ihex) != NULL) {
			line_count++;
			line_len = strlen(line);

			// Remove any trailing line feed character from line string.
			if(line[line_len - 1] == '\n') {
				line[line_len - 1] = '\0';
				line_len--;
			}

			// Check for minimum line length.
			if(line_len < LINE_MIN_LEN) {
				printf("Invalid line %u of Intel Hex file; incomplete record, not enough characters.\n", line_count);
				return -1;
			}

#ifdef IHEX_DETAIL
			printf("Line %u: \"%s\" (%zu chars)\n", line_count, line, line_len);
#endif

			// Decode the line into a record (i.e. byte array).
			record_len = intelhex_decode_record(line, line_len, record, sizeof(record));
			if(record_len == 0) {
				printf("Invalid line %u of Intel Hex file; error parsing hex digits.\n", line_count);
				return -1;
			}

			// Verify that record length and byte count field correlate.
			if((size_t)record[REC_OFFSET_BYTE_COUNT] + 5 != record_len) {
				printf("Invalid line %u of Intel Hex file; byte count field value does not correlate with record length.\n", line_count);
				return -1;
			}

			// Calculate the record's checksum. Sum all record bytes preceding the checksum, take the
			// LSB of the sum, then calculate two's complement of it (i.e. invert bits and add 1).
			checksum = 0;
			for(size_t i = 0; i < record_len - 1; i++) {
				checksum += record[i];
			}
			checksum = ~checksum + 1;

#ifdef IHEX_DETAIL
			printf("Checksum: Record = 0x%02X, Calculated = 0x%02X\n", record[record_len -1], checksum);
#endif

			// Verify calculated checksum matches record's.
			if(checksum != record[record_len - 1]) {
				printf("Invalid line %u of Intel Hex file; failed checksum verification (got %02X, expected %02X).\n", line_count, checksum, record[record_len - 1]);
				return -1;
			}

			switch(record[REC_OFFSET_TYPE]) {
				case REC_TYPE_DATA:
					// Take the data bytes contained in the record and write them to the specified
					// memory address, offset by any previously defined extended address.
					mem_addr = be16toh(record[REC_OFFSET_ADDR]) + mem_offset;
					for(size_t i = 0; i < record[REC_OFFSET_BYTE_COUNT]; i++) {
#ifdef IHEX_DETAIL
						printf("Mem Write: Addr = 0x%08X, Byte = 0x%02X\n", mem_addr, record[REC_OFFSET_DATA + i]);
#endif
						memory_write(mem_addr++, record[REC_OFFSET_DATA + i]);
					}
					break;
				case REC_TYPE_EOF:
					// Don't process file any further.
					end = true;
					break;
				case REC_TYPE_EXT_SEG_ADDR:
					// Data field contains a 16-bit segment base address. Multiply by 16, to be added
					// to any further record addresses.
					mem_offset = be16toh(record[REC_OFFSET_DATA]) << 4;
#ifdef IHEX_DETAIL
					printf("Offset: Addr = 0x%08X\n", mem_offset);
#endif
					break;
				case REC_TYPE_EXT_LIN_ADDR:
					// Data field contains upper 16-bits of a 32-bit absolute address. Shift to upper
					// 16 bits, to be added to any further record addresses.
					mem_offset = be16toh(record[REC_OFFSET_DATA]) << 16;
#ifdef IHEX_DETAIL
					printf("Offset: Addr = 0x%08X\n", mem_offset);
#endif
					break;
				case REC_TYPE_START_SEG_ADDR:
				case REC_TYPE_START_LIN_ADDR:
					// Don't need to handle these; just skip them.
					break;
				default:
					printf("Invalid line %u of Intel Hex file; unknown record type (%u).\n", line_count, record[REC_OFFSET_TYPE]);
					return -1;
			}
		}

		if(ferror(ihex)) {
			printf("Error reading file!\n"); // TODO: print err msg with strerror()?
			return -1;
		}
	} while(!end && !feof(ihex));

	fclose(ihex);

	return 0;
}
