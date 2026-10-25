/*
 * cr_freemap.h - cluster-granular free/used map over a scanned slice
 * (S-009: the engine side of the og's IBuildVolumeBitmap).
 *
 * Corpus lineage: the reference's shared IBuildVolumeBitmap interface
 * ("which clusters are free/used" - corpus
 * searchdll-family--architecture.md section 2; the NTFS controller is
 * the bitmap builder per ntfs-engine--deep-dive.md section 1). The og's
 * engines produce free/used truth from the filesystem allocation
 * structures; this type is the core's producer-side carrier for the
 * same truth, consumed by the disk-map builder (cr_diskmap.h) to paint
 * the free overlay the og's AnalysisGridWidget renders.
 *
 * Truth rules (nothing invented):
 *  - bit SET = cluster FREE (the og's query is free/used; the core keeps
 *    free as the set bit so an all-zero map degrades to "no map").
 *  - coverage: [base_byte, base_byte + cluster_count * cluster_bytes)
 *    in ABSOLUTE device bytes (the orchestrator rebases the engines'
 *    slice-relative bases exactly like the tree extents - F-1).
 *  - out-of-range clusters are NOT free (unknown != free; a missing bit
 *    must never paint a cell free that may hold data).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_FREEMAP_H_
#define CR_FREEMAP_H_

#include "../cr_types.h"
#include <vector>

namespace cr {
namespace storage {

struct FreeMap {
    u64 base_byte = 0;     /* absolute byte of cluster 0 */
    u64 cluster_count = 0; /* clusters covered by the map */
    u32 cluster_bytes = 0; /* 0 = invalid map */
    std::vector<u8> bits;  /* bit set = cluster free */

    bool valid() const { return cluster_bytes != 0 && cluster_count != 0; }

    /* Corpus truth rule: out-of-range / missing bit = NOT free. */
    bool is_free(u64 cluster) const {
        if (cluster >= cluster_count || cluster_bytes == 0)
            return false;
        const size_t byte = size_t(cluster >> 3);
        if (byte >= bits.size())
            return false;
        return (bits[byte] & (1u << (cluster & 7))) != 0;
    }

    /* Byte range [abs_byte, abs_byte + len) fully inside the map's
     * coverage AND every covering cluster free. Conservative by design:
     * partial coverage answers false (unknown is not free). */
    bool range_free(u64 abs_byte, u64 len) const {
        if (!valid() || len == 0)
            return false;
        const u64 end = abs_byte + len; /* no overflow at call sites: the
                                         * diskmap passes clamped views */
        if (abs_byte < base_byte || end > base_byte + cluster_count * cluster_bytes)
            return false;
        const u64 k0 = (abs_byte - base_byte) / cluster_bytes;
        const u64 k1 = (end - 1 - base_byte) / cluster_bytes;
        for (u64 k = k0; k <= k1; ++k)
            if (!is_free(k))
                return false;
        return true;
    }

    void clear() {
        base_byte = 0;
        cluster_count = 0;
        cluster_bytes = 0;
        bits.clear();
    }
};

} // namespace storage
} // namespace cr

#endif /* CR_FREEMAP_H_ */
