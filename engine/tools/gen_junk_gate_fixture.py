#!/usr/bin/env python3
"""gen_junk_gate_fixture.py - the M-071 CLI junk-gate contract fixture.

Generates tests/data/images/junk-gate-fat16.raw: a deterministic FAT16
volume carrying the og's FilterFolder.ini hidden set (desktop.ini,
$UsnJrnl, "System Volume Information", the zh-CN "New Folder") alongside
positive controls (DOC.TXT, the deleted DELFILE.TXT, KEEP.ME, and a file
INSIDE the junk dir so the subtree-skip semantic is observable).

The layout mirrors tests/unit/test_orchestrator.cpp's build_fat16()
geometry (5074 sectors, 1 sector/cluster, 2 FATs x 20 sectors, root at
sector 42, data at sector 74) so the in-memory suite and the file-backed
fixture assert the same volume shape.

Usage: python3 tools/gen_junk_gate_fixture.py [output-path]
Default output: tests/data/images/junk-gate-fat16.raw (from repo root).
Pure stdlib, no dependencies, byte-deterministic (no timestamps - FAT
date fields are the fixed 2026-01-01/00:00 sentinel used by the suite's
in-memory builders).
"""

import struct
import sys
from pathlib import Path

SECTOR = 512
TOTAL_SECTORS = 5074
RESERVED = 2          # sectors before FAT1
FAT_SECTORS = 20      # per FAT
FAT_COUNT = 2
ROOT_ENTRIES = 512    # 32 sectors
ROOT_SECTOR = 42      # RESERVED + FAT_COUNT*FAT_SECTORS
DATA_SECTOR = 74      # ROOT_SECTOR + ROOT_ENTRIES*32/SECTOR

ATTR_RO = 0x01
ATTR_DIR = 0x10
ATTR_ARCH = 0x20

# FAT date/time sentinel: 2026-01-01 00:00:00 (matches the suite's
# in-memory builders; determinism over realism).
FAT_DATE = (0 << 9) | (1 << 5) | 1   # year-1980=0, month=1, day=1
FAT_TIME = 0                          # 00:00:00


def put16(buf: bytearray, off: int, val: int) -> None:
    struct.pack_into("<H", buf, off, val & 0xFFFF)


def put32(buf: bytearray, off: int, val: int) -> None:
    struct.pack_into("<I", buf, off, val & 0xFFFFFFFF)


def lfn_checksum(alias11: bytes) -> int:
    s = 0
    for b in alias11:
        s = ((s & 1) << 7) + (s >> 1) + b
    return s & 0xFF


def dentry(name11: bytes, attrs: int, first_cluster: int, size: int,
           deleted: bool = False) -> bytes:
    e = bytearray(32)
    e[0:11] = name11
    if deleted:
        e[0] = 0xE5
    e[11] = attrs
    # creation/access/write times: the fixed sentinel
    struct.pack_into("<H", e, 14, FAT_TIME)   # creation time
    struct.pack_into("<H", e, 16, FAT_DATE)   # creation date
    struct.pack_into("<H", e, 18, FAT_DATE)   # access date
    struct.pack_into("<H", e, 20, (first_cluster >> 16) & 0xFFFF)
    struct.pack_into("<H", e, 22, FAT_TIME)   # write time
    struct.pack_into("<H", e, 24, FAT_DATE)   # write date
    struct.pack_into("<H", e, 26, first_cluster & 0xFFFF)
    struct.pack_into("<I", e, 28, size)
    return bytes(e)


def lfn_slots(name: str, alias11: bytes) -> list:
    """LFN chain: fragments physically DESCENDING (last first), 13 UTF-16
    chars per slot, checksum ties to the 8.3 alias that follows - the
    engine's lfn_assemble contract (mirrors the suite's writer)."""
    chk = lfn_checksum(alias11)
    u16name = name.encode("utf-16-le")
    frags = (len(name) + 12) // 13
    slots = []
    for f in range(frags, 0, -1):
        e = bytearray(b"\xff" * 32)
        e[0] = (0x40 | f) if f == frags else f
        e[11] = 0x0F   # LFN
        e[12] = 0
        e[13] = chk
        e[26] = 0
        e[27] = 0
        pos = (f - 1) * 13
        for off, cnt in ((1, 5), (14, 6), (28, 2)):
            for c in range(cnt):
                if pos < len(name):
                    ch = struct.unpack_from("<H", u16name, pos * 2)[0]
                elif pos == len(name):
                    ch = 0x0000
                else:
                    ch = 0xFFFF
                struct.pack_into("<H", e, off + 2 * c, ch)
                pos += 1
        slots.append(bytes(e))
    return slots


