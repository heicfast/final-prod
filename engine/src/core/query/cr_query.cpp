/* cr_query.cpp - core-side query/filter/sort/paging (G-12).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/query/cr_query.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

namespace cr {
namespace query {

/* ---- extension -> category ---- */

/* Generated from the corpus FileFilter.xml by scripts/gen_filefilter.py
 * (M-023): 8 categories, 1,481 unique extensions, first-writer-wins for
 * the 8 cross-listed ones (ari/gz/insp/insv/jar/md/uzip/xz). The table
 * itself lives in cr_filefilter_data.cpp (sorted by ext for binary
 * search - the 64-entry linear scan would be 1,481 comparisons deep). */
namespace filefilter_data {
struct ExtCat {
    const char* ext;
    Category cat;
};
extern const ExtCat kEntries[];
extern const size_t kCount;
} // namespace filefilter_data

namespace {

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
    /* N-126: delegate to category_for_extension - ONE source of truth
     * for the extension -> category mapping (the pre-fix duplicate
     * lookup desynced the moment the overlay landed: the filter path
     * missed the Unsaved rows the public classifier returned). */
    return category_for_extension(ext_of(name));
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

/* ASCII-case-insensitive prefix test (the path-prefix filter's comparator). */
bool starts_with_ci(const std::string& hay, const std::string& prefix) {
    if (prefix.empty())
        return true;
    if (hay.size() < prefix.size())
        return false;
    for (size_t k = 0; k < prefix.size(); ++k) {
        char a = hay[k], b = prefix[k];
        if (a >= 'A' && a <= 'Z')
            a = char(a - 'A' + 'a');
        if (b >= 'A' && b <= 'Z')
            b = char(b - 'A' + 'a');
        if (a != b)
            return false;
    }
    return true;
}

/* Subtree prune test for the path-prefix walk: can ANY descendant of a node
 * whose accumulated path is `dir` still match `prefix`? Descendant paths all
 * start with dir + '/'; some descendant matches iff `dir` and `prefix` are
 * prefix-compatible (one is a prefix of the other - '/' is case-neutral). */
bool path_compatible(const std::string& dir, const std::string& prefix) {
    return starts_with_ci(dir, prefix) || starts_with_ci(prefix, dir);
}

struct Ctx {
    const Filter* f;
    std::vector<const tree::FileNode*> matched;
};

bool passes(const Filter& f, const tree::FileNode* n, const tree::FileProperties* pr,
            Category& cat_out, const std::string& full_path) {
    if (n->is_full() || pr == nullptr)
        return false; /* roots/structure: never result rows */
    if ((pr->flags & tree::NODE_FLAG_JUNK_FILTERED) != 0)
        return false; /* M-018: the og's FilterFolder hidden set - never a
                       * result row (the PushFolderFilter presentation
                       * pass; sessions re-derive the flag on resume) */
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
    /* M-072: the state dimension as a mask (deleted_only remains sugar
     * for the Lost-files-only toggle; the mask expresses the og's group) */
    if ((f.effective_state_mask() & (1u << u32(n->deleted_state()))) == 0)
        return false;
    if ((f.origin_mask & (1u << u32(n->origin()))) == 0)
        return false;
    if (f.category_mask != 0x3FF || !f.extensions.empty() || !f.name_substring.empty()) {
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
    /* the path-prefix filter (og AdvanceFilterCondition path group): the
     * row's full path must start with the prefix; the walk only passes a
     * materialized path when the filter is active (the plain count path
     * keeps its no-string-conversion fast lane). M-072 adds the substring
     * form on the same axis (the og's filter-bar path box). */
    if (!f.path_prefix.empty() && !starts_with_ci(full_path, f.path_prefix))
        return false;
    if (!f.path_substring.empty() && !contains_ci(full_path, f.path_substring))
        return false;
    return true;
}

void walk(const tree::FileNode* n, const std::string& prefix, Ctx& c) {
    for (const tree::FileNode* ch = n->first_child(); ch; ch = ch->next_sibling()) {
        const tree::FileProperties* pr = ch->props();
        Category cat = Category::Other;
        /* the child's accumulated path is materialized ONLY when the
         * path-prefix filter is active (prefix carries the ancestors,
         * '/'-joined, no trailing separator - the Row::path_utf8 shape) */
        std::string full;
        if (!c.f->path_prefix.empty() || !c.f->path_substring.empty()) {
            const std::string name = ch->name_utf8();
            full = prefix.empty() ? name : prefix + "/" + name;
        }
        if (passes(*c.f, ch, pr, cat, full)) {
            c.matched.push_back(ch);
            (void)cat;
        }
        if (ch->first_child() &&
            (c.f->path_prefix.empty() || path_compatible(full, c.f->path_prefix)))
            walk(ch, full, c);
    }
}

} // namespace

Category category_for_extension(const std::string& ext_utf8) {
    const std::string e = lower_ascii(ext_utf8);
    if (e.empty())
        return Category::Other;
    /* N-126: the og's "Unsaved File" category - the office
     * unsaved-document family. The corpus decodes the Word autosave/
     * backup pair (.asd/.wbk); the og's full membership table (the
     * office tmp forms, the ~-prefixed locks) is not decoded anywhere
     * in the corpus (the R-27 assistant space carries only the six
     * 5-0xB categories; the UI's type-mask bit 6 owns the axis) - the
     * overlay carries the decoded subset ahead of the generated
     * FileFilter table (which has no autosave rows: the og's
     * FileFilter.xml carries none either - the category is a UI-layer
     * classification over engine results, which is why the overlay -
     * not the generated table - is the right seam). */
    if (e == "asd" || e == "wbk")
        return Category::Unsaved;
    const filefilter_data::ExtCat* first = filefilter_data::kEntries;
    const filefilter_data::ExtCat* last = first + filefilter_data::kCount;
    const filefilter_data::ExtCat* it = std::lower_bound(
        first, last, e.c_str(),
        [](const filefilter_data::ExtCat& m, const char* key) { return strcmp(m.ext, key) < 0; });
    if (it != last && strcmp(it->ext, e.c_str()) == 0)
        return it->cat;
    return Category::Other;
}

size_t filefilter_entry_count() {
    return filefilter_data::kCount;
}

Result run(const tree::ResultTree& tree, const Query& q) {
    Result out;
    Ctx ctx;
    ctx.f = &q.filter;
    ctx.matched.reserve(1024);
    for (const tree::FileNode* r : tree.roots()) {
        /* the root's own path segment (FullNode role, ASCII fallback) -
         * only materialized when the path filter is active */
        std::string root_seg;
        if ((!q.filter.path_prefix.empty() || !q.filter.path_substring.empty()) && r->is_full()) {
            const auto* fn = static_cast<const tree::FullNode*>(r);
            for (u16 ch : fn->role)
                root_seg.push_back(char(u8(ch < 0x80 ? ch : u'_')));
        }
        walk(r, root_seg, ctx);
    }
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
        case SortKey::Grade: /* S-023: the og's results comparator puts
                              * grade-2 first (DRWUI :535155-535161) - asc
                              * here = Low..High, desc = High first */
            std::stable_sort(ctx.matched.begin(), ctx.matched.end(),
                             [&](const tree::FileNode* a, const tree::FileNode* b) {
                                 const u32 x = u32(a->grade()), y = u32(b->grade());
                                 return desc ? x > y : x < y;
                             });
            break;
        case SortKey::Path:
        case SortKey::None:
            break; /* Path: rows below sort lexicographically by path */
        case SortKey::Type: {
            /* M-029: the og's Type column. Category in the FileFilter.xml
             * iconIndex display order (Graphic=0 Document=1 Email=2
             * Audio=3 Video=4 Other=5 Archive=6 Bookmarks=7 - the finding's
             * corpus census), then extension, then name; None last. */
            const auto type_order = [](const tree::FileNode* n) -> u32 {
                switch (cat_of(n->name_utf8())) {
                case Category::Image:
                    return 0;
                case Category::Document:
                    return 1;
                case Category::Email:
                    return 2;
                case Category::Audio:
                    return 3;
                case Category::Video:
                    return 4;
                case Category::Other:
                    return 5;
                case Category::Archive:
                    return 6;
                case Category::Bookmarks:
                    return 7;
                default:
                    return 8; /* None: sorts last */
                }
            };
            const auto ext_of = [](const tree::FileNode* n) -> std::string {
                const std::string nm = n->name_utf8();
                const size_t dot = nm.find_last_of('.');
                return dot == std::string::npos ? std::string() : nm.substr(dot + 1);
            };
            std::stable_sort(ctx.matched.begin(), ctx.matched.end(),
                             [&](const tree::FileNode* a, const tree::FileNode* b) {
                                 const u32 xa = type_order(a), xb = type_order(b);
                                 if (xa != xb)
                                     return desc ? xa > xb : xa < xb;
                                 const std::string ea = ext_of(a), eb = ext_of(b);
                                 const int r = ea.compare(eb);
                                 if (r != 0)
                                     return desc ? r > 0 : r < 0;
                                 const int rn = key_name(a).compare(key_name(b));
                                 return desc ? rn > 0 : rn < 0;
                             });
            break;
        }
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

/* ---- M-029: Date/Size bucket-ID translation -------------------------- */

namespace {

/* FILETIME (100ns since 1601) -> time_t, the engine layer's shared
 * conversion (cr_fat_quick.cpp's unix*1e7 + 116444736000000000 inverse). */
std::time_t ft_to_unix(u64 ft) {
    return std::time_t(ft / 10'000'000 - 11'644'473'600);
}

u64 unix_to_ft(std::time_t t) {
    return u64(t + 11'644'473'600) * 10'000'000;
}

/* Local-time decomposition of a FILETIME (the cr_crsf.cpp platform seam
 * pattern: localtime_s on MSVC, localtime_r elsewhere). */
std::tm ft_to_local_tm(u64 ft) {
    std::time_t t = ft_to_unix(ft);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &t);
#else
    localtime_r(&t, &local);
#endif
    return local;
}

/* Local midnight of the FILETIME's local day (the og's day truncation:
 * FileTimeToLocalFileTime -> FileTimeToSystemTime -> zero the time of
 * day -> SystemTimeToFileTime, DRWUI FUN_1402556f0/FUN_140256830). */
u64 local_day_floor(u64 ft) {
    std::tm local = ft_to_local_tm(ft);
    local.tm_hour = 0;
    local.tm_min = 0;
    local.tm_sec = 0;
    return unix_to_ft(std::mktime(&local));
}

/* +N local days from a day-floor FILETIME, wall-clock day arithmetic
 * (tm_mday + mktime normalization = the og's COleDateTimeSpan semantics;
 * a DST transition inside the window resolves to local midnight of the
 * target day, exactly as the og's DATE-double arithmetic does). */
u64 local_day_plus(u64 day_floor_ft, int days) {
    std::tm local = ft_to_local_tm(day_floor_ft);
    local.tm_mday += days;
    local.tm_hour = 0;
    local.tm_min = 0;
    local.tm_sec = 0;
    return unix_to_ft(std::mktime(&local));
}

} // namespace

bool apply_date_bucket(Filter& f, u16 bucket, u64 now_ft, u64 custom_from_ft, u64 custom_to_ft) {
    const u64 today = local_day_floor(now_ft);
    /* the Filter's range compare is inclusive on both ends, so every
     * half-open day window [lo_day, hi_day) becomes
     * [lo_day, hi_day - 1 tick] */
    switch (bucket) {
    case DateBucket_None:
        return true;
    case DateBucket_Today:
        f.modified_from = today;
        f.modified_to = local_day_plus(today, 1) - 1;
        return true;
    case DateBucket_Yesterday:
        f.modified_from = local_day_plus(today, -1);
        f.modified_to = today - 1;
        return true;
    case DateBucket_Last7Days:
        /* og: (today-7d, today] -> local days today-6 .. today (7 days) */
        f.modified_from = local_day_plus(today, -6);
        f.modified_to = local_day_plus(today, 1) - 1;
        return true;
    case DateBucket_Last1Month:
        /* og: (today-30d, today] -> local days today-29 .. today (30 days) */
        f.modified_from = local_day_plus(today, -29);
        f.modified_to = local_day_plus(today, 1) - 1;
        return true;
    case DateBucket_Custom:
        /* og: both bounds day-truncated locally, comparison inclusive */
        f.modified_from = local_day_floor(custom_from_ft);
        f.modified_to = local_day_plus(local_day_floor(custom_to_ft), 1) - 1;
        return true;
    default:
        /* 0x104 is skipped by the og's menu; anything else is unknown:
         * no semantics invented (fail closed, filter untouched) */
        return false;
    }
}

bool apply_size_bucket(Filter& f, u16 bucket, u64 custom_min, u64 custom_max) {
    /* og FUN_140255820's exact boundaries (< bound+1 compares = inclusive
     * upper bounds; the else-chain makes the buckets mutually exclusive):
     * <= 512 KiB / (512 KiB, 1 MiB] / (1 MiB, 512 MiB] / (512 MiB, 1 GiB]
     * / > 1 GiB */
    constexpr u64 KiB = 1024, MiB = 1024 * KiB, GiB = 1024 * MiB;
    switch (bucket) {
    case SizeBucket_None:
        return true;
    case SizeBucket_To512KiB:
        f.size_min = 0;
        f.size_max = 512 * KiB;
        return true;
    case SizeBucket_To1MiB:
        f.size_min = 512 * KiB + 1;
        f.size_max = 1 * MiB;
        return true;
    case SizeBucket_To512MiB:
        f.size_min = 1 * MiB + 1;
        f.size_max = 512 * MiB;
        return true;
    case SizeBucket_To1GiB:
        f.size_min = 512 * MiB + 1;
        f.size_max = 1 * GiB;
        return true;
    case SizeBucket_Over1GiB:
        f.size_min = 1 * GiB + 1;
        f.size_max = ~u64(0);
        return true;
    case SizeBucket_Custom:
        f.size_min = custom_min;
        f.size_max = custom_max;
        return true;
    default:
        /* 0x1005 is skipped by the og's menu; anything else is unknown */
        return false;
    }
}

/* ---- M-125: the og's progress display math (the DRWUI decode) ----
 * The three-function family, ported verbatim from the decompile:
 * FUN_1401af820 (the piecewise remap + the defensive stall cap),
 * FUN_1401af950 (the type dispatch + the raw computation), and
 * FUN_1401afa60 (the monotonic publish guard). The finding's own test
 * table carried four transcription errors against these corpus
 * constants (raw 20 -> 21.8 not 22; raw 29 -> 23.6 not 23.8; raw 98
 * -> 99.0 not 99.4; raw 99 -> 99.0 not 99.99 - the stall cap only
 * fires for raw > 100, the og's dead defensive branch): the constants
 * here are the corpus's, and the tests pin the corrected table (the
 * corpus wins, per the protocol). */
double progress_display_remap(double raw_percent) {
    double out;
    if (11.0 <= raw_percent) {
        if (30.0 <= raw_percent) {
            out = raw_percent;
            if (raw_percent <= 98.0) {
                if (raw_percent <= 90.0) {
                    out = raw_percent * 0.8;
                } else {
                    out = (raw_percent - 88.0) * 2.7 + 72.0;
                }
            }
        } else {
            out = (raw_percent - 11.0) * 0.2 + 20.0;
        }
    } else {
        out = raw_percent * 2.0;
    }
    if (100.0 < out && raw_percent != 100.0)
        out = 99.99; /* the defensive stall cap (dead for raw in [0,100]) */
    return out;
}

double progress_display_percent(u32 message_type, u64 done, u64 total) {
    if (total < 1)
        return 0.0;
    const double raw = (double(done) * 100.0) / double(total);
    if (message_type == 0)
        return 0.0; /* the reset message */
    if (message_type == 1) {
        double p = progress_display_remap(raw);
        if (100.0 < p)
            p = 100.0; /* FUN_1401af950's outer clamp */
        return p;
    }
    return raw; /* the non-remapped message types */
}

double progress_display_publish(double current, double next) {
    return next > current ? next : current; /* the monotonic guard */
}

/* ---- M-126: the og's ETA model (FUN_1401af410, DRWUI :314835-314887) ---- */

EtaModel eta_compute(unsigned long long done, unsigned long long total,
                     unsigned long long elapsed_ms, unsigned long long prev_smoothed,
                     bool time_budget_variant, unsigned long long budget_seconds) {
    EtaModel out;
    /* FUN_1401af410's opening gate: done == 0, total < done, or
     * total == 0 -> -1 (no estimate) */
    if (done == 0 || total < done || total == 0)
        return out;
    const long long eta_s = (long long)(((total - done) * elapsed_ms) / done / 1000ull);
    if (eta_s <= 0)
        return out; /* the og: no positive estimate -> the field keeps -1 */
    long long rate = (long long)((total - done) / (unsigned long long)eta_s);
    if (prev_smoothed != 0)
        rate = (rate * 2 + (long long)prev_smoothed * 8) / 10; /* EMA 0.2/0.8 */
    out.smoothed_rate = (unsigned long long)rate;
    if (!time_budget_variant) {
        out.eta_seconds = eta_s; /* the type-0 leg: the plain whole-run ETA */
        return out;
    }
    /* the time-budget variant (:314873-314883): projected whole-run
     * seconds, floored at 1, then the remaining budget scaled by 0.6 */
    long long elapsed_s = (long long)(elapsed_ms / 1000ull);
    long long projected = (long long)((total * (unsigned long long)elapsed_s) / done);
    if (projected == 0)
        projected = 1;
    out.eta_seconds = (long long)(((double)((long long)budget_seconds -
                                            (long long)budget_seconds * elapsed_s / projected)) *
                                  0.6);
    return out;
}

std::string eta_format_m_s(long long seconds) {
    if (seconds < 0)
        return "3m00s"; /* the og's empty-label placeholder literal (:1527664) */
    const long long m = seconds / 60;
    const long long sec = seconds % 60;
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%02lldm%02llds", m, sec);
    return std::string(buf);
}

long long eta_countdown_tick(const std::string& label) {
    /* the og's section parse: minutes = the part before 'm', seconds =
     * the part between 'm' and 's' (the leading zero padding and the
     * countdown's unpadded re-render both parse) */
    const size_t m = label.find('m');
    const size_t sc = label.find('s');
    if (m == std::string::npos || sc == std::string::npos || m == 0 || m + 1 >= sc)
        return -1;
    const std::string mpart = label.substr(0, m);
    const std::string spart = label.substr(m + 1, sc - m - 1);
    char* mend = nullptr;
    char* send = nullptr;
    const long long minutes = std::strtoll(mpart.c_str(), &mend, 10);
    const long long seconds = std::strtoll(spart.c_str(), &send, 10);
    if (mend == mpart.c_str() || send == spart.c_str())
        return -1; /* non-numeric sections do not parse */
    /* the og's stop condition: minutes*60 - 1 + seconds < 0 (the
     * countdown reached zero -> FUN_1409cb4e0 stops the slot) */
    const long long total = minutes * 60 + seconds;
    if (total - 1 < 0)
        return -1;
    return total - 1;
}

/* ---- N-194: the og's two byte-size display formatters ---- */

std::string display_size_log(u64 bytes) {
    /* FUN_140163580/FUN_140163650 verbatim (DRWUI :264525-264583): the
     * og's opening gate is param < 1 on a SIGNED input - byte 0 (and,
     * unrepresentable here, negatives) writes nothing and returns 0. */
    if (bytes < 1)
        return "";
    const double v = double(bytes);
    char buf[32];
    if (bytes < 0x40000001ull) {
        if (bytes < 0x100001ull)
            std::snprintf(buf, sizeof(buf), "%.2f KB", v / 1024.0);
        else
            std::snprintf(buf, sizeof(buf), "%.2f MB", v / 1048576.0);
    } else {
        std::snprintf(buf, sizeof(buf), "%.2f GB", v / 1073741824.0);
    }
    return std::string(buf);
}

std::string display_size_ui(u64 bytes, int precision) {
    /* the tr-family formatter (DRWUI :1556295-1556400): B..TB tiers,
     * 'f' format, caller precision; the zero-bytes quirk forces
     * precision 0 AFTER the tier pick (the og's local_100 = 0 - "0 B"). */
    if (precision < 0)
        precision = 0;
    double v;
    const char* unit;
    if (bytes >= 0x10000000001ull) {
        v = double(bytes) / 1099511627776.0;
        unit = "TB";
    } else if (bytes >= 0x40000001ull) {
        v = double(bytes) / 1073741824.0;
        unit = "GB";
    } else if (bytes >= 0x100001ull) {
        v = double(bytes) / 1048576.0;
        unit = "MB";
    } else if (bytes >= 0x401ull) {
        v = double(bytes) / 1024.0;
        unit = "KB";
    } else {
        v = double(bytes);
        unit = "B";
    }
    if (bytes == 0)
        precision = 0;
    char buf[48];
    std::snprintf(buf, sizeof(buf), "%.*f %s", precision, v, unit);
    std::string out(buf);
    /* the og's strip tail: ".%1".arg('0', precision) - the all-zero
     * fraction spanning the FULL precision - is replaced with "" in the
     * final string ("5.00 KB" -> "5 KB"; a partial fraction is kept) */
    if (precision > 0) {
        std::string frac = ".";
        frac.append(size_t(precision), '0');
        size_t at = out.find(frac);
        if (at != std::string::npos)
            out.erase(at, frac.size());
    }
    return out;
}

} // namespace query
} // namespace cr
