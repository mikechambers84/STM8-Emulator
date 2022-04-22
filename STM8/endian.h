#ifndef _ENDIAN_H_
#define _ENDIAN_H_

// Macros to take a 16-, 24- or 32-bit big-endian integer value and swap the bytes
// (if necessary) to host order. NOTE: will not work on literal values!
#define be16toh(x) ((((uint8_t *)&(x))[1] << 0) | (((uint8_t *)&(x))[0] << 8))
#define be24toh(x) ((((uint8_t *)&(x))[2] << 0) | (((uint8_t *)&(x))[1] << 8) | (((uint8_t *)&(x))[0] << 16))
#define be32toh(x) ((((uint8_t *)&(x))[3] << 0) | (((uint8_t *)&(x))[2] << 8) | (((uint8_t *)&(x))[1] << 16) | (((uint8_t *)&(x))[0] << 24))

// TODO: make work with literal values, perhaps by assigning arg to intermediate variable.

#endif
