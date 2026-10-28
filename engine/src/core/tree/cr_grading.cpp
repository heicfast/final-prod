/*
 * cr_grading.cpp - the post-scan CRecoverability grading pass (S-023).
 *
 * Implementation of the decision tree decoded in
 * diff-to-fix/evidence/decompiled-quotes/s4-crecoverability-grading-engine.md
 * (see cr_grading.h for the full corpus citation). The undecoded
 * sub-rules (the og device vt+0xb8 / vt+0x100 == 0xa8 flags; the exact
 * FUN_14038c470 deep-validation content) are NOT adopted - a wrong analog
 * would mis-grade; the structural rules are exact. See the S-023
 * Resolution for the deliberate-non-adoption register.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/tree/cr_grading.h"
#include <algorithm>
#include <cstring>

namespace cr {
namespace tree {
namespace grading {

/* ---- the 169-entry deep-validation extension table -------------------
 * Extracted from the corpus DRWUI.exe .data VA 0x141bb0500 (169 x 9-byte
 * entries {8-char NUL-padded key, flag byte 0}; first key "Raw"). The
 * lookup site (DRWUI_decompiled.c:605297-605304) inserts every entry into
 * a map consulted at :605740-605741 (found != end -> the deep path).
 * Sorted for binary search; every flag byte is 0 (pure membership). */
static const char* const kGradingExtensions[] = {
    "3DS",  "3FR",    "3GP",  "7Z",     "AAC",     "ACCDB", "ADP",  "AEP",   "AI",       "AIFF",
    "ANI",  "APE",    "ARW",  "ASF",    "AU",      "AVI",   "BMP",  "BRAW",  "CAB",      "CDR",
    "CHM",  "CLK",    "CMX",  "COMMON", "CPT",     "CR2",   "CR3",  "CRM",   "CRW",      "CSL",
    "CUR",  "DBX",    "DCR",  "DLL",    "DMG",     "DNG",   "DOC",  "DOCX",  "DPX",      "DRP",
    "DWG",  "DXF",    "EDDX", "EMF",    "EPS",     "EPUB",  "ERF",  "EXE",   "FCPEVENT", "FFF",
    "FH11", "FLV",    "GIF",  "GPR",    "GPX",     "GZ",    "HEIC", "HEIF",  "HFP",      "HIK",
    "HLP",  "HTML",   "HWP",  "HWPX",   "ICO",     "INSP",  "INSV", "JP2",   "JPF",      "JPG",
    "JPM",  "KDC",    "KEY",  "LZH",    "M1V",     "M2T",   "M2TS", "M4A",   "M4P",      "M4V",
    "MAT",  "MAX",    "MDB",  "MDC",    "MEF",     "MID",   "MKV",  "MMAP",  "MNY",      "MOBI",
    "MOF",  "MOV",    "MP3",  "MP4",    "MPG",     "MPP",   "MRW",  "MTS",   "MUSE",     "MUSX",
    "MXF",  "NEF",    "NEV",  "NRW",    "NUMBERS", "ODP",   "ODS",  "ODT",   "OGG",      "OLE",
    "ONE",  "ORF",    "OSP",  "PAGES",  "PDF",     "PEF",   "PIX",  "PNG",   "POS",      "PPT",
    "PPTX", "PSD",    "PSP",  "PST",    "PTX",     "PUB",   "QPW",  "R3D",   "RAF",      "RAR",
    "RAW",  "RIFF",   "RM",   "RMVB",   "RTF",     "RVT",   "RW2",  "Raw",   "SHW",      "SIT",
    "SITX", "SLN",    "SR2",  "SRW",    "STL",     "SVG",   "SWF",  "TAR",   "TIF",      "TTF",
    "VCF",  "VCPROJ", "VPJ",  "VSD",    "WAV",     "WEBM",  "WEBP", "WFP",   "WMA",      "WMF",
    "WMV",  "WPD",    "WPG",  "X3F",    "XIB",     "XLS",   "XLSX", "XMIND", "ZIP",
};

bool extension_in_grading_table(const std::string& ext_utf8) {
    if (ext_utf8.empty())
        return false;
    /* the table stores UPPERCASE keys; normalize any input case */
    std::string key;
    key.reserve(ext_utf8.size());
    for (char c : ext_utf8) {
        if (c >= 'a' && c <= 'z')
            c = char(c - 'a' + 'A');
        key.push_back(c);
    }
    const char* const* first = kGradingExtensions;
    const char* const* last = first + sizeof(kGradingExtensions) / sizeof(kGradingExtensions[0]);
    const char* const* it = std::lower_bound(
        first, last, key.c_str(), [](const char* m, const char* k) { return strcmp(m, k) < 0; });
    return it != last && strcmp(*it, key.c_str()) == 0;
}

