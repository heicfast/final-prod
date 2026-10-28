/* cr_node.cpp - result tree implementation.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/tree/cr_node.h"
#include <algorithm>
#include <cstring>

namespace cr {
namespace tree {

/* ---- utf16 -> utf8 (for CLI output) ---- */
namespace {
void utf16_to_utf8(const std::u16string& in, std::string& out) {
    out.clear();
    unsigned int cp;
    for (size_t i = 0; i < in.size(); ++i) {
        u16 c = in[i];
        if (c >= 0xD800 && c <= 0xDBFF && i + 1 < in.size() && in[i + 1] >= 0xDC00 &&
            in[i + 1] <= 0xDFFF) {
            cp = 0x10000 + ((static_cast<unsigned int>(c) - 0xD800) << 10) +
                 (static_cast<unsigned int>(in[i + 1]) - 0xDC00);
            ++i;
        } else {
            cp = c;
        }
        if (cp < 0x80)
            out.push_back(static_cast<char>(cp));
        else if (cp < 0x800) {
            out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else if (cp < 0x10000) {
            out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else {
            out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        }
    }
}
} // namespace

FileProperties::~FileProperties() = default;

FileProperties* FileProperties::clone() const {
    FileProperties* p = new (std::nothrow) FileProperties();
    if (!p)
        return nullptr;
    p->record_ref = record_ref;
    p->fs_kind = fs_kind;
    p->logical_size = logical_size;
    p->allocated_size = allocated_size;
    p->times = times;
    p->attributes = attributes;
    p->flags = flags;
    p->name = name;
    p->name_raw = name_raw; /* N-016b: hard links keep the raw bytes too */
    p->resident_data = resident_data;
    p->extents = extents;
    p->carve_format = carve_format;
    p->carve_confidence = carve_confidence;
    p->carve_checksum = carve_checksum; /* M-051: RAW provenance */
    p->carve_note = carve_note;
    p->carve_anchor_b = carve_anchor_b;
    p->carve_flag = carve_flag;
    /* OQ-SA-1 NTFS half: the location anchor survives the hard-link fan-out
     * clone (clone() is the NTFS walk's own fan-out path — the per-FS
     * carriers of the non-cloning engines need no line here by
     * construction). */
    p->ntfs_origin_lcn = ntfs_origin_lcn;
    /* N-100: the +0x60 total-clusters twin rides the same clone (the og's
     * clone copies the whole props block; the fan-out keeps both anchors). */
    p->ntfs_total_clusters = ntfs_total_clusters;
    std::memcpy(p->sha128, sha128, sizeof(sha128));
    p->hash_valid = hash_valid;
    return p;
}

void FileNode::set_props(FileProperties* p) {
    if (props_)
        props_->release();
    props_ = p;
    if (props_)
        props_->add_ref();
}

std::string FileNode::name_utf8() const {
    std::string s;
    if (props_)
        utf16_to_utf8(props_->name, s);
    return s;
}

void* ResultTree::arena_alloc(size_t bytes) {
    if (arenas_.empty() || arenas_.back().used + bytes > arenas_.back().cap) {
        size_t cap = arenas_.empty() ? 131072 : arenas_.back().cap * 2;
        while (cap < bytes)
            cap *= 2;
        Arena a;
        a.base = new (std::nothrow) unsigned char[cap];
        if (!a.base)
            return nullptr;
        a.cap = cap;
        a.used = 0;
        arenas_.push_back(a);
    }
    Arena& a = arenas_.back();
    void* p = static_cast<unsigned char*>(a.base) + a.used;
    a.used += bytes;
    return p;
}

FullNode* ResultTree::add_root(const std::u16string& role) {
    std::lock_guard<std::mutex> lock(mutex_);
    void* mem = arena_alloc(sizeof(FullNode));
    if (!mem)
        return nullptr;
    FullNode* n = new (mem) FullNode();
    n->role = role;
    FileProperties* p = new (std::nothrow) FileProperties();
    if (!p)
        return nullptr;
    p->name = role;
    n->set_props(p);
    p->release(); /* set_props took the ref */
    n->node_id_ = nodes_.size();
    n->full_ = true;
    nodes_.push_back(n);
    roots_.push_back(n);
    return n;
}

FileNode* ResultTree::add_file(FileNode* parent, FileProperties* props) {
    if (!props || !parent)
        return nullptr;
    std::lock_guard<std::mutex> lock(mutex_);
    void* mem = arena_alloc(sizeof(FileNode));
    if (!mem)
        return nullptr;
    FileNode* n = new (mem) FileNode();
    n->set_props(props); /* +1 ref: caller releases its own when done */
    n->parent_ = parent;
    /* O(1) append via the tail pointer - the old sibling-chain walk was
     * O(n) per insert (O(n^2) for a million-file carve root; audit:
     * million-node scans must stay time-disciplined too) */
    if (parent->first_child_ == nullptr) {
        parent->first_child_ = n;
        parent->last_child_ = n;
    } else {
        FileNode* t = parent->last_child_;
        t->next_sibling_ = n;
        n->prev_sibling_ = t;
        parent->last_child_ = n;
    }
    n->node_id_ = nodes_.size();
    nodes_.push_back(n);
    index_node_locked(n);
    return n;
}

void ResultTree::index_node_locked(FileNode* n) {
    if (!n->props())
        return;
    const FileProperties* p = n->props();
    if (p->record_ref != 0)
        by_record_[p->record_ref] = n;
    if (!p->extents.empty()) {
        u64 first = p->extents.front().offset;
        u64 len_class = 0;
        u64 sz = p->logical_size;
        /* size classes: 64 KiB buckets under 1 MiB, log2 above (fix F9) */
        if (sz >= (1 << 20)) {
            while ((1ULL << len_class) < sz)
                ++len_class;
            len_class += 64;
        } else {
            len_class = sz / 65536;
        }
        by_extent_[{first, len_class}] = n;
    }
}

FileNode* ResultTree::find_by_record(u64 record_ref) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = by_record_.find(record_ref);
    return it != by_record_.end() ? it->second : nullptr;
}

/* ---- F-1 basis normalization (see cr_node.h) ---- */

u64 ResultTree::extent_len_class_(const FileProperties* p) {
    u64 len_class = 0;
    const u64 sz = p->logical_size;
    if (sz >= (1 << 20)) {
        while ((1ULL << len_class) < sz)
            ++len_class;
        len_class += 64;
    } else {
        len_class = sz / 65536;
    }
    return len_class;
}

void ResultTree::rebase_props_locked(FileNode* n, u64 base,
                                     std::unordered_set<FileProperties*>& visited) {
    FileProperties* p = n->props();
    if (!p || p->extents.empty())
        return;
    if (p->flags & NODE_FLAG_EXT_SHIFTED)
        return; /* already on the absolute basis (idempotency guard) */
    if (!visited.insert(p).second)
        return; /* hard-link shared props: shift exactly once */
    const u64 len_class = extent_len_class_(p);
    const u64 old_first = p->extents.front().offset;
    for (auto& e : p->extents)
        e.offset += base;
    p->flags = u16(p->flags | NODE_FLAG_EXT_SHIFTED);
    /* re-key the dedup index so post-scan find_by_extent stays truthful */
    by_extent_.erase({old_first, len_class});
    by_extent_[{p->extents.front().offset, len_class}] = n;
}

size_t ResultTree::node_count_snapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return nodes_.size();
}

