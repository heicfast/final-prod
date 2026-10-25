/*
 * cr_carve.h - the carving engine (R4).
 *
 * Pipeline: 1 MiB sequential windows (+4 KiB overlap), per-format magic
 * search, grammar validation, dedup against structured results (F9),
 * content hashing (128-bit id), host-provided naming callback (corpus:
 * naming lives in the host, not the engine).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_CARVE_H_
#define CR_CARVE_H_

#include "../storage/cr_device.h"
#include "../storage/cr_freemap.h"
#include "../tree/cr_node.h"
#include "cr_signatures.h"
#include <functional>
#include <vector>

namespace cr {
namespace carve {

struct CarveStats {
    u64 hits = 0;       /* magic candidates */
    u64 validated = 0;  /* grammar-confirmed */
    u64 rejected = 0;   /* header-only noise (F8 guard) */
    u64 duplicates = 0; /* deduped vs structured/carved */
    u64 hashed = 0;
    u64 bytes_scanned = 0;
    u64 sign_hits = 0;       /* raw-sign layer-1 candidates (row 32) */
    u64 sign_validated = 0;  /* header+tail paired hits */
    u64 windows_skipped = 0; /* unreadable windows (post-retry); their bytes
                                are NOT in bytes_scanned (FB-C01 honesty) */
    /* M-005: allocation-bitmap gating (the og's block-scan cluster gate,
     * RAWSearchDLL FUN_18000c820 :9446-9460 + FUN_180058df0 :67842-67856,
     * corpus-verified first-hand). Counted at the GRAMMAR layer (the
     * per-byte dispatch dominates cost); the raw-sign layer's suppression
     * rides the same map but does not double-count. */
    u64 bitmap_skipped_clusters = 0;
    u64 bitmap_skipped_bytes = 0; /* skipped_clusters x cluster_bytes (0 when
                                     no map was supplied) */
};

struct CarveFlags {
    bool hash_small_files = true; /* <= 8 MiB: compute 128-bit id at emit */
    u64 hash_threshold = 8ull << 20;
    u32 threads = 1; /* 1 = single-threaded; >1 = worker fan-out */
    /* M-005: allocation-bitmap gate (nullable). Bit set = cluster FREE
     * (the S-009 FreeMap, inverted from the og's USED bitmap). With a
     * valid map, candidate positions whose cluster is USED are
     * suppressed from dispatch, cluster-aligned hits skip the whole
     * cluster (the og's local_1e0 += spc-1); null/invalid map -> carve
     * everything (raw disks unchanged). SLICE-RELATIVE basis (the
     * orchestrator rebases stats.free_map from the absolute basis). */
    const storage::FreeMap* free_map = nullptr;
};

/* Naming callback: returns the display name for a carved hit (host policy). */
using NameCallback = std::function<std::u16string(const Signature& sig, u64 size, u32 index)>;

/* Default namer: File0001.jpg style (corpus default policy). */
std::u16string default_namer(const Signature& sig, u64 size, u32 index);

/*
 * carve_slice - scan one slice (a partition or whole disk view).
 *   dedup_tree: existing structured results - carved duplicates are skipped
 *   (F9: a carved JPEG never appears twice).
 *
 * Parallel mode (flags.threads > 1): window-index space is fanned out over
 * the caller-provided worker count; the ResultTree serializes emission;
 * per-worker overlap sets + the tree extent index backstop the window-boundary
 * dedup. Progress stays monotonic (completed-window counter).
 */
u32 carve_slice(IStorageDevice* slice, const CarveFlags& flags, CancelToken* cancel,
                bool (*progress)(u64, u64, void*), void* progress_user, tree::ResultTree& tree,
                tree::FullNode* carve_root, const tree::ResultTree* dedup_tree,
                const NameCallback& namer, CarveStats& stats);

} // namespace carve
} // namespace cr

#endif // CR_CARVE_H_
