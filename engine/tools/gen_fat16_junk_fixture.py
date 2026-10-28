#!/usr/bin/env python3
"""tools/gen_fat16_junk_fixture.py - M-071 CLI junk-gate fixture generator.

Produces the SAME FAT16 image as tests/unit/test_cr_ui.cpp's
build_fat16_junk() (byte-identical geometry and plants), so the
Windows-lane CLI probe (checks.yml, "CLI junk-gate probes") and the
Linux-lane ABI test (CrUi.RecoverAllSkipsJunkFlaggedNodes) exercise one
fixture contract:

  DOC.TXT      1000 B, live    (the negative control)
  DELFILE.TXT   500 B, deleted
  PHOTO.JPG     200 B, live
  DESKTOP.INI    64 B, live    (the og FilterFolder FileName pool)
  $USNJRNL       20 B, live    (the og FilterFolder FileName pool)

The junk filter (filterfolder::apply) flags the last two at scan
completion; the CLI tree print and recover-all plan walk must skip them
(M-018/M-071), and the ABI bulk-enqueue paths must skip them too.

Usage: gen_fat16_junk_fixture.py OUT.img

Licensed under the MIT license. See LICENSE.
"""

import struct
import sys

IMG_SECS = 5074
FAT1 = 2 * 512
FAT2 = 22 * 512
ROOT = 42 * 512
DATA = 74 * 512


def put16(img, at, x):
    img[at] = x & 0xFF
    img[at + 1] = (x >> 8) & 0xFF


def put32(img, at, x):
    img[at] = x & 0xFF
    img[at + 1] = (x >> 8) & 0xFF
    img[at + 2] = (x >> 16) & 0xFF
    img[at + 3] = (x >> 24) & 0xFF


def dentry(img, slot, n11, attr, cl, sz, deleted):
    e = ROOT + slot * 32
    for i in range(11):
        img[e + i] = ord(n11[i])
    if deleted:
        img[e] = 0xE5
    img[e + 11] = attr
    put16(img, e + 20, cl >> 16)
    put16(img, e + 26, cl & 0xFFFF)
    put32(img, e + 28, sz)


def main():
    if len(sys.argv) != 2:
        print("usage: gen_fat16_junk_fixture.py OUT.img", file=sys.stderr)
        return 2
    img = bytearray(IMG_SECS * 512)

    img[3:11] = b"MSWIN4.1"
    put16(img, 0x0B, 512)
    img[0x0D] = 1
    put16(img, 0x0E, 2)
    img[0x10] = 2
    put16(img, 0x11, 512)
    put16(img, 0x13, IMG_SECS)
    img[0x15] = 0xF8
    put16(img, 0x16, 20)
    put16(img, 0x18, 63)
    put16(img, 0x1A, 255)
    img[0x26] = 0x29
    img[0x2B:0x36] = b"TESTVOL    "
    img[0x36:0x3E] = b"FAT16   "
    img[510] = 0x55
    img[511] = 0xAA

    # M-015 canonical media words + the per-file chains
    put16(img, FAT1 + 0 * 2, 0xFFF8)
    put16(img, FAT1 + 1 * 2, 0xFFFF)
    put16(img, FAT2 + 0 * 2, 0xFFF8)
    put16(img, FAT2 + 1 * 2, 0xFFFF)
    put16(img, FAT1 + 5 * 2, 0xFFFF)
    put16(img, FAT2 + 5 * 2, 0xFFFF)  # root dir
    put16(img, FAT1 + 10 * 2, 11)
    put16(img, FAT2 + 10 * 2, 11)
    put16(img, FAT1 + 11 * 2, 12)
    put16(img, FAT2 + 11 * 2, 12)
    put16(img, FAT1 + 12 * 2, 0xFFFF)
    put16(img, FAT2 + 12 * 2, 0xFFFF)  # doc
    put16(img, FAT1 + 20 * 2, 0xFFFF)
    put16(img, FAT2 + 20 * 2, 0xFFFF)  # del file
    put16(img, FAT1 + 30 * 2, 31)
    put16(img, FAT2 + 30 * 2, 31)
    put16(img, FAT1 + 31 * 2, 0xFFFF)
    put16(img, FAT2 + 31 * 2, 0xFFFF)  # photo
    put16(img, FAT1 + 40 * 2, 0xFFFF)
    put16(img, FAT2 + 40 * 2, 0xFFFF)  # desktop.ini
    put16(img, FAT1 + 41 * 2, 0xFFFF)
    put16(img, FAT2 + 41 * 2, 0xFFFF)  # $usnjrnl

    dentry(img, 0, "DOC     TXT", 0x20, 10, 1000, False)
    dentry(img, 1, "DELFILE TXT", 0x20, 20, 500, True)
    dentry(img, 2, "PHOTO   JPG", 0x20, 30, 200, False)
    dentry(img, 3, "DESKTOP INI", 0x20, 40, 64, False)
    dentry(img, 4, "$USNJRNL   ", 0x20, 41, 20, False)

    img[DATA + (10 - 2) * 512: DATA + (10 - 2) * 512 + 7] = b"DOCDATA"
    img[DATA + (20 - 2) * 512: DATA + (20 - 2) * 512 + 7] = b"DELDATA"
    p = DATA + (30 - 2) * 512
    for i in range(200):
        img[p + i] = ord("A") + (i % 26)
    img[DATA + (40 - 2) * 512: DATA + (40 - 2) * 512 + 18] = b"[junk-desktop-ini]"
    img[DATA + (41 - 2) * 512: DATA + (41 - 2) * 512 + 14] = b"[junk-usnjrnl]"

    with open(sys.argv[1], "wb") as f:
        f.write(img)
    print(f"fat16-junk fixture written: {sys.argv[1]} ({len(img)} bytes)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
