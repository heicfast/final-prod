/* test_rescue.cpp - F-33/F-34 acceptance: the ddrescue-style two-pass
 * imager and the resumable mapfile.
 *
 * Coverage (every claim a test):
 *   - RescueMap: mark splits/carves/merges; find_next/find_prev clipping;
 *     mapfile round-trip (to text -> parse -> identical); out-of-range
 *     marks refuse; parse rejects overlaps/out-of-order/unknown status;
 *   - the clean device: fully rescued, all Finished, correct bytes;
 *   - the damaged device: a two-sector bad patch isolated to exactly
 *     BadSector == 2*sector, everything else Finished (the SF test's
 *     behavior);
 *   - resume: a preloaded map does not redo Finished work (the sink sees
 *     writes ONLY outside the finished region); a wrong-size map refuses;
 *     a map claiming more than the dest holds refuses (checked at the
 *     driver level - here: the imager's total-vs-source refusal);
 *   - reverse: the last chunk is attempted first (the sink's first write
 *     is at the tail) and damage still isolates;
 *   - periodic-checkpoint policy: a forced map-save failure propagates;
 *   - the stats derive from the map (rescued/bad/pending sum to total).
 *
 * Licensed under the MIT license. See LICENSE.
 */
#include <gtest/gtest.h>
#include "cr/storage/cr_rescue.h"
#include <cstring>
#include <map>
#include <string>
#include <vector>

using namespace cr;
using namespace cr::storage;

