/* cr_mftpatch.cpp - $MFT run-list reconstruction + record-0 in-place patch.
 *
 * W2/F-02: behavioral re-implementation of ntfs-recover's repair path
 * (0BSD, @5072bbc7; spec worklog/VALIDATION/specs/F-02-mft-runlist-patch.md).
 * See cr_mftpatch.h for the full contract.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/ntfs/cr_mftpatch.h"
#include "cr/ntfs/cr_attributes.h"
#include "cr/ntfs/cr_fixup.h"
#include "cr/ntfs/cr_runlist.h"
#include <algorithm>
#include <cstring>
#include <map>
#include <unordered_map>

namespace cr {
namespace ntfs {

namespace {

u16 rd16(const u8* p) {
    return u16(p[0]) | (u16(p[1]) << 8);
}
void wr16(u8* p, u16 v) {
    p[0] = u8(v);
    p[1] = u8(v >> 8);
}

/* bit length of an unsigned value (0 -> 0). */
u32 bit_length(u64 v) {
    u32 b = 0;
    while (v) {
        ++b;
        v >>= 1;
    }
    return b;
}

/* Signed-field byte count that spares the sign bit (ntfs-3g decodes
 * both runlist fields sign-extended): ceil((bits + 1) / 8), clamped to
 * [1, 8]. For a non-negative value the top byte's bit 7 stays clear. */
u32 signed_safe_width(u64 value) {
    const u32 bits = bit_length(value);
    const u32 bytes = (bits + 1 + 7) / 8;
    return std::min<u32>(8u, std::max<u32>(1u, bytes));
}

/* Byte count for a signed offset delta (two's complement): positive
 * values as in signed_safe_width; negative values sized from the
 * leading-ones run. */
u32 signed_delta_width(i64 delta) {
    if (delta >= 0)
        return signed_safe_width(u64(delta));
    /* negative: how many bytes represent this exact value with the sign
     * bit set and correct sign-extension? ceil((significant_bits+1)/8)
     * where significant bits count from the highest set bit of the
     * complement (the leading ones of the original). */
    const u64 mag = ~u64(delta); /* count leading ones == bit_length(mag) */
    const u32 bits = bit_length(mag);
    const u32 bytes = (bits + 1 + 7) / 8;
    return std::min<u32>(8u, std::max<u32>(1u, bytes));
}

struct GroupAccum {
    u64 first_mft = ~0ull;
    u64 last_mft = 0;
    u64 delta = 0;
};

} // namespace

