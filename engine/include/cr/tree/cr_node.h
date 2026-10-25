/*
 * cr_node.h - the canonical result FileNode graph (R6.4).
 *
 * Corpus model (rsf-format section 5.4, fully validated on real sessions):
 *   - dual node classes: lightweight CFileNode (0x93) for the millions of
 *     file entries, full CFileSystemNode (0x290) ONLY for roots/virtuals;
 *   - intrusive parent/child/sibling links (no per-node allocations);
 *   - refcounted IFileProperties content objects shared by hard links;
 *   - per-child marker byte on the session wire (0 = light, 1 = full).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_NODE_H_
#define CR_NODE_H_

#include "../cr_types.h"
#include "cr/storage/cr_device.h"
#include <memory>
#include <mutex>
#include <unordered_set>
#include <string>
#include <unordered_map>
#include <vector>

namespace cr {
namespace tree {

/* NODE_FLAG_EXT_SHIFTED: tree-internal marker (bit 15 - no engine sets it)
 * that this node's props->extents already carry the ABSOLUTE device basis.
 * Set by the F-1 rebase paths exactly once; guards against double-shifts
 * when several rebase passes (stage-end range, candidate subtree, old-session
 * load-time normalization) could touch the same node. Persisted with the
 * props - a reloaded session's nodes are already absolute. */
constexpr u16 NODE_FLAG_EXT_SHIFTED = 0x8000;

enum class Origin : u8 { Structured = 0, Carved = 1, Lost = 2, Virtual = 3 };
enum class DeletedState : u8 { Active = 0, Deleted = 1, PartiallyOverwritten = 2, Damaged = 3 };
enum class Confidence : u8 {
    Unknown = 0,
    Excellent = 1,    /* all extents verified unallocated in $Bitmap (R2.4) */
    PartialRisk = 2,  /* some extents reused */
    Overwritten = 3,  /* all extents reused - metadata-only salvage */
    MetadataOnly = 4, /* no extents at all (residentless / torn record) */
};

/* S-023: the og's CRecoverability 3-grade model (DRWUI bits 5-7 of
 * node+0x73). A SEPARATE decision system from Confidence (which is the
 * in-walk allocation-axis model): grades are computed by the post-scan
 * grading pass (cr_grading.cpp) from category/device/overlap/extension
 * inputs, stored with og semantics - monotonic-max, grade-2 sticky -
 * and re-graded on demand for grade-1 nodes (per-selection). */
enum class Grade : u8 { Low = 0, Average = 1, High = 2 };

/* G-02: mid-scan value snapshot (defined in cr_snapshot.h). */
class Snapshot;

using FileTimes = ::cr::FileTimes; /* moved to cr_types.h (fundamental) */

/*
 * FileProperties - the refcounted content object (IFileProperties family).
 * Field map mirrors the corpus serializer table: {flags, name, size, times,
 * attributes, runlist}. Shared across hard links of the same file.
 */
class FileProperties {
public:
    /* NTFS identity (structured nodes): MFT record number for hard-link
     * grouping; 0 for carved. */
    u64 record_ref = 0;   /* raw NTFS reference (record | seq<<48) */
    u32 fs_kind = 0;      /* FsKind the node came from */
    u64 logical_size = 0; /* real_size */
    u64 allocated_size = 0;
    FileTimes times;
    u32 attributes = 0;  /* DOS attributes (dir/hidden/...) */
    u16 flags = 0;       /* internal: 4 = system-filter (corpus) */
    std::u16string name; /* full display name (LFN) */
    /* N-011a (diff-to-fix): the og's props carry a separate 8.3 short
     * name (GetShortNameLen/SetShortName/GetShortNamePtr vtable surface;
     * persisted per SaveScan ifileproperties-part1 :19-21). Empty when
     * the record has no distinct DOS-namespace $FILE_NAME (modern
     * volumes usually carry one WIN32_AND_DOS name that serves both). */
    std::u16string short_name; /* 8.3 DOS name when distinct from `name` */
    /* resident content: NTFS resident $DATA / ADS streams, ZFS embedded
     * block-pointer payloads, Btrfs inline extents. Present only when the
     * content lives in metadata instead of disk extents (or alongside
     * extents for mixed streams - resident bytes come first). Bounded by
     * the metadata area that holds them. */
    std::vector<u8> resident_data;
    /* extents in ABSOLUTE device bytes (resolved through the slice at emit
     * time - engines work slice-relative, the graph stores absolute).
     * codec 0 = plain bytes (psize == length); codec != 0 = the on-disk
     * psize bytes at `offset` decode to `length` logical bytes via
     * cr::compress::decompress_block (ZFS-family compressed extents). */
    struct Extent {
        u64 offset;
        u64 length;
        u32 codec = 0; /* cr::compress::Codec */
        u64 psize = 0; /* on-disk size (0 == length: raw) */
    };
    std::vector<Extent> extents;
    /* carving provenance (Origin::Carved nodes) */
    u32 carve_format = 0;     /* signature registry format id */
    u32 carve_confidence = 0; /* 0..1000 */
    /* M-051: RAW provenance (Origin::Carved nodes) — the og's
     * CRAWFileProperties fields (ifileproperties-part3 :77-90): the
     * analyser checksum (0xFFFFFFFF = the invalid/unknown marker; the
     * og's census: 12,030/12,147 nonzero), the note-family string
     * (usually empty), the tail anchor B (0 = none; the header anchor A
     * IS the extent start), and the boolean flag (the og's semantics
     * undecoded — OQ-S6-3; carried and round-tripped, never invented). */
    u32 carve_checksum = 0xFFFFFFFFu;
    std::u16string carve_note;
    u64 carve_anchor_b = 0;
    bool carve_flag = false;
    /* content hash when computed (carve dedup + writer verification) */
    u8 sha128[16] = {0}; /* truncated sha-256 digest, 128-bit id */
    bool hash_valid = false;

