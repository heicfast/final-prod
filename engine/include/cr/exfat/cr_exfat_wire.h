/*
 * cr_exfat_wire.h - exFAT on-disk structures (pure).
 *
 * Constants cross-checked against the RE corpus (fat-exfat v2 section 3) -
 * see docs/ground-truth-contracts.md section 5.
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

constexpr u16 EXFAT_SIGNATURE = 0xAA55;

struct BootSectorMain {          /* boot sector 0 of the 24-sector boot region */
    u8 jump[3];
    char fs_name[8];             /* "EXFAT   " */
    u8 must_be_zero[53];
    u64 partition_offset;        /* in sectors */
    u64 volume_length;           /* in sectors */
    u32 fat_offset;              /* sector, from volume start */
    u32 fat_length;              /* sectors; may be 0 for FAT-less (contiguous) */
    u32 cluster_heap_offset;
    u32 cluster_count;
    u32 first_cluster_of_root;
    u32 volume_serial;
    u16 fs_revision;
    u16 volume_flags;            /* bit 0: active FAT; bit 1: volume dirty */
    u8 bytes_per_sector_shift;   /* log2(bytes per sector): 9..12 */
    u8 sectors_per_cluster_shift;/* log2(sectors per cluster) */
    u8 fat_count;                /* 1 */
    u8 drive_select;
    u8 percent_in_use;
    u8 reserved[7];
    u8 boot_code[390];
    u8 signature[2];
};
static_assert(sizeof(BootSectorMain) == 512);

/* ---- Directory entries (0x20 bytes; type byte low 7 bits + InUse 0x80) --- */

constexpr u8 ENTRY_IN_USE = 0x80;    /* corpus: bit 0x80, cleared on delete */
constexpr u8 ENTRY_TYPE_MASK = 0x7F;

/* Entry types (live values; +0x80 when in use) - corpus section 3 table. */
constexpr u8 TYPE_NONE = 0x00;
constexpr u8 TYPE_BITMAP = 0x01;         /* 0x81: allocation bitmap descriptor */
constexpr u8 TYPE_UPCASE = 0x02;         /* 0x82: up-case table */
constexpr u8 TYPE_VOLUME_LABEL = 0x03;   /* 0x83: volume label (triple anchor) */
constexpr u8 TYPE_FILE = 0x05;           /* 0x85: file dentry (set start) */
constexpr u8 TYPE_VOLUME_GUID = 0x20;
constexpr u8 TYPE_STREAM = 0x40;         /* 0xC0: stream extension */
constexpr u8 TYPE_NAME = 0x41;           /* 0xC1: filename fragment */

struct GenericEntry {
    u8 type;
    u8 body[31];
};
static_assert(sizeof(GenericEntry) == 32);

struct BitmapEntry {
    u8 type;                     /* 0x81 */
    u8 flags;                    /* bit 0: which FAT/copy this bitmap tracks */
    u8 reserved;
    u32 first_cluster;
    u64 length;                  /* valid-data bytes of the bitmap */
};
static_assert(sizeof(BitmapEntry) == 16);

struct UpcaseEntry {
    u8 type;                     /* 0x82 */
    u8 reserved1[3];
    u32 checksum;                /* of the up-case table contents */
    u32 first_cluster;
    u64 length;                  /* bytes of table */
};
static_assert(sizeof(UpcaseEntry) == 20);

struct VolumeLabelEntry {
    u8 type;                     /* 0x83 */
    u8 character_count;          /* 0..11, 0 == none */
    u16 label[11];
    u8 reserved[8];
};
static_assert(sizeof(VolumeLabelEntry) == 32);

struct FileEntry {
    u8 type;                     /* 0x85 */
    u8 attribute_count;          /* 0..15 count of TYPE_STREAM+TYPE_NAME after */
    u16 reserved1;
    u32 checksum;                /* of the following stream+name entries */
    u16 creation_time;
    u16 creation_date;
    u16 modification_time;
    u16 modification_date;
    u16 access_time;
    u16 access_date;
    u8 creation_tenths;
    u8 modification_tenths;      /* high bit set = 10ms units w/ sub-second */
    u8 creation_timezone;
    u8 modification_timezone;
    u8 access_timezone;
    u8 reserved2[7];
};
static_assert(sizeof(FileEntry) == 32);

struct StreamEntry {
    u8 type;                     /* 0xC0 */
    u8 flags;                    /* bit 0: bit 5 (0x20) copied in flags; see spec:
                                    bit 0x01 = possible cluster allocation */
    u8 reserved1;                /* corpus: must be zero */
    u8 name_length;              /* ucs2 chars, 1..255 */
    u16 name_hash;               /* of the up-cased name (FNV over ucs2) */
    u16 reserved2;               /* corpus: must be zero */
    u64 valid_data_length;
    u32 reserved3;               /* corpus: must be zero */
    u32 first_cluster;           /* corpus: == 1 (no-FAT) or >= 0xFFFFFF7 (EOC-family gate) */
    u64 data_length;
};
static_assert(sizeof(StreamEntry) == 24);

struct NameEntry {
    u8 type;                     /* 0xC1 */
    u8 flags;                    /* bit 0x40 = last fragment */
    u16 name[15];                /* ucs2 fragment of this step */
};
static_assert(sizeof(NameEntry) == 32);

#pragma pack(pop)

inline u8 entry_type(u8 raw) { return raw & ENTRY_TYPE_MASK; }
inline bool entry_in_use(u8 raw) { return (raw & ENTRY_IN_USE) != 0; }

inline bool boot_signature_ok(const u8 *sector) {
    return sector[510] == 0x55 && sector[511] == 0xAA;
}
inline bool oem_is_exfat(const BootSectorMain &bs) {
    char want[8] = {'E', 'X', 'F', 'A', 'T', ' ', ' ', ' '};
    return std::memcmp(bs.fs_name, want, 8) == 0;
}

/* Per-sector boot-region checksum: the 24-sector boot region carries, in each
 * sector's +0x1FC slot (u32), the value computed over the OTHER 23 sectors
 * with signature/percent/reserved/boot fields zeroed per the spec flow. */
constexpr u32 EXFAT_BOOT_REGION_SECTORS = 24;

} // namespace exfat
} // namespace cr

#endif // CR_EXFAT_WIRE_H_
