/* cr_scan.cpp - the staged scan orchestrator.
 *
 * Threading model (R6, mirrors the corpus: engines single-threaded per unit,
 * the HOST fans units out): one structured engine pass (quick and deep share
 * it - the old code walked the MFT twice and double-emitted FAT), deep adds
 * the remnant sweep + parallel lost-partition candidate scans, carve fans
 * window ranges out over the pool. Engine progress callbacks are wired into
 * the weighted overall progress (fine-grained, monotonic).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/orchestrator/cr_scan.h"
#include "cr/carve/cr_carve.h"
#include "cr/cr_parallel.h"
#include "cr/exfat/cr_exfat_engine.h"
#include "cr/fat/cr_fat_engine.h"
#include "cr/lost/cr_lostpart.h"
#include "cr/ntfs/cr_bootstrap.h"
#include "cr/ntfs/cr_bitmap.h"
#include "cr/ntfs/cr_mftwalk.h"
#include "cr/session/cr_crsf.h"
#include "cr/storage/cr_slice.h"
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
    if (cfg_.run_quick || cfg_.run_deep) {
        root_found_ = tree_.add_root(u"Found");
        root_deleted_ = tree_.add_root(u"Deleted");
        root_lost_ = tree_.add_root(u"Lost Files");
    }
    if (cfg_.run_carve)
        root_carved_ = tree_.add_root(u"Other Deleted Files (Carved)");
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
    /* FAT: bytes-per-sector sane + 55AA */
    if (sec[510] == 0x55 && sec[511] == 0xAA) {
        const u16 bps = u16_from_bytes_le(sec + 0x0B);
        if (bps >= 512 && bps <= 4096 && (bps & (bps - 1)) == 0)
            return FsFamily::Fat;
    }
    return FsFamily::Unknown;
}

u32 ScanController::run(IStorageDevice* slice, CancelToken* cancel, bool (*progress)(u32, void*),
                        void* progress_user, ScanStats& stats) {
    if (slice == nullptr)
        return ERR_BAD_PARAMETER;
    const auto t0 = std::chrono::steady_clock::now();
    if (cfg_.fs_hint == FsFamily::Unknown && (cfg_.run_quick || cfg_.run_deep))
        cfg_.fs_hint = probe_family(slice);

    const auto stamp = [&stats, t0](u32 rc) -> u32 {
        const auto t1 = std::chrono::steady_clock::now();
        stats.elapsed_ms =
            u64(std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count());
        return rc;
    };
    if (cfg_.run_quick) {
        if (u32 rc = run_quick(slice, cancel, progress, progress_user, stats))
            return stamp(rc);
    }
    if (cfg_.run_deep) {
        if (u32 rc = run_deep(slice, cancel, progress, progress_user, stats))
            return stamp(rc);
    }
    if (cfg_.run_carve) {
        if (u32 rc = run_carve_stage(slice, cancel, progress, progress_user, stats))
            return stamp(rc);
    }
    tree_.refresh_stats();
    return stamp(OK);
}

u32 ScanController::run_quick(IStorageDevice* slice, CancelToken* cancel,
                              bool (*progress)(u32, void*), void* pu, ScanStats& stats) {
    /* One engine pass, quick semantics. When deep will also run, the pass
     * uses deep settings already (single walk - the corpus engines emit
     * active+deleted in one MFT walk; re-walking was pure waste). */
    const bool deep_coming = cfg_.run_deep;
    u32 rc = run_engine_pass(slice, deep_coming, cancel, progress, pu, stats);
    if (rc == OK)
        quick_done_ = true;
    if (progress)
        (void)progress(weighted_progress(cfg_.weights, 1000, 0, 0), pu);
    return rc;
}

