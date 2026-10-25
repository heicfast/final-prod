/* cr_chain.cpp - FatTable + chain walker (R5.1).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/fat/cr_fat_engine.h"
#include <cstring>

namespace cr {
namespace fat {

Flavor flavor_from_clusters(u32 cluster_count_data) {
    /* og brackets (FATSearchDLL FUN_180007e50 :5219-5231, N-010a):
     * FAT12 [0, 0xFF5), FAT16 [0xFF5, 0xFFF4), FAT32 [0xFFF4, ...) -
     * 0xFFF4 = 65524 data clusters is FAT32 (the old `< 65525` boundary
     * made it FAT16; one-cluster-window divergence on the boundary). */
    if (cluster_count_data < 4085)
        return Flavor::Fat12;
    if (cluster_count_data < 65524)
        return Flavor::Fat16;
    return Flavor::Fat32;
}

u64 FatTable::cluster_offset(const FatParams& p, u32 cluster) {
    if (cluster < 2)
        return 0;
    return (u64(cluster) - 2) * p.cluster_bytes + (p.first_data_sector * p.bytes_per_sector);
}

u32 FatTable::entry_at(const std::vector<u8>& f, u32 cluster) const {
    if (f.empty())
        return 0;
    switch (flavor_) {
    case Flavor::Fat12: {
        size_t off = size_t(cluster) + size_t(cluster) / 2;
        if (off + 2 > f.size())
            return 0;
        u32 v = u16_from_bytes_le(f.data() + off);
        if (cluster & 1)
            v >>= 4;
        else
            v &= 0xFFF;
        /* canonicalize the 12-bit sentinels exactly like the FAT16/32
         * branches: 0xFF7 = bad cluster, >= 0xFF8 = end of chain
         * (PARITY-AUDIT F-A1-1: without this, every FAT12 chain ended
         * "broken" — the raw 0xFFF EOC value is < CHAIN_EOC_MIN). */
        if (v == 0xFF7)
            return CHAIN_BAD;
        if (v >= 0xFF8)
            return CHAIN_EOC_MIN;
        return v;
    }
    case Flavor::Fat16: {
        size_t off = size_t(cluster) * 2;
        if (off + 2 > f.size())
            return 0;
        u16 v = u16_from_bytes_le(f.data() + off);
        if (v == 0xFFF7)
            return CHAIN_BAD;
        if (v >= 0xFFF8)
            return CHAIN_EOC_MIN; /* canonical EOC marker */
        return v;
    }
    case Flavor::Fat32:
    default: {
        size_t off = size_t(cluster) * 4;
        if (off + 4 > f.size())
            return 0;
        const u32 raw = u32_from_bytes_le(f.data() + off);
        u32 v = raw & 0x0FFFFFFF;
        if (v == 0x0FFFFFF7)
            return CHAIN_BAD;
        if (v >= 0x0FFFFFF8)
            return CHAIN_EOC_MIN;
        /* N-002 (diff-to-fix): the og's tolerant mask-24 EOC family
         * (FATSearchDLL Next, :8030-8048): `(next & 0x07FFFFF8) ==
         * 0x07FFFFF8 -> EOC` - BIOS/embedded formatters write EOC variants
         * with dirty high bits (0x27FFFFF8, 0xFFFFFFF8, ...) that the og
         * accepts; the mask-24 test also subsumes the og's raw
         * `entry == 0xFFFFFFF8` form. Single-entry approximation: the og's
         * two-slot alternate-family forms collapse into the reserved-range
         * branch below (values 0x0FFFFFF0..F7 already end the chain). */
        if ((raw & 0x07FFFFF8) == 0x07FFFFF8)
            return CHAIN_EOC_MIN;
        return v;
    }
    }
}

u32 FatTable::load(IStorageDevice* dev, const FatParams& p) {
    flavor_ = p.flavor;
    max_cluster_ = p.cluster_count + 1; /* clusters 2..cluster_count+1 */
    u64 table_bytes = u64(p.fat_sectors) * p.bytes_per_sector;
    if (table_bytes == 0 || table_bytes > (16u << 20))
        return ERR_BAD_PARAMETER;
    fat1_.resize(size_t(table_bytes));
    if (dev->read_at(p.fat_start, fat1_.data(), u32(table_bytes)) != OK) {
        fat1_.clear();
        return ERR_READ_FAILED;
    }
    if (p.fat_count >= 2 && p.fat2_start != 0) {
        fat2_.resize(size_t(table_bytes));
        if (dev->read_at(p.fat2_start, fat2_.data(), u32(table_bytes)) != OK) {
            fat2_.clear(); /* mirror unreadable: single-FAT mode */
        } else {
            mirror_ = true;
        }
    }
    return OK;
}

