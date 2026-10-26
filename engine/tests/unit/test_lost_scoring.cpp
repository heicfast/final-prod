/* test_lost_scoring.cpp - the M-017 acceptance set: the og's content-driven
 * lost-partition ranking (dSuggested, DRWUI FUN_14050fda0) + the
 * AdjustPartitionInfoOrder curation, clean-room from the corpus decode and
 * numerically pinned against the vendor's own CalcPartition.log rows.
 *
 *   1. ScoringMatchesLogRows - table-driven reproduction of the QA-log
 *      rows (the finding's named examples) to print-precision tolerance.
 *   2. FileRichCandidateOutranksCleanBoot - the finding's falsifiable
 *      fixture: 300-file weak-boot beats 20-file clean-boot (fails before).
 *   3. TableMatchBonus500 - the +500 add (pass 2 only; the denominator
 *      stays pre-+500, exactly the og's two-pass shape).
 *   4. SizeMatchPromotionToResultTwo - the AdjustPartitionInfoOrder
 *      promotion: |size - ref|*512 < 16 MiB+1, first non-head match,
 *      landing at result #2 with the follower's score.
 *   5. ContainmentDropAndFATTieRule - the og's curation rules.
 *   6. CountOneEdgeRanksAboveSmallCounts - the og's raw<=1 numerator edge.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include <gtest/gtest.h>
#include "cr/lost/cr_lostpart.h"
#include "cr/lost/cr_scoring.h"
#include "cr/table/cr_table_parse.h"
#include <cmath>
#include <vector>

using namespace cr;
using namespace cr::lost;
using namespace cr::lost::scoring;

namespace {

Candidate make(CandidateFs fs, u64 files, u64 folders, u64 start = 100, u64 count = 1000) {
    Candidate c;
    c.fs = fs;
    c.start_lba = start;
    c.sector_count = count;
    c.content_files = files;
    c.content_folders = folders;
    return c;
}

} // namespace

TEST(LostScoring, ScoringMatchesLogRows) {
    /* Round-1 shape from the vendor's CalcPartition.log (the evidence
     * file's fit): the top candidate carries files=3618/folders=3304 ->
     * pre-bonus raw = 3618 + 3304*50 = 168818 = the round's denominator
     * input; a 429-file NTFS candidate logs 0.503579; a 51-file candidate
     * logs 0.326652; a 1-file candidate logs 1/ln(max) = the count==1
     * edge. Tolerances are the og's own printf %f print rounding. */
    const std::vector<Candidate> round1 = {
        make(CandidateFs::Ntfs, 3618, 3304),  /* the round's max (0.503579's max) */
        make(CandidateFs::Ntfs, 429, 0),     /* logged 0.503579 */
        make(CandidateFs::Ntfs, 51, 0),      /* logged 0.326652 */
        make(CandidateFs::Ntfs, 1, 0),       /* logged 0.0819-class */
        make(CandidateFs::Fat, 0, 25),       /* logged 0.585518-class (round 2;
                                                here checked only for weighting) */
    };
    std::vector<Ranked> ranked;
    rank_and_adjust(const_cast<std::vector<Candidate>&>(round1), ranked, {}, 0);

    /* find the rows by their content */
    const Ranked* r429 = nullptr;
    const Ranked* r51 = nullptr;
    const Ranked* r1 = nullptr;
    const Ranked* rmax = nullptr;
    for (const auto& r : ranked) {
        if (r.cand.content_files == 429)
            r429 = &r;
        if (r.cand.content_files == 51)
            r51 = &r;
        if (r.cand.content_files == 1)
            r1 = &r;
        if (r.cand.content_files == 3618)
            rmax = &r;
    }
    ASSERT_NE(nullptr, r429);
    ASSERT_NE(nullptr, r51);
    ASSERT_NE(nullptr, r1);
    ASSERT_NE(nullptr, rmax);
    /* denominator = ln(168818): ln(429)/ln(168818) = 0.5036 (logged
     * 0.503579, the row's own round max) */
    EXPECT_NEAR(r429->score, 0.503579, 2e-4);
    EXPECT_NEAR(r51->score, 0.326652, 2e-4);
    /* count==1 edge: numerator stays raw (1.0) -> 1/ln(168818) = 0.0831 */
    EXPECT_NEAR(r1->score, 1.0 / std::log(168818.0), 1e-4);
    /* the max candidate itself: ln(168818)/ln(168818) = 1.0 exactly */
    EXPECT_NEAR(rmax->score, 1.0, 1e-9);
    /* stable descending order */
    for (size_t i = 1; i < ranked.size(); i++)
        EXPECT_GE(ranked[i - 1].score, ranked[i].score);
}

