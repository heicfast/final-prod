/* cr_snapshot.cpp - mid-scan immutable value snapshot (G-02).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/tree/cr_snapshot.h"
#include <chrono>
#include <cstring>
#include <unordered_map>

namespace cr {
namespace tree {

/* ResultTree member: full access to nodes_ / roots_ / mutex_. Everything
 * is read under the lock, so the copy is provably race-free (all emitters
 * serialize on the same mutex). */
std::unique_ptr<Snapshot> ResultTree::capture_snapshot(bool partial) const {
    auto snap = std::unique_ptr<Snapshot>(new (std::nothrow) Snapshot());
    if (!snap)
        return nullptr;
    snap->partial_ = partial;
    snap->captured_at_ms_ = u64(std::chrono::duration_cast<std::chrono::milliseconds>(
                                    std::chrono::steady_clock::now().time_since_epoch())
                                    .count());

    std::lock_guard<std::mutex> lock(mutex_);

    /* index map: node pointer -> snapshot index (capture order = arena
     * order, which the DFS reproduces the live tree's emission order on) */
    std::unordered_map<const FileNode*, u32> idx;
    idx.reserve(nodes_.size() * 2);
    snap->nodes_.reserve(nodes_.size());
    for (const FileNode* n : nodes_) {
        SnapNode s;
        s.node_id = n->node_id();
        s.full = n->is_full();
        s.basis = n->basis(); /* S-052: the basis rides every row (roots carry the stamp) */
        s.origin = u8(n->origin());
        s.deleted = u8(n->deleted_state());
        s.confidence = u8(n->confidence());
        s.grade = u8(n->grade());
        s.overlap = n->overlap();
        s.tags_extracted =
            n->tags_extracted(); /* M-101: the runtime mark
                                  * rides the node like graded/failed - never the props */
        if (const FileProperties* p = n->props()) {
            s.name = p->name;
            s.name_raw = p->name_raw; /* N-016b: raw bytes survive snapshots */
            s.fs_kind = p->fs_kind;
            s.record_ref = p->record_ref;
            s.logical_size = p->logical_size;
            s.allocated_size = p->allocated_size;
            s.times = p->times;
            s.attributes = p->attributes;
            s.flags = p->flags;
            s.extents = p->extents;
            s.resident_data = p->resident_data;
            s.carve_format = p->carve_format;
            s.carve_confidence = p->carve_confidence;
            s.version_group = p->version_group; /* M-101 */
            std::memcpy(s.sha128, p->sha128, sizeof(s.sha128));
            s.hash_valid = p->hash_valid;
        }
        idx.emplace(n, u32(snap->nodes_.size()));
        snap->nodes_.push_back(std::move(s));
    }

    /* topology pass: translate pointers to indices. The sibling/child
     * pointers of nodes visible at capture time are stable under the
     * lock (every mutation takes the lock). */
    for (const FileNode* n : nodes_) {
        const u32 i = idx[n];
        SnapNode& s = snap->nodes_[i];
        if (n->parent())
            s.parent = idx[n->parent()];
        if (n->first_child())
            s.first_child = idx[n->first_child()];
        if (n->next_sibling())
            s.next_sibling = idx[n->next_sibling()];
        if (n->prev_sibling())
            s.prev_sibling = idx[n->prev_sibling()];
    }
    snap->roots_.reserve(roots_.size());
    for (const FileNode* r : roots_)
        snap->roots_.push_back(idx[r]);
    return snap;
}

std::unique_ptr<Snapshot> Snapshot::capture(const ResultTree& tree, bool partial) {
    return tree.capture_snapshot(partial);
}

void Snapshot::dfs(std::vector<u32>& out) const {
    /* equals the live tree's recursive first_child/next_sibling walk:
     *   emit(n); for (c = first_child; c; c = next_sibling) visit(c)
     * iterative with an explicit stack, children pushed reversed so the
     * first child pops first */
    out.clear();
    out.reserve(nodes_.size());
    std::vector<u32> stack;
    stack.reserve(64);
    for (auto it = roots_.rbegin(); it != roots_.rend(); ++it)
        stack.push_back(*it);
    while (!stack.empty()) {
        const u32 n = stack.back();
        stack.pop_back();
        out.push_back(n);
        std::vector<u32> kids;
        for (u32 c = nodes_[n].first_child; c != SNAP_NONE; c = nodes_[c].next_sibling)
            kids.push_back(c);
        for (auto kit = kids.rbegin(); kit != kids.rend(); ++kit)
            stack.push_back(*kit);
    }
}

FileProperties* Snapshot::materialize_props(u32 index) const {
    const SnapNode& s = nodes_[index];
    FileProperties* p = new (std::nothrow) FileProperties();
    if (!p)
        return nullptr;
    p->record_ref = s.record_ref;
    p->fs_kind = s.fs_kind;
    p->logical_size = s.logical_size;
    p->allocated_size = s.allocated_size;
    p->times = s.times;
    p->attributes = s.attributes;
    p->flags = s.flags;
    p->name = s.name;
    p->name_raw = s.name_raw; /* N-016b */
    p->resident_data = s.resident_data;
    p->extents = s.extents;
    p->carve_format = s.carve_format;
    p->carve_confidence = s.carve_confidence;
    p->version_group = s.version_group; /* M-101: the materialized copy
                                         * carries the group so snapshot-side recovery/preview
                                         * addresses see the same relation the live tree does */
    std::memcpy(p->sha128, s.sha128, sizeof(p->sha128));
    p->hash_valid = s.hash_valid;
    return p;
}

} // namespace tree
} // namespace cr
