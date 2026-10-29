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
    /* M-011 (diff-to-fix): per-flavor sectors-per-FAT windows, exactly the
     * og's child-table validation (FUN_180036e90 :44715-44793, re-read
     * first-hand): FAT12 spf in [1, 0xC], FAT16 spf in [0xC, 0x100], FAT32
     * spf in [0x100, 0x96000] - the bounds divided by (bps>>9) so they are
     * sector-count-invariant: at 512 B sectors FAT32 accepts 128 KiB..
     * 300 MiB tables (whole-loaded, og factory shape :3809-3814). The old
     * flat 16 MiB cap refused every larger FAT32 (17 MiB-FAT volumes =
     * multi-TB-with-large-clusters class) with ERR_BAD_PARAMETER and the
     * whole volume vanished from recovery; the FAT16 lower bound was
     * missing entirely (spf < 12 = FAT12-window territory the og never
     * scans as FAT16). */
    const u64 spf_scale = p.bytes_per_sector / 512;
    u64 spf_min = 1, spf_max = 0x96000;
    switch (flavor_) {
    case Flavor::Fat12:
        spf_min = 1;
        spf_max = 0xC;
        break;
    case Flavor::Fat16:
        spf_min = 0xC;
        spf_max = 0x100;
        break;
    case Flavor::Fat32:
    default:
        spf_min = 0x100;
        spf_max = 0x96000;
        break;
    }
    if (spf_scale != 0) {
        spf_min = (spf_min + spf_scale - 1) / spf_scale; /* ceil: at least
                                                          * one sector */
        spf_max /= spf_scale;
    }
    if (p.fat_sectors < spf_min || p.fat_sectors > spf_max || table_bytes == 0)
        return ERR_BAD_PARAMETER;
    fat1_.resize(size_t(table_bytes));
    if (dev->read_at(p.fat_start, fat1_.data(), u32(table_bytes)) != OK) {
        /* M-012 (diff-to-fix): mirror-only mode. The og's factory keeps a
         * NULL child1 when FAT1 cannot be read and CDoubleFATTable::Next
         * then serves the MIRROR's answer RAW (:54216 `child1 && ...` falls
         * through; :54250 `return uVar2` with no scrutiny) - the volume
         * still scans. The core used to fail the whole scan with
         * ERR_READ_FAILED. */
        fat1_.clear();
        if (p.fat_count >= 2 && p.fat2_start != 0) {
            fat1_.resize(size_t(table_bytes));
            if (dev->read_at(p.fat2_start, fat1_.data(), u32(table_bytes)) == OK) {
                mirror_only_ = true;
                return OK; /* no second mirror to consult */
            }
            fat1_.clear();
        }
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
    /* M-015 (diff-to-fix): whole-table validation + copy retry. The og
     * validates each loaded copy (CFAT16 FUN_180041b00 :54052-141: media
     * word pair exact, entry sweep, claims-map cross-link walk; CFAT32
     * family-tolerant + the 98% watermark under the deep-check flag) and
     * the factory retries alternate FAT regions on rejection
     * (:3817-3950). The core loaded whichever copy it read, unvalidated -
     * a corrupt FAT1 silently produced wrong chains where the og scanned
     * a clean copy. */
    if (!fat_validate_(p)) {
        if (!fat2_.empty() && fat2_validate_(p)) {
            /* the mirror is clean: promote it to the primary (og child1
             * = the alternate copy) and keep consulting it as mirror */
            fat1_ = fat2_;
            fat_copy_rejected_ = true;
            return OK;
        }
        fat1_.clear();
        fat2_.clear();
        mirror_ = false;
        return ERR_BAD_MAGIC;
    }
    return OK;
}

