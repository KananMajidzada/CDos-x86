#include "cpu6502.h"

static void set_zn(struct cpu6502 *cpu, uint8_t val)
{
    if (val == 0) cpu->p |= FLAG_Z; else cpu->p &= ~FLAG_Z;
    if (val & 0x80) cpu->p |= FLAG_N; else cpu->p &= ~FLAG_N;
}

static uint8_t fetch8(struct cpu6502 *cpu)
{
    return cpu6502_read(cpu->pc++);
}

static uint16_t fetch16(struct cpu6502 *cpu)
{
    uint16_t lo = fetch8(cpu);
    uint16_t hi = fetch8(cpu);
    return lo | (hi << 8);
}

static void push8(struct cpu6502 *cpu, uint8_t val)
{
    cpu6502_write(0x0100 + cpu->sp, val);
    cpu->sp--;
}

static uint8_t pop8(struct cpu6502 *cpu)
{
    cpu->sp++;
    return cpu6502_read(0x0100 + cpu->sp);
}

/* --- Addressing mode helpers: return the effective address --- */

static uint16_t addr_zp(struct cpu6502 *cpu)
{
    return fetch8(cpu);
}

static uint16_t addr_zpx(struct cpu6502 *cpu)
{
    return (uint8_t)(fetch8(cpu) + cpu->x);
}

static uint16_t addr_zpy(struct cpu6502 *cpu)
{
    return (uint8_t)(fetch8(cpu) + cpu->y);
}

static uint16_t addr_abs(struct cpu6502 *cpu)
{
    return fetch16(cpu);
}

static uint16_t addr_absx(struct cpu6502 *cpu)
{
    return (uint16_t)(fetch16(cpu) + cpu->x);
}

static uint16_t addr_absy(struct cpu6502 *cpu)
{
    return (uint16_t)(fetch16(cpu) + cpu->y);
}

static uint16_t addr_indx(struct cpu6502 *cpu)
{
    uint8_t zp = (uint8_t)(fetch8(cpu) + cpu->x);
    uint16_t lo = cpu6502_read(zp);
    uint16_t hi = cpu6502_read((uint8_t)(zp + 1));
    return lo | (hi << 8);
}

static uint16_t addr_indy(struct cpu6502 *cpu)
{
    uint8_t zp = fetch8(cpu);
    uint16_t lo = cpu6502_read(zp);
    uint16_t hi = cpu6502_read((uint8_t)(zp + 1));
    uint16_t base = lo | (hi << 8);
    return (uint16_t)(base + cpu->y);
}

/* --- Shared arithmetic/logic helpers --- */

static void do_adc(struct cpu6502 *cpu, uint8_t val)
{
    uint16_t carry = (cpu->p & FLAG_C) ? 1 : 0;
    uint16_t result = (uint16_t)cpu->a + val + carry;

    if (result > 0xFF) cpu->p |= FLAG_C; else cpu->p &= ~FLAG_C;

    /* overflow: sign of result differs from sign of both operands agreeing */
    if (((cpu->a ^ result) & (val ^ result) & 0x80) != 0) cpu->p |= FLAG_V;
    else cpu->p &= ~FLAG_V;

    cpu->a = (uint8_t)result;
    set_zn(cpu, cpu->a);
}

static void do_sbc(struct cpu6502 *cpu, uint8_t val)
{
    /* SBC is ADC with the operand inverted (standard 6502 trick) */
    do_adc(cpu, (uint8_t)(~val));
}

static uint8_t do_asl(struct cpu6502 *cpu, uint8_t val)
{
    if (val & 0x80) cpu->p |= FLAG_C; else cpu->p &= ~FLAG_C;
    val <<= 1;
    set_zn(cpu, val);
    return val;
}

static uint8_t do_lsr(struct cpu6502 *cpu, uint8_t val)
{
    if (val & 0x01) cpu->p |= FLAG_C; else cpu->p &= ~FLAG_C;
    val >>= 1;
    set_zn(cpu, val);
    return val;
}

static uint8_t do_rol(struct cpu6502 *cpu, uint8_t val)
{
    uint8_t carry_in = (cpu->p & FLAG_C) ? 1 : 0;
    if (val & 0x80) cpu->p |= FLAG_C; else cpu->p &= ~FLAG_C;
    val = (uint8_t)((val << 1) | carry_in);
    set_zn(cpu, val);
    return val;
}

