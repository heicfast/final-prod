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
};

struct CarveFlags {
    bool hash_small_files = true; /* <= 8 MiB: compute 128-bit id at emit */
    u64 hash_threshold = 8ull << 20;
    u32 threads = 1; /* 1 = single-threaded; >1 = worker fan-out */
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