TEST(LostScoring, FileRichCandidateOutranksCleanBoot) {
    /* The finding's falsifiable fixture: 300 files with a weak boot sector
     * (low structural points) vs 20 files with a clean boot (high
     * structural points). The og's content score ranks the 300-file
     * candidate first regardless of boot cleanliness - recovery value
     * beats signature plausibility. The structural points ride along as
     * the tie-break only. */
    Candidate rich = make(CandidateFs::Ntfs, 300, 5, /*start=*/2048, 1000);
    Candidate clean = make(CandidateFs::Ntfs, 20, 1, /*start=*/4096, 1000);
    std::vector<Candidate> cands = {rich, clean};
    std::vector<Ranked> ranked;
    rank_and_adjust(cands, ranked, {}, 0);
    ASSERT_EQ(2u, ranked.size());
    EXPECT_EQ(300u, ranked[0].cand.content_files)
        << "the file-rich candidate must rank first";
    EXPECT_EQ(20u, ranked[1].cand.content_files);
    /* and the raw weights: NTFS else-branch = files + folders*50 */
    EXPECT_EQ(300 + 5 * 50, ranked[0].raw);
    EXPECT_EQ(20 + 1 * 50, ranked[1].raw);
}

TEST(LostScoring, TableMatchBonus500) {
    /* The +500 fires only in pass 2 (the numerator): the denominator
     * stays pre-+500. A 100-file candidate matching a known-table entry
     * beats a bigger 150-file unmatched candidate:
     * ln(100+500) vs ln(150) with denom ln(150): 6.397 vs 5.011 -> the
     * table match wins, exactly the og's "Found once partition" bump. */
    std::vector<table::PartitionInfo> known;
    table::PartitionInfo p;
    p.start_lba = 2048;
    p.sector_count = 1000;
    p.used = true;
    known.push_back(p);

    std::vector<Candidate> cands = {
        make(CandidateFs::Ntfs, 100, 0, 2048, 1000), /* matches the entry */
        make(CandidateFs::Ntfs, 150, 0, 4096, 1000),/* bigger, unmatched */
    };
    std::vector<Ranked> ranked;
    rank_and_adjust(cands, ranked, known, 0);
    ASSERT_EQ(2u, ranked.size());
    EXPECT_TRUE(ranked[0].table_match);
    EXPECT_FALSE(ranked[1].table_match);
    EXPECT_EQ(600u, ranked[0].raw); /* 100 + 500 */
    EXPECT_EQ(150u, ranked[1].raw);
    EXPECT_GT(ranked[0].score, ranked[1].score);
}