static uint8_t do_ror(struct cpu6502 *cpu, uint8_t val)
{
    uint8_t carry_in = (cpu->p & FLAG_C) ? 0x80 : 0;
    if (val & 0x01) cpu->p |= FLAG_C; else cpu->p &= ~FLAG_C;
    val = (uint8_t)((val >> 1) | carry_in);
    set_zn(cpu, val);
    return val;
}

void cpu6502_reset(struct cpu6502 *cpu, uint16_t start_pc)
{
    cpu->a = 0;
    cpu->x = 0;
    cpu->y = 0;
    cpu->sp = 0xFD;
    cpu->pc = start_pc;
    cpu->p = FLAG_U | FLAG_I;
    cpu->cycles = 0;
    cpu->halted = 0;
}

void cpu6502_irq(struct cpu6502 *cpu)
{
    /* Real 6502 IRQ behavior: ignored if I flag is set (interrupts
       disabled) -- this matters a lot early in boot, since SEI runs
       almost immediately and interrupts stay off until KERNAL/BASIC
       init explicitly re-enables them with CLI. */
    if (cpu->p & FLAG_I) return;
    if (cpu->halted) return;

    push8(cpu, (uint8_t)((cpu->pc >> 8) & 0xFF));
    push8(cpu, (uint8_t)(cpu->pc & 0xFF));
    push8(cpu, (uint8_t)((cpu->p & ~FLAG_B) | FLAG_U));

    cpu->p |= FLAG_I;

    uint8_t lo = cpu6502_read(0xFFFE);
    uint8_t hi = cpu6502_read(0xFFFF);
    cpu->pc = (uint16_t)(lo | (hi << 8));

    cpu->cycles += 7;
}

