/* cr_query.cpp - core-side query/filter/sort/paging (G-12).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/query/cr_query.h"
#include <algorithm>
#include <cctype>

namespace cr {
namespace query {

/* ---- extension -> category (core-side; mirrors the corpus families) ---- */

namespace {

struct ExtCat {
    const char* ext; /* lower, no dot */
    Category cat;
};

const ExtCat kMap[] = {
    {"jpg", Category::Image},     {"jpeg", Category::Image},    {"png", Category::Image},
    {"gif", Category::Image},     {"bmp", Category::Image},     {"tif", Category::Image},
    {"tiff", Category::Image},    {"webp", Category::Image},    {"heic", Category::Image},
    {"cr2", Category::Image},     {"nef", Category::Image},     {"arw", Category::Image},
    {"dng", Category::Image},     {"orf", Category::Image},     {"raf", Category::Image},
    {"rw2", Category::Image},     {"psd", Category::Image},     {"svg", Category::Image},
    {"ico", Category::Image},     {"mp4", Category::Video},     {"mov", Category::Video},
    {"avi", Category::Video},     {"mkv", Category::Video},     {"wmv", Category::Video},
    {"flv", Category::Video},     {"m4v", Category::Video},     {"mpg", Category::Video},
    {"mpeg", Category::Video},    {"webm", Category::Video},    {"3gp", Category::Video},
    {"mp3", Category::Audio},     {"wav", Category::Audio},     {"flac", Category::Audio},
    {"aac", Category::Audio},     {"m4a", Category::Audio},     {"ogg", Category::Audio},
    {"wma", Category::Audio},     {"amr", Category::Audio},     {"mid", Category::Audio},
    {"pdf", Category::Document},  {"doc", Category::Document},  {"docx", Category::Document},
    {"xls", Category::Document},  {"xlsx", Category::Document}, {"ppt", Category::Document},
    {"pptx", Category::Document}, {"txt", Category::Document},  {"rtf", Category::Document},
    {"odt", Category::Document},  {"ods", Category::Document},  {"hwp", Category::Document},
    {"csv", Category::Document},  {"log", Category::Document},  {"md", Category::Document},
    {"zip", Category::Archive},   {"rar", Category::Archive},   {"7z", Category::Archive},
    {"tar", Category::Archive},   {"gz", Category::Archive},    {"bz2", Category::Archive},
    {"xz", Category::Archive},    {"cab", Category::Archive},   {"iso", Category::Archive},
    {"msi", Category::Archive},
};

std::string lower_ascii(std::string s) {
    for (char& c : s)
        if (c >= 'A' && c <= 'Z')
            c = char(c - 'A' + 'a');
    return s;
}

/* extension of a UTF-8 name (after the last dot, no dot); "" when none */
std::string ext_of(const std::string& name) {
    const size_t dot = name.rfind('.');
    if (dot == std::string::npos || dot + 1 == name.size())
        return "";
    return lower_ascii(name.substr(dot + 1));
}

Category cat_of(const std::string& name) {
    const std::string e = ext_of(name);
    if (e.empty())
        return Category::Other;
    for (const auto& m : kMap)
        if (e == m.ext)
            return m.cat;
    return Category::Other;
}

bool contains_ci(const std::string& hay, const std::string& needle) {
    if (needle.empty())
        return true;
    if (hay.size() < needle.size())
        return false;
    for (size_t i = 0; i + needle.size() <= hay.size(); ++i) {
        size_t k = 0;
        while (k < needle.size()) {
            char a = hay[i + k], b = needle[k];
            if (a >= 'A' && a <= 'Z')
                a = char(a - 'A' + 'a');
            if (b >= 'A' && b <= 'Z')
                b = char(b - 'A' + 'a');
            if (a != b)
                break;
            ++k;
        }
        if (k == needle.size())
            return true;
    }
    return false;
}

struct Ctx {
    const Filter* f;
    std::vector<const tree::FileNode*> matched;
};

bool passes(const Filter& f, const tree::FileNode* n, const tree::FileProperties* pr,
            Category& cat_out) {
    if (n->is_full() || pr == nullptr)
        return false; /* roots/structure: never result rows */
    /* Fast paths first: name-dependent predicates (category, extension,
     * substring) are evaluated LAST and only when configured, so the
     * plain count path (size/date/confidence/origin) never converts a
     * name to UTF-8 (the 1M-node < 200 ms budget depends on this). */
    if (pr->logical_size < f.size_min || pr->logical_size > f.size_max)
        return false;
    if (pr->times.modified < f.modified_from || pr->times.modified > f.modified_to)
        return false;
    if (u32(n->confidence()) < f.confidence_min)
        return false;
    if (f.deleted_only && n->deleted_state() == tree::DeletedState::Active)
        return false;
    if ((f.origin_mask & (1u << u32(n->origin()))) == 0)
        return false;
    if (f.category_mask != 0xFF || !f.extensions.empty() || !f.name_substring.empty()) {
        const std::string name = n->name_utf8();
        cat_out = cat_of(name);
        if ((f.category_mask & (1u << u32(cat_out))) == 0)
            return false;
        if (!f.extensions.empty()) {
            const std::string e = "." + ext_of(name);
            bool hit = false;
            for (const auto& want : f.extensions)
                if (e == want) {
                    hit = true;
                    break;
                }
            if (!hit)
                return false;
        }
        if (!f.name_substring.empty() && !contains_ci(name, f.name_substring))
            return false;
    }
    return true;
}

void walk(const tree::FileNode* n, const std::string& prefix, Ctx& c) {
    for (const tree::FileNode* ch = n->first_child(); ch; ch = ch->next_sibling()) {
        const tree::FileProperties* pr = ch->props();
        Category cat = Category::Other;
        if (passes(*c.f, ch, pr, cat)) {
            c.matched.push_back(ch);
            (void)cat;
            (void)prefix;
        }
        if (ch->first_child())
            walk(ch, prefix, c);
    }
}

} // namespace