void ResultTree::rebase_new_extents(size_t from_node, u64 base) {
    if (base == 0)
        return; /* whole-disk scan / synthetic device: already absolute */
    std::lock_guard<std::mutex> lock(mutex_);
    if (from_node >= nodes_.size())
        return;
    std::unordered_set<FileProperties*> visited;
    for (size_t i = from_node; i < nodes_.size(); ++i)
        rebase_props_locked(nodes_[i], base, visited);
}

void ResultTree::rebase_subtree_extents(FileNode* root, u64 base) {
    if (base == 0 || !root)
        return;
    std::lock_guard<std::mutex> lock(mutex_);
    std::unordered_set<FileProperties*> visited;
    /* iterative DFS: candidate roots can hold millions of nodes */
    std::vector<FileNode*> stack;
    stack.push_back(root);
    while (!stack.empty()) {
        FileNode* n = stack.back();
        stack.pop_back();
        rebase_props_locked(n, base, visited);
        for (FileNode* c = n->first_child(); c; c = c->next_sibling())
            stack.push_back(c);
    }
}

FileNode* ResultTree::find_by_extent(u64 first_offset, u64 length_class) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = by_extent_.find({first_offset, length_class});
    return it != by_extent_.end() ? it->second : nullptr;
}

size_t ResultTree::file_count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t c = 0;
    for (auto* n : nodes_)
        if (n->props() && !(n->props()->attributes & 0x10))
            ++c;
    return c;
}

