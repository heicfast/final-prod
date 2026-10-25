/*
 * cr_safewriter.h - the safe recovery writer (R7).
 *
 * Pure policy + queue + verification. The actual file IO is injected as a
 * DestIo interface (Win32 implementation in the platform layer).
 *
 * R7.2 (fix F12): HARD refusal when the destination volume's disk extent
 * intersects any queued node's source extents - code-level, not UI.
 * R7.1: collision naming (numeric suffix), atomic temp+rename, hashes.
 * R7.3 (G-09): per-item report evidence (duration, hash, outcome) feeding
 * the machine-readable recovery report (cr_report.h).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_SAFEWRITER_H_
#define CR_SAFEWRITER_H_

#include "../recovery/cr_report.h"
#include "../crypto/cr_sha256.h"
#include "../storage/cr_device.h"
#include "../tree/cr_node.h"
#include <set>
#include <string>
#include <vector>

namespace cr {
namespace recovery {

/* Destination IO abstraction (Win32 file backend in the platform layer). */
class DestIo {
public:
    virtual ~DestIo() = default;
    /* create the temp file (unique name per item); returns handle id */
    /* create the temp file; the per-item suffix (".part-<id>") keeps
     * concurrent same-final-name items from colliding (audit row 35c) */
    virtual u32 create_temp(u64 item_id, const std::u16string& final_path_utf16,
                            const std::u16string& temp_suffix_utf16) = 0;
    /* create a directory (recursively: parents first) - "already exists"
     * is success, any other failure must surface as an error code
     * (G-08: destination structure reconstruction). */
    virtual u32 mkdir(const std::u16string& path_utf16) = 0;
    virtual u32 write(u64 item_id, u64 offset, const void* data, u32 len) = 0;
    /* fsync-equivalent + atomic rename temp -> final */
    virtual u32 commit(u64 item_id) = 0;
    virtual void close(u64 item_id) = 0;
    /* N-015: free bytes at the destination root (0 = unknown -> the
     * writer's pre-check is skipped; the ENOSPC fast-fail still guards
     * the pass). WinDestIo maps GetDiskFreeSpaceExW "available to
     * caller"; mock IOs default 0. */
    virtual u64 free_bytes(const std::u16string&) const { return 0; }
    /* check existing final path (resume detection) */
    virtual bool exists(const std::u16string& path_utf16) = 0;
};

enum class ItemState : u8 { Pending = 0, Reading, Writing, Verifying, Done, Error, Refused };

/* Destination conflict policy (G-08/G-09): what to do when the exact
 * final path already exists on the destination.
 * - Rename (default): numeric suffix "name (2)" (corpus R7.1; og parity).
 * - Skip: keep the name; the pre-existing final is counted and skipped
 *   (idempotent re-runs; never overwrites user content).
 * - Overwrite: temp + REPLACE_EXISTING onto the final.
 * Intra-queue duplicates (two queued nodes, same dir+name) ALWAYS rename
 * regardless of policy. */
enum class ConflictPolicy : u8 { Rename = 0, Skip = 1, Overwrite = 2 };

struct QueueItem {
    tree::FileNode* node = nullptr;
    /* G-02b: snapshot-value path - when set, the writer reads this
     * instead of node->props() (exactly one of the two is non-null for
     * content items). */
    const tree::FileProperties* value_props = nullptr;
    std::u16string dest_dir;    /* utf16 directory (with trailing sep) */
    std::u16string final_name;  /* collision-resolved */
    std::u16string temp_suffix; /* ".part-" + id */
    u64 item_id = 0;
    ItemState state = ItemState::Pending;
    u32 error = 0;
    u64 bytes_done = 0;
    u64 bytes_total = 0;
    bool hash_verified = false;
    bool is_dir = false;             /* G-08: structure item - mkdir, no content */
    bool nothing_to_recover = false; /* GAP-S6-7: skip before any write -
                                      * no extents, no resident data, size>0 */
    /* ---- G-09 report evidence (captured during process) ---- */
    u64 duration_ms = 0;    /* per-item processing time */
    bool resumed = false;   /* final pre-existed: counted + skipped */
    std::string sha256_hex; /* streamed digest (lowercase hex; empty when not computed) */
    report::HashStatus hash_status = report::HashStatus::NotComputed;
    bool had_unreadable = false; /* FB-R02: a device/decompress failure
                                  * zero-filled a span (Outcome::Salvaged) */
};

struct WriterConfig {
    u64 read_chunk = 1 << 20; /* source read granularity */
    u32 rename_attempts = 64; /* collision suffix cap */
    bool verify_hash = true;  /* salvage-on-mismatch: commit + report the
                               * mismatch (Outcome::HashMismatch), never a
                               * silent drop */
    /* G-08: reconstruct the destination directory structure - every item's
     * dest_dir is created (mkdir -p, cached per unique dir) before writing,
     * and directory nodes are enqueueable so empty leaves materialize. */
    bool reconstruct_dirs = true;
    ConflictPolicy conflict = ConflictPolicy::Rename;
};