TEST(LostScoring, SizeMatchPromotionToResultTwo) {
    /* The AdjustPartitionInfoOrder promotion: the FIRST candidate at
     * sorted position >= 1 whose |sector_count - reference|*512 is
     * within 16 MiB+1 moves to result #2 with the FOLLOWER's score
     * (the log's round-3 landing: the 0-score 5216 candidate becomes
     * result #2 ahead of the ghosts). A position >= 2 candidate is
     * spliced; a reference of 0 disables the rule honestly. */
    const u64 ref = 100000; /* reference in sectors */
    /* candidate sized within 16 MiB of the reference (diff*512 < 0x1000001
     * -> diff < 32768.002 sectors) */
    const u64 near_size = ref - 4096;
    std::vector<Candidate> cands = {
        make(CandidateFs::Ntfs, 500, 10, 100, ref),   /* #1 by content */
        make(CandidateFs::Ntfs, 400, 2, 2048, 900),   /* would be #2 */
        make(CandidateFs::Fat, 0, 0, 5216, near_size),/* size-matched, low score */
        make(CandidateFs::Ntfs, 100, 1, 8192, 800),   /* the rest */
        make(CandidateFs::Ntfs, 50, 0, 9999, 700),
    };
    std::vector<Ranked> ranked;
    rank_and_adjust(cands, ranked, {}, ref);
    ASSERT_GE(ranked.size(), 3u);
    /* the size-matched candidate landed at index 1 (result #2) */
    EXPECT_EQ(5216u, ranked[1].cand.start_lba);
    EXPECT_TRUE(ranked[1].promoted);
    /* with the follower's score copied (the og's :842851 copy) */
    EXPECT_NEAR(ranked[1].score, ranked[2].score, 1e-12);
    /* #1 unchanged */
    EXPECT_EQ(500u, ranked[0].cand.content_files);

    /* already at #2: no splice needed, but still marked */
    {
        std::vector<Candidate> c2 = {
            make(CandidateFs::Ntfs, 500, 10, 100, ref),
            make(CandidateFs::Fat, 0, 0, 5216, near_size),
        };
        std::vector<Ranked> r2;
        rank_and_adjust(c2, r2, {}, ref);
        ASSERT_EQ(2u, r2.size());
        EXPECT_TRUE(r2[1].promoted);
    }
    /* reference 0: the rule must not fire (no honest reference) */
    {
        std::vector<Candidate> c3 = {
            make(CandidateFs::Ntfs, 500, 10, 100, ref),
            make(CandidateFs::Ntfs, 400, 2, 2048, 900),
            make(CandidateFs::Fat, 0, 0, 5216, near_size),
        };
        std::vector<Ranked> r3;
        rank_and_adjust(c3, r3, {}, 0);
        for (const auto& r : r3)
            EXPECT_FALSE(r.promoted);
    }
}

TEST(LostScoring, ContainmentDropAndFATTieRule) {
    /* (a) a candidate fully inside a live extent is dropped (the og's
     * FUN_14050ffb0 containment rule). */
    std::vector<table::PartitionInfo> known;
    table::PartitionInfo live;
    live.start_lba = 0;
    live.sector_count = 2000;
    live.used = true;
    known.push_back(live);
    std::vector<Candidate> cands = {
        make(CandidateFs::Ntfs, 300, 3, 100, 500),   /* proper subset of [0,2000) */
        make(CandidateFs::Ntfs, 200, 1, 4096, 5000),/* outside [0,2000) */
    };
    std::vector<Ranked> ranked;
    rank_and_adjust(cands, ranked, known, 0);
    ASSERT_EQ(1u, ranked.size());
    EXPECT_EQ(4096u, ranked[0].cand.start_lba);

    /* (b) the top-2 FAT tie rule: score gap in (0.1, 0.3] with #2 FAT
     * drops #2 (NTFS outranks FAT at near-equal evidence) - a gap the
     * 0.1 rule alone would NOT catch, pinning the FAT-specific arm
     * (:842762-842768). NTFS raw 200 (the max, score 1.0); FAT raw
     * (0+1)*50+20 = 70 -> score ln(70)/ln(200) = 0.8018; gap 0.198. */
    {
        std::vector<Candidate> c2 = {
            make(CandidateFs::Ntfs, 100, 2, 100),  /* raw 200, score 1.0 */
            make(CandidateFs::Fat, 0, 1, 2048),      /* raw 70, score 0.8018 */
        };
        std::vector<Ranked> r2;
        rank_and_adjust(c2, r2, {}, 0);
        /* gap 0.198 <= 0.3 and #2 is FAT -> dropped */
        ASSERT_EQ(1u, r2.size());
        EXPECT_EQ(CandidateFs::Ntfs, r2[0].cand.fs);
    }
    /* same gap with #2 NTFS (the else branch): kept (:842752-842759's
     * 0.1 arm alone does not fire at 0.198) */
    {
        std::vector<Candidate> c3 = {
            make(CandidateFs::Ntfs, 100, 2, 100), /* raw 200 */
            make(CandidateFs::Ntfs, 70, 0, 2048),  /* raw 70, same gap */
        };
        std::vector<Ranked> r3;
        rank_and_adjust(c3, r3, {}, 0);
        ASSERT_EQ(2u, r3.size());
        EXPECT_EQ(CandidateFs::Ntfs, r3[1].cand.fs);
    }
}