Category category_for_extension(const std::string& ext_utf8) {
    const std::string e = lower_ascii(ext_utf8);
    for (const auto& m : kMap)
        if (e == m.ext)
            return m.cat;
    return Category::Other;
}

Result run(const tree::ResultTree& tree, const Query& q) {
    Result out;
    Ctx ctx;
    ctx.f = &q.filter;
    ctx.matched.reserve(1024);
    for (const tree::FileNode* r : tree.roots())
        walk(r, std::string(), ctx);
    out.total_matched = ctx.matched.size();

    /* sort when requested: stable, over the matched index only */
    if (q.sort != SortKey::None && !q.count_only && ctx.matched.size() > 1) {
        auto key_name = [](const tree::FileNode* n) { return n->name_utf8(); };
        auto key_size = [](const tree::FileNode* n) {
            return n->props() ? n->props()->logical_size : u64(0);
        };
        auto key_mod = [](const tree::FileNode* n) {
            return n->props() ? n->props()->times.modified : u64(0);
        };
        auto key_conf = [](const tree::FileNode* n) { return u32(n->confidence()); };
        const bool desc = q.dir == SortDir::Desc;
        switch (q.sort) {
        case SortKey::Name:
            std::stable_sort(ctx.matched.begin(), ctx.matched.end(),
                             [&](const tree::FileNode* a, const tree::FileNode* b) {
                                 const int r = key_name(a).compare(key_name(b));
                                 return desc ? r > 0 : r < 0;
                             });
            break;
        case SortKey::Size:
            std::stable_sort(ctx.matched.begin(), ctx.matched.end(),
                             [&](const tree::FileNode* a, const tree::FileNode* b) {
                                 const u64 x = key_size(a), y = key_size(b);
                                 return desc ? x > y : x < y;
                             });
            break;
        case SortKey::Modified:
            std::stable_sort(ctx.matched.begin(), ctx.matched.end(),
                             [&](const tree::FileNode* a, const tree::FileNode* b) {
                                 const u64 x = key_mod(a), y = key_mod(b);
                                 return desc ? x > y : x < y;
                             });
            break;
        case SortKey::Confidence:
            std::stable_sort(ctx.matched.begin(), ctx.matched.end(),
                             [&](const tree::FileNode* a, const tree::FileNode* b) {
                                 const u32 x = key_conf(a), y = key_conf(b);
                                 return desc ? x > y : x < y;
                             });
            break;
        case SortKey::Path:
        case SortKey::None:
            break; /* Path: rows below sort lexicographically by path */
        default:
            break;
        }
        if (q.sort == SortKey::Path) {
            /* paths needed for ordering: materialize once */
            std::vector<std::string> paths(ctx.matched.size());
            std::vector<size_t> idx(ctx.matched.size());
            for (size_t i = 0; i < ctx.matched.size(); ++i) {
                idx[i] = i;
                /* walk up to compute (bounded by tree depth) */
                std::vector<const tree::FileNode*> chain;
                for (const tree::FileNode* p = ctx.matched[i]; p; p = p->parent())
                    chain.push_back(p);
                std::string s;
                for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
                    if (!s.empty())
                        s.push_back('/');
                    if ((*it)->is_full()) {
                        const auto* fn = static_cast<const tree::FullNode*>(*it);
                        for (u16 ch : fn->role)
                            s.push_back(char(u8(ch < 0x80 ? ch : u'_')));
                    } else if ((*it)->props()) {
                        s += (*it)->name_utf8();
                    }
                }
                paths[i] = s;
            }
            std::stable_sort(idx.begin(), idx.end(), [&](size_t a, size_t b) {
                const int r = paths[a].compare(paths[b]);
                return desc ? r > 0 : r < 0;
            });
            std::vector<const tree::FileNode*> sorted(ctx.matched.size());
            for (size_t i = 0; i < idx.size(); ++i)
                sorted[i] = ctx.matched[idx[i]];
            ctx.matched.swap(sorted);
        }
    }

    if (q.count_only)
        return out;

    /* page */
    if (q.offset >= ctx.matched.size())
        return out;
    const size_t avail = ctx.matched.size() - size_t(q.offset);
    const size_t take = q.limit ? size_t(q.limit) : avail;
    const size_t n = take < avail ? take : avail;
    out.rows.reserve(n);

    /* build the ancestry stack once, mutate the tail per row (paths):
     * role/names '/'-joined, no leading separator, leaf included */
    std::vector<const tree::FileNode*> stack;
    auto emit_path = [&](const tree::FileNode* leaf, std::string& out_path) {
        stack.clear();
        for (const tree::FileNode* p = leaf->parent(); p; p = p->parent())
            stack.push_back(p);
        for (auto it = stack.rbegin(); it != stack.rend(); ++it) {
            if (!out_path.empty())
                out_path.push_back('/');
            if ((*it)->is_full()) {
                const auto* fn = static_cast<const tree::FullNode*>(*it);
                for (u16 ch : fn->role)
                    out_path.push_back(char(u8(ch < 0x80 ? ch : u'_')));
            } else if ((*it)->props()) {
                out_path += (*it)->name_utf8();
            }
        }
        if (leaf->props()) {
            if (!out_path.empty())
                out_path.push_back('/');
            out_path += leaf->name_utf8();
        }
    };

    for (size_t i = size_t(q.offset); i < size_t(q.offset) + n; ++i) {
        const tree::FileNode* node = ctx.matched[i];
        const tree::FileProperties* pr = node->props();
        Row row;
        row.node = node;
        row.name_utf8 = node->name_utf8();
        row.size = pr ? pr->logical_size : 0;
        row.modified = pr ? pr->times.modified : 0;
        row.origin = u32(node->origin());
        row.deleted = u32(node->deleted_state());
        row.confidence = u32(node->confidence());
        row.category = u32(cat_of(row.name_utf8));
        emit_path(node, row.path_utf8);
        out.rows.push_back(std::move(row));
    }
    return out;
}

} // namespace query
} // namespace cr
