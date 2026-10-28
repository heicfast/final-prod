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

struct BpbCommon {          /* +0x0B in boot sector */
    u16 bytes_per_sector;   /* 512/1024/2048/4096 */
    u8 sectors_per_cluster; /* power of two */
    u16 reserved_sectors;
    u8 fat_count;         /* usually 2 (mirrored - CDoubleFATTable) */
    u16 root_entry_count; /* FAT12/16 only */
    u16 total_sectors_16;
    u8 media_descriptor;
    u16 sectors_per_fat_16;
    u16 sectors_per_track;
    u16 heads;
    u32 hidden_sectors;
    u32 total_sectors_32;
};
static_assert(sizeof(BpbCommon) == 25);

struct BootSectorFat12_16 { /* +0x24 layout used by FAT12/16 volumes */
    u8 jump[3];
    char oem[8];
    BpbCommon bpb;
    u8 drive_number;
    u8 reserved1;
    u8 boot_signature; /* 0x29 = extended boot info present */
    u32 volume_id;
    u8 volume_label[11];
    u8 fs_type[8]; /* "FAT12   " / "FAT16   " */
    u8 boot_code[448];
    u8 signature[2];
};
static_assert(sizeof(BootSectorFat12_16) == 512);

struct BootSectorFat32 {
    u8 jump[3];
    char oem[8];
    BpbCommon bpb;
    u32 sectors_per_fat_32;
    u16 flags;        /* bit 7-8: active FAT index (0 = mirror) */
    u16 version;      /* 0 */
    u32 root_cluster; /* 2 */
    u16 fs_info_sector;
    u16 backup_boot_sector; /* 6 */
    u8 reserved[12];
    u8 drive_number;
    u8 reserved1;
    u8 boot_signature;
    u32 volume_id;
    u8 volume_label[11];
    u8 fs_type[8]; /* "FAT32   " */
    u8 boot_code[420];
    u8 signature[2];
};
static_assert(sizeof(BootSectorFat32) == 512);

/* ---- Directory entry (0x20 bytes - corpus fixed) ----------------------- */

constexpr u8 DENTRY_DELETED = 0xE5; /* corpus: deleted marker */
constexpr u8 DENTRY_END = 0x00;
constexpr u8 ATTR_VOLUME_ID = 0x08;
constexpr u8 ATTR_DIRECTORY = 0x10; /* corpus: '.' detection */
constexpr u8 ATTR_LFN = 0x0F;       /* VFAT long-name slot */

struct DirEntry {
    u8 name[11]; /* 8.3, space padded; 0xE5 = deleted, 0x00 = end */
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
    u8 sequence;       /* bit 0x40 = last fragment, 0x20 was deleted */
    u16 name1[5];      /* ucs2 chars 1-5 */
    u8 attributes;     /* 0x0F */
    u8 type;           /* 0 */
    u8 checksum;       /* of the 8.3 name */
    u16 name2[6];      /* chars 6-11 */
    u16 first_cluster; /* 0 */
    u16 name3[2];      /* chars 12-13 */
};
static_assert(sizeof(LfnEntry) == 32);

/* FAT date/time -> unix seconds helpers (pure). */
u64 fat_date_to_unix(u16 date, u16 time, u8 tenths); /* N-173: u64 - no 2106 clamp */

/* Shared FAT-family datetime core (W4 dedup, AUDIT-FAT §6 row 20): both the
 * FAT engine (cr_lfn.cpp) and the exFAT engine (cr_exfat_quick.cpp) pack
 * dates as bits 15-9 = year-1980, 8-5 month, 4-0 day and times as 15-11
 * hour, 10-5 minute, 4-0 second/2. Returns seconds since 1970-01-01, 0 on
 * invalid/out-of-range fields. */
inline u64 fat_datetime_to_unix_seconds(u16 date, u16 time) {
    const u32 year = 1980u + ((date >> 9) & 0x7F);
    const u32 month = (date >> 5) & 0x0F;
    const u32 day = date & 0x1F;
    const u32 hour = (time >> 11) & 0x1F;
    const u32 minute = (time >> 5) & 0x3F;
    const u32 second = (time & 0x1F) * 2;
    /* A-014: the time-word gate - the og's DosDateTimeToFileTime
     * (FATSearchDLL_decompiled.c:32135-32154, the three zero-on-fail
     * slots; exFAT twin exFATSearchDLL:45174-45194) rejects hour>=24,
     * minute>=60 and seconds>=60 (2s field >= 30) and zeroes the slot;
     * the pre-fix core validated month/day only and fabricated shifted
     * plausible timestamps (hour 24..31 = +1 day..+7.5 h). This gate is
     * the exact Win32 accept set; the exFAT path is unchanged (its
     * M-014 dentry gate fat_datetime_fields_ok rejects the class
     * upstream before the shared helper). */
    if (month < 1 || month > 12 || day < 1 || day > 31)
        return 0;
    if (hour > 23 || minute > 59 || (time & 0x1F) > 29)
        return 0;
    /* days since 1970-01-01 (Howard Hinnant civil algorithm) */
    const i64 y = i64(year) - (month <= 2 ? 1 : 0);
    const i64 era = (y >= 0 ? y : y - 399) / 400;
    const u64 yoe = u64(y - era * 400);
    const u64 doy = (153u * ((month > 2 ? month - 3 : month + 9)) + 2) / 5 + day - 1;
    const u64 doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    const i64 days = era * 146097 + i64(doe) - 719468;
    if (days < 0)
        return 0;
    return u64(days) * 86400 + u64(hour) * 3600 + u64(minute) * 60 + second;
}

#pragma pack(pop)

/* Canonical chain constants (corpus IFATTable: BAD = 0xFFFFFF7, EOC >= 0xFFFFFF8). */
constexpr u32 CHAIN_BAD = 0x0FFFFFF7;
constexpr u32 CHAIN_EOC_MIN = 0x0FFFFFF8;

inline bool boot_signature_ok(const u8* sector) {
    return sector[510] == 0x55 && sector[511] == 0xAA;
}

} // namespace fat
} // namespace cr

#endif // CR_FAT_WIRE_H_
