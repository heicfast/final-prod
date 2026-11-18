/*
 * cr_extractplan.cpp - the structure-preserving extract plan (F-00d/F-01).
 *
 * See cr_extractplan.h for the behavioral contract and the SectorForge
 * citations. Implementation notes:
 *
 *  - single-pass emit with tail rollback: emit() appends items
 *    parents-before-children and returns whether the subtree produced
 *    anything; a subtree that produced nothing has its items truncated
 *    back (they sit contiguously at the tail), giving the F-31
 *    fast-path ("an in-use subtree with no deleted descendant is
 *    pruned whole") in O(n) total.
 *  - Active ancestors of deleted nodes become path components only
 *    (never items): the ORIGINAL path is the deleted file's full
 *    ancestry, live or not.
 *  - orphan zone: the "File Path Lost" root and its LostDir<NNNN>
 *    groups map to the "_orphaned/" base; the top of every orphaned
 *    chain (direct child of the lost root or of a LostDir group)
 *    carries the " (id N)" suffix - SF extract.rs:199-213.
 *  - carved/raw-origin nodes are excluded everywhere they hang (the
 *    raw-category flow owns them; they have no original structure).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/recovery/cr_extractplan.h"
#include "cr/tree/cr_snapshot.h"

#include <cstdio>

namespace cr {
namespace recovery {

/* ---- SF parity helpers (behavior re-derived; see header) ---------------- */

static bool is_reserved_stem_sf(const std::u16string& stem) {
    /* the reserved device set: CON PRN AUX NUL CONIN$ CONOUT$, COM1-9
     * (plus the superscript spellings Windows also reserves), LPT1-9.
     * Compared against the part before the FIRST dot, case-folded to
     * upper - "con.txt" is as reserved as "CON". */
    static const char* kNames[] = {
        "CON",  "PRN",  "AUX",  "NUL",  "CONIN$", "CONOUT$", "COM1", "COM2",
        "COM3", "COM4", "COM5", "COM6", "COM7",   "COM8",    "COM9", "LPT1",
        "LPT2", "LPT3", "LPT4", "LPT5", "LPT6",   "LPT7",    "LPT8", "LPT9",
    };
    char ascii[8];
    size_t n = 0;
    for (u16 c : stem) {
        if (n >= sizeof(ascii))
            return false; /* longer than every reserved name */
        if (c >= 0x80) {
            /* the superscript spellings of COM1..COM3: U+00B9 "1",
             * U+00B2 "2", U+00B3 "3" (the only non-ASCII reserved forms) */
            if (stem.size() != 4)
                return false;
            if (c == 0x00B9)
                ascii[n++] = '1';
            else if (c == 0x00B2)
                ascii[n++] = '2';
            else if (c == 0x00B3)
                ascii[n++] = '3';
            else
                return false;
            continue;
        }
        char ch = char(c);
        if (ch >= 'a' && ch <= 'z')
            ch = char(ch - 'a' + 'A');
        if (ch == '$' || (ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'Z'))
            ascii[n++] = ch;
        else
            return false;
    }
    if (n == 0)
        return false;
    for (const char* nm : kNames) {
        size_t k = 0;
        while (k < n && nm[k] != '\0' && ascii[k] == nm[k])
            ++k;
        if (k == n && nm[k] == '\0')
            return true; /* exact match */
    }
    return false;
}

