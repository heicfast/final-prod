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

#include <algorithm>
#include <functional>
#include <ctime>
#include "cr/cr_log.h"
#include "cr/cr_bits.h"
#include "cr/cr_text.h"
#include "cr/cr_errmod.h"
#include "cr/btrfs/cr_btrfs_engine.h"
#include "cr/carve/cr_carve.h"
#include "cr/cr_bits.h"
#include "cr/cr_parallel.h"
#include "cr/exfat/cr_exfat_engine.h"
#include "cr/ext4/cr_ext4_engine.h"
#include "cr/fat/cr_fat_engine.h"
#include "cr/hfsplus/cr_hfsplus_engine.h"
#include "cr/lost/cr_lostpart.h"
#include "cr/lost/cr_consistency.h"
#include "cr/ntfs/cr_bootstrap.h"
#include "cr/ntfs/cr_bitmap.h"
#include "cr/ntfs/cr_mftwalk.h"
#include "cr/refs/cr_refs_engine.h"
#include "cr/session/cr_crsf.h"
#include "cr/orchestrator/cr_filterfolder.h"
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

/* M-049: FsFamily -> the module id stamped on that family's engine
 * sub-pass (the engine-side subset of the og's per-module error
 * registrars). The structured families map 1:1; the recognition-only
 * and unknown families attribute to the orchestrator (they have no
 * walk engine - whatever reads happen are the scan layer's own).
 * Pinned by the ABI static_asserts in cr_ui.cpp (CR_ERRMOD_*). */
u32 module_for_family(FsFamily fam) {
    switch (fam) {
    case FsFamily::Ntfs:
        return errmod::kNtfs;
    case FsFamily::Fat:
        return errmod::kFat;
    case FsFamily::Exfat:
        return errmod::kExfat;
    case FsFamily::Ext4:
        return errmod::kExt4;
    case FsFamily::Btrfs:
        return errmod::kBtrfs;
    case FsFamily::Hfsplus:
        return errmod::kHfsplus;
    case FsFamily::Refs:
        return errmod::kRefs;
    case FsFamily::Zfs:
        return errmod::kZfs;
    default:
        return errmod::kOrchestrator;
    }
}

/* M-035: collect the walked files under a candidate root for the
 * content-consistency scorer. Safe on the candidate's own worker thread
 * after its dispatch returned (single-writer handoff: only this thread
 * emitted into this root; other candidates' workers never touch it). */