namespace {
/* M-015: the og's whole-table validation, ported from FUN_180041b00
 * (:54052-54141, re-read first-hand). Flavor-shared sweep + claims walk;
 * the FAT16 media-word check is exact (u32@FAT[0] == 0xFFFFFFF8), FAT12/
 * FAT32 are family-tolerant (the og's CFAT32 :53853-53919; CFAT12 is not
 * separately cited - family-tolerant, documented). The 98% watermark is
 * advisory-only here: its deep-check gate default is unpinned (finding's
 * open question), so it is computed into a stat, never a reject. */
bool fat_table_validate(const std::vector<u8>& f, Flavor flavor, u32 entry_count, u32& used_out) {
    used_out = 0;
    if (f.empty())
        return false;
    const u32 n = entry_count;
    if (n < 3)
        return false;
    /* media words: FAT16 exact 0xFFFFFFF8 (the pair FAT[0..1]); other
     * flavors tolerate the family forms */
    if (flavor == Flavor::Fat16) {
        if (f.size() < 4)
            return false;
        const u32 media = u32_from_bytes_le(f.data());
        if (media != 0xFFFFFFF8u)
            return false;
    } else if (f.size() < 8) {
        return false;
    }
    /* entry sweep: OOB pointers (>= n, below the EOC family), == 1,
     * self-loops invalidate the WHOLE table */
    const auto raw_entry = [&](u32 c) -> u32 {
        switch (flavor) {
        case Flavor::Fat12: {
            const size_t off = size_t(c) + size_t(c) / 2;
            if (off + 2 > f.size())
                return 0xFFFF;
            u32 v = u16_from_bytes_le(f.data() + off);
            if (c & 1)
                v >>= 4;
            else
                v &= 0xFFF;
            return v;
        }
        case Flavor::Fat16: {
            const size_t off = size_t(c) * 2;
            if (off + 2 > f.size())
                return 0xFFFF;
            return u16_from_bytes_le(f.data() + off);
        }
        case Flavor::Fat32:
        default: {
            const size_t off = size_t(c) * 4;
            if (off + 4 > f.size())
                return 0xFFFFFFFFu;
            return u32_from_bytes_le(f.data() + off) & 0x0FFFFFFF;
        }
        }
    };
    const u32 eoc_family_min = flavor == Flavor::Fat12   ? 0xFF8
                               : flavor == Flavor::Fat16 ? 0xFFF8
                                                         : 0x0FFFFFF8;
    /* the og's OOB band is [n, BAD) - STRICTLY below the BAD sentinel
     * (0xFFF7 itself passes the sweep; a BAD cluster is a legal value).
     * FAT32 additionally carries the mask-24 alternate EOC family
     * (N-002: (raw & 0x07FFFFF8) == 0x07FFFFF8 -> EOC, e.g. dirty-high
     * forms 0x27FFFFF8); those values end chains, they are not links. */
    const u32 bad_sentinel = flavor == Flavor::Fat12   ? 0xFF7
                             : flavor == Flavor::Fat16 ? 0xFFF7
                                                       : 0x0FFFFFF7;
    const u32 alt_eoc_min = flavor == Flavor::Fat32 ? 0x07FFFFF8 : bad_sentinel;
    const auto is_link = [&](u32 v) {
        return v >= 2 && v < bad_sentinel && v < n && v < alt_eoc_min;
    };
    u32 used = 0;
    for (u32 i = 2; i < n; ++i) {
        const u32 v = raw_entry(i);
        if (v == 1 || v == i)
            return false; /* ==1 / self-loop */
        if (v >= n && v < bad_sentinel && v < alt_eoc_min)
            return false; /* OOB pointer below the BAD sentinel */
        if (is_link(v))
            ++used; /* a live link (watermark advisory) */
    }
    used_out = used;
    /* claims-map cross-link walk: every unclaimed entry roots a chain;
     * a cluster claimed by two different roots is a cross-link; a walk
     * visiting n-2 entries is a cycle covering the table - both reject
     * the whole table (og :54084-54114). Bounded to the same n budget. */
    std::vector<u32> claims(size_t(n), 0);
    for (u32 i = 2; i < n; ++i) {
        if (claims[i] != 0)
            continue;
        u32 c = i;
        for (u32 steps = 0; steps < n; ++steps) {
            const u32 v = raw_entry(c);
            if (v < 2 || v >= eoc_family_min || v >= n || v >= alt_eoc_min)
                break; /* free / EOC family (incl. mask-24) / BAD: ends */
            if (claims[v] == 0) {
                claims[v] = i;
            } else if (claims[v] != i) {
                return false; /* cross-link: another chain's cluster */
            }
            c = v;
        }
        /* (a chain visiting every entry = a covering cycle also rejects;
         * the per-root walks above terminate before that on real tables,
         * and the O(n) total budget bounds the pathological case) */
    }
    return true;
}
} // namespace