u32 ScanController::run_engine_pass(IStorageDevice* slice, bool deep_mode, CancelToken* cancel,
                                    bool (*progress)(u32, void*), void* pu, ScanStats& stats) {
    /* Engine-internal progress feeds the quick-stage permille via a
     * stack-context trampoline (raw function pointers take user data). */
    struct EngineProgress {
        bool (*overall)(u32, void*);
        void* user;
        u32 quick_weight;
        u32 total_weight;
    } ep{progress, pu, cfg_.weights.quick,
       cfg_.weights.quick + cfg_.weights.deep + cfg_.weights.carve};

    ntfs::WalkProgress ntfs_cb = nullptr;
    void* ntfs_cb_user = nullptr;
    if (progress && ep.total_weight > 0) {
        ntfs_cb_user = &ep;
        ntfs_cb = [](u64 done, u64 total, void* user) -> bool {
            auto* p = static_cast<EngineProgress*>(user);
            const u32 pm = total ? u32(done * 1000 / total) : 1000;
            const u32 overall = u32(u64(p->quick_weight) * pm / p->total_weight);
            return p->overall ? p->overall(overall > 1000 ? 1000u : overall, p->user) : true;
        };
    }

    u32 rc = OK;
    switch (cfg_.fs_hint) {
    case FsFamily::Ntfs: {
        auto vol = ntfs::NtfsVolume::open(
            std::unique_ptr<IStorageDevice>(cr::SliceShim::shim(slice)), &rc);
        if (!vol)
            return rc == OK ? ERR_OPEN_FAILED : rc;
        std::unique_ptr<ntfs::VolumeBitmap> bm;
        (void)ntfs::VolumeBitmap::load(vol.get(), bm);
        ntfs::WalkerFlags wf;
        ntfs::WalkStats ws;
        rc = ntfs::walk_mft(vol.get(), bm.get(), wf, cancel, ntfs_cb, ntfs_cb_user, tree_,
                            root_found_, root_deleted_, root_lost_, ws);
        stats.files_found += ws.active;
        stats.deleted_found += ws.deleted;
        break;
    }
    case FsFamily::Fat: {
        fat::FatScanFlags fl;
        fl.reconstruct_deleted_chains = deep_mode; /* complete behavior when deep runs */
        fat::FatScanStats fs;
        rc = fat::scan_fat(slice, fl, cancel, nullptr, nullptr, tree_, root_found_, root_deleted_,
                           root_lost_, fs);
        stats.files_found += fs.files;
        stats.deleted_found += fs.deleted_entries;
        break;
    }
    case FsFamily::Exfat: {
        exfat::ExfatScanFlags fl;
        exfat::ExfatScanStats es;
        rc = exfat::scan_exfat(slice, fl, cancel, nullptr, nullptr, tree_, root_found_,
                               root_deleted_, root_lost_, es);
        stats.files_found += es.files;
        stats.deleted_found += es.deleted_entries;
        break;
    }
    default:
        rc = OK; /* unknown FS: quick yields nothing; deep/carve still run */
        break;
    }
    return rc;
}

