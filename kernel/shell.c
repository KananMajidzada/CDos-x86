#include "shell.h"
#include "console.h"
#include "string.h"
#include "ata.h"
#include "fs.h"
#include "keyboard.h"
#include "cpu6502_test.h"
#include "pic.h"
#include "c64.h"
#include <stdint.h>
#include <stddef.h>

#define LINE_MAX 128

static char line_buf[LINE_MAX];
static int line_len = 0;

static void shell_prompt(void)
{
    console_print("NULL:\\>");
    console_mark_line_start();
}

void shell_init(void)
{
    line_len = 0;
    shell_prompt();
}

static int parse_number(const char *s, uint32_t *out)
{
    uint32_t val = 0;

    if (*s == '$') {
        s++;
        if (!*s) return 0;
        while (*s) {
            char c = *s;
            uint32_t digit;
            if (c >= '0' && c <= '9') digit = c - '0';
            else if (c >= 'A' && c <= 'F') digit = c - 'A' + 10;
            else if (c >= 'a' && c <= 'f') digit = c - 'a' + 10;
            else return 0;
            val = (val << 4) | digit;
            s++;
        }
        *out = val;
        return 1;
    }

    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        s += 2;
        if (!*s) return 0;
        while (*s) {
            char c = *s;
            uint32_t digit;
            if (c >= '0' && c <= '9') digit = c - '0';
            else if (c >= 'A' && c <= 'F') digit = c - 'A' + 10;
            else if (c >= 'a' && c <= 'f') digit = c - 'a' + 10;
            else return 0;
            val = (val << 4) | digit;
            s++;
        }
        *out = val;
        return 1;
    }

    if (!*s) return 0;
    while (*s) {
        if (*s < '0' || *s > '9') return 0;
        val = val * 10 + (*s - '0');
        s++;
    }
    *out = val;
    return 1;
}

static int tokenize(char *line, char *tokens[4])
{
    int count = 0;
    char *p = line;

    while (*p && count < 4) {
        while (*p == ' ') p++;
        if (!*p) break;
        tokens[count++] = p;
        while (*p && *p != ' ') p++;
        if (*p) { *p = '\0'; p++; }
    }
    return count;
}

static void cmd_help(void)
{
    console_print("Available commands:\n");
    console_print("  HELP           - show this list\n");
    console_print("  CLEAR          - clear the screen\n");
    console_print("  PEEK <addr>    - read a byte from memory\n");
    console_print("  POKE <addr> <val> - write a byte to memory\n");
    console_print("  DUMP <addr> <len> - hex dump memory\n");
    console_print("  DIR            - list files on disk\n");
    console_print("  TYPE <file>    - print a text file\n");
    console_print("  DISKTEST       - test raw disk read/write\n");
    console_print("  CPUTEST        - run 6502 interpreter self-tests\n");
    console_print("  C64BOOT        - enter the C64 (ESC to exit)\n");
    console_print("  C64KEY <char>  - inject one character into C64\n");
    console_print("  C64ENTER       - inject RETURN into C64\n");
    console_print("  C64RESUME      - continue C64 execution (legacy)\n");
    console_print("  C64SCREEN      - show current C64 screen\n");
    console_print("  C64MEM <a> <l> - dump C64 memory\n");
    console_print("  C64TRACE <n>   - step-trace the C64 CPU\n");
    console_print("Addresses/values: decimal, 0xHEX, or $HEX\n");
}

static void cmd_peek(char *tokens[4], int count)
{
    if (count < 2) {
        console_print("Usage: PEEK <addr>\n");
        return;
    }
    uint32_t addr;
    if (!parse_number(tokens[1], &addr)) {
        console_print("Bad address\n");
        return;
    }
    uint8_t val = *(volatile uint8_t *)addr;
    console_print("[");
    console_print_hex(addr);
    console_print("] = ");
    console_print_hex((uint32_t)val);
    console_print(" (");
    console_print_dec((uint32_t)val);
    console_print(")\n");
}

