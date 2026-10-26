/*
 * cr_scoring.h - the lost-partition candidate scoring model (M-017):
 * the og's dSuggested + AdjustPartitionInfoOrder, clean-room from the
 * corpus decode.
 *
 * Corpus ground truth (all lead-verified, numerically validated against the
 * vendor's own CalcPartition.log to <= 2e-5):
 *  - DRWUI_decompiled.c:842164-842279 (FUN_14050fda0): raw count per
 *    candidate = per-FS weighted file/folder count + the per-candidate
 *    integer bonus (+0x68) + 500 on a known-partition-table match;
 *    denominator = log(max raw over the scan) (natural log, 1.0 when the
 *    max is <= 1); score = (raw > 1 ? log(raw) : raw) / denom.
 *  - FS weights (FS code = the og's enum, FUN_140510080 :842341-842391):
 *    code 8 (HFS+) -> files + folders*10; codes 1/2 (FAT16/32) ->
 *    (files + folders) * 50; everything else (NTFS 3, exFAT 7, ext, ReFS,
 *    RAW, unknown, FAT12 0 - the unsigned FS-1<2 edge) -> files + folders*50.
 *  - Bonus +0x68: ctor default 1 (FUN_1405aa6a0 :942638); observed in the
 *    QA log as 0 (NTFS rows) and 20 (FAT32 rows), one ~60 row; the producer
 *    decode is tracked as OQ-S2M-2. Defaults here reproduce the observed
 *    rows: FAT family 20, everything else 0 - overridable per call.
 *  - Table match (+500): candidate matches a known-partition-array entry
 *    on FS AND start sector (the og's stride-0x18 compare, :842260-842267).
 *  - AdjustPartitionInfoOrder (FUN_140510410 :842495-842859):
 *    (a) known-extent containment drop (:842305-842306),
 *    (b) top-2 tie rules - score gap <= 0.1 drops #2, <= 0.3 drops #2
 *        when #2 is FAT16/32 (:842752-842769),
 *    (c) size-match promotion - the FIRST size-matched candidate at
 *        list position >= 1 (|totalSector - reference|*512 < 16 MiB+1,
 *        :842770-842796) is marked promoted, and when it sits at position
 *        >= 2 it is spliced to position #2 with the FOLLOWING candidate's
 *        score copied into it (LAB_140510a0e :842843-842858 - the log
 *        confirms the landing at result #2, ahead of the four 0.632748
 *        ghosts in round 3),
 *    (d) FAT multi-child drop (the CFATSearchedPartitionInfo vector
 *        >1 check, :842811-842821 - NO core-side input exists; registered
 *        as a residual, not invented),
 *    (e) the ranking sort is a STABLE descending merge sort
 *        (FUN_1405178a0 :846927-846928) - std::stable_sort, not std::sort.
 *
 * The reference sector count (the og's param_4 = *(this+0x40), :845670) is a
 * scan-context value whose PRODUCER is not decoded; the core passes the scan
 * view's total sectors as the honest equivalent (whole-disk flow), and the
 * API takes it explicitly so tests pin the rule itself. The og's result cap
 * (2/4/5/6, :845665-845731) is a UI curation layer that moves overflow to a
 * secondary list - the core surfaces every candidate (its CLI is not the
 * og's curation UI; dropping results in the core would be data loss) and
 * that boundary is documented in the finding's Resolution.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_SCORING_H_
#define CR_SCORING_H_

#include "cr_lostpart.h"
#include <vector>

namespace cr {
namespace lost {
namespace scoring {

/* The og's FS enum values that the weights switch on (the full enum is
 * FUN_140510080; only the ones the scorer distinguishes are named). */
constexpr u32 kFsFat12 = 0;
constexpr u32 kFsFat16 = 1;
constexpr u32 kFsFat32 = 2;
constexpr u32 kFsNtfs = 3;
constexpr u32 kFsHfsPlus = 8;

/* The og's raw count for one candidate (FUN_14050fda0 :842209-842223 and
 * the +500 table match at :842260-842267). */
u64 raw_count(u64 files, u64 folders, u32 og_fs_code, u64 bonus, bool table_match);

/* The og's per-family bonus defaults: FAT16/32 rows fit the QA log with
 * 20, NTFS rows with 0 (OQ-S2M-2 tracks the producer). */
u64 default_bonus(u32 og_fs_code);

/* The og's normalizer: denom = max>1 ? ln(max) : 1.0; score =
 * (raw>1 ? ln(raw) : raw) / denom. Pass the scan's max raw (pass 1). */
double normalize(u64 raw, u64 max_raw);

/* The og's FS code for a core candidate. The core's Fat family covers the
 * FAT12/16/32 flavors; the QA log's FAT rows are FAT32-weighted
 * ((f+d)*50), and FAT16 is the same weight - the core cannot distinguish
 * 1 from 2 without re-probing, and both share the (f+d)*50 weight, so
 * Fat -> Fat32. GPT is the og's table layer (not a CalcPartition FS) and
 * scores on the else branch like every non-FAT/HFS+ family. */
u32 og_fs_code(CandidateFs fs);

struct KnownExtent {
    u64 start_lba;
    u64 sector_count;
};

struct Ranked {
    lost::Candidate cand;
    u64 files = 0;
    u64 folders = 0;
    u64 raw = 0;
    double score = 0.0; /* dSuggested, 0..1 (can exceed 1.0 in theory like
                         * the og when raw == max and denom < ln edge) */
    bool table_match = false;
    bool promoted = false; /* the size-match promotion mark (+0x78) */
};

/* Full pipeline over the scan's candidates (already scanned: files/folders
 * counted by the caller):
 *  1. raw counts (weights + bonus + table match),
 *  2. denom from the max raw,
 *  3. per-candidate score,
 *  4. STABLE descending sort by score (FUN_1405178a0),
 *  5. AdjustPartitionInfoOrder: containment drop, top-2 ties,
 *     size-match promotion (position #2 + follower-score copy).
 * `known` carries the live table's extents for the containment drop and
 * the entry starts for the +500 match (the og compares FS AND start;
 * the core matches start plus a same-family check, since the core's
 * FsKind-vs-CandidateFs code spaces differ). reference_sectors = the
 * scan view's total sectors (the promotion's reference; 0 disables the
 * promotion honestly rather than with a fabricated reference).
 * Dropped candidates are REMOVED from `out` exactly like the og's list
 * curation; the promoted candidate is moved to index 1 (0-based). */
void rank_and_adjust(std::vector<Candidate>& candidates, std::vector<Ranked>& out,
                     const std::vector<table::PartitionInfo>& known, u64 reference_sectors);

/* The raw-count inputs the orchestrator collects per scanned candidate. */
struct ContentCounts {
    u64 files = 0;
    u64 folders = 0;
};

} // namespace scoring
} // namespace lost
} // namespace cr

#endif // CR_SCORING_H_