struct WriterStats {
    u64 queued = 0;
    u64 done = 0;
    u64 refused = 0;
    u64 errors = 0;
    u64 hash_mismatch = 0;
    u64 resumed = 0;            /* detected complete .part files finalized */
    u64 salvaged = 0;           /* committed with zero-filled spans (FB-R02) */
    u64 dirs = 0;               /* G-08: directory nodes materialized */
    u64 nothing_to_recover = 0; /* GAP-S6-7: no-extent/no-resident items
                                 * skipped before any write */
    /* N-015: the pass aborted (or refused upfront) because the
     * destination ran out of space - remaining items stayed Pending. */
    bool dest_full = false;
};

class SafeWriter {
public:
    SafeWriter(DestIo& io, IStorageDevice* source, WriterConfig cfg);

    /*
     * set_destination - records the destination volume's disk extent.
     * Source slices report their absolute placement via slice_offset param.
     */
    void set_destination(u64 dest_disk_start, u64 dest_disk_bytes);

    /* N-013: ALL disk spans of the destination volume (a spanned/RAID
     * destination has more than one extent - first-extent-only arming
     * misses overlap on the second+ spans). Replaces any prior arming. */
    void set_destination_spans(const std::vector<std::pair<u64, u64>>& spans);

    /* enqueue one node; computes the collision-resolved name. Returns
     * ERR_DEST_OVERLAP (refusal) when the node's extents hit the dest. */
    u32 enqueue(tree::FileNode* node, const std::u16string& dest_dir);

    /* G-02b: value-enqueue from an immutable snapshot row - mid-scan
     * recovery without touching the live tree. The snapshot owns every
     * byte read here; the source device is contract-safe concurrently.
     * Same collision/refusal semantics as the node-based enqueue. */
    u32 enqueue(const tree::Snapshot& snap, u32 snap_index, const std::u16string& dest_dir);

    /* process the queue: read source extents -> temp -> verify -> commit. */
    u32 process(CancelToken* cancel, bool (*progress)(u64 done, u64 total, void*),
                void* progress_user, WriterStats& stats);

    const std::vector<QueueItem>& queue() const { return queue_; }

    /* G-09: report material over this writer's outcomes. `source`/
     * `destination` are caller-supplied UTF-8 identities (e.g. the volume
     * label/path and the dest dir); every queue item appears with its
     * truthful outcome - hashes only where the writer actually computed
     * them (never faked). */
    report::Totals report_totals(const std::string& source, const std::string& destination,
                                 u64 elapsed_ms, const WriterStats& stats) const;
    std::vector<report::Item> report_items() const;

    /* collision resolution policy: intra-queue duplicates always rename
     * ("name" -> "name (2)"...); pre-existing destination finals rename only
     * under ConflictPolicy::Rename (corpus R7.1) - Skip keeps the name so
     * the recover pass detects + skips them (idempotent re-runs). */
    std::u16string resolve_collision(const std::u16string& dir,
                                     const std::u16string& base_name) const;

    /* sanitize a node display name for the destination FS (corpus sanitizer
     * replaces / * : ? " < > | with _). */
    static std::u16string sanitize_name(const std::u16string& in);

private:
    u32 enqueue_common(const tree::FileProperties* pr, tree::FileNode* node,
                       const tree::FileProperties* value_props, const std::u16string& dest_dir);
    u32 recover_item(QueueItem& it, CancelToken* cancel, WriterStats& stats);
    /* timed body: recover_item stamps duration_ms on every exit path */
    u32 recover_item_impl_(QueueItem& it, CancelToken* cancel, WriterStats& stats);
    /* G-08: create `dir` (recursive) once per unique directory per run */
    u32 ensure_dir_(const std::u16string& dir);
    DestIo& io_;
    IStorageDevice* source_;
    WriterConfig cfg_;
    /* N-013: every armed destination span (multi-extent volumes). The
     * legacy single-extent pair is span[0] of this vector. */
    std::vector<std::pair<u64, u64>> dest_spans_;
    bool have_dest_ = false;
    u64 next_id_ = 1;
    /* G-02b: snapshot-materialized props live here for the writer's
     * lifetime (QueueItem holds a non-owning view; no copy hazards). */
    std::vector<std::unique_ptr<tree::FileProperties>> value_store_;
    std::vector<QueueItem> queue_;
    std::set<std::u16string> made_dirs_; /* mkdir cache (G-08) */
    /* names assigned within THIS queue ("dir|name") - intra-queue
     * collision source of truth, independent of destination state */
    mutable std::set<std::u16string> claimed_names_;
};

} // namespace recovery
} // namespace cr

#endif // CR_SAFEWRITER_H_
