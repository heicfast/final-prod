/*
 * cr_types.h - shared scalar types for the core_recovery stack.
 *
 * Everything here compiles identically on MSVC (C++20) and gcc/clang (C++20,
 * Linux CI). No OS headers may be included from this file or anything it
 * includes - the pure core must build on both sides (dual-verification, the
 * same methodology as rda-driver's shared headers).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_TYPES_H_
#define CR_TYPES_H_

#include <cstdint>
#include <cstddef>

namespace cr {

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

constexpr u32 u32_from_bytes_le(const u8* p) {
    return static_cast<u32>(p[0]) | (static_cast<u32>(p[1]) << 8) | (static_cast<u32>(p[2]) << 16) |
           (static_cast<u32>(p[3]) << 24);
}
constexpr u16 u16_from_bytes_le(const u8* p) {
    return static_cast<u16>(static_cast<u16>(p[0]) | (static_cast<u16>(p[1]) << 8));
}
constexpr u64 u64_from_bytes_le(const u8* p) {
    return static_cast<u64>(p[0]) | (static_cast<u64>(p[1]) << 8) | (static_cast<u64>(p[2]) << 16) |
           (static_cast<u64>(p[3]) << 24) | (static_cast<u64>(p[4]) << 32) |
           (static_cast<u64>(p[5]) << 40) | (static_cast<u64>(p[6]) << 48) |
           (static_cast<u64>(p[7]) << 56);
}

/* Times: NTFS-style 100ns since 1601, preserved exactly (R2.2). Used by
 * engines and the node graph alike - fundamental value type. */
struct FileTimes {
    u64 created = 0;
    u64 modified = 0;
    u64 changed = 0;
    u64 accessed = 0;
};

/* unix -> filetime (100ns since 1601-01-01), the props time contract
 * shared by every engine (W4 dedup, AUDIT-LINUX-FS DC-13: ext4, zfs and
 * btrfs each carried a private copy of this fold). nsec folds in at
 * 100ns granularity; pre-epoch values clamp to 0. */
inline u64 unix_to_filetime_ns(i64 unix_sec, u32 nsec) {
    constexpr i64 EPOCH_DELTA = 11644473600LL; /* 1970 -> 1601 */
    if (unix_sec <= -EPOCH_DELTA)
        return 0;
    return u64((unix_sec + EPOCH_DELTA) * 10000000LL) + u64(nsec) / 100u;
}
inline u64 unix_to_filetime(i64 unix_sec) {
    return unix_to_filetime_ns(unix_sec, 0);
}

/* Round down v to a multiple of m. (round_up was deleted in W4 - 0 uses;
 * AUDIT-ARCH 8a #13.) */
constexpr u64 round_down(u64 v, u64 m) {
    return v / m * m;
}

/* Canonical per-file FS codes (session/fs_kind wire values).
 *
 * Session-5 audit row 45: the corpus FACTORY map is the single source of
 * truth - {0,1,2} FAT12/16/32, 3 NTFS, 6 RAW, 7 exFAT, 8 HFS+, 9 EXT4,
 * 10 ReFS, 11 Btrfs, 13 ZFS. The old impl mixed the factory map with the
 * .rsf CONTENT map (Btrfs=7 collided with exFAT; ZFS sat at the content
 * map's 0xC; carved files used an invented 0xFF). Remap on session load:
 * legacy 12 (content-map ZFS) -> 13; legacy Btrfs-7 is indistinguishable
 * from exFAT-7 (documented as F16 in docs/error-fix-registry.md). */
enum class FsKindCode : u32 {
    Fat12 = 0,
    Fat16 = 1,
    Fat32 = 2,
    Ntfs = 3,
    /* OQ-S52-1 (resolved): the og's ENGINE kind 4 = the EXT2/3 FAMILY
     * (M-133's twin-key shift member; the DRWUI factory's cases 4 AND 5
     * both create the EXT2 searcher - the family code is the proven
     * member). Stamped by the ext engine per the decoded analyser rule:
     * the pre-ext4 family shapes (incompat within {FILETYPE, HAS_JOURNAL,
     * META_BG/EXT_ATTR} AND ro_compat within {SPARSE_SUPER, LARGE_FILE,
     * DIR_INDEX}); ext4 (the 0x2c0 EXTENTS|64BIT|FLEX_BG shape) stays 9. */
    Ext2 = 4,
    Raw = 6, /* carved / raw-signature files */
    Exfat = 7,
    Hfsplus = 8,
    Ext4 = 9,
    Refs = 10,
    Btrfs = 11,
    Zfs = 13,
};

} // namespace cr

#endif // CR_TYPES_H_
