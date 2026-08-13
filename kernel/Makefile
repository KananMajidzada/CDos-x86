CC = i686-elf-gcc
AS = nasm
CFLAGS = -std=gnu99 -ffreestanding -O2 -Wall -Wextra -I.

ASM_OBJS = boot.o gdt_flush.o idt_flush.o isr_asm.o
C_OBJS = kernel.o gdt.o idt.o isr_c.o pic.o irq.o \
         keyboard.o console.o shell.o string.o ata.o fs.o \
         cpu6502.o cpu6502_mem.o cpu6502_test.o c64.o

OBJS = $(ASM_OBJS) $(C_OBJS)

all: cdos-x86.iso

boot.o: boot.asm
	$(AS) -f elf32 boot.asm -o boot.o

gdt_flush.o: gdt_flush.asm
	$(AS) -f elf32 gdt_flush.asm -o gdt_flush.o

idt_flush.o: idt_flush.asm
	$(AS) -f elf32 idt_flush.asm -o idt_flush.o

isr_asm.o: isr.asm
	$(AS) -f elf32 isr.asm -o isr_asm.o

isr_c.o: isr.c isr.h idt.h
	$(CC) $(CFLAGS) -c isr.c -o isr_c.o

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

cdos-x86.bin: $(OBJS)
	$(CC) -T linker.ld -o cdos-x86.bin -ffreestanding -O2 -nostdlib $(OBJS) -lgcc

cdos-x86.iso: cdos-x86.bin
	mkdir -p ../iso/boot/grub
	cp cdos-x86.bin ../iso/boot/cdos-x86.bin
	echo 'menuentry "CDos-x86" {' > ../iso/boot/grub/grub.cfg
	echo '    multiboot /boot/cdos-x86.bin' >> ../iso/boot/grub/grub.cfg
	echo '    boot' >> ../iso/boot/grub/grub.cfg
	echo '}' >> ../iso/boot/grub/grub.cfg
	grub-mkrescue -o ../cdos-x86.iso ../iso

run: cdos-x86.iso
	qemu-system-i386 -cdrom ../cdos-x86.iso -boot d \
	  -drive file=../disk/disk.img,format=raw,if=ide,index=0,media=disk

clean:
	rm -f $(OBJS) cdos-x86.bin ../cdos-x86.iso

.PHONY: all run clean
