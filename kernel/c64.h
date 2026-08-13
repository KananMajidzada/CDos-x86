#ifndef C64_H
#define C64_H
#include "cpu6502.h"

int c64_init(void);
struct cpu6502 *c64_get_cpu(void);
void c64_inject_key(char c);
int c64_exit_was_requested(void);

#endif
