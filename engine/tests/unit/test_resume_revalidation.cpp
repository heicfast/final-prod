/* test_resume_revalidation.cpp - the M-042 acceptance set: the og's
 * resume-time disk-truth pass (PartitionRecoveryD PRD:20748-20842 per-record
 * re-validation + PRD:20998-21039 partinfo layout compare) reproduced on the
 * core's session surface.
 *
 *   1. ResumeDropsStaleCandidateAfterRelayout - the finding's fail-before
 *      proof: a candidate persisted on disk A is DROPPED when the session
 *      is resumed/listed against a disk whose layout changed.
 *   2. ResumeKeepsValidCandidate - the honest counterpart: an unchanged
 *      disk keeps the record (plus the geometry round-trip).
 *   3. DiskLayoutRecordRoundTrip - the partinfo-equivalent record.
 *   4. LayoutChangedFlagged - the "disk changed" signal (surfaced, not
 *      enforced) and its stay-false behavior for old sessions.
 *   5. CandidateGeometryRoundTrip - the og's per-record geometry payload
 *      (bps/SPC/hidden) + v1-session back-compat (25-byte records).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include <gtest/gtest.h>
#include "cr/orchestrator/cr_scan.h"
#include "cr/lost/cr_lostpart.h"
#include "cr/session/cr_crsf.h"
#include "cr/session/cr_records.h"
#include "cr/storage/cr_device.h"
#include "cr/storage/cr_slice.h"
#include "cr/table/cr_table_parse.h"
#include "cr/fat/cr_fat_engine.h"
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

using namespace cr;
using namespace cr::orchestrator;
using namespace cr::session;

namespace {

class MemDevice final : public IStorageDevice {
public:
    explicit MemDevice(std::vector<u8> b) : data_(std::move(b)) {
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

void put16(std::vector<u8>& v, size_t at, u16 x) {
    v[at] = u8(x);
    v[at + 1] = u8(x >> 8);
}
void put32v(std::vector<u8>& v, size_t at, u32 x) {
    v[at] = u8(x);
    v[at + 1] = u8(x >> 8);
    v[at + 2] = u8(x >> 16);
    v[at + 3] = u8(x >> 24);
}

/* The FAT boot the discovery's probe_fat accepts (the N-034/M-045 gate
 * shapes: bps 512, spc 1, numFATs 2, real totals). */
void plant_fat_boot(std::vector<u8>& img, u64 lba) {
    u8* b = img.data() + lba * 512;
    b[0] = 0xEB;
    b[1] = 0x3C;
    b[2] = 0x90;
    std::memcpy(b + 3, "MSDOS5.0", 8);
    put16(img, lba * 512 + 0x0B, 512);
    b[0x0D] = 1;              /* spc */
    put16(img, lba * 512 + 0x0E, 2);  /* reserved */
    b[0x10] = 2;              /* numFATs (M-045: != 0) */
    put16(img, lba * 512 + 0x11, 512); /* root entries */
    b[0x15] = 0xF8;
    put16(img, lba * 512 + 0x16, 20); /* spf */
    put32v(img, lba * 512 + 0x20, 4096); /* total32 */
    put32v(img, lba * 512 + 0x24, 0);
    b[510] = 0x55;
    b[511] = 0xAA;
}

/* A valid MBR with one entry at 2048/100 sectors, type `t` (the layout-
 * compare fixtures need parse_disk_table to succeed). */
void plant_mbr(std::vector<u8>& img, u32 t) {
    u8* mbr = img.data();
    mbr[446 + 4] = u8(t);
    put32v(img, 446 + 8, 2048);
    put32v(img, 446 + 12, 100);
    mbr[510] = 0x55;
    mbr[511] = 0xAA;
}

/* ---- in-memory session sink (the test_session.cpp MemFile shape) ----- */

struct MemFile {
    std::vector<u8> data;
    u64 pos = 0;
    static u32 wr(void* u, const void* b, u32 len) {
        auto* m = static_cast<MemFile*>(u);
        if (m->pos + len > m->data.size())
            m->data.resize(size_t(m->pos + len));
        std::memcpy(m->data.data() + m->pos, b, len);
        m->pos += len;
        return 0;
    }
    static u64 tl(void* u) { return static_cast<MemFile*>(u)->pos; }
    static u32 sk(void* u, u64 abs) {
        auto* m = static_cast<MemFile*>(u);
        m->pos = abs;
        return 0;
    }
    static u32 rd(void* u, void* b, u32 len) {
        auto* m = static_cast<MemFile*>(u);
        if (m->pos + len > m->data.size())
            return 1;
        std::memcpy(b, m->data.data() + m->pos, len);
        m->pos += len;
        return 0;
    }
    static u64 sz(void* u) { return static_cast<MemFile*>(u)->data.size(); }
};

