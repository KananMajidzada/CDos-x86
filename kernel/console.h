#ifndef CONSOLE_H
#define CONSOLE_H
#include <stdint.h>

void console_init(void);
void console_clear(void);
void console_putchar(char c);
void console_print(const char *s);
void console_backspace(void);
void console_mark_line_start(void);
void console_print_hex(uint32_t val);
void console_print_dec(uint32_t val);

#endif
