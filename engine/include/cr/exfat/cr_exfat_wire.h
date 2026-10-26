/*
 * cr_exfat_wire.h - exFAT on-disk structures (pure).
 *
 * Layouts are the SPECIFICATION values (elm-chan exFAT documentation /
 * MS SDCFF v1.00 - see docs/ground-truth-contracts.md section 5).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_EXFAT_WIRE_H_
#define CR_EXFAT_WIRE_H_

#include "../cr_types.h"
#include <cstring>

namespace cr {
namespace exfat {

#pragma pack(push, 1)

struct BootSectorMain {           /* boot sector 0 of the 24-sector boot region */
    u8 jump[3];                   /* +0x00 EB 76 90 */
    char fs_name[8];              /* +0x03 "EXFAT   " */
    u8 must_be_zero[53];          /* +0x0B (spans the FAT BPB area) */
    u64 partition_offset;         /* +0x40 sectors */
    u64 volume_length;            /* +0x48 sectors */
    u32 fat_offset;               /* +0x50 */
    u32 fat_length;               /* +0x54 sectors per FAT */
    u32 cluster_heap_offset;      /* +0x58 */
    u32 cluster_count;            /* +0x5C max 0xFFFFFFF5 */
    u32 first_cluster_of_root;    /* +0x60 */
    u32 volume_serial;            /* +0x64 */
    u16 fs_revision;              /* +0x68 */
    u16 volume_flags;             /* +0x6A bit0 ActiveFat, bit1 dirty */
    u8 bytes_per_sector_shift;    /* +0x6C 9..12 */
    u8 sectors_per_cluster_shift; /* +0x6D 0..25-bps_shift */
    u8 fat_count;                 /* +0x6E 1 or 2 */
    u8 drive_select;              /* +0x6F */
    u8 percent_in_use;            /* +0x70 0xFF = n/a */
    u8 reserved[7];               /* +0x71 */
    u8 boot_code[390];            /* +0x78 */
    u8 signature[2];              /* +0x1FE 55 AA */
    /* +0x1FC carries the boot-region checksum u32 (all 24 sectors) */
};
static_assert(sizeof(BootSectorMain) == 512);

/* ---- Directory entries (0x20 bytes; InUse = bit 0x80 of the type byte) --- */

constexpr u8 ENTRY_IN_USE = 0x80;
constexpr u8 ENTRY_TYPE_MASK = 0x7F;

constexpr u8 TYPE_NONE = 0x00;
constexpr u8 TYPE_BITMAP = 0x01;       /* 0x81 */
constexpr u8 TYPE_UPCASE = 0x02;       /* 0x82 */
constexpr u8 TYPE_VOLUME_LABEL = 0x03; /* 0x83 */
constexpr u8 TYPE_FILE = 0x05;         /* 0x85 */
constexpr u8 TYPE_STREAM = 0x40;       /* 0xC0 */
constexpr u8 TYPE_NAME = 0x41;         /* 0xC1 */

/* File entry (0x85) - spec-exact field order */
struct FileEntry {
    u8 type;              /* +0x00 0x85 */
    u8 secondary_count;   /* +0x01 stream+name entries following */
    u16 set_checksum;     /* +0x02 16-bit, excludes only this field */
    u16 file_attribute;   /* +0x04 bit0 RO, 1 hidden, 2 system, 4 dir, 5 archive */
    u16 reserved1;        /* +0x06 */
    u32 create_timestamp; /* +0x08 date<<16 | time (FAT layout) */
    u32 modify_timestamp; /* +0x0C */
    u32 access_timestamp; /* +0x10 */
    u8 create_10ms;       /* +0x14 10ms units 0..199 */
    u8 modify_10ms;       /* +0x15 */
    u8 create_tz;         /* +0x16 quarter-hours | 0x80; 0x00 = unused.
                           * Parsed for completeness but NOT applied (S-011):
                           * the og's engine never reads these bytes - it
                           * feeds the raw date/time words to
                           * DosDateTimeToFileTime (six call sites, zero
                           * quarter-hour arithmetic in the whole DLL). */
    u8 modify_tz;         /* +0x17 (same) */
    u8 access_tz;         /* +0x18 (same) */
    u8 reserved2[7];      /* +0x19 */
};
static_assert(sizeof(FileEntry) == 32);

