#include "console.h"
#include "io.h"

#define VGA_MEM ((uint16_t*)0xB8000)
#define VGA_WHITE_ON_BLACK 0x0F00
#define VGA_COLS 80
#define VGA_ROWS 25

static int cursor_pos = 0;
static int line_start = 0;

static void update_cursor(void)
{
    uint16_t pos = cursor_pos / 2;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void console_set_hw_cursor(int row, int col)
{
    if (row < 0) row = 0;
    if (row >= VGA_ROWS) row = VGA_ROWS - 1;
    if (col < 0) col = 0;
    if (col >= VGA_COLS) col = VGA_COLS - 1;

    uint16_t pos = (uint16_t)(row * VGA_COLS + col);
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

static void scroll_if_needed(void)
{
    uint16_t *vga = VGA_MEM;
    if (cursor_pos < VGA_COLS * VGA_ROWS * 2) return;

    for (int i = 0; i < VGA_COLS * (VGA_ROWS - 1); i++) {
        vga[i] = vga[i + VGA_COLS];
    }
    for (int i = VGA_COLS * (VGA_ROWS - 1); i < VGA_COLS * VGA_ROWS; i++) {
        vga[i] = VGA_WHITE_ON_BLACK | ' ';
    }
    cursor_pos -= VGA_COLS * 2;
    if (line_start >= VGA_COLS * 2) line_start -= VGA_COLS * 2;
}

void console_init(void)
{
    cursor_pos = 0;
    line_start = 0;
    update_cursor();
}

void console_clear(void)
{
    uint16_t *vga = VGA_MEM;
    for (int i = 0; i < VGA_COLS * VGA_ROWS; i++) {
        vga[i] = VGA_WHITE_ON_BLACK | ' ';
    }
    cursor_pos = 0;
    line_start = 0;
    update_cursor();
}

void console_mark_line_start(void)
{
    line_start = cursor_pos;
}

void console_putchar(char c)
{
    uint16_t *vga = VGA_MEM;

    if (c == '\n') {
        cursor_pos += (VGA_COLS * 2) - (cursor_pos % (VGA_COLS * 2));
    } else {
        vga[cursor_pos / 2] = VGA_WHITE_ON_BLACK | (uint8_t)c;
        cursor_pos += 2;
    }

    scroll_if_needed();
    update_cursor();
}

void console_backspace(void)
{
    uint16_t *vga = VGA_MEM;
    if (cursor_pos <= line_start) return;
    cursor_pos -= 2;
    vga[cursor_pos / 2] = VGA_WHITE_ON_BLACK | ' ';
    update_cursor();
}

void console_putchar_at(int row, int col, char c)
{
    uint16_t *vga = VGA_MEM;
    if (row < 0 || row >= VGA_ROWS || col < 0 || col >= VGA_COLS) return;
    vga[row * VGA_COLS + col] = VGA_WHITE_ON_BLACK | (uint8_t)c;
}

void console_print(const char *s)
{
    while (*s) {
        console_putchar(*s);
        s++;
    }
}

void console_print_hex(uint32_t val)
{
    console_print("0x");
    char buf[9];
    buf[8] = '\0';
    const char *digits = "0123456789ABCDEF";
    for (int i = 7; i >= 0; i--) {
        buf[i] = digits[val & 0xF];
        val >>= 4;
    }
    console_print(buf);
}

void console_print_dec(uint32_t val)
{
    if (val == 0) {
        console_putchar('0');
        return;
    }
    char buf[11];
    int i = 10;
    buf[i] = '\0';
    while (val > 0 && i > 0) {
        buf[--i] = '0' + (val % 10);
        val /= 10;
    }
    console_print(&buf[i]);
}
