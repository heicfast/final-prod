/*
 * cr_scoring.cpp - the og's dSuggested + AdjustPartitionInfoOrder port
 * (M-017). See cr_scoring.h for the full corpus citation block.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/lost/cr_scoring.h"
#include <algorithm>
#include <cmath>

namespace cr {
namespace lost {
namespace scoring {

u64 raw_count(u64 files, u64 folders, u32 og_fs_code, u64 bonus, bool table_match) {
    /* FUN_14050fda0 :842209-842223. NOTE the unsigned edge: `FS-1 < 2`
     * selects FAT16(1)/FAT32(2) only - FAT12(0) wraps to the else branch
     * (files + folders*50), exactly the decompile's arithmetic. */
    u64 raw = 0;
    if (og_fs_code == kFsHfsPlus) {
        raw = files + folders * 10; /* :842214-842215 */
    } else if (og_fs_code == kFsFat16 || og_fs_code == kFsFat32) {
        raw = (files + folders) * 50; /* :842217-842218 */
    } else {
        raw = files + folders * 50; /* :842220-842221 */
    }
    raw += bonus; /* :842223 (the +0x68 per-candidate integer) */
    if (table_match)
        raw += 500; /* :842260-842267 ("Found once partition:%I64d") */
    return raw;
}

u64 default_bonus(u32 og_fs_code) {
    /* The observed QA-log values (NTFS rows fit with 0, the FAT32 row with
     * 20; the producer decode is OQ-S2M-2). */
    if (og_fs_code == kFsFat16 || og_fs_code == kFsFat32)
        return 20;
    return 0;
}

double normalize(u64 raw, u64 max_raw) {
    /* :842226-842239 (denominator) + :842269-842274 (numerator/score).
     * The count==1 edge: raw <= 1 keeps the raw numerator (1.0), which is
     * what makes the og rank a 1-file candidate above 2..6-file ones. */
    const double denom = (double)max_raw > 1.0 ? std::log((double)max_raw) : 1.0;
    const double num = (double)raw > 1.0 ? std::log((double)raw) : (double)raw;
    return num / denom;
}

u32 og_fs_code(CandidateFs fs) {
    switch (fs) {
    case CandidateFs::Fat:
        return kFsFat32; /* the FAT16/32 weights are identical; the log's
                          * FAT rows are FAT32-coded */
    case CandidateFs::Ntfs:
        return kFsNtfs;
    case CandidateFs::HfsPlus:
        return kFsHfsPlus;
    default:
        /* exFAT/GPT/ext/Btrfs/ZFS/ReFS/BitLocker/Unknown: the og's else
         * branch (files + folders*50). exFAT=7, ext2=4/ext4=9, ReFS=10,
         * RAW=6, unknown=0xE - all share it. */
        return 0xFFFFFFFEu; /* sentinel: not a named special case */
    }
}

namespace {

bool inside_extent(const Ranked& r, const KnownExtent& e) {
    /* FUN_14050ffb0 :842305-842306: extent.start <= cand.start &&
     * cand.start + cand.totalSector <= extent.start + extent.size.
     * Corpus arbitration (the round-1 log): the +500 table-match candidate
     * - an EXACT match of a live entry - SURVIVED AdjustPartitionInfoOrder
     * and topped the results, so the og's ActivePartition list and the
     * scorer's known-partition array are disjoint inputs; with one live
     * table feeding both rules here, the log-observed behavior wins:
     * only PROPER subsets drop, exact matches (the +500 winners) stay. */
    if (r.cand.start_lba == e.start_lba &&
        r.cand.sector_count == e.sector_count)
        return false;
    return e.start_lba <= r.cand.start_lba &&
           r.cand.start_lba + r.cand.sector_count <= e.start_lba + e.sector_count;
}

bool matches_known_entry(const Ranked& r, const std::vector<table::PartitionInfo>& known) {
    /* The og's table match: FS AND start (:842260-842267). The core's
     * live-table entries carry FsKind (a different code space) - match on
     * the start LBA plus a same-family check derived from the entry's
     * MBR type when the kind is unknown. */
    for (const auto& p : known) {
        if (!p.used)
            continue;
        if (p.start_lba != r.cand.start_lba)
            continue;
        const bool same_family =
            (p.from_gpt && r.cand.fs == CandidateFs::GptTable) ||
            (!p.from_gpt && r.cand.fs != CandidateFs::GptTable);
        if (same_family)
            return true;
    }
    return false;
}

} // namespace