size_t grading_table_entry_count() {
    return sizeof(kGradingExtensions) / sizeof(kGradingExtensions[0]);
}

/* ---- device context ---------------------------------------------------- */

const DeviceSlot* GradeContext::slot_for(u64 abs_offset) const {
    if (devices.empty())
        return nullptr;
    /* the slot whose [base, next base) contains the offset; devices are
     * sorted by base */
    size_t lo = 0, hi = devices.size();
    while (lo < hi) {
        const size_t mid = (lo + hi) / 2;
        if (devices[mid].base <= abs_offset)
            lo = mid + 1;
        else
            hi = mid;
    }
    if (lo == 0)
        return nullptr; /* before the first slice: not owned */
    const DeviceSlot* s = &devices[lo - 1];
    return s;
}

u32 GradeContext::fs_kind_of(const tree::FileNode* n) const {
    /* props->fs_kind is the engine-recorded truth (per node, accurate on
     * whole-disk multi-FS scans); the owning slot's kind is the fallback
     * for nodes that never recorded one. */
    const tree::FileProperties* pr = n->props();
    if (pr && pr->fs_kind != 0)
        return pr->fs_kind;
    if (pr && !pr->extents.empty()) {
        const DeviceSlot* s = slot_for(pr->extents.front().offset);
        if (s && s->fs_kind != 0)
            return s->fs_kind;
    }
    return 0;
}

u32 GradeContext::disk_ordinal_of(const tree::FileNode* n) const {
    const tree::FileProperties* pr = n->props();
    if (pr && !pr->extents.empty()) {
        const DeviceSlot* s = slot_for(pr->extents.front().offset);
        if (s)
            return s->disk_ordinal;
    }
    return 0; /* extentless: the first device (the og's node always has
               * a volume parent; ordinal 0 is the conservative default) */
}

/* ---- extent-end twin keys ----------------------------------------------
 * og: base + len + device_index * unit, the index offset by -2 for
 * FAT-family devices (kinds 1/4) - an ADDRESS offset that keeps the
 * FAT/exFAT twin space distinct from other families (FUN_14038ac90
 * :605752-605768). Same key + same category -> twins (FUN_14038ba60). */
namespace {
struct TwinKey {
    u64 key;
    query::Category cat;
    tree::FileNode* node;
};

u64 twin_key_of(const tree::FileNode* n, const GradeContext& ctx) {
    const tree::FileProperties* pr = n->props();
    u64 base = 0, len = 0;
    if (pr && !pr->extents.empty()) {
        base = pr->extents.front().offset;
        len = pr->extents.front().length;
    }
    /* M-133: the og shifts kinds {1, 4} ONLY (DRWUI :605752-756 and the
     * device-walk twin :605924-927, re-read first-hand): kind 1 = FAT16
     * and kind 4 = the EXT2/3 family code. FAT12 (0), FAT32 (2), exFAT
     * (7), NTFS (3), HFS+ (8), EXT4 (9), ReFS (10), ZFS (13) all keep
     * their ordinals unshifted. The pre-fix core shifted the whole FAT
     * family {0,1,2,7} - four of five members wrong. The EXT2/3 leg
     * (kind 4) is not expressible in the core's FsKindCode today: the
     * ext engine stamps the whole family Ext4 (9), which the og does
     * NOT shift - the kind-reporting gap is tracked as OQ-S52-1 (the
     * og's ext2/3-vs-ext4 classification rule is undecoded in the
     * corpus; adding a guessed classifier would be hallucinated
     * parity). */
    const u32 fs = ctx.fs_kind_of(n);
    const bool shifted = fs == u32(FsKindCode::Fat16);
    const i64 index = i64(ctx.disk_ordinal_of(n)) - (shifted ? 2 : 0);
    return u64(i64(base + len) + index * i64(kDeviceUnit));
}

/* extension of a u16 name, UPPERCASED ASCII - the stored case of the
 * 169-entry table ("DOC", "PST", "CR3"...; dumped from .data 0x141bb0500).
 * The og's normalizer (FUN_140389400) must match the table's case for
 * the map lookup to hit; ours uppercases for the same reason. NOTE:
 * query::category_for_extension has its OWN lowercase table (the
 * FileFilter.xml families) - two tables, two cases, by corpus truth. */
std::string ext_of_name(const tree::FileProperties* pr) {
    std::string s;
    if (!pr)
        return s;
    bool after_dot = false;
    for (u16 c : pr->name) {
        if (c == u'.') {
            after_dot = true;
            s.clear();
            continue;
        }
        if (!after_dot)
            continue;
        char ch = char(c);
        if (ch >= 'a' && ch <= 'z')
            ch = char(ch - 'a' + 'A');
        s.push_back(ch);
    }
    return s;
}
} // namespace

