/* cr_carve.cpp - carving pipeline (R4).
 *
 * Scan: offset-outer with a first-byte dispatch; per-hit formats are tried
 * in specificity order (DOCX before ZIP, CR2 before TIFF) and the first
 * grammar-validated format wins the candidate.
 *
 * Parallel mode (flags.threads > 1): the window-index space is fanned out
 * over a ThreadPool; emission serializes inside ResultTree; per-worker
 * overlap sets backstopped by the tree extent index keep F9 dedup exact
 * across worker boundaries.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/carve/cr_carve.h"
#include "cr/carve/cr_rawsigns.h"
#include "cr/cr_parallel.h"
#include "cr/crypto/cr_sha256.h"
#include <atomic>
#include <chrono>
#include <cstring>
#include <set>

namespace cr {
namespace carve {

namespace {
constexpr u64 WINDOW = 1ull << 20; /* 1 MiB */
constexpr u64 OVERLAP = 4096;      /* magic search continuity */
constexpr u64 MAX_FILE_READ_FOR_HASH = 8ull << 20;

/* trial order: most specific grammar first when magics coincide */
/* specificity-first: same-magic families order their most specific
 * member first (DOCX>ZIP, CR2>DNG>TIFF, RAR5>RAR4, M4A>MP4/MOV) */
constexpr u32 kTrialOrder[] = {
    FMT_DOCX,   FMT_ZIP, FMT_CR2,     FMT_DNG,      FMT_TIFF,     FMT_MOV,   FMT_MP4,
    FMT_HEIC,   FMT_JPG, FMT_PNG,     FMT_WEBP,     FMT_WAV,      FMT_PDF,   FMT_7Z,
    FMT_SQLITE, FMT_AVI, FMT_MKV,     FMT_RAR5,     FMT_RAR4,     FMT_PST,   FMT_ORF,
    FMT_RAF,    FMT_RW2, FMT_PSD,     FMT_MP3,      FMT_M4A,      FMT_ASF,   FMT_FLV,
    FMT_CAB,    FMT_ISO, FMT_TIFF_BE, FMT_ORF_IIRS, FMT_ORF_MMOR, FMT_ZIP_CD};
constexpr u32 kTrialCount = sizeof(kTrialOrder) / sizeof(kTrialOrder[0]);

/* Shared scan context (single-thread: one worker touches it; parallel:
 * counters are atomic, tree emission is internally locked). */
struct ScanCtx {
    IStorageDevice* slice;
    const Signature* reg;
    const CarveFlags* flags;
    tree::ResultTree* tree;
    tree::FullNode* carve_root;
    const tree::ResultTree* dedup_tree;
    NameCallback namer;
    u64 total;

    std::atomic<u64> hits{0};
    std::atomic<u64> validated{0};
    std::atomic<u64> rejected{0};
    std::atomic<u64> duplicates{0};
    std::atomic<u64> hashed{0};
    std::atomic<u64> bytes_scanned{0};
    std::atomic<u64> sign_hits{0};
    std::atomic<u64> sign_validated{0};
    std::atomic<u64> windows_skipped{0}; /* unreadable after retry (FB-C01) */
    std::atomic<u32> file_index{0};
    std::atomic<bool> aborted{false};
    /* GAP-S6-8: the disk-absolute basis for extents + dedup queries.
     * Carve positions are slice-relative (reads, bounds, emitted_at stay
     * relative); the TREE keys must be disk-absolute so the resumed carve
     * dedups against checkpoint-loaded nodes (which are absolute - F-1)
     * instead of re-emitting them as extra entries. Base 0 on synthetic /
     * whole-disk devices: behavior identical to the old code. */
    u64 ext_base = 0;
};

/* Scan ONE window [base, base+want) fully (magic dispatch, validation,
 * dedup, emit). Returns OK, or ERR_CANCELLED when aborted mid-way. */