namespace {
/* The og's SPC acceptance set (N-010b/N-034): engine-open validator
 * FUN_18002f970 :38050-52 tests `spc-1` against the bitmask
 * 0x800000008000808b (bits 0,1,3,7,15,31,63 -> spc 1,2,4,8,16,32,64)
 * plus the separate `spc == 0x80` special case; the PartitionRecoveryD
 * deep gates enumerate the same set {1,2,4,8,0x10,0x20,0x40,0x80}. */
bool spc_in_og_set(u8 spc) {
    switch (spc) {
    case 1:
    case 2:
    case 4:
    case 8:
    case 16:
    case 32:
    case 64:
    case 128:
        return true;
    default:
        return false;
    }
}

/* case-insensitive 8-byte FS-name compare (the og's _strnicmp at
 * FUN_18002f970 :38048/:38062). */
bool fs_name_matches(const u8* at, const char* want) {
    for (int i = 0; i < 8; ++i) {
        u8 a = at[i];
        if (a >= u8('A') && a <= u8('Z'))
            a = u8(a - u8('A') + u8('a'));
        if (a != u8(want[i]))
            return false;
    }
    return true;
}
} // namespace

u32 FatTable::next(u32 cluster) const {
    if (cluster < 2 || cluster > max_cluster_)
        return 0;
    u32 v = entry_at(fat1_, cluster);
    /* N-022: mirror consult on primary FREE only (og parity); the
     * primary's BAD answer is served directly (chain truncates). */
    if (mirror_ && v == 0) {
        u32 alt = entry_at(fat2_, cluster);
        if (alt != 0 && alt != CHAIN_BAD)
            return alt;
    }
    if (mirror_ && v == CHAIN_BAD && mirror_bad_salvage_) {
        /* opt-in torn-pair salvage (recovery mode, NOT the og default) */
        u32 alt = entry_at(fat2_, cluster);
        if (alt != 0 && alt != CHAIN_BAD)
            return alt;
    }
    /* N-010 (diff-to-fix, discovered by I-002's pin test): the BAD sentinel
     * (0x0FFFFFF7) sits inside the 0x0FFFFFF0..F7 reserved window by
     * construction - it must be returned BEFORE the reserved-range test, or
     * every BAD-terminated chain grades as a clean end (the og canonicalizes
     * 0xFFF7 -> 0xFFFFFF7 -> out of range -> chain BROKEN, :54125). */
    if (v == CHAIN_BAD)
        return CHAIN_BAD;
    if (v >= CHAIN_EOC_MIN && v < 0x0FFFFFF0)
        return CHAIN_EOC_MIN; /* canonical marker */
    if (v >= 0x0FFFFFF0) {
        /* FAT12 0xFF0-0xFF6 / FAT16 0xFFF0-0xFFF6 / FAT32 reserved: treat as
         * reserved-value -> end of chain for walking purposes */
        return CHAIN_EOC_MIN;
    }
    if (v == 0)
        return v;
    return v;
}

u32 FatTable::raw(u32 cluster) const {
    if (cluster < 2 || cluster > max_cluster_)
        return 0;
    u32 v = entry_at(fat1_, cluster);
    /* N-022: same consult policy as next() - primary FREE consults the
     * mirror; primary BAD is served raw unless salvage is opted in. */
    if (mirror_ && v == 0) {
        u32 alt = entry_at(fat2_, cluster);
        if (alt != 0 && alt != CHAIN_BAD)
            return alt;
    }
    if (mirror_ && v == CHAIN_BAD && mirror_bad_salvage_) {
        u32 alt = entry_at(fat2_, cluster);
        if (alt != 0 && alt != CHAIN_BAD)
            return alt;
    }
    return v;
}