    /* Deep copy for hard links (per-link name, shared extents semantics):
     * caller owns the returned ref. */
    FileProperties* clone() const;

    void add_ref() const { ++refs_; }
    void release() const {
        if (--refs_ == 0)
            delete this;
    }
    ~FileProperties();

private:
    mutable u32 refs_ = 1;
    friend class FileNode;
};

/*
 * FileNode - intrusive tree node. NEVER allocated alone by callers; use
 * ResultTree::add_file / add_root. Ownership: ResultTree owns all nodes in
 * one arena (destructor frees once - no per-node delete storm).
 */
class FileNode {
public:
    FileNode* parent() const { return parent_; }
    FileNode* first_child() const { return first_child_; }
    FileNode* next_sibling() const { return next_sibling_; }
    FileNode* prev_sibling() const { return prev_sibling_; }

    const FileProperties* props() const { return props_; }
    FileProperties* props() { return props_; }
    void set_props(FileProperties* p); /* takes a reference */

    Origin origin() const { return origin_; }
    DeletedState deleted_state() const { return deleted_; }
    Confidence confidence() const { return confidence_; }
    void set_confidence(Confidence c) { confidence_ = c; }
    void set_deleted_state(DeletedState d) { deleted_ = d; }
    void set_origin(Origin o) { origin_ = o; }

    /* S-023 grade: og setter semantics (FUN_140089630) - grade 0 clears
     * unless the current grade is 2 (sticky); grades 1/2 only raise
     * (monotonic max). The overlap flag (og bit 2 of +0x73, set by
     * FUN_1400895b0 together with forcing grade >= 2) never clears. */
    Grade grade() const { return grade_; }
    void set_grade(Grade g) {
        if (g == Grade::Low) {
            if (grade_ != Grade::High)
                grade_ = Grade::Low; /* grade-2 nodes are never cleared */
        } else if (u8(grade_) < u8(g)) {
            grade_ = g; /* max(cur, new) */
        }
    }
    bool overlap() const { return overlap_; }
    void set_overlap() { /* og: bit 2 + grade >= 2 together */
        overlap_ = 1;
        set_grade(Grade::High);
    }
    /* Direct field set (session deserialize round-trip ONLY - never a
     * rule application; the setter above is the graded path). */
    void set_grade_raw(Grade g) { grade_ = g; }

    u64 node_id() const { return node_id_; } /* stable within the tree */
    bool is_full() const { return full_; }   /* light/full discrimination */

    /* u16 name in utf8 convenience (lossy for exotic) */
    std::string name_utf8() const;

protected:
    friend class ResultTree;
    FileNode() = default;
    ~FileNode() {
        if (props_)
            props_->release();
    }
    FileNode* parent_ = nullptr;
    FileNode* first_child_ = nullptr;
    FileNode* last_child_ = nullptr; /* O(1) appends (million-node roots) */
    FileNode* next_sibling_ = nullptr;
    FileNode* prev_sibling_ = nullptr;
    FileProperties* props_ = nullptr;
    Origin origin_ : 4 = Origin::Structured;
    DeletedState deleted_ : 4 = DeletedState::Active;
    Confidence confidence_ : 4 = Confidence::Unknown;
    Grade grade_ : 2 = Grade::Average; /* og engine default: grade 1 at emit */
    u8 overlap_ : 1 = 0;               /* og twin/overlap flag (bit 2 of +0x73) */
    bool full_ = false;                /* FullNode marker (non-polymorphic discrimination) */
    u64 node_id_ = 0;
};

/*
 * FullNode - roots and virtual shortcut nodes only (corpus 0x290 class).
 * Carries accumulated stats (recursive counts/sizes) refreshed on add.
 */
