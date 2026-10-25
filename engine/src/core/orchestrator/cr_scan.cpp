/* cr_scan.cpp - the staged scan orchestrator.
 *
 * Threading model (R6; the corpus shows host-side fan-out AND engine-side
 * workers - FAT QuickSearchWorker, FATSearchDLL:26931 - so the honest
 * framing is "extends the corpus's host-fans-out model"): one structured
 * engine pass (quick and deep share it - the old code walked the MFT twice
 * and double-emitted FAT), deep adds the remnant sweep + parallel
 * lost-partition candidate scans, carve fans window ranges out over the
 * pool. Engine progress callbacks are wired into the weighted overall
 * progress (fine-grained, monotonic).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/orchestrator/cr_scan.h"
#include "cr/btrfs/cr_btrfs_engine.h"
#include "cr/carve/cr_carve.h"
#include "cr/cr_parallel.h"
#include "cr/exfat/cr_exfat_engine.h"
#include "cr/ext4/cr_ext4_engine.h"
#include "cr/fat/cr_fat_engine.h"
#include "cr/hfsplus/cr_hfsplus_engine.h"
#include "cr/lost/cr_lostpart.h"
#include "cr/ntfs/cr_bootstrap.h"
#include "cr/ntfs/cr_bitmap.h"
#include "cr/ntfs/cr_mftwalk.h"
#include "cr/refs/cr_refs_engine.h"
#include "cr/session/cr_crsf.h"
#include "cr/storage/cr_cached.h"
#include "cr/storage/cr_retry.h"
#include "cr/storage/cr_slice.h"
#include "cr/table/cr_ldm.h"
#include "cr/table/cr_table_parse.h"
#include "cr/tree/cr_diskmap.h"
#include "cr/zfs/cr_zfs_engine.h"
#include <chrono>
#include <cstring>

namespace cr {
namespace orchestrator {

namespace {

u32 resolve_threads(u32 requested) {
    if (requested == 1)
        return 1;
    if (requested > 1)
        return requested > 64 ? 64 : requested;
    const unsigned hw = std::thread::hardware_concurrency();
    u32 n = hw > 1 ? hw - 1 : 1;
    if (n > 8)
        n = 8;
    return n;
}

} // namespace

u32 weighted_progress(const StageWeights& w, u32 quick_pm, u32 deep_pm, u32 carve_pm) {
    const u32 total = w.quick + w.deep + w.carve;
    if (total == 0)
        return 0;
    if (quick_pm > 1000)
        quick_pm = 1000;
    if (deep_pm > 1000)
        deep_pm = 1000;
    if (carve_pm > 1000)
        carve_pm = 1000;
    /* exact single-division form: sum(w_i * pm_i) / total - no double
     * truncation (500,500,500 with 40/35/25 must be exactly 500) */
    const u64 num = u64(w.quick) * quick_pm + u64(w.deep) * deep_pm + u64(w.carve) * carve_pm;
    const u32 permille = u32(num / total);
    return permille > 1000 ? 1000 : permille;
}

ScanController::ScanController(tree::ResultTree& tree, ScanConfig cfg)
    : tree_(tree), cfg_(std::move(cfg)) {
    event_fn_ = cfg_.event_fn; /* GAP-S6-10: config seed, setter overrides */
    event_user_ = cfg_.event_user;
    if (cfg_.run_quick || cfg_.run_deep) {
        root_found_ = tree_.add_root(u"Found");
        root_deleted_ = tree_.add_root(u"Deleted");
        root_lost_ = tree_.add_root(u"Lost Files");
    }
    if (cfg_.run_carve)
        root_carved_ = tree_.add_root(u"Other Deleted Files (Carved)");
}

ScanController::ScanController(tree::ResultTree& tree, ScanConfig cfg,
                               const session::VolumeSessionInfo& resumed)
    : tree_(tree), cfg_(std::move(cfg)) {
    event_fn_ = cfg_.event_fn; /* GAP-S6-10: config seed, setter overrides */
    event_user_ = cfg_.event_user;
    /* BENCH-F2 note: the ORIGINAL configured set is carried into
     * session_configured_mask_ by adopt_resume_state_ at the end of this
     * ctor (commit d20f374; independently also found from the same
     * 40073093997 artifacts as BENCH-F2 - duplicate resolved to the
     * adopt_resume_state_ placement, the single deeper fix point). */
    /* The saved session already carries the roots the ORIGINAL run created
     * (the ctor created them before any stage executed, so even a
     * quick-incomplete session has them). Recover the pointers by role
     * instead of re-adding (which would duplicate). */
    const bool pass_staged = (resumed.configured_mask & (STAGE_QUICK | STAGE_DEEP)) != 0;
    if ((cfg_.run_quick || cfg_.run_deep) && pass_staged) {
        for (tree::FileNode* r : tree_.roots()) {
            if (!r->is_full())
                continue;
            const std::u16string& role = static_cast<tree::FullNode*>(r)->role;
            if (role == u"Found")
                root_found_ = static_cast<tree::FullNode*>(r);
            else if (role == u"Deleted")
                root_deleted_ = static_cast<tree::FullNode*>(r);
            else if (role == u"Lost Files")
                root_lost_ = static_cast<tree::FullNode*>(r);
            else if (role == u"Other Deleted Files (Carved)")
                root_carved_ = static_cast<tree::FullNode*>(r);
        }
    } else if (cfg_.run_quick || cfg_.run_deep) {
        root_found_ = tree_.add_root(u"Found");
        root_deleted_ = tree_.add_root(u"Deleted");
        root_lost_ = tree_.add_root(u"Lost Files");
    }
    if (cfg_.run_carve && root_carved_ == nullptr)
        root_carved_ = tree_.add_root(u"Other Deleted Files (Carved)");
    adopt_resume_state_(resumed);
}

u32 ScanController::set_event_sink(event::EventFn fn, void* user) {
    if (tel_running_.load(std::memory_order_acquire))
        return ERR_BAD_PARAMETER; /* pre-run only: a mid-run swap could
                                     double-deliver or drop events */
    event_fn_ = fn;
    event_user_ = user;
    return OK;
}

void ScanController::adopt_resume_state_(const session::VolumeSessionInfo& resumed) {
    /* Restore ACTUAL completion flags so run() skips finished stages and
     * save_session() re-stamps the mask truthfully. Bit semantics (documented
     * in cr_records.h SessionState): 0 = quick engine pass, 1 = deep stage
     * complete (pass + remnant + lost), 2 = carve. */
    const bool quick_done = (resumed.stage_done_mask & STAGE_QUICK) != 0;
    const bool deep_done = (resumed.stage_done_mask & STAGE_DEEP) != 0;
    const bool carve_done = (resumed.stage_done_mask & STAGE_CARVE) != 0;
    quick_done_ = quick_done;
    /* bit0 set means the engine pass completed - with deep semantics when
     * deep was configured (the shared-walk design), or plain quick pass. */
    pass_deep_done_ = quick_done && (resumed.configured_mask & STAGE_DEEP) != 0;
    deep_extras_done_ = deep_done && pass_deep_done_;
    carve_done_ = carve_done;
    if (cfg_.fs_hint == FsFamily::Unknown && resumed.fs_family != 0)
        cfg_.fs_hint = FsFamily(u8(resumed.fs_family));
    /* SESS-2: adopt the loaded identity card so a re-save without any new
     * run (load -> immediate save, e.g. cancel-during-resume) preserves it.
     * A fresh run() overwrites vol_boot_hash_ with the recomputed value. */
    vol_boot_hash_ = resumed.boot_hash;
    if (cfg_.volume_model[0] == '\0')
        for (size_t i = 0; i < sizeof(cfg_.volume_model); ++i)
            cfg_.volume_model[i] = resumed.model[i];
    if (cfg_.volume_serial[0] == '\0')
        for (size_t i = 0; i < sizeof(cfg_.volume_serial); ++i)
            cfg_.volume_serial[i] = resumed.serial[i];
    /* G-07 (bench run 40073093997 resume probe): the ORIGINAL configured
     * stage set must survive a resume. cfg_ carries only the RESIDUAL stages
     * (completed ones were masked off in ScanController::resume), so without
     * this the re-save wrote vi.flags = residual-only while the truthful
     * done_mask carried the completed stages too - done-not-subset-of-
     * configured on reload = ERR_SESSION_FORMAT, and every save-after-resume
     * session was unloadable. The member existed for exactly this and was
     * never set. */
    session_configured_mask_ = resumed.configured_mask;
}

/* ---- telemetry (G-04/G-05/G-06) ---- */

namespace {
u64 ms_since(std::chrono::steady_clock::time_point t0) {
    return u64(
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0)
            .count());
}
} // namespace

