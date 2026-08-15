#ifndef KEYBOARD_H
#define KEYBOARD_H
#include <stdint.h>

void keyboard_install(void);
void keyboard_set_c64_mode(int active);
int keyboard_f12_was_pressed(void);
uint32_t keyboard_get_handler_call_count(void);
uint8_t keyboard_get_last_scancode(void);
uint8_t keyboard_get_history(int index);
char keyboard_poll_char(void);

#endif