u32 scan_one_window(ScanCtx& ctx, u64 base, const u8* win, u64 want, std::set<u64>& emitted_at) {
    for (u64 off = 0; off + 12 <= want; ++off) {
        if (ctx.aborted.load(std::memory_order_relaxed))
            return ERR_CANCELLED;
        const u8 c = win[off];
        /* first-byte dispatch: only formats whose magic starts with c */
        for (u32 ti = 0; ti < kTrialCount; ++ti) {
            const Signature& sig = ctx.reg[kTrialOrder[ti]];
            if (sig.header[0] != c)
                continue;
            if (off + sig.header_len > want)
                continue;
            if (std::memcmp(win + off, sig.header, sig.header_len) != 0)
                continue;
            /* magic found at window offset off: candidate file start */
            if (off < sig.header_offset)
                continue; /* window head clip */
            const u64 cand_abs = base + off - sig.header_offset;
            if (cand_abs + sig.min_size > ctx.total)
                continue;
            ctx.hits.fetch_add(1, std::memory_order_relaxed);

            /* try THIS format's grammar; first validated format at this
             * magic position wins (specificity order) */
            CarveResult cr = sig.validator(ctx.slice, cand_abs, sig.max_size);
            if (!cr.ok) {
                ctx.rejected.fetch_add(1, std::memory_order_relaxed);
                continue;
            }
            u64 file_len = cr.end - cand_abs;
            if (file_len < sig.min_size || file_len > sig.max_size) {
                ctx.rejected.fetch_add(1, std::memory_order_relaxed);
                continue;
            }
            if (emitted_at.count(cand_abs)) {
                ctx.duplicates.fetch_add(1, std::memory_order_relaxed);
                break;
            }

            /* F9 dedup vs structured results + prior carved nodes (the
             * locked extent index is the cross-worker backstop) */
            u64 len_class = 0;
            if (file_len >= (1 << 20)) {
                while ((1ULL << len_class) < file_len)
                    ++len_class;
                len_class += 64;
            } else {
                len_class = file_len / 65536;
            }
            bool dup = false;
            if (ctx.dedup_tree &&
                ctx.dedup_tree->find_by_extent(cand_abs + ctx.ext_base, len_class))
                dup = true;
            if (!dup && ctx.tree->find_by_extent(cand_abs + ctx.ext_base, len_class))
                dup = true;
            if (dup) {
                ctx.duplicates.fetch_add(1, std::memory_order_relaxed);
                emitted_at.insert(cand_abs); /* position resolved: a file lives here (the
                                                raw-sign layer must not re-probe it) */
                break;
            }

            tree::FileProperties* props = new (std::nothrow) tree::FileProperties();
            if (props == nullptr) {
                ctx.aborted.store(true, std::memory_order_relaxed);
                return ERR_NO_MEMORY;
            }
            const u32 idx = ctx.file_index.fetch_add(1, std::memory_order_relaxed);
            props->name =
                ctx.namer ? ctx.namer(sig, file_len, idx) : default_namer(sig, file_len, idx);
            props->fs_kind =
                u32(FsKindCode::Raw); /* carved (row 45: was invented 0xFF; factory RAW=6) */
            props->logical_size = file_len;
            props->carve_format = sig.id;
            props->carve_confidence = u16((sig.base_confidence + cr.confidence) / 2);
            if (props->carve_confidence > 1000)
                props->carve_confidence = 1000;
            /* GAP-S6-8: disk-absolute extent + the idempotency guard so the
             * stage-boundary rebase never double-shifts carved nodes */
            props->extents.push_back({cand_abs + ctx.ext_base, file_len});
            if (ctx.ext_base != 0)
                props->flags = u16(props->flags | tree::NODE_FLAG_EXT_SHIFTED);

            if (ctx.flags->hash_small_files && file_len <= ctx.flags->hash_threshold &&
                file_len <= MAX_FILE_READ_FOR_HASH) {
                std::vector<u8> content((size_t(file_len)));
                if (ctx.slice->read_at(cand_abs, content.data(), u32(file_len)) == OK) {
                    u8 full[32];
                    crypto::Sha256::digest(content.data(), size_t(file_len), full);
                    std::memcpy(props->sha128, full, 16);
                    props->hash_valid = true;
                    ctx.hashed.fetch_add(1, std::memory_order_relaxed);
                }
            }
            tree::FileNode* n = ctx.tree->add_file(ctx.carve_root, props);
            props->release();
            if (n == nullptr) {
                ctx.aborted.store(true, std::memory_order_relaxed);
                return ERR_NO_MEMORY;
            }
            n->set_origin(tree::Origin::Carved);
            n->set_confidence(cr.confidence >= 850 ? tree::Confidence::Excellent
                                                   : tree::Confidence::PartialRisk);
            ctx.validated.fetch_add(1, std::memory_order_relaxed);
            emitted_at.insert(cand_abs);
            break; /* this magic position resolved */
        }
    }

    /* ---- raw-sign layer 1 (audit row 32): the og's 139-format sign
     * table, probed at 512-byte strides. Grammar hits own their start
     * positions (emitted_at); the extent index dedups the rest. ---- */
    {
        const rawsigns::SignIndex& idx = rawsigns::SignIndex::get();
        std::vector<const rawsigns::RawSign*> matches;
        for (u64 off = 0; off + 512 <= want; off += 512) {
            if (ctx.aborted.load(std::memory_order_relaxed))
                return ERR_CANCELLED;
            const u64 start_abs = base + off;
            if (emitted_at.count(start_abs))
                continue; /* a grammar-validated file starts here */
            idx.probe_header(win + off, want - off, matches);
            if (matches.empty())
                continue;
            /* highest-confidence hit drives the candidate */
            const rawsigns::RawSign* best = matches[0];
            for (const auto* m : matches)
                if (m->conf > best->conf)
                    best = m;
            const rawsigns::RawSignFormat& fmt = idx.format(best->fmt);
            u64 len = fmt.max_size;
            u16 conf = best->conf;
            bool tail_paired = false;
            /* tail pairing: bounded search inside [start+min, start+max) */
            if (fmt.min_size < fmt.max_size && start_abs + fmt.min_size < ctx.total) {
                const u64 lo = fmt.min_size;
                const u64 hi =
                    fmt.max_size < ctx.total - start_abs ? fmt.max_size : ctx.total - start_abs;
                if (hi > lo && hi - lo <= (2ull << 20)) { /* bounded (<=2 MiB) */
                    std::vector<u8> tail_buf(size_t(hi - lo), 0);
                    if (ctx.slice->read_at(start_abs + lo, tail_buf.data(), u32(tail_buf.size())) ==
                        OK) {
                        rawsigns::SignIndex::TailHit th;
                        if (idx.scan_tail(tail_buf.data(), tail_buf.size(), th)) {
                            len = lo + th.pos + 512;
                            conf = u16((u32(best->conf) + u32(th.sign->conf)) / 2 + 100);
                            if (conf > 1000)
                                conf = 1000;
                            tail_paired = true;
                        }
                    }
                }
            }
            if (len > ctx.total - start_abs)
                len = ctx.total - start_abs;
            if (len < fmt.min_size)
                continue; /* device too small for the format's floor */
            ctx.sign_hits.fetch_add(1, std::memory_order_relaxed);
            if (tail_paired)
                ctx.sign_validated.fetch_add(1, std::memory_order_relaxed);

            /* F9 dedup (extent class) against structured + carved */
            u64 len_class = 0;
            if (len >= (1 << 20)) {
                while ((1ULL << len_class) < len)
                    ++len_class;
                len_class += 64;
            } else {
                len_class = len / 65536;
            }
            bool dup = false;
            if (ctx.dedup_tree &&
                ctx.dedup_tree->find_by_extent(start_abs + ctx.ext_base, len_class))
                dup = true;
            if (!dup && ctx.tree->find_by_extent(start_abs + ctx.ext_base, len_class))
                dup = true;
            if (dup) {
                ctx.duplicates.fetch_add(1, std::memory_order_relaxed);
                continue;
            }

            tree::FileProperties* props = new (std::nothrow) tree::FileProperties();
            if (props == nullptr) {
                ctx.aborted.store(true, std::memory_order_relaxed);
                return ERR_NO_MEMORY;
            }
            const u32 fi = ctx.file_index.fetch_add(1, std::memory_order_relaxed);
            char nbuf[40];
            snprintf(nbuf, sizeof(nbuf), "File%06u.%s", fi, fmt.ext);
            std::u16string uname;
            for (const char* p = nbuf; *p; ++p)
                uname.push_back(u16(u8(*p)));
            props->name = uname;
            props->fs_kind = u32(FsKindCode::Raw);
            props->logical_size = len;
            props->carve_format = 0xFFFF0000u + best->fmt; /* raw-sign tag */
            props->carve_confidence = conf;
            /* GAP-S6-8: disk-absolute extent + the idempotency guard (see
             * the grammar layer above) */
            props->extents.push_back({start_abs + ctx.ext_base, len});
            if (ctx.ext_base != 0)
                props->flags = u16(props->flags | tree::NODE_FLAG_EXT_SHIFTED);
            if (ctx.flags->hash_small_files && len <= ctx.flags->hash_threshold &&
                len <= MAX_FILE_READ_FOR_HASH) {
                std::vector<u8> content((size_t(len)));
                if (ctx.slice->read_at(start_abs, content.data(), u32(len)) == OK) {
                    u8 full[32];
                    crypto::Sha256::digest(content.data(), size_t(len), full);
                    std::memcpy(props->sha128, full, 16);
                    props->hash_valid = true;
                    ctx.hashed.fetch_add(1, std::memory_order_relaxed);
                }
            }
            tree::FileNode* n = ctx.tree->add_file(ctx.carve_root, props);
            props->release();
            if (n == nullptr) {
                ctx.aborted.store(true, std::memory_order_relaxed);
                return ERR_NO_MEMORY;
            }
            n->set_origin(tree::Origin::Carved);
            n->set_confidence(tail_paired ? tree::Confidence::PartialRisk
                                          : tree::Confidence::MetadataOnly);
            emitted_at.insert(start_abs);
        }
    }
    return OK;
}

