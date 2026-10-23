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
    FMT_DOCX, FMT_ZIP, FMT_CR2, FMT_DNG, FMT_TIFF,   FMT_MOV, FMT_MP4, FMT_HEIC, FMT_JPG,  FMT_PNG,
    FMT_WEBP, FMT_WAV, FMT_PDF, FMT_7Z,  FMT_SQLITE, FMT_AVI, FMT_MKV, FMT_RAR5, FMT_RAR4, FMT_PST,
    FMT_ORF,  FMT_RAF, FMT_RW2, FMT_PSD, FMT_MP3,    FMT_M4A, FMT_ASF, FMT_FLV,  FMT_CAB,  FMT_ISO};
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
    std::atomic<u32> file_index{0};
    std::atomic<bool> aborted{false};
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
            if (ctx.dedup_tree && ctx.dedup_tree->find_by_extent(cand_abs, len_class))
                dup = true;
            if (!dup && ctx.tree->find_by_extent(cand_abs, len_class))
                dup = true;
            if (dup) {
                ctx.duplicates.fetch_add(1, std::memory_order_relaxed);
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
            props->extents.push_back({cand_abs, file_len});

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
    return OK;
}

/* Read + scan the window with index wi (base = wi * WINDOW). */
u32 scan_window_index(ScanCtx& ctx, u64 wi, std::set<u64>& emitted_at) {
    const u64 base = wi * WINDOW;
    const u64 avail = ctx.total - base;
    u64 want = avail < WINDOW + OVERLAP ? avail : WINDOW + OVERLAP;
    if (want < 16)
        return OK;
    std::vector<u8> win(static_cast<size_t>(want), 0);
    if (ctx.slice->read_at(base, win.data(), u32(want)) != OK) {
        ctx.bytes_scanned.fetch_add(want, std::memory_order_relaxed);
        return OK;
    }
    ctx.bytes_scanned.fetch_add(want, std::memory_order_relaxed);
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

    const u64 window_count = (ctx.total + WINDOW - 1) / WINDOW;
    const bool multi = flags.threads > 1 && window_count > 1 && ctx.total > 0;

    if (!multi) {
        /* single-threaded path: identical semantics to the original scan */
        std::set<u64> emitted_at;
        for (u64 wi = 0; wi < window_count; ++wi) {
            if (cancel && cancel->cancelled())
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
    return OK;
}

} // namespace carve
} // namespace cr
