/* cr_filterfolder.cpp - the result-set junk filter pass (M-018).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/orchestrator/cr_filterfolder.h"
#include "cr/cr_text.h"
#include <atomic>
#include <cstring>
#include <functional>
#include <set>

namespace cr {
namespace filterfolder {

namespace {

/* split a generated ';a;b;' pool into UTF-16 name sets, built once */
struct JunkSets {
    std::set<std::u16string> folders;
    std::set<std::u16string> files;
    std::set<std::u16string> any_kind;

    JunkSets() {
        split_into(data::kFolderPool, data::kFolderPoolLen, folders);
        split_into(data::kFilePool, data::kFilePoolLen, files);
        split_into(data::kFatNamePool, data::kFatNamePoolLen, any_kind);
    }

    /* N-081 (NEW-FINDING, the M-018 landing's ASan red): the pools are
     * semicolon-delimited with a TRAILING delimiter and NO NUL
     * terminator - the first cut read them as C strings and ran
     * strchr/strlen past the array end (global-buffer-overflow on the
     * linux-gcc lane). The parse is now bounded by the exported byte
     * lengths; identical tokens for well-formed data (the trailing
     * ';' yields no empty tail). */
    static void split_into(const u8* pool, u32 pool_len, std::set<std::u16string>& out) {
        const std::string_view all(reinterpret_cast<const char*>(pool), pool_len);
        size_t pos = 0;
        while (pos < all.size()) {
            const size_t semi = all.find(';', pos);
            const size_t end = semi == std::string_view::npos ? all.size() : semi;
            if (end > pos)
                out.insert(text::utf8_to_u16_lossless(all.substr(pos, end - pos)));
            if (semi == std::string_view::npos)
                break;
            pos = semi + 1;
        }
    }
};

const JunkSets& sets() {
    static JunkSets s; /* C++11 magic static: thread-safe one-time build */
    return s;
}

/* ASCII-case-insensitive u16 compare (the lists are ASCII except the
 * CJK "New Folder" entries, which compare exactly - the fold is a no-op
 * for non-ASCII). The og's comparator mode is not decoded in the corpus;
 * case-insensitive is the only sane Windows-name semantics (desktop.ini
 * vs DESKTOP.INI is one file) - registered in PARITY-DELTAS. */
bool eq_fold(const std::u16string& a, const std::u16string& b) {
    if (a.size() != b.size())
        return false;
    for (size_t i = 0; i < a.size(); ++i) {
        u16 ca = a[i], cb = b[i];
        if (ca >= u16('A') && ca <= u16('Z'))
            ca = u16(ca - u16('A') + u16('a'));
        if (cb >= u16('A') && cb <= u16('Z'))
            cb = u16(cb - u16('A') + u16('a'));
        if (ca != cb)
            return false;
    }
    return true;
}

bool in_set(const std::set<std::u16string>& set, const std::u16string& name) {
    /* exact hit first (the common path; CJK entries only match here) */
    if (set.find(name) != set.end())
        return true;
    /* ASCII fold scan (25 entries max - linear is cheaper than a second
     * index for a per-node check that almost never reaches here) */
    for (const auto& e : set)
        if (eq_fold(e, name))
            return true;
    return false;
}

} // namespace

/* N-216: the og's MaxFileSize cap state. The og's shipped posture is the
 * compiled-in default (version.ini carries no key -> 0x400 GiB = 1 TiB);
 * the core mirrors that as the constant default with a host-side setter
 * for the tunable surface. Relaxed-atomic: apply() runs on the pipeline
 * thread; the setter runs at host config time before the scan (the same
 * ordering the og's ctor-time ini read has). */
namespace {
constexpr u64 kDefaultMaxFileSize = u64(0x400) << 30; /* the og's 1024 GiB default */
std::atomic<u64> g_max_file_size{kDefaultMaxFileSize};
} // namespace

u64 max_file_size() {
    return g_max_file_size.load(std::memory_order_relaxed);
}

void set_max_file_size(u64 bytes) {
    g_max_file_size.store(bytes, std::memory_order_relaxed);
}

bool size_exceeds_cap(u64 logical_size) {
    const u64 cap = max_file_size();
    return cap != 0 && logical_size > cap;
}

bool is_junk_name(const std::u16string& name, bool is_dir) {
    if (name.empty())
        return false;
    const JunkSets& s = sets();
    if (in_set(s.any_kind, name))
        return true;
    return is_dir ? in_set(s.folders, name) : in_set(s.files, name);
}

u64 apply(tree::ResultTree& tree) {
    u64 flagged = 0;
    /* one recursive walk over every root; a flagged node's subtree stays
     * (the og hides entries, not their children - a user file inside
     * System Volume Information is still a result) */
    std::function<void(tree::FileNode*)> walk = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            tree::FileProperties* pr = c->props();
            if (pr != nullptr && (pr->flags & tree::NODE_FLAG_JUNK_FILTERED) == 0) {
                const bool is_dir = (pr->attributes & 0x10) != 0;
                if (is_junk_name(pr->name, is_dir) ||
                    /* N-216: the size cap hides oversize nodes exactly as
                     * the og's tree-builder does (DRWUI:750172-175 - the
                     * cap < size compare clears the node's visibility;
                     * files and folders both pass through the check in
                     * the og, folders simply carry size 0) */
                    size_exceeds_cap(pr->logical_size)) {
                    pr->flags = u16(pr->flags | tree::NODE_FLAG_JUNK_FILTERED);
                    ++flagged;
                }
            }
            walk(c);
        }
    };
    for (tree::FileNode* r : tree.roots())
        walk(r);
    return flagged;
}

} // namespace filterfolder
} // namespace cr