void cpu6502_step(struct cpu6502 *cpu)
{
    if (cpu->halted) return;

    uint8_t opcode = fetch8(cpu);

    switch (opcode) {

    /* ---- LDA ---- */
    case 0xA9: cpu->a = fetch8(cpu); set_zn(cpu, cpu->a); cpu->cycles += 2; break;
    case 0xA5: cpu->a = cpu6502_read(addr_zp(cpu)); set_zn(cpu, cpu->a); cpu->cycles += 3; break;
    case 0xB5: cpu->a = cpu6502_read(addr_zpx(cpu)); set_zn(cpu, cpu->a); cpu->cycles += 4; break;
    case 0xAD: cpu->a = cpu6502_read(addr_abs(cpu)); set_zn(cpu, cpu->a); cpu->cycles += 4; break;
    case 0xBD: cpu->a = cpu6502_read(addr_absx(cpu)); set_zn(cpu, cpu->a); cpu->cycles += 4; break;
    case 0xB9: cpu->a = cpu6502_read(addr_absy(cpu)); set_zn(cpu, cpu->a); cpu->cycles += 4; break;
    case 0xA1: cpu->a = cpu6502_read(addr_indx(cpu)); set_zn(cpu, cpu->a); cpu->cycles += 6; break;
    case 0xB1: cpu->a = cpu6502_read(addr_indy(cpu)); set_zn(cpu, cpu->a); cpu->cycles += 5; break;

    /* ---- STA ---- */
    case 0x85: cpu6502_write(addr_zp(cpu), cpu->a); cpu->cycles += 3; break;
    case 0x95: cpu6502_write(addr_zpx(cpu), cpu->a); cpu->cycles += 4; break;
    case 0x8D: cpu6502_write(addr_abs(cpu), cpu->a); cpu->cycles += 4; break;
    case 0x9D: cpu6502_write(addr_absx(cpu), cpu->a); cpu->cycles += 5; break;
    case 0x99: cpu6502_write(addr_absy(cpu), cpu->a); cpu->cycles += 5; break;
    case 0x81: cpu6502_write(addr_indx(cpu), cpu->a); cpu->cycles += 6; break;
    case 0x91: cpu6502_write(addr_indy(cpu), cpu->a); cpu->cycles += 6; break;

    /* ---- LDX ---- */
    case 0xA2: cpu->x = fetch8(cpu); set_zn(cpu, cpu->x); cpu->cycles += 2; break;
    case 0xA6: cpu->x = cpu6502_read(addr_zp(cpu)); set_zn(cpu, cpu->x); cpu->cycles += 3; break;
    case 0xB6: cpu->x = cpu6502_read(addr_zpy(cpu)); set_zn(cpu, cpu->x); cpu->cycles += 4; break;
    case 0xAE: cpu->x = cpu6502_read(addr_abs(cpu)); set_zn(cpu, cpu->x); cpu->cycles += 4; break;
    case 0xBE: cpu->x = cpu6502_read(addr_absy(cpu)); set_zn(cpu, cpu->x); cpu->cycles += 4; break;

    /* ---- STX ---- */
    case 0x86: cpu6502_write(addr_zp(cpu), cpu->x); cpu->cycles += 3; break;
    case 0x96: cpu6502_write(addr_zpy(cpu), cpu->x); cpu->cycles += 4; break;
    case 0x8E: cpu6502_write(addr_abs(cpu), cpu->x); cpu->cycles += 4; break;

    /* ---- LDY ---- */
    case 0xA0: cpu->y = fetch8(cpu); set_zn(cpu, cpu->y); cpu->cycles += 2; break;
    case 0xA4: cpu->y = cpu6502_read(addr_zp(cpu)); set_zn(cpu, cpu->y); cpu->cycles += 3; break;
    case 0xB4: cpu->y = cpu6502_read(addr_zpx(cpu)); set_zn(cpu, cpu->y); cpu->cycles += 4; break;
    case 0xAC: cpu->y = cpu6502_read(addr_abs(cpu)); set_zn(cpu, cpu->y); cpu->cycles += 4; break;
    case 0xBC: cpu->y = cpu6502_read(addr_absx(cpu)); set_zn(cpu, cpu->y); cpu->cycles += 4; break;

    /* ---- STY ---- */
    case 0x84: cpu6502_write(addr_zp(cpu), cpu->y); cpu->cycles += 3; break;
    case 0x94: cpu6502_write(addr_zpx(cpu), cpu->y); cpu->cycles += 4; break;
    case 0x8C: cpu6502_write(addr_abs(cpu), cpu->y); cpu->cycles += 4; break;

    /* ---- Transfers ---- */
    case 0xAA: cpu->x = cpu->a; set_zn(cpu, cpu->x); cpu->cycles += 2; break; /* TAX */
    case 0x8A: cpu->a = cpu->x; set_zn(cpu, cpu->a); cpu->cycles += 2; break; /* TXA */
    case 0xA8: cpu->y = cpu->a; set_zn(cpu, cpu->y); cpu->cycles += 2; break; /* TAY */
    case 0x98: cpu->a = cpu->y; set_zn(cpu, cpu->a); cpu->cycles += 2; break; /* TYA */
    case 0x9A: cpu->sp = cpu->x; cpu->cycles += 2; break;                    /* TXS (no flags) */
    case 0xBA: cpu->x = cpu->sp; set_zn(cpu, cpu->x); cpu->cycles += 2; break; /* TSX */

    /* ---- Stack ---- */
    case 0x48: push8(cpu, cpu->a); cpu->cycles += 3; break; /* PHA */
    case 0x68: cpu->a = pop8(cpu); set_zn(cpu, cpu->a); cpu->cycles += 4; break; /* PLA */
    case 0x08: push8(cpu, (uint8_t)(cpu->p | FLAG_B | FLAG_U)); cpu->cycles += 3; break; /* PHP */
    case 0x28: cpu->p = (uint8_t)((pop8(cpu) & ~FLAG_B) | FLAG_U); cpu->cycles += 4; break; /* PLP */

    /* ---- Flag ops ---- */
    case 0x78: cpu->p |= FLAG_I; cpu->cycles += 2; break; /* SEI */
    case 0x58: cpu->p &= ~FLAG_I; cpu->cycles += 2; break; /* CLI */
    case 0xF8: cpu->p |= FLAG_D; cpu->cycles += 2; break; /* SED */
    case 0xD8: cpu->p &= ~FLAG_D; cpu->cycles += 2; break; /* CLD */
    case 0x38: cpu->p |= FLAG_C; cpu->cycles += 2; break; /* SEC */
    case 0x18: cpu->p &= ~FLAG_C; cpu->cycles += 2; break; /* CLC */
    case 0xB8: cpu->p &= ~FLAG_V; cpu->cycles += 2; break; /* CLV */

    /* ---- Increment/decrement (registers) ---- */
    case 0xE8: cpu->x++; set_zn(cpu, cpu->x); cpu->cycles += 2; break; /* INX */
    case 0xC8: cpu->y++; set_zn(cpu, cpu->y); cpu->cycles += 2; break; /* INY */
    case 0xCA: cpu->x--; set_zn(cpu, cpu->x); cpu->cycles += 2; break; /* DEX */
    case 0x88: cpu->y--; set_zn(cpu, cpu->y); cpu->cycles += 2; break; /* DEY */

    /* ---- Increment/decrement (memory) ---- */
    case 0xE6: { uint16_t a = addr_zp(cpu); uint8_t v = (uint8_t)(cpu6502_read(a) + 1); cpu6502_write(a, v); set_zn(cpu, v); cpu->cycles += 5; break; } /* INC zp */
    case 0xF6: { uint16_t a = addr_zpx(cpu); uint8_t v = (uint8_t)(cpu6502_read(a) + 1); cpu6502_write(a, v); set_zn(cpu, v); cpu->cycles += 6; break; } /* INC zp,X */
    case 0xEE: { uint16_t a = addr_abs(cpu); uint8_t v = (uint8_t)(cpu6502_read(a) + 1); cpu6502_write(a, v); set_zn(cpu, v); cpu->cycles += 6; break; } /* INC abs */
    case 0xFE: { uint16_t a = addr_absx(cpu); uint8_t v = (uint8_t)(cpu6502_read(a) + 1); cpu6502_write(a, v); set_zn(cpu, v); cpu->cycles += 7; break; } /* INC abs,X */
    case 0xC6: { uint16_t a = addr_zp(cpu); uint8_t v = (uint8_t)(cpu6502_read(a) - 1); cpu6502_write(a, v); set_zn(cpu, v); cpu->cycles += 5; break; } /* DEC zp */
    case 0xD6: { uint16_t a = addr_zpx(cpu); uint8_t v = (uint8_t)(cpu6502_read(a) - 1); cpu6502_write(a, v); set_zn(cpu, v); cpu->cycles += 6; break; } /* DEC zp,X */
    case 0xCE: { uint16_t a = addr_abs(cpu); uint8_t v = (uint8_t)(cpu6502_read(a) - 1); cpu6502_write(a, v); set_zn(cpu, v); cpu->cycles += 6; break; } /* DEC abs */
    case 0xDE: { uint16_t a = addr_absx(cpu); uint8_t v = (uint8_t)(cpu6502_read(a) - 1); cpu6502_write(a, v); set_zn(cpu, v); cpu->cycles += 7; break; } /* DEC abs,X */

    /* ---- Compare ---- */
    case 0xE0: { uint8_t v = fetch8(cpu); uint8_t r = (uint8_t)(cpu->x - v); if (cpu->x >= v) cpu->p |= FLAG_C; else cpu->p &= ~FLAG_C; set_zn(cpu, r); cpu->cycles += 2; break; } /* CPX # */
    case 0xC0: { uint8_t v = fetch8(cpu); uint8_t r = (uint8_t)(cpu->y - v); if (cpu->y >= v) cpu->p |= FLAG_C; else cpu->p &= ~FLAG_C; set_zn(cpu, r); cpu->cycles += 2; break; } /* CPY # */
    case 0xC9: { uint8_t v = fetch8(cpu); uint8_t r = (uint8_t)(cpu->a - v); if (cpu->a >= v) cpu->p |= FLAG_C; else cpu->p &= ~FLAG_C; set_zn(cpu, r); cpu->cycles += 2; break; } /* CMP # */
    case 0xC5: { uint8_t v = cpu6502_read(addr_zp(cpu)); uint8_t r = (uint8_t)(cpu->a - v); if (cpu->a >= v) cpu->p |= FLAG_C; else cpu->p &= ~FLAG_C; set_zn(cpu, r); cpu->cycles += 3; break; } /* CMP zp */
    case 0xCD: { uint8_t v = cpu6502_read(addr_abs(cpu)); uint8_t r = (uint8_t)(cpu->a - v); if (cpu->a >= v) cpu->p |= FLAG_C; else cpu->p &= ~FLAG_C; set_zn(cpu, r); cpu->cycles += 4; break; } /* CMP abs */
    case 0xD5: { uint8_t v = cpu6502_read(addr_zpx(cpu)); uint8_t r = (uint8_t)(cpu->a - v); if (cpu->a >= v) cpu->p |= FLAG_C; else cpu->p &= ~FLAG_C; set_zn(cpu, r); cpu->cycles += 4; break; } /* CMP zp,X */
    case 0xDD: { uint8_t v = cpu6502_read(addr_absx(cpu)); uint8_t r = (uint8_t)(cpu->a - v); if (cpu->a >= v) cpu->p |= FLAG_C; else cpu->p &= ~FLAG_C; set_zn(cpu, r); cpu->cycles += 4; break; } /* CMP abs,X */
    case 0xD9: { uint8_t v = cpu6502_read(addr_absy(cpu)); uint8_t r = (uint8_t)(cpu->a - v); if (cpu->a >= v) cpu->p |= FLAG_C; else cpu->p &= ~FLAG_C; set_zn(cpu, r); cpu->cycles += 4; break; } /* CMP abs,Y */
    case 0xC1: { uint8_t v = cpu6502_read(addr_indx(cpu)); uint8_t r = (uint8_t)(cpu->a - v); if (cpu->a >= v) cpu->p |= FLAG_C; else cpu->p &= ~FLAG_C; set_zn(cpu, r); cpu->cycles += 6; break; } /* CMP (ind,X) */
    case 0xD1: { uint8_t v = cpu6502_read(addr_indy(cpu)); uint8_t r = (uint8_t)(cpu->a - v); if (cpu->a >= v) cpu->p |= FLAG_C; else cpu->p &= ~FLAG_C; set_zn(cpu, r); cpu->cycles += 5; break; } /* CMP (ind),Y */
    case 0xE4: { uint8_t v = cpu6502_read(addr_zp(cpu)); uint8_t r = (uint8_t)(cpu->x - v); if (cpu->x >= v) cpu->p |= FLAG_C; else cpu->p &= ~FLAG_C; set_zn(cpu, r); cpu->cycles += 3; break; } /* CPX zp */
    case 0xEC: { uint8_t v = cpu6502_read(addr_abs(cpu)); uint8_t r = (uint8_t)(cpu->x - v); if (cpu->x >= v) cpu->p |= FLAG_C; else cpu->p &= ~FLAG_C; set_zn(cpu, r); cpu->cycles += 4; break; } /* CPX abs */
    case 0xC4: { uint8_t v = cpu6502_read(addr_zp(cpu)); uint8_t r = (uint8_t)(cpu->y - v); if (cpu->y >= v) cpu->p |= FLAG_C; else cpu->p &= ~FLAG_C; set_zn(cpu, r); cpu->cycles += 3; break; } /* CPY zp */
    case 0xCC: { uint8_t v = cpu6502_read(addr_abs(cpu)); uint8_t r = (uint8_t)(cpu->y - v); if (cpu->y >= v) cpu->p |= FLAG_C; else cpu->p &= ~FLAG_C; set_zn(cpu, r); cpu->cycles += 4; break; } /* CPY abs */
    case 0x35: cpu->a &= cpu6502_read(addr_zpx(cpu)); set_zn(cpu, cpu->a); cpu->cycles += 4; break; /* AND zp,X */
    case 0x3D: cpu->a &= cpu6502_read(addr_absx(cpu)); set_zn(cpu, cpu->a); cpu->cycles += 4; break; /* AND abs,X */
    case 0x39: cpu->a &= cpu6502_read(addr_absy(cpu)); set_zn(cpu, cpu->a); cpu->cycles += 4; break; /* AND abs,Y */
    case 0x21: cpu->a &= cpu6502_read(addr_indx(cpu)); set_zn(cpu, cpu->a); cpu->cycles += 6; break; /* AND (ind,X) */
    case 0x31: cpu->a &= cpu6502_read(addr_indy(cpu)); set_zn(cpu, cpu->a); cpu->cycles += 5; break; /* AND (ind),Y */
    case 0x15: cpu->a |= cpu6502_read(addr_zpx(cpu)); set_zn(cpu, cpu->a); cpu->cycles += 4; break; /* ORA zp,X */
    case 0x1D: cpu->a |= cpu6502_read(addr_absx(cpu)); set_zn(cpu, cpu->a); cpu->cycles += 4; break; /* ORA abs,X */
    case 0x19: cpu->a |= cpu6502_read(addr_absy(cpu)); set_zn(cpu, cpu->a); cpu->cycles += 4; break; /* ORA abs,Y */
    case 0x01: cpu->a |= cpu6502_read(addr_indx(cpu)); set_zn(cpu, cpu->a); cpu->cycles += 6; break; /* ORA (ind,X) */
    case 0x11: cpu->a |= cpu6502_read(addr_indy(cpu)); set_zn(cpu, cpu->a); cpu->cycles += 5; break; /* ORA (ind),Y */
    case 0x55: cpu->a ^= cpu6502_read(addr_zpx(cpu)); set_zn(cpu, cpu->a); cpu->cycles += 4; break; /* EOR zp,X */
    case 0x5D: cpu->a ^= cpu6502_read(addr_absx(cpu)); set_zn(cpu, cpu->a); cpu->cycles += 4; break; /* EOR abs,X */
    case 0x59: cpu->a ^= cpu6502_read(addr_absy(cpu)); set_zn(cpu, cpu->a); cpu->cycles += 4; break; /* EOR abs,Y */
    case 0x41: cpu->a ^= cpu6502_read(addr_indx(cpu)); set_zn(cpu, cpu->a); cpu->cycles += 6; break; /* EOR (ind,X) */
    case 0x51: cpu->a ^= cpu6502_read(addr_indy(cpu)); set_zn(cpu, cpu->a); cpu->cycles += 5; break; /* EOR (ind),Y */
    case 0x75: do_adc(cpu, cpu6502_read(addr_zpx(cpu))); cpu->cycles += 4; break; /* ADC zp,X */
    case 0x7D: do_adc(cpu, cpu6502_read(addr_absx(cpu))); cpu->cycles += 4; break; /* ADC abs,X */
    case 0x79: do_adc(cpu, cpu6502_read(addr_absy(cpu))); cpu->cycles += 4; break; /* ADC abs,Y */
    case 0x61: do_adc(cpu, cpu6502_read(addr_indx(cpu))); cpu->cycles += 6; break; /* ADC (ind,X) */
    case 0x71: do_adc(cpu, cpu6502_read(addr_indy(cpu))); cpu->cycles += 5; break; /* ADC (ind),Y */
    case 0xF5: do_sbc(cpu, cpu6502_read(addr_zpx(cpu))); cpu->cycles += 4; break; /* SBC zp,X */
    case 0xFD: do_sbc(cpu, cpu6502_read(addr_absx(cpu))); cpu->cycles += 4; break; /* SBC abs,X */
    case 0xF9: do_sbc(cpu, cpu6502_read(addr_absy(cpu))); cpu->cycles += 4; break; /* SBC abs,Y */
    case 0xE1: do_sbc(cpu, cpu6502_read(addr_indx(cpu))); cpu->cycles += 6; break; /* SBC (ind,X) */
    case 0xF1: do_sbc(cpu, cpu6502_read(addr_indy(cpu))); cpu->cycles += 5; break; /* SBC (ind),Y */

    /* ---- Logic ---- */
    case 0x29: cpu->a &= fetch8(cpu); set_zn(cpu, cpu->a); cpu->cycles += 2; break; /* AND # */
    case 0x25: cpu->a &= cpu6502_read(addr_zp(cpu)); set_zn(cpu, cpu->a); cpu->cycles += 3; break; /* AND zp */
    case 0x2D: cpu->a &= cpu6502_read(addr_abs(cpu)); set_zn(cpu, cpu->a); cpu->cycles += 4; break; /* AND abs */
    case 0x09: cpu->a |= fetch8(cpu); set_zn(cpu, cpu->a); cpu->cycles += 2; break; /* ORA # */
    case 0x05: cpu->a |= cpu6502_read(addr_zp(cpu)); set_zn(cpu, cpu->a); cpu->cycles += 3; break; /* ORA zp */
    case 0x0D: cpu->a |= cpu6502_read(addr_abs(cpu)); set_zn(cpu, cpu->a); cpu->cycles += 4; break; /* ORA abs */
    case 0x49: cpu->a ^= fetch8(cpu); set_zn(cpu, cpu->a); cpu->cycles += 2; break; /* EOR # */
    case 0x45: cpu->a ^= cpu6502_read(addr_zp(cpu)); set_zn(cpu, cpu->a); cpu->cycles += 3; break; /* EOR zp */
    case 0x4D: cpu->a ^= cpu6502_read(addr_abs(cpu)); set_zn(cpu, cpu->a); cpu->cycles += 4; break; /* EOR abs */

    case 0x24: { uint8_t v = cpu6502_read(addr_zp(cpu)); uint8_t r = (uint8_t)(cpu->a & v);
                 if (r == 0) cpu->p |= FLAG_Z; else cpu->p &= ~FLAG_Z;
                 if (v & 0x40) cpu->p |= FLAG_V; else cpu->p &= ~FLAG_V;
                 if (v & 0x80) cpu->p |= FLAG_N; else cpu->p &= ~FLAG_N;
                 cpu->cycles += 3; break; } /* BIT zp */
    case 0x2C: { uint8_t v = cpu6502_read(addr_abs(cpu)); uint8_t r = (uint8_t)(cpu->a & v);
                 if (r == 0) cpu->p |= FLAG_Z; else cpu->p &= ~FLAG_Z;
                 if (v & 0x40) cpu->p |= FLAG_V; else cpu->p &= ~FLAG_V;
                 if (v & 0x80) cpu->p |= FLAG_N; else cpu->p &= ~FLAG_N;
                 cpu->cycles += 4; break; } /* BIT abs */

    /* ---- Arithmetic ---- */
    case 0x69: do_adc(cpu, fetch8(cpu)); cpu->cycles += 2; break; /* ADC # */
    case 0x65: do_adc(cpu, cpu6502_read(addr_zp(cpu))); cpu->cycles += 3; break; /* ADC zp */
    case 0x6D: do_adc(cpu, cpu6502_read(addr_abs(cpu))); cpu->cycles += 4; break; /* ADC abs */
    case 0xE9: do_sbc(cpu, fetch8(cpu)); cpu->cycles += 2; break; /* SBC # */
    case 0xE5: do_sbc(cpu, cpu6502_read(addr_zp(cpu))); cpu->cycles += 3; break; /* SBC zp */
    case 0xED: do_sbc(cpu, cpu6502_read(addr_abs(cpu))); cpu->cycles += 4; break; /* SBC abs */

    /* ---- Shifts/rotates ---- */
    case 0x0A: cpu->a = do_asl(cpu, cpu->a); cpu->cycles += 2; break; /* ASL A */
    case 0x06: { uint16_t a = addr_zp(cpu); cpu6502_write(a, do_asl(cpu, cpu6502_read(a))); cpu->cycles += 5; break; } /* ASL zp */
    case 0x4A: cpu->a = do_lsr(cpu, cpu->a); cpu->cycles += 2; break; /* LSR A */
    case 0x46: { uint16_t a = addr_zp(cpu); cpu6502_write(a, do_lsr(cpu, cpu6502_read(a))); cpu->cycles += 5; break; } /* LSR zp */
    case 0x2A: cpu->a = do_rol(cpu, cpu->a); cpu->cycles += 2; break; /* ROL A */
    case 0x26: { uint16_t a = addr_zp(cpu); cpu6502_write(a, do_rol(cpu, cpu6502_read(a))); cpu->cycles += 5; break; } /* ROL zp */
    case 0x6A: cpu->a = do_ror(cpu, cpu->a); cpu->cycles += 2; break; /* ROR A */
    case 0x66: { uint16_t a = addr_zp(cpu); cpu6502_write(a, do_ror(cpu, cpu6502_read(a))); cpu->cycles += 5; break; } /* ROR zp */
    case 0x16: { uint16_t a = addr_zpx(cpu); cpu6502_write(a, do_asl(cpu, cpu6502_read(a))); cpu->cycles += 6; break; } /* ASL zp,X */
    case 0x0E: { uint16_t a = addr_abs(cpu); cpu6502_write(a, do_asl(cpu, cpu6502_read(a))); cpu->cycles += 6; break; } /* ASL abs */
    case 0x1E: { uint16_t a = addr_absx(cpu); cpu6502_write(a, do_asl(cpu, cpu6502_read(a))); cpu->cycles += 7; break; } /* ASL abs,X */
    case 0x56: { uint16_t a = addr_zpx(cpu); cpu6502_write(a, do_lsr(cpu, cpu6502_read(a))); cpu->cycles += 6; break; } /* LSR zp,X */
    case 0x4E: { uint16_t a = addr_abs(cpu); cpu6502_write(a, do_lsr(cpu, cpu6502_read(a))); cpu->cycles += 6; break; } /* LSR abs */
    case 0x5E: { uint16_t a = addr_absx(cpu); cpu6502_write(a, do_lsr(cpu, cpu6502_read(a))); cpu->cycles += 7; break; } /* LSR abs,X */
    case 0x36: { uint16_t a = addr_zpx(cpu); cpu6502_write(a, do_rol(cpu, cpu6502_read(a))); cpu->cycles += 6; break; } /* ROL zp,X */
    case 0x2E: { uint16_t a = addr_abs(cpu); cpu6502_write(a, do_rol(cpu, cpu6502_read(a))); cpu->cycles += 6; break; } /* ROL abs */
    case 0x3E: { uint16_t a = addr_absx(cpu); cpu6502_write(a, do_rol(cpu, cpu6502_read(a))); cpu->cycles += 7; break; } /* ROL abs,X */
    case 0x76: { uint16_t a = addr_zpx(cpu); cpu6502_write(a, do_ror(cpu, cpu6502_read(a))); cpu->cycles += 6; break; } /* ROR zp,X */
    case 0x6E: { uint16_t a = addr_abs(cpu); cpu6502_write(a, do_ror(cpu, cpu6502_read(a))); cpu->cycles += 6; break; } /* ROR abs */
    case 0x7E: { uint16_t a = addr_absx(cpu); cpu6502_write(a, do_ror(cpu, cpu6502_read(a))); cpu->cycles += 7; break; } /* ROR abs,X */

    /* ---- Branches ---- */
    case 0xD0: { int8_t off = (int8_t)fetch8(cpu); cpu->cycles += 2; if (!(cpu->p & FLAG_Z)) { cpu->pc = (uint16_t)(cpu->pc + off); cpu->cycles += 1; } break; } /* BNE */
    case 0xF0: { int8_t off = (int8_t)fetch8(cpu); cpu->cycles += 2; if (cpu->p & FLAG_Z) { cpu->pc = (uint16_t)(cpu->pc + off); cpu->cycles += 1; } break; } /* BEQ */
    case 0x10: { int8_t off = (int8_t)fetch8(cpu); cpu->cycles += 2; if (!(cpu->p & FLAG_N)) { cpu->pc = (uint16_t)(cpu->pc + off); cpu->cycles += 1; } break; } /* BPL */
    case 0x30: { int8_t off = (int8_t)fetch8(cpu); cpu->cycles += 2; if (cpu->p & FLAG_N) { cpu->pc = (uint16_t)(cpu->pc + off); cpu->cycles += 1; } break; } /* BMI */
    case 0x50: { int8_t off = (int8_t)fetch8(cpu); cpu->cycles += 2; if (!(cpu->p & FLAG_V)) { cpu->pc = (uint16_t)(cpu->pc + off); cpu->cycles += 1; } break; } /* BVC */
    case 0x70: { int8_t off = (int8_t)fetch8(cpu); cpu->cycles += 2; if (cpu->p & FLAG_V) { cpu->pc = (uint16_t)(cpu->pc + off); cpu->cycles += 1; } break; } /* BVS */
    case 0x90: { int8_t off = (int8_t)fetch8(cpu); cpu->cycles += 2; if (!(cpu->p & FLAG_C)) { cpu->pc = (uint16_t)(cpu->pc + off); cpu->cycles += 1; } break; } /* BCC */
    case 0xB0: { int8_t off = (int8_t)fetch8(cpu); cpu->cycles += 2; if (cpu->p & FLAG_C) { cpu->pc = (uint16_t)(cpu->pc + off); cpu->cycles += 1; } break; } /* BCS */

    /* ---- Jumps/calls ---- */
    case 0x4C: cpu->pc = addr_abs(cpu); cpu->cycles += 3; break; /* JMP abs */
    case 0x6C: { uint16_t ptr = fetch16(cpu);
                 /* NOTE: real 6502 has a page-boundary bug here (doesn't
                    carry into high byte); not reproduced yet -- flag for
                    later if a real program depends on the buggy behavior */
                 uint16_t lo = cpu6502_read(ptr);
                 uint16_t hi = cpu6502_read((uint16_t)(ptr + 1));
                 cpu->pc = (uint16_t)(lo | (hi << 8));
                 cpu->cycles += 5; break; } /* JMP (ind) */
    case 0x20: { uint16_t addr = fetch16(cpu); uint16_t ret = (uint16_t)(cpu->pc - 1);
                 push8(cpu, (uint8_t)((ret >> 8) & 0xFF));
                 push8(cpu, (uint8_t)(ret & 0xFF));
                 cpu->pc = addr; cpu->cycles += 6; break; } /* JSR */
    case 0x60: { uint16_t lo = pop8(cpu); uint16_t hi = pop8(cpu);
                 cpu->pc = (uint16_t)((lo | (hi << 8)) + 1); cpu->cycles += 6; break; } /* RTS */
    case 0x40: { cpu->p = (uint8_t)((pop8(cpu) & ~FLAG_B) | FLAG_U);
                 uint16_t lo = pop8(cpu); uint16_t hi = pop8(cpu);
                 cpu->pc = (uint16_t)(lo | (hi << 8)); cpu->cycles += 6; break; } /* RTI */

    /* ---- Misc ---- */
    case 0xEA: cpu->cycles += 2; break; /* NOP */
    case 0x00: cpu->halted = 1; cpu->cycles += 7; break; /* BRK */

    default:
        cpu->halted = 1;
        break;
    }
}
