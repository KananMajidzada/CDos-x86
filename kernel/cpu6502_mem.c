#include "cpu6502.h"
#include "console.h"

static uint8_t c64_ram[65536];

#define VIC_RASTER  0xD012
#define CIA1_PRB    0xDC01
#define PAL_RASTER_LINES 312

#define C64_SCREEN_START 0x0400
#define C64_SCREEN_END   0x07E7 /* inclusive: 40*25 - 1 cells from start */
#define C64_SCREEN_COLS  40

static uint32_t raster_counter = 0;

static char screen_code_to_ascii(uint8_t sc)
{
    uint8_t masked = sc & 0x7F;

    /* Codes 64-127 (before masking to 0-63) come from the alternate/
       graphics charset -- mostly decorative line-drawing glyphs with no
       sensible ASCII equivalent. Render as blank rather than guessing. */
    if (masked > 63) return ' ';

    uint8_t base = masked;

    if (base == 0) return '@';
    if (base >= 1 && base <= 26) return (char)('A' + (base - 1));
    if (base == 27) return '[';
    if (base == 28) return ' '; /* pound sign -- no ASCII equivalent */
    if (base == 29) return ']';
    if (base == 30) return '^';
    if (base == 31) return '<';
    if (base >= 32 && base <= 63) return (char)base;
    return ' ';
}

static uint8_t io_read(uint16_t addr)
{
    switch (addr) {
    case VIC_RASTER:
        raster_counter = (raster_counter + 1) % PAL_RASTER_LINES;
        return (uint8_t)(raster_counter & 0xFF);
    case CIA1_PRB:
        return 0xFF;
    default:
        return c64_ram[addr];
    }
}

uint8_t cpu6502_read(uint16_t addr)
{
    if (addr == VIC_RASTER || addr == CIA1_PRB) {
        return io_read(addr);
    }
    return c64_ram[addr];
}

void cpu6502_write(uint16_t addr, uint8_t val)
{
    c64_ram[addr] = val;

    if (addr >= C64_SCREEN_START && addr <= C64_SCREEN_END) {
        uint16_t offset = (uint16_t)(addr - C64_SCREEN_START);
        int row = offset / C64_SCREEN_COLS;
        int col = offset % C64_SCREEN_COLS;
        console_putchar_at(row, col, screen_code_to_ascii(val));
        console_set_hw_cursor(row, col + 1);
    }
}
