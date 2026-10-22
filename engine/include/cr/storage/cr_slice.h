/*
 * cr_slice.h - PartitionSlice: the bounded virtual-volume reader.
 *
 * Clean-room equivalent of CPartitionVirtualVolumeDevice (corpus:
 * 05-scan-engine-architecture.md section 4.2): "Virtual = works for volumes the
 * OS can't see (lost/deleted partitions)". Every filesystem engine receives a
 * bounded ReadAt(offset, buffer) view into a candidate partition region and
 * cannot read outside it - enforced here, not by convention (R1.5).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_SLICE_H_
#define CR_SLICE_H_

#include "cr_device.h"
#include <memory>

namespace cr {

/*
 * A read-only window [start_byte, start_byte + length_bytes) over a parent
 * device. Child offset 0 == partition start. All accesses bounds-checked in
 * BYTES (fix F1: no baked-in 512).
 *
 * slice_length need not be sector-aligned at its END (the tail of a lost
 * partition candidate can be ragged); the start must be parent-aligned to the
 * parent's logical sector.
 */
class PartitionSlice final : public IStorageDevice {
public:
    static std::unique_ptr<PartitionSlice> create(IStorageDevice *parent, u64 start_byte,
                                                  u64 length_bytes, u32 *err = nullptr);

    u32 read_at(u64 offset, void *buffer, u32 length) override;
    const Geometry &geometry() const override { return geom_; }

    IStorageDevice *parent() const { return parent_; }
    u64 start() const { return start_; }
    u64 length() const { return length_; }

private:
    PartitionSlice() = default;
    IStorageDevice *parent_ = nullptr; /* not owned */
    u64 start_ = 0;
    u64 length_ = 0;
    Geometry geom_;
};

/*
 * SliceShim - non-owning adapter: hands an EXISTING device to APIs that
 * take ownership (NtfsVolume::open). The shim dies with the owner; the
 * underlying device is untouched.
 */
class SliceShim final : public IStorageDevice {
public:
    explicit SliceShim(IStorageDevice *target) : target_(target) {}
    u32 read_at(u64 offset, void *buffer, u32 length) override {
        return target_->read_at(offset, buffer, length);
    }
    const Geometry &geometry() const override { return target_->geometry(); }

    static std::unique_ptr<SliceShim> shim(IStorageDevice *t) {
        return std::unique_ptr<SliceShim>(new SliceShim(t));
    }

private:
    IStorageDevice *target_;
};

} // namespace cr

#endif // CR_SLICE_H_