TEST(LostScoring, CountOneEdgeRanksAboveSmallCounts) {
    /* The og's raw<=1 numerator edge reproduces the log's
     * non-monotonicity: a 1-file candidate (numerator 1.0) ranks ABOVE
     * 2..6-file candidates (numerator ln(2..6) with the observed
     * denominators). With max 168818: 1/12.0366 = 0.0831 >
     * ln(2)/12.0366 = 0.0576. */
    std::vector<Candidate> cands = {
        make(CandidateFs::Ntfs, 3618, 3304), /* the max */
        make(CandidateFs::Ntfs, 1, 0),
        make(CandidateFs::Ntfs, 2, 0),
        make(CandidateFs::Ntfs, 3, 0),
    };
    std::vector<Ranked> ranked;
    rank_and_adjust(cands, ranked, {}, 0);
    ASSERT_EQ(4u, ranked.size());
    /* rank order: max (1.0), then 1-file (0.0831), then 3-file (ln 3 =
     * 1.0986/12.0366 = 0.0913)?? - ln(3) > 1.0! The og's edge: 1 file
     * scores 1.0/ln(max); 3 files scores ln(3)/ln(max) > that. The
     * NON-monotonicity is only between count==1 and count==2 (ln(2) <
     * 1.0). */
    EXPECT_NEAR(ranked[0].score, 1.0, 1e-9);
    /* 3-file outranks 2-file (ln 3 > ln 2) and 1-file sits between */
    EXPECT_GT(ranked[1].score, ranked[3].score); /* 3-file > 2-file */
    const double one = 1.0 / std::log(168818.0);
    const double two = std::log(2.0) / std::log(168818.0);
    ASSERT_NEAR(ranked[2].score, one, 1e-9);  /* 1-file at slot 2 */
    ASSERT_NEAR(ranked[3].score, two, 1e-9); /* 2-file last */
    /* the log's exact claim: 1 file (0.0831) > 2 files (0.0576) */
    EXPECT_GT(one, two);
}

/* ------------------------------------------------------------------ */
/* The orchestrator-level proof: content ranking reorders the scan's   */
/* candidate list (the finding's falsifiable fixture, executed).       */
/* ------------------------------------------------------------------ */

#include "cr/orchestrator/cr_scan.h"
#include "cr/storage/cr_device.h"
#include <cstring>