/* Read + scan the window with index wi (base = wi * WINDOW). */
u32 scan_window_index(ScanCtx& ctx, u64 wi, std::set<u64>& emitted_at) {
    const u64 base = wi * WINDOW;
    const u64 avail = ctx.total - base;
    u64 want = avail < WINDOW + OVERLAP ? avail : WINDOW + OVERLAP;
    if (want < 16)
        return OK;
    /* honest fill-rate accounting (G-05): the OVERLAP tail re-reads the
     * next window's head, so attribute only the window's OWN range
     * [base, base+WINDOW) - bytes_scanned must never exceed the device.
     * The last window's avail is always <= WINDOW (window_count =
     * ceil(total/WINDOW)), so the sum of attributed ranges == total. */
    const u64 counted = want < WINDOW ? want : WINDOW;
    std::vector<u8> win(static_cast<size_t>(want), 0);
    if (ctx.slice->read_at(base, win.data(), u32(want)) != OK) {
        /* Unreadable window: skipped, NOT counted as scanned - the honest
         * fill-rate contract (G-05) forbids claiming coverage of bytes
         * that were never read; the retry pipe's error map already
         * carries the failure (FB-C01). */
        ctx.windows_skipped.fetch_add(1, std::memory_order_relaxed);
        return OK;
    }
    ctx.bytes_scanned.fetch_add(counted, std::memory_order_relaxed);
    return scan_one_window(ctx, base, win.data(), want, emitted_at);
}

} // namespace

