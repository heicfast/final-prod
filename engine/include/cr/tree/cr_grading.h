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
 *      FS not in {NTFS,    -> generic path: extension NOT in the 169-entry
 *      ReFS, Btrfs}           table gates the twin detection (S-058:
 *                             FUN_1401bebe0 = iterator inequality, the
 *                             :605743 block runs on the lookup MISS;
 *                             the runtime set's "" quirk makes dot-less
 *                             files skip too): two
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
#include "../storage/cr_device.h" /* M-134: GradeReader's IStorageDevice */
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

/* M-134 (diff-to-fix): the deep-read grade validation's bounded reader
 * (the og resolves one through its global manager chain,
 * FUN_14038c470 :606319-331). Optional: absent (dev == nullptr) the
 * content rules are skipped exactly as they were pre-M-134 - the
 * structural rules never depended on it. The scan threads the device
 * at the completion pass; the session-regrade ABI (device-decoupled by
 * design) runs the rule set without the content half, the registered
 * seam noted in the M-134 Resolution. */
struct GradeReader {
    IStorageDevice* dev = nullptr; /* the device the extents are rebased to */
    u32 sector_size = 0;           /* 0 = derive from dev->geometry() */
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

    /* M-134: the optional bounded reader for the deep-read content
     * rules (one sector per node, the not-in-table no-twin population
     * only). */
    GradeReader reader;

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
 *
 * raw_count semantics (N-205, registered deliberate divergence D-146 [renumbered D-143->D-144->D-145->D-146 across three push-rebases: the racing windows' N-196/N-198/N-202 registrations keep the lower seats per SA-9]):
 * the og's rawCount (+0x1a0) is a VALIDATED-RECOVERABLE count - the
 * forced observe-only deep-read probe over the grade-1 NTFS population,
 * ANDed with the size rule (FUN_1401c76e0: the 0x23 ctor-default arm ->
 * size <= 0x77fffff) - ticked at quick-scan populate (FUN_1401c7d80,
 * DRWUI:331183-331311). The core's raw_count keeps the landed
 * Origin::Carved census (the report field's shipped meaning); the
 * og-parity population is carried by raw_tail_zero below.
 */
struct AssistantStats {
    u64 total_file_count = 0;
    u64 raw_count = 0;
    /* N-205: the og's rawCount population - the forced observe-only
     * tail-zero probe (FUN_14038c470's param_3=1 form, no grade change)
     * over grade-1 NTFS-family nodes with 0 < key < device-size, ANDed
     * with the 0x77fffff size arm. Core-internal counter (the C ABI
     * surface keeps its shipped field set; an append-only exposure rides
     * a future ABI bump if a finding demands it). */
    u64 raw_tail_zero = 0;
    u64 lost_partition_exist_file_count = 0;
    u64 lost_partition_delete_file_count = 0;
    u64 quick_scan_exist_file_count = 0;
    u64 quick_scan_delete_file_count = 0;
    u64 deep_scan_file_count = 0;
    u64 quick_scan_delete_extend = 0;
    /* N-183 (diff-to-fix): the og's DAT_141be5818 tick is the gauge-matched
     * global, NOT QSDE - the NTFS fixer-index rule's true counter (the
     * finding's sub-gap (c)). */
    u64 ntfs_device_index_high = 0;
    /* category counts, index = query::Category - 1 (Image..Bookmarks);
     * the og's 6 counted IDs map onto our 8, Bookmarks/None land in the
     * Other bucket (og 0xB) exactly like the fallback category. */
    u64 category_counts[8] = {0};
    /* the recoverability gauge + small-result heuristic (total < 100 ->
     * matched forced 0; the og never shows the gauge for tiny results) */
    u64 matched = 0;
    u64 total = 0;
    bool small_result = false;
    /* N-183 (diff-to-fix): the og's gauge population is the 20-format
     * member set (the .rdata ctor map at DRWUI:605502-513) plus the
     * auto-High classes (email + ReFS/Btrfs non-existing, S-061) - not
     * every file; and the verdict copy renders a 0-10 score via
     * int((matched/total + 0.05) * 10) (FUN_14021b030 :374451-481). */
    u64 gauge_matched = 0; /* grade-High within the member-set population */
    u64 gauge_total = 0;   /* the member set + the auto-High classes */
    u32 score = 0;         /* int((matched/total + 0.05) * 10), total==0 -> 0 */
    u64 graded_count = 0;  /* nodes the pass visited (telemetry) */
};

/* Run the full grading pass over a completed scan tree. Idempotent for
 * a fresh tree (all grades start Average); re-running obeys the og's
 * monotonic semantics (grade-2 stays 2). Fills `out` with the assistant
 * statistics. The pass is single-threaded by design: it runs on the
 * post-scan stable tree (the og's two background jobs are an
 * implementation detail of its message-pump architecture). */
void grade_tree(tree::ResultTree& t, const GradeContext& ctx, AssistantStats& out);

/* Per-selection re-grade (FUN_1401d7110): re-run the decision rules on
 * ONE node. Only grade-1 (Average) nodes with the graded bit CLEAR are
 * eligible - the og's one-shot re-grade gate (grade==1 && graded-bit
 * clear, :340852-873). M-134: the re-grade now re-runs the FULL
 * per-node tree shape (solo rules + the S-058 table gate + the twin
 * lookup + the deep read) instead of the solo rules alone; pass the
 * tree to enable the twin lookup (the og's FUN_14038ac90 can set
 * ANOTHER node's grade through the twin map). Returns true when the
 * node was re-graded. */
bool regrade_node(tree::FileNode* n, const GradeContext& ctx,
                  const tree::ResultTree* tree = nullptr);

/* The 169-entry deep-validation extension table (dumped from the corpus
 * DRWUI.exe .data 0x141bb0500; first key "Raw", every flag byte 0).
 * NON-membership enters the generic path's twin detection (S-058: the
 * og's gate runs the deep block on the lookup MISS); the empty key is
 * skipped by the call site (the og's runtime set carries the
 * ctor-inserted "" key, so dot-less names skip too). */
bool extension_in_grading_table(const std::string& ext_utf8_lower);
size_t grading_table_entry_count();

} // namespace grading
} // namespace tree
} // namespace cr

#endif // CR_GRADING_H_
