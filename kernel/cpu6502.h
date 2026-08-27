#ifndef CPU6502_H
#define CPU6502_H
#include <stdint.h>

/* Status flag bits (P register) */
#define FLAG_C 0x01  /* Carry */
#define FLAG_Z 0x02  /* Zero */
#define FLAG_I 0x04  /* Interrupt disable */
#define FLAG_D 0x08  /* Decimal mode */
#define FLAG_B 0x10  /* Break */
#define FLAG_U 0x20  /* Unused, always 1 */
#define FLAG_V 0x40  /* Overflow */
#define FLAG_N 0x80  /* Negative */

struct cpu6502 {
    uint8_t  a;
    uint8_t  x;
    uint8_t  y;
    uint8_t  sp;
    uint16_t pc;
    uint8_t  p;      /* status flags */
    uint64_t cycles; /* running cycle count, useful for later timing work */
    int      halted; /* set by BRK for now, until we have real IRQ/BRK handling */
};

void cpu6502_reset(struct cpu6502 *cpu, uint16_t start_pc);
void cpu6502_step(struct cpu6502 *cpu);
void cpu6502_irq(struct cpu6502 *cpu);

/* Circular trace buffer -- records the last N (PC, opcode) pairs
   executed, so a halt can be diagnosed after the fact without needing
   to re-run with step-by-step single-stepping. Widened from 32 to 128
   to capture longer call chains (e.g. through FRMEVL operator dispatch
   and FCOMP) before they scroll out of the ring buffer. */
#define CPU6502_TRACE_LEN 64
extern uint16_t cpu6502_trace_pc[CPU6502_TRACE_LEN];
extern uint8_t  cpu6502_trace_op[CPU6502_TRACE_LEN];
extern int      cpu6502_trace_idx;
void cpu6502_trace_dump_ordered(uint16_t *pc_out, uint8_t *op_out, int count);

extern uint32_t cpu6502_debug_hit_e3e0;
extern uint32_t cpu6502_debug_sp_at_a06e;

/* Memory access -- backed by a flat reserved RAM region for now.
   Later this is where ROM-mapping and I/O register mapping will hook in. */
uint8_t  cpu6502_read(uint16_t addr);
void     cpu6502_write(uint16_t addr, uint8_t val);

#endif
