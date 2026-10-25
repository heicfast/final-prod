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

struct VolumeParams {
    u64 record_size = 0; /* bytes per MFT record (>= 0x400 clamp) */
    u32 cluster_bytes = 0;
    u32 sector_bytes = 0;
    u64 total_clusters = 0;
    u64 mft_lcn = 0;
    u64 mftmirr_lcn = 0;
    u16 flags = 0; /* from DBR-derived volume state */
};

/* Opened NTFS volume: owns the MFT stream mapping and the geometry. */
class NtfsVolume {
public:
    /* Open + validate. Fills err on failure. The device is adopted (owned). */
    static std::unique_ptr<NtfsVolume> open(std::unique_ptr<IStorageDevice> device, u32* err);

    /* Read one MFT record (by number) into out (record_size bytes), fixups
     * applied in place by the caller via cr_fixup. Reads through the cache.
     * rc ERR_RETRY_EXHAUSTED propagates from transport. $MFTMirr fallback
     * for records 0..3 lives INSIDE read_record (corpus bootstrap path). */
    u32 read_record(u64 record_no, std::vector<u8>& out);

    /* S-012 size-0 rescue: read one MFT record STRICTLY from the $MFTMirr
     * copy (mirror-first; read_record prefers the primary). Returns
     * ERR_RANGE_OUT_OF_BOUNDS when the mirror does not cover the record.
     * The og rescue chain reads MFT copies this way (FUN_18001e120). */
    u32 read_record_mirror(u64 record_no, std::vector<u8>& out);

    const VolumeParams& params() const { return p_; }
    u64 max_record_no() const {
        if (p_.record_size == 0)
            return 0;
        /* the record gate must cover the fragmented-MFT extents too:
         * the $ATTRIBUTE_LIST fragments extend the stream beyond the
         * primary $DATA run (session-5 audit row 3 - extent records were
         * unreachable before). Holes simply fail vcn_to_lcn downstream. */
        u64 covered = mft_run_.total_clusters();
        for (const DataRun& dr : mft_extents_)
            if (dr.vcn + dr.length > covered)
                covered = dr.vcn + dr.length;
        return covered * p_.cluster_bytes / p_.record_size;
    }

    u32 read_at(u64 offset, void* buf, u32 len) { return dev_->read_at(offset, buf, len); }

private:
    NtfsVolume() = default;
    u32 load_mft_stream();
    u32 load_mftmirr_stream();
    u32 enum_mirr_attrs(const std::vector<u8>& buf);

    std::unique_ptr<IStorageDevice> dev_;
    VolumeParams p_;
    RunList mft_run_;
    RunList mirr_run_;
    std::vector<DataRun> mft_extents_;
    bool mirr_valid_ = false;
    /* N-001: mirror service capacity in records, derived from the mirror's
     * own $DATA runlist span (og InitMFTMirror this+0x70; 0 when invalid) */
    u64 mirr_record_capacity_ = 0;
};

/* DBR validation: returns OK + fills params, or ERR_* on hard invalidity.
 * Sector-size sanity: 512 <= bps <= 4096 power of two; spc power of two;
 * record size >= 0x400 after resolution (corpus clamp); MFT LCN in range. */
u32 validate_boot_sector(const BootSector& bs, const Geometry& geom, VolumeParams& out);

} // namespace ntfs
} // namespace cr

#endif // CR_BOOTSTRAP_H_
