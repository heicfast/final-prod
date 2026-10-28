/*
 * cr_grading.h - the post-scan CRecoverability grading pass (S-023).
 *
 * Corpus model (DRWUI CRecoverability, decoded in
 * diff-to-fix/evidence/decompiled-quotes/s4-crecoverability-grading-engine.md;
 * all cites below are DRWUI_decompiled.c unless noted):
 *
 *  - engines emit grade=1 only (NTFSSearchDLL :39242/:39779 - the ctor
 *    default 0x20 = grade 1 in bits 5-7 of node+0x73); every 0/2 verdict
 *    is computed UI-side at scan completion (OQ-S2M-1 resolution);
 *  - setter semantics: monotonic-max, grade-2 sticky (FUN_140089630);
 *    the overlap flag (bit 2) is set together with grade >= 2
 *    (FUN_1400895b0);
 *  - decision tree (FUN_14038ac90 + the device-walk job FUN_14038b0e0):
 *      size < 1            -> grade 0
 *      category == email   -> grade 2   (og category 0xC: the mapper
 *                                         FUN_14039aa40 rewrites type 6
 *                                         -> 0xC on bit 35 of node+0x89)
 *      existing file       -> grade 2 at insert if size >= 1
 *                             (FUN_14037a8a0 family, bit 6 of +0x72)
 *      raw/carve device    -> size < 0x400 -> 2 else 0  (FUN_14038b0e0
 *                             vt+0xe8 branch: small carved results are
 *                             intact, large ones presumptively partial)
 *      NTFS device walk    -> device index in [0,9) -> grade 2 and the
 *                             QuickScanDeleteExtend counter ticks
 *                             (DAT_141be5818); vt+0xb8/vt+0x100 sub-rules
 *                             are UNDECODED (see Resolution notes)
 *      ReFS / Btrfs        -> grade 2 at insert, unconditional - the
 *                             insert-time hook FUN_14038a470 :605481-497
 *                             (kind 0xB/10 -> FUN_140089630(node,2) +
 *                             the gauge ticks DAT_141be5830/5828); S-061
 *      FS not in {NTFS,    -> generic path: extension NOT in the
 *      ReFS, Btrfs}           169-entry table (dumped from .data
 *                             0x141bb0500, first key "Raw"; the S-058
 *                             polarity: FUN_1401bebe0 is the inequality
 *                             comparator, "" is in the runtime set so
 *                             dot-less skips) gates the twin detection:
 *                             two
 *                             nodes with the same extent-end key
 *                             (base + len + device_index*unit, the
 *                             index offset by -2 for FAT-family devices
 *                             - an ADDRESS offset, never a grade change)
 *                             and the same category -> both grade 2 +
 *                             overlap flags (FUN_14038ba60)
 *  - per-selection re-grade: grade-1 nodes re-run the rules when the
 *    user selects/previews them (FUN_1401d7110);
 *  - small-result heuristic: total < 100 -> matched forced 0;
 *  - RecoveryAssistant statistics: 8 counters + 6 per-category counts
 *    (FUN_1401c6f10, IDs 5/6/7/8/9/0xB = Graphics/Document/Archive/
 *    Video/Audio/Other).
 *
 * The core's in-walk 5-tier Confidence is a DIFFERENT decision system
 * (allocation axis) and stays untouched; this pass adds the og's model
 * as a separate, monotonic field (tree::Grade).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_GRADING_H_
#define CR_GRADING_H_

#include "cr_node.h"
#include "../query/cr_query.h"
#include <string>
#include <vector>

namespace cr {
namespace tree {
namespace grading {

/* The og's per-scan device slot: the scan's device list position (the
 * NTFS index rule's [0,9) domain) + the FS that owns the byte range. */
struct DeviceSlot {
    u64 base = 0;         /* slice base, absolute device bytes */
    u32 disk_ordinal = 0; /* position in the scan's device list */
    u32 fs_kind = 0;      /* FsKindCode (cr_types.h) */
};