void ScanController::tel_stage_begin_(u32 stage, const CancelToken* cancel) {
    {
        std::lock_guard<std::mutex> lock(tel_.m);
        tel_.stage_t0 = std::chrono::steady_clock::now();
        tel_.stage_pause_base = cancel ? cancel->paused_ms() : 0;
        tel_stage_.store(stage, std::memory_order_relaxed);
        StageTelemetry& st = stage == 0   ? tel_.snap.quick
                             : stage == 1 ? tel_.snap.deep
                                          : tel_.snap.carve;
        st = StageTelemetry{};
    }
    emit_event_(event::Type::ScanStage, stage); /* GAP-S6-10 (outside the lock) */
}

void ScanController::tel_stage_end_(u32 stage, const CancelToken* cancel) {
    std::lock_guard<std::mutex> lock(tel_.m);
    StageTelemetry& st = stage == 0   ? tel_.snap.quick
                         : stage == 1 ? tel_.snap.deep
                                      : tel_.snap.carve;
    st.permille = 1000;
    st.completed = true;
    const u64 paused = cancel ? cancel->paused_ms() : 0;
    const u64 span = ms_since(tel_.stage_t0);
    const u64 stage_paused = paused > tel_.stage_pause_base ? paused - tel_.stage_pause_base : 0;
    st.elapsed_ms = span > stage_paused ? span - stage_paused : 0;
    const u32 overall = weighted_progress(cfg_.weights, tel_.snap.quick.permille,
                                          tel_.snap.deep.permille, tel_.snap.carve.permille);
    if (overall > tel_.snap.overall_permille)
        tel_.snap.overall_permille = overall;
}

void ScanController::tel_publish_(u32 stage, u64 done, u64 total, bool bytes_units,
                                  const CancelToken* cancel) {
    /* Called from engine worker threads via the bridge and from the scan
     * thread. All mutations under the telemetry mutex; permilles are clamped
     * monotonic (G-04 acceptance). */
    std::lock_guard<std::mutex> lock(tel_.m);
    StageTelemetry& st = stage == 0   ? tel_.snap.quick
                         : stage == 1 ? tel_.snap.deep
                                      : tel_.snap.carve;
    u32 pm = 0;
    if (total != 0) {
        if (done >= total)
            pm = 1000;
        else
            pm = u32(done * 1000 / total);
        if (pm > 1000)
            pm = 1000;
        if (pm < st.permille)
            pm = st.permille; /* monotonic within the stage */
        st.permille = pm;
        st.units_done = done;
        st.units_total = total;
    }
    if (bytes_units)
        st.bytes_done = done;
    /* stage elapsed, pause-compensated (RE parity) */
    const u64 paused_now = cancel ? cancel->paused_ms() : 0;
    const u64 stage_paused =
        paused_now > tel_.stage_pause_base ? paused_now - tel_.stage_pause_base : 0;
    const u64 span = ms_since(tel_.stage_t0);
    st.elapsed_ms = span > stage_paused ? span - stage_paused : 0;
    const u32 overall = weighted_progress(cfg_.weights, tel_.snap.quick.permille,
                                          tel_.snap.deep.permille, tel_.snap.carve.permille);
    if (overall > tel_.snap.overall_permille)
        tel_.snap.overall_permille = overall;
}

void ScanController::tel_counts_(const ScanStats& stats, const CancelToken* cancel) {
    std::lock_guard<std::mutex> lock(tel_.m);
    tel_.snap.files_found = stats.files_found;
    tel_.snap.deleted_found = stats.deleted_found;
    tel_.snap.carved_found = stats.carved_found;
    tel_.snap.lost_partitions = stats.lost_partitions;
    tel_.snap.bytes_scanned = stats.bytes_scanned;
    tel_.snap.paused = cancel ? cancel->paused() : false;
    /* S-016: feed the LIVE retry-pipe error map into the snapshot at every
     * stage-boundary count refresh - this runs immediately BEFORE every
     * emit_boundary_events_ call site (stamp + the three stage ends), and
     * those emitters read tel_.snap.errors. Before this, the snapshot's
     * error map was only merged in stamp()'s finalize block AFTER the
     * boundary emits - so the ErrorRange event could never fire mid-run
     * (count was always 0 at every emit point) and the documented error
     * delta stream was dead. ReadErrorMap::add normalizes (absorbs
     * touching/overlapping runs), so re-merging the same pipe state is
     * idempotent; the count only grows when new failures are recorded. */
    if (RetryingDevice* rd = tel_retry_.load(std::memory_order_acquire))
        tel_.snap.errors.merge(rd->error_map_copy());
}

/* ---- typed event stream (GAP-S6-10; ABI-CONTRACT.md section 6) ----
 * Emissions never hold the telemetry mutex (callbacks must not block and
 * must never be called under a core lock). */

void ScanController::emit_event_(event::Type type, u32 stage) const {
    if (event_fn_ == nullptr)
        return;
    event::Event e;
    e.type = type;
    e.stage = stage;
    {
        std::lock_guard<std::mutex> lock(tel_.m);
        e.timestamp_ms = tel_.snap.elapsed_ms;
        e.overall_permille = tel_.snap.overall_permille;
        e.files_found = tel_.snap.files_found;
        e.deleted_found = tel_.snap.deleted_found;
        e.carved_found = tel_.snap.carved_found;
        e.error_ranges = u32(tel_.snap.errors.ranges().size());
        const StageTelemetry& st = stage == 0   ? tel_.snap.quick
                                   : stage == 1 ? tel_.snap.deep
                                                : tel_.snap.carve;
        e.stage_permille = st.permille;
        e.bytes_done = stage == 2 ? st.bytes_done : 0;
        e.bytes_total = stage == 2 ? st.units_total : 0;
    }
    event_fn_(e, event_user_);
}

void ScanController::emit_progress_event_(u32 stage) const {
    if (event_fn_ == nullptr)
        return;
    /* coalesce: fire only when the overall permille advanced (<= 1000
     * events per scan plus stage flips - the documented sample cadence) */
    u32 overall;
    {
        std::lock_guard<std::mutex> lock(tel_.m);
        overall = tel_.snap.overall_permille;
    }
    u32 last = evt_last_overall_.load(std::memory_order_relaxed);
    if (overall == last)
        return;
    if (last != 0xFFFFFFFFu && overall < last)
        return; /* stale out-of-order worker delivery */
    if (!evt_last_overall_.compare_exchange_strong(last, overall, std::memory_order_relaxed))
        return; /* another worker emitted this step first */
    emit_event_(event::Type::ScanProgress, stage);
}

void ScanController::emit_boundary_events_(u32 stage) const {
    if (event_fn_ == nullptr)
        return;
    /* FilesFound: cumulative counters at the boundary where they advance */
    emit_event_(event::Type::FilesFound, stage);
    /* ErrorRange deltas: the newest appended range + the cumulative count */
    u64 newest_start = 0, newest_len = 0;
    u32 count = 0;
    {
        std::lock_guard<std::mutex> lock(tel_.m);
        const auto& ranges = tel_.snap.errors.ranges();
        count = u32(ranges.size());
        if (count > 0) {
            newest_start = ranges[size_t(count) - 1].offset;
            newest_len = ranges[size_t(count) - 1].length;
        }
    }
    const u32 last = evt_last_errors_.load(std::memory_order_relaxed);
    if (count > last) {
        u32 expected = last;
        if (evt_last_errors_.compare_exchange_strong(expected, count, std::memory_order_relaxed)) {
            event::Event e;
            e.type = event::Type::ErrorRange;
            e.stage = stage;
            {
                std::lock_guard<std::mutex> lock(tel_.m);
                e.timestamp_ms = tel_.snap.elapsed_ms;
                e.overall_permille = tel_.snap.overall_permille;
                e.error_ranges = u32(tel_.snap.errors.ranges().size());
            }
            e.err_start_bytes = newest_start;
            e.err_len_bytes = newest_len;
            /* A-004: deliver through the LIVE sink (set_event_sink's
             * event_fn_/event_user_), like every other event type - the
             * config copy is only the ctor seed and is never updated by
             * set_event_sink. The previous cfg_.event_fn delivery was a
             * null-call crash on the documented post-construction
             * subscription pattern (ScanConfig{} + set_event_sink) and a
             * silent mis-delivery when both sinks differ. */
            event_fn_(e, event_user_);
        }
    }
}

void ScanController::emit_lost_candidate_(const lost::Candidate& cand) const {
    if (event_fn_ == nullptr)
        return;
    event::Event e;
    e.type = event::Type::LostCandidate;
    e.stage = 1; /* the deep stage's discovery pass */
    e.candidate_lba = cand.start_lba;
    {
        std::lock_guard<std::mutex> lock(tel_.m);
        e.timestamp_ms = tel_.snap.elapsed_ms;
        e.overall_permille = tel_.snap.overall_permille;
        e.error_ranges = u32(tel_.snap.errors.ranges().size());
    }
    event_fn_(e, event_user_);
}

