/* cr_slice.cpp - PartitionSlice bounded reader (R1.5).
 *
 * Reference: CPartitionVirtualVolumeDevice - "works for volumes the OS can't
 * see (lost/deleted partitions)" (corpus 05 section 4.2). The bound is
 * enforced HERE: child offset 0 == partition start; every access clamps and
 * verifies against [0, length).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/storage/cr_slice.h"
#include <cstring>

namespace cr {

std::unique_ptr<PartitionSlice> PartitionSlice::create(IStorageDevice* parent, u64 start_byte,
                                                       u64 length_bytes, u32* err) {
    if (parent == nullptr || length_bytes == 0) {
        if (err)
            *err = ERR_BAD_PARAMETER;
        return nullptr;
    }
    const u32 ls = parent->geometry().logical_sector;
    if (ls == 0 || start_byte % ls != 0) {
        if (err)
            *err = ERR_BAD_PARAMETER;
        return nullptr;
    }
    const u64 parent_size = parent->size();
    if (start_byte >= parent_size) {
        /* N-046's honest posture, kept by M-119's fix plan: a START past
         * the device is genuinely unopenable - only the LENGTH clamps. */
        if (err)
            *err = ERR_RANGE_OUT_OF_BOUNDS;
        return nullptr;
    }
    u64 length = length_bytes;
    if (start_byte + length_bytes > parent_size) {
        /* M-119: the og's tolerant create - the declared window overruns
         * the media (a corrupt/oversized table entry, a table restored
         * onto smaller media, a shrunken device): CLAMP to the device
         * end and record the delta. The og scans such volumes (real
         * bytes to the end, zeros beyond); the core did too - after
         * this - instead of refusing the whole volume. */
        length = parent_size - start_byte;
    }
    auto s = std::unique_ptr<PartitionSlice>(new PartitionSlice());
    s->parent_ = parent;
    s->start_ = start_byte;
    s->length_ = length;
    s->declared_ = length_bytes;
    s->geom_ = parent->geometry();
    s->geom_.total_bytes = length;
    s->geom_.sector_count = length / s->geom_.logical_sector;
    return s;
}

u32 PartitionSlice::read_at(u64 offset, void* buffer, u32 length) {
    if (length == 0)
        return OK;
    /* M-119: the og ladder (CPartitionVirtualVolumeDevice's read slot,
     * HFSPlusSearchDLL FUN_180009290 :7219-7264, confirmed first-hand:
     * the gate is (0 <= offset < length) && (count <= length) - both
     * parameter errors refuse; the straddle clamps to the in-bounds
     * prefix, reads it via the parent, and zero-fills the tail on
     * success. The D-74 strict refusal is superseded by this landing
     * (dated addendum in PARITY-DELTAS); the og's under-zeroed tail
     * memset (sector-count bytes) stays unadopted - the FULL tail is
     * zeroed. */
    if (offset >= length_ || u64(length) > length_)
        return ERR_RANGE_OUT_OF_BOUNDS;
    if (offset + u64(length) > length_) {
        /* straddle: the in-bounds prefix via the parent */
        const u32 prefix = u32(length_ - offset);
        const u32 rc = parent_->read_at(start_ + offset, buffer, prefix);
        if (rc != OK)
            return rc; /* the og's error path carries no zero-fill */
        std::memset(static_cast<u8*>(buffer) + prefix, 0, length - prefix);
        return OK;
    }
    /* propagate with retries handled by the parent transport layer */
    return parent_->read_at(start_ + offset, buffer, length);
}

u32 PartitionSlice::refresh_geometry() {
    if (parent_ == nullptr)
        return ERR_BAD_PARAMETER;
    /* N-046: the og's RefreshSectorSize-from-parent (FUN_140643870)
     * blindly stores the parent's sector size and returns 1. We
     * revalidate before adopting: a media change that invalidates the
     * slice (misaligned start, shrunken parent) is refused with the
     * snapshot untouched - the honest refusal over the silent adoption. */
    const Geometry pg = parent_->geometry();
    if (pg.logical_sector == 0 || start_ % pg.logical_sector != 0)
        return ERR_BAD_PARAMETER;
    if (start_ + length_ > parent_->size())
        return ERR_RANGE_OUT_OF_BOUNDS;
    geom_ = pg;
    geom_.total_bytes = length_;
    geom_.sector_count = length_ / geom_.logical_sector;
    return OK;
}

} // namespace cr