/* ---- the decision rules (one node, no twin context) -------------------- */

namespace {

/* the og decision tree minus the twin detection (which needs the global
 * key map). Returns the grade to apply for the solo rules. */
bool solo_rules(tree::FileNode* n, const GradeContext& ctx, AssistantStats* stats) {
    const tree::FileProperties* pr = n->props();
    const u64 size = pr ? pr->logical_size : 0;
    if (size < 1) {
        n->set_grade(Grade::Low); /* FUN_14038ac90 :605726-605727 */
        return true;
    }
    /* email-category fast path (og category 0xC -> 2, guarded by != 2) */
    const query::Category cat =
        pr ? query::category_for_extension(ext_of_name(pr)) : query::Category::Other;
    if (cat == query::Category::Email && n->grade() != Grade::High) {
        n->set_grade(Grade::High);
        return true;
    }
    /* existing files: grade 2 at insert when size >= 1 (FUN_14037a8a0
     * family, bit 6 of +0x72). Our DeletedState::Active == existing. */
    if (n->deleted_state() == tree::DeletedState::Active) {
        n->set_grade(Grade::High);
        return true;
    }
    /* raw/carve device rule (FUN_14038b0e0 vt+0xe8 branch): small carved
     * results are intact (High), large ones presumptively partial (Low). */
    if (n->origin() == tree::Origin::Carved || (pr && pr->fs_kind == u32(FsKindCode::Raw))) {
        n->set_grade(size < 0x400 ? Grade::High : Grade::Low);
        return true;
    }
    /* FS in {NTFS, ReFS, Btrfs}: the device-walk job's rules. NTFS:
     * device index in [0,9) -> 2 + the QuickScanDeleteExtend tick.
     * (vt+0xb8 / vt+0x100 sub-rules: UNDECODED - not adopted.) */
    const u32 fs = ctx.fs_kind_of(n);
    if (fs == u32(FsKindCode::Ntfs)) {
        const u32 ordinal = ctx.disk_ordinal_of(n);
        if (ordinal < 9) {
            n->set_grade(Grade::High);
            if (stats)
                ++stats->quick_scan_delete_extend; /* DAT_141be5818 */
        }
        return true; /* NTFS processed; twins are a FAT-family path */
    }
    if (fs == u32(FsKindCode::Refs) || fs == u32(FsKindCode::Btrfs)) {
        /* S-061: the og's insert-time hook (FUN_14038a470 :605481-497)
         * grades every non-existing ReFS/Btrfs node High (grade 2) with
         * the 5830/5828 gauge ticks - the completion pass excludes FS
         * {3, 10, 0xB} from the generic path precisely because insert
         * already graded them (the pre-fix comment's "device walk
         * covers these" premise was disproven by the decode). Placed
         * after the Active rule (existing files already grade High
         * there - the og gates the hook on non-existing nodes); the
         * final gauge pass counts these as matched (the tick
         * equivalent). */
        n->set_grade(Grade::High);
        return true;
    }
    /* generic path continues to twin detection in grade_tree */
    return false;
}

} // namespace

/* ---- the pass ------------------------------------------------------------ */