bool ScanController::engine_progress_bridge_(u64 done, u64 total, void* user) {
    StageBridge* b = static_cast<StageBridge*>(user);
    b->self->tel_publish_(b->stage, done, total, b->stage == 2, b->cancel);
    if (b->covers_both)
        b->self->tel_publish_(1, done, total, false, b->cancel);
    b->self->emit_progress_event_(b->stage); /* GAP-S6-10 */
    if (b->overall) {
        u32 overall;
        {
            std::lock_guard<std::mutex> lock(b->self->tel_.m);
            overall = b->self->tel_.snap.overall_permille;
        }
        return b->overall(overall, b->user);
    }
    return true;
}

ScanTelemetrySnapshot ScanController::telemetry() const {
    /* All live reads happen under the same mutex the stamp path uses for
     * teardown, so a query racing run()'s exit never touches a dead retry
     * pipe or a destroyed caller token. Live refresh (G-04/G-05/G-06):
     * paused + elapsed come straight from the token, the bad-sector map
     * from the retry pipe - the snapshot copy alone can lag (it is only
     * stamped at stage boundaries). */
    std::lock_guard<std::mutex> lock(tel_.m);
    ScanTelemetrySnapshot out = tel_.snap;
    out.running = tel_running_.load(std::memory_order_acquire);
    if (!out.running)
        return out; /* finalized snapshot: stamp() already merged the
                       final error map, elapsed, cancelled, paused=false */
    const CancelToken* tok = tel_cancel_.load(std::memory_order_acquire);
    if (tok) {
        out.paused = tok->paused();
        /* pause-compensated elapsed so far (RE parity) */
        const u64 paused = tok->paused_ms();
        const u64 span = ms_since(tel_.run_t0);
        out.elapsed_ms = span > paused ? span - paused : 0;
    }
    if (RetryingDevice* rd = tel_retry_.load(std::memory_order_acquire))
        out.errors.merge(rd->error_map_copy());
    return out;
}

u32 ScanController::tel_overall_() const {
    std::lock_guard<std::mutex> lock(tel_.m);
    return tel_.snap.overall_permille;
}

u32 ScanController::emit_checkpoint_() const {
    if (!cfg_.checkpoint)
        return OK;
    /* fresh container per checkpoint: rewind the sink first so every
     * emit overwrites from position 0 (the file always holds the newest
     * COMPLETE checkpoint; a torn one is no worse than none - the reader
     * re-indexes forward by construction). The sink triple IS the writer's
     * IO (a file-backed sink makes this <path>.tmp + flush + rename). The
     * record set matches save_session exactly - mask truth included. */
    if (cfg_.checkpoint.seek)
        if (u32 rc = cfg_.checkpoint.seek(cfg_.checkpoint.user, 0))
            return rc;
    session::Writer w(cfg_.checkpoint.write, cfg_.checkpoint.tell, cfg_.checkpoint.seek,
                      cfg_.checkpoint.user);
    w.set_flags(session::CRSF_FLAG_ABS_EXTENTS); /* F-1: stage rebased */
    if (u32 rc = w.open(0x52434B50))             /* 'RKPP' checkpoint container tag */
        return rc;
    /* snapshot the stats shape the session needs: counts live in the
     * caller's stats object (run owns it); emit with a zero-stats shape
     * is a LIE - instead defer to the caller: run() passes stats via a
     * member captured at run() start (checkpoint_stats_) */
    if (u32 rc = save_session(w, *checkpoint_stats_))
        return rc;
    if (u32 rc = w.finalize())
        return rc;
    ++checkpoint_count_;
    if (event_fn_ != nullptr) { /* GAP-S6-10 */
        event::Event e;
        e.type = event::Type::Checkpoint;
        e.stage = tel_stage_.load(std::memory_order_relaxed);
        e.checkpoint_index = checkpoint_count_;
        {
            std::lock_guard<std::mutex> lock(tel_.m);
            e.timestamp_ms = tel_.snap.elapsed_ms;
            e.overall_permille = tel_.snap.overall_permille;
            e.files_found = tel_.snap.files_found;
            e.deleted_found = tel_.snap.deleted_found;
            e.carved_found = tel_.snap.carved_found;
            e.error_ranges = u32(tel_.snap.errors.ranges().size());
        }
        event_fn_(e, event_user_);
    }
    return OK;
}

FsFamily ScanController::probe_family(IStorageDevice* slice) {
    if (slice == nullptr)
        return FsFamily::Unknown;
    u8 sec[512];
    if (slice->read_at(0, sec, 512) != OK)
        return FsFamily::Unknown;
    if (std::memcmp(sec + 3, "NTFS    ", 8) == 0)
        return FsFamily::Ntfs;
    if (std::memcmp(sec + 3, "EXFAT   ", 8) == 0)
        return FsFamily::Exfat;
    /* ReFS: OEM "ReFS" + the DBR bytes-per-sector field (u32 @+0x20 == 512,
     * decompile-verified gate from RefsProc::CRefsVolumeBootRecord). */
    if (std::memcmp(sec + 3, "ReFS", 4) == 0)
        return FsFamily::Refs;
    /* BitLocker: OEM "-FVE-FS-" (S-019; the og's type-0xB classifier gate,
     * PartitionRecoveryD_decompiled.c:2778-2785). Recognized explicitly so
     * encrypted content never falls through to the ext4-magic probe (a
     * random 0xEF53 in the ciphertext would mis-dispatch); the engine
     * dispatch's default path surfaces the candidate as a locked volume
     * with zero files - the og's pre-unlock behavior. */
    if (std::memcmp(sec + 3, "-FVE-FS-", 8) == 0)
        return FsFamily::BitLocker;
    /* HFS+/HFSX: volume-header signature at byte 1024 ("H+"/"HX") with a
     * sane version (4 or 5) - the version gate disambiguates from an ext4
     * superblock whose s_inodes_count happens to read as the signature. */
    {
        u8 h[4] = {};
        if (slice->read_at(1024, h, 4) == OK && h[0] == 0x48 && (h[1] == 0x2B || h[1] == 0x58)) {
            const u16 ver = (u16(h[2]) << 8) | h[3];
            if (ver == 4 || ver == 5)
                return FsFamily::Hfsplus;
        }
    }
    /* FAT: bytes-per-sector sane + 55AA */
    if (sec[510] == 0x55 && sec[511] == 0xAA) {
        const u16 bps = u16_from_bytes_le(sec + 0x0B);
        if (bps >= 512 && bps <= 4096 && (bps & (bps - 1)) == 0)
            return FsFamily::Fat;
    }
    /* ext2/3/4: superblock magic at byte offset 1024+0x38 */
    u8 sb2[2] = {};
    if (slice->read_at(1024 + 0x38, sb2, 2) == OK && sb2[0] == 0x53 && sb2[1] == 0xEF)
        return FsFamily::Ext4;
    /* Btrfs: primary superblock magic at 64 KiB + 0x40 */
    {
        u8 bt[8] = {};
        if (slice->read_at(0x10040, bt, 8) == OK && std::memcmp(bt, "_BHRfS_M", 8) == 0)
            return FsFamily::Btrfs;
    }
    /* ZFS: vdev label-0 uberblock magic 0x00BAB10C at 128 KiB (LE). */
    {
        u8 z[4] = {};
        if (slice->read_at(0x20000, z, 4) == OK && z[0] == 0x0C && z[1] == 0xB1 && z[2] == 0xBA &&
            z[3] == 0x00)
            return FsFamily::Zfs;
    }
    return FsFamily::Unknown;
}

