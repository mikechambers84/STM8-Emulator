#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "memory.h"
#include "endian.h"
#include "srec.h"

// #define SREC_DETAIL

// Nominal max length in characters of a record is 514 (excluding line breaks
// and terminating null).
#define LINE_MAX_LEN 520

// Minimum posible line length in characters is 16-bit termination record:
// 2 digit start/type + 2 digit byte count + 4 digit addr + 2 digit checksum.
#define LINE_MIN_LEN 10

// Maximum specifyable byte count in a record is 255, so max record length in
// bytes is that plus size of other fields (type, count, 32-bit addr, checksum).
#define REC_MAX_LEN 262

#define REC_OFFSET_TYPE 0
#define REC_OFFSET_BYTE_COUNT 1
#define REC_OFFSET_ADDR 2
#define REC_OFFSET_DATA_S1 4
#define REC_OFFSET_DATA_S2 5
#define REC_OFFSET_DATA_S3 6

// Amount by which the byte count field is greater than the actual number of
// data bytes (i.e. minus address and checksum field lengths).
#define REC_COUNT_EXCESS_S1 3
#define REC_COUNT_EXCESS_S2 4
#define REC_COUNT_EXCESS_S3 5

#define REC_TYPE_S0_HEADER 0x0
#define REC_TYPE_S1_DATA 0x1
#define REC_TYPE_S2_DATA 0x2
#define REC_TYPE_S3_DATA 0x3
#define REC_TYPE_S5_COUNT 0x5
#define REC_TYPE_S6_COUNT 0x6
#define REC_TYPE_S7_TERM 0x7
#define REC_TYPE_S8_TERM 0x8
#define REC_TYPE_S9_TERM 0x9

static uint8_t srec_convert_hex_char(char c) {
	c = toupper(c);
	if(isdigit(c)) {
		return c - '0';
	} else {
		return c - ('A' - 10);
	}
}

