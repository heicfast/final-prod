/* test_writer_readpipe.cpp - M-136: the recovery reads ride the same
 * retry+cache substrate the scan built (orchestrator::build_read_pipe:
 * SliceShim -> RetryingDevice(3, Skip) -> CachedReader). The pre-fix
 * handoff gave the SafeWriter the BARE device, so a single transient
 * read error zero-filled a span permanently - the scan and the export
 * disagreed about the readability of the same byte range within one
 * session. These tests pin the fixed contract at the writer level:
 * a first-attempt failure recovers byte-identical (Done, not Salvaged),
 * and the zero-fill path fires only after the retry budget is spent.
 *
 * Licensed under the MIT license. See LICENSE.
 */
#include <gtest/gtest.h>
#include "test_tmpdir.hpp"
#include "cr/recovery/cr_safewriter.h"
#include "cr/orchestrator/cr_scan.h"
#include "cr/storage/cr_device.h"
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <vector>

using namespace cr;
using namespace cr::recovery;
namespace fs = std::filesystem;

namespace {

/* a device whose first `flaky` read calls fail with ERR_READ_FAILED and
 * every later call succeeds - the transient-hiccup model (USB flake, a
 * busy virtual disk, an SMR pause): the classic one-shot error the scan
 * has always retried past. */
class FlakyStartDevice final : public IStorageDevice {
public:
    FlakyStartDevice(std::vector<u8> b, u32 flaky) : data_(std::move(b)), flaky_(flaky) {
        geom_.logical_sector = 512;
        geom_.physical_sector = 512;
        geom_.sector_count = data_.size() / 512;
        geom_.total_bytes = data_.size();
    }
    u32 read_at(u64 off, void* buf, u32 len) override {
        ++calls_;
        if (failed_ < flaky_) {
            ++failed_;
            return ERR_READ_FAILED;
        }
        if (off > data_.size() || len > data_.size() - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        std::memcpy(buf, data_.data() + off, len);
        return OK;
    }
    const Geometry& geometry() const override { return geom_; }
    u32 calls() const { return calls_; }
    u32 failed() const { return failed_; }

private:
    std::vector<u8> data_;
    Geometry geom_{};
    u32 flaky_ = 0;
    u32 calls_ = 0;
    u32 failed_ = 0;
};

/* a device with a permanently bad byte range - the hard-failure model:
 * after the retry budget is spent the Skip policy zero-fills the span
 * and the item commits Salvaged (the pre-existing, correct contract). */
class BadRangeDevice final : public IStorageDevice {
public:
    BadRangeDevice(std::vector<u8> b, u64 bad_off, u64 bad_len)
        : data_(std::move(b)), bad_off_(bad_off), bad_len_(bad_len) {
        geom_.logical_sector = 512;
        geom_.physical_sector = 512;
        geom_.sector_count = data_.size() / 512;
        geom_.total_bytes = data_.size();
    }
    u32 read_at(u64 off, void* buf, u32 len) override {
        ++calls_;
        if (off > data_.size() || len > data_.size() - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        if (off < bad_off_ + bad_len_ && bad_off_ < off + len) {
            ++bad_calls_;
            return ERR_READ_FAILED;
        }
        std::memcpy(buf, data_.data() + off, len);
        return OK;
    }
    const Geometry& geometry() const override { return geom_; }
    u32 bad_calls() const { return bad_calls_; }

private:
    std::vector<u8> data_;
    Geometry geom_{};
    u64 bad_off_ = 0, bad_len_ = 0;
    u32 calls_ = 0;
    u32 bad_calls_ = 0;
};

/* the portable filesystem DestIo (the test_writer_dirs.cpp harness shape) */
class FsIo final : public DestIo {
public:
    explicit FsIo(fs::path root) : root_(std::move(root)) {}
    static fs::path norm(std::u16string p) {
        for (auto& c : p)
            if (c == u'\\')
                c = u'/';
        return fs::path(p);
    }
    u32 create_temp(u64 id, const std::u16string& final_path,
                    const std::u16string& temp_suffix) override {
        const fs::path fin = root_ / norm(final_path);
        std::basic_string<fs::path::value_type> suffix;
        for (u16 c : temp_suffix)
            suffix.push_back(fs::path::value_type(c));
        const fs::path tmp = fs::path(fin.native() + suffix);
        std::error_code ec;
        fs::create_directories(fin.parent_path(), ec);
        std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
        if (!f)
            return ERR_DEST_UNWRITABLE;
        f.close();
        open_[id] = Open{tmp, fin};
        return OK;
    }
    u32 mkdir(const std::u16string& path) override {
        std::error_code ec;
        fs::create_directories(root_ / norm(path), ec);
        return ec ? u32(ERR_DEST_UNWRITABLE) : u32(OK);
    }
    u32 write(u64 id, u64 off, const void* data, u32 len) override {
        auto it = open_.find(id);
        if (it == open_.end())
            return ERR_BAD_PARAMETER;
        std::ofstream f(it->second.temp, std::ios::binary | std::ios::in | std::ios::out);
        if (!f)
            return ERR_WRITE_FAILED;
        f.seekp(std::streamoff(off));
        f.write(static_cast<const char*>(data), len);
        return f ? OK : ERR_WRITE_FAILED;
    }
    u32 commit(u64 id) override {
        auto it = open_.find(id);
        if (it == open_.end())
            return ERR_BAD_PARAMETER;
        std::error_code ec;
        fs::rename(it->second.temp, it->second.final, ec);
        return ec ? u32(ERR_WRITE_FAILED) : u32(OK);
    }
    void close(u64 id) override {
        auto it = open_.find(id);
        if (it == open_.end())
            return;
        std::error_code ec;
        fs::remove(it->second.temp, ec);
        open_.erase(it);
    }
    bool exists(const std::u16string& p) override {
        std::error_code ec;
        return fs::exists(root_ / norm(p), ec);
    }

private:
    struct Open {
        fs::path temp, final;
    };
    fs::path root_;
    std::map<u64, Open> open_;
};

struct TmpDir {
    fs::path p;
    TmpDir() {
        p = cr_test::tmp_path("cr_wrpipe", "");
        fs::create_directories(p);
    }
    ~TmpDir() {
        std::error_code ec;
        fs::remove_all(p, ec);
    }
};

tree::FileProperties* file_props(const char* name, u64 off, u64 len) {
    auto* p = new tree::FileProperties();
    for (const char* q = name; *q; ++q)
        p->name.push_back(u16(u8(*q)));
    p->extents.push_back({off, len});
    p->logical_size = len;
    return p;
}

} // namespace

/* M-136's primary pin: a source whose first read attempts fail recovers
 * byte-identical through the pipe the entry points now build - the item
 * commits Done (not Salvaged), no zero-filled span, and the underlying
 * device actually served the retried reads. */
TEST(WriterReadPipe, TransientFailureRetriedNotZeroFilled) {
    const u64 len = 1ull << 20;
    std::vector<u8> img(4ull << 20, 0);
    std::memset(img.data() + 0x100000, 0xAA, size_t(len)); /* the file body */

    tree::ResultTree tree;
    auto* found = tree.add_root(u"Found");
    tree::FileProperties* pr = file_props("flake.dat", 0x100000, len);
    tree::FileNode* n = tree.add_file(found, pr);
    pr->release();
    tree.refresh_stats();

    /* four first-attempt failures anywhere in the stream: the hiccup */
    FlakyStartDevice dev(img, 4);
    orchestrator::ReadPipe rp = orchestrator::build_read_pipe(&dev, 8ull << 20, 256 * 1024);
    ASSERT_NE(nullptr, rp.pipe);
    ASSERT_NE(nullptr, rp.retry);

    TmpDir tmp;
    FsIo io(tmp.p);
    SafeWriter writer(io, rp.pipe, WriterConfig{});
    writer.set_read_error_source(rp.retry); /* the entry-point wiring */
    ASSERT_EQ(OK, writer.enqueue(n, u"out\\"));
    WriterStats stats;
    ASSERT_EQ(OK, writer.process(nullptr, nullptr, nullptr, stats));
    ASSERT_EQ(u64(1), stats.done);
    EXPECT_EQ(u64(0), stats.salvaged) << "a retried transient error must not Salvaged";
    EXPECT_EQ(u64(0), stats.errors);

    /* byte-identical content: every byte is the 0xAA pattern (a zero
     * fill anywhere in the body would show as 0x00) */
    std::error_code ec;
    const fs::path out = tmp.p / "out" / "flake.dat";
    ASSERT_TRUE(fs::exists(out, ec));
    EXPECT_EQ(len, u64(fs::file_size(out, ec)));
    std::ifstream f(out, std::ios::binary);
    ASSERT_TRUE(f);
    std::vector<u8> body(size_t(len), 1);
    f.read(reinterpret_cast<char*>(body.data()), std::streamsize(len));
    for (size_t i = 0; i < body.size(); ++i)
        ASSERT_EQ(0xAA, body[i]) << "zero-filled span at body byte " << i;

    /* the transient failures happened and were served on retry */
    EXPECT_GE(dev.failed(), 4u) << "the fault injection must have fired";
    EXPECT_GT(dev.calls(), dev.failed()) << "the retries must have succeeded";
}

/* the hard-failure contract, re-pinned by the M-135 landing (the
 * S72 lane's truncate-and-keep): a permanently bad range is attacked by
 * the full retry budget, then the item TRUNCATES at the first
 * unreadable byte - ReadError with cause 5, the partial committed, the
 * readable prefix intact (the og's error-9 family; the zero-fill
 * Salvaged shape this test originally pinned was the pre-M-135
 * behavior). */
TEST(WriterReadPipe, HardFailureZeroFillsOnlyAfterRetriesSpent) {
    const u64 len = 1ull << 20;
    std::vector<u8> img(4ull << 20, 0);
    std::memset(img.data() + 0x100000, 0xBB, size_t(len));
    const u64 bad_off = 0x100000 + 0x10000; /* 64 KiB into the body */
    const u64 bad_len = 512;                /* one sector */

    tree::ResultTree tree;
    auto* found = tree.add_root(u"Found");
    tree::FileProperties* pr = file_props("bad.dat", 0x100000, len);
    tree::FileNode* n = tree.add_file(found, pr);
    pr->release();
    tree.refresh_stats();

    BadRangeDevice dev(img, bad_off, bad_len);
    orchestrator::ReadPipe rp = orchestrator::build_read_pipe(&dev, 8ull << 20, 256 * 1024);
    ASSERT_NE(nullptr, rp.pipe);
    ASSERT_NE(nullptr, rp.retry);

    TmpDir tmp;
    FsIo io(tmp.p);
    SafeWriter writer(io, rp.pipe, WriterConfig{});
    writer.set_read_error_source(rp.retry); /* the entry-point wiring */
    ASSERT_EQ(OK, writer.enqueue(n, u"out\\"));
    WriterStats stats;
    ASSERT_EQ(OK, writer.process(nullptr, nullptr, nullptr, stats));
    /* M-135: the og's runlist path - truncate at the bad byte, keep the
     * partial, record the numbered cause; NOT done, NOT Salvaged */
    EXPECT_EQ(u64(0), stats.done);
    EXPECT_EQ(u64(0), stats.salvaged);
    EXPECT_EQ(u64(1), stats.read_error);
    const auto items = writer.report_items();
    ASSERT_EQ(1u, items.size());
    EXPECT_EQ(recovery::report::Outcome::ReadError, items[0].outcome);
    EXPECT_EQ(5u, items[0].cause_id);
    EXPECT_EQ(0x10000u, items[0].first_failed_offset);
    EXPECT_EQ(0x10000u, items[0].bytes_written); /* the readable prefix */

    /* the bad range was retried before being given up (>= the policy's
     * 3 attempts touch it - bisection may add more, never fewer) */
    EXPECT_GE(dev.bad_calls(), 3u) << "the truncation must fire only after the retry budget";

    /* content: the partial carries the 0xBB prefix up to the bad byte */
    std::error_code ec;
    const fs::path out = tmp.p / "out" / "bad.dat";
    ASSERT_TRUE(fs::exists(out, ec));
    EXPECT_EQ(0x10000u, u64(fs::file_size(out, ec))); /* the truncation */
    std::ifstream f(out, std::ios::binary);
    ASSERT_TRUE(f);
    std::vector<u8> body(0x10000, 1);
    f.read(reinterpret_cast<char*>(body.data()), std::streamsize(body.size()));
    for (size_t i = 0; i < body.size(); ++i)
        ASSERT_EQ(0xBB, body[i]) << "good-data byte " << i << " must be the real prefix";
}

/* the pre-fix shape as a CONTRAST PIN (the BacktrackOff precedent),
 * re-pinned by the M-135 landing: the same transient failure against
 * the BARE device (the pre-M-136 entry-point handoff) TRUNCATES the
 * item at the failing read - ReadError, cause 5 - where the pipe
 * retries it and completes Done (the finding's falsifiable case, kept
 * executable so the pipe's value stays proven: scan and export must
 * agree about the readability of the same range). */
TEST(WriterReadPipe, BareDeviceLegacyShapeZeroFillsTransients) {
    const u64 len = 1ull << 20;
    std::vector<u8> img(4ull << 20, 0);
    std::memset(img.data() + 0x100000, 0xAA, size_t(len));

    tree::ResultTree tree;
    auto* found = tree.add_root(u"Found");
    tree::FileProperties* pr = file_props("legacy.dat", 0x100000, len);
    tree::FileNode* n = tree.add_file(found, pr);
    pr->release();
    tree.refresh_stats();

    FlakyStartDevice dev(img, 4);
    TmpDir tmp;
    FsIo io(tmp.p);
    SafeWriter writer(io, &dev, WriterConfig{}); /* the pre-fix handoff */
    ASSERT_EQ(OK, writer.enqueue(n, u"out\\"));
    WriterStats stats;
    ASSERT_EQ(OK, writer.process(nullptr, nullptr, nullptr, stats));
    /* the pre-fix defect, re-pinned by M-135: without the retry layer a
     * retryable hiccup truncates the item at the failing read - the
     * bytes the device COULD serve on attempt 2 never fetched */
    EXPECT_EQ(u64(0), stats.done);
    EXPECT_EQ(u64(0), stats.salvaged);
    EXPECT_EQ(u64(1), stats.read_error);
    const auto items = writer.report_items();
    ASSERT_EQ(1u, items.size());
    EXPECT_EQ(recovery::report::Outcome::ReadError, items[0].outcome);
    EXPECT_EQ(5u, items[0].cause_id);
}

/* the construction-seam pin: the pipe the entry points build IS the
 * scan's own substrate - retries visible at the device AND the cache
 * absorbing a repeat read (two identical pipe reads = one device read).
 * This is the "CachedReaderSharedWithScan" seam: same helper, same
 * constants, same layered device. */
TEST(WriterReadPipe, ConstructionIsTheScanSubstrate) {
    std::vector<u8> img(1ull << 20, 0x7E);
    FlakyStartDevice dev(img, 1); /* one transient failure to retry past */
    orchestrator::ReadPipe rp = orchestrator::build_read_pipe(&dev, 8ull << 20, 256 * 1024);
    ASSERT_NE(nullptr, rp.pipe);
    ASSERT_NE(nullptr, rp.retry);
    ASSERT_NE(nullptr, rp.chain); /* the full chain is live (no degrade) */

    u8 a[4096], b[4096];
    /* read 1: the transient failure is retried inside the pipe */
    ASSERT_EQ(OK, rp.pipe->read_at(0, a, sizeof(a)));
    EXPECT_EQ(0x7E, a[0]);
    const u32 calls_after_first = /* device reads so far */ dev.calls();
    /* read 2: identical range - served by the cache, no device read */
    ASSERT_EQ(OK, rp.pipe->read_at(0, b, sizeof(b)));
    EXPECT_EQ(0, std::memcmp(a, b, sizeof(a)));
    EXPECT_EQ(calls_after_first, dev.calls()) << "the cache must absorb the repeat read";
    EXPECT_GE(dev.failed(), 1u) << "the retry layer must have absorbed the hiccup";
}