std::u16string sanitize_component_sf(const std::u16string& in, u16 max_units) {
    if (max_units < 8)
        max_units = 8; /* keep the cap sane even under hostile config */
    std::u16string out;
    out.reserve(in.size());
    for (u16 c : in) {
        const bool bad = c == u'\\' || c == u'/' || c == u':' || c == u'*' || c == u'?' ||
                         c == u'"' || c == u'<' || c == u'>' || c == u'|' || c < 0x20;
        out.push_back(bad ? u'_' : c);
    }
    if (out.empty())
        out = u"_";
    if (out == u".")
        out = u"_";
    else if (out == u"..")
        out = u"__";
    /* reserved device names: checked on the pre-first-dot stem, then
     * neutralized by prefixing '_' (SF: "CON" -> "_CON") */
    const size_t first_dot = out.find(u'.');
    const std::u16string stem = first_dot == std::u16string::npos ? out : out.substr(0, first_dot);
    if (is_reserved_stem_sf(stem))
        out.insert(out.begin(), u'_');
    /* trailing dots and spaces become '_' (Windows strips them; two
     * different names would collide on disk after the strip) - the
     * ENTIRE trailing run, every unit of it */
    {
        size_t end = out.size();
        while (end > 0 && (out[end - 1] == u'.' || out[end - 1] == u' '))
            --end;
        for (size_t i = end; i < out.size(); ++i)
            out[i] = u'_';
    }
    /* UTF-16 cap, preserving a short extension when one exists: when
     * the truncation point would cut a <=16-unit extension that sits
     * in the tail, the extension keeps its bytes and the stem loses
     * the difference. A dot inside the kept window (or no short
     * extension) is a plain hard truncate. */
    if (out.size() > max_units) {
        const size_t last_dot = out.rfind(u'.');
        const size_t ext_len = last_dot == std::u16string::npos ? 0 : out.size() - last_dot;
        const bool short_ext = last_dot != std::u16string::npos && last_dot != 0 && ext_len <= 16;
        if (short_ext && last_dot > size_t(max_units) - ext_len) {
            const std::u16string ext = out.substr(last_dot); /* dot included */
            out.resize(size_t(max_units) - ext_len);
            out += ext;
        } else {
            out.resize(max_units);
        }
        /* post-truncation trailing dot/space fix (the cut may have
         * exposed one) */
        size_t end = out.size();
        while (end > 0 && (out[end - 1] == u'.' || out[end - 1] == u' '))
            --end;
        for (size_t i = end; i < out.size(); ++i)
            out[i] = u'_';
    }
    return out;
}

std::u16string with_id_suffix_sf(const std::u16string& name, u64 id) {
    char buf[32];
    const int n =
        std::snprintf(buf, sizeof(buf), " (id %llu)", static_cast<unsigned long long>(id));
    std::u16string suffix;
    suffix.reserve(n > 0 ? size_t(n) : size_t(0));
    for (int i = 0; i < n; ++i)
        suffix.push_back(u16(u8(buf[i])));
    const size_t last_dot = name.rfind(u'.');
    if (last_dot == std::u16string::npos || last_dot == 0)
        return name + suffix;
    return name.substr(0, last_dot) + suffix + name.substr(last_dot);
}

/* u64 -> u16 digits (std::to_u16string does not exist) */
static std::u16string to_u16_digits(u64 v) {
    char buf[24];
    const int n = std::snprintf(buf, sizeof(buf), "%llu", static_cast<unsigned long long>(v));
    std::u16string out;
    out.reserve(size_t(n > 0 ? n : 1));
    for (int i = 0; i < n; ++i)
        out.push_back(u16(u8(buf[i])));
    return out;
}