/* Hand-build a minimal whole-disk session: state(all-done) + volume-info
 * (whole-disk basis) + one partition context (the persisted candidate) and
 * optionally the disk-layout snapshot. */
MemFile build_session(const lost::Candidate& cand, bool with_layout,
                      const std::vector<DiskLayoutEntry>& layout) {
    MemFile mf;
    Writer w(MemFile::wr, MemFile::tl, MemFile::sk, &mf);
    if (w.open(9) != OK)
        ADD_FAILURE();
    std::vector<u8> blob;

    SessionState st;
    st.stage_done_mask = STAGE_QUICK | STAGE_DEEP | STAGE_CARVE;
    st.files_found = 1;
    if (serialize_session_state(st, blob) == OK)
        if (w.add_record(REC_SESSION_STATE, blob.data(), u32(blob.size())) != OK)
            ADD_FAILURE();

    VolumeInfo vi;
    vi.fs_kind = u32(FsFamily::Unknown);
    vi.flags = STAGE_QUICK | STAGE_DEEP | STAGE_CARVE;
    vi.slice_offset = 0; /* whole-disk basis: the M-042 re-validation basis */
    vi.slice_bytes = 8192u * 512;
    vi.logical_sector = 512;
    vi.physical_sector = 512;
    if (serialize_volume_info(vi, blob) == OK)
        if (w.add_record(REC_VOLUME_INFO, blob.data(), u32(blob.size())) != OK)
            ADD_FAILURE();

    PartitionContext pc;
    pc.start_lba = cand.start_lba;
    pc.sector_count = cand.sector_count;
    pc.fs_kind = u32(cand.fs);
    pc.score = u32(cand.score > 0 ? cand.score : 0);
    pc.kind = 1;
    pc.bytes_per_sector = cand.bytes_per_sector;
    pc.sectors_per_cluster = cand.sectors_per_cluster;
    pc.hidden_sectors = cand.hidden_sectors;
    if (cand.size_from_device_end)
        pc.flags |= 0x1;
    if (cand.have_geometry)
        pc.flags |= 0x2;
    if (serialize_partition_context(pc, blob) == OK)
        if (w.add_record(REC_PARTITION_CONTEXT, blob.data(), u32(blob.size())) != OK)
            ADD_FAILURE();

    if (with_layout && serialize_disk_layout(layout, blob) == OK)
        if (w.add_record(REC_DISK_LAYOUT, blob.data(), u32(blob.size())) != OK)
            ADD_FAILURE();

    if (w.finalize() != OK)
        ADD_FAILURE();
    return mf;
}

std::vector<u8> base_disk() {
    std::vector<u8> img(u64(8192) * 512, 0);
    plant_mbr(img, 0x0B);
    plant_fat_boot(img, 2048);
    return img;
}

lost::Candidate fat_candidate() {
    lost::Candidate c;
    c.start_lba = 2048;
    c.sector_count = 100;
    c.fs = lost::CandidateFs::Fat;
    c.score = 40;
    c.bytes_per_sector = 512;
    c.sectors_per_cluster = 1;
    c.hidden_sectors = 63;
    c.have_geometry = 1;
    return c;
}

} // namespace

/* ------------------------------------------------------------------ */
/* 1. the fail-before proof: stale records are dropped                */
/* ------------------------------------------------------------------ */

TEST(ResumeRevalidation, ResumeDropsStaleCandidateAfterRelayout) {
    /* The finding's exact scenario: the candidate was saved when LBA 2048
     * held a FAT boot; the disk has since been re-laid-out and the sector
     * now holds something else. Both device-bearing surfaces (the listing
     * load AND the run-start pass) must drop it. */
    auto mf = build_session(fat_candidate(), false, {});
    tree::ResultTree tree;

    /* (a) the device-bearing listing surface */
    {
        auto img = base_disk();
        /* relayout: wipe the FAT boot at 2048, leave the rest */
        std::memset(img.data() + 2048u * 512, 0, 512);
        MemDevice relaid(std::move(img));
        tree::ResultTree t;
        ScanStats stats;
        VolumeSessionInfo info;
        Reader r(MemFile::rd, MemFile::sk, MemFile::sz, &mf);
        ASSERT_EQ(OK, ScanController::load_session_summary(r, t, stats, info, &relaid));
        EXPECT_TRUE(stats.lost_candidates.empty())
            << "the listing surface must not surface a stale record";
        EXPECT_EQ(1u, stats.dropped_stale_candidates);
        EXPECT_EQ(1u, stats.revalidated_candidates);
    }

    /* (b) the resume + run() surface (the og's at-search-start timing) */
    {
        auto img = base_disk();
        std::memset(img.data() + 2048u * 512, 0, 512);
        MemDevice relaid(std::move(img));
        tree::ResultTree t;
        ScanStats stats;
        VolumeSessionInfo info;
        Reader r(MemFile::rd, MemFile::sk, MemFile::sz, &mf);
        std::unique_ptr<ScanController> ctrl;
        ASSERT_EQ(OK, ScanController::resume(r, t, ScanConfig{}, stats, info, ctrl,
                                             &relaid));
        ASSERT_EQ(OK, ctrl->run(&relaid, nullptr, nullptr, nullptr, stats));
        EXPECT_TRUE(stats.lost_candidates.empty())
            << "run() must drop the stale record before any stage executes";
        EXPECT_GE(stats.dropped_stale_candidates, 1u);
    }
}

