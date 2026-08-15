#include "keyboard.h"
#include "irq.h"
#include "io.h"
#include "shell.h"
#include "c64.h"
#include <stdint.h>

static int shift_pressed = 0;
static int c64_mode_active = 0;
static volatile int f12_pressed = 0;

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

#define F12_SCANCODE 0x58

static volatile uint32_t handler_call_count = 0;
static volatile uint8_t last_scancode = 0;
static volatile uint8_t scancode_history[64];
static volatile int scancode_history_pos = 0;

uint32_t keyboard_get_handler_call_count(void)
{
    return handler_call_count;
}

uint8_t keyboard_get_last_scancode(void)
{
    return last_scancode;
}

uint8_t keyboard_get_history(int index)
{
    return scancode_history[index & 0x3F];
}

static void keyboard_handler(void)
{
    uint8_t scancode = inb(0x60);
    handler_call_count++;
    last_scancode = scancode;
    scancode_history[scancode_history_pos & 0x3F] = scancode;
    scancode_history_pos++;

    if (scancode == F12_SCANCODE) {
        f12_pressed = 1;
        return;
    }

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

int keyboard_f12_was_pressed(void)
{
    if (f12_pressed) {
        f12_pressed = 0;
        return 1;
    }
    return 0;
}

/* Returns a decoded ASCII character if a key was pressed since the last
   poll, or 0 if nothing is waiting. Reads the keyboard controller
   directly (port 0x64 status, 0x60 data) rather than going through the
   IRQ-driven handler -- used when IRQ1 is masked during live C64 mode. */
char keyboard_poll_char(void)
{
    uint8_t status = inb(0x64);
    if (!(status & 0x01)) {
        return 0; /* nothing waiting */
    }

    uint8_t scancode = inb(0x60);

    if (scancode & 0x80) {
        if (scancode == LSHIFT_RELEASE || scancode == RSHIFT_RELEASE) {
            shift_pressed = 0;
        }
        return 0;
    }

    if (scancode == LSHIFT_PRESS || scancode == RSHIFT_PRESS) {
        shift_pressed = 1;
        return 0;
    }

    if (scancode == F12_SCANCODE) {
        f12_pressed = 1;
        return 0;
    }

    if (scancode >= 128) return 0;

    return shift_pressed ? scancode_ascii_shift[scancode] : scancode_ascii[scancode];
}

void keyboard_install(void)
{
    irq_install_handler(1, keyboard_handler);
}
