#include "keyboard.h"
#include "irq.h"
#include "io.h"
#include "shell.h"
#include "c64.h"
#include <stdint.h>

static int shift_pressed = 0;
static int c64_mode_active = 0;

void keyboard_set_c64_mode(int active)
{
    c64_mode_active = active;
}

static const char scancode_ascii[128] = {
    0,  27, '1','2','3','4','5','6','7','8','9','0','-','=','\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,  'a','s','d','f','g','h','j','k','l',';','\'','`',
    0, '\\','z','x','c','v','b','n','m',',','.','/', 0,
    '*', 0, ' ', 0,
};

static const char scancode_ascii_shift[128] = {
    0,  27, '!','@','#','$','%','^','&','*','(',')','_','+','\b',
    '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n',
    0,  'A','S','D','F','G','H','J','K','L',':','"','~',
    0, '|','Z','X','C','V','B','N','M','<','>','?', 0,
    '*', 0, ' ', 0,
};

#define LSHIFT_PRESS   0x2A
#define RSHIFT_PRESS   0x36
#define LSHIFT_RELEASE 0xAA
#define RSHIFT_RELEASE 0xB6

static void keyboard_handler(void)
{
    uint8_t scancode = inb(0x60);

    if (scancode & 0x80) {
        if (scancode == LSHIFT_RELEASE || scancode == RSHIFT_RELEASE) {
            shift_pressed = 0;
        }
        return;
    }

    if (scancode == LSHIFT_PRESS || scancode == RSHIFT_PRESS) {
        shift_pressed = 1;
        return;
    }

    if (scancode >= 128) return;

    char c = shift_pressed ? scancode_ascii_shift[scancode] : scancode_ascii[scancode];
    if (c != 0) {
        if (c64_mode_active) {
            c64_inject_key(c);
        } else {
            shell_input_char(c);
        }
    }
}

void keyboard_install(void)
{
    irq_install_handler(1, keyboard_handler);
}