void reconstruct_mft_runlist(const std::vector<FoundRecord>& records, u64 cluster_bytes,
                             u64 record_size, u64 min_fragment_clusters, MftRunlistPlan& plan,
                             MftPatchStats& stats) {
    /* plan.max_record is INPUT here: the caller derives the VCN cap from
     * a usable record 0 (derive_vcn_cap_from_record0) BEFORE calling,
     * or leaves it ~0 for uncapped reconstruction (record 0 absent).
     * plan.fragments is cleared; the cap is echoed back as `capped`. */
    const u64 cap = plan.max_record;
    const bool have_cap = cap != ~0ull;
    plan.fragments.clear();
    plan.capped = false;
    stats = MftPatchStats{};

    if (cluster_bytes == 0 || record_size == 0 || records.empty())
        return;

    /* Step 1: filter extension records + dedup by record number keeping
     * the highest sequence. */
    std::unordered_map<u64, const FoundRecord*> unique;
    unique.reserve(records.size() * 2);
    for (const auto& r : records) {
        if (r.base_record != 0)
            continue; /* extension record: belongs to another base */
        if (have_cap && r.record_number > cap)
            continue;
        auto it = unique.find(r.record_number);
        if (it == unique.end()) {
            unique.emplace(r.record_number, &r);
        } else if (r.sequence >= it->second->sequence) {
            it->second = &r;
        }
    }
    stats.scanned_records = records.size();
    stats.unique_records = unique.size();
    if (unique.empty())
        return;

    /* Step 2: group by delta = disk_offset - record# * record_size. */
    std::map<i64, GroupAccum> groups;
    for (const auto& [recno, rp] : unique) {
        const i64 delta =
            i64(rp->disk_offset) - i64(rp->record_number * record_size); /* no overflow at
                                                                          * sane geometries;
                                                                          * i64 range covers
                                                                          * 8 EiB offsets */
        GroupAccum& g = groups[delta];
        if (rp->record_number < g.first_mft) {
            g.first_mft = rp->record_number;
        }
        if (rp->record_number > g.last_mft) {
            g.last_mft = rp->record_number;
        }
        g.delta = u64(delta);
    }
    stats.delta_groups = groups.size();

    const u64 rpc = cluster_bytes / record_size; /* records per cluster */
    if (rpc == 0)
        return; /* geometry nonsense (record larger than cluster) */

    /* Step 3: fragment creation per group. */
    std::vector<MftFragment> frags;
    for (const auto& [delta, g] : groups) {
        if (g.first_mft > g.last_mft)
            continue;
        const u64 logical_start = g.first_mft - (g.first_mft % rpc); /* floor-align */
        const u64 logical_end_excl = ((g.last_mft / rpc) + 1) * rpc; /* ceil-align */
        if (logical_end_excl <= logical_start)
            continue;
        const u64 cluster_count = (logical_end_excl - logical_start) / rpc;
        const i64 disk_start = i64(delta) + i64(logical_start * record_size);
        if (disk_start < 0) {
            ++stats.dropped_negative;
            continue;
        }
        const u64 disk_start_u = u64(disk_start);
        if (disk_start_u % cluster_bytes != 0) {
            ++stats.dropped_unaligned;
            continue;
        }
        MftFragment f;
        f.logical_start = logical_start;
        f.cluster_count = cluster_count;
        f.cluster_start = disk_start_u / cluster_bytes;
        f.delta = u64(delta);
        frags.push_back(f);
    }

    /* Step 4: drop tiny fragments (stale records of a previous layout). */
    {
        std::vector<MftFragment> kept;
        kept.reserve(frags.size());
        for (auto& f : frags) {
            if (f.cluster_count < min_fragment_clusters) {
                ++stats.dropped_tiny;
                continue;
            }
            kept.push_back(f);
        }
        frags.swap(kept);
    }

    /* Step 5: sort by logical_start, cap to record 0's VCN range. */
    std::sort(frags.begin(), frags.end(), [](const MftFragment& a, const MftFragment& b) {
        return a.logical_start < b.logical_start;
    });
    if (have_cap) {
        const u64 max_logical_end = cap + 1;
        plan.capped = true;
        std::vector<MftFragment> capped;
        capped.reserve(frags.size());
        for (auto& f : frags) {
            const u64 frag_end = f.logical_start + f.cluster_count * rpc;
            if (frag_end > max_logical_end) {
                const u64 room =
                    max_logical_end > f.logical_start ? max_logical_end - f.logical_start : 0;
                const u64 new_count = (room + rpc - 1) / rpc; /* ceil */
                if (new_count == 0) {
                    ++stats.dropped_tiny;
                    continue;
                }
                if (new_count < f.cluster_count) {
                    ++stats.truncated_to_cap;
                    f.cluster_count = new_count;
                }
            }
            capped.push_back(f);
        }
        frags.swap(capped);
    }

    /* Step 6: merge - contiguous both logically and physically extends;
     * overlaps trim or skip; gaps append. */
    std::vector<MftFragment>& out = plan.fragments;
    out.reserve(frags.size());
    for (auto& f : frags) {
        if (!out.empty()) {
            MftFragment& prev = out.back();
            const u64 prev_end_logical = prev.logical_start + prev.cluster_count * rpc;
            const u64 prev_end_cluster = prev.cluster_start + prev.cluster_count;
            if (f.logical_start == prev_end_logical && f.cluster_start == prev_end_cluster) {
                prev.cluster_count += f.cluster_count; /* contiguous: extend */
                ++stats.merged;
                continue;
            }
            if (f.logical_start < prev_end_logical) {
                /* overlap: the newcomer starts inside the predecessor.
                 * The part beyond prev_end_logical is appended (physical
                 * cluster start advances by the overlap); a fully
                 * contained newcomer is skipped. */
                const u64 overlap_clusters = (prev_end_logical - f.logical_start) / rpc;
                if (f.cluster_count > overlap_clusters) {
                    MftFragment trimmed = f;
                    trimmed.logical_start = prev_end_logical;
                    trimmed.cluster_start += overlap_clusters;
                    trimmed.cluster_count -= overlap_clusters;
                    out.push_back(trimmed);
                } else {
                    ++stats.overlap_skipped;
                }
                continue;
            }
        }
        out.push_back(f);
    }
}

bool derive_vcn_cap_from_record0(const u8* record0, u32 record_bytes, u64 cluster_bytes,
                                 u64 record_size, u64& max_record) {
    if (record0 == nullptr || record_bytes < 0x400 || cluster_bytes == 0 || record_size == 0)
        return false;
    RecordHeader rh;
    std::memcpy(&rh, record0, sizeof(rh));
    if (rh.magic != RECORD_MAGIC_FILE)
        return false;
    std::vector<AttrView> attrs;
    if (enum_attributes(record0, record_bytes, attrs) != OK)
        return false;
    const u64 rpc = cluster_bytes / record_size;
    if (rpc == 0)
        return false;
    for (const auto& a : attrs) {
        if (a.type != ATTR_TYPE_DATA || !a.nonresident || a.name_length != 0)
            continue;
        if (a.last_vcn == 0)
            return false; /* no meaningful cap */
        max_record = (a.last_vcn + 1) * rpc - 1;
        return true;
    }
    return false;
}

