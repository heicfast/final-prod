/* test_pause_resume.cpp - G-07 acceptance: PausableToken semantics,
 * pause-time compensation, truthful stage_done_mask, save-on-cancel, and
 * resume-skips-completed-stages. Also exercises telemetry concurrency
 * (G-04/G-05/G-06) on the same scan threads (the TSan build runs this file
 * under -fsanitize=thread in CI).
 *
 * Licensed under the MIT license. See LICENSE.
 */
#include <gtest/gtest.h>
#include "test_tmpdir.hpp"
#include "cr/orchestrator/cr_scan.h"
#include "cr/carve/cr_usersigns.h"
#include "cr/recovery/cr_safewriter.h"
#include "cr/session/cr_crsf.h"
#include "cr/storage/cr_device.h"
#include "cr/storage/cr_slice.h"
#include <algorithm>
#include <atomic>
#include <functional>
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
    /* M-015: canonical media words (real-volume shape; the whole-table
     * validator requires u32@FAT[0] == 0xFFFFFFF8 on FAT16) */
    put16(img, FAT1 + 0 * 2, 0xFFF8);
    put16(img, FAT1 + 1 * 2, 0xFFFF);
    put16(img, FAT2 + 0 * 2, 0xFFF8);
    put16(img, FAT2 + 1 * 2, 0xFFFF);
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
    return fseek(static_cast<FILE*>(user), long(absolute), SEEK_SET) == 0 ? OK : ERR_SESSION_IO;
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
    /* GAP-CI-1: MSVC fopen does not resolve "/tmp" - resolve the real
     * temp dir (TMPDIR/TMP/TEMP, /tmp on the Linux runners) */
    return cr_test::tmp_path("cr_test_session", ".crsf");
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
    std::thread scan([&] { (void)ctrl.run(gate, &tok, nullptr, nullptr, stats); });
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

