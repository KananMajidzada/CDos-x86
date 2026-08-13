#include "c64.h"
#include "cpu6502.h"
#include "fs.h"
#include "console.h"

#define BASIC_ROM_ADDR  0xA000
#define BASIC_ROM_SIZE  8192
#define KERNAL_ROM_ADDR 0xE000
#define KERNAL_ROM_SIZE 8192

static struct cpu6502 g_cpu;

static int load_rom(const char *filename, uint16_t load_addr, uint32_t expected_size)
{
    struct fs_entry entry;
    if (fs_find(filename, &entry) != 0) {
        console_print("ROM not found on disk: ");
        console_print(filename);
        console_putchar('\n');
        return -1;
    }

    if (entry.length != expected_size) {
        console_print("ROM size mismatch for ");
        console_print(filename);
        console_print(": expected ");
        console_print_dec(expected_size);
        console_print(", got ");
        console_print_dec(entry.length);
        console_putchar('\n');
        return -1;
    }

    uint8_t buffer[8192];
    int bytes_read = fs_read_file(&entry, buffer, sizeof(buffer));
    if (bytes_read != (int)expected_size) {
        console_print("ROM read failed for ");
        console_print(filename);
        console_putchar('\n');
        return -1;
    }

    for (uint32_t i = 0; i < expected_size; i++) {
        cpu6502_write((uint16_t)(load_addr + i), buffer[i]);
    }

    console_print("Loaded ");
    console_print(filename);
    console_print(" at $");
    console_print_hex(load_addr);
    console_putchar('\n');

    return 0;
}

int c64_init(void)
{
    if (fs_mount() != 0) {
        console_print("c64_init: disk not mounted, cannot load ROMs\n");
        return -1;
    }

    if (load_rom("KERNAL.ROM", KERNAL_ROM_ADDR, KERNAL_ROM_SIZE) != 0) return -1;
    if (load_rom("BASIC.ROM", BASIC_ROM_ADDR, BASIC_ROM_SIZE) != 0) return -1;

    uint8_t lo = cpu6502_read(0xFFFC);
    uint8_t hi = cpu6502_read(0xFFFD);
    uint16_t reset_vector = (uint16_t)(lo | (hi << 8));

    console_print("Reset vector: $");
    console_print_hex(reset_vector);
    console_putchar('\n');

    cpu6502_reset(&g_cpu, reset_vector);

    return 0;
}

struct cpu6502 *c64_get_cpu(void)
{
    return &g_cpu;
}

#define NDX  0x00C6
#define KEYD 0x0277
#define KEYD_MAX 10 /* $277-$280 = 10 bytes, real C64 buffer size */

static int exit_requested = 0;

void c64_inject_key(char c)
{
    if (c == 27) { /* ESC -- reserved as the exit key, never passed to C64 */
        exit_requested = 1;
        return;
    }

    uint8_t count = cpu6502_read(NDX);
    if (count >= KEYD_MAX) return; /* buffer full, drop the key like real hardware */

    cpu6502_write((uint16_t)(KEYD + count), (uint8_t)c);
    cpu6502_write(NDX, (uint8_t)(count + 1));
}

int c64_exit_was_requested(void)
{
    if (exit_requested) {
        exit_requested = 0;
        return 1;
    }
    return 0;
}
