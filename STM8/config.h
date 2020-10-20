#ifndef _CONFIG_H_
#define _CONFIG_H_

//#define DEBUG_OUTPUT

void (*product_init)(void);
void (*product_update)(void);
void (*product_portwrite)(uint32_t addr, uint8_t val);
uint8_t(*product_portread)(uint32_t addr, uint8_t* dst);
void (*product_markforupdate)(void);
void (*product_loop)(void);

#ifdef _WIN32
#define FUNC_INLINE __forceinline
#else
#define FUNC_INLINE __attribute__((always_inline))
#endif

#endif