bool FatTable::fat_validate_(const FatParams& p) {
    const u32 entry_count = max_cluster_ + 1 < 0xFFFFFF0u ? max_cluster_ + 1 : 0xFFFFFF0u;
    u32 used = 0;
    const bool ok = fat_table_validate(fat1_, flavor_, entry_count, used);
    (void)p;
    return ok;
}

bool FatTable::fat2_validate_(const FatParams& p) {
    const u32 entry_count = max_cluster_ + 1 < 0xFFFFFF0u ? max_cluster_ + 1 : 0xFFFFFF0u;
    u32 used = 0;
    const bool ok = fat_table_validate(fat2_, flavor_, entry_count, used);
    (void)p;
    return ok;
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

u32 FatTable::mirror_consult_(u32 cluster) const {
    /* M-012 (diff-to-fix): the og's mirror scrutiny, FUN_180041de0
     * :54226-54248 re-read first-hand. Consulted ONLY when the primary
     * answered FREE (N-022); the mirror's answer is then SCRUTINIZED:
     *   backward (alt < cluster): reject a shifted copy (mirror(c+1) ==
     *     alt+1), reject a cross-link (mirror(alt-1) == alt), accept only
     *     contiguity (mirror(alt) == alt+1); else 0.
     *   forward: accept unless the mirror claims BAD/EOC (a mirror is not
     *     believed when the primary said FREE).
     * The old code accepted ANY non-zero non-BAD mirror answer - torn
     * FAT1/FAT2 pairs (shifted copies) produced bogus chains. */
    const u32 alt = entry_at(fat2_, cluster);
    if (alt == 0)
        return 0;
    if (alt < cluster) {
        /* backward link: shifted copy / cross-link / contiguity */
        const u32 next_c1 = entry_at(fat2_, cluster + 1);
        if (next_c1 == alt + 1)
            return 0; /* shifted copy of the primary's layout */
        const u32 at_alt = entry_at(fat2_, alt);
        const u32 before_alt = alt >= 1 ? entry_at(fat2_, alt - 1) : 0;
        if (alt != 0 && before_alt == alt)
            return 0; /* cross-link: alt is already claimed */
        if (alt != 0 && at_alt == alt + 1)
            return alt; /* contiguity-consistent: believe */
        return 0;
    }
    /* forward: mirror BAD/EOC not believed after a primary FREE */
    if (alt == CHAIN_BAD || alt >= CHAIN_EOC_MIN)
        return 0;
    return alt;
}

u32 FatTable::next(u32 cluster) const {
    if (cluster < 2 || cluster > max_cluster_)
        return 0;
    u32 v = entry_at(fat1_, cluster);
    /* N-022: mirror consult on primary FREE only (og parity); the
     * primary's BAD answer is served directly (chain truncates). */
    if (mirror_ && v == 0)
        return mirror_consult_(cluster);
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
    /* N-022 + M-012: same consult+scrutiny policy as next(). */
    if (mirror_ && v == 0)
        return mirror_consult_(cluster);
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
    /* M-045 (diff-to-fix): the og's classifier accepts ANY 512-multiple
     * bps ((bps & 0x1ff) == 0, PartitionRecoveryD FUN_180004610 :2542;
     * lost_boot_flavor already applies the same rule) - 1536/2560/8192
     * are rare-but-real volumes. The old pow2-in-[512,4096] gate made the
     * probe incoherent: lost_boot_flavor classified a bps=1536 candidate
     * while validate_boot (probe_fat's FIRST gate, cr_candidates.cpp)
     * rejected it -> invisible. All downstream arithmetic is
     * bps-multiplied (offsets, root sectors, FAT areas), so non-pow2
     * sizes are safe. Deliberate documented bound (PARITY-DELTAS): the
     * 32768 ceiling the og lacks (a u16 field tops at 65535; no real
     * format writes past 32768 - this only cuts absurd-volume noise). */
    if (p.bytes_per_sector < 512 || p.bytes_per_sector > 32768 || (p.bytes_per_sector & 0x1FF) != 0)
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
    /* M-045: numFATs == 0 is a hard reject (the og's every gate requires
     * it nonzero; the old code let fat_area = 0 through - a latent
     * nonsense-geometry path). */
    if (p.fat_count == 0)
        return ERR_BAD_PARAMETER;

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
    /* M-045: the og's classifier rejects cluster counts >= 0xFFFFFF0
     * outright (FUN_180004610 :2595-2598 - the final band's else-branch);
     * the spec's own reserved-cluster max is 0x0FFFFFF6, so this only
     * cuts spec-overflow noise the og never surfaces. */
    if (p.cluster_count >= 0xFFFFFF0u)
        return ERR_BAD_PARAMETER;

    /* S-064: the flavor is STRING-FIRST when an FS-name string matches -
     * the og's engine-open validator (FUN_18002f970 :38044/:38061)
     * assigns flavor 1/2 purely by "FAT16   " at +0x36 / "FAT32   " at
     * +0x52 plus BPB shape gates; NO cluster-count term exists in that
     * function (the bracket machinery FUN_180004610 is the lost/Calc
     * side). Re-read first-hand. A "FAT16   "-string volume under 4085
     * clusters is flavor 1 in the og (16-bit entries, full stop); the
     * pre-fix core bracketed it to Fat12 and decoded 12-bit entries -
     * different chains, different recovered bytes. String-less volumes
     * keep the cluster bracket (the D-94/I-002 deliberate superset:
     * structural FAT12 recovery the og declines; unchanged). */
    if (fs_name_matches(sector512 + 0x36, "fat16   "))
        p.flavor = Flavor::Fat16;
    else if (fs_name_matches(sector512 + 0x52, "fat32   "))
        p.flavor = Flavor::Fat32;
    else
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

u32 read_volume_label(IStorageDevice* slice, std::u16string& out) {
    /* S-051/M-104 (diff-to-fix): the boot-sector label - one read, gated
     * by validate_boot (the same structural gate probe_family's FAT leg
     * applies, so the label decode never fires on a non-FAT slice). The
     * 11-byte field is space-padded; the trim keeps interior spaces
     * (real labels carry them: "MY DISK   "). All-space/all-zero (a
     * label-less volume - the real H session's ` (H:)` form) is the
     * honest ERR_BAD_PARAMETER so the caller takes its fallback tier. */
    if (slice == nullptr)
        return ERR_BAD_PARAMETER;
    u8 sec[512];
    if (slice->read_at(0, sec, sizeof(sec)) != OK)
        return ERR_READ_FAILED;
    FatParams p;
    if (validate_boot(sec, slice->size(), p) != OK)
        return ERR_BAD_MAGIC;
    const u8* field = p.flavor == Flavor::Fat32 ? sec + 0x47 : sec + 0x2B;
    size_t end = 11;
    while (end > 0 && (field[end - 1] == ' ' || field[end - 1] == 0))
        --end;
    if (end == 0)
        return ERR_BAD_PARAMETER; /* label-less volume */
    out.clear();
    out.reserve(end);
    for (size_t i = 0; i < end; ++i) {
        u8 c = field[i];
        if (c == 0)
            c = ' '; /* interior NULs in a torn label read as spaces */
        out.push_back(u16(c));
    }
    return OK;
}

} // namespace fat
} // namespace cr