std::vector<u8> encode_runlist_ntfs3g_safe(const std::vector<MftFragment>& fragments,
                                           u64 records_per_cluster) {
    (void)records_per_cluster; /* fragment cluster counts are already in
                                * cluster units; the parameter documents
                                * the unit contract at call sites */
    std::vector<u8> out;
    out.reserve(fragments.size() * 8 + 1);
    u64 prev_lcn = 0;
    for (const auto& f : fragments) {
        const u64 len_width = signed_safe_width(f.cluster_count);
        const i64 delta = i64(f.cluster_start) - i64(prev_lcn);
        const u32 off_width = signed_delta_width(delta);
        out.push_back(u8((off_width << 4) | len_width));
        /* length: unsigned LE, top byte's sign bit spared by len_width */
        for (u32 i = 0; i < len_width; ++i)
            out.push_back(u8(f.cluster_count >> (8 * i)));
        /* offset: two's-complement LE of the signed delta */
        const u64 enc = u64(delta);
        for (u32 i = 0; i < off_width; ++i)
            out.push_back(u8(enc >> (8 * i)));
        prev_lcn = f.cluster_start;
    }
    out.push_back(0); /* terminator */
    return out;
}

u32 patch_record0_runlist(u8* record, u32 record_bytes, u32 sector_bytes,
                          const std::vector<u8>& encoded_runs) {
    if (record == nullptr || record_bytes < 0x400 || sector_bytes == 0)
        return ERR_BAD_PARAMETER;

    RecordHeader rh;
    std::memcpy(&rh, record, sizeof(rh));
    if (rh.magic != RECORD_MAGIC_FILE)
        return ERR_BAD_PARAMETER;

    /* Un-fixup: validate + apply (restores each sector tail's original
     * word from its USA slot). fixup_apply validates first and rolls
     * back on mismatch - exactly the reference's warn-and-continue is
     * NOT taken here: a mismatched USA means the record is more damaged
     * than the patch path assumes, and a refused repair is safer than a
     * patch over a torn fixup (S3 ambiguity-refusal). */
    if (rh.update_sequence_size == 0)
        return ERR_BAD_PARAMETER;
    if (u32 r = fixup_validate(record, record_bytes, rh.update_sequence_offset,
                               rh.update_sequence_size, sector_bytes);
        r != OK)
        return r;
    if (u32 r = fixup_apply(record, record_bytes, rh.update_sequence_offset,
                            rh.update_sequence_size, sector_bytes);
        r != OK)
        return r;

    /* Locate the unnamed non-resident $DATA attribute. */
    std::vector<AttrView> attrs;
    if (enum_attributes(record, record_bytes, attrs) != OK)
        return ERR_BAD_PARAMETER;
    const AttrView* data = nullptr;
    for (const auto& a : attrs) {
        if (a.type == ATTR_TYPE_DATA && a.nonresident && a.name_length == 0) {
            data = &a;
            break;
        }
    }
    if (data == nullptr || data->at == nullptr)
        return ERR_BAD_PARAMETER;

    /* Run space: from the attribute's runlist offset to its end. */
    const u64 attr_pos = u64(data->at - record);
    const u64 runs_start = attr_pos + data->runlist_offset;
    const u64 attr_end = attr_pos + data->record_length;
    if (data->runlist_offset == 0 || runs_start >= attr_end || attr_end > record_bytes)
        return ERR_BAD_PARAMETER;
    const u64 runs_space = attr_end - runs_start;
    if (encoded_runs.empty() || encoded_runs.size() > runs_space)
        return ERR_REPAIR_REFUSED; /* never restructure the record */

    /* Zero the run space (redundant termination) and copy the new runs. */
    std::memset(record + runs_start, 0, size_t(runs_space));
    std::memcpy(record + runs_start, encoded_runs.data(), encoded_runs.size());

    /* Re-fixup with a bumped USA number: +1 wrapping, 0xFFFE -> 0x0001
     * (never 0x0000 / 0xFFFF). Save each sector's current tail word into
     * its USA slot, then stamp the new number at every tail. */
    u16 new_sig = rd16(record + rh.update_sequence_offset);
    new_sig = u16(new_sig + 1);
    if (new_sig == 0x0000 || new_sig == 0xFFFF)
        new_sig = 0x0001;
    const u64 num_sectors = record_bytes / sector_bytes;
    for (u64 i = 0; i < num_sectors; ++i) {
        const u64 tail = (i + 1) * sector_bytes - 2;
        if (tail + 2 > record_bytes)
            return ERR_BAD_PARAMETER;
        const u16 tail_word = rd16(record + tail);
        wr16(record + rh.update_sequence_offset + 2 + i * 2, tail_word);
    }
    wr16(record + rh.update_sequence_offset, new_sig);
    for (u64 i = 0; i < num_sectors; ++i) {
        const u64 tail = (i + 1) * sector_bytes - 2;
        wr16(record + tail, new_sig);
    }
    return OK;
}

} // namespace ntfs
} // namespace cr