u32 ScanController::run_deep(IStorageDevice* slice, CancelToken* cancel,
                             bool (*progress)(u32, void*), void* pu, ScanStats& stats) {
    u32 rc = OK;
    const u32 threads = resolve_threads(cfg_.threads);

    /* Deep extras. The engine pass itself already ran inside quick (single
     * walk) - here we only do what quick did NOT: the remnant sweep and the
     * lost-partition candidates. When quick was disabled, run the engine
     * pass now with full deep semantics. */
    if (!quick_done_) {
        if (u32 e = run_engine_pass(slice, true, cancel, progress, pu, stats))
            return e;
    }

    if (cfg_.fs_hint == FsFamily::Ntfs) {
        /* remnant sweep (R2.5) - parallel chunk fan-out */
        u32 rc2 = OK;
        {
            auto vol = ntfs::NtfsVolume::open(
                std::unique_ptr<IStorageDevice>(cr::SliceShim::shim(slice)), &rc2);
            if (vol) {
                ntfs::WalkStats rem;
                rc = ntfs::scan_mft_remnants(slice, vol->params().sector_bytes,
                                             vol->params().cluster_bytes,
                                             vol->params().record_size, cancel, nullptr, nullptr,
                                             tree_, root_lost_, rem, threads);
                stats.files_found += rem.emitted;
            }
        }
    }

    /* lost-partition discovery (whole-disk mode, R3) */
    if (cfg_.whole_disk) {
        lost::DiscoveryResult dr;
        u32 lrc = lost::discover(slice, {}, 1, cancel, nullptr, nullptr, dr);
        if (lrc == OK) {
            stats.lost_candidates = dr.candidates;
            stats.lost_partitions = dr.candidates.size();
            if (!dr.candidates.empty()) {
                /* Pre-create roots and slices on THIS thread (tree + slice
                 * construction is cheap), then fan the volume scans out. */
                struct CandWork {
                    std::unique_ptr<IStorageDevice> slice;
                    tree::FullNode* root;
                };
                std::vector<CandWork> work;
                for (const auto& c : dr.candidates) {
                    auto cand_slice = lost::materialize(slice, c);
                    if (!cand_slice)
                        continue;
                    std::u16string role = u"Candidate @";
                    char buf[32];
                    snprintf(buf, sizeof(buf), " %llu", (unsigned long long)c.start_lba);
                    for (const char* p = buf; *p; ++p)
                        role.push_back(u16(u8(*p)));
                    work.push_back({std::move(cand_slice), tree_.add_root(role)});
                }
                if (!work.empty()) {
                    if (threads > 1 && work.size() > 1) {
                        /* Per-candidate stats (workers never touch shared
                         * plain counters); merged on the caller thread. */
                        struct Out {
                            u64 files = 0;
                            u32 rc = OK;
                        };
                        std::vector<Out> outs(work.size());
                        CandWork* items = work.data();
                        Out* out_arr = outs.data();
                        const u64 n = work.size();
                        ThreadPool pool(threads);
                        PoolCancelToken pool_cancel(&pool);
                        u32 first_err = parallel_for(pool, n, [&](u64 i) -> u32 {
                            out_arr[i].rc =
                                scan_candidate(items[i].slice.get(), items[i].root, cancel,
                                               out_arr[i].files);
                            return OK; /* candidate misses never abort the scan */
                        });
                        for (const auto& o : outs) {
                            stats.files_found += o.files;
                            (void)o.rc;
                        }
                        if (first_err != OK)
                            rc = first_err;
                    } else {
                        for (auto& w : work) {
                            u64 files = 0;
                            (void)scan_candidate(w.slice.get(), w.root, cancel, files);
                            stats.files_found += files;
                        }
                    }
                }
            }
        }
    }
    if (progress)
        (void)progress(weighted_progress(cfg_.weights, 1000, 1000, 0), pu);
    return rc;
}

u32 ScanController::scan_candidate(IStorageDevice* cand_slice, tree::FullNode* croot,
                                   CancelToken* cancel, u64& files_out) {
    FsFamily fam = probe_family(cand_slice);
    u32 rc = OK;
    files_out = 0;
    if (fam == FsFamily::Ntfs) {
        auto vol = ntfs::NtfsVolume::open(
            std::unique_ptr<IStorageDevice>(cr::SliceShim::shim(cand_slice)), &rc);
        if (vol) {
            std::unique_ptr<ntfs::VolumeBitmap> bm;
            (void)ntfs::VolumeBitmap::load(vol.get(), bm);
            ntfs::WalkerFlags wf;
            ntfs::WalkStats ws;
            (void)ntfs::walk_mft(vol.get(), bm.get(), wf, cancel, nullptr, nullptr, tree_, croot,
                                 croot, croot, ws);
            files_out += ws.emitted;
        }
        /* open failure = not a viable NTFS volume: not a scan error */
        rc = OK;
    } else if (fam == FsFamily::Fat) {
        fat::FatScanFlags fl;
        fat::FatScanStats fs;
        (void)fat::scan_fat(cand_slice, fl, cancel, nullptr, nullptr, tree_, croot, croot, croot,
                            fs);
        files_out += fs.files;
        rc = OK;
    } else if (fam == FsFamily::Exfat) {
        exfat::ExfatScanFlags fl;
        exfat::ExfatScanStats es;
        (void)exfat::scan_exfat(cand_slice, fl, cancel, nullptr, nullptr, tree_, croot, croot,
                                croot, es);
        files_out += es.files;
        rc = OK;
    }
    return rc;
}