TEST(CarveStats, UserSignCarvedFoundSessionRoundTrip) {
    /* N-171 session leg: the runtime carved_found (now counting the
     * user-sign channel) must persist into the session record and
     * re-read identically on resume - the undercount used to persist
     * too (resume/preview re-read the wrong headline). */
    const std::string path = tmp_session_path();
    ScanConfig cfg; /* defaults: quick+deep+carve, whole-disk off */
    {
        std::vector<u8> img(2ull << 20, 0);
        img[0x2000] = 0xDE;
        img[0x2001] = 0xAD;
        img[0x2002] = 0xBE;
        img[0x2003] = 0xEF;
        const char* json =
            "{\"uvw\": {\"signs\": [\"0,DEADBEEF\"], \"mime\": \"application/x-uvw\"}}";
        carve::usersigns::UserSignSet set;
        ASSERT_EQ(OK, carve::usersigns::parse(json, std::strlen(json), set));
        ASSERT_EQ(OK, carve::usersigns::install_set(std::move(set)));

        MemDevice dev(std::move(img));
        tree::ResultTree tree;
        ScanController ctrl(tree, cfg);
        ScanStats stats;
        const u32 rc = ctrl.run(&dev, nullptr, nullptr, nullptr, stats);
        ASSERT_EQ(rc, OK) << err_text(rc);
        EXPECT_EQ(0u, stats.files_found) << "nothing structured lives on the unformatted device";
        EXPECT_EQ(1u, stats.carved_found)
            << "the user-sign record counts in the headline carved count";
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
        tree::ResultTree tree;
        ScanStats stats;
        session::VolumeSessionInfo info;
        FILE* f = fopen(path.c_str(), "rb");
        ASSERT_NE(nullptr, f);
        session::Reader r(file_read, file_seek, file_size, f);
        std::unique_ptr<ScanController> ctrl;
        ASSERT_EQ(OK, ScanController::resume(r, tree, cfg, stats, info, ctrl));
        fclose(f);
        EXPECT_EQ(1u, stats.carved_found)
            << "the persisted carved_found re-reads the user-sign record";
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

TEST(PauseResume, ResumedSessionPreservesOriginalConfiguredMask) {
    /* BENCH-F2 regression (bench run 40073093997, resume probe): a session
     * with quick+deep complete is resumed by a FULL config (carve newly
     * allowed); after the resumed run completes, the re-saved session must
     * carry VolumeInfo.flags = ORIGINAL configured (quick|deep) UNION the
     * resume-added stage (carve) - and must RELOAD cleanly. The bug:
     * session_configured_mask_ was never assigned on the resume path, so
     * flags collapsed to the remaining stages (carve only) while done
     * stamped all three -> the session failed its own mask-integrity check
     * on the next load (tree --in / --resume both refused it). */
    const std::string path1 = tmp_session_path();
    const std::string path2 = tmp_session_path() + ".2";
    {
        /* Phase 1: quick+deep only (the '--deep --no-carve' shape) */
        MemDevice dev(build_fat16());
        tree::ResultTree tree;
        ScanConfig cfg;
        cfg.run_carve = false;
        ScanController ctrl(tree, cfg);
        ScanStats stats;
        ASSERT_EQ(OK, ctrl.run(&dev, nullptr, nullptr, nullptr, stats));
        tree.refresh_stats();
        FILE* f = fopen(path1.c_str(), "wb");
        ASSERT_NE(nullptr, f);
        session::Writer w(file_write, file_tell, file_seek, f);
        ASSERT_EQ(OK, w.open(0x52434F46));
        ASSERT_EQ(OK, ctrl.save_session(w, stats));
        ASSERT_EQ(OK, w.finalize());
        fclose(f);
    }
    {
        /* Phase 2: resume with the FULL config (carve newly configured).
         * resume() must skip quick+deep and run carve only. */
        MemDevice dev(build_fat16());
        tree::ResultTree tree;
        ScanStats stats;
        session::VolumeSessionInfo info;
        FILE* f = fopen(path1.c_str(), "rb");
        ASSERT_NE(nullptr, f);
        session::Reader r(file_read, file_seek, file_size, f);
        std::unique_ptr<ScanController> ctrl;
        ASSERT_EQ(OK, ScanController::resume(r, tree, ScanConfig{}, stats, info, ctrl));
        fclose(f);
        ASSERT_NE(nullptr, ctrl.get());
        EXPECT_FALSE(ctrl->config().run_quick);
        EXPECT_FALSE(ctrl->config().run_deep);
        EXPECT_TRUE(ctrl->config().run_carve);
        ASSERT_EQ(OK, ctrl->run(&dev, nullptr, nullptr, nullptr, stats));
        tree.refresh_stats();
        FILE* f2 = fopen(path2.c_str(), "wb");
        ASSERT_NE(nullptr, f2);
        session::Writer w2(file_write, file_tell, file_seek, f2);
        ASSERT_EQ(OK, w2.open(0x52434F47));
        ASSERT_EQ(OK, ctrl->save_session(w2, stats));
        ASSERT_EQ(OK, w2.finalize());
        fclose(f2);
    }
    {
        /* Phase 3: the re-saved session must reload cleanly with the union
         * mask - this is exactly the bench probe's `tree --in` step that
         * failed with 0x50040001 before the fix. */
        tree::ResultTree tree;
        ScanStats stats;
        session::VolumeSessionInfo info;
        FILE* f = fopen(path2.c_str(), "rb");
        ASSERT_NE(nullptr, f);
        session::Reader r(file_read, file_seek, file_size, f);
        std::unique_ptr<ScanController> ctrl;
        EXPECT_EQ(OK, ScanController::resume(r, tree, ScanConfig{}, stats, info, ctrl));
        fclose(f);
        EXPECT_EQ(STAGE_QUICK | STAGE_DEEP | STAGE_CARVE, info.stage_done_mask);
        EXPECT_EQ(STAGE_QUICK | STAGE_DEEP | STAGE_CARVE, info.configured_mask);
    }
    std::remove(path1.c_str());
    std::remove(path2.c_str());
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

/* ---------------- G-13: auto-checkpoint ---------------- */

namespace {

/* vector-backed CRSF IO (the checkpoint sink target) */
struct VecIo {
    std::vector<u8> data;
    u64 pos = 0;
    static u32 wr(void* user, const void* bytes, u32 len) {
        auto* v = static_cast<VecIo*>(user);
        if (v->pos + len > v->data.size())
            v->data.resize(size_t(v->pos + len));
        std::memcpy(v->data.data() + v->pos, bytes, len);
        v->pos += len;
        return 0;
    }
    static u64 tl(void* user) { return static_cast<VecIo*>(user)->pos; }
    static u32 sk(void* user, u64 abs) {
        static_cast<VecIo*>(user)->pos = abs;
        return 0;
    }
    static u32 rd(void* user, void* buf, u32 len) {
        auto* v = static_cast<VecIo*>(user);
        if (v->pos + len > v->data.size())
            return 1;
        std::memcpy(buf, v->data.data() + v->pos, len);
        v->pos += len;
        return 0;
    }
    static u64 sz(void* user) { return static_cast<VecIo*>(user)->data.size(); }
};

} // namespace

TEST(Checkpoint, StageBoundaryCheckpointResumesAfterCrash) {
    /* crash simulation: the sink captures the post-quick checkpoint, then
     * the progress callback cancels at the quick->deep boundary (the
     * "process died after the first checkpoint" case). The captured
     * bytes must be a valid resumable session: resume skips quick and
     * completes deep+carve with reference-equal results. */
    MemDevice dev(build_fat16());
    tree::ResultTree tree;
    ScanConfig cfg;
    VecIo sink;
    cfg.checkpoint.write = VecIo::wr;
    cfg.checkpoint.tell = VecIo::tl;
    cfg.checkpoint.seek = VecIo::sk;
    cfg.checkpoint.user = &sink;
    ScanController ctrl(tree, cfg);
    ScanStats stats;
    PausableToken tok;
    /* crash point: after the quick stage STAMPS complete (the post-quick
     * checkpoint is emitted), cancel before deep - the same boundary
     * discrimination the SaveOnCancel test uses (the walk's final tick
     * also reports 750 while quick.completed is still false) */
    struct CrashCtx {
        PausableToken* tok;
        ScanController* ctrl;
    } crash{&tok, &ctrl};
    auto cancel_after_quick = [](u32 overall, void* user) {
        CrashCtx* c = static_cast<CrashCtx*>(user);
        if (overall >= 750 && c->ctrl->telemetry().quick.completed) {
            c->tok->cancel(); /* "process dies" after checkpoint #1 */
            return false;
        }
        return true;
    };
    ASSERT_EQ(ERR_CANCELLED, ctrl.run(&dev, &tok, cancel_after_quick, &crash, stats));
    EXPECT_EQ(u64(1), ctrl.checkpoint_count());
    ASSERT_FALSE(sink.data.empty());

    /* the captured checkpoint resumes: quick results present, quick not
     * re-run, deep+carve complete */
    {
        VecIo ckpt;
        ckpt.data = sink.data; /* copy: read-only use */
        ckpt.pos = 0;
        session::Reader r(VecIo::rd, VecIo::sk, VecIo::sz, &ckpt);
        tree::ResultTree tree2;
        ScanStats stats2;
        session::VolumeSessionInfo info;
        std::unique_ptr<ScanController> resumed;
        ASSERT_EQ(OK, ScanController::resume(r, tree2, cfg, stats2, info, resumed));
        EXPECT_EQ(STAGE_QUICK, info.stage_done_mask);
        EXPECT_FALSE(resumed->config().run_quick);
        MemDevice dev2(build_fat16());
        ASSERT_EQ(OK, resumed->run(&dev2, nullptr, nullptr, nullptr, stats2));

        /* reference: fresh full scan */
        tree::ResultTree ref_tree;
        ScanController ref(ref_tree, ScanConfig{});
        ScanStats ref_stats;
        MemDevice ref_dev(build_fat16());
        ASSERT_EQ(OK, ref.run(&ref_dev, nullptr, nullptr, nullptr, ref_stats));
        EXPECT_EQ(ref_tree.node_count(), tree2.node_count());
        EXPECT_EQ(ref_stats.files_found + ref_stats.deleted_found + ref_stats.carved_found,
                  stats2.files_found + stats2.deleted_found + stats2.carved_found);
    }
}

TEST(Checkpoint, CorruptMagicRefused) {
    VecIo ckpt;
    ckpt.data = {0xDE, 0xAD, 0xBE, 0xEF, 0, 0, 0, 0};
    tree::ResultTree tree;
    ScanStats stats;
    session::VolumeSessionInfo info;
    std::unique_ptr<ScanController> out;
    session::Reader r(VecIo::rd, VecIo::sk, VecIo::sz, &ckpt);
    EXPECT_EQ(ERR_SESSION_FORMAT, ScanController::resume(r, tree, ScanConfig{}, stats, info, out));
    EXPECT_EQ(nullptr, out.get());
}

/* =========================================================================
 * F-1: extent-basis normalization (absolute device offsets in the tree)
 * ========================================================================= */

namespace {

/* collect every props->extents offset in the tree (sorted) */
std::vector<u64> all_extent_offsets(const tree::ResultTree& t) {
    std::vector<u64> out;
    std::function<void(const tree::FileNode*)> walk = [&](const tree::FileNode* n) {
        if (n->props())
            for (const auto& e : n->props()->extents)
                out.push_back(e.offset);
        for (auto* c = n->first_child(); c; c = c->next_sibling())
            walk(c);
    };
    for (auto* r : const_cast<tree::ResultTree&>(t).roots())
        walk(r);
    std::sort(out.begin(), out.end());
    return out;
}

bool tree_has_shifted_flags(const tree::ResultTree& t) {
    std::function<bool(const tree::FileNode*)> walk = [&](const tree::FileNode* n) {
        if (n->props() && !n->props()->extents.empty() &&
            !(n->props()->flags & tree::NODE_FLAG_EXT_SHIFTED))
            return false;
        for (auto* c = n->first_child(); c; c = c->next_sibling())
            if (!walk(c))
                return false;
        return true;
    };
    for (auto* r : const_cast<tree::ResultTree&>(t).roots())
        if (!walk(r))
            return false;
    return true;
}

} // namespace

TEST(ExtentBasis, TreeRebaseShiftsOnceAndIsIdempotent) {
    tree::ResultTree t;
    auto* root = t.add_root(u"R");
    auto* p1 = new tree::FileProperties();
    p1->name = u"a.txt";
    p1->logical_size = 100;
    p1->extents.push_back({1000, 100});
    auto* n1 = t.add_file(root, p1);
    ASSERT_NE(nullptr, n1);
    /* hard link: the SAME props attached under a second parent */
    auto* root2 = t.add_root(u"R2");
    auto* n2 = t.add_file(root2, p1);
    ASSERT_NE(nullptr, n2);

    t.rebase_subtree_extents(root, 4096);
    EXPECT_EQ(1000u + 4096u, p1->extents.front().offset);
    EXPECT_TRUE(p1->flags & tree::NODE_FLAG_EXT_SHIFTED);
    /* second pass (different base, other subtree): the flag must hold -
     * a rebase may never shift twice (resume/normalization safety) */
    t.rebase_subtree_extents(root2, 999999);
    EXPECT_EQ(1000u + 4096u, p1->extents.front().offset)
        << "hard-link shared props must not shift twice";
    /* base == 0 must be a no-op */
    t.rebase_subtree_extents(root, 0);
    EXPECT_EQ(1000u + 4096u, p1->extents.front().offset);
    /* refcount contract: N attaches (hard links) consume the CALLER's single
     * ref - release exactly once (two releases would over-free -> UAF) */
    p1->release();
}

TEST(ExtentBasis, SliceScanSavesAbsoluteExtents) {
    /* THE F-1 contract: a scan through a PARTITION slice stores ABSOLUTE
     * device offsets in the tree/session, so `recover --disk N` reads the
     * right bytes. Fixture: FAT16 volume placed at LBA 63 of a disk image. */
    const u64 kBase = 512u * 63;
    const std::string path = tmp_session_path();
    {
        std::vector<u8> whole_bytes(u64(10148) * 512, 0);
        {
            std::vector<u8> fat = build_fat16();
            std::memcpy(whole_bytes.data() + kBase, fat.data(), fat.size());
        }
        MemDevice whole(std::move(whole_bytes));
        auto slice = PartitionSlice::create(&whole, kBase, 5074u * 512);
        ASSERT_NE(nullptr, slice);
        tree::ResultTree tree;
        ScanController ctrl(tree, ScanConfig{});
        ScanStats stats;
        ASSERT_EQ(OK, ctrl.run(slice.get(), nullptr, nullptr, nullptr, stats));
        ASSERT_GT(stats.files_found + stats.deleted_found, 0u);

        /* in-memory tree: every extent must now be absolute (>= the slice
         * base) and flagged */
        for (u64 off : all_extent_offsets(tree))
            EXPECT_GE(off, kBase) << "extent still slice-relative";
        EXPECT_TRUE(tree_has_shifted_flags(tree));

        FILE* f = fopen(path.c_str(), "wb");
        ASSERT_NE(nullptr, f);
        session::Writer w(file_write, file_tell, file_seek, f);
        w.set_flags(session::CRSF_FLAG_ABS_EXTENTS);
        ASSERT_EQ(OK, w.open(9));
        ASSERT_EQ(OK, ctrl.save_session(w, stats));
        ASSERT_EQ(OK, w.finalize());
        fclose(f);
    }
    {
        /* reload: the flag says absolute - no re-shift (extents stay >= base) */
        tree::ResultTree tree;
        ScanStats stats;
        session::VolumeSessionInfo info;
        FILE* f = fopen(path.c_str(), "rb");
        ASSERT_NE(nullptr, f);
        session::Reader r(file_read, file_seek, file_size, f);
        std::unique_ptr<ScanController> ctrl;
        ASSERT_EQ(OK, ScanController::resume(r, tree, ScanConfig{}, stats, info, ctrl));
        fclose(f);
        EXPECT_EQ(kBase, info.slice_offset);
        for (u64 off : all_extent_offsets(tree))
            EXPECT_GE(off, kBase) << "reload double-shifted extents";
    }
}

TEST(ExtentBasis, LegacySliceRelativeSessionNormalizedOnLoad) {
    /* A pre-F-1 session (container flag ABSENT, props flags absent) carries
     * slice-relative extents; load_session_summary must normalize them with
     * the recorded VolumeInfo.slice_offset exactly once. */
    const u64 kBase = 512u * 63;
    const std::string path = tmp_session_path();
    {
        /* hand-build the records: state + volume-info(slice_offset=kBase) +
         * tree(relative extents, no SHIFTED flag) */
        session::SessionState st;
        st.stage_done_mask = STAGE_QUICK;
        st.files_found = 1;
        std::vector<u8> stb;
        ASSERT_EQ(OK, session::serialize_session_state(st, stb));
        session::VolumeInfo vi;
        vi.fs_kind = u32(FsFamily::Fat);
        vi.flags = STAGE_QUICK | STAGE_DEEP | STAGE_CARVE;
        vi.slice_offset = kBase;
        vi.slice_bytes = 5074u * 512;
        std::vector<u8> vib;
        ASSERT_EQ(OK, session::serialize_volume_info(vi, vib));

        tree::ResultTree tree;
        auto* root = tree.add_root(u"Deleted");
        auto* p = new tree::FileProperties();
        p->name = u"legacy.txt";
        p->logical_size = 10;
        p->extents.push_back({777, 10}); /* slice-relative, flag clear */
        ASSERT_NE(nullptr, tree.add_file(root, p));
        p->release();
        std::vector<u8> tb;
        ASSERT_EQ(OK, session::serialize_tree(tree, tb));

        FILE* f = fopen(path.c_str(), "wb");
        ASSERT_NE(nullptr, f);
        session::Writer w(file_write, file_tell, file_seek, f);
        ASSERT_EQ(OK, w.open(11)); /* NO CRSF_FLAG_ABS_EXTENTS - legacy file */
        ASSERT_EQ(OK, w.add_record(session::REC_SESSION_STATE, stb.data(), u32(stb.size())));
        ASSERT_EQ(OK, w.add_record(session::REC_VOLUME_INFO, vib.data(), u32(vib.size())));
        ASSERT_EQ(OK, w.add_record(session::REC_RESULT_TREE, tb.data(), u32(tb.size())));
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
        /* normalized: relative 777 + base -> absolute, and flagged so a
         * re-save/re-load cycle never shifts again */
        const auto offs = all_extent_offsets(tree);
        ASSERT_EQ(1u, offs.size());
        EXPECT_EQ(777u + kBase, offs[0]);
        EXPECT_TRUE(tree_has_shifted_flags(tree));
    }
}

TEST(PauseResume, SaveAfterResumeReloadsCleanly) {
    /* G-07 regression (bench run 40073093997 resume probe): resume a
     * partially-complete session, run the remaining stages, SAVE - and the
     * saved session must reload without the mask-integrity violation.
     * The bug: save_session ORs only the RESIDUAL cfg stages into
     * VolumeInfo.flags while stage_done_mask carries the completed stages
     * from the ORIGINAL run; done-not-subset-of-configured refused the load.
     * This exact save-after-resume flow had no coverage before. */
    const std::string path = tmp_session_path();
    ScanConfig cfg; /* quick+deep+carve all on */
    tree::ResultTree tree;
    ScanStats stats;
    {
        /* phase 1: fresh scan, cancel at the quick/deep boundary (quick+
         * deep stamped done, carve not) - mirrors the bench cancel probe */
        struct Ctx {
            PausableToken* tok;
            ScanController* ctrl;
        };
        MemDevice dev(build_fat16());
        ScanController ctrl(tree, cfg);
        PausableToken tok;
        Ctx c{&tok, &ctrl};
        auto cancel_at = [](u32 overall, void* user) {
            Ctx* ctx = static_cast<Ctx*>(user);
            if (overall >= 750 && ctx->ctrl->telemetry().quick.completed) {
                ctx->tok->cancel();
                return false;
            }
            return true;
        };
        ASSERT_EQ(ERR_CANCELLED, ctrl.run(&dev, &tok, cancel_at, &c, stats));
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
        /* phase 2: resume + run the remaining carve + SAVE */
        MemDevice dev(build_fat16());
        tree::ResultTree tree2;
        ScanStats stats2;
        session::VolumeSessionInfo info;
        FILE* f = fopen(path.c_str(), "rb");
        ASSERT_NE(nullptr, f);
        session::Reader r(file_read, file_seek, file_size, f);
        std::unique_ptr<ScanController> resumed;
        ASSERT_EQ(OK, ScanController::resume(r, tree2, cfg, stats2, info, resumed));
        fclose(f);
        ASSERT_EQ(OK, resumed->run(&dev, nullptr, nullptr, nullptr, stats2));
        tree2.refresh_stats();
        FILE* f2 = fopen(path.c_str(), "wb");
        ASSERT_NE(nullptr, f2);
        session::Writer w2(file_write, file_tell, file_seek, f2);
        w2.set_flags(session::CRSF_FLAG_ABS_EXTENTS);
        ASSERT_EQ(OK, w2.open(0x52434F56));
        ASSERT_EQ(OK, resumed->save_session(w2, stats2));
        ASSERT_EQ(OK, w2.finalize());
        fclose(f2);
    }
    {
        /* phase 3: the re-saved session MUST reload (the regression) and
         * report a full completion mask under the FULL configured set */
        tree::ResultTree tree3;
        ScanStats stats3;
        session::VolumeSessionInfo info3;
        FILE* f = fopen(path.c_str(), "rb");
        ASSERT_NE(nullptr, f);
        session::Reader r(file_read, file_seek, file_size, f);
        std::unique_ptr<ScanController> ctrl3;
        const u32 rc = ScanController::resume(r, tree3, cfg, stats3, info3, ctrl3);
        fclose(f);
        ASSERT_EQ(OK, rc) << "save-after-resume session must reload without "
                             "the mask-integrity violation";
        EXPECT_EQ(STAGE_QUICK | STAGE_DEEP | STAGE_CARVE, info3.stage_done_mask);
        EXPECT_EQ(STAGE_QUICK | STAGE_DEEP | STAGE_CARVE, info3.configured_mask);
    }
}

TEST(Checkpoint, CommitHookFiresAfterEveryFinalizeAndAbortsOnError) {
    /* N-014: the orchestrator calls the sink's commit hook after every
     * successful finalize() (the atomic-rename seam), and a FAILED
     * commit aborts the scan with its error code - the same contract a
     * failed write carries (never a silent checkpoint gap). */
    MemDevice dev(build_fat16());
    tree::ResultTree tree;
    ScanConfig cfg;
    struct Sink : VecIo {
        unsigned commits = 0;
        bool fail_next = false;
        static u32 commit(void* user) {
            auto* s = static_cast<Sink*>(user);
            if (s->fail_next) {
                s->fail_next = false;
                return ERR_SESSION_IO;
            }
            ++s->commits;
            return OK;
        }
    };
    Sink sink;
    cfg.checkpoint.write = VecIo::wr;
    cfg.checkpoint.tell = VecIo::tl;
    cfg.checkpoint.seek = VecIo::sk;
    cfg.checkpoint.commit = Sink::commit;
    cfg.checkpoint.user = &sink;
    {
        ScanController ctrl(tree, cfg);
        ScanStats stats;
        ASSERT_EQ(OK, ctrl.run(&dev, nullptr, nullptr, nullptr, stats));
        EXPECT_GT(ctrl.checkpoint_count(), 0u);
        EXPECT_EQ(sink.commits, unsigned(ctrl.checkpoint_count()))
            << "commit fires exactly once per checkpoint";
    }
    {
        /* a failing commit aborts the run with the commit's own code */
        tree::ResultTree tree2;
        ScanConfig cfg2 = cfg;
        Sink sink2;
        sink2.fail_next = true;
        cfg2.checkpoint.user = &sink2;
        ScanController ctrl2(tree2, cfg2);
        ScanStats stats2;
        EXPECT_EQ(ERR_SESSION_IO, ctrl2.run(&dev, nullptr, nullptr, nullptr, stats2));
        EXPECT_EQ(0u, ctrl2.checkpoint_count())
            << "the failed checkpoint does not count as emitted";
    }
}
