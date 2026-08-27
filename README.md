# CDos-x86

A bare-metal x86 hobby operating system that blends **C**64 emulation, **DOS** compatibility, and native **x86** development into a single project.

## Overview

CDos-x86 combines two subsystems under one kernel:

- **C64 subsystem** — a from-scratch 6502 CPU interpreter running real, legally clean KERNAL/BASIC ROM images (MEGA65 Open ROMs; see `THIRD_PARTY_LICENSES.md`), with support for loading `.PRG`/`.T64` files.
- **DOS subsystem** *(planned)* — `.COM` file execution via a software x86 real-mode interpreter ([libx86emu](https://github.com/wfeldt/libx86emu)), with `INT 21h` calls trapped and handled in software.

No copyrighted Commodore or Microsoft ROM/DOS binaries are used anywhere in this project.

## Project Status

| Phase | Description | Status |
|-------|-------------|--------|
| 1 | Toolchain + boot |  Complete |
| 2 | GDT, IDT, PIC, keyboard |  Complete |
| 3 | Console + shell (`HELP`, `PEEK`/`POKE`/`DUMP`, `CLEAR`) |  Complete |
| 4 | Disk driver + custom filesystem |  Complete |
| 5 | C64 subsystem |  In progress |
| 6 | DOS subsystem |  Not started |

**Phase 5 detail:** the full legal 6502 instruction set is implemented, and the real KERNAL/BASIC ROMs boot to a working `READY.` prompt. Remaining work: `.PRG`/`.T64` loaders and illegal-opcode support.

## Requirements

- WSL2 (Ubuntu) or native Linux
- `build-essential`, `bison`, `flex`, `libgmp3-dev`, `libmpc-dev`, `libmpfr-dev`, `texinfo`, `libisl-dev`
- `nasm`, `xorriso`, `grub-pc-bin`, `grub-common`, `mtools`, `qemu-system-x86`

For Phase 6 (DOS subsystem), also fetch libx86emu:

```bash
git clone https://github.com/wfeldt/libx86emu vendor/libx86emu
```

## Building

**1. Build the i686-elf cross-compiler** *(one-time; see toolchain build steps)*

**2. Fetch the C64 ROM images:**

```bash
mkdir -p roms
curl -sL -o roms/kernal_generic.rom \
  https://raw.githubusercontent.com/MEGA65/open-roms/master/bin/kernal_generic.rom
curl -sL -o roms/basic_generic.rom \
  https://raw.githubusercontent.com/MEGA65/open-roms/master/bin/basic_generic.rom
curl -sL -o roms/chargen_openroms.rom \
  https://raw.githubusercontent.com/MEGA65/open-roms/master/bin/chargen_openroms.rom
```

**3. Build the kernel:**

```bash
cd kernel
# (see build commands in project notes / commit history)
```

**4. Build the disk image:**

The custom filesystem limits filenames to 15 characters, so the ROMs must be
copied in with shortened names (not their original MEGA65 filenames):

```bash
cp ../roms/kernal_generic.rom ../diskfiles/KERNAL.ROM
cp ../roms/basic_generic.rom ../diskfiles/BASIC.ROM
cp ../roms/chargen_openroms.rom ../diskfiles/CHARGEN.ROM
python3 ../tools/build_disk.py ../diskfiles ../disk/disk.img 16
```

**5. Boot in QEMU:**

```bash
qemu-system-i386 -cdrom cdos-x86.iso -boot d \
  -drive file=disk/disk.img,format=raw,if=ide,index=0,media=disk
```

## Shell Commands

| Command | Description |
|---------|-------------|
| `HELP` | List available commands |
| `CLEAR` | Clear the screen |
| `PEEK <addr>` | Read a byte from memory |
| `POKE <addr> <val>` | Write a byte to memory |
| `DUMP <addr> <len>` | Dump a memory region |
| `DIR` | List directory contents |
| `TYPE <file>` | Print a file's contents |
| `DISKTEST` | Run disk driver diagnostics |
| `CPUTEST` | Run CPU diagnostics |
| `C64BOOT` | Boot into the C64 subsystem |
| `C64KEY <char>` | Send a keypress to the C64 subsystem |
| `C64ENTER` | Send Enter to the C64 subsystem |
| `C64RESUME` | Resume C64 execution |
| `C64SCREEN` | Display the C64 screen buffer |
| `C64MEM <addr> <len>` | Dump C64 memory |
| `C64TRACE <n>` | Trace `n` C64 instructions |

Numeric addresses accept decimal, `0x` hex, or `$` hex notation.

## License

No copyrighted Commodore or Microsoft ROM/DOS binaries are included. See `THIRD_PARTY_LICENSES.md` for details on the MEGA65 Open ROMs used by the C64 subsystem.
