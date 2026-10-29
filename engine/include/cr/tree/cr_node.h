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
#include <atomic>
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

/* NODE_FLAG_JUNK_FILTERED (M-018, bit 14 - tree-internal, set only by the
 * orchestrator's filterfolder pass): the node matched the og's shipped
 * FilterFolder.ini hidden sets (OS droppings, metadata noise, localized
 * "New Folder" names, System Volume Information, BitLocker recovery
 * artifacts). Presentation surfaces (query rows, CLI tree, recover-all
 * planning) skip flagged nodes - the og's PushFolderFilter "hide OS junk
 * from results". Engines never set it; sessions re-derive it on resume
 * (the pass is idempotent and names are stable). */
constexpr u16 NODE_FLAG_JUNK_FILTERED = 0x4000;

/* NODE_FLAG_SIZE_EXCEEDS_BLOCKS (N-177, bit 9 - the SAME numeric bit as
 * the og's node FLAGS 0x200, EXT4SearchDLL :6300-6303): the ext4 engine's
 * truncation-damage marker - i_blocks coverage cannot cover i_size. Set
 * for FILES only; persisted with FileProperties.flags (the session
 * round-trips the field). */
constexpr u16 NODE_FLAG_SIZE_EXCEEDS_BLOCKS = 0x0200;

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
    u32 attributes = 0; /* DOS attributes (dir/hidden/...) */
    /* internal flags: 4 = system-filter (corpus); 0x10 = in-use (NTFS);
     * 0x02 = NTFS dir mirror. N-177: 0x0200 = size-exceeds-blocks
     * damage (the og's ext4 FLAGS bit 0x200 at +0x39, FUN_180007510's
     * i_blocks < size>>9 compare - regular files only; persisted with
     * the session, a grading/diagnostics input). The constant itself is
     * NODE_FLAG_SIZE_EXCEEDS_BLOCKS above (the N-177 convergence repair
     * folded the duplicate PFLAG spelling into the winner's name). */
    u16 flags = 0;
    std::u16string name; /* full display name (LFN) */
    /* N-011a (diff-to-fix): the og's props carry a separate 8.3 short
     * name (GetShortNameLen/SetShortName/GetShortNamePtr vtable surface;
     * persisted per SaveScan ifileproperties-part1 :19-21). Empty when
     * the record has no distinct DOS-namespace $FILE_NAME (modern
     * volumes usually carry one WIN32_AND_DOS name that serves both). */
    std::u16string short_name; /* 8.3 DOS name when distinct from `name` */
    /* N-016b (N-029 residual, s17): the ORIGINAL on-disk name bytes for
     * raw-byte engines (ext4/Btrfs) whose decode introduced lossless
     * U+E000+byte mappings - the og's .rsf keeps the original bytes
     * (substitution happens at display/restore: DRWUI converts via
     * MultiByteToWideChar(CP_UTF8, 0), corpus :264664-694). Populated
     * only when the decoded name carries band units (text::name_has_raw_band)
     * - clean UTF-8 names round-trip without it. The session container
     * persists it (nflags bit 32); the writer derives restore filenames
     * from it (the og's restore semantics - FFFD, not private-use). */
    std::string name_raw; /* original on-disk bytes when the decode mapped */
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
        /* S-059: the extent's LOGICAL start within the file (the
         * VCN/unit-derived position; 0 in the legacy shape - the
         * writer's cumulative walk). A HOLE extent (offset == 0,
         * hole == true) contributes `length` logical bytes of zeros -
         * the og's type-2 zero chunk (DRWUI :875062-75: a full-sparse
         * chunk writes a zeroed buffer with NO read). Old sessions read
         * back with logical == 0 / hole == false and take the legacy
         * cumulative walk - byte-identical to the pre-S-059 behavior. */
        u64 logical = 0;
        bool hole = false;
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
     * IS the extent start), and the boolean flag (OQ-S6-3 decoded: the
     * og's +0xF4 is a per-analyser-type marker — set iff the og analyser's
     * type id == 0xA8(168), the same id space as the persisted +0xF8 type
     * registry; produced in DRWUI FUN_140c41450, loaded opaquely by the
     * CRAW reader FUN_140c40d80, and NEVER consumed after load — the
     * getter is a dead accessor with zero callers and no vtable slot.
     * The core has no og-analyser-type-id space, so the carrier is
     * round-tripped and never invented; the og's +0x56 checksum and
     * +0x94 anchor B are likewise informational/write-only at load —
     * no session-load validator exists in the og, none added here.) */
    u32 carve_checksum = 0xFFFFFFFFu;
    std::u16string carve_note;
    u64 carve_anchor_b = 0;
    bool carve_flag = false;
    /* N-059 (diff-to-fix): per-FS secondary payload — the og's Btrfs
     * record (ifileproperties-part3 :23-25: u32 secondary id A +0x3B,
     * u32 secondary id B +0x3F, qword extra +0x81 "generation/transid
     * family"), the ZFS record (:52-65: u16 secondary flags +0x5A), and
     * — the session-6 addendum's fold — the HFS+ record (part2 :46-48:
     * u16 mode/flags secondary +0xE, u32 type id +0x74). The og field
     * semantics are census-level; the carriers are named for the ENGINE
     * data they hold: btrfs_id_a = the owning subvolume's root id (the
     * naming/N-047 datum), btrfs_id_b = 0 (no decoded source — never
     * invented), btrfs_generation = the inode item's generation qword,
     * zfs_secondary_flags = the znode ZPL_FLAGS low u16 (census-level,
     * carried opaque), hfs_secondary_flags = the catalog record's u16
     * flags, hfs_type_id = the Finder file type. Zero when the engine
     * did not populate them; persisted as the nflags bit-6 extras block
     * (cr_records.cpp) so reloads keep the provenance. */
    u32 btrfs_id_a = 0;
    u32 btrfs_id_b = 0;
    u64 btrfs_generation = 0;
    u16 zfs_secondary_flags = 0;
    u16 hfs_secondary_flags = 0;
    u32 hfs_type_id = 0;
    /* OQ-SA-1 (resolved, scan-engine lane 2026-10-28): the FAT-family
     * first-cluster origin anchor — the og's CFATFileProperties +0x3e /
     * CExFatFileProperties +0x50 slot (part1 :125-127 census; the virtual
     * SetFirstCluster contract, vtable+0x28, getter vtable slot 6). Decode
     * chain: the FAT walk stamps the props anchor from the DENTRY's
     * first-cluster pair (dentry +0x14/+0x1A) for live entries
     * (FATSearchDLL :11652-11660) and, for deleted entries, the
     * chain-reconstruction helper FUN_180012e60 ALWAYS ends with
     * props->SetFirstCluster(start & 0xffffffff) where start = the
     * reconstructed head on success and the DENTRY'S OWN first-cluster
     * pair when the reconstruction produced nothing (:13702-13738, entry
     * fallback :13716-13719, the unconditional stamp :13737-13738). The
     * exFAT walk is the same contract at the other object offset: the
     * LostDir/orphan path calls SetFirstCluster(entry->first_cluster)
     * unconditionally (exFATSearchDLL :5246-5252, vt+0x28 with
     * source+0x18) and the emitted node carries the cluster's absolute
     * byte offset (node[10], :5290-5298). So a chainless deleted file
     * KEEPS its origin cluster in the og's persisted record while the
     * core reconstructed nothing. Carrier semantics: PASSIVE datum — the
     * raw dentry/stream-dentry first-cluster value stamped at emission,
     * independent of chain resolution (0 = empty file / not a FAT-family
     * record); never consumed by the read/recovery paths (the og's
     * degraded contiguous-read use is NOT decoded and NOT reproduced).
     * Persisted as the nflags bit-7 block (cr_records.cpp). */
    u32 fat_origin_cluster = 0;
    /* OQ-SA-1 NTFS half (resolved, scan-engine lane session 3, convergent
     * with SCAN-2f): the og's CNTFSFileProperties +0x64 anchor — the low
     * 32 bits of the FIRST $DATA RUN'S STARTING LCN, the NTFS member of
     * the vtable+0x30 location-anchor family (FAT +0x3e / exFAT +0x50 /
     * NTFS +0x64 / CRAW +0xec; HFS+/ReFS/Generic return -1). Decode chain
     * (all sites read first-hand): the runlist encoder FUN_1800336f0
     * (:41028) emits out1 = the first run cursor entry's stored start LCN
     * and out2 = the total run length in clusters (sparse-zeroed
     * :41096-98); the $DATA parse FUN_1800069e0 (:5126-5127) writes them
     * into the raw props block at +0x53/+0x4f; the copier FUN_180003020
     * (:1987-1988) lands them at +0x64/+0x60; the attach twin
     * FUN_180033250 (:2264-2265) agrees. The serializer FUN_180003440
     * (:2362-2363) persists both. The $FILE_NAME deep path
     * FUN_18002a340 zeroes the slot (:33912) — runs attach later. The
     * prior "+0x60 type-registry id" census reading is refuted (the +0x20
     * getter is a return-0 stub for 8 of 10 per-FS classes; the DRWUI twin
     * serializer FUN_140520d20 persists the engine values verbatim).
     * Carrier semantics: PASSIVE datum — stamped from the first decoded
     * $DATA run at emission (sparse first run -> 0xFFFFFFFF, the og's
     * stored-start read; resident / no-runs -> 0 — the og's resident
     * wrapper stamps a synthetic {record-anchor, 1} pair, and core's 0 is
     * the honest deliberate no-runs value, a REGISTERED divergence, not
     * "the og's zeroed raw block");
     * never consumed by the read/recovery paths. The
     * og's degraded contiguous-READ use of the anchor is NOT decoded and
     * NOT reproduced. Persisted as the nflags bit-8 block
     * (cr_records.cpp). */
    u32 ntfs_origin_lcn = 0;
    /* N-100: the og's +0x60 twin — the $DATA stream's TOTAL cluster
     * count, stamped beside the anchor at every site (the copier
     * FUN_180003020 :1987-1988 writes +0x64 <- raw+0x53 AND +0x60 <-
     * raw+0x4f; the GUID-dump twin FUN_18001c660 :22470-22471 the same;
     * rsf record-4 persists u32(+0x64) then u32(+0x60), rsf-format
     * :398). Semantics: the sum of the run lengths in clusters, ZEROED
     * when ANY run is sparse (FUN_1800336f0 :41096-98 — a sparse
     * anywhere means the total is not a contiguous allocation figure, so
     * the og zeroes it rather than lie; a sparse file therefore carries
     * total 0 while its allocated_size stays nonzero - the two are NOT
     * derivable from each other, which is why this field exists).
     * Resident / no-runs -> 0 (the deliberate no-runs value, as the
     * anchor above). PASSIVE datum, never consumed by read/recovery.
     * Persisted as the nflags bit-9 block (cr_records.cpp). */
    u32 ntfs_total_clusters = 0;
    /* N-131: the og's FS-native PARENT anchors — the lineage datum the
     * structural parent_id (a session-file index) cannot carry.
     *
     * NTFS half (+0x40 qword, the CNTFSFileProperties parent reference):
     * the $FILE_NAME parent-directory FILE REFERENCE masked to the record
     * number (`*fn & 0xffffffffffff` — FUN_18002a340's local_41, first-
     * hand: the mask read at :33902; the copier FUN_180003020 :1982 maps
     * it into the props qword that DRWUI serializes at +0x40, part1 :84).
     * Stamped per-link at every $FILE_NAME-derived emission (the main
     * walk's per-parent fan-out, the GUID-dump record, the INDX
     * second-source entry). The USN journal pass does NOT stamp it (the
     * USN parent comes from the USN_RECORD_V2 header, not a $FILE_NAME
     * attribute; the og's USN emission builds minimal name-only props —
     * no parent stamp decoded). PASSIVE lineage datum, never consumed by
     * read/recovery; persisted as the nflags bit-12 block (u64).
     *
     * FAT half (+0x42 u32, the CFAT parent-directory anchor): the og
     * stamps every child's +0x42 with the CONTAINING DIRECTORY NODE's
     * +0x3e origin cluster (FATSearchDLL :11654 `props+0x42 = local_110`
     * where local_110 = the parent node's +0x3e read directly at :11500
     * for CFAT parents or via the vt+0x30 family getter at :11504).
     * The volume-root node carries +0x3e == 0 by construction (engine
     * init FUN_18000e130 creates it with SetFirstCluster(0) on every
     * flavor — FAT32 included), so root-level files persist 0; a walked
     * subdirectory's children persist the dir's own first cluster (the
     * dentry value the dir's fat_origin_cluster already carries). The
     * og's LostDir nodes carry the FPL sentinel 0xFFFFFFFF (children of
     * the File-Path-Lost root, whose own anchor is 0xFFFFFFFF) — the
     * core's root_lost/root_deleted hosts have no props, so the sentinel
     * does not arise. exFAT has NO parent anchor: the exFAT engine never
     * stamps +0x54 (the only writes are the root/lost-dir 0xFFFFFFFF
     * initializations at :3895/:5026/:11461 and clone verbatim copies;
     * the child emission at :45155 stamps vt+0x28 only) — the part1
     * "type-registry ID" census label and the anchor-twin hypothesis are
     * both refuted first-hand; the slot is dead for children in the og,
     * so the core carries nothing (landing a carrier would not be
     * parity). PASSIVE lineage datum, never consumed by read/recovery;
     * persisted as the nflags bit-13 block (u32). */
    u64 ntfs_parent_ref = 0;
    u32 fat_parent_cluster = 0;
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
    /* M-134: the re-grade one-shot bit (the og marks the node graded at
     * FUN_140389540(node,1) after the deep-read pass; the per-selection
     * re-grade gate is grade==1 && graded-bit clear, :340852-873).
     * Runtime-only by design: it exists to make the per-selection
     * re-grade a true one-shot, not a repeat per selection - a reloaded
     * session re-runs the (idempotent) rules once more, matching the
     * og's observable behavior on unchanged content. */
    bool graded() const { return graded_; }
    void set_graded() { graded_ = 1; }
    /* Direct field set (session deserialize round-trip ONLY - never a
     * rule application; the setter above is the graded path). */
    void set_grade_raw(Grade g) { grade_ = g; }

    u64 node_id() const { return node_id_; } /* stable within the tree */
    bool is_full() const { return full_; }   /* light/full discrimination */

    /* S-052: the node's EXTENT BASIS - which device its extents are
     * relative to. 0 = ABSOLUTE device offsets (the F-1 default every
     * consumer is documented against); >0 = an opaque scan-owned basis
     * handle (LDM dynamic-volume layouts - the value is an index+1 into
     * the basis table the scan/session owns; LdmVolumeDevice reads
     * map it to the member disks). Only ROOTS carry a stamped basis
     * (set_root_basis); descendants read their root's value by walking
     * parent() - the same walk rel_dir_of() already makes. */
    u32 basis() const { return basis_; }

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
    u8 graded_ : 1 = 0;                /* M-134: the re-grade one-shot bit */
    bool full_ = false;                /* FullNode marker (non-polymorphic discrimination) */
    u32 basis_ = 0;                    /* S-052: root's extent-basis id (0 = absolute) */
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
    /* S-052: stamp a root's extent basis (see FileNode::basis). Call
     * between add_root and the subtree's emission; id 0 clears. The
     * scan's LDM loop stamps every dynamic-volume root with its
     * layout's index+1 so recovery/preview consumers can pick the
     * right device without a numeric rebase (multi-disk bases are not
     * numeric - the F-1 rebase cannot express them). */
    void set_root_basis(FileNode* root, u32 basis_id);
    /* S-048: attach an intermediate FOLDER under parent — a FullNode child
     * with a role (the og's tree carries intermediate folders: the carve
     * tree's per-volume groups + per-extension skeleton folders under the
     * "Reconstructed" root; the real og sessions persist them, evidence/
     * executions/s10b-real-rsf-session-mining.md). Never a root; the
     * session protocol's marker=2 record (roots stay marker=1, so old
     * readers keep loading old files unchanged and the reader never
     * re-attaches a child folder as a root). */
    FullNode* add_folder(FileNode* parent, const std::u16string& role);
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
    /* N-166: the live attach counter - every add_file ticks it under the
     * tree's own lock (O(1); detach_subtree subtracts, reset zeroes, and
     * a deserialized tree self-seeds through its own add_file calls).
     * The telemetry layer samples it at progress-event time so mid-stage
     * counters MOVE (the og's results grid tracks the live tree at ~2 s
     * cadence; the core's stage-boundary stamps stay the authoritative
     * close-of-stage sample). Lock-free read: an atomic load. Hard links
     * count per tree entry (per-parent attach), matching the structured
     * engines' own per-link counting.
     *
     * The raw/carved subset (fs_kind == FsKindCode::Raw at attach time —
     * carve stamps it before add_file) so carved_found refreshes live
     * too. Relaxed atomics bumped inside add_file's existing lock;
     * readable from ANY thread without the tree mutex. Monotonic by
     * design: a detached rejected-candidate subtree does NOT decrement
     * (the boundary stamp re-truths at the stage close). */
    u64 live_files() const { return live_files_.load(std::memory_order_relaxed); }
    u64 live_raw() const { return live_raw_.load(std::memory_order_relaxed); }

    /* Rebuild/refresh all FullNode stats (call once after mutations). */
    void refresh_stats();

    /* Move a node to a new parent (link surgery is tree-internal). */
    void reparent(FileNode* node, FileNode* new_parent);

    /* M-035: detach a root's subtree from the tree - the rejected-candidate
     * mechanism (the og's engine-side scorer drops a content-inconsistent
     * lost candidate's results entirely; ours walks first, then scores, so
     * the walk's emission is undone here). Removes the subtree from
     * roots_, nodes_ and both dedup indexes, runs the nodes' destructors
     * (props references released exactly once per node - hard links keep
     * their surviving refs), and leaves the arena bytes reserved until
     * the tree's own destruction (the arena is bulk-freed by design; the
     * memory is bounded by the rejected candidates' file counts). Stats
     * recomputed by the next refresh_stats() call. Call on the scan thread
     * after the subtree's own emission finished (single-writer handoff). */
    void detach_subtree(FileNode* root);

    /* N-166: the live counters (see the accessors above) */
    mutable std::atomic<u64> live_files_{0};
    std::atomic<u64> live_raw_{0};

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
     * truthful. S-052: LDM dynamic-volume roots are exempt from the
     * NUMERIC rebase (multi-disk bases are not numeric) - they carry a
     * stamped basis id instead (set_root_basis), and the basis-aware
     * consumers (SafeWriter/preview via the LdmBasisTable) resolve
     * their layout-relative extents through the retained virtual
     * devices. GAP-S6-15 closed by that wiring.
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
