/*
 * cr_mftpatch.h - $MFT run-list reconstruction + record-0 in-place patch.
 *
 * W2/F-02 (cross-repo harvest): re-implemented from the documented
 * behavior of ntfs-recover (0BSD) @ 5072bbc7 - repairer.rs
 * reconstruct_fragments / patch_record_data_runs / data_runs.rs, battle-
 * tested on a 2TB volume (5,635,394 records; final patch 53 runs). No
 * code copied; spec: worklog/VALIDATION/specs/F-02-mft-runlist-patch.md.
 *
 * The corruption shape this closes: $MFT's own $DATA runlist is damaged
 * (record 0's unnamed non-resident $DATA run bytes) so the MFT appears
 * tiny or unreadable, while the records themselves still sit on disk.
 * A brute-force scan finds 'FILE'-magic records; delta-grouping their
 * (disk_offset, record#) pairs reconstructs the extent layout; a fresh
 * runlist is encoded sign-extension-safe (ntfs-3g decodes both fields
 * signed) and patched INTO record 0 - never a rebuild from scratch (the
 * reference's proven failure: a synthesized record 0 loses
 * $ATTRIBUTE_LIST + $BITMAP and mounts see only the first extent).
 *
 * Safety posture: this module is PURE - it computes and patches buffers;
 * it never opens devices. The caller (rawfixer S1-S7 transactional
 * execute) owns consent, undo snapshot, quiesce and verification. The
 * patch refuses (never truncates) when the new runs exceed the
 * attribute's run space.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_MFTPATCH_H_
#define CR_MFTPATCH_H_

#include "cr_ntfs_wire.h"
#include "../cr_types.h"
#include <vector>

namespace cr {
namespace ntfs {

/* One record found by the brute-force scan. */
struct FoundRecord {
    u64 disk_offset = 0;   /* byte offset of the record start on the volume */
    u64 record_number = 0; /* the record's own number field */
    u64 sequence = 0;      /* for dedup: highest sequence wins */
    u64 base_record = 0;   /* extension records (base != 0) are skipped */
};

/* One reconstructed MFT extent, in cluster units. */
struct MftFragment {
    u64 logical_start = 0; /* first MFT record slot (cluster-aligned) */
    u64 cluster_count = 0; /* clusters in the extent */
    u64 cluster_start = 0; /* volume LCN of the extent start */
    u64 delta = 0;         /* the group's disk_offset - record# * record_size */
};

/* Reconstructed-runlist result (analysis output; feeds the patch step). */
struct MftRunlistPlan {
    std::vector<MftFragment> fragments; /* sorted by logical_start, merged */
    u64 max_record = ~0ull;             /* cap from record 0's end_vcn
                                         * (record# of the last valid slot);
                                         * ~0 = uncapped (record 0 absent) */
    bool capped = false;                /* max_record derived from record 0 */
};

struct MftPatchStats {
    u64 scanned_records = 0;   /* input records after base-record skip */
    u64 unique_records = 0;    /* after dedup by record number */
    u64 delta_groups = 0;      /* distinct deltas observed */
    u64 dropped_tiny = 0;      /* fragments dropped: < 4 clusters */
    u64 dropped_unaligned = 0; /* fragments dropped: not cluster-aligned */
    u64 dropped_negative = 0;  /* fragments dropped: pre-volume offset */
    u64 truncated_to_cap = 0;  /* fragments truncated at the VCN cap */
    u64 merged = 0;            /* fragments merged into a predecessor */
    u64 overlap_skipped = 0;   /* fragments fully contained in a predecessor */
};

