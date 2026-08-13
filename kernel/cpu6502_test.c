#include "cpu6502.h"
#include "console.h"

/*
 * Test 1: counting loop (LDX/INX/CPX/BNE/BRK)
 * Test 2: JSR/RTS subroutine call/return
 */

static void test1_loop(void)
{
    struct cpu6502 cpu;
    uint16_t base = 0x0800;

    uint8_t program[] = {
        0xA2, 0x00,       /* LDX #$00 */
        0xE8,             /* loop: INX */
        0xE0, 0x05,       /* CPX #$05 */
        0xD0, 0xFB,       /* BNE loop */
        0x00              /* BRK */
    };

    for (int i = 0; i < (int)sizeof(program); i++) {
        cpu6502_write(base + i, program[i]);
    }

    cpu6502_reset(&cpu, base);

    int max_steps = 100;
    int steps = 0;
    while (!cpu.halted && steps < max_steps) {
        cpu6502_step(&cpu);
        steps++;
    }

    console_print("Test 1 (LDX/INX/CPX/BNE loop):\n");
    console_print("  Steps: ");
    console_print_dec((uint32_t)steps);
    console_print("  X: ");
    console_print_dec((uint32_t)cpu.x);
    console_print(" (expect 5)  Z: ");
    console_print_dec((cpu.p & FLAG_Z) ? 1 : 0);
    console_print(" (expect 1)  Halted: ");
    console_print_dec((uint32_t)cpu.halted);
    console_print(" (expect 1)\n");

    if (cpu.x == 5 && (cpu.p & FLAG_Z) && cpu.halted) {
        console_print("  RESULT: PASS\n\n");
    } else {
        console_print("  RESULT: FAIL\n\n");
    }
}

static void test2_jsr_rts(void)
{
    struct cpu6502 cpu;
    uint16_t base = 0x0900;

    /*
     * main:
     *   0900: LDA #$11        A9 11
     *   0902: JSR sub         20 09 09   (sub is at $0909)
     *   0905: STA $10         85 10      (should run AFTER sub returns)
     *   0907: BRK             00
     *   ...
     * sub:
     *   0909: LDA #$22        A9 22      (overwrites A -- proves we jumped in)
     *   090B: RTS             60
     *
     * Expected: if JSR/RTS +1/-1 addressing is correct, execution returns
     * to $0905 (the instruction right after JSR), runs STA $10, storing
     * whatever A held at that point ($22, from inside the subroutine),
     * then BRK. If the +1/-1 math were wrong, PC would land mid-instruction
     * and either crash (unknown opcode halt) or behave incorrectly.
     */
    uint8_t program[] = {
        0xA9, 0x11,             /* 0900: LDA #$11 */
        0x20, 0x09, 0x09,       /* 0902: JSR $0909 */
        0x85, 0x10,             /* 0905: STA $10 */
        0x00,                   /* 0907: BRK */
        0xEA,                   /* 0908: (padding, unused) */
        0xA9, 0x22,             /* 0909: LDA #$22 */
        0x60                    /* 090B: RTS */
    };

    for (int i = 0; i < (int)sizeof(program); i++) {
        cpu6502_write(base + i, program[i]);
    }

    cpu6502_reset(&cpu, base);

    int max_steps = 100;
    int steps = 0;
    while (!cpu.halted && steps < max_steps) {
        cpu6502_step(&cpu);
        steps++;
    }

    uint8_t mem_val = cpu6502_read(0x0010);

    console_print("Test 2 (JSR/RTS):\n");
    console_print("  Steps: ");
    console_print_dec((uint32_t)steps);
    console_print("  A: ");
    console_print_dec((uint32_t)cpu.a);
    console_print(" (expect 34/0x22)  mem[$10]: ");
    console_print_dec((uint32_t)mem_val);
    console_print(" (expect 34/0x22)  Halted: ");
    console_print_dec((uint32_t)cpu.halted);
    console_print(" (expect 1)\n");

    if (cpu.a == 0x22 && mem_val == 0x22 && cpu.halted) {
        console_print("  RESULT: PASS\n\n");
    } else {
        console_print("  RESULT: FAIL\n\n");
    }
}

void cpu6502_run_test(void)
{
    console_print("6502 CPU tests:\n");
    test1_loop();
    test2_jsr_rts();
}