u32 ScanController::run(IStorageDevice* slice, CancelToken* cancel, bool (*progress)(u32, void*),
                        void* progress_user, ScanStats& stats) {
    if (slice == nullptr)
        return ERR_BAD_PARAMETER;
    const auto t0 = std::chrono::steady_clock::now();
    {
        std::lock_guard<std::mutex> lock(tel_.m);
        tel_.snap = ScanTelemetrySnapshot{};
        tel_.snap.overall_permille = 0;
        tel_.run_t0 = t0;
    }
    tel_running_.store(true, std::memory_order_release);
    tel_cancel_.store(cancel, std::memory_order_release);

    /* ---- S-008 pre-scan repair (corpus RawFixer position: repair, then
     * scan) ----
     * Runs BEFORE the retry/cache pipe is constructed: repair writes must
     * bypass the cache, and the engines/probe_family below must observe
     * the repaired bytes (a cached stale boot sector would defeat the
     * repair). Consent-gated (S1): analyze() is read-only; nothing is
     * written unless the consent hook approves AND the caller provides a
     * write view. Outcome never aborts the scan - the og falls through to
     * scanning the unrepaired volume the same way. Skipped entirely when
     * the token is already cancelled (the stamp path below owns that
     * return; an early return here would leave telemetry running). */
    if (cfg_.pre_scan_repair && !(cancel && cancel->checkpoint())) {
        repair::RepairPlan rplan;
        u32 rrc = repair::analyze(slice, rplan);
        stats.repair_ran = true;
        stats.repair_detected = rplan.detected;
        std::snprintf(stats.repair_family, sizeof(stats.repair_family), "%s",
                      rplan.fs_family.c_str());
        stats.repair_advisories = rplan.advisories;
        stats.repair_actions_planned = u32(rplan.actions.size());
        stats.repair_status = rrc;
        const auto emit_repair = [&](u32 outcome, u64 actions) {
            if (!event_fn_)
                return;
            event::Event e;
            e.type = event::Type::Repair;
            e.stage = 0xFFFFFFFFu;
            e.timestamp_ms = 0; /* pre-stage: no run clock yet */
            e.repair_actions = actions;
            e.repair_outcome = outcome;
            event_fn_(e, event_user_);
        };
        if (rrc == OK && !rplan.refused && !rplan.actions.empty()) {
            emit_repair(0, rplan.actions.size()); /* ready for consent */
            bool do_exec = false;
            if (cfg_.repair.consent)
                do_exec = cfg_.repair.consent(rplan, cfg_.repair.user);
            if (!do_exec) {
                stats.repair_consent_denied = true;
                emit_repair(2, rplan.actions.size());
            } else {
                repair::IWriteDevice* wdev =
                    cfg_.repair.open_writer ? cfg_.repair.open_writer(cfg_.repair.user) : nullptr;
                if (wdev == nullptr) {
                    stats.repair_writer_unavailable = true;
                    emit_repair(3, 0);
                } else {
                    /* A-002: the plan's offsets are SLICE-relative (the
                     * analyze ran on the scan's slice view), but the
                     * hooks' writer is a PARENT device (whole disk / whole
                     * image file). Rebase before execute so the repair
                     * lands at the volume's parent-absolute offset - and
                     * the undo log carries parent-absolute offsets, the
                     * exact contract 'fix --undo-apply' documents. */
                    repair::rebase_plan(rplan, slice->absolute_base());
                    rrc = repair::execute(wdev, rplan, stats.repair_undo);
                    stats.repair_status = rrc;
                    stats.repair_actions_applied =
                        rrc == OK ? u32(rplan.actions.size()) : 0; /* atomic: all or rolled back */
                    if (rrc == OK)
                        emit_repair(1, stats.repair_actions_applied);
                    else
                        emit_repair(3, 0); /* rolled back; scan continues unrepaired */
                    if (cfg_.repair.close_writer)
                        cfg_.repair.close_writer(wdev, cfg_.repair.user);
                }
            }
        }
    }

    /* R1.6 production wiring (audit row 40a): every engine read goes
     * through retry+bisection (bad-sector isolation with Skip-mode
     * zero-fill, so one dead spot cannot abort a scan) and a bounded LRU
     * cache (MFT re-reads, FAT chains). The borrowed slice enters the
     * chain via the non-owning SliceShim. The accumulated error map
     * surfaces in ScanStats. */
    RetryPolicy policy;
    policy.max_attempts = 3;
    policy.mode = RetryMode::Skip; /* surface + zero-fill, never abort */
    auto shim = std::unique_ptr<IStorageDevice>(cr::SliceShim::shim(slice));
    auto retry = RetryingDevice::create(std::move(shim), policy);
    /* Raw pointer captured BEFORE the unique_ptr moves into the cache:
     * `retry` is null after CachedReader::create(std::move(retry)) - the
     * previous tel_retry_.store(retry.get()) stored nullptr, silently
     * disabling BOTH the live error map (telemetry G-06) and the
     * end-of-run stats merge (save-on-cancel contract). retry_raw stays
     * valid exactly while the pipe chain keeps the device alive: the
     * cache owns it on success; on cache-creation failure create()
     * destroys it, so the pointer is only published when `cached` owns
     * it and the pipe falls back to the bare slice. */
    RetryingDevice* retry_raw = retry.get();
    /* Cache block 256 KiB (core-api session, read-granularity tuning): the
     * reference's device layer chunks ReadFile at 4 MiB and its NTFS engine
     * batches the MFT stream at 2 MiB (Device_decompiled.c:56545;
     * NTFSSearchDLL_decompiled.c:5649-5678) - every og chunk is a 64 KiB
     * multiple by construction. 64 KiB blocks paid 16x the user/kernel
     * transitions per byte; 256 KiB recovers 4x of that while bounding the
     * over-fetch on random misses (FAT deleted-chain walks) at 4x. The
     * transport-level sweep (bench/tools/alignbench, PERFORMANCE-BASELINE
     * section 5) measures the full curve; tune on its data. Cache stays
     * 8 MiB total (capacity = 32 blocks). */
    auto cached = CachedReader::create(std::move(retry), 8ull << 20, 256 * 1024);
    IStorageDevice* pipe = cached ? static_cast<IStorageDevice*>(cached.get())
                                  : (retry_raw ? static_cast<IStorageDevice*>(retry_raw) : slice);
    if (cached)
        tel_retry_.store(retry_raw, std::memory_order_release);

    if (cfg_.fs_hint == FsFamily::Unknown && (cfg_.run_quick || cfg_.run_deep))
        cfg_.fs_hint = probe_family(pipe);
    /* volume facts for the session VolumeInfo (G-14) - recorded from the
     * caller's slice before the pipe wrapped it */
    vol_offset_ = slice->absolute_base();
    vol_bytes_ = slice->size();
    vol_logical_sector_ = slice->geometry().logical_sector;
    vol_physical_sector_ = slice->geometry().physical_sector;
    /* SESS-2/GAP-W3-9a: volume identity hash - CRC32 over the first 512
     * bytes of the slice (the boot sector / volume header). Recorded at
     * run start; the resume path recomputes and compares, so a session
     * resumed against a different volume warns instead of silently
     * producing garbage. Zero (unreadable start) = unverifiable, and the
     * resume side skips comparison on zero. */
    {
        u8 boot[512] = {};
        vol_boot_hash_ =
            slice->read_at(0, boot, sizeof(boot)) == OK ? table::crc32(boot, sizeof(boot)) : 0;
    }

    const auto stamp = [&stats, t0, cancel, this](u32 rc) -> u32 {
        const auto t1 = std::chrono::steady_clock::now();
        u64 ms = u64(std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count());
        /* pause compensation (RE parity): elapsed excludes paused time */
        const u64 paused = cancel ? cancel->paused_ms() : 0;
        stats.elapsed_ms = ms > paused ? ms - paused : 0;
        /* the retry pipe's error map always lands in stats - including the
         * cancel/error paths, so a save-on-cancel session carries it */
        if (RetryingDevice* rd = tel_retry_.load(std::memory_order_acquire))
            stats.errors.merge(rd->error_map_copy());
        tel_counts_(stats, cancel);
        /* N-016 (diff-to-fix): this was emitted twice back-to-back (a
         * 56cce87 merge artifact) - FilesFound double-fired at teardown;
         * one emit, like the other three call sites. */
        emit_boundary_events_(tel_stage_.load(std::memory_order_relaxed)); /* GAP-S6-10 */
        {
            std::lock_guard<std::mutex> lock(tel_.m);
            /* Finalize under the same lock telemetry() queries: the snapshot
             * keeps the final error map + elapsed after the pipe dies, and
             * the live pointers (retry pipe, caller token) are cleared
             * before anything they point at can be destroyed - a query in
             * flight either saw them alive or sees them gone, never dead. */
            tel_.snap.errors.merge(stats.errors);
            tel_.snap.elapsed_ms = stats.elapsed_ms;
            tel_.snap.cancelled = rc == ERR_CANCELLED;
            tel_.snap.paused = false; /* the run has ended */
            tel_running_.store(false, std::memory_order_release);
            tel_retry_.store(nullptr, std::memory_order_release);
            tel_cancel_.store(nullptr, std::memory_order_release);
        }
        return rc;
    };
    if (cancel && cancel->checkpoint())
        return stamp(ERR_CANCELLED);
    checkpoint_stats_ = &stats; /* G-13: checkpoint serialization source */
    if (cfg_.run_quick) {
        /* F-1: quick's nodes are slice-relative at emit; rebase to the
         * absolute device basis BEFORE the stage-boundary checkpoint so
         * sessions/checkpoints/consumers all see one basis */
        const size_t quick_mark = tree_.node_count_snapshot();
        if (u32 rc = run_quick(pipe, cancel, progress, progress_user, stats))
            return stamp(rc);
        tree_.rebase_new_extents(quick_mark, vol_offset_);
        if (u32 rc = emit_checkpoint_()) /* stage boundary (G-13) */
            return stamp(rc);
    }
    if (cfg_.run_deep) {
        if (cancel && cancel->checkpoint())
            return stamp(ERR_CANCELLED);
        if (u32 rc = run_deep(pipe, cancel, progress, progress_user, stats))
            return stamp(rc);
        if (u32 rc = emit_checkpoint_()) /* stage boundary (G-13) */
            return stamp(rc);
    }
    if (cfg_.run_carve) {
        if (cancel && cancel->checkpoint())
            return stamp(ERR_CANCELLED);
        const size_t carve_mark = tree_.node_count_snapshot();
        if (u32 rc = run_carve_stage(pipe, cancel, progress, progress_user, stats))
            return stamp(rc);
        /* GAP-S6-8: carve now emits DISK-absolute extents with the
         * NODE_FLAG_EXT_SHIFTED guard (cr_carve.cpp) - this rebase skips
         * every carved node and is retained as the boundary invariant. */
        tree_.rebase_new_extents(carve_mark, vol_offset_);
        if (u32 rc = emit_checkpoint_()) /* stage boundary (G-13) */
            return stamp(rc);
    }
    checkpoint_stats_ = nullptr;
    /* surface the retry layer's error map (R1.6; merged inside stamp for
     * every exit path) + cache counters */
    if (cached)
        stats.cached_hits = cached->hits(), stats.cached_misses = cached->misses();
    tree_.refresh_stats();
    /* S-023: the post-scan CRecoverability grading pass - the og runs its
     * two "Recoverability analyze" jobs at scan completion (DRWUI
     * FUN_14038aad0); ours is one deterministic pass over the stable
     * tree. Device context: this scan's single slice (ordinal 0 - the
     * ABI scans one device; the og's multi-disk device index is not
     * expressible here, registered in the S-023 Resolution). */
    {
        tree::grading::GradeContext gctx;
        tree::grading::DeviceSlot slot;
        slot.base = 0; /* extents are absolute device bytes post-rebase */
        slot.disk_ordinal = 0;
        slot.fs_kind = 0; /* per-node props->fs_kind is the truth source */
        gctx.devices.push_back(slot);
        tree::grading::grade_tree(tree_, gctx, stats.assistant);
    }
    return stamp(OK);
}