static void cmd_poke(char *tokens[4], int count)
{
    if (count < 3) {
        console_print("Usage: POKE <addr> <val>\n");
        return;
    }
    uint32_t addr, val;
    if (!parse_number(tokens[1], &addr) || !parse_number(tokens[2], &val)) {
        console_print("Bad address or value\n");
        return;
    }
    *(volatile uint8_t *)addr = (uint8_t)val;
    console_print("OK\n");
}

static void cmd_dump(char *tokens[4], int count)
{
    if (count < 3) {
        console_print("Usage: DUMP <addr> <len>\n");
        return;
    }
    uint32_t addr, len;
    if (!parse_number(tokens[1], &addr) || !parse_number(tokens[2], &len)) {
        console_print("Bad address or length\n");
        return;
    }
    if (len > 256) len = 256;

    for (uint32_t i = 0; i < len; i += 16) {
        console_print_hex(addr + i);
        console_print(": ");
        for (uint32_t j = 0; j < 16 && (i + j) < len; j++) {
            uint8_t b = *(volatile uint8_t *)(addr + i + j);
            const char *digits = "0123456789ABCDEF";
            console_putchar(digits[(b >> 4) & 0xF]);
            console_putchar(digits[b & 0xF]);
            console_putchar(' ');
        }
        console_putchar('\n');
    }
}

static void dir_print_entry(const struct fs_entry *e)
{
    console_print("  ");
    console_print(e->name);
    console_print("  ");
    console_print_dec(e->length);
    console_print(" bytes\n");
}

static void cmd_dir(void)
{
    if (fs_mount() != 0) {
        console_print("No disk mounted or unformatted disk.\n");
        return;
    }
    console_print("Directory listing:\n");
    fs_list(dir_print_entry);
}

static void cmd_type(char *tokens[4], int count)
{
    if (count < 2) {
        console_print("Usage: TYPE <filename>\n");
        return;
    }

    if (fs_mount() != 0) {
        console_print("No disk mounted or unformatted disk.\n");
        return;
    }

    struct fs_entry entry;
    if (fs_find(tokens[1], &entry) != 0) {
        console_print("File not found: ");
        console_print(tokens[1]);
        console_putchar('\n');
        return;
    }

    uint8_t buffer[513];
    int len = fs_read_file(&entry, buffer, 512);
    if (len < 0) {
        console_print("Read error.\n");
        return;
    }
    buffer[len] = '\0';
    console_print((const char *)buffer);
    console_putchar('\n');
}

static void cmd_c64screen(void);

static void cmd_c64boot(void)
{
    if (c64_init() != 0) {
        console_print("C64 init failed.\n");
        return;
    }
    struct cpu6502 *cpu = c64_get_cpu();

    console_clear();

    /* Mask the keyboard IRQ -- we're switching to direct polling instead,
       so the interrupt-driven handler shouldn't also be racing to
       consume the same keystrokes. */
    pic_set_mask(1);

    int killed_by_f12 = 0;
    long total_steps = 0;

    long steps_since_irq = 0;

    while (!cpu->halted) {
        /* Small batch -- frequent enough to poll the keyboard responsively. */
        for (int i = 0; i < 5000 && !cpu->halted; i++) {
            cpu6502_step(cpu);
            total_steps++;
            steps_since_irq++;
        }

        /* Fire the jiffy/keyboard-scan IRQ much less often than every batch --
           our interpreter runs far faster than real 6502 hardware, so ticking
           the IRQ every batch made KERNAL's cursor blink toggle absurdly fast,
           making typed characters statistically invisible mid-blink. */
        if (steps_since_irq >= 200000) {
            cpu6502_irq(cpu);
            steps_since_irq = 0;
        }

        char c = keyboard_poll_char();
        if (c != 0) {
            c64_inject_key(c);
        }

        if (keyboard_f12_was_pressed()) {
            killed_by_f12 = 1;
            break;
        }

        if (c64_exit_was_requested()) {
            break;
        }
    }

    pic_clear_mask(1); /* restore normal interrupt-driven keyboard for our own shell */
    console_clear();

    if (killed_by_f12) {
        console_print("Exited via F12.\n");
    } else if (cpu->halted) {
        console_print("C64 halted -- opcode at PC-1: $");
        console_print_hex((uint32_t)cpu6502_read((uint16_t)(cpu->pc - 1)));
        console_putchar('\n');
    } else {
        console_print("Exited C64 mode.\n");
    }
}

