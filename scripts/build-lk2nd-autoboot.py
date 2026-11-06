#!/usr/bin/env python3
"""Build an autoboot lk2nd image for amber from the stock (force-fastboot) one.

The stock image is a valid Android boot image whose "kernel" area is:
    lk.bin-dtb (301,520 bytes, built with LK2ND_FORCE_FASTBOOT=1)
    followed by zeros up to kernel_size (21,464,764).
Its total size (header + kernel pages + ramdisk pages) is 25,180,160, which is
what the stock ABL accepts.

We keep the header and the zero padding, and put an lk.bin-dtb built WITHOUT
LK2ND_FORCE_FASTBOOT at the start of the kernel area.  Nothing else changes.

Usage:
    python3 build-lk2nd-autoboot.py \
        --stock lk2nd-amber-stocklayout.img \
        --lk build-lk2nd-autoboot/lk.bin-dtb \
        --out lk2nd-amber-autoboot.img

Flash the result to the boot_b partition:
    BOOT_B=$(blkid -t PARTLABEL=boot_b -o device)
    dd if=lk2nd-amber-autoboot.img of="$BOOT_B" bs=1M conv=fsync
"""
import argparse
import struct
import sys

PAGE = 2048
BOOT_MAGIC = b"ANDROID!"


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--stock", required=True, help="stock force-fastboot lk2nd image")
    ap.add_argument("--lk", required=True, help="autoboot lk.bin-dtb")
    ap.add_argument("--out", required=True)
    args = ap.parse_args()

    data = bytearray(open(args.stock, "rb").read())
    if data[:8] != BOOT_MAGIC:
        print("error: --stock is not an Android boot image", file=sys.stderr)
        return 1

    kernel_size, = struct.unpack("<I", data[8:12])
    ramdisk_size, = struct.unpack("<I", data[16:20])
    second_size, = struct.unpack("<I", data[24:28])

    def pages(n):
        return (n + PAGE - 1) // PAGE * PAGE

    total = PAGE + pages(kernel_size) + pages(ramdisk_size) + pages(second_size)
    print(f"stock: kernel_size={kernel_size} ramdisk_size={ramdisk_size} total={total}")

    lk = open(args.lk, "rb").read()
    if len(lk) > kernel_size:
        print("error: lk image is larger than the stock kernel area", file=sys.stderr)
        return 1

    data[PAGE:PAGE + kernel_size] = b"\x00" * kernel_size
    data[PAGE:PAGE + len(lk)] = lk

    open(args.out, "wb").write(data)
    print(f"wrote {args.out} ({len(data)} bytes, lk {len(lk)} bytes)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