namespace {

/* a device with a configurable bad range: reads touching [bad, bad+bad_len)
 * fail with ERR_READ_FAILED, everything else serves the pattern */
class BadRangeDevice final : public IStorageDevice {
public:
    BadRangeDevice(u64 size, u64 bad_off, u64 bad_len)
        : data_(size_t(size)), bad_(bad_off), bad_len_(bad_len) {
        for (size_t i = 0; i < data_.size(); ++i)
            data_[i] = u8((i * 7 + 3) & 0xFF);
        geom_.logical_sector = 512;
        geom_.physical_sector = 512;
        geom_.sector_count = size / 512;
        geom_.total_bytes = size;
    }
    u32 read_at(u64 off, void* buf, u32 len) override {
        if (off > data_.size() || len > data_.size() - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        if (bad_len_ > 0 && off < bad_ + bad_len_ && off + len > bad_)
            return ERR_READ_FAILED;
        std::memcpy(buf, data_.data() + off, len);
        reads_++;
        return OK;
    }
    const Geometry& geometry() const override { return geom_; }
    u64 reads() const { return reads_; }
    const std::vector<u8>& bytes() const { return data_; }

private:
    std::vector<u8> data_;
    u64 bad_ = 0, bad_len_ = 0, reads_ = 0;
    Geometry geom_;
};

/* in-memory sink recording every positioned write */
class MemSink final : public RescueImageSink {
public:
    u32 pwrite(u64 offset, const void* data, u32 len) override {
        if (offset + len > image_.size())
            image_.resize(size_t(offset + len));
        std::memcpy(image_.data() + offset, data, len);
        writes_.push_back({offset, len});
        return OK;
    }
    u32 flush() override { return OK; }
    const std::vector<std::pair<u64, u32>>& writes() const { return writes_; }
    std::vector<u8> image_;

private:
    std::vector<std::pair<u64, u32>> writes_;
};

/* in-memory mapfile IO with an injectable forced-write failure */
class MemMapIo final : public RescueMapIo {
public:
    u32 write_map(const std::string& text, bool forced) override {
        if (fail_forced_ && forced)
            return ERR_WRITE_FAILED;
        last_text_ = text;
        saved_++;
        return OK;
    }
    u32 read_map(std::string* out) override {
        if (!have_)
            return ERR_OPEN_FAILED;
        *out = last_text_;
        return OK;
    }
    bool map_exists() const override { return have_; }
    void set_have(bool h) { have_ = h; }
    const std::string& text() const { return last_text_; }
    int saves() const { return saved_; }
    void fail_forced_writes() { fail_forced_ = true; }

private:
    std::string last_text_;
    int saved_ = 0;
    bool have_ = false, fail_forced_ = false;
};

} // namespace

/* ---- the map -------------------------------------------------------------- */

TEST(RescueMap, MarkSplitsCarvesMerges) {
    RescueMap m;
    m.reset(10 * 512);
    EXPECT_EQ(m.blocks().size(), size_t(1));
    EXPECT_TRUE(m.mark(512, 512, RescueStatus::Finished));
    /* two finished neighbors merge (the trailing NonTried keeps its own
     * block: [0,512)? + [512,1536)+ + [1536,5120)? ) */
    EXPECT_TRUE(m.mark(1024, 512, RescueStatus::Finished));
    EXPECT_EQ(m.blocks().size(), size_t(3));
    EXPECT_EQ(m.blocks()[0].status, RescueStatus::NonTried);
    EXPECT_EQ(m.blocks()[1].status, RescueStatus::Finished);
    EXPECT_EQ(m.blocks()[1].pos, 512u);
    EXPECT_EQ(m.blocks()[1].size, 1024u);
    /* a carve in the middle */
    EXPECT_TRUE(m.mark(1024, 512, RescueStatus::BadSector));
    EXPECT_EQ(m.blocks().size(), size_t(4));
    EXPECT_EQ(m.blocks()[1].status, RescueStatus::Finished);  /* [512,1024) */
    EXPECT_EQ(m.blocks()[2].status, RescueStatus::BadSector); /* [1024,1536) */
    /* out-of-range refuses */
    EXPECT_FALSE(m.mark(10 * 512, 512, RescueStatus::Finished));
    EXPECT_FALSE(m.mark(9 * 512, 1024, RescueStatus::Finished));
}

TEST(RescueMap, FindNextPrevClipping) {
    RescueMap m;
    m.reset(4096);
    EXPECT_TRUE(m.mark(1024, 2048, RescueStatus::Finished));
    const RescueStatus fin[1] = {RescueStatus::Finished};
    u64 pos = 0, size = 0;
    /* from the middle of the finished block: clipped to the remainder */
    EXPECT_TRUE(m.find_next(2048, fin, 1, &pos, &size));
    EXPECT_EQ(pos, 2048u);
    EXPECT_EQ(size, 1024u);
    /* past it: nothing */
    EXPECT_FALSE(m.find_next(4096, fin, 1, &pos, &size));
    /* before: the last finished block ends at 3072 */
    EXPECT_TRUE(m.find_prev(4096, fin, 1, &pos, &size));
    EXPECT_EQ(pos, 1024u);
    EXPECT_EQ(size, 2048u);
    /* before the block: nothing */
    EXPECT_FALSE(m.find_prev(1024, fin, 1, &pos, &size));
}

TEST(RescueMap, MapfileRoundTrip) {
    RescueMap m;
    m.reset(64 * 1024);
    m.mark(512, 4096, RescueStatus::Finished);
    m.mark(512 + 4096, 512, RescueStatus::BadSector);
    m.mark(32 * 1024, 1024, RescueStatus::NonScraped);
    m.set_current_pos(4608);
    const std::string text = m.to_mapfile_text();
    RescueMap back;
    ASSERT_TRUE(RescueMap::parse(text, &back));
    ASSERT_EQ(back.blocks().size(), m.blocks().size());
    for (size_t i = 0; i < m.blocks().size(); ++i) {
        EXPECT_EQ(back.blocks()[i].pos, m.blocks()[i].pos);
        EXPECT_EQ(back.blocks()[i].size, m.blocks()[i].size);
        EXPECT_EQ(back.blocks()[i].status, m.blocks()[i].status);
    }
    EXPECT_EQ(back.current_pos(), m.current_pos());
    EXPECT_EQ(back.total(), m.total());
    /* the status characters are the ddrescue set */
    EXPECT_NE(text.find('*'), std::string::npos);
}

TEST(RescueMap, ParseRejectsHostileInput) {
    RescueMap m;
    /* overlap */
    EXPECT_FALSE(RescueMap::parse("0x0  0x1000  +\n0x800  0x1000  +\n", &m));
    /* out of order (hole) */
    EXPECT_FALSE(RescueMap::parse("0x0  0x200  +\n0x400  0x200  +\n", &m));
    /* unknown status */
    EXPECT_FALSE(RescueMap::parse("0x0  0x200  X\n", &m));
    /* zero-size block */
    EXPECT_FALSE(RescueMap::parse("0x0  0x0  +\n", &m));
    /* comments and blank lines are tolerated (blocks cover [0,total) -
     * the map starts at 0 by construction) */
    EXPECT_TRUE(RescueMap::parse("# a comment\n\n0x0  0x200  ?\n", &m));
}

/* ---- the imager ------------------------------------------------------------ */

TEST(RescueImager, CleanDeviceFullyRescued) {
    BadRangeDevice dev(4u << 20, 0, 0);
    MemSink sink;
    MemMapIo map_io;
    RescueImagerConfig cfg;
    cfg.initial_chunk = 64 * 1024;
    RescueImager imager(dev, sink, map_io, cfg);
    RescueStats st;
    ASSERT_EQ(imager.run(nullptr, nullptr, nullptr, nullptr, &st), u32(OK));
    EXPECT_TRUE(imager.map().all_finished());
    EXPECT_EQ(st.rescued_bytes, dev.size());
    EXPECT_EQ(st.bad_bytes, 0u);
    EXPECT_EQ(st.pending_bytes, 0u);
    /* the image is byte-identical to the source */
    ASSERT_EQ(sink.image_.size(), dev.bytes().size());
    EXPECT_EQ(std::memcmp(sink.image_.data(), dev.bytes().data(), sink.image_.size()), 0);
    /* stats sum to the total (the truth invariant) */
    EXPECT_EQ(st.rescued_bytes + st.bad_bytes + st.pending_bytes, dev.size());
}

TEST(RescueImager, BadPatchIsolatedExactly) {
    /* a two-sector bad patch: the map isolates EXACTLY those bytes as
     * BadSector, everything else Finished (the SF acceptance shape) */
    const u64 bad_off = 1u << 20; /* 1 MiB in */
    BadRangeDevice dev(4u << 20, bad_off, 1024);
    MemSink sink;
    MemMapIo map_io;
    RescueImagerConfig cfg;
    cfg.initial_chunk = 64 * 1024;
    cfg.skip_size = 64 * 1024;
    RescueImager imager(dev, sink, map_io, cfg);
    RescueStats st;
    ASSERT_EQ(imager.run(nullptr, nullptr, nullptr, nullptr, &st), u32(OK));
    EXPECT_EQ(imager.map().bytes_of(RescueStatus::BadSector), 1024u);
    EXPECT_EQ(imager.map().bytes_of(RescueStatus::Finished), dev.size() - 1024);
    EXPECT_EQ(st.bad_bytes, 1024u);
    /* the good bytes around the hole are the REAL device bytes */
    ASSERT_GE(sink.image_.size(), size_t(bad_off));
    EXPECT_EQ(std::memcmp(sink.image_.data(), dev.bytes().data(), size_t(bad_off)), 0);
    /* the hole was never written (no synthesized zeros) */
    bool hole_written = false;
    for (const auto& w : sink.writes())
        if (w.first < bad_off + 1024 && w.first + w.second > bad_off)
            hole_written = true;
    EXPECT_FALSE(hole_written);
}

TEST(RescueImager, ReverseAttemptsTailFirst) {
    BadRangeDevice dev(2u << 20, 0, 0);
    MemSink sink;
    MemMapIo map_io;
    RescueImagerConfig cfg;
    cfg.initial_chunk = 64 * 1024;
    cfg.reverse = true;
    RescueImager imager(dev, sink, map_io, cfg);
    RescueStats st;
    ASSERT_EQ(imager.run(nullptr, nullptr, nullptr, nullptr, &st), u32(OK));
    ASSERT_FALSE(sink.writes().empty());
    /* the FIRST write sits at the tail (the reverse sweep starts at the
     * end - "reading a damaged area from the other side often gets more
     * of it") */
    const auto& first = sink.writes().front();
    EXPECT_GT(first.first, dev.size() - cfg.initial_chunk - 1);
    EXPECT_TRUE(imager.map().all_finished());
}

TEST(RescueImager, ResumeDoesNotRedoFinishedWork) {
    BadRangeDevice dev(4u << 20, 0, 0);
    MemSink sink;
    MemMapIo map_io;
    RescueImagerConfig cfg;
    cfg.initial_chunk = 64 * 1024;

    /* run 1: cancel after the first progress event */
    struct Ctx {
        int events = 0;
        static void cb(void* user, const RescueStats&) {
            auto* c = static_cast<Ctx*>(user);
            if (++c->events >= 3)
                c->token.cancel();
        }
        PausableToken token; /* the concrete cancel surface (G-07) */
    } ctx;
    RescueImager run1(dev, sink, map_io, cfg);
    const u32 rc1 = run1.run(nullptr, &ctx.token, Ctx::cb, &ctx, nullptr);
    EXPECT_EQ(rc1, u32(ERR_CANCELLED));
    const u64 rescued1 = run1.map().finished_bytes();
    EXPECT_GT(rescued1, 0u);
    EXPECT_LT(rescued1, dev.size());

    /* run 2 (resume): a FRESH sink; the preloaded map must prevent any
     * write inside the finished region */
    MemSink sink2;
    RescueImager run2(dev, sink2, map_io, cfg);
    RescueMap pre = run1.map();
    RescueStats st2;
    ASSERT_EQ(run2.run(&pre, nullptr, nullptr, nullptr, &st2), u32(OK));
    for (const auto& w : sink2.writes()) {
        /* no write touches the already-finished prefix */
        EXPECT_GE(w.first, rescued1) << "resume redid finished work at " << w.first;
    }
    EXPECT_TRUE(run2.map().all_finished());
    /* and the total rescued is the full device */
    EXPECT_EQ(st2.rescued_bytes, dev.size());
}

TEST(RescueImager, WrongSizeMapRefused) {
    BadRangeDevice dev(4u << 20, 0, 0);
    MemSink sink;
    MemMapIo map_io;
    RescueImagerConfig cfg;
    RescueMap wrong;
    wrong.reset(dev.size() - 512);
    RescueImager imager(dev, sink, map_io, cfg);
    EXPECT_EQ(imager.run(&wrong, nullptr, nullptr, nullptr, nullptr), u32(ERR_BAD_PARAMETER));
}

TEST(RescueImager, ForcedMapSaveFailurePropagates) {
    BadRangeDevice dev(1u << 20, 0, 0);
    MemSink sink;
    MemMapIo map_io;
    map_io.fail_forced_writes();
    RescueImagerConfig cfg;
    RescueImager imager(dev, sink, map_io, cfg);
    /* the fresh-run initial forced checkpoint fails -> the run refuses */
    EXPECT_EQ(imager.run(nullptr, nullptr, nullptr, nullptr, nullptr), u32(ERR_WRITE_FAILED));
}

TEST(RescueImager, PeriodicCheckpointsFire) {
    BadRangeDevice dev(4u << 20, 0, 0);
    MemSink sink;
    MemMapIo map_io;
    RescueImagerConfig cfg;
    cfg.initial_chunk = 64 * 1024;
    cfg.checkpoint_interval_ms = 0; /* every chunk boundary */
    RescueImager imager(dev, sink, map_io, cfg);
    ASSERT_EQ(imager.run(nullptr, nullptr, nullptr, nullptr, nullptr), u32(OK));
    EXPECT_GE(map_io.saves(), 2); /* initial + pass boundary + final */
}

TEST(RescueImager, EmptyDeviceParity) {
    BadRangeDevice dev(0, 0, 0);
    MemSink sink;
    MemMapIo map_io;
    RescueImager imager(dev, sink, map_io);
    RescueStats st;
    ASSERT_EQ(imager.run(nullptr, nullptr, nullptr, nullptr, &st), u32(OK));
    EXPECT_TRUE(imager.map().all_finished());
    EXPECT_EQ(st.rescued_bytes, 0u);
    EXPECT_TRUE(sink.writes().empty());
}