/* Stream extension entry (0xC0) */
struct StreamEntry {
    u8 type;               /* +0x00 0xC0 */
    u8 flags;              /* +0x01 bit0 AllocationPossible, bit1 NoFatChain */
    u8 reserved1;          /* +0x02 must be 0 */
    u8 name_length;        /* +0x03 u16 chars, 1..255 */
    u16 name_hash;         /* +0x04 */
    u16 reserved2;         /* +0x06 must be 0 */
    u64 valid_data_length; /* +0x08 */
    u32 reserved3;         /* +0x10 must be 0 */
    u32 first_cluster;     /* +0x14 */
    u64 data_length;       /* +0x18 */
};
static_assert(sizeof(StreamEntry) == 32);

/* File name entry (0xC1): 15 u16 chars at +2 */
struct NameEntry {
    u8 type;      /* +0x00 0xC1 */
    u8 flags;     /* +0x01 always 0 */
    u16 name[15]; /* +0x02 UTF-16LE fragment */
};
static_assert(sizeof(NameEntry) == 32);

/* Allocation bitmap entry (0x81 — the og's own walk validates this shape
 * at decompile :24380-24460: flags@+1 free, reserved@+2..+0x13 zero,
 * cluster@+0x14, length@+0x18; S-033 INVALID record) */
struct BitmapEntry {
    u8 type;           /* +0x00 0x81 */
    u8 bitmap_flags;   /* +0x01 bit0: which FAT/bitmap copy */
    u8 reserved[18];   /* +0x02 */
    u32 first_cluster; /* +0x14 */
    u64 data_length;   /* +0x18 bytes of valid bitmap */
};
static_assert(sizeof(BitmapEntry) == 32);

/* Up-case-table entry (0x82) — reintroduced by N-067: the volume-integrity
 * pass reads its TableChecksum / FirstCluster / DataLength. Layout is the
 * og's own case -0x7e validation shape (decompile :24440-24452):
 * reserved1@+1..+3 zero-checked, TableChecksum@+4 free (the checked field),
 * reserved2@+8..+0x13 zero-checked, then the shared cluster/length tail. */
struct UpcaseEntry {
    u8 type;            /* +0x00 0x82 */
    u8 reserved1[3];    /* +0x01 og: zero-checked */
    u32 table_checksum; /* +0x04 checksum32 of the table bytes */
    u8 reserved2[12];   /* +0x08 og: zero-checked */
    u32 first_cluster;  /* +0x14 */
    u64 data_length;    /* +0x18 table bytes */
};
static_assert(sizeof(UpcaseEntry) == 32);

/* Volume-label (0x83) entries: the engine skips them by TYPE byte only -
 * no field is ever parsed - so the struct stays deleted (AUDIT-FAT §6
 * rows 16; W4: structs are kept only as layout docs where the parser
 * reads their fields, cf. BootSectorMain). */

#pragma pack(pop)

inline u8 entry_type(u8 raw) {
    return raw & ENTRY_TYPE_MASK;
}
inline bool entry_in_use(u8 raw) {
    return (raw & ENTRY_IN_USE) != 0;
}

inline bool boot_signature_ok(const u8* sector) {
    return sector[510] == 0x55 && sector[511] == 0xAA;
}
inline bool oem_is_exfat(const BootSectorMain& bs) {
    char want[8] = {'E', 'X', 'F', 'A', 'T', ' ', ' ', ' '};
    return std::memcmp(bs.fs_name, want, 8) == 0;
}

/* FAT entry semantics (exFAT): EOC 0xFFFFFFFF, bad 0xFFFFFFF7; a FREE
 * cluster is indicated by the ALLOCATION BITMAP bit 0 (FAT entry then has
 * no meaning). The engine keeps its local EOC/BAD constants at
 * cr_exfat_quick.cpp (the wire-level duplicates were never consumed). */

constexpr u32 BOOT_REGION_SECTORS = 24; /* main 0..11, backup 12..23 */
constexpr u32 BACKUP_BOOT_SECTOR = 12;

} // namespace exfat
} // namespace cr

#endif // CR_EXFAT_WIRE_H_
