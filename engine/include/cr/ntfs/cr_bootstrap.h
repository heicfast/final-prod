/*
 * cr_bootstrap.h - NTFS volume bootstrap: boot sector validation, $MFT stream
 * construction, $MFTMirr fallback, fragmented-MFT ($ATTRIBUTE_LIST) path.
 *
 * Corpus mapping (ntfs deep-dive section 2 + ground-truth section 3):
 *   - 64 KiB first-read region, fixup applied, 'FILE' magic, header sanity
 *   - $ATTRIBUTE_LIST on $MFT record -> fragmented-MFT handler (depth-limited,
 *     fix F6); trace-equivalent logging
 *   - $MFTMirr cross-reference as repair/verification source (first 4 records
 *     mirrored at volume midpoint)
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_BOOTSTRAP_H_
#define CR_BOOTSTRAP_H_

#include "cr_runlist.h"
#include "cr_ntfs_wire.h"
#include "../storage/cr_device.h"
#include <memory>
#include <vector>

namespace cr {
namespace ntfs {

constexpr u32 MFT_RESERVED_RECORDS = 24;   /* 0..15 meta + historically 24 */
constexpr u64 MFT_FIRST_READ_BYTES = 64 * 1024; /* corpus: 64 KiB buffer */

struct VolumeParams {
    u64 record_size = 0;         /* bytes per MFT record (>= 0x400 clamp) */
    u32 cluster_bytes = 0;
    u32 sector_bytes = 0;
    u64 total_clusters = 0;
    u64 mft_lcn = 0;
    u64 mftmirr_lcn = 0;
    u16 flags = 0;               /* from DBR-derived volume state */
};

/* Opened NTFS volume: owns the MFT stream mapping and the geometry. */
class NtfsVolume {
public:
    /* Open + validate. Fills err on failure. The device is adopted (owned). */
    static std::unique_ptr<NtfsVolume> open(std::unique_ptr<IStorageDevice> device, u32 *err);

    /* Read one MFT record (by number) into out (record_size bytes), fixups
     * applied in place by the caller via cr_fixup. Reads through the cache.
     * rc ERR_RETRY_EXHAUSTED propagates from transport. */
    u32 read_record(u64 record_no, std::vector<u8> &out);

    /* $MFTMirr variant for records 0..3 (verification/repair source). */
    u32 read_record_mirr(u64 record_no, std::vector<u8> &out);

    const VolumeParams &params() const { return p_; }
    IStorageDevice *device() const { return dev_.get(); }
    u64 max_record_no() const {
        return p_.record_size ? mft_run_.total_clusters() * p_.cluster_bytes / p_.record_size
                              : 0;
    }

    /* True when record falls inside the first-MFT-reader region already
     * mapped (used by the walker to batch). */
    const RunList &mft_runlist() const { return mft_run_; }
    const RunList &mftmirr_runlist() const { return mirr_run_; }

    /* $ATTRIBUTE_LIST on $MFT itself: extent list assembled during open. */
    const std::vector<DataRun> &mft_extents() const { return mft_extents_; }

    u32 read_at(u64 offset, void *buf, u32 len) { return dev_->read_at(offset, buf, len); }
    u64 size() const { return dev_->size(); }

private:
    NtfsVolume() = default;
    u32 load_mft_stream();
    u32 load_mftmirr_stream();

    std::unique_ptr<IStorageDevice> dev_;
    VolumeParams p_;
    RunList mft_run_;
    RunList mirr_run_;
    std::vector<DataRun> mft_extents_;
    bool mirr_valid_ = false;
};

/* DBR validation: returns OK + fills params, or ERR_* on hard invalidity.
 * Sector-size sanity: 512 <= bps <= 4096 power of two; spc power of two;
 * record size >= 0x400 after resolution (corpus clamp); MFT LCN in range. */
u32 validate_boot_sector(const BootSector &bs, const Geometry &geom, VolumeParams &out);

} // namespace ntfs
} // namespace cr

#endif // CR_BOOTSTRAP_H_
