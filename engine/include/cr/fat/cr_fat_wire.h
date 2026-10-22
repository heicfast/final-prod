/*
 * cr_fat_wire.h - FAT12/16/32 on-disk structures (pure).
 *
 * Constants cross-checked against the RE corpus (fat-exfat v1+v2) - see
 * docs/ground-truth-contracts.md section 4.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_FAT_WIRE_H_
#define CR_FAT_WIRE_H_

#include "../cr_types.h"
#include <cstring>

namespace cr {
namespace fat {

#pragma pack(push, 1)

/* ---- BPB (BIOS Parameter Block) ---------------------------------------- */

constexpr u16 FAT_SIGNATURE = 0xAA55;

struct BpbCommon {              /* +0x0B in boot sector */
    u16 bytes_per_sector;       /* 512/1024/2048/4096 */
    u8 sectors_per_cluster;     /* power of two */
    u16 reserved_sectors;
    u8 fat_count;               /* usually 2 (mirrored - CDoubleFATTable) */
    u16 root_entry_count;       /* FAT12/16 only */
    u16 total_sectors_16;
    u8 media_descriptor;
    u16 sectors_per_fat_16;
    u16 sectors_per_track;
    u16 heads;
    u32 hidden_sectors;
    u32 total_sectors_32;
};
static_assert(sizeof(BpbCommon) == 25);

struct BootSectorFat12_16 {     /* +0x24 layout used by FAT12/16 volumes */
    u8 jump[3];
    char oem[8];
    BpbCommon bpb;
    u8 drive_number;
    u8 reserved1;
    u8 boot_signature;         /* 0x29 = extended boot info present */
    u32 volume_id;
    u8 volume_label[11];
    u8 fs_type[8];             /* "FAT12   " / "FAT16   " */
    u8 boot_code[448];
    u8 signature[2];
};
static_assert(sizeof(BootSectorFat12_16) == 512);

struct BootSectorFat32 {
    u8 jump[3];
    char oem[8];
    BpbCommon bpb;
    u32 sectors_per_fat_32;
    u16 flags;                 /* bit 7-8: active FAT index (0 = mirror) */
    u16 version;               /* 0 */
    u32 root_cluster;          /* 2 */
    u16 fs_info_sector;
    u16 backup_boot_sector;    /* 6 */
    u8 reserved[12];
    u8 drive_number;
    u8 reserved1;
    u8 boot_signature;
    u32 volume_id;
    u8 volume_label[11];
    u8 fs_type[8];             /* "FAT32   " */
    u8 boot_code[420];
    u8 signature[2];
};
static_assert(sizeof(BootSectorFat32) == 512);

/* ---- Directory entry (0x20 bytes - corpus fixed) ----------------------- */

constexpr u8 DENTRY_DELETED = 0xE5;          /* corpus: deleted marker */
constexpr u8 DENTRY_END = 0x00;
constexpr u8 ATTR_READ_ONLY = 0x01;
constexpr u8 ATTR_HIDDEN = 0x02;
constexpr u8 ATTR_SYSTEM = 0x04;
constexpr u8 ATTR_VOLUME_ID = 0x08;
constexpr u8 ATTR_DIRECTORY = 0x10;          /* corpus: '.' detection */
constexpr u8 ATTR_ARCHIVE = 0x20;
constexpr u8 ATTR_LFN = 0x0F;                /* VFAT long-name slot */
constexpr u8 ATTR_MASK_LONGNAME = 0x3F;

struct DirEntry {
    u8 name[11];                /* 8.3, space padded; 0xE5 = deleted, 0x00 = end */
    u8 attributes;
    u8 nt_reserved;
    u8 creation_tenths;
    u16 creation_time;
    u16 creation_date;
    u16 access_date;
    u16 first_cluster_high;
    u16 write_time;
    u16 write_date;
    u16 first_cluster_low;
    u32 file_size;
};
static_assert(sizeof(DirEntry) == 32);

/* VFAT long-name slot (also 0x20 bytes, attributes == 0x0F). */
struct LfnEntry {
    u8 sequence;                /* bit 0x40 = last fragment, 0x20 was deleted */
    u16 name1[5];               /* ucs2 chars 1-5 */
    u8 attributes;              /* 0x0F */
    u8 type;                    /* 0 */
    u8 checksum;                /* of the 8.3 name */
    u16 name2[6];               /* chars 6-11 */
    u16 first_cluster;          /* 0 */
    u16 name3[2];               /* chars 12-13 */
};
static_assert(sizeof(LfnEntry) == 32);

/* FAT date/time -> unix seconds helpers (pure). */
u32 fat_date_to_unix(u16 date, u16 time, u8 tenths);

#pragma pack(pop)

/* Canonical chain constants (corpus IFATTable: BAD = 0xFFFFFF7, EOC >= 0xFFFFFF8). */
constexpr u32 CHAIN_BAD = 0x0FFFFFF7;
constexpr u32 CHAIN_EOC_MIN = 0x0FFFFFF8;
constexpr u32 CHAIN_FREE = 0x00000000;

inline bool chain_is_bad(u32 next) { return next >= CHAIN_BAD && next < CHAIN_EOC_MIN; }
inline bool chain_is_eoc(u32 next) { return next >= CHAIN_EOC_MIN; }
inline bool chain_is_reserved(u32 next) { return next >= 0x0FFFFFF0 && next < CHAIN_BAD; }

inline bool boot_signature_ok(const u8 *sector) {
    return sector[510] == 0x55 && sector[511] == 0xAA;
}

/* '.' self-entry detection (corpus: 8 chars "."+spaces, +0xA space, attr 0x10). */
inline bool is_dot_entry(const DirEntry &e) {
    return e.name[0] == '.' && e.name[1] == ' ' && e.name[2] == ' ' && e.name[3] == ' ' &&
           e.name[4] == ' ' && e.name[5] == ' ' && e.name[6] == ' ' && e.name[7] == ' ' &&
           e.nt_reserved == ' ' && (e.attributes & ATTR_DIRECTORY) != 0;
}

} // namespace fat
} // namespace cr

#endif // CR_FAT_WIRE_H_
