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

class CarvePostProcessor; /* M-021: defined below (CarveFlags binds it) */

struct CarveStats {
    u64 hits = 0;       /* magic candidates */
    u64 validated = 0;  /* grammar-confirmed */
    u64 rejected = 0;   /* header-only noise (F8 guard) */
    u64 duplicates = 0; /* deduped vs structured/carved */
    u64 hashed = 0;
    u64 bytes_scanned = 0;
    /* M-039: positions suppressed by the occupancy interval gate (the
     * cross-engine FS-accounted-extent skip; both layers count). */
    u64 occupancy_skipped = 0;
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
    /* N-031: the og's all-zero-sector fast-skip (leading/trailing
     * zero-dword metrics feed its block-scan skip, :9490-9520). Counted
     * at the raw-sign layer: strides whose sign window (first 47 bytes
     * - the whole probe_header span) is all zero skip the 47-lookup
     * sign-table walk. The grammar layer's first-byte dispatch is
     * near-free on zeros and keeps running (the finding's own profile
     * rule). */
    u64 zero_strides_skipped = 0;
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
    /* M-039 (diff-to-fix): the cross-engine occupancy gate - the og's
     * per-volume bitmap every FS engine fills from its walk (EXT4
     * FUN_180012040 :14892-93 device-sized init :2763 fill-from-every-
     * extent :6866-6869; Btrfs per-node :2547-2551; Zfs :40576/:40599),
     * available to the raw phase to skip FS-accounted regions. The
     * core's shape (the finding's fix plan step 3: "interval set
     * first"): a sorted, coalesced [start, end) interval vector of
     * every STRUCTURED node's extents (slice-relative), built by the
     * orchestrator from the result tree after the engine passes. A
     * candidate position inside any interval is suppressed from
     * dispatch (the interval end is the jump target at the grammar
     * layer, like the M-005 cluster jump). null/empty -> no gate (the
     * og's no-bitmap behavior). The dedup tree stays as the safety
     * net (the finding's own requirement). */
    const std::vector<std::pair<u64, u64>>* occupied_ranges = nullptr;
    /* M-024 (diff-to-fix): runtime per-format enable/disable — the og's
     * RawConfig.ini [Filter] + IAnalyserFilter registry rebuild
     * (RAWSearchDLL FUN_180006b30 :4386-4434; 37 named toggles, STL/VPJ
     * ship default-off). A comma/space/semicolon-separated list of
     * extensions or display names (case-insensitive; "jpg,JPEG" or
     * "HEIC WEBM"); both carve layers (grammar + raw-sign) skip listed
     * formats. null/empty = every format enabled (the default; the
     * core ships no format off - the og's STL/VPJ defaults are a host
     * policy, not an engine constant, and are documented in
     * PARITY-DELTAS). */
    const char* disabled_formats = nullptr;
    /* M-021: the optional post-carve processing hook (the og's
     * EUVideoRepair binding at controller+0x158). null = unbound: raw
     * emission, exactly the og's DLL-absent degrade. See
     * CarvePostProcessor below. */
    CarvePostProcessor* post_processor = nullptr;
};

/* ---- M-021: the post-carve processing hook -------------------------
 *
 * The reference binds its video-repair module at searcher init:
 * LoadLibraryA("...\EUVideoRepair.dll") -> GetProcAddress("GetIMovMp4
 * Repair") -> the interface pointer stored at controller+0x158
 * (RAWSearchDLL FUN_18000beb0 :9210-9237, corpus-verified first-hand).
 * The load is OPTIONAL - module absent -> null -> the pipeline runs
 * unchanged. At the deferred-analysis poll, every FINALIZED candidate is
 * fed to the binding when present: GetVideoInfo (candidate vtable+0x18,
 * the stream-geometry probe that returns false for non-videos) and, on
 * success, the repair check (interface vtable+0x48) with the geometry
 * (FUN_18000d670 :10130-10136).
 *
 * The core's equivalent: a versioned interface the host binds through
 * CarveFlags::post_processor. The pipeline invokes on_candidate() per
 * VALIDATED candidate AFTER validation/dedup and BEFORE tree insertion
 * (the og's deferred-poll position), on BOTH emit layers (grammar +
 * raw-sign - the og's block scan creates its CRawFileInfo candidates
 * through one loop). Emission is NOT gated - the og's hook is a FEED,
 * not a filter (the poll never drops a candidate); a repair module
 * consumes the notification, never vetoes the file. Thread safety: the
 * hook may be invoked from worker threads when flags.threads > 1 (the
 * grammar layer fans out); implementations must synchronize themselves. */
struct CarveCandidateInfo {
    u32 format_tag = 0;          /* the value the node will carry in
                                  * props->carve_format: the FormatId for
                                  * grammar-layer hits, 0xFFFF0000 + the
                                  * raw-sign format id for raw-sign hits */
    const char* format_ext = ""; /* display extension ("mov", "mp4") */
    u64 offset = 0;              /* SLICE-relative extent start (the processor
                                  * reads the stream through the slice at this
                                  * offset - the analog of the og feeding the
                                  * candidate object to GetVideoInfo) */
    u64 length = 0;              /* validated extent length */
    u16 confidence = 0;          /* the emitted node's carve_confidence */
};

class CarvePostProcessor {
public:
    virtual ~CarvePostProcessor() = default;
    /* Interface version (the og's COM-style versioned binding analog);
     * v1 = on_candidate per finalized carve candidate. */
    virtual u32 api_version() const { return 1; }
    virtual void on_candidate(IStorageDevice* /*slice*/, const CarveCandidateInfo& /*info*/) {}
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