void collect_consistency_files(tree::FullNode* root, std::vector<lost::ConsistencyFile>& out) {
    if (root == nullptr)
        return;
    std::vector<tree::FileNode*> stack;
    for (tree::FileNode* c = root->first_child(); c != nullptr; c = c->next_sibling())
        stack.push_back(c);
    while (!stack.empty()) {
        tree::FileNode* n = stack.back();
        stack.pop_back();
        const tree::FileProperties* p = n->props();
        if (p != nullptr) {
            lost::ConsistencyFile f;
            f.name = &p->name;
            f.is_directory = (p->attributes & 0x10) != 0; /* ATTR_DIRECTORY */
            f.length = p->logical_size;
            if (!p->extents.empty()) {
                f.has_extent = true;
                f.data_start = p->extents.front().offset;
            }
            out.push_back(f);
        }
        for (tree::FileNode* c = n->first_child(); c != nullptr; c = c->next_sibling())
            stack.push_back(c);
    }
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

std::u16string ScanController::raw_root_name_() const {
    /* M-029: the configured display name when present (the og's
     * "filter_generalraw" naming surface), the default role otherwise. */
    for (const char c : cfg_.raw_category_name)
        if (c != '\0')
            return text::utf8_to_u16(cfg_.raw_category_name);
    return u"Other Deleted Files (Carved)";
}

ReadPipe build_read_pipe(IStorageDevice* slice, u64 cache_bytes, u32 block_bytes) {
    /* M-058: single construction point for the scan read chain
     * (slice -> SliceShim -> RetryingDevice -> CachedReader), with the
     * degraded path as a PINNED contract: when CachedReader::create fails
     * (bad cache geometry - unreachable from run()'s constants), create()
     * has already DESTROYED the retry device it took by value, so the only
     * memory-safe fallback is the BARE SLICE. The pre-M-058 inline code
     * fell back to the raw retry pointer there - a use-after-free on the
     * defensive path, and the opposite of what its own comment documented.
     * Degradation truth: no cache (re-read cost only) AND no retry
     * isolation (a dead spot aborts its engine read; surfaced, never
     * hidden). */
    ReadPipe rp;
    if (slice == nullptr)
        return rp; /* caller error; run() null-checks before calling */
    RetryPolicy policy;
    policy.max_attempts = 3;
    policy.mode = RetryMode::Skip; /* surface + zero-fill, never abort */
    auto shim = std::unique_ptr<IStorageDevice>(cr::SliceShim::shim(slice));
    auto retry = RetryingDevice::create(std::move(shim), policy);
    if (!retry) {
        /* shim/retry construction failure (never taken for slice != null):
         * degrade to the bare slice, same contract as the cache-failure
         * path below. */
        rp.pipe = slice;
        return rp;
    }
    RetryingDevice* retry_raw = retry.get();
    auto cached = CachedReader::create(std::move(retry), cache_bytes, block_bytes);
    if (!cached) {
        /* create() destroyed the retry device on this path - the bare
         * slice is the only live device left. */
        rp.pipe = slice;
        return rp;
    }
    rp.retry = retry_raw; /* valid: the cache owns it from here to teardown */
    rp.pipe = cached.get();
    rp.chain = std::move(cached);
    return rp;
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
        root_carved_ = tree_.add_root(raw_root_name_());
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
            /* M-029: the raw root may carry the ORIGINAL run's configured
             * display name (persisted as its role); recognize both the
             * default and the current config so a resume never spawns a
             * duplicate raw root. */
            else if (role == u"Other Deleted Files (Carved)" || role == raw_root_name_())
                root_carved_ = static_cast<tree::FullNode*>(r);
        }
    } else if (cfg_.run_quick || cfg_.run_deep) {
        root_found_ = tree_.add_root(u"Found");
        root_deleted_ = tree_.add_root(u"Deleted");
        root_lost_ = tree_.add_root(u"Lost Files");
    }
    if (cfg_.run_carve && root_carved_ == nullptr)
        root_carved_ = tree_.add_root(raw_root_name_());
    /* M-042: a whole-disk-basis resume (the only basis persisted lost
     * candidates exist on - whole_disk scans) enables the run-start
     * re-validation; volume-basis resumes keep the candidates as-is
     * (their LBAs would read against the wrong base). */
    resumed_whole_disk_ = resumed.slice_offset == 0;
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
    /* M-079: the label joins the adopted identity card (the SESS-2
     * pattern - a re-save without a new run preserves the loaded label;
     * a fresh run's derivation then sees it non-empty and leaves it). */
    if (cfg_.volume_label[0] == '\0')
        for (size_t i = 0; i < sizeof(cfg_.volume_label); ++i)
            cfg_.volume_label[i] = resumed.label[i];
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
    /* N-048: the stage-begin boundary (the og's Quick/Deep scan range
     * lines bracket each stage) */
    if (cr::log::enabled())
        cr::log::write(cr::log::Level::Info, "stage %u begin", stage);
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
    u32 newest_module = 0, newest_attempts = 0;
    {
        std::lock_guard<std::mutex> lock(tel_.m);
        const auto& ranges = tel_.snap.errors.ranges();
        count = u32(ranges.size());
        if (count > 0) {
            newest_start = ranges[size_t(count) - 1].offset;
            newest_len = ranges[size_t(count) - 1].length;
            /* M-049: the newest range's module attribution + retry state
             * (recorded at failure time by the retry device - cr_errmod.h
             * for the attribution model). */
            newest_module = ranges[size_t(count) - 1].module;
            newest_attempts = ranges[size_t(count) - 1].attempts;
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
            e.err_module = newest_module;
            e.err_operation = errmod::op_read;
            e.err_attempts = newest_attempts;
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
    /* N-061 (the og's CalcPartition.log: candidate generation + scoring
     * is the lost-partition search's real output format - qa-logs
     * section 3): every discovered candidate is a run-log line with the
     * scored truth. */
    if (cr::log::enabled())
        cr::log::write(cr::log::Level::Info, "lost candidate: lba=%llu sectors=%llu fs=%u score=%u",
                       cand.start_lba, cand.sector_count, u32(cand.fs), cand.score);
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
     * emit overwrites from position 0 (the STAGING side; the committed
     * final path always holds the newest COMPLETE checkpoint - a torn
     * staging file is no worse than none, the previous good final is
     * never destroyed: N-014's atomic-rename contract). The sink triple
     * IS the writer's IO (a file-backed sink stages <path>.tmp and the
     * commit hook renames over <path>). The record set matches
     * save_session exactly - mask truth included. */
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
    /* N-014: the atomic commit - file-backed sinks rename the completed
     * .tmp over the final path here (a crash before this point leaves
     * the previous good checkpoint in place; a crash after it leaves the
     * new one - both sides of the window are good states). */
    if (cfg_.checkpoint.commit)
        if (u32 rc = cfg_.checkpoint.commit(cfg_.checkpoint.user))
            return rc;
    ++checkpoint_count_;
    /* N-048 (the og's stage-boundary/checkpoint trail): every committed
     * checkpoint is a bracket event in the run log. */
    if (cr::log::enabled())
        cr::log::write(cr::log::Level::Info, "checkpoint %llu committed", u64(checkpoint_count_));
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
    if (std::memcmp(sec + 3, "NTFS    ", 8) == 0) {
        /* M-043 (the confirmed half): the og's analyser proves NTFS by
         * content BEFORE the engine - the boot gates (bps 512-multiple
         * <= 0x1000, SPC pow2 nonzero; FUN_180005470 :4292-4307, decode
         * re-verified first-hand this session) then the first-records
         * probe with per-record validation (FILE magic + USA count < 0x11
         * + USA offset in [0x2a, 0x2a+0xd7); FUN_180005180 :4038-4058).
         * The cheap form here: record 0's magic + USA bounds after a
         * plausible-geometry check ($MFT LCN u64@0x30 per the spec-exact
         * wire struct - kernel NTFS_BOOT_SECTOR layout: total_sectors
         * occupies 0x28; spc u8@0x0D, bps u16@0x0B). A spoofed OEM on a non-NTFS sector no longer
         * dispatches the engine - the og's analyser bails the same way
         * (it never reaches result 4). Failure falls through to the
         * remaining family probes. NOTE (M-043 corrected half, corpus-
         * proven): the og has NO scan-path backup-boot fallback - the
         * analyser only memcmp-verifies the volume-end backup against
         * the primary and stores the verified location (+0x510) as the
         * REPAIR input; neither it nor NTFSSearchDLL (zero backup reads
         * in the decompile) ever scans from the backup. The core's
         * consent-gated rawfixer owns that path. */
        const u16 bps = u16_from_bytes_le(sec + 0x0B);
        const u8 spc = sec[0x0D];
        const u64 mft_lcn = le64(sec + 0x30); /* $MFT LCN (0x28 is total_sectors) */
        if (bps >= 512 && bps <= 0x1000 && (bps % 512) == 0 && spc != 0 && (spc & (spc - 1)) == 0 &&
            mft_lcn != 0) {
            const u64 at = mft_lcn * u64(spc) * bps;
            u8 rec[512]; /* the USA fields live in record 0's first sector */
            if (slice->read_at(at, rec, sizeof(rec)) == OK && std::memcmp(rec, "FILE", 4) == 0) {
                /* spec offsets: USA offset u16@0x04, USA count u16@0x06 (the
                 * og's bounds: count in [1,0x11), offset in [0x2a,0x201) —
                 * FUN_180005180 :4038-4058) */
                const u16 usa_off = u16_from_bytes_le(rec + 0x04);
                const u16 usa_cnt = u16_from_bytes_le(rec + 0x06);
                if (usa_cnt != 0 && usa_cnt < 0x11 && usa_off >= 0x2a && usa_off < 0x2a + 0xd7) {
                    log::write(log::Level::Info,
                               "recognition: ntfs content-verified (MFT record 0 + USA "
                               "bounds)");
                    return FsFamily::Ntfs;
                }
            }
        }
        /* content proof failed: not NTFS by proof - fall through */
    }
    if (std::memcmp(sec + 3, "EXFAT   ", 8) == 0) {
        /* N-050 (the exFAT half): the og's analyser layer proves content
         * BEFORE the engine runs; the cheap exFAT form is the engine's
         * own boot-region validator (EXFAT-4's structural gates: OEM,
         * 55AA, shift bounds, the MustBeZero sweep +0x0B..+0x3F, FAT
         * count/length, heap fit - exFATSearchDLL FUN_180021e80
         * :28180-28208). A spoofed EXFAT OEM on a FAT/NTFS sector fails
         * the sweep (their BPB bytes are nonzero there) and never
         * dispatches the engine; a real boot region proves itself. The
         * boot-checksum verdict stays informational exactly as the
         * engine treats it (the engine gates on the structural rc only
         * - matching verdicts, no recovery-negative strictness). */
        exfat::ExfatParams xp;
        if (exfat::validate_boot_region(slice, xp) == OK) {
            log::write(log::Level::Info, "recognition: exfat content-verified (boot region)");
            return FsFamily::Exfat;
        }
        /* content proof failed: fall through (the FAT gates underneath
         * decide - the og's multi-analyser outcome) */
    }
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
    /* UDF (M-044): the og's CFatVolumeAnalyser probes UDF BEFORE FAT
     * classification (FATFileSystemAnalyser_decompiled.c FUN_180002580
     * :1413-1435, re-verified first-hand against the corpus). The probe,
     * exactly the og's arithmetic:
     *  - fast-reject: boot bytes[0..2] ALL non-zero (jump code) -> the
     *    og's ||-short-circuit skips the probe entirely (a FAT/NTFS boot
     *    sector never touches the UDF path);
     *  - read one sector at volume start + 0x100 sectors (the UDF spec
     *    Anchor Volume Descriptor Pointer at LBA 256);
     *  - read one sector at volume start + u32@(AVDP + 0x14) (the
     *    ECMA-167 MainVolumeDescriptorSequenceExtent location field);
     *  - classify UDF iff bytes[0x19..0x1b] == "UDF" (the og's analyser
     *    result 0xc). Both reads go through the slice contract, so a
     *    truncated/foreign device fails the probe clean (the og's fail
     *    path) instead of guessing. Sector units: the og reads device
     *    sectors through its IStorageDevice vtable; our logical_sector
     *    is the same quantity (512 or 4096 on real transports).
     * No engine dispatch follows on either side (no UDF SearchDLL exists
     * in the og's 9-engine set) - the default dispatch yields nothing
     * for quick/deep and carve recovers content by signature. */
    if (sec[0] == 0 || sec[1] == 0 || sec[2] == 0) {
        const u32 ss = slice->geometry().logical_sector;
        if (ss >= 512 && ss <= 4096) {
            u8 anchor[4096];
            if (slice->read_at(u64(0x100) * ss, anchor, ss) == OK) {
                const u32 vds_lba = u32(anchor[0x14]) | (u32(anchor[0x15]) << 8) |
                                    (u32(anchor[0x16]) << 16) | (u32(anchor[0x17]) << 24);
                u8 vds[4096];
                if (slice->read_at(u64(vds_lba) * ss, vds, ss) == OK && vds[0x19] == 'U' &&
                    vds[0x1a] == 'D' && vds[0x1b] == 'F')
                    return FsFamily::Udf;
            }
        }
    }
    /* FAT: bytes-per-sector sane + 55AA + numFATs != 0 (the og FAT
     * analyser's gate - M-045's landed corpus set; an NTFS BPB has zeros
     * at 0x10-0x27, so a spoofed-NTFS boot falling through after a failed
     * content proof must NOT land here: the og's FAT analyser refuses it
     * the same way and the family result is Unknown). */
    if (sec[510] == 0x55 && sec[511] == 0xAA) {
        const u16 bps = u16_from_bytes_le(sec + 0x0B);
        if (sec[0x10] != 0 && bps >= 512 && bps <= 4096 && (bps & (bps - 1)) == 0) {
            /* N-050 (the FAT half): the og's analyser verifies the ROOT
             * DIRECTORY after FAT-type classification before recognition
             * (FATFileSystemAnalyser FUN_180002180 :1124-1144: fixed-root
             * scan for types 1/2 = FUN_180001f70, root-cluster chain walk
             * for type 3 = FUN_180001d70). The cheap form (the finding's
             * fix plan): ONE bounded read at the computed root offset -
             * FAT12/16: reserved + numFats*spf16; FAT32 (spf16 == 0):
             * reserved + numFats*spf32 + (rootCluster-2)*spc. A volume
             * whose root is unreadable (spoofed OEM over garbage, a
             * truncated image) fails the proof and never dispatches the
             * engine; a readable root sector proves the classification. */
            const u16 reserved = u16_from_bytes_le(sec + 0x0E);
            const u8 spc = sec[0x0D];
            const u16 spf16 = u16_from_bytes_le(sec + 0x16);
            u64 root_sector = 0;
            bool root_known = false;
            if (spf16 != 0) {
                root_sector = u64(reserved) + u64(sec[0x10]) * spf16;
                root_known = true; /* FAT12/16 fixed root */
            } else {
                const u32 spf32 = u32_from_bytes_le(sec + 0x24);
                const u32 root_cluster = u32_from_bytes_le(sec + 0x2C);
                if (spc != 0 && root_cluster >= 2) {
                    root_sector =
                        u64(reserved) + u64(sec[0x10]) * spf32 + (u64(root_cluster) - 2) * spc;
                    root_known = true; /* FAT32 root cluster */
                }
            }
            u8 root_first[512];
            if (root_known &&
                slice->read_at(root_sector * bps, root_first, sizeof(root_first)) == OK) {
                log::write(log::Level::Info,
                           "recognition: fat content-verified (root directory readable)");
                return FsFamily::Fat;
            }
            /* root proof failed: not FAT by proof - fall through */
        }
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

    /* ---- M-042: search-start disk-truth pass (the og's at-search-start
     * semantics: the partinfo layout compare PRD:20998-21039 + the
     * per-record re-validation PRD:20748-20842). Placement note: BEFORE
     * the pre-scan repair hook - the og runs its lostpart load/compare at
     * search start, prior to any repair pass, and a repaired boot must
     * not retroactively change the persisted-vs-fresh verdict. ----
     *
     * (a) capture the live whole-disk table (saved as REC_DISK_LAYOUT at
     *     save/checkpoint time - the og's partinfo rebuild);
     * (b) when the resumed session carried a layout snapshot, compare:
     *     stats.layout_changed = the honest "disk changed" signal
     *     (surfaced, never enforced);
     * (c) re-validate every RESTORED lost candidate (stats.lost_candidates
     *     is non-empty at run() start ONLY when it was populated by
     *     load_session/resume - a fresh run starts empty and discovery
     *     fills it mid-run): re-read each record's start LBA through the
     *     CURRENT disk and drop the records the disk no longer confirms.
     *     Only on whole-disk-basis resumes (see the ctor note). */
    disk_layout_.clear();
    {
        table::DiskTable live;
        if (table::parse_disk_table(slice, live) == OK) {
            disk_layout_.reserve(live.partitions.size());
            for (const auto& p : live.partitions) {
                session::DiskLayoutEntry e;
                e.start_lba = p.start_lba;
                e.sector_count = p.sector_count;
                e.mbr_type = p.mbr_type;
                disk_layout_.push_back(e);
            }
        }
    }
    if (!stats.persisted_layout.empty() && !disk_layout_.empty()) {
        bool changed = stats.persisted_layout.size() != disk_layout_.size();
        if (!changed) {
            for (size_t i = 0; i < disk_layout_.size(); i++) {
                const auto& a = stats.persisted_layout[i];
                const auto& b = disk_layout_[i];
                if (a.start_lba != b.start_lba || a.sector_count != b.sector_count ||
                    a.mbr_type != b.mbr_type) {
                    changed = true;
                    break;
                }
            }
        }
        stats.layout_changed = changed;
        stats.persisted_layout.clear();
        stats.persisted_layout.shrink_to_fit();
    }
    if (resumed_whole_disk_ && !stats.lost_candidates.empty()) {
        std::vector<lost::Candidate> confirmed;
        confirmed.reserve(stats.lost_candidates.size());
        stats.revalidated_candidates = stats.lost_candidates.size();
        for (const auto& c : stats.lost_candidates) {
            if (lost::revalidate_candidate(slice, c))
                confirmed.push_back(c);
            else
                ++stats.dropped_stale_candidates;
        }
        stats.lost_candidates = std::move(confirmed);
        stats.lost_partitions = stats.lost_candidates.size();
    }

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

    /* ---- M-041 pre-scan damage-audit check phase (the og's
     * FileSystemCheck position: after repair, before the engines) ----
     * Read-only; runs on the caller's raw slice (pre-cache: the audit
     * must see the same bytes repair wrote, uncached). The report NEVER
     * aborts the scan — the og's escalation ladder audits, then scans
     * anyway (rawfixer-fs-repair.md:59-65: only volumes that resist
     * RawFixer fall through to RAW carving; the audit informs, never
     * gates). Skipped when already cancelled (same rule as repair). */
    if (cfg_.pre_scan_check && !(cancel && cancel->checkpoint())) {
        stats.check_ran = true;
        stats.check_status = check::run_fscheck(slice, check::CheckCallbacks{}, stats.check_report);
        stats.check_detected = stats.check_report.family_detected;
        if (event_fn_) {
            event::Event e;
            e.type = event::Type::FsCheck;
            e.stage = 0xFFFFFFFFu;
            e.timestamp_ms = 0; /* pre-stage: no run clock yet */
            e.check_errors = stats.check_report.errors.size();
            e.check_family =
                stats.check_report.family_detected ? u32(stats.check_report.family[0]) : 0;
            event_fn_(e, event_user_);
        }
    }

    /* R1.6 production wiring (audit row 40a): every engine read goes
     * through retry+bisection (bad-sector isolation with Skip-mode
     * zero-fill, so one dead spot cannot abort a scan) and a bounded LRU
     * cache (MFT re-reads, FAT chains). The borrowed slice enters the
     * chain via the non-owning SliceShim. The accumulated error map
     * surfaces in ScanStats.
     *
     * M-058: the construction lives in build_read_pipe() - the defensive
     * fallback (cache-creation failure) is a pinned contract there, not
     * an inline pointer dance. Cache block 256 KiB (core-api session,
     * read-granularity tuning): the reference's device layer chunks
     * ReadFile at 4 MiB and its NTFS engine batches the MFT stream at
     * 2 MiB (Device_decompiled.c:56545; NTFSSearchDLL_decompiled.c:5649-
     * 5678) - every og chunk is a 64 KiB multiple by construction. 64 KiB
     * blocks paid 16x the user/kernel transitions per byte; 256 KiB
     * recovers 4x of that while bounding the over-fetch on random misses
     * (FAT deleted-chain walks) at 4x. The transport-level sweep
     * (bench/tools/alignbench, PERFORMANCE-BASELINE section 5) measures
     * the full curve; tune on its data. Cache stays 8 MiB total
     * (capacity = 32 blocks). The 8-vs-4 MiB budget AND the
     * reactive-LRU-vs-og-prefetch-thread algorithm are REGISTERED
     * deliberate divergences (N-045, PARITY-DELTAS D-72; og cites:
     * DRWUI_decompiled.c:919080-:919092 budget 0x400000/sector,
     * :962250-:962597 prefetch engine) — pinned by
     * Cached.ScanCacheBudgetIsTheRegisteredDivergence. */
    ReadPipe rp = build_read_pipe(slice, 8ull << 20, 256 * 1024);
    IStorageDevice* pipe = rp.pipe;
    /* tel_retry_ published only when the chain owns the retry device (the
     * live error-map source: telemetry G-06 + the end-of-run stats merge
     * for the save-on-cancel contract). On the degraded path (bare
     * slice) there is no retry layer, hence no error map to publish. */
    if (rp.retry)
        tel_retry_.store(rp.retry, std::memory_order_release);

    /* S-051: record whether the HOST asserted a family before the probe
     * mutates cfg_.fs_hint - a host-forced hint keeps the single dispatch
     * (the RUN 3 control), everything the probe produced routes to the
     * whole-disk table-target flow when the slice carries a table. */
    host_fs_hint_ = cfg_.fs_hint != FsFamily::Unknown;
    if (cfg_.fs_hint == FsFamily::Unknown && (cfg_.run_quick || cfg_.run_deep)) {
        /* M-049: the probe is the scan layer's own read - attributed to
         * the orchestrator module (a failing probe sector is the
         * orchestrator's failure, not an engine's). */
        errmod::Guard g(errmod::kOrchestrator);
        publish_owner_module_(errmod::kOrchestrator);
        /* read-truth (the fake-scan fix): snapshot the pipe's error map
         * around the probe read so a BLIND probe (Skip-mode zero-fill
         * returned OK with zeros - the retry pipe's contract) is
         * distinguishable from a readable sector with no filesystem
         * signature. probe_family alone cannot tell the two apart; without
         * this flag every fully-unreadable volume cascades into
         * family=Unknown -> no engines -> instant stage completes -> the
         * "scan complete, 0 files, exit 0" fake verdict (the exact
         * appears-alive-while-blind failure class the mandate kills). */
        const u64 errors_before = rp.retry ? rp.retry->error_map_copy().total_bytes() : u64(0);
        cfg_.fs_hint = probe_family(pipe);
        if (rp.retry) {
            const ReadErrorMap after = rp.retry->error_map_copy();
            /* the probe was blind IFF a recorded range COVERS OFFSET 0 -
             * the probe's own bytes. Map growth alone is the wrong
             * predicate: the cache's 256 KiB block-fill around the probe
             * read can record failures in regions far beyond the probe's
             * 512 bytes (partial damage elsewhere) without the header
             * itself being dead. Ranges are sorted; a range covering 0
             * must start at exactly 0. */
            bool covers_zero = false;
            for (const ReadErrorRange& r : after.ranges())
                if (r.offset == 0 && r.length > 0) {
                    covers_zero = true;
                    break;
                }
            if (covers_zero) {
                stats.probe_unreadable = true;
                cr::log::write(
                    cr::log::Level::Warn,
                    "recognition: volume header unreadable - family unknown because the "
                    "probe reads failed (%llu error bytes total)",
                    static_cast<unsigned long long>(after.total_bytes() - errors_before));
            }
        }
    }

    /* volume facts for the session VolumeInfo (G-14) - recorded from the
     * caller's slice before the pipe wrapped it */
    vol_offset_ = slice->absolute_base();
    vol_bytes_ = slice->size();
    vol_logical_sector_ = slice->geometry().logical_sector;
    vol_physical_sector_ = slice->geometry().physical_sector;

    /* N-048/N-061 (the og's scan bracket - CScanMgr::doScan + the
     * quick/deep sector-range lines, qa-logs channel): the START
     * bracket carries the slice extent, the probe hint and the
     * configured stage set. Silent unless a host registered a sink. */
    if (cr::log::enabled())
        cr::log::write(cr::log::Level::Info,
                       "scan start: extent=%llu+%llu fs_hint=%u stages=quick:%d deep:%d carve:%d",
                       vol_offset_, vol_bytes_, u32(cfg_.fs_hint), cfg_.run_quick ? 1 : 0,
                       cfg_.run_deep ? 1 : 0, cfg_.run_carve ? 1 : 0);
    /* SESS-2/GAP-W3-9a: volume identity hash - CRC32 over the first 512
     * bytes of the slice (the boot sector / volume header). Recorded at
     * run start; the resume path recomputes and compares, so a session
     * resumed against a different volume warns instead of silently
     * producing garbage. Zero (unreadable start) = unverifiable, and the
     * resume side skips comparison on zero. */
    {
        u8 boot[512] = {};
        if (slice->read_at(0, boot, sizeof(boot)) == OK) {
            vol_boot_hash_ = table::crc32(boot, sizeof(boot));
            /* N-066: keep the full sector image for the identity card's
             * v4 tail (the og record-3's +0x518 snapshot). */
            std::memcpy(vol_boot_snapshot_, boot, sizeof(boot));
            vol_boot_snapshot_valid_ = true;
        } else {
            vol_boot_hash_ = 0;
            vol_boot_snapshot_valid_ = false;
        }
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
        /* N-048/N-061 (the og's UpdateScanFinishedInfo bracket): the END
         * bracket carries the result code, the counters, the error-map
         * size and the pause-compensated duration. */
        if (cr::log::enabled())
            cr::log::write(cr::log::Level::Info,
                           "scan end: rc=%u files=%llu deleted=%llu carved=%llu lost=%llu "
                           "error_ranges=%u elapsed_ms=%llu",
                           rc, stats.files_found, stats.deleted_found, stats.carved_found,
                           stats.lost_partitions, u32(stats.errors.ranges().size()),
                           stats.elapsed_ms);
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
    if (rp.chain)
        stats.cached_hits = rp.chain->hits(), stats.cached_misses = rp.chain->misses();
    tree_.refresh_stats();
    /* M-018: the og's PushFolderFilter pass (FilterFolder.ini hidden
     * sets, pushed per scan-partition at scan setup - applog
     * :22:57:57.920 "run to CDoScanPartition::PushFolderFilter:84").
     * Post-walk, pre-grading, pre-session-capture: query rows, the CLI
     * tree and every bulk recovery walk (rrecov's recover-all plan, the
     * cr_ui session/snapshot subtree plans) skip flagged nodes, so
     * sessions/reports/recovery agree with the og's presented tree
     * (M-071 restored the rrecov pair the N-064 reconstruction dropped
     * and extended the gate to the cr_ui walks that postdate M-018;
     * explicit single-node selection stays allowed - the og's filter is
     * a results-set default, not a hard block). */
    stats.junk_filtered = filterfolder::apply(tree_);
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
    /* ---- read-truth completion gate (the fake-scan fix; docs/
     * DRIVER-REQUIRED.md section 7, PARITY-DELTAS D-DRV-6) ----
     *
     * A scan that could read (essentially) NOTHING must not report
     * success: Skip-mode zero-fill makes every failing read look like an
     * empty region downstream, the stages complete over zeroed data, and
     * the pre-fix verdict was "scan complete, 0 files, exit 0" - the
     * product appeared to work while being blind. The truth line: when
     * the retry pipe's recorded failure coverage spans the whole slice
     * (with one carve-window tail of slack for rounding) and nothing was
     * found, the run REFUSES with ERR_READ_FAILED. Partial damage keeps
     * the salvage semantics exactly as before (og parity: skip bad
     * sectors, surface the map); the zero-success line is the only new
     * refusal. A second, narrower gate: probe_unreadable + zero bytes
     * scanned + nothing found (no engine ran, no carve window read) -
     * the nothing-was-ever-attempted fake. */
    if (RetryingDevice* rd = tel_retry_.load(std::memory_order_acquire))
        stats.errors.merge(rd->error_map_copy()); /* idempotent (S-016) */
    stats.bytes_unreadable = stats.errors.total_bytes();
    {
        constexpr u64 kBlindnessSlack = 64ull * 1024; /* one carve window tail */
        const bool fully_blind = vol_bytes_ > 0 &&
                                 stats.bytes_unreadable + kBlindnessSlack >= vol_bytes_ &&
                                 stats.files_found == 0 && stats.carved_found == 0;
        const bool nothing_read = stats.probe_unreadable && stats.bytes_scanned == 0 &&
                                  stats.files_found == 0 && stats.carved_found == 0 &&
                                  stats.deleted_found == 0;
        if (fully_blind || nothing_read) {
            cr::log::write(cr::log::Level::Error,
                           "scan refused: %llu of %llu bytes unreadable (%u error range(s)) "
                           "- nothing on this volume could be read",
                           static_cast<unsigned long long>(stats.bytes_unreadable),
                           static_cast<unsigned long long>(vol_bytes_),
                           u32(stats.errors.ranges().size()));
            return stamp(ERR_READ_FAILED);
        }
        if (stats.bytes_unreadable > 0)
            cr::log::write(cr::log::Level::Warn,
                           "read truth: %llu of %llu bytes unreadable (%u range(s)) - "
                           "those regions were NOT scanned",
                           static_cast<unsigned long long>(stats.bytes_unreadable),
                           static_cast<unsigned long long>(vol_bytes_),
                           u32(stats.errors.ranges().size()));
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
            const size_t n =
                r.free_map.bits.size() < src.size() ? r.free_map.bits.size() : src.size();
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
    /* M-049: the engine-pass attribution - every read this pass issues
     * (the engine's own + the LDM probe below) carries the family's
     * module stamp on this thread; engine-internal worker pools fall
     * back to the published owner. The candidate path stamps itself
     * (scan_candidate) - it dispatches per-candidate families. */
    const u32 pass_module = module_for_family(cfg_.fs_hint);
    errmod::Guard pass_guard(pass_module);
    publish_owner_module_(pass_module);
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
        /* M-049: the database parse + volume reconstruction reads are the
         * table layer's own (not the fs_hint family's) - stamped LDM for
         * this block, restoring the pass module on exit. */
        errmod::Guard ldm_guard(errmod::kLdm);
        publish_owner_module_(errmod::kLdm);
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
    /* S-051: whole-disk target-set construction - the og's CDoScanPartition
     * flow (architecture/04-recovery-workflow.md section 3.4): the
     * controller "reads the partition list" at scan setup and makes every
     * known volume a scan target (lost partitions are ADDITIONAL targets,
     * inserted later by the --whole-disk discovery). Our plain whole-disk
     * flow previously probed LBA 0, saw the MBR (no FS signature),
     * returned Unknown and the dispatch default completed "successfully"
     * with ZERO files on a healthy multi-volume disk (the executed RUN 2:
     * rc=0, files_found=0 on a two-volume fixture). Gates, in order:
     *   - LDM keeps precedence (the block above returned for dynamic
     *     disks - a dynamic disk's volumes come from the database, not
     *     the table);
     *   - !host_fs_hint_: the host-unset case builds table targets. A
     *     host-forced family is an explicit slice assertion (the CLI
     *     --family / the ABI's cfg passthrough) and keeps the single
     *     dispatch - the og's controller builds the full searcher set
     *     for the deep flow, not for an asserted family. A PROBED family
     *     (the LBA-0 probe can alias FAT heuristics on MBR boot-code
     *     bytes) routes here too: a table-bearing LBA 0 is by definition
     *     not a filesystem volume, so the table flow is the honest path;
     *   - a parseable table with at least one used, non-container
     *     partition (Extended/Protective are containers; a superfloppy
     *     has no table and keeps the single-slice dispatch unchanged).
     * Each target is a PartitionSlice over the same retry/cache pipe
     * (bad-sector isolation + MFT/FAT re-read caching apply per volume),
     * probed independently - a volume whose probe fails is skipped
     * HONESTLY (counted in stats.table_volumes_skipped + an Info log
     * line), never fatal, never silent. */
    if (!host_fs_hint_) {
        errmod::Guard tbl_guard(errmod::kOrchestrator);
        publish_owner_module_(errmod::kOrchestrator);
        table::DiskTable t;
        if (slice->absolute_base() == 0 && table::parse_disk_table(slice, t) == OK &&
            t.layout != table::TableLayout::None) {
            std::vector<table::PartitionInfo> targets;
            for (const auto& p : t.partitions)
                if (p.used && p.fs != table::FsKind::Extended &&
                    p.fs != table::FsKind::Protective && p.sector_count != 0)
                    targets.push_back(p);
            if (!targets.empty()) {
                stats.table_volumes = targets.size();
                table_target_lbas_.clear();
                const u64 ls = slice->geometry().logical_sector;
                bool any_scanned = false;
                for (const auto& p : targets) {
                    table_target_lbas_.push_back(p.start_lba);
                    if (cancel && cancel->checkpoint())
                        return ERR_CANCELLED;
                    auto tgt = PartitionSlice::create(slice, p.start_lba * ls, p.sector_count * ls);
                    if (!tgt) {
                        ++stats.table_volumes_skipped;
                        continue;
                    }
                    FsFamily fam;
                    {
                        errmod::Guard probe_guard(errmod::kOrchestrator);
                        fam = probe_family(tgt.get());
                    }
                    if (fam == FsFamily::Unknown) {
                        ++stats.table_volumes_skipped;
                        if (cr::log::enabled())
                            cr::log::write(cr::log::Level::Info,
                                           "whole-disk target skipped: partition @LBA %llu (%llu "
                                           "sectors) has no identifiable filesystem",
                                           static_cast<unsigned long long>(p.start_lba),
                                           static_cast<unsigned long long>(p.sector_count));
                        continue;
                    }
                    /* the volume's tree root - one root per target (the
                     * og's per-volume display-name shape; the naming is
                     * M-104's surface, this root carries the LBA-anchored
                     * identity until then). GPT partitions carry the
                     * table's UTF16 name when the field is non-empty. */
                    std::u16string role;
                    if (p.from_gpt) {
                        for (int k = 0; k < 36 && p.name[k]; ++k)
                            role.push_back(p.name[k]);
                    }
                    if (role.empty()) {
                        role = u"Volume @ ";
                        char buf[32];
                        snprintf(buf, sizeof(buf), "%llu",
                                 static_cast<unsigned long long>(p.start_lba));
                        for (const char* q = buf; *q; ++q)
                            role.push_back(u16(u8(*q)));
                    }
                    tree::FullNode* vroot = tree_.add_root(role);
                    const u64 tbase = tgt->absolute_base();
                    /* per-volume engine errors are not scan errors (an
                     * unviable table volume must not abort the disk scan) -
                     * the candidate-mode contract; the per-volume probe
                     * already ran, so a dispatch open failure counts as a
                     * skipped volume, honestly. */
                    errmod::Guard vfam_guard(module_for_family(fam));
                    const EngineRun ter =
                        run_engine_dispatch(fam, tgt.get(), deep_mode, true, cancel, ep, ep_user,
                                            tree_, vroot, vroot, vroot);
                    /* F-1: the volume's nodes are slice-relative; the
                     * per-target subtree rebases to the disk-absolute
                     * basis like the lost candidates' */
                    tree_.rebase_subtree_extents(vroot, tbase);
                    if (ter.rc == OK) {
                        stats.files_found += ter.files;
                        stats.deleted_found += ter.deleted;
                        ++stats.table_volumes_scanned;
                        any_scanned = true;
                    } else {
                        ++stats.table_volumes_skipped;
                        if (cr::log::enabled())
                            cr::log::write(
                                cr::log::Level::Info,
                                "whole-disk target engine error: partition @LBA %llu rc=%u",
                                static_cast<unsigned long long>(p.start_lba), ter.rc);
                    }
                }
                /* one structured pass ran over the table's volumes - the
                 * stage's deep pass (remnant sweep etc.) does not apply
                 * on a per-volume basis; the whole-disk extras (discovery,
                 * carve) run by their own stages below. Boundary tick +
                 * telemetry come from the caller's post-stamp forward,
                 * exactly like the LDM block above. */
                (void)any_scanned;
                return OK;
            }
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
            /* M-049: the sweep's own reads + its internal fan-out
             * workers attribute to NTFS (the owner fallback covers the
             * pool threads - their thread stamps are unset). */
            errmod::Guard rem_guard(errmod::kNtfs);
            publish_owner_module_(errmod::kNtfs);
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
            /* M-049: the table re-read is the orchestrator's own */
            errmod::Guard tbl_guard(errmod::kOrchestrator);
            publish_owner_module_(errmod::kOrchestrator);
            table::DiskTable t;
            if (table::parse_disk_table(slice, t) == OK)
                known = t.partitions;
        }
        /* M-049: discovery's signature-sweep reads attribute to the
         * lost-partition search module */
        errmod::Guard lost_guard(errmod::kLost);
        publish_owner_module_(errmod::kLost);
        u32 lrc = lost::discover(slice, known, 1, cancel, deep_cb, &dbridge, dr);
        if (lrc == OK) {
            /* S-051: reconcile the discovery's table-matched candidates
             * against the engine pass's primary target set - a candidate
             * whose start LBA equals a scanned table volume is a
             * DUPLICATE (the volume is a primary target now, not a lost
             * partition; the og inserts lost partitions as ADDITIONAL
             * targets only). Drop BEFORE the persistence assignment so
             * the session's lost list never carries in-table volumes, and
             * each volume enters the tree exactly once. */
            if (!table_target_lbas_.empty() && !dr.candidates.empty()) {
                std::vector<lost::Candidate> kept;
                kept.reserve(dr.candidates.size());
                for (auto& c : dr.candidates) {
                    bool dup = false;
                    for (u64 lba : table_target_lbas_) {
                        if (lba == c.start_lba) {
                            dup = true;
                            break;
                        }
                    }
                    if (!dup)
                        kept.push_back(std::move(c));
                }
                dr.candidates = std::move(kept);
            }
            stats.lost_candidates = dr.candidates;
            stats.lost_partitions = dr.candidates.size();
            if (!dr.candidates.empty()) {
                /* M-042: the og's per-hit persistence (PRD:20409 - the
                 * scan loop appends every FOUND partition's record
                 * unconditionally, so a killed scan still leaves its
                 * finds on disk). The core's equivalent: force a full
                 * checkpoint the moment discovery produces candidates,
                 * ahead of any stage boundary - still gated on a
                 * configured sink (the core has no module-local .dat
                 * side-channel; a session-less run has nowhere to write,
                 * the honest architecture difference registered with the
                 * finding). */
                if (u32 hit_rc = emit_checkpoint_())
                    return hit_rc;
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
                            u64 content_rejected = 0;
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
                            out_arr[i].rc =
                                scan_candidate(items[i].slice.get(), items[i].root, cancel,
                                               out_arr[i].files, out_arr[i].deleted, deep_cb,
                                               &dbridge, &out_arr[i].content_rejected);
                            return OK; /* candidate misses never abort the scan */
                        });
                        cancel_watcher.done();
                        for (const auto& o : outs) {
                            stats.files_found += o.files;
                            stats.deleted_found += o.deleted;
                            stats.lost_content_rejected += o.content_rejected; /* M-035 */
                            (void)o.rc;
                        }
                        if (first_err != OK)
                            rc = first_err;
                        /* F-1: candidate nodes are CANDIDATE-relative; the pool
                         * has joined, so rebase each candidate's private
                         * subtree to the disk basis on this thread (a
                         * content-REJECTED candidate's subtree was detached
                         * - its root is empty and the rebase is a no-op,
                         * but skipping it keeps the intent explicit) */
                        for (size_t wi = 0; wi < work.size(); ++wi)
                            if (outs[wi].content_rejected == 0)
                                tree_.rebase_subtree_extents(work[wi].root, work[wi].base);
                    } else {
                        for (auto& w : work) {
                            u64 files = 0, deleted = 0, content_rejected = 0;
                            (void)scan_candidate(w.slice.get(), w.root, cancel, files, deleted,
                                                 deep_cb, &dbridge, &content_rejected);
                            stats.files_found += files;
                            stats.deleted_found += deleted;
                            stats.lost_content_rejected += content_rejected; /* M-035 */
                            if (content_rejected == 0) /* detached: nothing to rebase */
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
                                   bool (*progress)(u64, u64, void*), void* progress_user,
                                   u64* content_rejected) {
    /* M-049: the probe is the orchestrator's read; the dispatch is the
     * candidate's family - stamped HERE (not at the fan-out site) so
     * every pool task carries its own candidate's attribution exactly,
     * with no cross-task race on any shared state (thread stamps are
     * per-thread). */
    FsFamily fam;
    {
        errmod::Guard probe_guard(errmod::kOrchestrator);
        fam = probe_family(cand_slice);
    }
    files_out = 0;
    deleted_out = 0;
    /* single dispatch (ARCH-DUP-4): candidate mode - best-effort under the
     * one display root (engine errors are not candidate errors) with the
     * candidate display totals; deep_extras always true (probe everything
     * under a lost volume - the engines' flag defaults). */
    errmod::Guard fam_guard(module_for_family(fam));
    /* No owner publication here: the owner stays the ENCLOSING sub-pass's
     * (kLost from the deep-stage discovery / kLdm from the engine pass's
     * database block) - the honest coarse fallback for engine-internal
     * worker pools, which cannot know the candidate's family. This
     * thread's stamp (fam_guard) is exact for the direct reads. */
    const EngineRun er = run_engine_dispatch(fam, cand_slice, true, true, cancel, progress,
                                             progress_user, tree_, croot, croot, croot);
    files_out += er.files;
    deleted_out += er.deleted;

    /* M-035: the og's ENGINE-SIDE content-consistency validation, ported
     * to our seam (the og runs it inside the exFAT engine's candidate
     * walk - FUN_180025390, exFAT-only per OQ-FLT-1's cross-engine scan;
     * our engines emit directly into the shared tree, so the score runs
     * over the walk's output HERE and a reject detaches the subtree -
     * same observable outcome: a content-inconsistent candidate surfaces
     * no files). Uniform for every exFAT candidate-shaped walk (the lost
     * fan-out AND the LDM volume scans); the og's LDM-path wiring is
     * corpus-UNVERIFIED (documented with the finding). */
    if (fam == FsFamily::Exfat) {
        std::vector<lost::ConsistencyFile> files;
        collect_consistency_files(croot, files);
        const lost::ConsistencyVerdict v =
            lost::score_content_consistency(cand_slice, files.data(), files.size());
        if (v.rejected) {
            tree_.detach_subtree(croot);
            files_out = 0;
            deleted_out = 0;
            if (content_rejected != nullptr)
                ++(*content_rejected);
            /* N-048 seam: the rejection is an Info-level audit line (the
             * og's CalcPartition flow logs its accept/reject decisions). */
            if (cr::log::enabled())
                cr::log::write(cr::log::Level::Info,
                               "lost candidate content-rejected: considered=%llu "
                               "filterable=%llu matched=%llu ratio=%.3f%s",
                               v.considered, v.filterable, v.matched, v.ratio,
                               v.out_of_bounds ? " (data start out of bounds)" : "");
        }
    }
    return OK;
}

u32 ScanController::run_carve_stage(IStorageDevice* slice, CancelToken* cancel,
                                    bool (*progress)(u32, void*), void* pu, ScanStats& stats) {
    const u32 threads = resolve_threads(cfg_.threads);
    tel_stage_begin_(2, cancel);
    /* M-049: the carve sweep's reads (its internal window fan-out
     * included - the owner fallback) attribute to the carve module. */
    errmod::Guard carve_guard(errmod::kCarve);
    publish_owner_module_(errmod::kCarve);
    /* carve progress counts BYTES (G-05) - the bridge flags stage 2 as
     * byte-units so telemetry reports honest bytes_done */
    StageBridge cbridge{this, progress, pu, 2, false, cancel};
    auto carve_cb = +[](u64 done, u64 total, void* user) -> bool {
        return ScanController::engine_progress_bridge_(done, total, user);
    };
    carve::CarveFlags cf;
    cf.threads = threads;
    /* M-039 (diff-to-fix): the cross-engine occupancy gate - the og's
     * per-volume bitmap every FS engine fills from its walk (EXT4
     * fill-from-every-extent :6866-6869; Btrfs per-node :2547-2551;
     * Zfs :40576/:40599), available to the raw phase. The core's shape:
     * the orchestrator aggregates every STRUCTURED node's extents from
     * the result tree (the emit path already built them per file - the
     * fix plan's step 1 verbatim) into a sorted, coalesced interval set,
     * rebased slice-relative, and hands it to the carve as a skip gate
     * alongside the M-005 allocation-bitmap gate. Carved nodes never
     * feed the gate (their regions are the carve's own output; feeding
     * them would suppress re-carve on resume - the og's bitmap is
     * FS-engine-fed only). The dedup tree stays as the safety net. */
    std::vector<std::pair<u64, u64>> occupied;
    {
        std::vector<std::pair<u64, u64>> raw;
        std::function<void(tree::FileNode*)> collect = [&](tree::FileNode* n) {
            for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
                if (c->origin() == tree::Origin::Structured && c->props() != nullptr) {
                    for (const auto& ex : c->props()->extents) {
                        if (ex.length > 0)
                            raw.push_back({ex.offset, ex.offset + ex.length});
                    }
                }
                collect(c);
            }
        };
        for (auto* r : tree_.roots())
            collect(r);
        if (!raw.empty()) {
            std::sort(raw.begin(), raw.end());
            occupied.push_back(raw[0]);
            for (size_t k = 1; k < raw.size(); ++k) {
                if (raw[k].first <= occupied.back().second)
                    occupied.back().second = std::max(occupied.back().second, raw[k].second);
                else
                    occupied.push_back(raw[k]);
            }
            /* absolute -> slice-relative (the F-1 tree basis; the carve's
             * positions are slice-relative like the M-005 rebase below) */
            const u64 ab = slice->absolute_base();
            if (ab != 0) {
                for (auto& iv : occupied) {
                    iv.first = iv.first >= ab ? iv.first - ab : 0;
                    iv.second = iv.second >= ab ? iv.second - ab : 0;
                }
            }
            cf.occupied_ranges = &occupied;
        }
    }
    /* M-005: gate the sweep with the engine pass's allocation truth (the
     * S-009 FreeMap). stats.free_map is on the ABSOLUTE basis (F-1 rebase
     * at the engine dispatch tail); the carve works slice-relative - copy
     * with the base shifted back. No valid map (carve-only runs, raw
     * disks, pre-S-009 sessions) -> null gate -> carve everything, the
     * og's no-bitmap behavior. */
    storage::FreeMap carve_fm;
    if (stats.free_map.valid()) {
        carve_fm = stats.free_map;
        if (slice->absolute_base() != 0)
            carve_fm.base_byte = carve_fm.base_byte >= slice->absolute_base()
                                     ? carve_fm.base_byte - slice->absolute_base()
                                     : 0; /* defensive: a map starting before
                                             this slice gates from its start */
        cf.free_map = &carve_fm;
    }
    carve::CarveStats cs;
    /* N-014b: no orchestrator-side namer - both carve layers name
     * through the single registered chain (the og per-analyser
     * embedded_name first, then the File%06u default documented in
     * PARITY-DELTAS). The old "Carved%06u" lambda here produced a second
     * name family inside one scan (grammar layer "Carved...", raw-sign
     * layer "File...") - the split N-014b flagged; the family is one now.
     * Hosts wanting their own policy pass a namer to carve_slice
     * directly (both layers honor it since the N-014b routing). */
    u32 rc = carve::carve_slice(slice, cf, cancel, carve_cb, &cbridge, tree_, root_carved_, &tree_,
                                nullptr, cs);
    stats.carved_found = cs.validated;
    stats.bytes_scanned += cs.bytes_scanned;
    stats.carve_bitmap_skipped_bytes = cs.bitmap_skipped_bytes; /* M-005 */
    stats.carve_zero_strides = cs.zero_strides_skipped;         /* N-031 */
    stats.carve_occupancy_skipped = cs.occupancy_skipped;       /* M-039 */
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

u32 ScanController::save_session(session::Writer& w, const ScanStats& stats,
                                 session::SerializeProgress progress, void* progress_user) const {
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
    /* N-066: the record-3 axes + the boot snapshot (v4 tail). */
    for (size_t i = 0; i < sizeof(vi.firmware) - 1 && cfg_.volume_firmware[i]; ++i)
        vi.firmware[i] = cfg_.volume_firmware[i];
    for (size_t i = 0; i < sizeof(vi.wwn) - 1 && cfg_.volume_wwn[i]; ++i)
        vi.wwn[i] = cfg_.volume_wwn[i];
    for (size_t i = 0; i < sizeof(vi.interface_desc) - 1 && cfg_.volume_interface[i]; ++i)
        vi.interface_desc[i] = cfg_.volume_interface[i];
    if (vol_boot_snapshot_valid_)
        std::memcpy(vi.boot_snapshot, vol_boot_snapshot_, sizeof(vi.boot_snapshot));
    vi.scanned_bytes = stats.bytes_scanned;
    vi.elapsed_ms = stats.elapsed_ms;
    /* W1a v5 tail: the embedded save time (the og's record-2 SYSTEMTIME,
     * rsf-format section 5.2 +0x6C). Stamped at save - the session list
     * shows the recorded time, which survives copies unlike the mtime. */
    vi.save_unix = u64(std::time(nullptr));
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
        /* M-042 v2: the og's per-record geometry payload + flags */
        pc.bytes_per_sector = c.bytes_per_sector;
        pc.sectors_per_cluster = c.sectors_per_cluster;
        pc.hidden_sectors = c.hidden_sectors;
        pc.flags = 0;
        if (c.size_from_device_end)
            pc.flags |= 0x1;
        if (c.have_geometry)
            pc.flags |= 0x2;
        if (session::serialize_partition_context(pc, blob) == OK)
            if (u32 rc =
                    w.add_record(session::REC_PARTITION_CONTEXT, blob.data(), u32(blob.size())))
                return rc;
    }

    /* M-042: the whole-disk layout snapshot (the og's partinfo.dat
     * semantics) - captured at run() start, emitted with every
     * save/checkpoint so the next search can flag "layout changed".
     * Empty for volume-basis views (parse_disk_table found no table) and
     * for saves that never ran - honest emptiness, never fabricated. */
    if (!disk_layout_.empty()) {
        if (session::serialize_disk_layout(disk_layout_, blob) == OK)
            if (u32 rc = w.add_record(session::REC_DISK_LAYOUT, blob.data(), u32(blob.size())))
                return rc;
    }

    /* M-041: the damage-audit check report (the og's FileSystemCheck
     * error set). Optional record - emitted only when the check stage
     * ran and produced a report (detected family or findings); sessions
     * saved without the stage lack it and the report reads empty. */
    if (stats.check_ran &&
        (stats.check_report.family_detected || !stats.check_report.errors.empty())) {
        if (check::serialize_fs_check(stats.check_report, blob) == OK)
            if (u32 rc = w.add_record(session::REC_FS_CHECK, blob.data(), u32(blob.size())))
                return rc;
    }

    /* N-060: the tree record is the long pole on large sessions - the
     * progress/cancel hook rides it (per node; ERR_CANCELLED propagates
     * and the caller discards the partial container). */
    {
        const u32 rc = session::serialize_tree(tree_, blob, progress, progress_user);
        if (rc != OK)
            return rc;
        if (u32 rc2 = w.add_record(session::REC_RESULT_TREE, blob.data(), u32(blob.size())))
            return rc2;
    }

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

u32 ScanController::load_session(session::Reader& r, tree::ResultTree& tree, ScanStats& stats,
                                 IStorageDevice* revalidate_disk,
                                 session::DeserializeProgress progress, void* progress_user) {
    session::VolumeSessionInfo info;
    return load_session_summary(r, tree, stats, info, revalidate_disk, progress, progress_user);
}

u32 ScanController::read_summary(session::Reader& r, session::SessionSummary& out) {
    u32 count = 0;
    if (u32 rc = r.open(&count))
        return rc;
    out = session::SessionSummary{};
    out.record_count = count;
    bool id_seen = false;
    for (u32 i = 0; i < count; ++i) {
        session::RecordType type = session::REC_SESSION_STATE;
        u64 sid = 0;
        if (u32 rc = r.peek_record(i, type, sid))
            return rc;
        if (!id_seen) {
            out.session_id = sid;
            id_seen = true;
        }
        switch (type) {
        case session::REC_SESSION_STATE:
        case session::REC_VOLUME_INFO: {
            session::Record rec;
            u32 rc = r.read_record(i, rec);
            if (rc == ERR_BAD_CHECKSUM)
                continue; /* corrupt: skip, keep going (F10 - the load rule) */
            if (rc != OK)
                return rc;
            if (type == session::REC_SESSION_STATE) {
                if (session::deserialize_session_state(rec.data.data(), rec.data.size(),
                                                       out.state) == OK)
                    out.have_state = true;
            } else if (session::deserialize_volume_info(rec.data.data(), rec.data.size(),
                                                        out.volume) == OK) {
                out.have_volume = true;
            }
            break;
        }
        default:
            break; /* heavy/unknown types: header-skipped, never read */
        }
    }
    out.configured_mask =
        out.have_volume ? out.volume.flags : (STAGE_QUICK | STAGE_DEEP | STAGE_CARVE);
    return OK;
}

u32 ScanController::load_session_summary(session::Reader& r, tree::ResultTree& tree,
                                         ScanStats& stats, session::VolumeSessionInfo& info,
                                         IStorageDevice* revalidate_disk,
                                         session::DeserializeProgress progress,
                                         void* progress_user) {
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
        case session::REC_RESULT_TREE: {
            /* W1b: ERR_CANCELLED propagates (the per-node load hook's
             * abort) - never flattened into a format error; the caller
             * discards the partial tree. */
            const u32 rc2 = session::deserialize_tree(rec.data.data(), rec.data.size(), tree,
                                                      nullptr, progress, progress_user);
            if (rc2 == ERR_CANCELLED)
                return ERR_CANCELLED;
            if (rc2 != OK)
                return ERR_SESSION_FORMAT;
            break;
        }
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
                /* M-042 v2: the geometry payload rides back into the
                 * candidate (have_geometry from flags bit 1 - v1
                 * sessions have no geometry and stay classification-only
                 * re-validatable). */
                c.bytes_per_sector = pc.bytes_per_sector;
                c.sectors_per_cluster = pc.sectors_per_cluster;
                c.hidden_sectors = pc.hidden_sectors;
                c.size_from_device_end = (pc.flags & 0x1) != 0;
                c.have_geometry = (pc.flags & 0x2) != 0;
                /* M-042: the og re-reads every persisted record at search
                 * start and DROPS the ones the current disk no longer
                 * confirms (PRD:20748-20842). Two execution points: here
                 * (the device-bearing listing surfaces - revalidate_disk)
                 * and at run() start (the resume flows - see run()); both
                 * use the same lost::revalidate_candidate arbiter, and the
                 * bookkeeping lands in the same stats fields. A null
                 * revalidate_disk = a device-less listing: candidates
                 * restore as-is (the og has no device-less surface). */
                if (revalidate_disk != nullptr) {
                    ++stats.revalidated_candidates;
                    if (!lost::revalidate_candidate(revalidate_disk, c)) {
                        ++stats.dropped_stale_candidates;
                        break;
                    }
                }
                stats.lost_candidates.push_back(c);
            }
            break;
        }
        case session::REC_DISK_LAYOUT:
            /* M-042: the persisted whole-disk layout (the og's partinfo
             * snapshot). Consumed by the run-start compare that surfaces
             * stats.layout_changed; a session saved before the record
             * existed carries none and the flag just stays false. */
            (void)session::deserialize_disk_layout(rec.data.data(), rec.data.size(),
                                                   stats.persisted_layout);
            break;
        case session::REC_FS_CHECK:
            /* M-041: the persisted damage-audit report (codes 0x1001..
             * 0x100e with cluster/LCN/version params). Restored so a
             * reloaded session surfaces the audit findings without a
             * re-run; sessions saved before the record existed carry
             * none and the report reads empty. */
            stats.check_ran = true;
            (void)check::deserialize_fs_check(rec.data.data(), rec.data.size(), stats.check_report);
            stats.check_detected = stats.check_report.family_detected;
            break;
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
    /* W1a: the embedded save time rides the summary out (0 = pre-v5). */
    info.save_unix = vi.save_unix;
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
                           session::VolumeSessionInfo& info, std::unique_ptr<ScanController>& out,
                           IStorageDevice* revalidate_disk, session::DeserializeProgress progress,
                           void* progress_user) {
    out.reset();
    if (u32 rc =
            load_session_summary(r, tree, stats, info, revalidate_disk, progress, progress_user))
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