/*
 * reconstruct_mft_runlist - delta-grouping fragment reconstruction.
 *
 * INPUT: plan.max_record carries the VCN cap when the caller derived one
 * from a usable record 0 (derive_vcn_cap_from_record0); leave it ~0ull
 * for uncapped reconstruction (record 0 absent from the scan). The cap
 * is echoed as plan.capped.
 *
 * The delta invariant: within one physical MFT extent, disk offsets are
 * linear in record number, so delta = disk_offset - record# * record_size
 * is constant per extent. Grouping by delta recovers the extents even
 * with gaps (missing/overwritten records do not split a group - only the
 * group's min/max matter).
 *
 * Post-grouping: floor/ceil cluster-align the record span, require
 * cluster-aligned extent starts, drop < 4-cluster fragments (stale
 * records from a previous layout), cap to record 0's VCN range when the
 * scan includes a usable record 0 (preserves $ATTRIBUTE_LIST/$BITMAP
 * consistency - the total cluster count must match the preserved
 * end_vcn), then sort + merge (contiguous both logically and physically
 * merge; overlaps trim/skip).
 *
 * geometry: cluster_bytes and record_size from the boot sector;
 * min_fragment_clusters is the drop threshold (reference: 4).
 */
void reconstruct_mft_runlist(const std::vector<FoundRecord>& records, u64 cluster_bytes,
                             u64 record_size, u64 min_fragment_clusters, MftRunlistPlan& plan,
                             MftPatchStats& stats);

/* Set plan.max_record from a record-0 buffer: parses its attributes,
 * finds the unnamed non-resident $DATA, max_record = (end_vcn+1) *
 * (cluster_bytes/record_size) - 1. Returns false when record 0 carries
 * no usable $DATA (resident or absent) - the plan stays uncapped. */
bool derive_vcn_cap_from_record0(const u8* record0, u32 record_bytes, u64 cluster_bytes,
                                 u64 record_size, u64& max_record);

/*
 * encode_runlist_ntfs3g_safe - sign-extension-safe runlist encoding.
 *
 * ntfs-3g decodes BOTH the length and offset fields by sign-extending
 * the most-significant byte (ntfs_mapping_pairs_decompress). A length
 * whose top byte has bit 7 set decodes negative and the mount fails
 * EIO. The encoder therefore sizes both fields as if signed, sparing
 * the sign bit: bytes = ceil((bit_length + 1) / 8). Offsets are signed
 * deltas from the previous run's LCN (first run relative to 0); a
 * sparse run carries a zero offset width.
 *
 * fragments: sorted by logical_start; returns the encoded byte string
 * (runs + 0x00 terminator). run count is bounded by the fragment count.
 */
std::vector<u8> encode_runlist_ntfs3g_safe(const std::vector<MftFragment>& fragments,
                                           u64 records_per_cluster);

/*
 * patch_record0_runlist - the in-place record-0 patch.
 *
 * Steps (on the caller's record buffer, record_bytes == record_size):
 *  1. verify 'FILE' magic;
 *  2. un-fixup (USA): validate each sector tail against the USA number,
 *     restore the original words;
 *  3. locate the unnamed non-resident $DATA attribute;
 *  4. REFUSE (ERR_REPAIR_REFUSED) when the encoded runs exceed the
 *     attribute's run space - the record is never restructured;
 *  5. zero the run space, copy the new runs in;
 *  6. re-fixup with a bumped USA number (wrapping, 0xFFFE -> 0x0001 -
 *     never 0x0000/0xFFFF);
 *  7. return OK; the caller writes the buffer back and snapshots it
 *     into the undo log FIRST (S4).
 *
 * Everything else in the record - every other attribute, the header
 * fields, start_vcn/end_vcn/allocated/real/initialized sizes - is
 * preserved byte-for-byte.
 *
 * Returns OK / ERR_BAD_PARAMETER (not a FILE record / no $DATA) /
 * ERR_REPAIR_REFUSED (runs do not fit the attribute's run space).
 */
u32 patch_record0_runlist(u8* record, u32 record_bytes, u32 sector_bytes,
                          const std::vector<u8>& encoded_runs);

} // namespace ntfs
} // namespace cr

#endif // CR_MFTPATCH_H_