static void cmd_c64mem(char *tokens[4], int count)
{
    if (count < 3) {
        console_print("Usage: C64MEM <addr> <len>\n");
        return;
    }
    uint32_t addr, len;
    if (!parse_number(tokens[1], &addr) || !parse_number(tokens[2], &len)) {
        console_print("Bad address or length\n");
        return;
    }
    if (len > 64) len = 64;

    for (uint32_t i = 0; i < len; i += 16) {
        console_print_hex(addr + i);
        console_print(": ");
        for (uint32_t j = 0; j < 16 && (i + j) < len; j++) {
            uint8_t b = cpu6502_read((uint16_t)(addr + i + j));
            const char *digits = "0123456789ABCDEF";
            console_putchar(digits[(b >> 4) & 0xF]);
            console_putchar(digits[b & 0xF]);
            console_putchar(' ');
        }
        console_putchar('\n');
    }
}

static void cmd_c64trace(char *tokens[4], int count)
{
    uint32_t trace_steps = 30; /* default */
    if (count >= 2) {
        if (!parse_number(tokens[1], &trace_steps)) {
            console_print("Bad step count\n");
            return;
        }
    }
    if (trace_steps > 200) trace_steps = 200; /* keep output readable */

    struct cpu6502 *cpu = c64_get_cpu();

    console_print("Tracing ");
    console_print_dec(trace_steps);
    console_print(" steps from current state:\n");

    for (uint32_t i = 0; i < trace_steps; i++) {
        if (cpu->halted) {
            console_print("(halted)\n");
            break;
        }
        uint16_t pc_before = cpu->pc;
        uint8_t opcode = cpu6502_read(pc_before);

        console_print("PC=$");
        console_print_hex((uint32_t)pc_before);
        console_print(" op=$");
        console_print_hex((uint32_t)opcode);
        console_print(" A=");
        console_print_dec((uint32_t)cpu->a);
        console_print(" X=");
        console_print_dec((uint32_t)cpu->x);
        console_print(" Y=");
        console_print_dec((uint32_t)cpu->y);
        console_print(" SP=");
        console_print_dec((uint32_t)cpu->sp);
        console_print(" P=$");
        console_print_hex((uint32_t)cpu->p);
        console_print(cpu->p & FLAG_I ? " (I=1)" : " (I=0)");
        console_putchar('\n');

        cpu6502_step(cpu);
        if ((i + 1) % 20 == 0) {
            cpu6502_irq(cpu);
        }
    }
}

static void cmd_c64key(char *tokens[4], int count)
{
    if (count < 2 || tokens[1][0] == '\0') {
        console_print("Usage: C64KEY <single char>\n");
        return;
    }
    c64_inject_key(tokens[1][0]);
    console_print("Injected '");
    console_putchar(tokens[1][0]);
    console_print("' into C64 keyboard buffer.\n");
}