void grade_tree(tree::ResultTree& t, const GradeContext& ctx, AssistantStats& out) {
    /* pass 1: solo rules + collect twin candidates (extension-gated) */
    std::vector<TwinKey> twins;
    std::vector<tree::FileNode*> stack;
    for (tree::FileNode* r : t.roots())
        stack.push_back(r);
    while (!stack.empty()) {
        tree::FileNode* n = stack.back();
        stack.pop_back();
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling())
            stack.push_back(c);
        if (n->is_full() || !n->props())
            continue;
        const tree::FileProperties* pr = n->props();
        ++out.graded_count;

        /* assistant counters (Origin x DeletedState + categories) */
        ++out.total_file_count;
        const query::Category cat = query::category_for_extension(ext_of_name(pr));
        out.category_counts[size_t(cat) > 0 && size_t(cat) <= 8 ? size_t(cat) - 1 : 5] += 1;
        switch (n->origin()) {
        case tree::Origin::Carved:
            ++out.raw_count;
            break;
        case tree::Origin::Lost:
            if (n->deleted_state() == tree::DeletedState::Active)
                ++out.lost_partition_exist_file_count;
            else
                ++out.lost_partition_delete_file_count;
            break;
        case tree::Origin::Virtual:
            break; /* virtuals are not scan files */
        default:
            if (n->deleted_state() == tree::DeletedState::Active)
                ++out.quick_scan_exist_file_count;
            else
                ++out.quick_scan_delete_file_count;
            break;
        }

        const bool generic_path = solo_rules(n, ctx, &out);
        if (generic_path)
            continue;

        /* generic path (FS not in {NTFS, ReFS, Btrfs}): the extension
         * table gates the twin detection. S-058: the og's gate is
         * NOT-IN-TABLE - FUN_1401bebe0 (:324923) is the iterator
         * INEQUALITY comparator, so `if (cVar1 == '\0')` at :605743
         * enters the deep-validation block exactly when the set lookup
         * MISSED; the control case (:585885-905, the found-in-set ||
         * name-in-consts idiom) calibrates the direction, and the
         * runtime set's "" quirk (the ctor's 2000-iteration init loop
         * inserts the empty string after the 169 real keys) makes
         * dot-less files skip too. The pre-fix gate ran the twins for
         * IN-table extensions - the s5 evidence's inverted polarity
         * line, implemented and test-pinned from that misreading. */
        const std::string gext = ext_of_name(pr);
        if (gext.empty() || extension_in_grading_table(gext))
            continue;
        TwinKey tk;
        tk.key = twin_key_of(n, ctx);
        tk.cat = cat;
        tk.node = n;
        twins.push_back(tk);
    }

    /* pass 2: twin detection - same extent-end key + same category ->
     * both nodes get the overlap flag + grade 2 (FUN_14038ba60 sets both
     * bit-2 flags and upgrades; the comparator at :605774-605781 matches
     * keys AND categories before flagging). */
    std::sort(twins.begin(), twins.end(), [](const TwinKey& a, const TwinKey& b) {
        return a.key != b.key ? a.key < b.key : size_t(a.cat) < size_t(b.cat);
    });
    for (size_t i = 1; i < twins.size(); ++i) {
        if (twins[i].key == twins[i - 1].key && twins[i].cat == twins[i - 1].cat) {
            twins[i].node->set_overlap();
            twins[i - 1].node->set_overlap();
        }
    }

    /* deep-surface census: nodes under lost roots (the "File Path Lost"
     * role + the legacy "Lost Files" name both carry "Lost"; the remnant
     * harvests - our analog of the og's deep-scan-only files) */
    for (tree::FileNode* r : t.roots()) {
        if (!r->is_full())
            continue;
        const tree::FullNode* f = static_cast<const tree::FullNode*>(r);
        if (f->role.find(u"Lost") == std::u16string::npos)
            continue;
        std::vector<tree::FileNode*> sub;
        sub.push_back(r);
        while (!sub.empty()) {
            tree::FileNode* n = sub.back();
            sub.pop_back();
            for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling())
                sub.push_back(c);
            if (!n->is_full() && n->props())
                ++out.deep_scan_file_count;
        }
    }

    /* the recoverability gauge + small-result heuristic
     * (FUN_1401d7110 :340824-340847: total < 100 -> matched forced 0) */
    out.total = out.total_file_count;
    stack.clear();
    for (tree::FileNode* r : t.roots())
        stack.push_back(r);
    while (!stack.empty()) {
        tree::FileNode* n = stack.back();
        stack.pop_back();
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling())
            stack.push_back(c);
        if (!n->is_full() && n->props() && n->grade() == Grade::High)
            ++out.matched;
    }
    out.small_result = out.total < 100;
    if (out.small_result)
        out.matched = 0;
}

bool regrade_node(tree::FileNode* n, const GradeContext& ctx) {
    if (!n || n->is_full() || !n->props())
        return false;
    /* the og re-runs the tree only for grade-1 nodes (grade not in
     * {0, 2} - FUN_1401d7110 :340852-340873) */
    if (n->grade() != Grade::Average)
        return false;
    solo_rules(n, ctx, nullptr);
    return true;
}

} // namespace grading
} // namespace tree
} // namespace cr