ChainResult follow_chain(const FatTable& t, u32 start, u32 max_len, const CancelToken* cancel) {
    ChainResult r;
    if (start < 2 || start > t.max_cluster()) {
        r.broken = true;
        return r;
    }
    std::vector<u64> visited; /* visited clusters (cycle guard, fix F7) */
    u32 c = start;
    u32 guard = 0;
    while (c >= 2 && c <= t.max_cluster()) {
        /* cancel poll at every chain step (F-A1-11, og cadence) */
        if (cancel && cancel->checkpoint()) {
            r.cancelled = true;
            return r;
        }
        if (++guard > max_len) {
            r.cycle = true;
            break;
        }
        for (u64 v : visited)
            if (v == c) {
                r.cycle = true;
                return r;
            }
        visited.push_back(c);
        r.clusters.push_back(c);
        u32 n = t.next(c);
        if (n == 0 || n == CHAIN_BAD) {
            r.broken = (n == CHAIN_BAD);
            break;
        }
        if (n >= CHAIN_EOC_MIN)
            break; /* clean end */
        if (n < 2 || n > t.max_cluster()) {
            r.broken = true;
            break;
        }
        c = n;
    }
    return r;
}

ChainResult reconstruct_deleted_chain(const FatTable& t, u32 start, u32 expect_clusters,
                                      const CancelToken* cancel) {
    /* For a deleted entry: FAT links may be gone (0) or intact. Walk while
     * entries are plausible; treat FREE mid-chain as end (salvage what's
     * linked). Expect bounds the walk. */
    ChainResult r;
    if (start < 2 || start > t.max_cluster()) {
        r.broken = true;
        return r;
    }
    std::vector<u64> visited;
    u32 c = start;
    u32 guard = 0;
    u32 limit = expect_clusters ? expect_clusters : 0x10000;
    while (c >= 2 && c <= t.max_cluster() && guard < limit) {
        /* cancel poll at every chain step (F-A1-11, og cadence) */
        if (cancel && cancel->checkpoint()) {
            r.cancelled = true;
            return r;
        }
        ++guard;
        for (u64 v : visited)
            if (v == c) {
                r.cycle = true;
                return r;
            }
        visited.push_back(c);
        r.clusters.push_back(c);
        if (r.clusters.size() >= limit)
            break;
        u32 n = t.next(c);
        if (n >= CHAIN_EOC_MIN)
            break;
        if (n == 0 || n == CHAIN_BAD || n < 2 || n > t.max_cluster()) {
            r.broken = (n == CHAIN_BAD);
            break;
        }
        c = n;
    }
    return r;
}

ChainResult reconstruct_contiguous_chain(const FatTable& t, u32 start, u32 expect_clusters,
                                         const CancelToken* cancel) {
    /* GAP-S6-2: unlinked-chain contiguous candidate - see the header comment
     * for the full rationale. Only applies with a real size expectation; the
     * run is validated cluster-by-cluster against the FAT free-state and
     * truncated at the first non-FREE cluster (honest partial prefix). */
    ChainResult r;
    if (start < 2 || start > t.max_cluster() || expect_clusters == 0)
        return r;
    u32 n = expect_clusters;
    if (u64(start) + u64(n) - 1 > t.max_cluster())
        n = u32(t.max_cluster() - start + 1);
    for (u32 c = start; c < start + n; ++c) {
        /* cancel poll at every candidate step (F-A1-11 cadence) */
        if (cancel && cancel->checkpoint()) {
            r.cancelled = true;
            return r;
        }
        if (t.raw(c) != 0)
            break; /* allocated/linked inside the candidate run: cut here */
        r.clusters.push_back(c);
    }
    if (!r.clusters.empty())
        r.inferred = true;
    return r;
}