namespace extractplan {

bool is_lost_root_role(const std::u16string& role) {
    return role == u"File Path Lost" || role == u"Lost Files";
}

/* LostDir<NNNN> group folders under the lost root (M-103: name is
 * "LostDir" + the dead parent's record number). */
static bool is_lostdir_role(const std::u16string& role) {
    if (role.size() < 8)
        return false;
    if (role.compare(0, 7, u"LostDir") != 0)
        return false;
    for (size_t i = 7; i < role.size(); ++i)
        if (role[i] < u'0' || role[i] > u'9')
            return false;
    return true;
}

/* ---- live-tree plan ------------------------------------------------------ */

namespace {

struct TreeCtx {
    const PlanOptions& opt;
    PlanStats& st;
    std::vector<PlanItem>& out;
    std::u16string orphan_base; /* sanitized, WITH trailing separator */
    u32 depth = 0;
};

bool is_carved_node(const tree::FileNode* n) {
    if (n->origin() == tree::Origin::Carved)
        return true;
    const tree::FileProperties* pr = n->props();
    return pr != nullptr && pr->fs_kind == u32(FsKindCode::Raw);
}

/* emit() - single pass with tail rollback (see file header). Returns
 * true when the subtree contributed at least one item. `rel` carries a
 * trailing separator (or is empty for the output root). */
bool emit_tree(TreeCtx& c, tree::FileNode* n, const std::u16string& rel, bool orphan_zone,
               bool orphan_top) {
    const size_t mark = c.out.size();
    if (c.depth >= c.opt.max_depth) {
        ++c.st.pruned_subtrees;
        return false;
    }

    if (n->is_full()) {
        /* virtual: lost root / LostDir groups steer the orphan zone;
         * every other virtual (volume group, intermediate folder) is
         * walked without contributing a dest component - the same
         * convention as the CLI's rel_dir_of (props-less ancestors
         * never appear in destination paths). DIRECT children of the
         * lost root and of a LostDir group are the TOPS of orphaned
         * chains (they carry the " (id N)"). */
        const tree::FullNode* fn = static_cast<const tree::FullNode*>(n);
        const bool lost = is_lost_root_role(fn->role);
        const bool lostdir = orphan_zone && is_lostdir_role(fn->role);
        const bool children_top = lost || lostdir;
        bool any = false;
        ++c.depth;
        for (tree::FileNode* ch = n->first_child(); ch; ch = ch->next_sibling())
            any = emit_tree(c, ch, rel, orphan_zone || lost, children_top) || any;
        --c.depth;
        if (!any) {
            c.out.resize(mark);
            ++c.st.pruned_subtrees;
        }
        return any;
    }

    tree::FileProperties* pr = n->props();
    if (pr == nullptr)
        return false;
    if ((pr->flags & tree::NODE_FLAG_JUNK_FILTERED) != 0)
        return false; /* M-018/M-071: the hidden junk set */
    if (c.opt.exclude_carved && is_carved_node(n)) {
        ++c.st.carved_excluded;
        return false; /* carved subtrees carry no original structure */
    }

    const bool is_dir = (pr->attributes & 0x10) != 0;
    const bool active = n->deleted_state() == tree::DeletedState::Active;

    if (c.opt.deleted_only && active) {
        /* F-31 fast path: the in-use node itself is never an item */
        if (!is_dir) {
            ++c.st.pruned_active;
            return false; /* an active file leaf: skipped */
        }
        /* an in-use DIRECTORY is walked only as a path component - it
         * stays exactly when a deleted descendant is emitted (the
         * ORIGINAL path is the deleted file's full ancestry); a
         * descendant-less branch is pruned whole. Used components are
         * NOT counted in pruned_active (they were used, not pruned). */
        const std::u16string child_rel =
            rel + sanitize_component_sf(pr->name, c.opt.max_component_units) + u"\\";
        bool any = false;
        ++c.depth;
        for (tree::FileNode* ch = n->first_child(); ch; ch = ch->next_sibling())
            any = emit_tree(c, ch, child_rel, orphan_zone, false) || any;
        --c.depth;
        if (!any) {
            c.out.resize(mark);
            ++c.st.pruned_subtrees;
        }
        return any;
    }

    /* a deleted (or mode-included) item: emit it */
    PlanItem it;
    it.node = n;
    it.id = n->node_id();
    it.is_dir = is_dir;
    it.logical_size = pr->logical_size;
    std::u16string name = sanitize_component_sf(pr->name, c.opt.max_component_units);
    std::u16string item_rel = rel;
    if (orphan_top) {
        /* SF extract.rs:199-213: the top of an orphaned chain carries
         * its id - files as "name (id N).ext", dirs as "name (id N)/" */
        name = is_dir ? (name + u" (id " + to_u16_digits(it.id) + u")")
                      : with_id_suffix_sf(name, it.id);
        item_rel = c.orphan_base;
        it.orphan_root = true;
        if (is_dir)
            ++c.st.orphan_dirs;
        else
            ++c.st.orphan_files;
    }
    it.rel_dir = item_rel;
    it.name = name;
    if (is_dir)
        ++c.st.dirs;
    else {
        ++c.st.files;
        c.st.bytes_total += pr->logical_size;
    }
    c.out.push_back(std::move(it));

    bool any = true;
    if (is_dir) {
        const std::u16string child_rel = (orphan_top ? c.orphan_base : rel) + name + u"\\";
        ++c.depth;
        for (tree::FileNode* ch = n->first_child(); ch; ch = ch->next_sibling())
            any = emit_tree(c, ch, child_rel, orphan_zone, false) || any;
        --c.depth;
    }
    return any;
}

} // namespace

PlanStats plan_deleted_tree(const tree::ResultTree& t, const PlanOptions& opt,
                            std::vector<PlanItem>& out) {
    PlanStats st;
    TreeCtx c{opt, st, out, sanitize_component_sf(opt.orphan_base, opt.max_component_units) + u"\\",
              0};
    for (tree::FileNode* root : t.roots()) {
        const tree::FullNode* fn =
            root->is_full() ? static_cast<const tree::FullNode*>(root) : nullptr;
        const bool lost = fn != nullptr && is_lost_root_role(fn->role);
        emit_tree(c, root, u"", lost, false);
    }
    return st;
}

/* ---- snapshot plan ------------------------------------------------------- */

namespace {

struct SnapCtx {
    const tree::Snapshot& s;
    const PlanOptions& opt;
    PlanStats& st;
    std::vector<PlanItem>& out;
    std::u16string orphan_base;
    u32 depth = 0;
};

bool snap_is_carved(const tree::SnapNode& n) {
    return n.origin == u8(tree::Origin::Carved) || n.fs_kind == u32(FsKindCode::Raw);
}

bool emit_snap(SnapCtx& c, u32 i, const std::u16string& rel, bool orphan_zone, bool orphan_top) {
    const size_t mark = c.out.size();
    if (c.depth >= c.opt.max_depth) {
        ++c.st.pruned_subtrees;
        return false;
    }
    const tree::SnapNode& n = c.s.node(i);

    if (n.full) {
        const bool lost = is_lost_root_role(n.name);
        const bool lostdir = orphan_zone && is_lostdir_role(n.name);
        const bool children_top = lost || lostdir;
        bool any = false;
        ++c.depth;
        for (u32 ch = n.first_child; ch != tree::SNAP_NONE; ch = c.s.node(ch).next_sibling)
            any = emit_snap(c, ch, rel, orphan_zone || lost, children_top) || any;
        --c.depth;
        if (!any) {
            c.out.resize(mark);
            ++c.st.pruned_subtrees;
        }
        return any;
    }

    if ((n.flags & tree::NODE_FLAG_JUNK_FILTERED) != 0)
        return false;
    if (c.opt.exclude_carved && snap_is_carved(n)) {
        ++c.st.carved_excluded;
        return false;
    }

    const bool is_dir = (n.attributes & 0x10) != 0;
    const bool active = n.deleted == u8(tree::DeletedState::Active);

    if (c.opt.deleted_only && active) {
        if (!is_dir) {
            ++c.st.pruned_active;
            return false;
        }
        const std::u16string child_rel =
            rel + sanitize_component_sf(n.name, c.opt.max_component_units) + u"\\";
        bool any = false;
        ++c.depth;
        for (u32 ch = n.first_child; ch != tree::SNAP_NONE; ch = c.s.node(ch).next_sibling)
            any = emit_snap(c, ch, child_rel, orphan_zone, false) || any;
        --c.depth;
        if (!any) {
            c.out.resize(mark);
            ++c.st.pruned_subtrees;
        }
        return any;
    }

    PlanItem it;
    it.snap_index = i;
    it.id = n.node_id;
    it.is_dir = is_dir;
    it.logical_size = n.logical_size;
    std::u16string name = sanitize_component_sf(n.name, c.opt.max_component_units);
    std::u16string item_rel = rel;
    if (orphan_top) {
        name = is_dir ? (name + u" (id " + to_u16_digits(it.id) + u")")
                      : with_id_suffix_sf(name, it.id);
        item_rel = c.orphan_base;
        it.orphan_root = true;
        if (is_dir)
            ++c.st.orphan_dirs;
        else
            ++c.st.orphan_files;
    }
    it.rel_dir = item_rel;
    it.name = name;
    if (is_dir)
        ++c.st.dirs;
    else {
        ++c.st.files;
        c.st.bytes_total += n.logical_size;
    }
    c.out.push_back(std::move(it));

    bool any = true;
    if (is_dir) {
        const std::u16string child_rel = (orphan_top ? c.orphan_base : rel) + name + u"\\";
        ++c.depth;
        for (u32 ch = n.first_child; ch != tree::SNAP_NONE; ch = c.s.node(ch).next_sibling)
            any = emit_snap(c, ch, child_rel, orphan_zone, false) || any;
        --c.depth;
    }
    return any;
}

} // namespace

PlanStats plan_deleted_snapshot(const tree::Snapshot& snap, const PlanOptions& opt,
                                std::vector<PlanItem>& out) {
    PlanStats st;
    SnapCtx c{
        snap, opt, st, out, sanitize_component_sf(opt.orphan_base, opt.max_component_units) + u"\\",
        0};
    for (u32 i = 0; i < snap.node_count(); ++i) {
        if (snap.node(i).parent == tree::SNAP_NONE)
            emit_snap(c, i, u"", false, false);
    }
    return st;
}

} // namespace extractplan
} // namespace recovery
} // namespace cr
