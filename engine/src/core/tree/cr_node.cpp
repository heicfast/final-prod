/* cr_node.cpp - result tree implementation.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/tree/cr_node.h"
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
    p->resident_data = resident_data;
    p->extents = extents;
    p->carve_format = carve_format;
    p->carve_confidence = carve_confidence;
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

void ResultTree::index_node(FileNode* n) {
    std::lock_guard<std::mutex> lock(mutex_);
    index_node_locked(n);
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
    /* friend of FileNode: protected links readable on every node */
    for (auto* n : nodes_) {
        if (!n->full_)
            continue;
        FullNode* root = static_cast<FullNode*>(n);
        root->stats_ = FullNode::Stats{};
        for (FileNode* c = root->first_child_; c != nullptr; c = c->next_sibling_) {
            if (!c->props())
                continue;
            if (c->props()->attributes & 0x10 /* ATTR_DIRECTORY */)
                ++root->stats_.dir_count;
            else {
                ++root->stats_.file_count;
                root->stats_.total_logical += c->props()->logical_size;
            }
            if (c->deleted_state() != DeletedState::Active)
                ++root->stats_.deleted_count;
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