u32 ScanController::run_quick(IStorageDevice* slice, CancelToken* cancel,
                              bool (*progress)(u32, void*), void* pu, ScanStats& stats) {
    /* One engine pass, quick semantics. When deep will also run, the pass
     * uses deep settings already (single walk - the corpus engines emit
     * active+deleted in one MFT walk; re-walking was pure waste). */
    const bool deep_coming = cfg_.run_deep;
    tel_stage_begin_(0, cancel);
    u32 rc = run_engine_pass(slice, deep_coming, cancel, progress, pu, stats, 0, deep_coming);
    if (rc == OK)
        quick_done_ = true;
    tel_stage_end_(0, cancel);
    tel_counts_(stats, cancel);
    emit_boundary_events_(tel_stage_.load(std::memory_order_relaxed)); /* GAP-S6-10 */
    /* boundary tick: forward the POST-STAMP telemetry overall - never a
     * hardcoded weighted() (with covers_both the walk already fed the
     * deep stage, so the bridge has forwarded values up to 750; a
     * 400-guess would regress below them and break monotonicity). */
    if (progress)
        (void)progress(tel_overall_(), pu);
    return rc;
}

namespace {

/* Single engine dispatch (ARCH-DUP-4 / W3-10): ONE table for the main
 * engine pass and the lost-candidate path - the og's single factory
 * dispatch shape (FUN_140577d40). The two old hand-maintained tables had
 * already drifted (the candidate chain never set the deep-semantics
 * flags - the engines' defaults are all true, so candidates ALWAYS ran
 * deep extras while the quick pass disabled them; now one parameter
 * makes that explicit at both call sites).
 *
 * deep_extras: the deep-semantics flags where an engine has them (FAT
 * chain reconstruction, ext4 deleted-inode sweep, HFS+ freed-node sweep,
 * ReFS deleted-row harvest). The main pass passes its deep_mode; the
 * candidate path passes true (probe everything under a lost volume).
 *
 * candidate_mode: best-effort under a single display root - engine
 * errors are swallowed (an unviable candidate volume is not a scan
 * error) and the counters are the candidate display totals (per-engine
 * shapes preserved: NTFS reports emitted, ext4/ReFS fold deleted into
 * the total, HFS+ does not). Otherwise errors propagate and the
 * counters are the main-path active/deleted split. */
struct EngineRun {
    u32 rc = OK;
    u64 files = 0;
    u64 deleted = 0;
    storage::FreeMap free_map; /* S-009: the bitmap truth (main pass only) */
};

EngineRun run_engine_dispatch(FsFamily fam, IStorageDevice* slice, bool deep_extras,
                              bool candidate_mode, CancelToken* cancel,
                              bool (*progress)(u64, u64, void*), void* pu, tree::ResultTree& tree,
                              tree::FullNode* root_active, tree::FullNode* root_deleted,
                              tree::FullNode* root_lost) {
    EngineRun r;
    u32 rc = OK;
    switch (fam) {
    case FsFamily::Ntfs: {
        auto vol = ntfs::NtfsVolume::open(
            std::unique_ptr<IStorageDevice>(cr::SliceShim::shim(slice)), &rc);
        if (!vol) {
            /* candidate mode: open failure = not a viable NTFS volume, not
             * a scan error; main mode: propagate (open failure IS an error) */
            r.rc = candidate_mode ? u32(OK) : (rc == OK ? u32(ERR_OPEN_FAILED) : rc);
            return r;
        }
        std::unique_ptr<ntfs::VolumeBitmap> bm;
        (void)ntfs::VolumeBitmap::load(vol.get(), bm);
        ntfs::WalkerFlags wf;
        ntfs::WalkStats ws;
        rc = ntfs::walk_mft(vol.get(), bm.get(), wf, cancel, progress, pu, tree, root_active,
                            root_deleted, root_lost, ws);
        r.files = candidate_mode ? ws.emitted : ws.active;
        r.deleted = ws.deleted;
        /* S-009: $Bitmap IS the allocation truth (bit set = in use);
         * invert to free (slice-relative base; rebased to the absolute
         * basis at the dispatch tail, exactly like the tree extents). */
        if (bm && bm->valid() && vol->params().cluster_bytes != 0) {
            r.free_map.base_byte = 0;
            r.free_map.cluster_bytes = vol->params().cluster_bytes;
            r.free_map.cluster_count = bm->bit_count();
            r.free_map.bits.assign(size_t((bm->bit_count() + 7) / 8), 0);
            const std::vector<u8>& src = bm->buffer();
            const size_t n = r.free_map.bits.size() < src.size() ? r.free_map.bits.size()
                                                                 : src.size();
            for (size_t i = 0; i < n; ++i)
                r.free_map.bits[i] = u8(~src[i]); /* invert: free = !in-use */
        }
        break;
    }
    case FsFamily::Fat: {
        fat::FatScanFlags fl;
        fl.reconstruct_deleted_chains = deep_extras;
        fat::FatScanStats fs;
        rc = fat::scan_fat(slice, fl, cancel, progress, pu, tree, root_active, root_deleted,
                           root_lost, fs, &r.free_map);
        r.files = fs.files;
        r.deleted = fs.deleted_entries;
        break;
    }
    case FsFamily::Exfat: {
        exfat::ExfatScanFlags fl;
        exfat::ExfatScanStats es;
        rc = exfat::scan_exfat(slice, fl, cancel, progress, pu, tree, root_active, root_deleted,
                               root_lost, es, &r.free_map);
        r.files = es.files;
        r.deleted = es.deleted_entries;
        break;
    }
    case FsFamily::Ext4: {
        ext4::Ext4ScanFlags fl;
        fl.scan_deleted_inodes = deep_extras;
        ext4::Ext4ScanStats es;
        rc = ext4::scan_ext4(slice, fl, cancel, progress, pu, tree, root_active, root_deleted,
                             root_lost, es);
        r.files = es.files + es.dirs + (candidate_mode ? es.deleted_inodes : 0);
        r.deleted = es.deleted_inodes;
        break;
    }
    case FsFamily::Hfsplus: {
        hfsplus::HfsScanFlags fl;
        fl.scan_freed_nodes = deep_extras;
        hfsplus::HfsScanStats hs;
        rc = hfsplus::scan_hfsplus(slice, fl, cancel, progress, pu, tree, root_active, root_deleted,
                                   root_lost, hs);
        r.files = hs.files + hs.dirs;
        r.deleted = hs.orphans;
        break;
    }
    case FsFamily::Btrfs: {
        btrfs::BtrfsScanStats bs;
        rc = btrfs::scan_btrfs(slice, cancel, progress, pu, tree, root_active, root_deleted,
                               root_lost, bs);
        r.files = bs.files + bs.dirs;
        r.deleted = bs.subvolumes;
        break;
    }
    case FsFamily::Zfs: {
        zfs::ZfsScanStats zs;
        rc = zfs::scan_zfs(slice, cancel, progress, pu, tree, root_active, root_deleted, root_lost,
                           zs);
        r.files = zs.files + zs.dirs;
        break;
    }
    case FsFamily::Refs: {
        refs::RefsScanFlags fl;
        fl.harvest_deleted = deep_extras;
        refs::RefsScanStats rs;
        rc = refs::scan_refs(slice, fl, cancel, progress, pu, tree, root_active, root_deleted,
                             root_lost, rs);
        r.files = rs.files + rs.dirs + (candidate_mode ? rs.deleted_rows : 0);
        r.deleted = rs.deleted_rows;
        break;
    }
    default:
        rc = OK; /* unknown FS: quick yields nothing; deep/carve still run */
        break;
    }
    /* S-009: the engine free maps are SLICE-relative; rebase to the
     * absolute device basis exactly like the tree extents (F-1) so the
     * disk map's cell math lines up with the extent overlays. */
    if (r.free_map.valid() && slice->absolute_base() != 0)
        r.free_map.base_byte += slice->absolute_base();
    r.rc = candidate_mode ? u32(OK) : rc;
    return r;
}

} // namespace

