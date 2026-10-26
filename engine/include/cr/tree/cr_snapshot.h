/*
 * cr_snapshot.h - mid-scan immutable value snapshot of a ResultTree (G-02).
 *
 * Copy-on-write VALUE snapshot: one O(n) pass under ResultTree::mutex_
 * copies every node's topology + content into flat arrays the reader owns
 * entirely. Rationale (ADR-UI-SNAPSHOT / UI-READINESS-PLAN #2): a
 * reader-writer lock serializes long-lived UI traversals against engine
 * emitters (throughput death); link-stability tricks stay C++-UB races
 * (add_file writes next_sibling_ of an already-visible tail node); a value
 * copy is provably race-free, matches the stage-boundary refresh model,
 * and costs the same as one serialize_tree pass (~150-300 ms at 1M nodes,
 * stall paid at stage boundaries or on explicit demand).
 *
 * Truth contract: SNAP_PARTIAL is reported while the source stage was
 * mid-flight - the API never pretends a mid-scan capture is final state.
 * Recovery from a snapshot (G-02b value-enqueue) reads only
 * snapshot-owned bytes + the source device (concurrent-read safe), so it
 * is race-free by construction.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_SNAPSHOT_H_
#define CR_SNAPSHOT_H_

#include "cr_node.h"
#include <memory>
#include <string>
#include <vector>

namespace cr {
namespace tree {

constexpr u32 SNAP_NONE = 0xFFFFFFFFu;

/* One captured node. Topology is by index into Snapshot::nodes() (the
 * capture order = arena/add order; DFS for serialization follows the
 * same links the live tree has). */
struct SnapNode {
    /* topology (indices; SNAP_NONE when absent) */
    u32 parent = SNAP_NONE;
    u32 first_child = SNAP_NONE;
    u32 next_sibling = SNAP_NONE;
    u32 prev_sibling = SNAP_NONE;

    /* identity + classification */
    std::u16string name; /* display name (FullNode: the role string) */
    std::string name_raw; /* N-016b: original on-disk bytes when mapped */
    u64 node_id = 0;
    u32 fs_kind = 0;
    u64 record_ref = 0;
    u8 origin = 0;        /* tree::Origin */
    u8 deleted = 0;       /* tree::DeletedState */
    u8 confidence = 0;    /* tree::Confidence */
    u8 grade = 1;         /* tree::Grade (S-023; 1 = the emit default) */
    bool overlap = false; /* tree overlap flag (S-023) */
    bool full = false;    /* FullNode discriminator (roots / virtual) */

    /* content (copied, snapshot-owned) */
    u64 logical_size = 0;
    u64 allocated_size = 0;
    FileTimes times;
    u32 attributes = 0;
    u16 flags = 0;
    std::vector<FileProperties::Extent> extents;
    std::vector<u8> resident_data;
    u32 carve_format = 0;
    u32 carve_confidence = 0;
    u8 sha128[16] = {0};
    bool hash_valid = false;
};

class Snapshot {
public:
    /* Capture the whole tree under its mutex. `partial` stamps
     * SNAP_PARTIAL (the caller knows whether the source stage was
     * mid-flight; the orchestrator sets it for mid-stage captures and
     * clears it at stage boundaries). Returns null on allocation
     * failure. */
    static std::unique_ptr<Snapshot> capture(const ResultTree& tree, bool partial);

    u32 node_count() const { return u32(nodes_.size()); }
    const SnapNode& node(u32 index) const { return nodes_[index]; }
    const std::vector<u32>& roots() const { return roots_; }
    bool partial() const { return partial_; }
    u64 captured_at_ms() const { return captured_at_ms_; }

    /* DFS in first_child/next_sibling order - the exact emission order
     * of the live tree's serializer (byte-parity contract: a
     * snapshot-based serialization equals the live one). */
    void dfs(std::vector<u32>& out) const;

    /* Materialize a FileProperties copy for one row (G-02b: the writer's
     * value-enqueue). Caller owns the returned reference. */
    FileProperties* materialize_props(u32 index) const;

private:
    friend class ResultTree;
    Snapshot() = default;
    std::vector<SnapNode> nodes_;
    std::vector<u32> roots_;
    bool partial_ = false;
    u64 captured_at_ms_ = 0;
};

} // namespace tree
} // namespace cr

#endif // CR_SNAPSHOT_H_
