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

namespace cr {

std::unique_ptr<PartitionSlice> PartitionSlice::create(IStorageDevice *parent, u64 start_byte,
                                                       u64 length_bytes, u32 *err) {
    if (parent == nullptr || length_bytes == 0) {
        if (err) *err = ERR_BAD_PARAMETER;
        return nullptr;
    }
    const u32 ls = parent->geometry().logical_sector;
    if (ls == 0 || start_byte % ls != 0) {
        if (err) *err = ERR_BAD_PARAMETER;
        return nullptr;
    }
    if (start_byte + length_bytes > parent->size()) {
        if (err) *err = ERR_RANGE_OUT_OF_BOUNDS;
        return nullptr;
    }
    auto s = std::unique_ptr<PartitionSlice>(new PartitionSlice());
    s->parent_ = parent;
    s->start_ = start_byte;
    s->length_ = length_bytes;
    s->geom_ = parent->geometry();
    s->geom_.total_bytes = length_bytes;
    s->geom_.sector_count = length_bytes / s->geom_.logical_sector;
    return s;
}

u32 PartitionSlice::read_at(u64 offset, void *buffer, u32 length) {
    if (length == 0) return OK;
    if (offset > length_ || u64(length) > length_ - offset)
        return ERR_RANGE_OUT_OF_BOUNDS;
    /* propagate with retries handled by the parent transport layer */
    return parent_->read_at(start_ + offset, buffer, length);
}

} // namespace cr
