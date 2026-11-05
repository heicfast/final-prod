#!/usr/bin/env python3
"""s10 whole-disk basic-partition fixture: MBR + 2 FAT16 volumes, each with
one live file (DOC.TXT) and one deleted file (DELFILE.TXT). Mirrors the real
og G-session shape (two known in-table volumes on one disk)."""
import struct, sys

BPS = 512
SPC = 1
RESERVED = 1
NUMFATS = 1
ROOT_ENTRIES = 512
ROOT_SECS = (ROOT_ENTRIES * 32 + BPS - 1) // BPS  # 32


def fat16_volume(label, oem="MSDOS5.0"):
    spf = 66
    total = RESERVED + NUMFATS * spf + ROOT_SECS + 16384 * SPC
    img = bytearray(total * BPS)
    bs = bytearray(BPS)
    bs[0:3] = b"\xeb\x3c\x90"
    bs[3:11] = oem.encode()
    struct.pack_into("<H", bs, 0x0B, BPS)
    bs[0x0D] = SPC
    struct.pack_into("<H", bs, 0x0E, RESERVED)
    bs[0x10] = NUMFATS
    struct.pack_into("<H", bs, 0x11, ROOT_ENTRIES)
    struct.pack_into("<H", bs, 0x16, spf)
    struct.pack_into("<H", bs, 0x18, 63)
    struct.pack_into("<H", bs, 0x1A, 255)
    struct.pack_into("<I", bs, 0x20, total)
    bs[0x24] = 0xF8  # media
    struct.pack_into("<H", bs, 0x1C, 63)  # hidden
    bs[0x2B:0x36] = label.encode()[:11].ljust(11)  # volume label
    bs[0x36:0x3E] = b"FAT16   "
    bs[0x1FE:0x200] = b"\x55\xaa"
    img[0:BPS] = bs
    fat = bytearray(spf * BPS)
    struct.pack_into("<H", fat, 0, 0xFFF8)
    struct.pack_into("<H", fat, 2, 0xFFFF)
    struct.pack_into("<H", fat, 4, 0xFFFF)  # cluster 2 EOC (DOC.TXT)
    struct.pack_into("<H", fat, 6, 0xFFFF)  # cluster 3 EOC (DELFILE.TXT)
    fat_off = RESERVED * BPS
    img[fat_off:fat_off + len(fat)] = fat
    root_off = (RESERVED + NUMFATS * spf) * BPS

    def dirent(name, ext, first_clus, size, deleted):
        e = bytearray(32)
        n = (name + "        ")[:8].encode()
        x = (ext + "   ")[:3].encode()
        e[0:8] = n
        e[8:11] = x
        e[0] = 0xE5 if deleted else n[0]
        e[11] = 0x20  # archive
        struct.pack_into("<H", e, 0x1A, first_clus)
        struct.pack_into("<I", e, 0x1C, size)
        return e

    img[root_off:root_off + 32] = dirent("DOC", "TXT", 2, 11, False)
    img[root_off + 32:root_off + 64] = dirent("DELFILE", "TXT", 3, 9, True)
    data_off = (RESERVED + NUMFATS * spf + ROOT_SECS) * BPS
    img[data_off:data_off + 11] = b"HELLO CORE"
    img[data_off + BPS:data_off + BPS + 9] = b"DELETED!!"
    return bytes(img), total


def main(path):
    vol, vol_secs = fat16_volume("VOLONE   ")
    vol2, vol2_secs = fat16_volume("VOLTWO   ")
    p1_start = 2048
    p2_start = p1_start + vol_secs
    disk_secs = p2_start + vol2_secs + 2048
    img = bytearray(disk_secs * BPS)
    img[p1_start * BPS:p1_start * BPS + len(vol)] = vol
    img[p2_start * BPS:p2_start * BPS + len(vol2)] = vol2
    mbr = bytearray(BPS)

    def pentry(start, secs, idx):
        e = bytearray(16)
        e[0] = 0x00
        e[1:4] = bytes([0x20, 0x21, 0x00])
        e[4] = 0x0E  # FAT16 LBA
        e[5:8] = bytes([0x20, 0x21, 0x00])
        struct.pack_into("<I", e, 8, start)
        struct.pack_into("<I", e, 12, secs)
        mbr[idx * 16 + 446:idx * 16 + 446 + 16] = e

    pentry(p1_start, vol_secs, 0)
    pentry(p2_start, vol2_secs, 1)
    mbr[510:512] = b"\x55\xaa"
    img[0:BPS] = mbr
    with open(path, "wb") as f:
        f.write(img)
    print(f"disk: {path} {len(img)//(1024*1024)} MiB; P1 FAT16 @{p1_start} ({vol_secs} secs), "
          f"P2 FAT16 @{p2_start} ({vol2_secs} secs); each: DOC.TXT live + DELFILE.TXT deleted")


if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else "/tmp/s10disk.img")
