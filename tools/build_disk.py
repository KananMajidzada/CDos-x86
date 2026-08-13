#!/usr/bin/env python3
"""
CDos-x86 disk image builder.
Scans a diskfiles/ folder and builds a raw disk image with our custom
filesystem: sector 0 = directory table, file data follows contiguously.
"""

import os
import sys
import struct

FS_MAGIC = 0x43444653  # "CDFS" little-endian
FS_MAX_ENTRIES = 21
FS_NAME_LEN = 16
SECTOR_SIZE = 512

def build_disk(diskfiles_dir, output_path, disk_size_mb=16):
    files = sorted(os.listdir(diskfiles_dir))
    files = [f for f in files if os.path.isfile(os.path.join(diskfiles_dir, f))]

    if len(files) > FS_MAX_ENTRIES:
        print(f"ERROR: {len(files)} files found, but max is {FS_MAX_ENTRIES}")
        sys.exit(1)

    entries = []
    current_lba = 1  # sector 0 is reserved for the directory table

    for fname in files:
        if len(fname) > FS_NAME_LEN - 1:
            print(f"ERROR: filename '{fname}' exceeds {FS_NAME_LEN - 1} chars")
            sys.exit(1)

        fpath = os.path.join(diskfiles_dir, fname)
        fsize = os.path.getsize(fpath)
        sectors_needed = (fsize + SECTOR_SIZE - 1) // SECTOR_SIZE

        entries.append({
            'name': fname,
            'start_lba': current_lba,
            'length': fsize,
            'path': fpath,
        })

        print(f"  {fname:16s}  LBA={current_lba:6d}  size={fsize:8d} bytes  ({sectors_needed} sectors)")
        current_lba += sectors_needed

    total_sectors_needed = current_lba
    disk_size_bytes = disk_size_mb * 1024 * 1024
    disk_size_sectors = disk_size_bytes // SECTOR_SIZE

    if total_sectors_needed > disk_size_sectors:
        print(f"ERROR: files need {total_sectors_needed} sectors, disk only has {disk_size_sectors}")
        sys.exit(1)

    dir_sector = bytearray(SECTOR_SIZE)
    struct.pack_into('<II', dir_sector, 0, FS_MAGIC, len(entries))

    offset = 8
    for e in entries:
        name_bytes = e['name'].encode('ascii')[:FS_NAME_LEN]
        name_bytes = name_bytes.ljust(FS_NAME_LEN, b'\x00')
        struct.pack_into(f'<{FS_NAME_LEN}sII', dir_sector, offset,
                          name_bytes, e['start_lba'], e['length'])
        offset += FS_NAME_LEN + 4 + 4

    with open(output_path, 'wb') as out:
        out.write(dir_sector)

        for e in entries:
            with open(e['path'], 'rb') as f:
                data = f.read()
            out.write(data)
            pad = (-len(data)) % SECTOR_SIZE
            out.write(b'\x00' * pad)

        current_size = out.tell()
        remaining = disk_size_bytes - current_size
        if remaining > 0:
            out.write(b'\x00' * remaining)

    print(f"\nDisk image written: {output_path}")
    print(f"  {len(entries)} files, {current_lba} sectors used of {disk_size_sectors} total")


if __name__ == '__main__':
    if len(sys.argv) < 3:
        print(f"Usage: {sys.argv[0]} <diskfiles_dir> <output.img> [size_mb]")
        sys.exit(1)

    diskfiles_dir = sys.argv[1]
    output_path = sys.argv[2]
    size_mb = int(sys.argv[3]) if len(sys.argv) > 3 else 16

    build_disk(diskfiles_dir, output_path, size_mb)