/* ------------------------------------------------------------------ */
/* 2. the counterpart: an unchanged disk keeps the record              */
/* ------------------------------------------------------------------ */

TEST(ResumeRevalidation, ResumeKeepsValidCandidate) {
    auto mf = build_session(fat_candidate(), false, {});
    MemDevice disk(base_disk());
    tree::ResultTree t;
    ScanStats stats;
    VolumeSessionInfo info;
    Reader r(MemFile::rd, MemFile::sk, MemFile::sz, &mf);
    ASSERT_EQ(OK, ScanController::load_session_summary(r, t, stats, info, &disk));
    ASSERT_EQ(1u, stats.lost_candidates.size());
    EXPECT_EQ(0u, stats.dropped_stale_candidates);
    const auto& c = stats.lost_candidates[0];
    EXPECT_EQ(2048u, c.start_lba);
    EXPECT_EQ(100u, c.sector_count);
    EXPECT_EQ(lost::CandidateFs::Fat, c.fs);
    /* M-042 v2: the geometry payload round-trips */
    EXPECT_EQ(512u, c.bytes_per_sector);
    EXPECT_EQ(1u, c.sectors_per_cluster);
    EXPECT_EQ(63u, c.hidden_sectors);
    EXPECT_EQ(1u, c.have_geometry);
}

/* ------------------------------------------------------------------ */
/* 3. the partinfo-equivalent record round-trips                        */
/* ------------------------------------------------------------------ */

TEST(ResumeRevalidation, DiskLayoutRecordRoundTrip) {
    std::vector<DiskLayoutEntry> in;
    in.push_back({2048, 100, 0x07});
    in.push_back({4096, 2048, 0x0B});
    in.push_back({8192, 4096, 0x83});
    std::vector<u8> blob;
    ASSERT_EQ(OK, serialize_disk_layout(in, blob));
    std::vector<DiskLayoutEntry> out;
    ASSERT_EQ(OK, deserialize_disk_layout(blob.data(), blob.size(), out));
    ASSERT_EQ(in.size(), out.size());
    for (size_t i = 0; i < in.size(); i++) {
        EXPECT_EQ(in[i].start_lba, out[i].start_lba);
        EXPECT_EQ(in[i].sector_count, out[i].sector_count);
        EXPECT_EQ(in[i].mbr_type, out[i].mbr_type);
    }
    /* hostile counts never round-trip */
    std::vector<u8> bad(4, 0xFF);
    std::vector<DiskLayoutEntry> sink;
    EXPECT_NE(OK, deserialize_disk_layout(bad.data(), bad.size(), sink));
    /* inside a container: unknown-type skip stays intact for OLD readers is
     * a corpus-format property; here we prove OUR reader picks the record
     * up through the standard record loop */
    {
        auto mf = build_session(fat_candidate(), true, in);
        tree::ResultTree t;
        ScanStats stats;
        VolumeSessionInfo info;
        Reader r(MemFile::rd, MemFile::sk, MemFile::sz, &mf);
        ASSERT_EQ(OK, ScanController::load_session_summary(r, t, stats, info, nullptr));
        ASSERT_EQ(in.size(), stats.persisted_layout.size());
        for (size_t i = 0; i < in.size(); i++)
            EXPECT_EQ(in[i].start_lba, stats.persisted_layout[i].start_lba);
    }
}

/* ------------------------------------------------------------------ */
/* 4. the layout-changed flag (surfaced, never enforced)                */
/* ------------------------------------------------------------------ */