u64 ResultTree::total_logical() const {
    std::lock_guard<std::mutex> lock(mutex_);
    u64 t = 0;
    for (auto* n : nodes_)
        if (n->props())
            t += n->props()->logical_size;
    return t;
}

void ResultTree::refresh_stats() {
    std::lock_guard<std::mutex> lock(mutex_);
    /* M-047: RECURSIVE accumulation - the og's stats accumulator
     * (rsf-format :285: vtable +0x58 = the recursive walk maintaining
     * +0x9C/+0xA8/+0xB4/+0xBC) counts the whole subtree, not direct
     * children: a root whose files live in nested directories reports
     * them. The old direct-only loop under-reported every nested tree
     * (and the session reader discarded the persisted image, so reloaded
     * roots showed 0 until a refresh - then the under-count). */
    for (auto* n : nodes_) {
        if (!n->full_)
            continue;
        FullNode* root = static_cast<FullNode*>(n);
        root->stats_ = FullNode::Stats{};
        /* iterative depth-first stack (subtrees can be deep - no recursion
         * depth risk); children of file nodes count too (S-032's inner
         * entry nodes live under carved archive FileNodes and are real
         * result rows in the og's tree as well). */
        std::vector<FileNode*> stack;
        for (FileNode* c = root->first_child_; c != nullptr; c = c->next_sibling_)
            stack.push_back(c);
        while (!stack.empty()) {
            FileNode* cur = stack.back();
            stack.pop_back();
            if (cur->props()) {
                if (cur->props()->attributes & 0x10 /* ATTR_DIRECTORY */)
                    ++root->stats_.dir_count;
                else {
                    ++root->stats_.file_count;
                    root->stats_.total_logical += cur->props()->logical_size;
                }
                if (cur->deleted_state() != DeletedState::Active)
                    ++root->stats_.deleted_count;
            }
            for (FileNode* c2 = cur->first_child_; c2 != nullptr; c2 = c2->next_sibling_)
                stack.push_back(c2);
        }
    }
}

void ResultTree::reparent(FileNode* node, FileNode* new_parent) {
    if (node == nullptr || node == new_parent || node->parent_ == new_parent)
        return;
    std::lock_guard<std::mutex> lock(mutex_);
    /* forbid cycles: new_parent must not be node itself or its descendant */
    for (FileNode* p = new_parent; p != nullptr; p = p->parent_)
        if (p == node)
            return;
    FileNode* cur = node->parent_;
    if (node->prev_sibling_)
        node->prev_sibling_->next_sibling_ = node->next_sibling_;
    else if (cur)
        cur->first_child_ = node->next_sibling_;
    if (node->next_sibling_)
        node->next_sibling_->prev_sibling_ = node->prev_sibling_;
    if (cur && cur->last_child_ == node)
        cur->last_child_ = node->prev_sibling_ ? node->prev_sibling_ : node->next_sibling_;
    node->prev_sibling_ = nullptr;
    node->next_sibling_ = nullptr;
    node->parent_ = new_parent;
    if (new_parent == nullptr)
        return;
    if (new_parent->first_child_ == nullptr) {
        new_parent->first_child_ = node;
        new_parent->last_child_ = node;
    } else {
        FileNode* t = new_parent->last_child_;
        t->next_sibling_ = node;
        node->prev_sibling_ = t;
        new_parent->last_child_ = node;
    }
}

