/*
 * cr_folderfilter.cpp - the scan-time folder filter (M-073, diff-to-fix).
 * The header carries the corpus contract and the scoping decisions; this
 * file is the prune walk itself.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/orchestrator/cr_folderfilter.h"

#include "cr/cr_text.h"

#include <cctype>

namespace cr {
namespace folderfilter {

namespace {

/* ASCII-lower one byte ('\\' -> '/' on the way through: hosts and users
 * write Windows-style prefixes; the tree's internal separator is '/'). */
inline char sep_fold(char c) {
    if (c == '\\')
        return '/';
    if (c >= 'A' && c <= 'Z')
        return char(c - 'A' + 'a');
    return c;
}

inline bool starts_with(const std::string& hay, const std::string& needle) {
    return hay.size() >= needle.size() && hay.compare(0, needle.size(), needle) == 0;
}

} // namespace

bool normalize_prefix(std::string_view in, std::string& out) {
    out.clear();
    /* drop leading separators, then fold; a trailing separator is dropped
     * at the end (both "Users/Alice/" and "\Users\Alice" are "users/alice") */
    size_t i = 0;
    while (i < in.size() && (in[i] == '/' || in[i] == '\\'))
        ++i;
    for (; i < in.size(); ++i)
        out.push_back(sep_fold(char(in[i])));
    while (!out.empty() && out.back() == '/')
        out.pop_back();
    return !out.empty();
}

bool any_active(const std::vector<std::string>& prefixes) {
    std::string tmp;
    for (const auto& p : prefixes)
        if (normalize_prefix(p, tmp))
            return true;
    return false;
}

bool file_path_matches(const std::string& path_lower, const std::string& prefix_lower) {
    return starts_with(path_lower, prefix_lower);
}

bool folder_path_compatible(const std::string& dir_lower, const std::string& prefix_lower) {
    return starts_with(dir_lower, prefix_lower) || starts_with(prefix_lower, dir_lower);
}

std::string node_path_key(const tree::FileNode* n) {
    std::string key;
    if (n == nullptr || n->props() == nullptr)
        return key;
    if (!text::lossless_u16_to_utf8(n->props()->name, key))
        key.clear(); /* exotic private-use mix: the lossless decoder's own
                      * inverse refused - an empty key never matches a
                      * non-empty prefix (honest, never a false keep) */
    for (char& c : key)
        c = sep_fold(c);
    return key;
}

namespace {

/* Directory-ness in the tree is TWO-shaped (the engines' own convention,
 * M-018's filterfolder::apply reads the same bit): the FAT/exFAT engines
 * emit subdirectories as LIGHT FileNodes whose props carry the DOS
 * directory attribute (0x10) and recurse into them through the node
 * handle, while add_folder (the carve skeleton, the pathless-tree's
 * intermediate folders) creates FullNodes. Either shape is a folder for
 * the filter's purposes; a file never carries the bit and is never full. */
bool node_is_dir(const tree::FileNode* n) {
    if (n->is_full())
        return true;
    const tree::FileProperties* p = n->props();
    return p != nullptr && (p->attributes & 0x10) != 0;
}

void count_subtree(tree::FileNode* n, PruneStats& st) {
    for (tree::FileNode* c = n->first_child(); c != nullptr; c = c->next_sibling()) {
        if (node_is_dir(c)) {
            count_subtree(c, st);
        } else {
            ++st.files;
            if (c->deleted_state() != tree::DeletedState::Active)
                ++st.deleted;
        }
    }
}

/* The recursive walk. `dir` is the accumulated folder path (ASCII-lower,
 * '/'-joined, root-excluded). Children are visited through a captured
 * next pointer - detach_child severs the detached node's sibling links,
 * so the walk must never read through a detached node. */
void prune_children(tree::ResultTree& tree, tree::FileNode* node, const std::string& dir,
                    const std::vector<std::string>& prefixes, PruneStats& st) {
    for (tree::FileNode* c = node->first_child(); c != nullptr;) {
        tree::FileNode* next = c->next_sibling();
        const std::string key = node_path_key(c);
        const std::string path = dir.empty() ? key : (dir + "/" + key);
        if (node_is_dir(c)) {
            /* a folder: keep it only while some descendant could match */
            bool compatible = false;
            for (const auto& p : prefixes)
                if (folder_path_compatible(path, p)) {
                    compatible = true;
                    break;
                }
            if (!compatible) {
                count_subtree(c, st);
                tree.detach_child(node, c);
            } else {
                prune_children(tree, c, path, prefixes, st);
            }
        } else {
            /* a file: its full path must start with a prefix */
            bool match = false;
            for (const auto& p : prefixes)
                if (file_path_matches(path, p)) {
                    match = true;
                    break;
                }
            if (!match) {
                ++st.files;
                if (c->deleted_state() != tree::DeletedState::Active)
                    ++st.deleted;
                tree.detach_child(node, c);
            }
        }
        c = next;
    }
}

} // namespace

PruneStats prune_root(tree::ResultTree& tree, tree::FileNode* root,
                      const std::vector<std::string>& prefixes) {
    PruneStats st;
    std::string tmp;
    std::vector<std::string> active;
    for (const auto& p : prefixes)
        if (normalize_prefix(p, tmp))
            active.push_back(tmp);
    if (active.empty() || root == nullptr)
        return st; /* off (or nothing to do): the behavior-unchanged pin */
    prune_children(tree, root, std::string(), active, st);
    return st;
}

} // namespace folderfilter
} // namespace cr
