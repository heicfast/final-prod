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
 *
 * M-119 (diff-to-fix): the og's boundary semantics are TOLERANT, at BOTH
 * the create and the read - CPartitionVirtualVolumeDevice's ctor accepts
 * the declared window and its read slot (HFSPlusSearchDLL FUN_180009290,
 * the engine-side device; the DRWUI twin FUN_1400d0510) clamps a
 * straddling read to the in-bounds prefix, reads it via the parent, and
 * ZERO-FILLS the unread tail on success. create() therefore clamps an
 * over-long DECLARED length to the device end (only the length - a start
 * beyond the device is still refused honestly, N-046's posture) and
 * read_at() implements the og ladder: offset >= length or count > length
 * are parameter errors (both sides refuse), a straddle clamps + zero-
 * fills + returns OK. The og's own tail memset is under-zeroed (sector-
 * count bytes, not sector-count x sector-size - the D-74 decode, confirmed
 * at both sites); the core zeroes the FULL tail - that arithmetic defect
 * stays unadopted. The write side is unaffected: IStorageDevice has no
 * write path, so the tolerance cannot leak into cr_apply/SafeWriter.
 */
class PartitionSlice final : public IStorageDevice {
public:
    static std::unique_ptr<PartitionSlice> create(IStorageDevice* parent, u64 start_byte,
                                                  u64 length_bytes, u32* err = nullptr);

    u32 read_at(u64 offset, void* buffer, u32 length) override;
    const Geometry& geometry() const override { return geom_; }
    u64 absolute_base() const override { return start_; }
    bool is_static_media() const override {
        return parent_ != nullptr && parent_->is_static_media();
    }

    /* N-046 (og FUN_140643870, DRWUI_decompiled.c:1044889-1044895: the
     * virtual volume device re-derives its bytes-per-sector from the
     * parent's vtable call and stores it - the removable-media-change
     * refresh; HFS twin :7163-7177). Re-reads the parent geometry and
     * recomputes this slice's derived fields (total_bytes stays the
     * slice length; sector_count re-floors). Revalidates first: if the
     * new geometry invalidates the slice (zero sector size, start no
     * longer parent-aligned, or the slice no longer fits the parent)
     * the refresh is REFUSED with the error and the snapshot stays
     * unchanged - a registered hardening over the og's blind store
     * (FUN_140643870 returns 1 unconditionally). The orchestrator does
     * NOT auto-call this: the corpus exposes the method but no caller
     * on the scan path (media-change detection is transport-layer;
     * read failures surface through the error map either way). */
    u32 refresh_geometry();

    IStorageDevice* parent() const { return parent_; }
    u64 start() const { return start_; }
    u64 length() const { return length_; }
    /* M-119: the table's DECLARED length (pre-clamp) and the clamp delta -
     * callers surface the delta so a shrunken-media/table-overrun volume
     * scans with an honest warning instead of being refused outright. */
    u64 declared_bytes() const { return declared_; }
    u64 clamp_delta() const { return declared_ - length_; }
    bool was_clamped() const { return declared_ != length_; }

private:
    PartitionSlice() = default;
    IStorageDevice* parent_ = nullptr; /* not owned */
    u64 start_ = 0;
    u64 length_ = 0;
    u64 declared_ = 0; /* M-119: the declared window, pre-clamp */
    Geometry geom_;
};

/*
 * SliceShim - non-owning adapter: hands an EXISTING device to APIs that
 * take ownership (NtfsVolume::open). The shim dies with the owner; the
 * underlying device is untouched.
 */
class SliceShim final : public IStorageDevice {
public:
    explicit SliceShim(IStorageDevice* target) : target_(target) {}
    u32 read_at(u64 offset, void* buffer, u32 length) override {
        return target_->read_at(offset, buffer, length);
    }
    const Geometry& geometry() const override { return target_->geometry(); }
    u64 absolute_base() const override { return target_->absolute_base(); }
    bool is_static_media() const override { return target_->is_static_media(); }

    static std::unique_ptr<SliceShim> shim(IStorageDevice* t) {
        return std::unique_ptr<SliceShim>(new SliceShim(t));
    }

private:
    IStorageDevice* target_;
};

} // namespace cr

#endif // CR_SLICE_H_
