#ifndef _ARGS_H_
#define _ARGS_H_

#include <stdint.h>

extern double speedarg;
extern uint8_t showclock, showdisplay, overridecpu, exitonIWDG, disableIWDG;
extern char* elffile;
extern char* ramfile;
extern char* eepromfile;
extern char* product_name;

int args_parse(int argc, char* argv[]);
void args_showHelp();

#endif
