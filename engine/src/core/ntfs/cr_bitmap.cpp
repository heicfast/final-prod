/* cr_bitmap.cpp - $Bitmap load + cluster tests.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/ntfs/cr_bitmap.h"
#include "cr/ntfs/cr_fixup.h"
#include "cr/ntfs/cr_attributes.h"
#include <cstring>

namespace cr {
namespace ntfs {

u32 VolumeBitmap::load(NtfsVolume *vol, std::unique_ptr<VolumeBitmap> &out) {
    if (vol == nullptr) return ERR_BAD_PARAMETER;
    const u64 cluster_bytes = vol->params().cluster_bytes;
    std::vector<u8> rec;
    u32 rc = vol->read_record(6, rec); /* $Bitmap = MFT record 6 */
    if (rc != OK) return rc;

    RecordHeader rh;
    std::memcpy(&rh, rec.data(), sizeof(rh));
    if (rh.magic != RECORD_MAGIC_FILE) return ERR_BAD_MAGIC;
    rc = fixup_apply(rec.data(), u32(rec.size()), rh.update_sequence_offset,
                     rh.update_sequence_size, vol->params().sector_bytes);
    if (rc != OK) return rc;

    std::vector<AttrView> attrs;
    rc = enum_attributes(rec.data(), u32(rec.size()), attrs);
    if (rc != OK) return rc;

    /* assemble the bitmap stream across $DATA fragments (incl. attrlist) */
    std::vector<std::vector<u8>> pieces;
    for (const AttrView &a : attrs) {
        if (a.type != ATTR_TYPE_DATA) continue;
        if (!a.nonresident) {
            const u8 *d = nullptr;
            u32 dlen = 0;
            if (resident_span(a, d, dlen) == OK && dlen > 0)
                pieces.emplace_back(d, d + dlen);
            continue;
        }
        RunList rl;
        if (parse_data_runs(a, vol->params().total_clusters, rl) != OK) continue;
        for (const DataRun &r : rl.runs()) {
            if (r.sparse) continue;
            u64 byte_off = r.lcn * cluster_bytes;
            u64 byte_len = r.length * cluster_bytes;
            std::vector<u8> buf;
            buf.resize(size_t(byte_len));
            u32 prc = vol->read_at(byte_off, buf.data(), u32(byte_len));
            if (prc != OK) continue; /* partial bitmap beats no bitmap */
            pieces.push_back(std::move(buf));
        }
    }
    if (pieces.empty()) return ERR_BAD_PARAMETER;

    auto bm = std::unique_ptr<VolumeBitmap>(new VolumeBitmap());
    u64 total = 0;
    for (const auto &p : pieces) total += p.size();
    bm->buffer_.reserve(size_t(total));
    for (const auto &p : pieces)
        bm->buffer_.insert(bm->buffer_.end(), p.begin(), p.end());
    bm->bit_count_ = vol->params().total_clusters;
    if (bm->buffer_.size() > (bm->bit_count_ + 7) / 8)
        bm->buffer_.resize(size_t((bm->bit_count_ + 7) / 8)); /* clamp to volume */
    out = std::move(bm);
    return OK;
}

double VolumeBitmap::reused_ratio(const RunList &runs) const {
    u64 total = 0, reused = 0;
    for (const DataRun &r : runs.runs()) {
        if (r.sparse) { total += r.length; reused += r.length; continue; }
        for (u64 c = r.lcn; c < r.lcn + r.length; ++c) {
            ++total;
            if (test(c)) ++reused;
        }
    }
    return total == 0 ? 0.0 : double(reused) / double(total);
}

} // namespace ntfs
} // namespace cr
