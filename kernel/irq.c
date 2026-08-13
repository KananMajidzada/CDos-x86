#include "irq.h"
#include "idt.h"
#include "pic.h"

#define IDT_FLAGS_INT_GATE 0x8E

extern void irq0(void);
extern void irq1(void);
extern void irq2(void);
extern void irq3(void);
extern void irq4(void);
extern void irq5(void);
extern void irq6(void);
extern void irq7(void);
extern void irq8(void);
extern void irq9(void);
extern void irq10(void);
extern void irq11(void);
extern void irq12(void);
extern void irq13(void);
extern void irq14(void);
extern void irq15(void);

static irq_handler_t irq_routines[16] = { 0 };

void irq_install_handler(int irq, irq_handler_t handler)
{
    irq_routines[irq] = handler;
}

void irq_install(void)
{
    idt_set_gate(32, (uint32_t)irq0, 0x08, IDT_FLAGS_INT_GATE);
    idt_set_gate(33, (uint32_t)irq1, 0x08, IDT_FLAGS_INT_GATE);
    idt_set_gate(34, (uint32_t)irq2, 0x08, IDT_FLAGS_INT_GATE);
    idt_set_gate(35, (uint32_t)irq3, 0x08, IDT_FLAGS_INT_GATE);
    idt_set_gate(36, (uint32_t)irq4, 0x08, IDT_FLAGS_INT_GATE);
    idt_set_gate(37, (uint32_t)irq5, 0x08, IDT_FLAGS_INT_GATE);
    idt_set_gate(38, (uint32_t)irq6, 0x08, IDT_FLAGS_INT_GATE);
    idt_set_gate(39, (uint32_t)irq7, 0x08, IDT_FLAGS_INT_GATE);
    idt_set_gate(40, (uint32_t)irq8, 0x08, IDT_FLAGS_INT_GATE);
    idt_set_gate(41, (uint32_t)irq9, 0x08, IDT_FLAGS_INT_GATE);
    idt_set_gate(42, (uint32_t)irq10, 0x08, IDT_FLAGS_INT_GATE);
    idt_set_gate(43, (uint32_t)irq11, 0x08, IDT_FLAGS_INT_GATE);
    idt_set_gate(44, (uint32_t)irq12, 0x08, IDT_FLAGS_INT_GATE);
    idt_set_gate(45, (uint32_t)irq13, 0x08, IDT_FLAGS_INT_GATE);
    idt_set_gate(46, (uint32_t)irq14, 0x08, IDT_FLAGS_INT_GATE);
    idt_set_gate(47, (uint32_t)irq15, 0x08, IDT_FLAGS_INT_GATE);
}

struct irq_registers {
    uint32_t ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
};

void irq_handler(struct irq_registers regs)
{
    int irq_no = regs.int_no - 32;

    if (irq_routines[irq_no] != 0) {
        irq_routines[irq_no]();
    }

    pic_send_eoi((uint8_t)irq_no);
}