void rank_and_adjust(std::vector<Candidate>& candidates, std::vector<Ranked>& out,
                     const std::vector<table::PartitionInfo>& known, u64 reference_sectors) {
    out.clear();
    out.reserve(candidates.size());
    for (auto& c : candidates) {
        Ranked r;
        r.cand = c;
        out.push_back(r);
    }

    /* pass 1: the weighted counts + bonus, and the shared denominator -
     * the og computes the denominator from the PRE-+500 maximum (pass 1
     * runs at :842209-842236; the table-match +500 is a PASS-2 per-
     * candidate add at :842260-842267 and never enters the denominator).
     * The QA log pins it: the 429-file row's 0.503579 = ln(429)/
     * ln(168818) where 168818 is the top candidate's pre-bonus raw. */
    u64 max_raw = 0;
    for (auto& r : out) {
        const u32 code = og_fs_code(r.cand.fs);
        r.table_match = matches_known_entry(r, known);
        r.raw = raw_count(r.cand.content_files, r.cand.content_folders, code,
                          default_bonus(code), false);
        if (r.raw > max_raw)
            max_raw = r.raw;
    }
    /* pass 2: per-candidate scores, the +500 table-match add included */
    for (auto& r : out) {
        r.raw = raw_count(r.cand.content_files, r.cand.content_folders,
                          og_fs_code(r.cand.fs), default_bonus(og_fs_code(r.cand.fs)),
                          r.table_match);
        r.score = normalize(r.raw, max_raw);
        /* carry the og-shaped score back onto the candidate (permille,
         * rounded) so persistence + display match the og's %f output */
        double permille = r.score * 1000.0;
        if (permille < 0)
            permille = 0;
        if (permille > 4000) /* raw==max can exceed 1.0 only by rounding */
            permille = 4000;
        r.cand.score = int(permille + 0.5);
    }

    /* the og's stable descending merge sort (FUN_1405178a0) */
    std::stable_sort(out.begin(), out.end(), [](const Ranked& a, const Ranked& b) {
        return a.score > b.score;
    });

    /* ---- AdjustPartitionInfoOrder (FUN_140510410) ---- */

    /* (a) known-extent containment drop (:842305-842306). The og drops
     * candidates FULLY INSIDE a live partition extent. */
    {
        std::vector<KnownExtent> extents;
        extents.reserve(known.size());
        for (const auto& p : known)
            if (p.used)
                extents.push_back({p.start_lba, p.sector_count});
        std::vector<Ranked> kept;
        kept.reserve(out.size());
        for (auto& r : out) {
            bool contained = false;
            for (const auto& e : extents)
                if (inside_extent(r, e)) {
                    contained = true;
                    break;
                }
            if (!contained)
                kept.push_back(std::move(r));
        }
        out = std::move(kept);
    }

    /* (b) top-2 tie rules (:842752-842769): score gap <= 0.1 drops #2;
     * gap <= 0.3 drops #2 when #2 is FAT16/32 (NTFS outranks FAT at
     * near-equal evidence). Applies to the FIRST TWO of the sorted list,
     * once - the og's loop shape. */
    if (out.size() >= 2) {
        const double gap = out[0].score - out[1].score;
        const bool drop_second = (gap <= 0.1) || (gap <= 0.3 && out[1].cand.fs == CandidateFs::Fat);
        if (drop_second)
            out.erase(out.begin() + 1);
    }

    /* (c) size-match promotion (:842770-842796 + LAB_140510a0e): the FIRST
     * candidate at sorted position >= 1 with |totalSector - reference|*512
     * < 16 MiB+1 is the promoted one; when it sits at position >= 2 it is
     * moved to position #2 with the FOLLOWING candidate's score copied
     * into it (the log's result-#2 landing). reference 0 = no honest
     * reference available: the rule does not fire. */
    if (reference_sectors != 0 && out.size() >= 2) {
        for (size_t i = 1; i < out.size(); i++) {
            const u64 diff =
                out[i].cand.sector_count > reference_sectors
                    ? out[i].cand.sector_count - reference_sectors
                    : reference_sectors - out[i].cand.sector_count;
            if (diff * 512 < 0x1000001ull) {
                out[i].promoted = true; /* the +0x78 mark */
                if (i >= 2) {
                    Ranked moving = std::move(out[i]);
                    out.erase(out.begin() + long(i));
                    /* score := the candidate that will follow it at #3
                     * (:842851 - lVar2+0x70 := second-node-payload+0x70) */
                    moving.score = out[1].score;
                    double permille = moving.score * 1000.0;
                    if (permille > 4000)
                        permille = 4000;
                    moving.cand.score = int(permille + 0.5);
                    out.insert(out.begin() + 1, std::move(moving));
                }
                break; /* the og marks the FIRST match, once */
            }
        }
    }

    /* (d) FAT multi-child drop: no core-side input exists (the og reads a
     * CFATSearchedPartitionInfo sub-object vector); registered as a
     * residual in the finding's Resolution - deliberately NOT invented. */
}

} // namespace scoring
} // namespace lost
} // namespace cr
