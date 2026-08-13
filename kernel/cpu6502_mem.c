#include "cpu6502.h"

/* Flat 64KB emulated address space -- backing RAM for everything that
   isn't a live hardware register. */
static uint8_t c64_ram[65536];

/* --- Minimal memory-mapped I/O stubs ---
   Real KERNAL/BASIC code polls hardware registers during init (timing
   loops, raster waits, etc). Since there's no real VIC-II/CIA/SID chip
   underneath us, reads to these addresses need to return *something*
   that changes over time, or KERNAL code waiting for a value to change
   will spin forever. These are deliberately crude for now -- just enough
   to unblock boot -- and will be replaced with real chip emulation later
   as we discover which registers actually matter. */

#define VIC_RASTER  0xD012  /* current raster scanline (low 8 bits) */
#define CIA1_PRB    0xDC01   /* keyboard matrix column reads -- real
                                 hardware pulls unconnected lines high,
                                 so idle (no key pressed) reads $FF, not
                                 $00. Confirmed against real KERNAL
                                 source (scnkey.s): it checks this
                                 register for $FF to detect "no keys" --
                                 our zero-initialized RAM was returning
                                 $00, which looks like every key on every
                                 row held down simultaneously, and was
                                 stalling the keyboard scan routine. */

#define PAL_RASTER_LINES 312

static uint32_t raster_counter = 0;

static uint8_t io_read(uint16_t addr)
{
    switch (addr) {
    case VIC_RASTER:
        raster_counter = (raster_counter + 1) % PAL_RASTER_LINES;
        return (uint8_t)(raster_counter & 0xFF);
    case CIA1_PRB:
        return 0xFF; /* no key currently pressed -- real host keyboard
                        input isn't wired into the emulated matrix yet */
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
}
