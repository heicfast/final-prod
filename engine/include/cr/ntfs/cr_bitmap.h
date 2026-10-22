/*
 * cr_bitmap.h - $Bitmap reader: cluster allocation test (R2.4).
 *
 * Corpus (FUN_18002c2e0 + algorithms-v2 section 3): bit-array with
 * base-offset re-basing; test = buffer[pos>>3] & (1 << (pos&7)). Used to
 * grade deleted-record viability: bit SET = cluster in use (reused).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_BITMAP_H_
#define CR_BITMAP_H_

#include "cr_bootstrap.h"
#include <vector>

namespace cr {
namespace ntfs {

class VolumeBitmap {
public:
    /* Read $Bitmap (MFT record 6) via the volume. bits cover clusters
     * [first_usable .. last_usable) semantics are caller-level; we simply
     * test cluster indices. size = cluster count of the volume. */
    static u32 load(NtfsVolume *vol, std::unique_ptr<VolumeBitmap> &out);

    /* Corpus bit test: bit set = in use. Out-of-range = false (treat unknown
     * as free - salvage-over-purity: a missing bit must not destroy files). */
    bool test(u64 cluster) const {
        if (buffer_.empty()) return false;
        if (cluster >= bit_count_) return false;
        const u8 *b = buffer_.data() + (cluster >> 3);
        if (size_t(cluster >> 3) >= buffer_.size()) return false;
        return (*b & (1u << (cluster & 7))) != 0;
    }

    /* ratio of IN-USE clusters across a run list (0.0..1.0). Sparse runs
     * count as reused (hole = never had content on disk). */
    double reused_ratio(const RunList &runs) const;

    u64 bit_count() const { return bit_count_; }
    bool valid() const { return !buffer_.empty(); }

private:
    VolumeBitmap() = default;
    std::vector<u8> buffer_;
    u64 bit_count_ = 0;
};

} // namespace ntfs
} // namespace cr

#endif // CR_BITMAP_H_
