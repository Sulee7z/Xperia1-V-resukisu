#!/usr/bin/env python3
"""Build a Sony-compatible DTBO image from per-overlay DTB files.

Android DTBO image format (v0):
  header(32): magic(4) total_size(4) header_size(4) dt_entry_size(4)
              dt_entry_count(4) dt_entries_offset(4) page_size(4) version(4)
  entries(32 each): dt_size(4) dt_offset(4) id(4) rev(4) custom(16)
  each DTB is page-aligned (4096).

Usage:
  python3 build_dtbo.py <dtb_dir> <output.img>
  - dtb_dir: directory containing overlay_XX.dtb files (sorted by name)
"""
import struct
import os
import sys


def build_dtbo(src_dir: str, out_path: str, page_size: int = 4096) -> None:
    files = sorted(f for f in os.listdir(src_dir) if f.endswith(".dtb"))
    if not files:
        print("no .dtb files in", src_dir)
        sys.exit(1)

    dtbs = [(f, open(os.path.join(src_dir, f), "rb").read()) for f in files]
    print(f"packing {len(dtbs)} overlays: {', '.join(f for f, _ in dtbs)}")

    header_size = 32
    dt_entry_size = 32
    dt_entry_count = len(dtbs)
    dt_entries_offset = header_size

    total = dt_entries_offset + dt_entry_count * dt_entry_size
    offsets = []
    for _, data in dtbs:
        padded = (len(data) + page_size - 1) // page_size * page_size
        offsets.append((total, len(data)))
        total += padded

    with open(out_path, "wb") as out:
        out.write(struct.pack(">IIIIIIII", 0xD7B7AB1E, total, header_size,
                              dt_entry_size, dt_entry_count, dt_entries_offset,
                              page_size, 0))
        for i, (f, data) in enumerate(dtbs):
            off, size = offsets[i]
            out.write(struct.pack(">IIII", size, off, i, 0) + b"\x00" * 16)
        for (f, data), (off, _) in zip(dtbs, offsets):
            out.seek(off)
            out.write(data)
        out.seek(total - 1)
        out.write(b"\x00")

    print(f"wrote {out_path} ({total} bytes)")


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print(__doc__)
        sys.exit(2)
    build_dtbo(sys.argv[1], sys.argv[2])
