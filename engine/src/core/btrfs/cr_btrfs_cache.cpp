/* cr_btrfs_cache.cpp - the Btrfs re-scan metadata cache codec (N-072).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/btrfs/cr_btrfs_cache.h"
#include "cr/table/cr_table_wire.h"
#include <cstring>

namespace cr {
namespace btrfs {

using table::crc32;

namespace {

constexpr char kMagic[8] = {'C', 'R', 'B', 'T', 'C', '0', '0', '1'};
constexpr u32 kVersion = 1;

/* bounds: a real chunk count is O(10k); hostile blobs are rejected by the
 * cap math before any allocation */
constexpr u32 kMaxChunkCount = 100000;
constexpr u32 kMaxStripesPerChunk = 4096;

void put16(std::vector<u8>& b, u16 v) {
    b.push_back(u8(v & 0xFF));
    b.push_back(u8((v >> 8) & 0xFF));
}

void put32(std::vector<u8>& b, u32 v) {
    for (int i = 0; i < 4; ++i)
        b.push_back(u8((v >> (8 * i)) & 0xFF));
}

void put64(std::vector<u8>& b, u64 v) {
    for (int i = 0; i < 8; ++i)
        b.push_back(u8((v >> (8 * i)) & 0xFF));
}

u16 get16(const u8* p) {
    return u16(p[0]) | (u16(p[1]) << 8);
}

u32 get32(const u8* p) {
    return u32(p[0]) | (u32(p[1]) << 8) | (u32(p[2]) << 16) | (u32(p[3]) << 24);
}

u64 get64(const u8* p) {
    u64 v = 0;
    for (int i = 7; i >= 0; --i)
        v = (v << 8) | u64(p[i]);
    return v;
}

} // namespace

bool cache_serialize(const u8 fsid[16], const BtrfsCacheData& data, u32 max_bytes,
                     std::vector<u8>& out) {
    if (fsid == nullptr || data.chunks.size() > kMaxChunkCount)
        return false;
    std::vector<u8> b;
    b.reserve(64 + data.chunks.size() * 64);
    b.insert(b.end(), kMagic, kMagic + 8);
    put32(b, kVersion);
    b.insert(b.end(), fsid, fsid + 16);
    put64(b, data.generation);
    put64(b, data.chunk_root);
    put64(b, data.root_tree);
    put32(b, u32(data.chunks.size()));
    for (const ChunkMap& c : data.chunks) {
        if (c.stripes.size() > kMaxStripesPerChunk)
            return false;
        put64(b, c.logical_start);
        put64(b, c.logical_len);
        put64(b, c.physical_start);
        put64(b, c.type);
        put64(b, c.stripe_len);
        put16(b, c.num_stripes);
        put16(b, c.sub_stripes);
        put16(b, c.mirror_index);
        put32(b, u32(c.stripes.size()));
        for (const ChunkStripe& s : c.stripes) {
            put64(b, s.devid);
            put64(b, s.offset);
        }
    }
    put32(b, crc32(b.data(), b.size()));
    if (b.size() > max_bytes)
        return false;
    out = std::move(b);
    return true;
}

bool cache_parse_validate(const u8 fsid[16], u64 generation, const std::vector<u8>& blob,
                          BtrfsCacheData& out) {
    if (fsid == nullptr || blob.size() < 8 + 4 + 16 + 8 * 3 + 4 + 4)
        return false;
    const u8* p = blob.data();
    const size_t size = blob.size();
    if (std::memcmp(p, kMagic, 8) != 0)
        return false;
    p += 8;
    if (get32(p) != kVersion)
        return false;
    p += 4;
    if (std::memcmp(p, fsid, 16) != 0)
        return false; /* foreign volume - never adopt */
    p += 16;
    BtrfsCacheData d;
    d.generation = get64(p);
    p += 8;
    if (d.generation != generation)
        return false; /* stale generation - the strict validity gate */
    d.chunk_root = get64(p);
    p += 8;
    d.root_tree = get64(p);
    p += 8;
    const u32 nchunks = get32(p);
    p += 4;
    if (nchunks > kMaxChunkCount)
        return false;
    /* every chunk needs >= 48 fixed bytes + its stripe array */
    for (u32 i = 0; i < nchunks; ++i) {
        if (size_t(p - blob.data()) + 48 + 4 > size)
            return false;
        ChunkMap c;
        c.logical_start = get64(p);
        p += 8;
        c.logical_len = get64(p);
        p += 8;
        c.physical_start = get64(p);
        p += 8;
        c.type = get64(p);
        p += 8;
        c.stripe_len = get64(p);
        p += 8;
        c.num_stripes = get16(p);
        p += 2;
        c.sub_stripes = get16(p);
        p += 2;
        c.mirror_index = get16(p);
        p += 2;
        const u32 nstripes = get32(p);
        p += 4;
        if (nstripes > kMaxStripesPerChunk || size_t(p - blob.data()) + u64(nstripes) * 16 > size)
            return false;
        c.stripes.resize(nstripes);
        for (u32 s = 0; s < nstripes; ++s) {
            c.stripes[s].devid = get64(p);
            p += 8;
            c.stripes[s].offset = get64(p);
            p += 8;
        }
        d.chunks.push_back(std::move(c));
    }
    /* trailing crc32 over everything before it */
    if (size_t(p - blob.data()) + 4 != size)
        return false;
    if (crc32(blob.data(), size - 4) != get32(p))
        return false; /* corrupt - discarded, never adopted */
    out = std::move(d);
    return true;
}

} // namespace btrfs
} // namespace cr
