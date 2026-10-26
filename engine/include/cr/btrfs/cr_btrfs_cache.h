/*
 * cr_btrfs_cache.h - the Btrfs re-scan metadata cache (N-072).
 *
 * Corpus lineage: the og's BtrfsSearchDLL persists a metadata cache to a
 * relative "BtrfsCache.txt" beside the engine DLL to speed re-scans of
 * the same volume - subvolume/chunk trees cached
 * (artifacts/SearchDLL/remaining-engines--ext4-hfs-refs-btrfs-zfs.md
 * :66-67, string evidence only: the format and the invalidation rule are
 * NOT decoded anywhere in the corpus).
 *
 * REGISTERED DIVERGENCE (docs/PARITY-DELTAS, the finding's own open
 * question resolved): because the og's format is undecoded, this cache's
 * format is OUR OWN (versioned binary blob + CRC32, documented here).
 * Two deliberate placement/policy deltas from the og, both required by
 * the finding's fix plan: (1) the cache lives in a CALLER-SUPPLIED
 * directory (the og's beside-the-DLL placement breaks under Program
 * Files / readonly media / multi-user contexts); (2) it is OFF by
 * default (the og writes unconditionally). One file per volume keyed by
 * fsid; the generation rides INSIDE the blob as the strict validity gate
 * (match-or-discard) so a live volume whose generation advanced never
 * adopts a stale map - the plan's "content-keyed filename (volume uuid +
 * generation)" intent is preserved (single size-bounded file, no churn).
 *
 * What is cached: the derived logical->physical chunk map (the full
 * mapping after the chunk-tree walk) plus the chunk-root and root-tree
 * bytenrs - exactly the derivations the re-scan would otherwise repeat
 * (the og's "subvolume/chunk trees cached"). The trees themselves are
 * NOT cached: root/subvolume walks re-read and re-validate their nodes
 * every scan (recovery-honest: a damaged node is seen as damaged, not
 * served from a snapshot).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_BTRFS_CACHE_H_
#define CR_BTRFS_CACHE_H_

#include "cr_btrfs_engine.h"
#include <vector>

namespace cr {
namespace btrfs {

/* The host-owned file seam (the platform layer owns real files; tests own
 * memory). `dir` is the caller-supplied cache directory; the blob codec
 * below defines the bytes; the seam owns the file name (fsid-keyed) and
 * the write policy (atomic temp+rename is the host's choice). */
class BtrfsCacheIo {
public:
    virtual ~BtrfsCacheIo() = default;
    /* Read the volume's cached blob into `out`; false = no cache / unreadable. */
    virtual bool read_blob(const std::u16string& dir, const u8 fsid[16], std::vector<u8>& out) = 0;
    /* Persist the blob; false = unwritable (the scan proceeds, cache-less). */
    virtual bool write_blob(const std::u16string& dir, const u8 fsid[16],
                            const std::vector<u8>& bytes) = 0;
};

/* The cache configuration: empty dir (or null io) = DISABLED (the default;
 * nothing is read, nothing is written - the og's unconditional beside-the-
 * DLL write is deliberately not copied). max_bytes bounds the serialized
 * blob (a hostile or absurd map cannot grow the cache unbounded). */
struct BtrfsCacheConfig {
    std::u16string dir;
    BtrfsCacheIo* io = nullptr;
    u32 max_bytes = 1u << 20; /* 1 MiB: ~10k chunks, far past any real volume */
};

/* The cached derivations. */
struct BtrfsCacheData {
    u64 generation = 0;
    u64 chunk_root = 0;
    u64 root_tree = 0;
    std::vector<ChunkMap> chunks;
};

/* Serialize `data` + the volume identity into the cache blob format:
 * magic "CRBTC001" | u32 version=1 | fsid[16] | u64 generation | u64
 * chunk_root | u64 root_tree | u32 chunk_count | per chunk: u64
 * logical_start/len/physical_start/type/stripe_len, u16
 * num_stripes/sub_stripes/mirror_index, u32 stripe_count, per stripe u64
 * devid+offset | u32 crc32 (zlib crc32 over all preceding bytes).
 * Returns false when the serialization would exceed max_bytes. */
bool cache_serialize(const u8 fsid[16], const BtrfsCacheData& data, u32 max_bytes,
                     std::vector<u8>& out);

/* Parse + validate a blob: magic, version, crc32, fsid equality and
 * generation equality (the strict validity gate - a stale-generation or
 * foreign-volume blob is DISCARDED, never adopted). On any mismatch or
 * malformed field: false, `out` untouched. */
bool cache_parse_validate(const u8 fsid[16], u64 generation, const std::vector<u8>& blob,
                          BtrfsCacheData& out);

} // namespace btrfs
} // namespace cr

#endif // CR_BTRFS_CACHE_H_