void ResultTree::detach_subtree(FileNode* root) {
    if (root == nullptr)
        return;
    std::lock_guard<std::mutex> lock(mutex_);
    /* collect the subtree (iterative DFS - candidate roots can hold
     * millions of nodes, same discipline as rebase_subtree_extents) */
    std::vector<FileNode*> sub;
    sub.push_back(root);
    for (size_t i = 0; i < sub.size(); ++i) {
        for (FileNode* c = sub[i]->first_child_; c != nullptr; c = c->next_sibling_)
            sub.push_back(c);
    }
    /* membership set for the index-guard below */
    std::unordered_set<FileNode*> dead(sub.begin(), sub.end());

    /* indexes: erase ONLY entries this subtree owns - hard links and
     * duplicate record refs may legitimately map to surviving nodes. */
    for (FileNode* n : sub) {
        const FileProperties* p = n->props();
        if (p == nullptr)
            continue;
        if (p->record_ref != 0) {
            const auto it = by_record_.find(p->record_ref);
            if (it != by_record_.end() && dead.count(it->second) != 0)
                by_record_.erase(it);
        }
        if (!p->extents.empty()) {
            const u64 first = p->extents.front().offset;
            const u64 len_class = extent_len_class_(p);
            const auto it = by_extent_.find({first, len_class});
            if (it != by_extent_.end() && dead.count(it->second) != 0)
                by_extent_.erase(it);
        }
    }
    /* nodes_: remove every subtree node (order-preserving remove_if over
     * the membership set; detached nodes must not survive in nodes_ or
     * file_count()/total_logical()/refresh_stats() would still see them) */
    nodes_.erase(std::remove_if(nodes_.begin(), nodes_.end(),
                                [&dead](FileNode* n) { return dead.count(n) != 0; }),
                 nodes_.end());
    /* roots_: remove the root itself (a detached root always is one; the
     * loop guards against a future non-root call shape). */
    roots_.erase(std::remove(roots_.begin(), roots_.end(), root), roots_.end());
    /* sever the root's child links BEFORE the destructors run: a detached
     * root must look EMPTY to any later subtree walker (the orchestrator's
     * per-candidate extent rebase, snapshot DFS) - the nodes below are
     * about to be destructed and must not be reachable through links. */
    root->first_child_ = nullptr;
    root->last_child_ = nullptr;
    /* run the destructors (props references released once per node; the
     * FullNode's u16string freed) - the arena bytes stay reserved until
     * reset(), mirroring reset()'s own per-node destruction discipline. */
    for (FileNode* n : sub) {
        if (n->full_)
            static_cast<FullNode*>(n)->~FullNode();
        else
            n->~FileNode();
    }
}

void ResultTree::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    /* full nodes carry a u16string: their OWN destructor must run */
    for (auto* n : nodes_) {
        if (n->full_)
            static_cast<FullNode*>(n)->~FullNode();
        else
            n->~FileNode();
    }
    nodes_.clear();
    roots_.clear();
    by_record_.clear();
    by_extent_.clear();
    for (auto& a : arenas_) {
        delete[] static_cast<unsigned char*>(a.base);
        a.base = nullptr;
        a.used = a.cap = 0;
    }
    arenas_.clear();
}

ResultTree::~ResultTree() {
    reset();
}

ResultTree::ResultTree() = default;

} // namespace tree
} // namespace cr