u32 validate_boot(const u8* sector512, u64 device_bytes, FatParams& out) {
    if (sector512 == nullptr || !fat::boot_signature_ok(sector512))
        return ERR_BAD_MAGIC;
    FatParams p;
    p.bytes_per_sector = u16_from_bytes_le(sector512 + 0x0B);
    if (p.bytes_per_sector < 512 || p.bytes_per_sector > 4096 ||
        (p.bytes_per_sector & (p.bytes_per_sector - 1)) != 0)
        return ERR_BAD_PARAMETER;
    p.sectors_per_cluster = sector512[0x0D];
    /* N-010b/N-034 (diff-to-fix, corrected this session): SPC is a u8
     * field, so the og's set {1,2,4,8,16,32,64,128} (engine-open bitmask
     * 0x800000008000808b + the 0x80 special case; the deep gates
     * enumerate the same set) is exactly the pow2 subset of the domain -
     * the old any-pow2 check was behaviorally identical. The explicit
     * set states the og gate and pins it against future loosening. */
    if (!spc_in_og_set(p.sectors_per_cluster))
        return ERR_BAD_PARAMETER;
    p.cluster_bytes = u32(p.sectors_per_cluster) * p.bytes_per_sector;
    p.reserved_sectors = u16_from_bytes_le(sector512 + 0x0E);
    p.fat_count = sector512[0x10];

    p.root_entries = u16_from_bytes_le(sector512 + 0x11);
    u32 total16 = u16_from_bytes_le(sector512 + 0x13);
    u32 total32 = u32_from_bytes_le(sector512 + 0x20);
    u32 total = total16 ? total16 : total32;

    u32 spf16 = u16_from_bytes_le(sector512 + 0x16);
    u32 spf32 = u32_from_bytes_le(sector512 + 0x24);

    p.fat_start = u64(p.reserved_sectors) * p.bytes_per_sector;
    /* FAT flavor first guess by root_entries/spf shape (final by clusters) */
    if (spf16 != 0 && p.root_entries != 0) {
        p.flavor = Flavor::Fat16; /* refined below */
        p.fat_sectors = spf16;
    } else {
        p.flavor = Flavor::Fat32;
        p.fat_sectors = spf32;
        p.root_cluster = u32_from_bytes_le(sector512 + 0x2C);
    }

    u64 fat_area = u64(p.fat_count) * p.fat_sectors;
    u32 root_sectors = 0;
    if (p.flavor != Flavor::Fat32) {
        root_sectors = (p.root_entries * 32 + p.bytes_per_sector - 1) / p.bytes_per_sector;
        p.root_dir_offset = (u64(p.reserved_sectors) + fat_area) * p.bytes_per_sector;
    } else {
        /* root dir is a cluster chain; first_data computed below */
        p.root_dir_offset = 0;
    }
    p.first_data_sector = u64(p.reserved_sectors) + fat_area + root_sectors; /* P1-15: u64 */
    if (p.first_data_sector >= total)
        return ERR_BAD_PARAMETER;
    const u32 data_sectors = total - u32(p.first_data_sector);
    p.cluster_count = data_sectors / p.sectors_per_cluster;

    p.flavor = flavor_from_clusters(p.cluster_count);
    p.fat2_start = p.fat_count >= 2 ? p.fat_start + u64(p.fat_sectors) * p.bytes_per_sector : 0;
    p.total_bytes = u64(total) * p.bytes_per_sector;
    if (p.total_bytes > device_bytes) {
        /* partition slice smaller than BPB claims (truncated tail): allow,
         * the transport bounds-checks every access */
    }
    out = p;
    return OK;
}

u32 boot_passes_open_gates(const u8* sector512, u32 device_sector, const FatParams& p) {
    /* N-010b (diff-to-fix): the og's engine-open boot validator
     * (FATSearchDLL FUN_18002f970 :38037-38090, consumed by the quick
     * search at FUN_180021000 :26318 which bails on flavor 0xE).
     * Everything below is decoded verbatim from that function. */
    if (sector512 == nullptr)
        return ERR_BAD_PARAMETER;
    /* numFATs in {1,2} (:38049 `(byte)(numFATs - 1) < 2`) */
    if (p.fat_count != 1 && p.fat_count != 2)
        return ERR_BAD_PARAMETER;
    /* bps == device sector size (:38055) - skipped when the device
     * geometry is unknown (0). */
    if (device_sector != 0 && p.bytes_per_sector != device_sector)
        return ERR_BAD_PARAMETER;
    switch (p.flavor) {
    case Flavor::Fat16:
        /* "FAT16   " at +0x36, case-insensitive (:38048) */
        if (!fs_name_matches(sector512 + 0x36, "fat16   "))
            return ERR_BAD_PARAMETER;
        /* root_entries >= 224 (:38054 `0xdf < root`) and
         * root_entries*32 % bps == 0 (:38055) */
        if (p.root_entries < 224)
            return ERR_BAD_PARAMETER;
        if ((u64(p.root_entries) * 32) % p.bytes_per_sector != 0)
            return ERR_BAD_PARAMETER;
        /* spf16 != 0 (:38056) */
        if (u16_from_bytes_le(sector512 + 0x16) == 0)
            return ERR_BAD_PARAMETER;
        break;
    case Flavor::Fat32:
        /* "FAT32   " at +0x52, case-insensitive (:38062) */
        if (!fs_name_matches(sector512 + 0x52, "fat32   "))
            return ERR_BAD_PARAMETER;
        /* spf32 != 0 (the :38062 block's `*(int*)(lVar2+0x24) == 0` reject) */
        if (u32_from_bytes_le(sector512 + 0x24) == 0)
            return ERR_BAD_PARAMETER;
        break;
    case Flavor::Fat12:
        /* Deliberate core superset (I-002 family, PARITY-DELTAS): the
         * og's validator has no FAT12 branch and declines such volumes
         * at quick-engine open; the core keeps structural FAT12
         * recovery. No string/geometry gate here. */
        break;
    }
    return OK;
}