u32 ScanController::run_engine_pass(IStorageDevice* slice, bool deep_mode, CancelToken* cancel,
                                    bool (*progress)(u32, void*), void* pu, ScanStats& stats,
                                    u32 bridge_stage, bool covers_both) {
    /* Uniform progress wiring (G-04): EVERY engine gets the bridge - it
     * publishes per-stage telemetry and forwards the monotonic weighted
     * overall permille to the caller's callback. The bridge returns the
     * user callback's verdict so engines that honour progress-false still
     * cancel through it (NTFS walk, carve, lost discovery). */
    StageBridge bridge{this, progress, pu, bridge_stage, covers_both, cancel};
    auto engine_cb = +[](u64 done, u64 total, void* user) -> bool {
        return ScanController::engine_progress_bridge_(done, total, user);
    };
    bool (*ep)(u64, u64, void*) = engine_cb;
    void* ep_user = &bridge;

    u32 rc = OK;
    /* Dynamic disks (LDM): the whole-disk slice carries no filesystem
     * of its own - the volumes live in the 1 MiB database. Parse it and
     * scan every reconstructed volume (simple / spanned / striped /
     * mirrored / RAID-5, incl. reconstruction of a missing RAID-5
     * column) through the standard candidate machinery. Probed
     * UNCONDITIONALLY before the fs_hint switch: probe_family's FAT
     * heuristic (55AA + a pow2 word at +0x0B) can alias an LDM disk's
     * boot-code bytes, which would gate this branch out (LDM audit
     * finding); parse_ldm_database self-rejects non-LDM disks after one
     * 512-byte read, so the cost on regular disks is negligible. */
    {
        table::LdmInfo ldm;
        if (table::parse_ldm_database(slice, ldm) == OK && ldm.found) {
            const table::LdmGuid& self = ldm.ph.disk_guid_bin;
            /* member resolution: platform-provided cross-disk registry
             * first (each member disk's own PRIVHEAD re-anchors its LBAs
             * inside LdmVolumeDevice), local disk as the fallback */
            table::LdmDiskResolver resolver = cfg_.ldm_resolver;
            table::LdmDiskResolver local = [&self,
                                            slice](const table::LdmGuid& g) -> IStorageDevice* {
                return g == self ? slice : nullptr;
            };
            if (!resolver)
                resolver = local;
            for (const auto& lay : ldm.layouts) {
                if (lay.kind == table::LdmVolumeKind::Broken || lay.copies.empty())
                    continue; /* surfaced via the LdmInfo report, not scannable */
                std::u16string role = u"Dynamic Volume ";
                for (const char* p = lay.name.c_str(); *p; ++p)
                    role.push_back(u16(u8(*p)));
                tree::FullNode* vroot = tree_.add_root(role);
                table::LdmVolumeDevice vol(lay, resolver);
                u64 files = 0, deleted = 0;
                (void)scan_candidate(&vol, vroot, cancel, files, deleted, ep, ep_user);
                stats.files_found += files;
                stats.deleted_found += deleted;
            }
            /* no direct boundary tick here: the candidate scans ticked the
             * bridge live, and run_quick/run_deep forward the post-stamp
             * telemetry overall right after this returns - a hardcoded
             * weighted() here could only regress below what the bridge
             * already forwarded */
            return OK;
        }
    }
    /* single dispatch (ARCH-DUP-4): one table for the main pass and the
     * candidate path; deep_extras = this pass's deep_mode, errors
     * propagate, counters are the main-path active/deleted split. */
    const EngineRun er =
        run_engine_dispatch(cfg_.fs_hint, slice, deep_mode, false, cancel, ep, ep_user, tree_,
                            root_found_, root_deleted_, root_lost_);
    if (er.rc != OK)
        return er.rc;
    stats.files_found += er.files;
    stats.deleted_found += er.deleted;
    /* S-009: the main pass owns the free-map truth (candidate probes
     * never overwrite it - a lost volume's bitmap is not the slice's). */
    stats.free_map = er.free_map;
    return rc;
}

u32 ScanController::run_deep(IStorageDevice* slice, CancelToken* cancel,
                             bool (*progress)(u32, void*), void* pu, ScanStats& stats) {
    u32 rc = OK;
    const u32 threads = resolve_threads(cfg_.threads);
    tel_stage_begin_(1, cancel);

    /* Deep extras. The engine pass itself already ran inside quick (single
     * walk) - here we only do what quick did NOT: the remnant sweep and the
     * lost-partition candidates. When quick was disabled, run the engine
     * pass now with full deep semantics. */
    if (!quick_done_) {
        /* F-1: the late engine pass's nodes are slice-relative; rebase */
        const size_t pass_mark = tree_.node_count_snapshot();
        if (u32 e = run_engine_pass(slice, true, cancel, progress, pu, stats, 1, false))
            return e;
        tree_.rebase_new_extents(pass_mark, vol_offset_);
        pass_deep_done_ = true;
    } else if (!pass_deep_done_) {
        /* quick ran the pass with deep semantics (deep_coming) - it counts
         * as the deep pass too (single-walk design) */
        pass_deep_done_ = true;
    }

    /* deep-stage progress bridge for the sweep/discovery/candidates */
    StageBridge dbridge{this, progress, pu, 1, false, cancel};
    auto deep_cb = +[](u64 done, u64 total, void* user) -> bool {
        return ScanController::engine_progress_bridge_(done, total, user);
    };

    if (cfg_.fs_hint == FsFamily::Ntfs) {
        /* remnant sweep (R2.5) - parallel chunk fan-out. F-1: remnant nodes
         * are slice-relative; the fan-out joins before this returns, so the
         * range rebase below is race-free */
        u32 rc2 = OK;
        {
            auto vol = ntfs::NtfsVolume::open(
                std::unique_ptr<IStorageDevice>(cr::SliceShim::shim(slice)), &rc2);
            if (vol) {
                const size_t rem_mark = tree_.node_count_snapshot();
                ntfs::WalkStats rem;
                rc = ntfs::scan_mft_remnants(slice, vol->params().sector_bytes,
                                             vol->params().cluster_bytes, vol->params().record_size,
                                             vol->params().total_clusters, cancel, deep_cb,
                                             &dbridge, tree_, root_lost_, rem, threads);
                tree_.rebase_new_extents(rem_mark, vol_offset_);
                stats.files_found += rem.emitted;
            }
        }
    }

    /* lost-partition discovery (whole-disk mode, R3) */
    if (cfg_.whole_disk) {
        lost::DiscoveryResult dr;
        /* live table for overlap scoring (audit row 31a: passing {}
         * left the Inside/Overlaps penalties dead code) */
        std::vector<table::PartitionInfo> known;
        {
            table::DiskTable t;
            if (table::parse_disk_table(slice, t) == OK)
                known = t.partitions;
        }
        u32 lrc = lost::discover(slice, known, 1, cancel, deep_cb, &dbridge, dr);
        if (lrc == OK) {
            stats.lost_candidates = dr.candidates;
            stats.lost_partitions = dr.candidates.size();
            if (!dr.candidates.empty()) {
                /* Pre-create roots and slices on THIS thread (tree + slice
                 * construction is cheap), then fan the volume scans out. */
                struct CandWork {
                    std::unique_ptr<IStorageDevice> slice;
                    tree::FullNode* root;
                    u64 base; /* F-1: candidate start on the disk */
                };
                std::vector<CandWork> work;
                for (const auto& c : dr.candidates) {
                    emit_lost_candidate_(c); /* GAP-S6-10 */
                    auto cand_slice = lost::materialize(slice, c);
                    if (!cand_slice)
                        continue;
                    std::u16string role = u"Candidate @";
                    char buf[32];
                    snprintf(buf, sizeof(buf), " %llu", (unsigned long long)c.start_lba);
                    for (const char* p = buf; *p; ++p)
                        role.push_back(u16(u8(*p)));
                    const u64 cbase = cand_slice->absolute_base();
                    work.push_back({std::move(cand_slice), tree_.add_root(role), cbase});
                }
                if (!work.empty()) {
                    if (threads > 1 && work.size() > 1) {
                        /* Per-candidate stats (workers never touch shared
                         * plain counters); merged on the caller thread.
                         * F-B-11 (AUDIT-ARCH): `deleted` IS merged now -
                         * the same scan with different --threads produced
                         * different deleted_found counts before. */
                        struct Out {
                            u64 files = 0;
                            u64 deleted = 0;
                            u32 rc = OK;
                        };
                        std::vector<Out> outs(work.size());
                        CandWork* items = work.data();
                        Out* out_arr = outs.data();
                        const u64 n = work.size();
                        ThreadPool pool(threads);
                        /* F-B-10 (AUDIT-ARCH): the constructed-but-unwired
                         * PoolCancelToken is REMOVED (dead). The workers
                         * poll the EXTERNAL cancel token inside
                         * scan_candidate (engine loop heads).
                         * N-017 (diff-to-fix): the carve-style 50 ms
                         * watcher ALSO aborts the pool on cancel - a
                         * QUEUED candidate that starts after the cancel
                         * previously ran to its first engine loop-head
                         * poll (seconds on a damaged volume); now the
                         * queue drains within one watcher tick, matching
                         * cr_carve.cpp's watcher (the core's own
                         * standard, and the og's abort-event shape). */
                        CancelPoolWatcher cancel_watcher(pool, cancel);
                        u32 first_err = parallel_for(pool, n, [&](u64 i) -> u32 {
                            out_arr[i].rc = scan_candidate(items[i].slice.get(), items[i].root,
                                                           cancel, out_arr[i].files,
                                                           out_arr[i].deleted, deep_cb, &dbridge);
                            return OK; /* candidate misses never abort the scan */
                        });
                        cancel_watcher.done();
                        for (const auto& o : outs) {
                            stats.files_found += o.files;
                            stats.deleted_found += o.deleted;
                            (void)o.rc;
                        }
                        if (first_err != OK)
                            rc = first_err;
                        /* F-1: candidate nodes are CANDIDATE-relative; the pool
                         * has joined, so rebase each candidate's private
                         * subtree to the disk basis on this thread */
                        for (const auto& w : work)
                            tree_.rebase_subtree_extents(w.root, w.base);
                    } else {
                        for (auto& w : work) {
                            u64 files = 0, deleted = 0;
                            (void)scan_candidate(w.slice.get(), w.root, cancel, files, deleted,
                                                 deep_cb, &dbridge);
                            stats.files_found += files;
                            stats.deleted_found += deleted;
                            tree_.rebase_subtree_extents(w.root, w.base);
                        }
                    }
                }
            }
        } else if (lrc == ERR_CANCELLED) {
            rc = ERR_CANCELLED; /* cancellation must reach run() (resume truth) */
        }
    }
    /* deep is COMPLETE only when everything it owed finished OK - a
     * cancelled discovery must not stamp the stage done (G-07 integrity) */
    if (rc == OK)
        deep_extras_done_ = true;
    tel_stage_end_(1, cancel);
    tel_counts_(stats, cancel);
    emit_boundary_events_(tel_stage_.load(std::memory_order_relaxed)); /* GAP-S6-10 */
    /* boundary tick: post-stamp telemetry overall (monotonic truth) */
    if (progress)
        (void)progress(tel_overall_(), pu);
    return rc;
}

