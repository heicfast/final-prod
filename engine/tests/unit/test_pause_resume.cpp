/* test_pause_resume.cpp - G-07 acceptance: PausableToken semantics,
 * pause-time compensation, truthful stage_done_mask, save-on-cancel, and
 * resume-skips-completed-stages. Also exercises telemetry concurrency
 * (G-04/G-05/G-06) on the same scan threads (the TSan build runs this file
 * under -fsanitize=thread in CI).
 *
 * Licensed under the MIT license. See LICENSE.
 */
#include <gtest/gtest.h>
#include "cr/orchestrator/cr_scan.h"
#include "cr/recovery/cr_safewriter.h"
#include "cr/session/cr_crsf.h"
#include "cr/storage/cr_device.h"
#include "cr/storage/cr_slice.h"
#include <atomic>
#include <chrono>
#include <cstring>
#include <thread>
#include <vector>

using namespace cr;
using namespace cr::orchestrator;

namespace {

class MemDevice final : public IStorageDevice {
public:
    explicit MemDevice(std::vector<u8> b) : data_(std::move(b)) {
        geom_.logical_sector = 512;
        geom_.physical_sector = 512;
        geom_.sector_count = data_.size() / 512;
        geom_.total_bytes = data_.size();
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

/* A device whose reads BLOCK until released - drives the scan into a
 * deterministic mid-stage point for cancel/pause tests. */
class GateDevice final : public IStorageDevice {
public:
    explicit GateDevice(std::vector<u8> b) : data_(std::move(b)) {
        geom_.logical_sector = 512;
        geom_.physical_sector = 512;
        geom_.sector_count = data_.size() / 512;
        geom_.total_bytes = data_.size();
    }
    void hold() { hold_.store(true, std::memory_order_release); }
    void release() { hold_.store(false, std::memory_order_release); }
    u32 read_at(u64 off, void* buf, u32 len) override {
        while (hold_.load(std::memory_order_acquire))
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        ++reads_;
        if (off > data_.size() || len > data_.size() - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        std::memcpy(buf, data_.data() + off, len);
        return OK;
    }
    const Geometry& geometry() const override { return geom_; }
    u64 reads() const { return reads_.load(); }

private:
    std::vector<u8> data_;
    Geometry geom_;
    std::atomic<bool> hold_{false};
    std::atomic<u64> reads_{0};
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

/* FAT16 fixture (same shape as test_orchestrator's): 5074 sectors,
 * clusters 5(root),10(file),20(file). */
std::vector<u8> build_fat16() {
    std::vector<u8> img(u64(5074) * 512, 0);
    u8* b = img.data();
    std::memcpy(b + 3, "MSWIN4.1", 8);
    put16(img, 0x0B, 512);
    b[0x0D] = 1;
    put16(img, 0x0E, 2);
    b[0x10] = 2;
    put16(img, 0x11, 512);
    put16(img, 0x13, 5074);
    b[0x15] = 0xF8;
    put16(img, 0x16, 20);
    put16(img, 0x18, 63);
    put16(img, 0x1A, 255);
    b[0x26] = 0x29;
    std::memcpy(b + 0x2B, "TESTVOL    ", 11);
    std::memcpy(b + 0x36, "FAT16   ", 8);
    b[510] = 0x55;
    b[511] = 0xAA;

    const size_t FAT1 = 2 * 512, FAT2 = 22 * 512, ROOT = 42 * 512, DATA = 74 * 512;
    put16(img, FAT1 + 5 * 2, 0xFFFF);
    put16(img, FAT2 + 5 * 2, 0xFFFF);
    put16(img, FAT1 + 10 * 2, 11);
    put16(img, FAT2 + 10 * 2, 11);
    put16(img, FAT1 + 11 * 2, 12);
    put16(img, FAT2 + 11 * 2, 12);
    put16(img, FAT1 + 12 * 2, 0xFFFF);
    put16(img, FAT2 + 12 * 2, 0xFFFF);
    put16(img, FAT1 + 20 * 2, 0xFFFF);
    put16(img, FAT2 + 20 * 2, 0xFFFF);

    u8* root = img.data() + ROOT;
    auto dentry = [&](size_t slot, const char* n11, u8 attr, u32 cl, u32 sz, bool del) {
        u8* e = root + slot * 32;
        std::memset(e, 0, 32);
        for (int i = 0; i < 11; ++i)
            e[i] = u8(n11[i]);
        if (del)
            e[0] = 0xE5;
        e[11] = attr;
        put16(img, ROOT + slot * 32 + 20, u16(cl >> 16));
        put16(img, ROOT + slot * 32 + 26, u16(cl));
        put32v(img, ROOT + slot * 32 + 28, sz);
    };
    dentry(0, "DOC     TXT", 0x20, 10, 1000, false);
    dentry(1, "DELFILE TXT", 0x20, 20, 500, true);
    std::memcpy(img.data() + DATA + (10 - 2) * 512, "DOCDATA", 7);
    std::memcpy(img.data() + DATA + (20 - 2) * 512, "DELDATA", 7);
    return img;
}

/* ---- file-backed CRSF IO adapters (same shape as the CLI's) ---- */
u32 file_write(void* user, const void* bytes, u32 len) {
    FILE* f = static_cast<FILE*>(user);
    return std::fwrite(bytes, 1, len, f) == len ? OK : ERR_SESSION_IO;
}
u64 file_tell(void* user) {
    return u64(ftell(static_cast<FILE*>(user)));
}
u32 file_seek(void* user, u64 absolute) {
    return fseek(static_cast<FILE*>(user), long(absolute), SEEK_SET) == 0 ? OK
                                                                          : ERR_SESSION_IO;
}
u32 file_read(void* user, void* buf, u32 len) {
    FILE* f = static_cast<FILE*>(user);
    return std::fread(buf, 1, len, f) == len ? OK : ERR_SESSION_IO;
}
u64 file_size(void* user) {
    FILE* f = static_cast<FILE*>(user);
    const long cur = ftell(f);
    fseek(f, 0, SEEK_END);
    const u64 sz = u64(ftell(f));
    fseek(f, cur, SEEK_SET);
    return sz;
}

std::string tmp_session_path() {
    static std::atomic<int> n{0};
    char buf[128];
    snprintf(buf, sizeof(buf), "/tmp/cr_test_session_%d_%d.crsf", int(getpid()), n.fetch_add(1));
    return std::string(buf);
}

} // namespace

/* ---------------- token semantics ---------------- */

TEST(PauseResume, TokenCheckpointBlocksAndResumes) {
    PausableToken tok;
    ASSERT_FALSE(tok.cancelled());
    ASSERT_FALSE(tok.paused());
    ASSERT_FALSE(tok.checkpoint());

    tok.pause();
    ASSERT_TRUE(tok.paused());
    ASSERT_FALSE(tok.cancelled());

    std::atomic<bool> unblocked{false};
    std::thread waiter([&] {
        ASSERT_FALSE(tok.checkpoint()); /* blocks until resume */
        unblocked.store(true);
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(80));
    ASSERT_FALSE(unblocked.load()); /* still parked */
    tok.resume();
    waiter.join();
    ASSERT_TRUE(unblocked.load());
    ASSERT_FALSE(tok.paused());
}

TEST(PauseResume, CancelWhilePausedWakesWaiter) {
    PausableToken tok;
    tok.pause();
    std::atomic<bool> saw_cancel{false};
    std::thread waiter([&] {
        if (tok.checkpoint())
            saw_cancel.store(true);
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    tok.cancel();
    waiter.join();
    ASSERT_TRUE(saw_cancel.load());
    ASSERT_TRUE(tok.cancelled());
    ASSERT_FALSE(tok.paused());
}

TEST(PauseResume, PausedMsAccounting) {
    PausableToken tok;
    ASSERT_EQ(u64(0), tok.paused_ms());
    tok.pause();
    std::this_thread::sleep_for(std::chrono::milliseconds(120));
    ASSERT_GE(tok.paused_ms(), u64(100)); /* running pause accumulates */
    tok.resume();
    const u64 held = tok.paused_ms();
    ASSERT_GE(held, u64(110));
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    ASSERT_EQ(held, tok.paused_ms()); /* no drift while running */
}

/* ---------------- scan integration ---------------- */

TEST(PauseResume, ScanPauseFreezesAndElapsedExcludesPause) {
    auto dev = std::make_unique<GateDevice>(build_fat16());
    GateDevice* gate = dev.get();
    PausableToken tok;
    tree::ResultTree tree;
    ScanConfig cfg; /* quick+deep+carve over the FAT volume */
    ScanController ctrl(tree, cfg);
    ScanStats stats;

    const auto wall_t0 = std::chrono::steady_clock::now();
    gate->hold(); /* park the very first read (boot probe) */
    std::thread scan([&] {
        (void)ctrl.run(gate, &tok, nullptr, nullptr, stats);
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(150)); /* scan parked */
    tok.pause();
    gate->release(); /* scan proceeds to the next checkpoint and freezes */
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    {
        const auto t = ctrl.telemetry();
        ASSERT_TRUE(t.running);
        ASSERT_TRUE(t.paused); /* live refresh from the token, not the
                                 stage-boundary-stamped snapshot */
    }
    const u64 paused_before = tok.paused_ms();
    ASSERT_GE(paused_before, u64(150));
    tok.resume();
    scan.join();
    /* pause compensation (RE parity), measured against the same clock:
     * elapsed + paused must reconstruct the wall time (within scheduler
     * slack) and the pause must have been substantial. Without
     * compensation elapsed == wall and the sum overshoots by ~200 ms. */
    const u64 wall = u64(std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - wall_t0)
                             .count());
    ASSERT_GE(tok.paused_ms(), u64(180));
    EXPECT_GE(wall, u64(300)); /* timing held (CI sanity) */
    EXPECT_LE(stats.elapsed_ms + tok.paused_ms(), wall + u64(60));
}

TEST(PauseResume, SaveOnCancelStampsTruthfulMaskAndResumeSkipsQuick) {
    const std::string path = tmp_session_path();
    ScanConfig cfg;
    {
        /* Phase 1: cancel the scan exactly at the quick->deep boundary.
         * covers_both: the single walk carries deep semantics, so quick
         * completion stamps BOTH stages -> boundary overall = 750
         * (= weighted(1000,1000,0)). The WALK's final tick also reports
         * 750 while quick.completed is still false - cancelling there
         * would abort the engine mid-run (mask would stay 0, which is
         * also truthful). The boundary is distinguished by the quick
         * stage being STAMPED COMPLETE: only run_quick's post-stamp
         * tick sees quick.completed == true, so the cancel lands between
         * the stages, with quick_done_ already set. */
        struct CancelCtx {
            PausableToken* tok;
            ScanController* ctrl;
        };
        MemDevice dev(build_fat16());
        tree::ResultTree tree;
        ScanController ctrl(tree, cfg);
        ScanStats stats;
        PausableToken tok;
        CancelCtx c{&tok, &ctrl};
        auto cancel_at_deep = [](u32 overall, void* user) {
            CancelCtx* ctx = static_cast<CancelCtx*>(user);
            if (overall >= 750 && ctx->ctrl->telemetry().quick.completed) {
                ctx->tok->cancel(); /* quick+deep walk done, carve not started */
                return false;
            }
            return true;
        };
        const u32 rc = ctrl.run(&dev, &tok, cancel_at_deep, &c, stats);
        ASSERT_EQ(ERR_CANCELLED, rc);
        ASSERT_TRUE(ctrl.telemetry().cancelled);
        /* tree retains quick results; refresh before saving (CLI parity) */
        tree.refresh_stats();
        FILE* f = fopen(path.c_str(), "wb");
        ASSERT_NE(nullptr, f);
        session::Writer w(file_write, file_tell, file_seek, f);
        ASSERT_EQ(OK, w.open(0x52434F56));
        ASSERT_EQ(OK, ctrl.save_session(w, stats));
        ASSERT_EQ(OK, w.finalize());
        fclose(f);
    }
    {
        /* Phase 2: resume - quick must NOT re-run (no duplicate emissions
         * into the restored roots) and carve must complete. */
        MemDevice dev(build_fat16());
        tree::ResultTree tree;
        ScanStats stats;
        session::VolumeSessionInfo info;
        FILE* f = fopen(path.c_str(), "rb");
        ASSERT_NE(nullptr, f);
        session::Reader r(file_read, file_seek, file_size, f);
        std::unique_ptr<ScanController> ctrl;
        ASSERT_EQ(OK, ScanController::resume(r, tree, cfg, stats, info, ctrl));
        fclose(f);
        /* truthful mask: quick done, deep NOT done (cancel hit at boundary),
         * carve NOT done */
        EXPECT_EQ(STAGE_QUICK, info.stage_done_mask);
        EXPECT_EQ(STAGE_QUICK | STAGE_DEEP | STAGE_CARVE, info.configured_mask);
        ASSERT_NE(nullptr, ctrl.get());
        EXPECT_FALSE(ctrl->config().run_quick); /* skipped */
        EXPECT_TRUE(ctrl->config().run_deep);
        EXPECT_TRUE(ctrl->config().run_carve);
        ASSERT_EQ(OK, ctrl->run(&dev, nullptr, nullptr, nullptr, stats));

        /* reference: a single full fresh scan */
        tree::ResultTree ref_tree;
        ScanController ref(ref_tree, cfg);
        ScanStats ref_stats;
        MemDevice ref_dev(build_fat16());
        ASSERT_EQ(OK, ref.run(&ref_dev, nullptr, nullptr, nullptr, ref_stats));
        /* resumed scan must not double-emit the quick pass: exactly the
         * reference node count (a re-walk would duplicate every file) */
        EXPECT_EQ(ref_tree.node_count(), tree.node_count());
        EXPECT_EQ(ref_stats.files_found + ref_stats.deleted_found + ref_stats.carved_found,
                  stats.files_found + stats.deleted_found + stats.carved_found);
        /* quick pass did not re-run: resumed files count comes only from
         * what the session carried + carve */
        EXPECT_EQ(ref_stats.carved_found, stats.carved_found);
    }
    std::remove(path.c_str());
}

TEST(PauseResume, MaskIntegrityViolationRefusesResume) {
    /* Build a valid session, then re-serialize a TAMPERED SessionState
     * (carve done bit set while carve was never configured) - resume must
     * fail with ERR_SESSION_FORMAT, never a silent partial. */
    const std::string path = tmp_session_path();
    std::vector<u8> tampered_state;
    {
        MemDevice dev(build_fat16());
        tree::ResultTree tree;
        ScanConfig cfg;
        cfg.run_carve = false; /* quick+deep only */
        ScanController ctrl(tree, cfg);
        ScanStats stats;
        ASSERT_EQ(OK, ctrl.run(&dev, nullptr, nullptr, nullptr, stats));
        FILE* f = fopen(path.c_str(), "wb");
        ASSERT_NE(nullptr, f);
        session::Writer w(file_write, file_tell, file_seek, f);
        ASSERT_EQ(OK, w.open(1));
        ASSERT_EQ(OK, ctrl.save_session(w, stats));
        ASSERT_EQ(OK, w.finalize());
        fclose(f);
        /* tamper: claim carve (bit 2) completed although it was never
         * configured (flags = quick|deep only) */
        session::SessionState st;
        st.stage_done_mask = STAGE_QUICK | STAGE_DEEP | STAGE_CARVE;
        st.files_found = stats.files_found;
        ASSERT_EQ(OK, session::serialize_session_state(st, tampered_state));
    }
    {
        /* rewrite the container with the tampered state record first */
        FILE* in = fopen(path.c_str(), "rb");
        ASSERT_NE(nullptr, in);
        session::Reader r(file_read, file_seek, file_size, in);
        u32 count = 0;
        ASSERT_EQ(OK, r.open(&count));
        std::vector<session::Record> recs;
        for (u32 i = 0; i < count; ++i) {
            session::Record rec;
            ASSERT_EQ(OK, r.read_record(i, rec));
            recs.push_back(std::move(rec));
        }
        fclose(in);
        ASSERT_FALSE(recs.empty());
        ASSERT_EQ(session::REC_SESSION_STATE, recs[0].type);
        recs[0].data = tampered_state; /* the lie */

        FILE* out = fopen(path.c_str(), "wb");
        ASSERT_NE(nullptr, out);
        session::Writer w(file_write, file_tell, file_seek, out);
        ASSERT_EQ(OK, w.open(1));
        for (const auto& rec : recs)
            ASSERT_EQ(OK, w.add_record(rec.type, rec.data.data(), u32(rec.data.size())));
        ASSERT_EQ(OK, w.finalize());
        fclose(out);
    }
    {
        tree::ResultTree tree;
        ScanStats stats;
        session::VolumeSessionInfo info;
        FILE* f = fopen(path.c_str(), "rb");
        ASSERT_NE(nullptr, f);
        session::Reader r(file_read, file_seek, file_size, f);
        std::unique_ptr<ScanController> ctrl;
        EXPECT_EQ(ERR_SESSION_FORMAT,
                  ScanController::resume(r, tree, ScanConfig{}, stats, info, ctrl));
        EXPECT_EQ(nullptr, ctrl.get());
        fclose(f);
    }
    std::remove(path.c_str());
}

TEST(PauseResume, SessionRestoresVolumeInfoAndCandidates) {
    /* G-14/G-23: a saved session restores slice placement, label, elapsed,
     * and (with a synthetic partition context) lost candidates. */
    const std::string path = tmp_session_path();
    {
        /* scan a PARTITION slice so slice_offset is non-zero (G-14): the
         * FAT fixture is placed at LBA 63 of the whole-disk image */
        std::vector<u8> whole_bytes(u64(10148) * 512, 0);
        {
            std::vector<u8> fat = build_fat16();
            std::memcpy(whole_bytes.data() + 512u * 63, fat.data(), fat.size());
        }
        MemDevice whole(std::move(whole_bytes));
        auto slice = PartitionSlice::create(&whole, 512u * 63, 5074u * 512);
        ASSERT_NE(nullptr, slice);
        tree::ResultTree tree;
        ScanConfig cfg;
        std::snprintf(cfg.volume_label, sizeof(cfg.volume_label), "TESTVOL");
        ScanController ctrl(tree, cfg);
        ScanStats stats;
        /* inject one synthetic lost candidate so a PARTITION_CONTEXT record
         * is written (G-23 round-trip) */
        lost::Candidate c;
        c.start_lba = 900;
        c.sector_count = 100;
        c.fs = lost::CandidateFs::Fat;
        c.score = 42;
        stats.lost_candidates.push_back(c);
        ASSERT_EQ(OK, ctrl.run(slice.get(), nullptr, nullptr, nullptr, stats));
        FILE* f = fopen(path.c_str(), "wb");
        ASSERT_NE(nullptr, f);
        session::Writer w(file_write, file_tell, file_seek, f);
        ASSERT_EQ(OK, w.open(7));
        ASSERT_EQ(OK, ctrl.save_session(w, stats));
        ASSERT_EQ(OK, w.finalize());
        fclose(f);
    }
    {
        tree::ResultTree tree;
        ScanStats stats;
        session::VolumeSessionInfo info;
        FILE* f = fopen(path.c_str(), "rb");
        ASSERT_NE(nullptr, f);
        session::Reader r(file_read, file_seek, file_size, f);
        std::unique_ptr<ScanController> ctrl;
        ASSERT_EQ(OK, ScanController::resume(r, tree, ScanConfig{}, stats, info, ctrl));
        fclose(f);
        EXPECT_EQ(512u * 63, info.slice_offset); /* G-14 */
        EXPECT_EQ(5074u * 512, info.slice_bytes);
        EXPECT_EQ(512u, info.logical_sector);
        EXPECT_EQ(0, std::strcmp(info.label, "TESTVOL"));
        EXPECT_EQ(u32(FsFamily::Fat), info.fs_family);
        ASSERT_EQ(1u, stats.lost_candidates.size()); /* G-23 */
        EXPECT_EQ(u64(900), stats.lost_candidates[0].start_lba);
        EXPECT_EQ(42, stats.lost_candidates[0].score);
    }
    std::remove(path.c_str());
}