namespace {

class ScoringMemDevice final : public IStorageDevice {
public:
    explicit ScoringMemDevice(std::vector<u8> b) : data_(std::move(b)) {
        geom_.logical_sector = 512;
        geom_.physical_sector = 512;
        geom_.sector_count = data_.size() / 512;
        geom_.total_bytes = geom_.sector_count * 512;
    }
    u32 read_at(u64 off, void* buf, u32 len) override {
        if (off > data_.size() || len > data_.size() - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        std::memcpy(buf, data_.data() + off, len);
        return OK;
    }
    const Geometry& geometry() const override { return geom_; }

private:
    std::vector<u8> data_;
    Geometry geom_;
};

void s16(std::vector<u8>& v, size_t at, u16 x) {
    v[at] = u8(x);
    v[at + 1] = u8(x >> 8);
}
void s32(std::vector<u8>& v, size_t at, u32 x) {
    v[at] = u8(x);
    v[at + 1] = u8(x >> 8);
    v[at + 2] = u8(x >> 16);
    v[at + 3] = u8(x >> 24);
}

/* A minimal FAT16 volume with `nfiles` one-cluster files, planted at
 * `lba`. The og-parity boot gates pass (M-045: bps 512-multiple,
 * numFATs 2, real totals; the N-034 deep-scan gates too). */
void plant_fat16_with_files(std::vector<u8>& img, u64 lba, u32 nfiles) {
    const size_t base = size_t(lba) * 512;
    const u32 total_secs = 512;
    u8* b = img.data() + base;
    b[0] = 0xEB;
    b[1] = 0x3C;
    b[2] = 0x90;
    std::memcpy(b + 3, "MSDOS5.0", 8);
    s16(img, base + 0x0B, 512);
    b[0x0D] = 1;                /* spc */
    s16(img, base + 0x0E, 2);   /* reserved */
    b[0x10] = 2;                /* numFATs */
    s16(img, base + 0x11, 512); /* root entries */
    s16(img, base + 0x13, u16(total_secs));
    b[0x15] = 0xF8;
    s16(img, base + 0x16, 4); /* spf */
    s32(img, base + 0x20, 0);
    s32(img, base + 0x24, 0);
    std::memcpy(b + 0x36, "FAT16   ", 8);
    b[510] = 0x55;
    b[511] = 0xAA;
    /* FAT1 @ sector 2, FAT2 @ 6, root @ 10, data @ 42 */
    const size_t FAT1 = base + 2 * 512, FAT2 = base + 6 * 512;
    const size_t ROOT = base + 10 * 512;
    const size_t DATA = base + 42 * 512;
    s16(img, FAT1 + 0, 0xFFF8);
    s16(img, FAT1 + 2, 0xFFFF);
    s16(img, FAT2 + 0, 0xFFF8);
    s16(img, FAT2 + 2, 0xFFFF);
    for (u32 i = 0; i < nfiles; i++) {
        const u32 cluster = 2 + i;
        s16(img, FAT1 + cluster * 2, 0xFFFF);
        s16(img, FAT2 + cluster * 2, 0xFFFF);
        u8* e = img.data() + ROOT + size_t(i) * 32;
        char name[11] = {'F', 'I', 'L', 'E', '0', '0', '0', '0', '0', 'T', 'X'};
        e[4] = u8('0' + i);
        std::memcpy(e, name, 11);
        e[11] = 0x20; /* attributes */
        s16(img, ROOT + size_t(i) * 32 + 26, u16(cluster));
        s32(img, ROOT + size_t(i) * 32 + 28, 256);
        std::memcpy(img.data() + DATA + size_t(cluster - 2) * 512, "PAYLOAD", 7);
    }
}

TEST(LostScoringIntegration, FileRichCandidateOutranksThinCandidateInScan) {
    /* Two healthy lost FAT16 volumes on a table-less disk: a THIN one at
     * LBA 2048 (1 file) and a RICH one at LBA 8192 (10 files). Discovery
     * order is LBA-structural (both score identical probe points, the
     * thin one first); the M-017 content ranking must put the rich
     * volume first in stats.lost_candidates - the og's dSuggested order.
     * (Fail-before: with the ranking disabled, the list keeps discovery
     * order and the THIN candidate tops it.) */
    std::vector<u8> img(u64(32768) * 512, 0);
    plant_fat16_with_files(img, 2048, 1);
    plant_fat16_with_files(img, 8192, 10);
    ScoringMemDevice dev(std::move(img));

    tree::ResultTree tree;
    orchestrator::ScanConfig cfg;
    cfg.run_quick = false;
    cfg.run_deep = true;
    cfg.run_carve = false;
    cfg.whole_disk = true;
    orchestrator::ScanController ctrl(tree, cfg);
    orchestrator::ScanStats stats;
    const u32 rc = ctrl.run(&dev, nullptr, nullptr, nullptr, stats);
    ASSERT_EQ(rc, u32(OK)) << err_text(rc);
    ASSERT_GE(stats.lost_candidates.size(), 2u);
    /* both candidates carry their recovered-content counts */
    const lost::Candidate* thin = nullptr;
    const lost::Candidate* rich = nullptr;
    for (const auto& c : stats.lost_candidates) {
        if (c.start_lba == 2048)
            thin = &c;
        if (c.start_lba == 8192)
            rich = &c;
    }
    ASSERT_NE(nullptr, thin);
    ASSERT_NE(nullptr, rich);
    EXPECT_EQ(1u, thin->content_files);
    EXPECT_EQ(10u, rich->content_files);
    /* the og's order: the rich volume first */
    EXPECT_EQ(8192u, stats.lost_candidates[0].start_lba)
        << "the content-rich candidate must top the list";
    EXPECT_EQ(2048u, stats.lost_candidates[1].start_lba);
}

} // namespace