u32 ScanController::scan_candidate(IStorageDevice* cand_slice, tree::FullNode* croot,
                                   CancelToken* cancel, u64& files_out, u64& deleted_out,
                                   bool (*progress)(u64, u64, void*), void* progress_user) {
    const FsFamily fam = probe_family(cand_slice);
    files_out = 0;
    deleted_out = 0;
    /* single dispatch (ARCH-DUP-4): candidate mode - best-effort under the
     * one display root (engine errors are not candidate errors) with the
     * candidate display totals; deep_extras always true (probe everything
     * under a lost volume - the engines' flag defaults). */
    const EngineRun er = run_engine_dispatch(fam, cand_slice, true, true, cancel, progress,
                                             progress_user, tree_, croot, croot, croot);
    files_out += er.files;
    deleted_out += er.deleted;
    return OK;
}

u32 ScanController::run_carve_stage(IStorageDevice* slice, CancelToken* cancel,
                                    bool (*progress)(u32, void*), void* pu, ScanStats& stats) {
    const u32 threads = resolve_threads(cfg_.threads);
    tel_stage_begin_(2, cancel);
    /* carve progress counts BYTES (G-05) - the bridge flags stage 2 as
     * byte-units so telemetry reports honest bytes_done */
    StageBridge cbridge{this, progress, pu, 2, false, cancel};
    auto carve_cb = +[](u64 done, u64 total, void* user) -> bool {
        return ScanController::engine_progress_bridge_(done, total, user);
    };
    carve::CarveFlags cf;
    cf.threads = threads;
    carve::CarveStats cs;
    carve::NameCallback namer = [](const carve::Signature& sig, u64 size, u32 index) {
        (void)size;
        char buf[32];
        snprintf(buf, sizeof(buf), "Carved%06u.%s", index, sig.ext);
        std::u16string out;
        for (const char* p = buf; *p; ++p)
            out.push_back(u16(u8(*p)));
        return out;
    };
    u32 rc = carve::carve_slice(slice, cf, cancel, carve_cb, &cbridge, tree_, root_carved_, &tree_,
                                namer, cs);
    stats.carved_found = cs.validated;
    stats.bytes_scanned += cs.bytes_scanned;
    if (rc == OK)
        carve_done_ = true;
    tel_stage_end_(2, cancel);
    tel_counts_(stats, cancel);
    emit_boundary_events_(tel_stage_.load(std::memory_order_relaxed)); /* GAP-S6-10 */
    /* boundary tick: post-stamp telemetry overall (monotonic truth) */
    if (progress)
        (void)progress(tel_overall_(), pu);
    return rc;
}

u32 ScanController::save_session(session::Writer& w, const ScanStats& stats) const {
    /* record order: session state, volume info, partition contexts, tree,
     * error map (the writer chains them; ids from the container)
     *
     * G-07 truth: stage_done_mask stamps ACTUAL completion (quick_done_,
     * pass+extras for deep, carve_done_) - never the configured mask. The
     * ORIGINAL configured set is preserved in VolumeInfo.flags so resume
     * can validate mask-⊆-configured. */
    session::SessionState st;
    st.stage_done_mask = (quick_done_ ? STAGE_QUICK : 0u) |
                         ((pass_deep_done_ && deep_extras_done_) ? STAGE_DEEP : 0u) |
                         (carve_done_ ? STAGE_CARVE : 0u);
    st.files_found = stats.files_found;
    st.deleted_found = stats.deleted_found; /* v2 counters */
    st.carved_found = stats.carved_found;
    st.bytes_scanned = stats.bytes_scanned;
    st.error_map_ranges = u32(stats.errors.ranges().size());
    std::vector<u8> blob;
    if (session::serialize_session_state(st, blob) == OK)
        if (u32 rc = w.add_record(session::REC_SESSION_STATE, blob.data(), u32(blob.size())))
            return rc;

    /* S-023: the grading pass's statistics (optional record - readers
     * that predate it skip unknown types; sessions saved without a
     * grading pass simply lack it). */
    if (session::serialize_assistant_stats(stats.assistant, blob) == OK)
        if (u32 rc = w.add_record(session::REC_ASSISTANT_STATS, blob.data(), u32(blob.size())))
            return rc;

    session::VolumeInfo vi;
    vi.fs_kind = u32(cfg_.fs_hint); /* FsFamily on the wire (D-1 truth) */
    vi.flags = session_configured_mask_ | (cfg_.run_quick ? STAGE_QUICK : 0u) |
               (cfg_.run_deep ? STAGE_DEEP : 0u) | (cfg_.run_carve ? STAGE_CARVE : 0u);
    vi.slice_offset = vol_offset_; /* G-14: real placement */
    vi.slice_bytes = vol_bytes_;
    vi.logical_sector = vol_logical_sector_;
    vi.physical_sector = vol_physical_sector_;
    for (size_t i = 0; i < sizeof(vi.label) - 1 && cfg_.volume_label[i]; ++i)
        vi.label[i] = cfg_.volume_label[i];
    /* identity card (SESS-2 v2 tail) */
    for (size_t i = 0; i < sizeof(vi.model) - 1 && cfg_.volume_model[i]; ++i)
        vi.model[i] = cfg_.volume_model[i];
    for (size_t i = 0; i < sizeof(vi.serial) - 1 && cfg_.volume_serial[i]; ++i)
        vi.serial[i] = cfg_.volume_serial[i];
    vi.boot_hash = vol_boot_hash_;
    vi.scanned_bytes = stats.bytes_scanned;
    vi.elapsed_ms = stats.elapsed_ms;
    /* S-030 v3 tail: the image target's file path (empty otherwise) */
    for (size_t i = 0; i < sizeof(vi.image) - 1 && cfg_.volume_image[i]; ++i)
        vi.image[i] = cfg_.volume_image[i];
    if (session::serialize_volume_info(vi, blob) == OK)
        if (u32 rc = w.add_record(session::REC_VOLUME_INFO, blob.data(), u32(blob.size())))
            return rc;

    for (const auto& c : stats.lost_candidates) {
        session::PartitionContext pc;
        pc.start_lba = c.start_lba;
        pc.sector_count = c.sector_count;
        pc.fs_kind = u32(c.fs);
        pc.score = u32(c.score > 0 ? c.score : 0);
        pc.kind = 1;
        if (session::serialize_partition_context(pc, blob) == OK)
            if (u32 rc =
                    w.add_record(session::REC_PARTITION_CONTEXT, blob.data(), u32(blob.size())))
                return rc;
    }

    if (session::serialize_tree(tree_, blob) == OK)
        if (u32 rc = w.add_record(session::REC_RESULT_TREE, blob.data(), u32(blob.size())))
            return rc;

    if (session::serialize_error_map(stats.errors, blob) == OK)
        if (u32 rc = w.add_record(session::REC_ERROR_MAP, blob.data(), u32(blob.size())))
                return rc;

    /* S-009: the downsampled free map (optional record; readers that
     * predate it skip unknown types). The RAW cluster map is never
     * persisted - the cell-granular form is bounded by the same overlay
     * budget the map builder honors, so a 16 TiB volume persists <= 8
     * MiB, not a 512 MiB $Bitmap copy. */
    if (stats.free_map.valid()) {
        storage::FreeMap down;
        if (viz::downsample_free_map(stats.free_map, vol_offset_, vol_bytes_, down) == OK) {
            if (session::serialize_free_map(down, blob) == OK)
                if (u32 rc = w.add_record(session::REC_ALLOC_MAP, blob.data(), u32(blob.size())))
                    return rc;
        }
    }
    return OK;
}

