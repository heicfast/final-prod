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

/* ---- M-130 (diff-to-fix): the og's dentry shape validator --------------
 * FATSearchDLL FUN_1800365a0 (:44263-44375), decoded gate-by-gate from the
 * raw decompile this session (the walk sites pass the dentry as BOTH
 * context slots [0] and [1] - verified at :8788-89 and :42944-45 - so
 * every field below is read at its standard dentry offset).
 *
 * The validator runs per 32-byte slot BEFORE the LFN/deleted dispatch at
 * the main walk sites; the two og walks apply different subsets:
 *   - the directory walks (:8792 in FUN_18000bfa0, :53647 in
 *     FUN_1800411d0) call with param_2 = 0: a failing gate BREAKS the
 *     walk (end-of-directory semantics - "parity means matching the
 *     loss");
 *   - the emission walk (:42948 in FUN_180034db0) calls with param_2 = 1:
 *     the size/chain and dir gates run there; a failure SKIPS the entry
 *     (not emitted - the walk continues).
 * The date/time sub-gates are the N-010c-lineage config family
 * (strict_date_sanity, default OFF) plus the A-014 emission-level
 * time-word zeroing - NOT part of these always-on gates. */

/* The walk-level gates (the param_2 = 0 set). Returns true when the slot
 * may continue through the walk (LFN slots included). */
inline bool fat_dentry_walk_ok(const DirEntry* e) {
    const u8 attr = e->attributes;
    /* (a) the LFN gate: an 0x0F slot with a ZERO first-cluster word is a
     * valid long-name fragment (FUN_1800365a0 :44286-89). */
    if (attr == ATTR_LFN && e->first_cluster_low == 0)
        return true;
    /* reserved attribute bits (>= 0x40) - the :44295-96 compare is on the
     * u32 at +0x08 whose top byte IS the attribute byte (the audit's
     * "ext-tail" attribution corrected: 0x3F < the ATTRIBUTES byte). */
    if (attr > 0x3F)
        return false;
    /* the erased/repeated-pattern gate (:44293-94): the extension+attr
     * quad equal to the ntres/tenths/creation-time quad (and not 0x20)
     * - the all-zero dentry and the uniform-byte garbage shapes. */
    const u32 ext_quad =
        u32(e->name[8]) | (u32(e->name[9]) << 8) | (u32(e->name[10]) << 16) | (u32(attr) << 24);
    const u32 crt_quad =
        u32(e->nt_reserved) | (u32(e->creation_tenths) << 8) | (u32(e->creation_time) << 16);
    if (ext_quad == crt_quad && crt_quad != 0x20)
        return false;
    /* label purity (:44298-44301): an entry with the volume bit set must
     * be a PURE label - attr in {0x08, 0x28} with the hidden bit clear
     * ((attr - 8) & 0xDF == 0 admits only the archive bit beside 8) -
     * otherwise it is corrupt (e.g. an 0x0F slot with a stray cluster,
     * a dir+label mix) and the walk stops. */
    const bool label_pure = (attr & 0x02) == 0 && ((attr - 0x08) & 0xDF) == 0;
    if ((attr & ATTR_VOLUME_ID) && !label_pure)
        return false;
    /* (f) the byte histogram (:44356-69): over the 21-byte metadata
     * window +0x0B..+0x1F, counting NONZERO bytes; the 11th occurrence
     * of any value rejects (the pre-increment compare is > 9). The main
     * guard against erased-tail patterns (11x 0xFF) and stale sectors. */
    u8 hist[256] = {};
    const u8* win = reinterpret_cast<const u8*>(e) + 0x0B;
    for (int i = 0; i < 21; ++i) {
        const u8 b = win[i];
        if (b != 0) {
            if (hist[b] > 9)
                return false;
            ++hist[b];
        }
    }
    return true;
}

/* The emission-level shape gates (the param_2 = 1 set, :44344-52). */
inline bool fat_dentry_shape_ok(const DirEntry* e) {
    if ((e->attributes & ATTR_DIRECTORY) == 0) {
        /* (d) size/chain XOR consistency: a zero-size file with a chain,
         * or a sized file with no chain, is inconsistent - reject. */
        const u32 chain = (u32(e->first_cluster_high) << 16) | e->first_cluster_low;
        if ((chain != 0) == (e->file_size == 0))
            return false;
    } else {
        /* (e) a directory with size != 0 AND a nonzero high cluster word
         * - the corrupt-dir shape. */
        if (e->file_size != 0 && e->first_cluster_high != 0)
            return false;
    }
    return true;
}

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
 * invalid/out-of-range fields. A-014: the invalid class is the FULL
 * DosDateTimeToFileTime reject set - month 0/13+, day 0/32+, hour >= 24,
 * minute >= 60, 2s-field >= 30 (>= 60 s) - matching the og's zero-on-fail
 * slots (FATSearchDLL :32135/:32144/:32151, the three dentry stamps) and
 * the exFAT sibling's own field gate (exFATSearchDLL FUN_180002f40). */
inline u64 fat_datetime_to_unix_seconds(u16 date, u16 time) {
    const u32 year = 1980u + ((date >> 9) & 0x7F);
    const u32 month = (date >> 5) & 0x0F;
    const u32 day = date & 0x1F;
    const u32 hour = (time >> 11) & 0x1F;
    const u32 minute = (time >> 5) & 0x3F;
    const u32 second = (time & 0x1F) * 2;
    if (month < 1 || month > 12 || day < 1 || day > 31 || hour > 23 || minute > 59 ||
        (time & 0x1F) > 29)
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