static size_t srec_decode_record(char* line, size_t line_len, uint8_t* buf, size_t buf_len) {
	size_t rec_len = 0;
	uint8_t val;

	// Check first character of line is start code (uppercase 'S'). If so, substitute it
	// for a zero digit, so that the initial pair can be parsed below with general hex
	// pair handling.
	if(line_len > 0 && *line == 'S') {
		*line = '0';
	} else {
		return 0;
	}

	while(line_len > 0) {
		if(isxdigit(*line) && isxdigit(*(line + 1))) {
			// Convert pair of hex digit characters into byte value.
			val = (srec_convert_hex_char(*line) << 4) | srec_convert_hex_char(*(line + 1));
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

int srec_load(const char* path) {
	FILE* srec;
	char line[LINE_MAX_LEN];
	unsigned int line_count = 0, mem_addr;
	uint8_t record[REC_MAX_LEN], checksum;
	size_t line_len, record_len;
	bool end = false;

	// Open the file in text mode (so CRLF line endings are automatically converted to LF).
	srec = fopen(path, "r");
	if(srec == NULL) {
		return -1;
	}

	do {
		memset(line, '\0', sizeof(line));

		if(fgets(line, sizeof(line) / sizeof(line[0]), srec) != NULL) {
			line_count++;
			line_len = strlen(line);

			// Remove any trailing line feed character from line string.
			if(line[line_len - 1] == '\n') {
				line[line_len - 1] = '\0';
				line_len--;
			}

			// Check for minimum line length.
			if(line_len < LINE_MIN_LEN) {
				printf("Invalid line %u of SREC file; incomplete record, not enough characters.\n", line_count);
				return -1;
			}

#ifdef SREC_DETAIL
			printf("Line %u: \"%s\" (%zu chars)\n", line_count, line, line_len);
#endif

			// Decode the line into a record.
			record_len = srec_decode_record(line, line_len, record, sizeof(record));
			if(record_len == 0) {
				printf("Invalid line %u of SREC file; error parsing hex digits.\n", line_count);
				return -1;
			}

			// Verify that record length and byte count field correlate. Count field also
			// includes address and checksum fields.
			if((size_t)record[REC_OFFSET_BYTE_COUNT] + 2 != record_len) {
				printf("Invalid line %u of SREC file; byte count field value does not correlate with record length.\n", line_count);
				return -1;
			}

			// Calculate the record's checksum. Sum all record bytes between type and checksum, take the
			// LSB of the sum, then calculate one's complement of it (i.e. invert bits).
			checksum = 0;
			for(size_t i = 1; i < record_len - 1; i++) {
				checksum += record[i];
			}
			checksum = ~checksum;

#ifdef SREC_DETAIL
			printf("Checksum: Record = 0x%02X, Calculated = 0x%02X\n", record[record_len - 1], checksum);
#endif

			// Verify calculated checksum matches record's.
			if(checksum != record[record_len - 1]) {
				printf("Invalid line %u of SREC file; failed checksum verification (got %02X, expected %02X).\n", line_count, checksum, record[record_len - 1]);
				return -1;
			}

			switch(record[REC_OFFSET_TYPE]) {
				case REC_TYPE_S1_DATA:
					// Take data bytes from the record and write to the specified 16-bit memory address.
					mem_addr = be16toh(record[REC_OFFSET_ADDR]);
					for(size_t i = 0; i < record[REC_OFFSET_BYTE_COUNT] - REC_COUNT_EXCESS_S1; i++) {
#ifdef SREC_DETAIL
						printf("Mem Write: Addr = 0x%08X, Byte = 0x%02X\n", mem_addr, record[REC_OFFSET_DATA_S1 + i]);
#endif
						memory_write(mem_addr++, record[REC_OFFSET_DATA_S1 + i]);
					}
					break;
				case REC_TYPE_S2_DATA:
					// Take data bytes from the record and write to the specified 24-bit memory address.
					mem_addr = be24toh(record[REC_OFFSET_ADDR]);
					for(size_t i = 0; i < record[REC_OFFSET_BYTE_COUNT] - REC_COUNT_EXCESS_S2; i++) {
#ifdef SREC_DETAIL
						printf("Mem Write: Addr = 0x%08X, Byte = 0x%02X\n", mem_addr, record[REC_OFFSET_DATA_S2 + i]);
#endif
						memory_write(mem_addr++, record[REC_OFFSET_DATA_S2 + i]);
					}
					break;
				case REC_TYPE_S3_DATA:
					// Take data bytes from the record and write to the specified 32-bit memory address.
					mem_addr = be32toh(record[REC_OFFSET_ADDR]);
					for(size_t i = 0; i < record[REC_OFFSET_BYTE_COUNT] - REC_COUNT_EXCESS_S3; i++) {
#ifdef SREC_DETAIL
						printf("Mem Write: Addr = 0x%08X, Byte = 0x%02X\n", mem_addr, record[REC_OFFSET_DATA_S3 + i]);
#endif
						memory_write(mem_addr++, record[REC_OFFSET_DATA_S3 + i]);
					}
					break;
				case REC_TYPE_S0_HEADER:
				case REC_TYPE_S5_COUNT:
				case REC_TYPE_S6_COUNT:
					// Don't really need to handle these; ignore them.
					break;
				case REC_TYPE_S7_TERM:
				case REC_TYPE_S8_TERM:
				case REC_TYPE_S9_TERM:
					// Don't process file any further.
					end = true;
					break;
				default:
					printf("Invalid line %u of SREC file; unknown record type (%u).\n", line_count, record[REC_OFFSET_TYPE]);
					return -1;
			}
		}

		if(ferror(srec)) {
			printf("Error reading file!\n"); // TODO: print err msg with strerror()?
			return -1;
		}
	} while(!end && !feof(srec));

	fclose(srec);

	return 0;
}
