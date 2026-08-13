#include "isr.h"
#include "idt.h"

#define IDT_FLAGS_INT_GATE 0x8E  /* present, ring 0, 32-bit interrupt gate */

extern void isr0(void);
extern void isr1(void);
extern void isr2(void);
extern void isr3(void);
extern void isr4(void);
extern void isr5(void);
extern void isr6(void);
extern void isr7(void);
extern void isr8(void);
extern void isr9(void);
extern void isr10(void);
extern void isr11(void);
extern void isr12(void);
extern void isr13(void);
extern void isr14(void);
extern void isr15(void);
extern void isr16(void);
extern void isr17(void);
extern void isr18(void);
extern void isr19(void);
extern void isr20(void);
extern void isr21(void);
extern void isr22(void);
extern void isr23(void);
extern void isr24(void);
extern void isr25(void);
extern void isr26(void);
extern void isr27(void);
extern void isr28(void);
extern void isr29(void);
extern void isr30(void);
extern void isr31(void);

void isr_install(void)
{
    idt_set_gate(0, (uint32_t)isr0, 0x08, IDT_FLAGS_INT_GATE);
    idt_set_gate(1, (uint32_t)isr1, 0x08, IDT_FLAGS_INT_GATE);
    idt_set_gate(2, (uint32_t)isr2, 0x08, IDT_FLAGS_INT_GATE);
    idt_set_gate(3, (uint32_t)isr3, 0x08, IDT_FLAGS_INT_GATE);
    idt_set_gate(4, (uint32_t)isr4, 0x08, IDT_FLAGS_INT_GATE);
    idt_set_gate(5, (uint32_t)isr5, 0x08, IDT_FLAGS_INT_GATE);
    idt_set_gate(6, (uint32_t)isr6, 0x08, IDT_FLAGS_INT_GATE);
    idt_set_gate(7, (uint32_t)isr7, 0x08, IDT_FLAGS_INT_GATE);
    idt_set_gate(8, (uint32_t)isr8, 0x08, IDT_FLAGS_INT_GATE);
    idt_set_gate(9, (uint32_t)isr9, 0x08, IDT_FLAGS_INT_GATE);
    idt_set_gate(10, (uint32_t)isr10, 0x08, IDT_FLAGS_INT_GATE);
    idt_set_gate(11, (uint32_t)isr11, 0x08, IDT_FLAGS_INT_GATE);
    idt_set_gate(12, (uint32_t)isr12, 0x08, IDT_FLAGS_INT_GATE);
    idt_set_gate(13, (uint32_t)isr13, 0x08, IDT_FLAGS_INT_GATE);
    idt_set_gate(14, (uint32_t)isr14, 0x08, IDT_FLAGS_INT_GATE);
    idt_set_gate(15, (uint32_t)isr15, 0x08, IDT_FLAGS_INT_GATE);
    idt_set_gate(16, (uint32_t)isr16, 0x08, IDT_FLAGS_INT_GATE);
    idt_set_gate(17, (uint32_t)isr17, 0x08, IDT_FLAGS_INT_GATE);
    idt_set_gate(18, (uint32_t)isr18, 0x08, IDT_FLAGS_INT_GATE);
    idt_set_gate(19, (uint32_t)isr19, 0x08, IDT_FLAGS_INT_GATE);
    idt_set_gate(20, (uint32_t)isr20, 0x08, IDT_FLAGS_INT_GATE);
    idt_set_gate(21, (uint32_t)isr21, 0x08, IDT_FLAGS_INT_GATE);
    idt_set_gate(22, (uint32_t)isr22, 0x08, IDT_FLAGS_INT_GATE);
    idt_set_gate(23, (uint32_t)isr23, 0x08, IDT_FLAGS_INT_GATE);
    idt_set_gate(24, (uint32_t)isr24, 0x08, IDT_FLAGS_INT_GATE);
    idt_set_gate(25, (uint32_t)isr25, 0x08, IDT_FLAGS_INT_GATE);
    idt_set_gate(26, (uint32_t)isr26, 0x08, IDT_FLAGS_INT_GATE);
    idt_set_gate(27, (uint32_t)isr27, 0x08, IDT_FLAGS_INT_GATE);
    idt_set_gate(28, (uint32_t)isr28, 0x08, IDT_FLAGS_INT_GATE);
    idt_set_gate(29, (uint32_t)isr29, 0x08, IDT_FLAGS_INT_GATE);
    idt_set_gate(30, (uint32_t)isr30, 0x08, IDT_FLAGS_INT_GATE);
    idt_set_gate(31, (uint32_t)isr31, 0x08, IDT_FLAGS_INT_GATE);
}

struct registers {
    uint32_t ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
};

static const char *exception_messages[] = {
    "Division By Zero", "Debug", "Non Maskable Interrupt", "Breakpoint",
    "Into Detected Overflow", "Out of Bounds", "Invalid Opcode",
    "No Coprocessor", "Double Fault", "Coprocessor Segment Overrun",
    "Bad TSS", "Segment Not Present", "Stack Fault", "General Protection Fault",
    "Page Fault", "Unknown Interrupt", "Coprocessor Fault", "Alignment Check",
    "Machine Check", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved",
    "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved",
    "Reserved", "Reserved"
};

#define VGA_MEM ((uint16_t*)0xB8000)
#define VGA_RED_ON_BLACK 0x0400

static void panic_print(const char *msg)
{
    uint16_t *vga = VGA_MEM;
    int i;
    for (i = 0; msg[i] != '\0'; i++) {
        vga[80 + i] = VGA_RED_ON_BLACK | (uint8_t)msg[i]; /* row 1 */
    }
}

void isr_handler(struct registers regs)
{
    panic_print("KERNEL PANIC: ");
    if (regs.int_no < 32) {
        panic_print(exception_messages[regs.int_no]);
    }
    for (;;) {
        __asm__ volatile ("hlt");
    }
}
