/* cr_chain.cpp - FatTable + chain walker (R5.1).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/fat/cr_fat_engine.h"
#include <cstring>

namespace cr {
namespace fat {

Flavor flavor_from_clusters(u32 cluster_count_data) {
    /* Microsoft FAT whitepaper thresholds (data clusters, incl. reserved 2) */
    if (cluster_count_data < 4085)
        return Flavor::Fat12;
    if (cluster_count_data < 65525)
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
        u32 v = u32_from_bytes_le(f.data() + off) & 0x0FFFFFFF;
        if (v == 0x0FFFFFF7)
            return CHAIN_BAD;
        if (v >= 0x0FFFFFF8)
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
            divergences_ = 0;
            for (u32 c = 2; c <= max_cluster_; ++c) {
                if (entry_at(fat1_, c) != entry_at(fat2_, c))
                    ++divergences_;
            }
        }
    }
    return OK;
}

u32 FatTable::next(u32 cluster) const {
    if (cluster < 2 || cluster > max_cluster_)
        return 0;
    u32 v = entry_at(fat1_, cluster);
    /* choose-healthy mirror: if primary says BAD/FREE, consult FAT2 */
    if (mirror_ && (v == 0 || v == CHAIN_BAD)) {
        u32 alt = entry_at(fat2_, cluster);
        if (alt != 0 && alt != CHAIN_BAD)
            return alt;
    }
    if (v >= CHAIN_EOC_MIN && v < 0x0FFFFFF0)
        return CHAIN_EOC_MIN; /* canonical marker */
    if (v >= 0x0FFFFFF0) {
        /* FAT12 0xFF0-0xFF6 / FAT16 0xFFF0-0xFFF6 / FAT32 reserved: treat as
         * reserved-value -> end of chain for walking purposes */
        return CHAIN_EOC_MIN;
    }
    if (v == 0 || v == CHAIN_BAD)
        return v;
    return v;
}

u32 FatTable::raw(u32 cluster) const {
    if (cluster < 2 || cluster > max_cluster_)
        return 0;
    u32 v = entry_at(fat1_, cluster);
    if (mirror_ && (v == 0 || v == CHAIN_BAD)) {
        u32 alt = entry_at(fat2_, cluster);
        if (alt != 0 && alt != CHAIN_BAD)
            return alt;
    }
    return v;
}

void FatTable::set_raw(u32 cluster, u32 value) {
    if (cluster < 2 || cluster > max_cluster_ || fat1_.empty())
        return;
    switch (flavor_) {
    case Flavor::Fat12: {
        size_t off = size_t(cluster) + size_t(cluster) / 2;
        if (off + 2 > fat1_.size())
            return;
        u16 cur = u16_from_bytes_le(fat1_.data() + off);
        u16 v = u16(value);
        if (cluster & 1) {
            v = u16((cur & 0x000F) | ((v & 0xFFF) << 4));
        } else {
            v = u16((cur & 0xF000) | (v & 0xFFF));
        }
        fat1_[off] = u8(v);
        fat1_[off + 1] = u8(v >> 8);
        break;
    }
    case Flavor::Fat16: {
        size_t off = size_t(cluster) * 2;
        if (off + 2 > fat1_.size())
            return;
        fat1_[off] = u8(value);
        fat1_[off + 1] = u8(value >> 8);
        break;
    }
    case Flavor::Fat32:
    default: {
        size_t off = size_t(cluster) * 4;
        if (off + 4 > fat1_.size())
            return;
        for (int i = 0; i < 4; ++i)
            fat1_[off + i] = u8(value >> (8 * i));
        break;
    }
    }
}

ChainResult follow_chain(const FatTable& t, u32 start, u32 max_len) {
    ChainResult r;
    if (start < 2 || start > t.max_cluster()) {
        r.broken = true;
        return r;
    }
    std::vector<u64> visited; /* visited clusters (cycle guard, fix F7) */
    u32 c = start;
    u32 guard = 0;
    while (c >= 2 && c <= t.max_cluster()) {
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

ChainResult reconstruct_deleted_chain(const FatTable& t, u32 start, u32 expect_clusters) {
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

u32 validate_boot(const u8* sector512, u64 device_bytes, FatParams& out) {
    if (sector512 == nullptr || !fat::boot_signature_ok(sector512))
        return ERR_BAD_MAGIC;
    FatParams p;
    p.bytes_per_sector = u16_from_bytes_le(sector512 + 0x0B);
    if (p.bytes_per_sector < 512 || p.bytes_per_sector > 4096 ||
        (p.bytes_per_sector & (p.bytes_per_sector - 1)) != 0)
        return ERR_BAD_PARAMETER;
    p.sectors_per_cluster = sector512[0x0D];
    if (p.sectors_per_cluster == 0 || (p.sectors_per_cluster & (p.sectors_per_cluster - 1)) != 0)
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
        p.backup_boot_sector = u16_from_bytes_le(sector512 + 0x32);
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
    p.first_usable_cluster = 2;
    out = p;
    return OK;
}

} // namespace fat
} // namespace cr