int lost_boot_flavor(const u8* sector512) {
    /* N-034 (diff-to-fix): the og PartitionRecoveryD deep-scan ladder
     * (FUN_180004610 :2516-2606 + gates FUN_1800048f0/4a40/4b70).
     * Returns 1 = FAT12, 2 = FAT16, 3 = FAT32 (the og's candidate type
     * codes), 0 = no gate passed. */
    if (sector512 == nullptr)
        return 0;
    /* every deep gate requires the 0x55AA signature (checked in-gate on
     * the og: :2634/:2711 etc.). No first-byte/jump check - the og's
     * deep gates never look at the jump bytes, so a torn-jump volume
     * with an intact BPB stays a candidate. */
    if (sector512[0x1FE] != 0x55 || sector512[0x1FF] != 0xAA)
        return 0;
    const u16 bps = u16_from_bytes_le(sector512 + 0x0B);
    const u8 spc = sector512[0x0D];
    const u8 num_fats = sector512[0x10];
    const u16 reserved = u16_from_bytes_le(sector512 + 0x0E);
    const u16 total16 = u16_from_bytes_le(sector512 + 0x13);
    const u32 total32 = u32_from_bytes_le(sector512 + 0x20);
    const u16 spf16 = u16_from_bytes_le(sector512 + 0x16);
    const u32 spf32 = u32_from_bytes_le(sector512 + 0x24);
    const u16 root_entries = u16_from_bytes_le(sector512 + 0x11);

    /* bps must be a 512 multiple (:2547 `(bps & 0x1ff) == 0`); bps==0 or
     * spc==0 are the ladder's own 0xFF rejects (:2538-2540). spc must be
     * in the gate set (every gate's enumeration). */
    if (bps == 0 || (bps & 0x1FF) != 0 || !spc_in_og_set(spc))
        return 0;
    /* total: total32 wins when nonzero, else total16 (:2531-2535) */
    const u32 total = total32 != 0 ? total32 : u32(total16);

    /* cluster count exactly like the ladder's arithmetic (:2549-2556):
     * (total - reserved - spf*numFATs - floor(root*32/bps)) / spc, where
     * spf is spf16 when nonzero else spf32 (:2525-2530). The og computes
     * this in wrapping uint arithmetic; an underflowing (corrupt) BPB is
     * rejected here instead - same outcome for every sane input. */
    const u64 spf = spf16 != 0 ? spf16 : spf32;
    const u64 root_sectors = (u64(root_entries) * 32) / bps; /* floor, og form */
    if (total <= u64(reserved) + spf * num_fats + root_sectors)
        return 0;
    const u32 clusters = u32((total - u64(reserved) - spf * num_fats - root_sectors) / spc);

    /* the three gates, exactly as decoded (see header comment) */
    const bool fat12_gate =
        num_fats != 0 && (total16 != 0 || total32 != 0) && spf16 >= 1 && spf16 <= 200;
    const bool fat16_gate = num_fats != 0 && (total16 != 0 || total32 != 0) && spf16 != 0;
    const bool fat32_gate = std::memcmp(sector512 + 0x52, "FAT32   ", 8) == 0 && num_fats != 0 &&
                            total32 != 0 && spf32 != 0;

    const int order_small[3] = {1, 2, 3}; /* < 0xFF5:  12, 16, 32 */
    const int order_mid[3] = {2, 3, 1};   /* < 0xFFF5: 16, 32, 12 */
    const int order_large[3] = {3, 2, 1}; /* < 0xFFFFFF0: 32, 16, 12 */
    const int* order =
        clusters < 0xFF5 ? order_small : (clusters < 0xFFF5 ? order_mid : order_large);
    if (clusters >= 0xFFFFFF0)
        return 0;
    for (int k = 0; k < 3; ++k) {
        switch (order[k]) {
        case 1:
            if (fat12_gate)
                return 1;
            break;
        case 2:
            if (fat16_gate)
                return 2;
            break;
        case 3:
            if (fat32_gate)
                return 3;
            break;
        }
    }
    return 0;
}

} // namespace fat
} // namespace cr