def main() -> int:
    out_path = Path(sys.argv[1]) if len(sys.argv) > 1 else \
        Path(__file__).resolve().parent.parent / "tests/data/images/junk-gate-fat16.raw"
    img = bytearray(TOTAL_SECTORS * SECTOR)

    # ---- boot sector (BPB) - the suite's build_fat16 geometry ----
    img[3:11] = b"MSWIN4.1"
    put16(img, 0x0B, SECTOR)
    img[0x0D] = 1                      # sectors/cluster
    put16(img, 0x0E, RESERVED)
    img[0x10] = FAT_COUNT
    put16(img, 0x11, ROOT_ENTRIES)
    put16(img, 0x13, TOTAL_SECTORS)    # 16-bit total
    img[0x15] = 0xF8                   # fixed disk media
    put16(img, 0x16, FAT_SECTORS)
    put16(img, 0x18, 63)               # sectors/track
    put16(img, 0x1A, 255)              # heads
    put32(img, 0x20, 0)                # total32: unused
    img[0x26] = 0x29                   # extended boot signature
    put32(img, 0x27, 0x1234ABCD)       # volume id (fixed: determinism)
    img[0x2B:0x36] = b"JUNKGATE   "  # volume label (11 bytes, space-padded: a
    # short slice assignment would RESIZE the bytearray - the size
    # assertion in test_orchestrator.cpp guards exactly this class)
    img[0x36:0x3E] = b"FAT16   "
    img[510] = 0x55
    img[511] = 0xAA

    # ---- FAT media words (M-015: u32@FAT[0] == 0xFFFFFFF8 on FAT16) ----
    fat1 = RESERVED * SECTOR
    fat2 = fat1 + FAT_SECTORS * SECTOR
    for fat in (fat1, fat2):
        put16(img, fat + 0, 0xFFF8)
        put16(img, fat + 2, 0xFFFF)
        # chains: 10->11 (DOC.TXT), 13/14/15/20/21/22 -> EOF
        put16(img, fat + 10 * 2, 11)
        put16(img, fat + 11 * 2, 0xFFFF)
        put16(img, fat + 13 * 2, 0xFFFF)   # DESKTOP.INI
        put16(img, fat + 14 * 2, 0xFFFF)   # $USNJRNL
        put16(img, fat + 15 * 2, 0xFFFF)   # KEEP.ME
        put16(img, fat + 20 * 2, 0xFFFF)   # DELFILE.TXT (deleted)
        put16(img, fat + 21 * 2, 0xFFFF)   # SVI dir
        put16(img, fat + 22 * 2, 0xFFFF)   # TRACKER.TXT (inside SVI)

    # ---- root directory ----
    root = ROOT_SECTOR * SECTOR

    def cluster_off(cl: int) -> int:
        return (DATA_SECTOR + (cl - 2)) * SECTOR

    entries = []
    entries.append(dentry(b"DOC     TXT", ATTR_ARCH, 10, 1000))
    entries.append(dentry(b"DELFILE TXT", ATTR_ARCH, 20, 500, deleted=True))
    entries.append(dentry(b"DESKTOP INI", ATTR_ARCH, 13, 100))
    entries.append(dentry(b"$USNJRNL   ", ATTR_ARCH, 14, 282))
    entries.append(dentry(b"KEEP    ME ", ATTR_ARCH, 15, 10))
    entries += lfn_slots("System Volume Information", b"SYSTEM~1   ")
    entries.append(dentry(b"SYSTEM~1   ", ATTR_DIR, 21, 0))
    entries += lfn_slots("\u65B0\u5EFA\u6587\u4EF6\u5939", b"NEWFOL~1   ")
    entries.append(dentry(b"NEWFOL~1   ", ATTR_DIR, 0, 0))
    # end-of-directory marker
    entries.append(b"\x00" * 32)

    off = root
    for e in entries:
        img[off:off + 32] = e
        off += 32

    # ---- the SVI subdirectory (cluster 21): one inner file ----
    svi = cluster_off(21)
    img[svi:svi + 32] = dentry(b"TRACKER TXT", ATTR_ARCH, 22, 16)
    img[svi + 32:svi + 64] = b"\x00" * 32

    # ---- file contents (markers the recover step can assert) ----
    def content(cl: int, data: bytes) -> None:
        o = cluster_off(cl)
        img[o:o + len(data)] = data

    content(10, b"DOCDATA" + b"A" * 993)            # 1000 bytes -> 2 clusters
    content(13, b"DESKTOPINI-JUNK" + b"\x00" * 86)  # 100
    content(14, b"$USNJRNL-JUNK-DATA" + b"\x00" * 264)  # 282
    content(15, b"KEEPME!TEN")                      # 10
    content(20, b"DELDATA" + b"\x00" * 493)         # 500
    content(22, b"INNERFILE-CONTENT")               # 16

    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_bytes(img)
    print(f"wrote {out_path} ({len(img)} bytes, {TOTAL_SECTORS} sectors)")
    print("junk (FilterFolder hidden set): DESKTOP.INI, $USNJRNL,")
    print("  System Volume Information (dir, contains TRACKER.TXT),")
    print("  \u65B0\u5EFA\u6587\u4EF6\u5939 (dir)")
    print("controls: DOC.TXT, DELFILE.TXT (deleted), KEEP.ME")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
