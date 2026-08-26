# CDos-x86

A bare-metal x86 hobby OS built from scratch, blending **C**64 + **DOS** + **x86**.


- A **C64 subsystem**: a from-scratch 6502 CPU interpreter running real,
  legally clean KERNAL/BASIC ROM images (MEGA65 Open ROMs -- see
  `THIRD_PARTY_LICENSES.md`), loading `.PRG`/`.T64` files
- A **DOS subsystem** (planned): `.COM` file execution via a software x86
  real-mode interpreter (libx86emu), with `INT 21h` calls trapped in software

No copyrighted Commodore or Microsoft ROM/DOS binaries are used anywhere in
this project.



## Status

- [x] Phase 1 -- Toolchain + boot
- [x] Phase 2 -- GDT, IDT, PIC, keyboard
- [x] Phase 3 -- Console + shell (`HELP`, `PEEK`/`POKE`/`DUMP`, `CLEAR`)
- [x] Phase 4 -- Disk driver + custom filesystem
- [x] Phase 5 (in progress) -- C64 subsystem: full legal 6502 instruction
      set implemented, real KERNAL/BASIC boot to a working `READY.` prompt
      confirmed. `.PRG`/`.T64` loaders and illegal-opcode support still to do.
- [ ] Phase 6 -- DOS subsystem

## Building

For Phase 6 (DOS subsystem), also fetch libx86emu:
````bash
git clone https://github.com/wfeldt/libx86emu vendor/libx86emu
```

Requires WSL2 (Ubuntu) or native Linux, with `build-essential`, `bison`,
`flex`, `libgmp3-dev`, `libmpc-dev`, `libmpfr-dev`, `texinfo`, `libisl-dev`,
`nasm`, `xorriso`, `grub-pc-bin`, `grub-common`, `mtools`, `qemu-system-x86`.

```bash
# 1. Build the i686-elf cross-compiler (one-time, see toolchain build steps)
# 2. Fetch the C64 ROM images:
mkdir -p roms
curl -sL -o roms/kernal_generic.rom \
  https://raw.githubusercontent.com/MEGA65/open-roms/master/bin/kernal_generic.rom
curl -sL -o roms/basic_generic.rom \
  https://raw.githubusercontent.com/MEGA65/open-roms/master/bin/basic_generic.rom
curl -sL -o roms/chargen_openroms.rom \
  https://raw.githubusercontent.com/MEGA65/open-roms/master/bin/chargen_openroms.rom

# 3. Build the kernel
cd kernel
# (see build commands in project notes / commit history)

# 4. Build the disk image
cp ../roms/*.rom ../diskfiles/
python3 ../tools/build_disk.py ../diskfiles ../disk/disk.img 16

# 5. Boot in QEMU
qemu-system-i386 -cdrom cdos-x86.iso -boot d \
  -drive file=disk/disk.img,format=raw,if=ide,index=0,media=disk
```

## Shell commands

`HELP`, `CLEAR`, `PEEK`/`POKE <addr> <val>`/`DUMP <addr> <len>` (decimal,
`0x` hex, or `$` hex), `DIR`, `TYPE <file>`, `DISKTEST`, `CPUTEST`,
`C64BOOT`, `C64KEY <char>`, `C64ENTER`, `C64RESUME`, `C64SCREEN`,
`C64MEM <addr> <len>`, `C64TRACE <n>`.
