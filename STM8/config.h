#ifndef _CONFIG_H_
#define _CONFIG_H_

#define DEBUG_OUTPUT

extern void (*product_init)(void);
extern void (*product_update)(void);
extern void (*product_portwrite)(uint32_t addr, uint8_t val);
extern uint8_t (*product_portread)(uint32_t addr, uint8_t* dst);
extern void (*product_markforupdate)(void);
extern void (*product_loop)(void);

#ifdef _WIN32
#define FUNC_INLINE __forceinline
#else
#define FUNC_INLINE __attribute__((always_inline))
#endif

#endif