/* The extent-end twin-key unit (the og's local_b8[0], vt-derived and
 * undecoded): a constant larger than any addressable disk so per-device
 * keys never collide across devices. The og's structural intent - same
 * device + same extent end -> twins - is preserved exactly. */
constexpr u64 kDeviceUnit = 1ull << 40;

struct GradeContext {
    /* The scan's slices, sorted by base. Empty = single implicit disk 0
     * (synthetic fixtures, whole-disk scans with no slice table). */
    std::vector<DeviceSlot> devices;

    const DeviceSlot* slot_for(u64 abs_offset) const;
    /* The FS that owns a node: props->fs_kind, or the owning slot's kind
     * when the node carries extents (the slice knows the truth better). */
    u32 fs_kind_of(const tree::FileNode* n) const;
    u32 disk_ordinal_of(const tree::FileNode* n) const;
};

/* RecoveryAssistant statistics (FUN_1401c6f10's 8 counters + the 6
 * per-category counts + the recoverability gauge). Counter mapping to
 * the core's model (documented divergence - the og's quick/deep stage
 * attribution is DRWUI-internal; our single-pass engine attributes by
 * Origin x DeletedState + the deep-surface "Lost Files" roots):
 *   total_file_count          = every file node
 *   raw_count                 = Origin::Carved
 *   lost_partition_*          = Origin::Lost (exist = Active, delete != Active)
 *   quick_scan_exist/delete   = Origin::Structured (exist = Active, delete != Active)
 *   deep_scan_file_count      = nodes under lost roots ("File Path Lost",
 *                             the legacy "Lost Files" role included; the
 *                               remnant-salvage surface: INDX harvest, GUID
 *                               dump, size-0 rescue, freed-node sweeps)
 *   quick_scan_delete_extend  = the NTFS device-index rule's tick (exact)
 */
struct AssistantStats {
    u64 total_file_count = 0;
    u64 raw_count = 0;
    u64 lost_partition_exist_file_count = 0;
    u64 lost_partition_delete_file_count = 0;
    u64 quick_scan_exist_file_count = 0;
    u64 quick_scan_delete_file_count = 0;
    u64 deep_scan_file_count = 0;
    u64 quick_scan_delete_extend = 0;
    /* category counts, index = query::Category - 1 (Image..Bookmarks);
     * the og's 6 counted IDs map onto our 8, Bookmarks/None land in the
     * Other bucket (og 0xB) exactly like the fallback category. */
    u64 category_counts[8] = {0};
    /* the recoverability gauge + small-result heuristic (total < 100 ->
     * matched forced 0; the og never shows the gauge for tiny results) */
    u64 matched = 0;
    u64 total = 0;
    bool small_result = false;
    u64 graded_count = 0; /* nodes the pass visited (telemetry) */
};

/* Run the full grading pass over a completed scan tree. Idempotent for
 * a fresh tree (all grades start Average); re-running obeys the og's
 * monotonic semantics (grade-2 stays 2). Fills `out` with the assistant
 * statistics. The pass is single-threaded by design: it runs on the
 * post-scan stable tree (the og's two background jobs are an
 * implementation detail of its message-pump architecture). */
void grade_tree(tree::ResultTree& t, const GradeContext& ctx, AssistantStats& out);

/* Per-selection re-grade (FUN_1401d7110): re-run the decision rules on
 * ONE node. Only grade-1 (Average) nodes are eligible - the og's
 * re-grade gate (grade not in {0, 2}). Returns true when the node was
 * re-graded. */
bool regrade_node(tree::FileNode* n, const GradeContext& ctx);

/* The 169-entry deep-validation extension table (dumped from the corpus
 * DRWUI.exe .data 0x141bb0500; first key "Raw", every flag byte 0).
 * Membership gates the generic path's twin detection. */
bool extension_in_grading_table(const std::string& ext_utf8_lower);
size_t grading_table_entry_count();

} // namespace grading
} // namespace tree
} // namespace cr

#endif // CR_GRADING_H_