static void cmd_c64type(const char *line)
{
    /* line points into the original raw input, past "C64TYPE " --
       inject every character, then Enter, then run the CPU forward
       so BASIC actually processes what we just typed. */
    struct cpu6502 *cpu = c64_get_cpu();

    for (int i = 0; line[i] != '\0'; i++) {
        c64_inject_key(line[i]);
    }
    c64_inject_key((char)0x0D);

    console_print("Typed: ");
    console_print(line);
    console_putchar('\n');

    int steps_since_irq = 0;
    long steps = 0;
    long max_steps = 5000000L;
    while (!cpu->halted && steps < max_steps) {
        cpu6502_step(cpu);
        steps++;
        steps_since_irq++;
        if (steps_since_irq >= 1000) {
            cpu6502_irq(cpu);
            steps_since_irq = 0;
        }
    }

    console_print("Ran ");
    console_print_dec((uint32_t)steps);
    console_print(" steps. Use C64SCREEN to see the result.\n");
}

static void cmd_c64resume(void)
{
    struct cpu6502 *cpu = c64_get_cpu();

    console_print("Resuming... (running live -- type on your keyboard!)\n");

    keyboard_set_c64_mode(1);

    int max_steps = 5000000;
    int steps = 0;
    while (!cpu->halted && steps < max_steps) {
        cpu6502_step(cpu);
        steps++;
        if (steps % 1000 == 0) {
            cpu6502_irq(cpu);
        }
    }

    keyboard_set_c64_mode(0);

    console_print("Stopped after ");
    console_print_dec((uint32_t)steps);
    console_print(" steps.\n");
    console_print("  PC: $");
    console_print_hex(cpu->pc);
    console_print("  A: ");
    console_print_dec((uint32_t)cpu->a);
    console_print("  X: ");
    console_print_dec((uint32_t)cpu->x);
    console_print("  Y: ");
    console_print_dec((uint32_t)cpu->y);
    console_print("  SP: ");
    console_print_dec((uint32_t)cpu->sp);
    console_putchar('\n');

    if (cpu->halted) {
        console_print("  Halted -- opcode at PC-1: $");
        console_print_hex((uint32_t)cpu6502_read((uint16_t)(cpu->pc - 1)));
        console_putchar('\n');
    }
}

static void cmd_c64enter(void)
{
    c64_inject_key((char)0x0D);
    console_print("Injected RETURN into C64 keyboard buffer.\n");
}

static char screen_code_to_ascii(uint8_t sc)
{
    /* Mask off inverse-video bit and fold into base 0-63 range */
    uint8_t base = sc & 0x7F;
    if (base > 63) base = (uint8_t)(base - 64);

    if (base == 0) return '@';
    if (base >= 1 && base <= 26) return (char)('A' + (base - 1));
    if (base == 27) return '[';
    if (base == 28) return '$'; /* pound sign placeholder */
    if (base == 29) return ']';
    if (base == 30) return '^';
    if (base == 31) return '<';
    if (base >= 32 && base <= 63) return (char)base; /* space..? matches ASCII */
    return '.';
}

static void cmd_c64screen(void)
{
    console_clear();
    console_print("C64 screen memory ($0400-$07E7), decoded:\n");
    console_print("+----------------------------------------+\n");

    for (int row = 0; row < 25; row++) {
        console_putchar('|');
        for (int col = 0; col < 40; col++) {
            uint16_t addr = (uint16_t)(0x0400 + row * 40 + col);
            uint8_t sc = cpu6502_read(addr);
            console_putchar(screen_code_to_ascii(sc));
        }
        console_print("|\n");
    }
    console_print("+----------------------------------------+\n");
}

static void cmd_kbdstat(void)
{
    console_print("Handler calls: ");
    console_print_dec(keyboard_get_handler_call_count());
    console_print("\nLast scancode: $");
    console_print_hex((uint32_t)keyboard_get_last_scancode());
    console_print("\nRecent history (oldest to newest): ");
    for (int i = 0; i < 64; i++) {
        console_print_hex((uint32_t)keyboard_get_history(i));
        console_putchar(' ');
    }
    console_putchar('\n');
}