TEST(ResumeRevalidation, LayoutChangedFlagged) {
    std::vector<DiskLayoutEntry> layout;
    layout.push_back({2048, 100, 0x0B});

    /* (a) same table on disk -> no flag */
    {
        auto mf = build_session(fat_candidate(), true, layout);
        MemDevice disk(base_disk()); /* MBR entry: 2048/100 type 0x0B */
        tree::ResultTree t;
        ScanStats stats;
        VolumeSessionInfo info;
        Reader r(MemFile::rd, MemFile::sk, MemFile::sz, &mf);
        std::unique_ptr<ScanController> ctrl;
        ASSERT_EQ(OK, ScanController::resume(r, t, ScanConfig{}, stats, info, ctrl));
        ASSERT_EQ(OK, ctrl->run(&disk, nullptr, nullptr, nullptr, stats));
        EXPECT_FALSE(stats.layout_changed) << "identical layout must not flag";
    }
    /* (b) the entry's type changed -> flag */
    {
        auto mf = build_session(fat_candidate(), true, layout);
        auto img = base_disk();
        plant_mbr(img, 0x07); /* now NTFS-typed */
        MemDevice disk(std::move(img));
        tree::ResultTree t;
        ScanStats stats;
        VolumeSessionInfo info;
        Reader r(MemFile::rd, MemFile::sk, MemFile::sz, &mf);
        std::unique_ptr<ScanController> ctrl;
        ASSERT_EQ(OK, ScanController::resume(r, t, ScanConfig{}, stats, info, ctrl));
        ASSERT_EQ(OK, ctrl->run(&disk, nullptr, nullptr, nullptr, stats));
        EXPECT_TRUE(stats.layout_changed) << "a changed table must flag";
    }
    /* (c) old sessions (no REC_DISK_LAYOUT) never flag */
    {
        auto mf = build_session(fat_candidate(), false, {});
        MemDevice disk(base_disk());
        tree::ResultTree t;
        ScanStats stats;
        VolumeSessionInfo info;
        Reader r(MemFile::rd, MemFile::sk, MemFile::sz, &mf);
        std::unique_ptr<ScanController> ctrl;
        ASSERT_EQ(OK, ScanController::resume(r, t, ScanConfig{}, stats, info, ctrl));
        ASSERT_EQ(OK, ctrl->run(&disk, nullptr, nullptr, nullptr, stats));
        EXPECT_FALSE(stats.layout_changed) << "no persisted layout = no claim";
    }
}

/* ------------------------------------------------------------------ */
/* 5. the per-record geometry payload (the og's CSearchedPartitionInfo) */
/* ------------------------------------------------------------------ */

/* N-049's own falsifiable claim, pinned (the flag half of the v2 payload;
 * M-042's landing persisted it, this is the reproduction the finding asked
 * for): save a session with a candidate whose size_from_device_end is
 * true, reload, and inspect the restored candidate - the flag MUST be
 * true (before M-042's v2 it was silently false: never serialized). */
TEST(ResumeRevalidation, SizeSourceFlagRoundTrip) {
    /* flagged fallback shape: geometry absent, size derived from the
     * device tail (the GAP-S6-6 fallback family) */
    lost::Candidate c = fat_candidate();
    c.have_geometry = 0;
    c.size_from_device_end = 1;
    auto mf = build_session(c, false, {});
    tree::ResultTree t;
    ScanStats stats;
    VolumeSessionInfo info;
    Reader r(MemFile::rd, MemFile::sk, MemFile::sz, &mf);
    ASSERT_EQ(OK, ScanController::load_session_summary(r, t, stats, info, nullptr));
    ASSERT_EQ(1u, stats.lost_candidates.size());
    EXPECT_TRUE(stats.lost_candidates[0].size_from_device_end)
        << "the size-source flag must survive the save/load round trip";
    EXPECT_FALSE(stats.lost_candidates[0].have_geometry)
        << "the geometry-valid bit is independent of the size-source bit";

    /* and the inverse: an ordinary geometry-derived candidate keeps the
     * flag CLEAR through the same path (no false positives either) */
    lost::Candidate g = fat_candidate(); /* have_geometry = 1, flag clear */
    auto mf2 = build_session(g, false, {});
    Reader r2(MemFile::rd, MemFile::sk, MemFile::sz, &mf2);
    tree::ResultTree t2;
    ScanStats stats2;
    VolumeSessionInfo info2;
    ASSERT_EQ(OK, ScanController::load_session_summary(r2, t2, stats2, info2, nullptr));
    ASSERT_EQ(1u, stats2.lost_candidates.size());
    EXPECT_FALSE(stats2.lost_candidates[0].size_from_device_end);
    EXPECT_TRUE(stats2.lost_candidates[0].have_geometry);
}

