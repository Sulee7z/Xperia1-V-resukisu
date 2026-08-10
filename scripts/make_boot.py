#!/usr/bin/env python3
"""Package a raw kernel Image into an Android boot header v4 image."""
import struct
import sys


def main():
    if len(sys.argv) != 3:
        print("usage: make_boot.py <Image> <out.img>")
        sys.exit(1)

    img, out = sys.argv[1], sys.argv[2]
    page = 4096

    with open(img, "rb") as f:
        kernel = f.read()

    hdr = bytearray(1584)
    hdr[0:8] = b"ANDROID!"
    struct.pack_into("<I", hdr, 8, len(kernel))    # kernel_size
    struct.pack_into("<I", hdr, 16, 0)             # ramdisk_size
    struct.pack_into("<I", hdr, 20, 0)             # os_version
    struct.pack_into("<I", hdr, 40, 4)             # header_version = 4

    def pad_to(n, size):
        return ((size + n - 1) // n) * n

    boot = bytearray()
    boot += hdr
    boot += b"\x00" * (page - len(hdr))
    boot += kernel
    boot += b"\x00" * (pad_to(page, len(kernel)) - len(kernel))

    with open(out, "wb") as f:
        f.write(boot)

    with open(out, "rb") as f:
        v = f.read(44)
    print("boot: %s (%d bytes)" % (out, len(boot)))
    print("magic=%s kernel_size=%d hdr_ver=%d" % (
        v[0:8].decode("latin1"),
        struct.unpack_from("<I", v, 8)[0],
        struct.unpack_from("<I", v, 40)[0],
    ))


if __name__ == "__main__":
    main()