u32 ScanController::run_carve_stage(IStorageDevice* slice, CancelToken* cancel,
                                    bool (*progress)(u32, void*), void* pu, ScanStats& stats) {
    const u32 threads = resolve_threads(cfg_.threads);
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
    u32 rc = carve::carve_slice(slice, cf, cancel, nullptr, nullptr, tree_, root_carved_, &tree_,
                                namer, cs);
    stats.carved_found = cs.validated;
    stats.bytes_scanned += cs.bytes_scanned;
    if (progress)
        (void)progress(weighted_progress(cfg_.weights, 1000, 1000, 1000), pu);
    return rc;
}

u32 ScanController::save_session(session::Writer& w, const ScanStats& stats) const {
    /* record order: session state, volume info, partition contexts, tree,
     * error map (the writer chains them; ids from the container) */
    session::SessionState st;
    st.stage_done_mask =
        (cfg_.run_quick ? 1 : 0) | (cfg_.run_deep ? 2 : 0) | (cfg_.run_carve ? 4 : 0);
    st.files_found = stats.files_found;
    st.bytes_scanned = stats.bytes_scanned;
    st.error_map_ranges = u32(stats.errors.ranges().size());
    std::vector<u8> blob;
    if (session::serialize_session_state(st, blob) == OK)
        if (u32 rc = w.add_record(session::REC_SESSION_STATE, blob.data(), u32(blob.size())))
            return rc;

    session::VolumeInfo vi;
    vi.fs_kind = u32(cfg_.fs_hint);
    vi.flags = st.stage_done_mask;
    vi.scanned_bytes = stats.bytes_scanned;
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
            if (u32 rc = w.add_record(session::REC_PARTITION_CONTEXT, blob.data(), u32(blob.size())))
                return rc;
    }

    if (session::serialize_tree(tree_, blob) == OK)
        if (u32 rc = w.add_record(session::REC_RESULT_TREE, blob.data(), u32(blob.size())))
            return rc;

    if (session::serialize_error_map(stats.errors, blob) == OK)
        if (u32 rc = w.add_record(session::REC_ERROR_MAP, blob.data(), u32(blob.size())))
            return rc;
    return OK;
}

u32 ScanController::load_session(session::Reader& r, tree::ResultTree& tree, ScanStats& stats) {
    u32 count = 0;
    if (u32 rc = r.open(&count))
        return rc;
    for (u32 i = 0; i < count; ++i) {
        session::Record rec;
        u32 rc = r.read_record(i, rec);
        if (rc == 0x50030002u)
            continue; /* corrupt: skip, keep going (F10) */
        if (rc != OK)
            return rc;
        switch (rec.type) {
        case session::REC_RESULT_TREE:
            if (session::deserialize_tree(rec.data.data(), rec.data.size(), tree) != OK)
                return 0x50040001;
            break;
        case session::REC_SESSION_STATE: {
            session::SessionState st;
            if (session::deserialize_session_state(rec.data.data(), rec.data.size(), st) == OK)
                stats.files_found = st.files_found;
            break;
        }
        case session::REC_ERROR_MAP:
            (void)session::deserialize_error_map(rec.data.data(), rec.data.size(), stats.errors);
            break;
        default:
            break;
        }
    }
    tree.refresh_stats();
    return OK;
}

} // namespace orchestrator
} // namespace cr