TEST(ResumeRevalidation, CandidateGeometryRoundTrip) {
    /* discovery side: probe_fat fills the geometry evidence */
    {
        auto img = base_disk();
        MemDevice disk(std::move(img));
        std::vector<table::PartitionInfo> known;
        lost::DiscoveryResult dr;
        ASSERT_EQ(OK, lost::discover(&disk, known, 1, nullptr, nullptr, nullptr, dr));
        const lost::Candidate* fat_c = nullptr;
        for (const auto& c : dr.candidates)
            if (c.fs == lost::CandidateFs::Fat && c.start_lba == 2048)
                fat_c = &c;
        ASSERT_NE(nullptr, fat_c) << "the FAT candidate must be discovered";
        EXPECT_EQ(512u, fat_c->bytes_per_sector);
        EXPECT_EQ(1u, fat_c->sectors_per_cluster);
        EXPECT_EQ(1u, fat_c->have_geometry);
    }
    /* record side: v2 (41-byte) round trip vs v1 (25-byte) back-compat */
    {
        auto mf = build_session(fat_candidate(), false, {});
        tree::ResultTree t;
        ScanStats stats;
        VolumeSessionInfo info;
        Reader r(MemFile::rd, MemFile::sk, MemFile::sz, &mf);
        ASSERT_EQ(OK, ScanController::load_session_summary(r, t, stats, info, nullptr));
        ASSERT_EQ(1u, stats.lost_candidates.size());
        EXPECT_EQ(512u, stats.lost_candidates[0].bytes_per_sector);
        EXPECT_EQ(1u, stats.lost_candidates[0].sectors_per_cluster);
        EXPECT_EQ(63u, stats.lost_candidates[0].hidden_sectors);
        EXPECT_TRUE(stats.lost_candidates[0].have_geometry);
    }
    {
        /* hand-build a v1 (25-byte) record: the extras stay zeroed, the
         * candidate still restores (classification-only re-validation) */
        PartitionContext pc;
        pc.start_lba = 2048;
        pc.sector_count = 100;
        pc.fs_kind = 1; /* Fat */
        pc.score = 40;
        pc.kind = 1;
        std::vector<u8> v1;
        v1.resize(8);
        for (int i = 0; i < 8; i++)
            v1[i] = u8(pc.start_lba >> (8 * i));
        for (int i = 0; i < 8; i++)
            v1.push_back(u8(pc.sector_count >> (8 * i)));
        v1.push_back(u8(pc.fs_kind));
        v1.push_back(0);
        v1.push_back(0);
        v1.push_back(0);
        v1.push_back(u8(pc.score));
        v1.push_back(0);
        v1.push_back(0);
        v1.push_back(0);
        v1.push_back(pc.kind);
        ASSERT_EQ(25u, v1.size());
        PartitionContext back;
        ASSERT_EQ(OK, deserialize_partition_context(v1.data(), v1.size(), back));
        EXPECT_EQ(2048u, back.start_lba);
        EXPECT_EQ(0u, back.bytes_per_sector);
        EXPECT_EQ(0u, back.flags) << "v1 records carry no geometry";
    }
}

/* ------------------------------------------------------------------ */
/* the arbiter's own edge semantics                                     */
/* ------------------------------------------------------------------ */

TEST(ResumeRevalidation, ArbiterStartOffDiskDropsAndUnknownKeeps) {
    /* start LBA past the device: stale by construction */
    {
        MemDevice small(std::vector<u8>(512u * 64, 0));
        lost::Candidate c = fat_candidate();
        c.start_lba = 4096; /* the 64-sector device has no such sector */
        EXPECT_FALSE(lost::revalidate_candidate(&small, c));
    }
    /* no arbiter (device-less listing): never drop on our own absence */
    {
        lost::Candidate c = fat_candidate();
        EXPECT_TRUE(lost::revalidate_candidate(nullptr, c));
    }
    /* Unknown-family records cannot be re-probed: kept, matching the og's
     * applied-flag skip for unprobeable records */
    {
        MemDevice disk(base_disk());
        lost::Candidate c = fat_candidate();
        c.fs = lost::CandidateFs::Unknown;
        EXPECT_TRUE(lost::revalidate_candidate(&disk, c));
    }
    /* a geometry mismatch on the SAME boot position: the og's per-record
     * payload check - the persisted bps no longer matches, drop */
    {
        MemDevice disk(base_disk());
        lost::Candidate c = fat_candidate();
        c.bytes_per_sector = 4096; /* the disk says 512 at that LBA now */
        EXPECT_FALSE(lost::revalidate_candidate(&disk, c));
    }
}
