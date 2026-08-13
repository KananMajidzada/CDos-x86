# Third-Party Components

CDos-x86 itself is original, clean-room code (see LICENSE). It does, however,
load real ROM images at runtime for the C64 subsystem's KERNAL/BASIC
compatibility layer. These are NOT part of CDos-x86's own codebase and carry
their own license terms:

## MEGA65 Open ROMs

- **Source:** https://github.com/MEGA65/open-roms
- **License:** LGPL v3 (core), MIT (portions of BASIC, derived from
  Microsoft's publicly released original MS-BASIC source)
- **What's used:** `kernal_generic.rom`, `basic_generic.rom`,
  `chargen_openroms.rom`
- **Why:** A legally clean, from-scratch reimplementation of C64 KERNAL/BASIC
  behavior -- NOT a disassembly or dump of original Commodore ROM code. Full
  license text is included in the upstream repository.

These ROM images are not committed to this repository. Run the fetch commands
in the README to download them directly from the authoritative source before
building.