class FullNode final : public FileNode {
public:
    struct Stats {
        u64 file_count = 0;
        u64 dir_count = 0;
        u64 total_logical = 0;
        u64 deleted_count = 0;
    };
    const Stats& stats() const { return stats_; }
    /* M-047: the persisted-image restore path (the og's trust model - the
     * reader stores the saved accumulator verbatim, rsf-format :318-320).
     * Callers otherwise never set stats by hand; refresh_stats
     * recomputes them recursively on demand. */
    void set_stats(const Stats& s) { stats_ = s; }
    std::u16string role; /* "Lost Files", volume label, "Other deleted files" */

private:
    friend class ResultTree;
    Stats stats_;
};

/*
 * ResultTree - owns everything; the one object passed between engines,
 * merge, filters, session persistence and the recovery writer.
 */
class ResultTree {
public:
    ResultTree();
    ~ResultTree();

    FullNode* add_root(const std::u16string& role);
    /* Attach a file under parent. Takes a REFERENCE (caller releases its own
     * ref when done; each add_file call adds one node-held ref). Hard links:
     * call once per parent, then release once. */
    FileNode* add_file(FileNode* parent, FileProperties* props);

    /* record_ref -> node index for hard links (structured engines) */
    FileNode* find_by_record(u64 record_ref) const;
    /* first-extent + size-class dedup key (merge phase, fix F9) */
    FileNode* find_by_extent(u64 first_offset, u64 length_class) const;

    const std::vector<FileNode*>& roots() const { return roots_; }
    size_t node_count() const { return nodes_.size(); }
    size_t file_count() const;
    u64 total_logical() const;

    /* Rebuild/refresh all FullNode stats (call once after mutations). */
    void refresh_stats();

    /* Move a node to a new parent (link surgery is tree-internal). */
    void reparent(FileNode* node, FileNode* new_parent);

    /* clear without destroying (engines may reuse) */
    void reset();

    /* G-02: immutable value snapshot - one O(n) copy under mutex_.
     * `partial` stamps SNAP_PARTIAL (mid-stage captures never pretend
     * to be final state). See cr_snapshot.h for the race-freedom
     * rationale and the DFS byte-parity contract. */
    std::unique_ptr<Snapshot> capture_snapshot(bool partial = false) const;

    /*
     * F-1 basis normalization. Engines emit extents SLICE-RELATIVE
     * (cr_mftwalk: "extents in slice-relative bytes; the tree consumer
     * resolves to absolute via the slice base (orchestrator does this
     * at merge)"). These methods ARE that merge step: they shift extent
     * offsets by `base` so session/tree extents become ABSOLUTE device
     * offsets - the basis every consumer (SafeWriter reads, the F12
     * source/destination overlap check) is documented against.
     *
     * rebase_new_extents: shifts nodes appended since `from_node`
     * (a node_count_snapshot() mark taken BEFORE the stage ran; ranges
     * never overlap across stages, so no double-shift on resume).
     * rebase_subtree_extents: shifts every node under `root` (lost
     * candidates keep private roots; walked after their pool joins).
     * Shared props (hard links) shift exactly once via the visited set.
     * base == 0 is a no-op (whole-disk scans, synthetic devices).
     * The extent dedup index is re-keyed so post-scan lookups stay
     * truthful. LDM volume nodes keep their layout-relative basis
     * (multi-disk bases are not expressible; registered limitation).
     */
    size_t node_count_snapshot() const;
    void rebase_new_extents(size_t from_node, u64 base);
    void rebase_subtree_extents(FileNode* root, u64 base);

private:
    void index_node_locked(FileNode* n); /* caller holds mutex_ */
    void rebase_props_locked(FileNode* n, u64 base, std::unordered_set<FileProperties*>& visited);
    static u64 extent_len_class_(const FileProperties* p);

    std::vector<FileNode*> roots_;
    /* arena: linear storage, bulk-freed */
    struct Arena {
        void* base = nullptr;
        size_t used = 0;
        size_t cap = 0;
    };
    std::vector<Arena> arenas_;
    void* arena_alloc(size_t bytes);
    std::vector<FileNode*> nodes_; /* every node ever added (light+full) */
    /* indexes */
    /* Hashed dedup indexes (audit row 34: the linear scans were O(n^2)
     * at carved-file scale). record_ref -> node; (first offset, size
     * class) -> node. The vectors are kept for deterministic iteration;
     * the maps answer the lookups. */
    std::unordered_map<u64, FileNode*> by_record_;
    struct ExtentKey {
        u64 off;
        u64 len_class;
        bool operator==(const ExtentKey& o) const {
            return off == o.off && len_class == o.len_class;
        }
    };
    struct ExtentKeyHash {
        size_t operator()(const ExtentKey& k) const {
            return std::hash<u64>()(k.off ^ (k.len_class << 1));
        }
    };
    std::unordered_map<ExtentKey, FileNode*, ExtentKeyHash> by_extent_;

    /* Thread safety: engines emit from orchestrator worker threads (R6).
     * All mutating operations and index lookups serialize on this mutex;
     * roots()/node_count() are stable only between stages. */
    mutable std::mutex mutex_;
};

} // namespace tree
} // namespace cr

#endif // CR_NODE_H_
