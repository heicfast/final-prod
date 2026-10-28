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

/* S-052 fwd: the LDM virtual device lives in the table layer
 * (cr/table/cr_ldm.h); the basis adapter below is implemented in
 * cr_safewriter.cpp where both layers meet. */
namespace cr {
namespace table {
class LdmVolumeDevice;
} // namespace table
} // namespace cr

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

    /* S-060: post-commit metadata restoration - the og's per-item
     * tail (DRWUI :874686-718, the only SetFileTime/SetFileAttributesW
     * pair in the export body): SetFileAttributesW(path, attrs &
     * 0xfffd) then a CreateFileW handle and the mask-gated SetFileTime
     * trio, both failures NON-FATAL (ReleaseTrace + continue). The
     * writer calls this AFTER a successful commit; a non-OK return
     * never fails the item - the report row records it. Backends
     * without a metadata surface keep the default (the writer reports
     * "not restored" truthfully). */
    struct FileMeta {
        u64 created = 0;  /* 100ns since 1601 (the FileTimes wire unit) */
        u64 accessed = 0;
        u64 modified = 0;
        u32 attributes = 0; /* the DOS attribute word (the og clears 0x2) */
        /* per-field enables - the og's mask gates (bit 0x8 creation,
         * 0x10 access, 0x20 write) translated to "the engine emitted
         * this field" (a zero slot is the og's no-date and stays unset) */
        bool set_created = false;
        bool set_accessed = false;
        bool set_modified = false;
        bool set_attributes = false;
    };
    virtual u32 set_metadata(u64 item_id, const std::u16string& final_path_utf16,
                             const FileMeta& meta) {
        (void)item_id;
        (void)final_path_utf16;
        (void)meta;
        return ERR_UNSUPPORTED;
    }
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
    /* S-052: the device this item's extents read through - the node's
     * root basis resolved at enqueue (basis 0 / unresolvable = the
     * writer's absolute source_). Recover_item reads HERE, never
     * source_ directly. */
    IStorageDevice* read_src = nullptr;
    u32 basis = 0;              /* the stamped basis id (diagnostics + re-lookup) */
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
    bool name_exhausted = false;     /* N-065: the collision rename space is
                                      * exhausted - the item FAILS at process
                                      * time (the og's cap-break; no _dup) */
    /* ---- G-09 report evidence (captured during process) ---- */
    u64 duration_ms = 0;    /* per-item processing time */
    bool resumed = false;   /* final pre-existed: counted + skipped */
    std::string sha256_hex; /* streamed digest (lowercase hex; empty when not computed) */
    report::HashStatus hash_status = report::HashStatus::NotComputed;
    bool had_unreadable = false; /* FB-R02: a device/decompress failure
                                  * zero-filled a span (Outcome::Salvaged) */
    /* S-060: the post-commit metadata restoration outcome (the og's
     * SetFileAttributesW + mask-gated SetFileTime tail). 0 = restored;
     * a code = the backend's refusal (non-fatal - the item stays Done);
     * the default-constructed value marks "never attempted" (metadata-
     * less backends, directory items, refused items). */
    u32 metadata_rc = 0xFFFFFFFFu;
};

struct WriterConfig {
    u64 read_chunk = 1 << 20;    /* source read granularity */
    u32 rename_attempts = 10000; /* N-065: the og's collision cap
                                     (DRWUI :357131; was 64 + a _dup
                                     fallback - both og-foreign) */
    bool verify_hash = true;     /* salvage-on-mismatch: commit + report the
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

/* S-052: the multi-basis source table - the LDM virtual devices a
 * scan/session owns, indexed by the tree roots' basis ids (basis id N
 * == index N-1; 0 is always the writer's absolute `source`). The table
 * is NON-OWNING: whoever constructs the LdmVolumeDevices (the scan
 * controller while the scan handle lives, the session's re-arm after a
 * load) keeps them alive for the writer's lifetime. */
class LdmBasisTable {
public:
    /* register a device (basis ids are assigned in call order: the
     * first add() is basis 1) */
    void add(::cr::table::LdmVolumeDevice* dev);
    ::cr::table::LdmVolumeDevice* at(u32 basis) const; /* null when absent */
    size_t size() const { return devs_.size(); }

    /* the physical spans a basis-relative extent touches on its member
     * disks - the destination-overlap check's input (R7.2/F12 compares
     * PHYSICAL placements). Empty for an absent basis / out-of-range. */
    struct Span {
        IStorageDevice* dev = nullptr;
        u64 offset = 0;
        u64 length = 0;
    };
    std::vector<Span> spans(u32 basis, u64 offset, u64 length) const;

private:
    std::vector<::cr::table::LdmVolumeDevice*> devs_;
};

class SafeWriter {
public:
    /* `bases` (nullable, attachable later via set_basis_table): the LDM
     * basis devices - S-052. Nodes whose root carries basis N>0 read
     * through bases->at(N); basis-0 nodes read through `source`
     * unchanged. The pointer is BORROWED and may be null when no
     * dynamic volume was scanned. */
    SafeWriter(DestIo& io, IStorageDevice* source, WriterConfig cfg,
               const LdmBasisTable* bases = nullptr);

    /* S-052: late-attach/retract the basis table (the UI arms the
     * session's lazily-rebuilt devices at the first basis-aware
     * enqueue). Read at enqueue time; null = every basis resolves to
     * the absolute source. */
    void set_basis_table(const LdmBasisTable* bases) { bases_ = bases; }

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

    /* collision resolution policy (N-065, the og's DRWUI :357116-357131):
     * intra-queue duplicates always rename to name_1.ext / name_2.ext
     * (%s_%d%s, counter from 1); pre-existing destination finals rename
     * only under ConflictPolicy::Rename (corpus R7.1) - Skip keeps the
     * name so the recover pass detects + skips them (idempotent re-runs).
     * Exhaustion (rename_attempts, the og's cap 10,000) returns EMPTY and
     * the item FAILS with ERR_DEST_NAME_EXHAUSTED (never a _dup name). */
    std::u16string resolve_collision(const std::u16string& dir,
                                     const std::u16string& base_name) const;

    /* sanitize a node display name for the destination FS (corpus sanitizer
     * replaces / * : ? " < > | with _). */
    static std::u16string sanitize_name(const std::u16string& in);

private:
    u32 enqueue_common(const tree::FileProperties* pr, tree::FileNode* node,
                       const tree::FileProperties* value_props, const std::u16string& dest_dir,
                       u32 basis /* S-052: the item's root basis id */);
    u32 recover_item(QueueItem& it, CancelToken* cancel, WriterStats& stats);
    /* timed body: recover_item stamps duration_ms on every exit path */
    u32 recover_item_impl_(QueueItem& it, CancelToken* cancel, WriterStats& stats);
    /* G-08: create `dir` (recursive) once per unique directory per run */
    u32 ensure_dir_(const std::u16string& dir);
    DestIo& io_;
    IStorageDevice* source_;
    /* S-052: the multi-basis table (borrowed; may be null/empty) */
    const LdmBasisTable* bases_ = nullptr;
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