u32 ScanController::load_session(session::Reader& r, tree::ResultTree& tree, ScanStats& stats) {
    session::VolumeSessionInfo info;
    return load_session_summary(r, tree, stats, info);
}

u32 ScanController::load_session_summary(session::Reader& r, tree::ResultTree& tree,
                                         ScanStats& stats, session::VolumeSessionInfo& info) {
    u32 count = 0;
    if (u32 rc = r.open(&count))
        return rc;
    session::SessionState st;
    bool have_state = false;
    session::VolumeInfo vi;
    bool have_vi = false;
    for (u32 i = 0; i < count; ++i) {
        session::Record rec;
        u32 rc = r.read_record(i, rec);
        if (rc == ERR_BAD_CHECKSUM)
            continue; /* corrupt: skip, keep going (F10) */
        if (rc != OK)
            return rc;
        switch (rec.type) {
        case session::REC_RESULT_TREE:
            if (session::deserialize_tree(rec.data.data(), rec.data.size(), tree) != OK)
                return ERR_SESSION_FORMAT;
            break;
        case session::REC_SESSION_STATE: {
            if (session::deserialize_session_state(rec.data.data(), rec.data.size(), st) == OK) {
                stats.files_found = st.files_found;
                stats.deleted_found = st.deleted_found; /* v2 (0 on v1 records) */
                stats.carved_found = st.carved_found;
                stats.bytes_scanned = st.bytes_scanned;
                have_state = true;
            }
            break;
        }
        case session::REC_ERROR_MAP:
            (void)session::deserialize_error_map(rec.data.data(), rec.data.size(), stats.errors);
            break;
        case session::REC_ALLOC_MAP: /* S-009: the free overlay truth */
            (void)session::deserialize_free_map(rec.data.data(), rec.data.size(), stats.free_map);
            break;
        case session::REC_ASSISTANT_STATS: /* S-023 */
            (void)session::deserialize_assistant_stats(rec.data.data(), rec.data.size(),
                                                       stats.assistant);
            break;
        case session::REC_VOLUME_INFO:
            if (session::deserialize_volume_info(rec.data.data(), rec.data.size(), vi) == OK)
                have_vi = true;
            break;
        case session::REC_PARTITION_CONTEXT: {
            /* G-23: restore lost candidates so a reloaded session can be
             * recovered from (and re-saved) without re-running deep. */
            session::PartitionContext pc;
            if (session::deserialize_partition_context(rec.data.data(), rec.data.size(), pc) ==
                    OK &&
                pc.kind == 1 && pc.fs_kind <= u32(lost::CandidateFs::BitLocker)) {
                /* S-006: Ext2..Refs appended (values 6..10); sessions written
                 * before S-006 only carry <= Unknown (5) and stay valid.
                 * S-019: BitLocker appended (value 10) - same rule. */
                lost::Candidate c;
                c.start_lba = pc.start_lba;
                c.sector_count = pc.sector_count;
                c.fs = lost::CandidateFs(u8(pc.fs_kind));
                c.score = int(pc.score);
                stats.lost_candidates.push_back(c);
            }
            break;
        }
        default:
            break;
        }
    }
    tree.refresh_stats();

    /* G-14 summary + G-07 integrity: the completion mask must be a subset
     * of the configured-stages flags stored alongside it. Legacy sessions
     * (no VolumeInfo) assume all stages configured - their masks were
     * config-derived by construction, so the subset check holds trivially. */
    info = session::VolumeSessionInfo{};
    info.fs_family = have_vi ? vi.fs_kind : 0;
    info.stage_done_mask = have_state ? st.stage_done_mask : 0;
    info.configured_mask = have_vi ? vi.flags : (STAGE_QUICK | STAGE_DEEP | STAGE_CARVE);
    info.slice_offset = vi.slice_offset;
    info.slice_bytes = vi.slice_bytes;
    info.logical_sector = vi.logical_sector;
    info.physical_sector = vi.physical_sector;
    for (size_t i = 0; i < sizeof(info.label); ++i)
        info.label[i] = vi.label[i];
    for (size_t i = 0; i < sizeof(info.model); ++i)
        info.model[i] = vi.model[i];
    for (size_t i = 0; i < sizeof(info.serial); ++i)
        info.serial[i] = vi.serial[i];
    for (size_t i = 0; i < sizeof(info.image); ++i)
        info.image[i] = vi.image[i];
    info.boot_hash = vi.boot_hash;
    info.scanned_bytes = vi.scanned_bytes;
    info.elapsed_ms = vi.elapsed_ms;
    if ((info.stage_done_mask & ~info.configured_mask) != 0)
        return ERR_SESSION_FORMAT; /* mask integrity violation - never a
                                      silent partial resume */
    /* F-1: pre-normalization sessions carry SLICE-RELATIVE extents (the
     * merge-time rebase was never implemented before the flag existed).
     * Normalize to the absolute basis using the recorded slice placement,
     * so every consumer (writer reads, F12 overlap, re-save) sees one
     * basis regardless of which build wrote the session. Old whole-disk
     * sessions with lost candidates keep their candidate-relative nodes
     * (the container carries no per-candidate placement for the tree) -
     * documented limitation, unchanged behavior. */
    if (!(r.flags() & session::CRSF_FLAG_ABS_EXTENTS) && info.slice_offset != 0)
        for (auto* root : tree.roots())
            tree.rebase_subtree_extents(root, info.slice_offset);
    stats.lost_partitions = stats.lost_candidates.size();
    return OK;
}

u32 ScanController::resume(session::Reader& r, tree::ResultTree& tree,
                           const ScanConfig& original_cfg, ScanStats& stats,
                           session::VolumeSessionInfo& info, std::unique_ptr<ScanController>& out) {
    out.reset();
    if (u32 rc = load_session_summary(r, tree, stats, info))
        return rc;
    ScanConfig cfg = original_cfg;
    if (info.stage_done_mask & STAGE_QUICK)
        cfg.run_quick = false; /* completed stages keep their results */
    if (info.stage_done_mask & STAGE_DEEP)
        cfg.run_deep = false;
    if (info.stage_done_mask & STAGE_CARVE)
        cfg.run_carve = false;
    if (info.fs_family != 0 && cfg.fs_hint == FsFamily::Unknown)
        cfg.fs_hint = FsFamily(u8(info.fs_family));
    out.reset(new (std::nothrow) ScanController(tree, std::move(cfg), info));
    if (!out)
        return ERR_NO_MEMORY;
    return OK;
}

} // namespace orchestrator
} // namespace cr
