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
#include <string>
#include <vector>

namespace cr {
namespace tree {

enum class Origin : u8 { Structured = 0, Carved = 1, Lost = 2, Virtual = 3 };
enum class DeletedState : u8 { Active = 0, Deleted = 1, PartiallyOverwritten = 2, Damaged = 3 };
enum class Confidence : u8 {
    Unknown = 0,
    Excellent = 1,     /* all extents verified unallocated in $Bitmap (R2.4) */
    PartialRisk = 2,   /* some extents reused */
    Overwritten = 3,   /* all extents reused - metadata-only salvage */
    MetadataOnly = 4,  /* no extents at all (residentless / torn record) */
};

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
    u64 record_ref = 0;         /* raw NTFS reference (record | seq<<48) */
    u32 fs_kind = 0;           /* FsKind the node came from */
    u64 logical_size = 0;      /* real_size */
    u64 allocated_size = 0;
    FileTimes times;
    u32 attributes = 0;        /* DOS attributes (dir/hidden/...) */
    u16 flags = 0;             /* internal: 4 = system-filter (corpus) */
    std::u16string name;       /* full display name (LFN) */
    /* extents in ABSOLUTE device bytes (resolved through the slice at emit
     * time - engines work slice-relative, the graph stores absolute) */
    struct Extent {
        u64 offset;
        u64 length;
    };
    std::vector<Extent> extents;
    /* carving provenance (Origin::Carved nodes) */
    u32 carve_format = 0;      /* signature registry format id */
    u32 carve_confidence = 0;  /* 0..1000 */
    /* content hash when computed (carve dedup + writer verification) */
    u8 sha128[16] = {0};       /* truncated sha-256 digest, 128-bit id */
    bool hash_valid = false;

    /* Deep copy for hard links (per-link name, shared extents semantics):
     * caller owns the returned ref. */
    FileProperties *clone() const;

    void add_ref() const { ++refs_; }
    void release() const {
        if (--refs_ == 0) delete this;
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
    FileNode *parent() const { return parent_; }
    FileNode *first_child() const { return first_child_; }
    FileNode *next_sibling() const { return next_sibling_; }
    FileNode *prev_sibling() const { return prev_sibling_; }

    const FileProperties *props() const { return props_; }
    FileProperties *props() { return props_; }
    void set_props(FileProperties *p); /* takes a reference */

    Origin origin() const { return origin_; }
    DeletedState deleted_state() const { return deleted_; }
    Confidence confidence() const { return confidence_; }
    void set_confidence(Confidence c) { confidence_ = c; }
    void set_deleted_state(DeletedState d) { deleted_ = d; }
    void set_origin(Origin o) { origin_ = o; }

    u64 node_id() const { return node_id_; } /* stable within the tree */

    /* u16 name in utf8 convenience (lossy for exotic) */
    std::string name_utf8() const;

protected:
    friend class ResultTree;
    FileNode() = default;
    ~FileNode() { if (props_) props_->release(); }
    FileNode *parent_ = nullptr;
    FileNode *first_child_ = nullptr;
    FileNode *next_sibling_ = nullptr;
    FileNode *prev_sibling_ = nullptr;
    FileProperties *props_ = nullptr;
    Origin origin_ : 4 = Origin::Structured;
    DeletedState deleted_ : 4 = DeletedState::Active;
    Confidence confidence_ : 4 = Confidence::Unknown;
    bool full_ = false; /* FullNode marker (non-polymorphic discrimination) */
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
    const Stats &stats() const { return stats_; }
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

    FullNode *add_root(const std::u16string &role);
    /* Attach a file under parent. Takes a REFERENCE (caller releases its own
     * ref when done; each add_file call adds one node-held ref). Hard links:
     * call once per parent, then release once. */
    FileNode *add_file(FileNode *parent, FileProperties *props);

    /* record_ref -> node index for hard links (structured engines) */
    FileNode *find_by_record(u64 record_ref) const;
    /* first-extent + size-class dedup key (merge phase, fix F9) */
    FileNode *find_by_extent(u64 first_offset, u64 length_class) const;

    const std::vector<FileNode *> &roots() const { return roots_; }
    size_t node_count() const { return nodes_.size(); }
    size_t file_count() const;
    u64 total_logical() const;

    /* Rebuild/refresh all FullNode stats (call once after mutations). */
    void refresh_stats();

    /* Registration for dedup lookups (called by add_file). */
    void index_node(FileNode *n);

    /* Move a node to a new parent (link surgery is tree-internal). */
    void reparent(FileNode *node, FileNode *new_parent);

    /* clear without destroying (engines may reuse) */
    void reset();

private:
    std::vector<FileNode *> roots_;
    /* arena: linear storage, bulk-freed */
    struct Arena {
        void *base = nullptr;
        size_t used = 0;
        size_t cap = 0;
    };
    std::vector<Arena> arenas_;
    void *arena_alloc(size_t bytes);
    std::vector<FileNode *> nodes_; /* every node ever added (light+full) */
    /* indexes */
    struct RecordIndexEntry { u64 ref; FileNode *n; };
    std::vector<RecordIndexEntry> by_record_;
    struct ExtentIndexEntry { u64 off; u64 len_class; FileNode *n; };
    std::vector<ExtentIndexEntry> by_extent_;
};

} // namespace tree
} // namespace cr

#endif // CR_NODE_H_