static void cmd_disktest(void)
{
    uint8_t write_buf[512];
    uint8_t read_buf[512];

    for (int i = 0; i < 512; i++) {
        write_buf[i] = (uint8_t)(i & 0xFF);
    }

    console_print("Writing test pattern to LBA 0...\n");
    if (ata_write_sector(0, write_buf) != 0) {
        console_print("WRITE FAILED\n");
        return;
    }
    console_print("Write OK. Reading back...\n");

    for (int i = 0; i < 512; i++) read_buf[i] = 0;

    if (ata_read_sector(0, read_buf) != 0) {
        console_print("READ FAILED\n");
        return;
    }

    int mismatch = 0;
    for (int i = 0; i < 512; i++) {
        if (read_buf[i] != write_buf[i]) {
            mismatch = 1;
            break;
        }
    }

    if (mismatch) {
        console_print("MISMATCH -- data corrupted\n");
    } else {
        console_print("PASS -- 512 bytes verified\n");
    }
}

static void cmd_clear(void)
{
    console_clear();
    console_print("READY.\n");
}

static void shell_execute(char *line)
{
    /* C64TYPE needs the raw remainder of the line (spaces intact),
       so check for it BEFORE tokenize() destructively splits the
       string on spaces. */
    if (k_strncasecmp(line, "C64TYPE ", 8) == 0) {
        cmd_c64type(line + 8);
        return;
    }

    char *tokens[4];
    int count = tokenize(line, tokens);

    if (count == 0) return;

    if (k_strcasecmp(tokens[0], "HELP") == 0) {
        cmd_help();
    } else if (k_strcasecmp(tokens[0], "C64TRACE") == 0) {
        cmd_c64trace(tokens, count);
    } else if (k_strcasecmp(tokens[0], "C64MEM") == 0) {
        cmd_c64mem(tokens, count);
    } else if (k_strcasecmp(tokens[0], "C64SCREEN") == 0) {
        cmd_c64screen();
    } else if (k_strcasecmp(tokens[0], "C64ENTER") == 0) {
        cmd_c64enter();
    } else if (k_strcasecmp(tokens[0], "C64RESUME") == 0) {
        cmd_c64resume();
    } else if (k_strcasecmp(tokens[0], "C64KEY") == 0) {
        cmd_c64key(tokens, count);
    } else if (k_strcasecmp(tokens[0], "KBDSTAT") == 0) {
        cmd_kbdstat();
    } else if (k_strcasecmp(tokens[0], "C64BOOT") == 0) {
        cmd_c64boot();
    } else if (k_strcasecmp(tokens[0], "CPUTEST") == 0) {
        cpu6502_run_test();
    } else if (k_strcasecmp(tokens[0], "DIR") == 0) {
        cmd_dir();
    } else if (k_strcasecmp(tokens[0], "TYPE") == 0) {
        cmd_type(tokens, count);
    } else if (k_strcasecmp(tokens[0], "DISKTEST") == 0) {
        cmd_disktest();
    } else if (k_strcasecmp(tokens[0], "CLEAR") == 0) {
        cmd_clear();
    } else if (k_strcasecmp(tokens[0], "PEEK") == 0) {
        cmd_peek(tokens, count);
    } else if (k_strcasecmp(tokens[0], "POKE") == 0) {
        cmd_poke(tokens, count);
    } else if (k_strcasecmp(tokens[0], "DUMP") == 0) {
        cmd_dump(tokens, count);
    } else {
        console_print("Unknown command: ");
        console_print(tokens[0]);
        console_putchar('\n');
    }
}

void shell_input_char(char c)
{
    if (c == '\n') {
        console_putchar('\n');
        line_buf[line_len] = '\0';
        shell_execute(line_buf);
        line_len = 0;
        shell_prompt();
    } else if (c == '\b') {
        if (line_len > 0) {
            line_len--;
            console_backspace();
        }
    } else {
        if (line_len < LINE_MAX - 1) {
            line_buf[line_len++] = c;
            console_putchar(c);
        }
    }
}