std::u16string default_namer(const Signature& sig, u64 size, u32 index) {
    (void)size;
    char buf[32];
    /* File000001.jpg - zero-padded for stable ordering (corpus policy) */
    int n = snprintf(buf, sizeof(buf), "File%06u.%s", index, sig.ext);
    if (n <= 0)
        buf[0] = 0;
    std::u16string out;
    for (const char* p = buf; *p; ++p)
        out.push_back(u16(u8(*p)));
    return out;
}

u32 carve_slice(IStorageDevice* slice, const CarveFlags& flags, CancelToken* cancel,
                bool (*progress)(u64, u64, void*), void* progress_user, tree::ResultTree& tree,
                tree::FullNode* carve_root, const tree::ResultTree* dedup_tree,
                const NameCallback& namer, CarveStats& stats) {
    if (slice == nullptr || carve_root == nullptr)
        return ERR_BAD_PARAMETER;
    if (validate_registry(nullptr) != 0)
        return ERR_BAD_PARAMETER; /* F8 gate */

    ScanCtx ctx;
    ctx.slice = slice;
    ctx.reg = registry();
    ctx.flags = &flags;
    ctx.tree = &tree;
    ctx.carve_root = carve_root;
    ctx.dedup_tree = dedup_tree;
    ctx.namer = namer;
    ctx.total = slice->size();
    ctx.ext_base = slice->absolute_base(); /* GAP-S6-8: disk-absolute basis */

    const u64 window_count = (ctx.total + WINDOW - 1) / WINDOW;
    const bool multi = flags.threads > 1 && window_count > 1 && ctx.total > 0;

    if (!multi) {
        /* single-threaded path: identical semantics to the original scan */
        std::set<u64> emitted_at;
        for (u64 wi = 0; wi < window_count; ++wi) {
            if (cancel && cancel->checkpoint())
                return ERR_CANCELLED;
            if (progress && !progress(wi * WINDOW, ctx.total, progress_user))
                return ERR_CANCELLED;
            if (u32 rc = scan_window_index(ctx, wi, emitted_at))
                return rc;
        }
        if (progress)
            (void)progress(ctx.total, ctx.total, progress_user);
    } else {
        /* parallel path: fan window indices out over a bounded pool */
        ThreadPool pool(flags.threads);
        PoolCancelToken pool_cancel(&pool);
        std::atomic<u64> windows_done{0};
        const u64 total = ctx.total;

        /* external cancel propagates into the pool */
        struct WatchState {
            CancelToken* external;
            ThreadPool* pool;
        } watch_state{cancel, &pool};

        auto worker_job = [&](u64 wi) -> u32 {
            /* pause-aware gate (G-07): a PausableToken freezes carve workers
             * here; plain tokens behave exactly like the old poll. */
            if (cancel && cancel->checkpoint())
                return ERR_CANCELLED;
            if (pool.aborted())
                return ERR_CANCELLED;
            std::set<u64> emitted_at; /* per-worker overlap set */
            u32 rc = scan_window_index(ctx, wi, emitted_at);
            const u64 done = windows_done.fetch_add(1, std::memory_order_relaxed) + 1;
            if (progress &&
                !progress(done * WINDOW > total ? total : done * WINDOW, total, progress_user)) {
                pool.abort();
                return ERR_CANCELLED;
            }
            return rc;
        };

        /* main-thread watch loop: submit all windows, watch external cancel */
        u32 first_err = OK;
        if (cancel) {
            /* poll cancel while submitting/running */
            std::atomic<bool> submit_done{false};
            std::thread watcher([&watch_state, &submit_done]() {
                while (!submit_done.load(std::memory_order_acquire)) {
                    if (watch_state.external->cancelled()) {
                        watch_state.pool->abort();
                        return;
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                }
            });
            first_err = parallel_for(pool, window_count, worker_job);
            submit_done.store(true, std::memory_order_release);
            watcher.join();
            if (cancel->cancelled() && first_err == OK)
                first_err = ERR_CANCELLED;
        } else {
            first_err = parallel_for(pool, window_count, worker_job);
        }
        if (first_err != OK)
            return first_err;
        if (progress)
            (void)progress(total, total, progress_user);
    }

    stats.hits += ctx.hits.load();
    stats.validated += ctx.validated.load();
    stats.rejected += ctx.rejected.load();
    stats.duplicates += ctx.duplicates.load();
    stats.hashed += ctx.hashed.load();
    stats.bytes_scanned += ctx.bytes_scanned.load();
    /* PARITY-AUDIT F-A3-6: sign_hits/sign_validated were each added TWICE
     * (copy-paste duplicate lines) — every raw-sign statistic reported
     * double its true value. */
    stats.sign_hits += ctx.sign_hits.load();
    stats.sign_validated += ctx.sign_validated.load();
    stats.windows_skipped += ctx.windows_skipped.load();
    return OK;
}

} // namespace carve
} // namespace cr
