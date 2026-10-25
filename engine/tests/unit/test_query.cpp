/* test_query.cpp - G-12 acceptance: core-side query/filter/sort/paging.
 * Correctness against a brute-force reference on randomized fixtures plus
 * deterministic cases; the 1M-node performance gate lives in
 * test_query_perf.cpp (CI-enforced < 200 ms).
 *
 * Licensed under the MIT license. See LICENSE.
 */
#include <gtest/gtest.h>
#include "cr/query/cr_query.h"
#include <algorithm>
#include <random>
#include <set>
#include <string>

using namespace cr;
using namespace cr::query;

namespace {

tree::FileNode* add_file(tree::ResultTree& t, tree::FileNode* parent, const char* name, u64 size,
                         u64 modified, u32 confidence, bool deleted, tree::Origin origin) {
    auto* p = new tree::FileProperties();
    for (const char* q = name; *q; ++q)
        p->name.push_back(u16(u8(*q)));
    p->logical_size = size;
    p->times.modified = modified;
    t.add_file(parent, p);
    p->release();
    tree::FileNode* n = parent->first_child();
    /* find the just-added node (last child) */
    while (n->next_sibling())
        n = n->next_sibling();
    n->set_confidence(tree::Confidence(confidence));
    n->set_deleted_state(deleted ? tree::DeletedState::Deleted : tree::DeletedState::Active);
    n->set_origin(origin);
    return n;
}

struct Ref {
    std::string name, path;
    u64 size, modified;
    u32 origin, deleted, confidence;
    const tree::FileNode* node;
};

/* brute-force reference: linear scan with the same predicates, page
 * computed independently of the implementation */
std::vector<Ref> brute(const tree::ResultTree& t, const Filter& f, bool include_all) {
    std::vector<Ref> out;
    std::function<void(tree::FileNode*, std::string)> walk = [&](tree::FileNode* n,
                                                                 std::string path) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            const tree::FileProperties* pr = c->props();
            if (c->is_full() || !pr) {
                walk(c, path + "/" + std::to_string(c->node_id()));
                continue;
            }
            Ref r;
            r.name = c->name_utf8();
            r.size = pr->logical_size;
            r.modified = pr->times.modified;
            r.origin = u32(c->origin());
            r.deleted = u32(c->deleted_state());
            r.confidence = u32(c->confidence());
            r.node = c;
            r.path = path + "/" + r.name;
            bool ok =
                include_all ||
                (r.size >= f.size_min && r.size <= f.size_max && r.modified >= f.modified_from &&
                 r.modified <= f.modified_to && r.confidence >= f.confidence_min &&
                 (!f.deleted_only || r.deleted != 0) && (f.origin_mask & (1u << r.origin)) != 0);
            if (ok)
                out.push_back(r);
            if (c->first_child())
                walk(c, r.path);
        }
    };
    for (auto* r : t.roots())
        walk(r, "");
    return out;
}

} // namespace

TEST(Query, CategoryMap) {
    EXPECT_EQ(Category::Image, category_for_extension("JPG"));
    EXPECT_EQ(Category::Image, category_for_extension("png"));
    EXPECT_EQ(Category::Video, category_for_extension("mp4"));
    EXPECT_EQ(Category::Audio, category_for_extension("wav"));
    EXPECT_EQ(Category::Document, category_for_extension("pdf"));
    EXPECT_EQ(Category::Archive, category_for_extension("7z"));
    EXPECT_EQ(Category::Other, category_for_extension("xyzzy"));
}

/* M-023: the corpus FileFilter.xml category model (8 categories, 1,481
 * unique extensions) vs the old 6-category/64-extension hand table. */
TEST(Query, EmailCategoryExists) {
    /* the og's Email class: pst/eml/emlx/dbx/nsf/edb/... (19 extensions) */
    EXPECT_EQ(Category::Email, category_for_extension("pst"));
    EXPECT_EQ(Category::Email, category_for_extension("PST"));
    EXPECT_EQ(Category::Email, category_for_extension("eml"));
    EXPECT_EQ(Category::Email, category_for_extension("emlx"));
    EXPECT_EQ(Category::Email, category_for_extension("dbx"));
    EXPECT_EQ(Category::Email, category_for_extension("nsf"));
}

TEST(Query, BookmarksCategory) {
    EXPECT_EQ(Category::Bookmarks, category_for_extension("bookmarks"));
}

TEST(Query, CameraRawClassified) {
    /* camera-RAW extensions the old 64-entry table did not carry:
     * a "Graphic only" filter dropped exactly these */
    EXPECT_EQ(Category::Image, category_for_extension("cr3"));
    EXPECT_EQ(Category::Image, category_for_extension("x3f"));
    EXPECT_EQ(Category::Image, category_for_extension("srw"));
    EXPECT_EQ(Category::Image, category_for_extension("3fr"));
    EXPECT_EQ(Category::Image, category_for_extension("erf"));
    EXPECT_EQ(Category::Image, category_for_extension("mef"));
    EXPECT_EQ(Category::Image, category_for_extension("pef"));
    EXPECT_EQ(Category::Image, category_for_extension("mrw"));
    EXPECT_EQ(Category::Image, category_for_extension("mos"));
}

TEST(Query, TableCountMatchesCorpusUniques) {
    /* regeneration-drift guard: the corpus FileFilter.xml at 03ad46c
     * carries 1,481 unique extensions (1,489 entries, 8 cross-listed) */
    EXPECT_EQ(size_t(1481), filefilter_entry_count());
}

TEST(Query, CrossListedFirstWriterWins) {
    /* the og's document order decides the cross-listed extensions
     * (verified against the corpus XML first-hand): ari and md appear in
     * two categories - Video/Document first, then Archive; gz/xz/jar/uzip
     * repeat within/into Archive only (their first listing IS Archive). */
    EXPECT_EQ(Category::Document, category_for_extension("md"));
    EXPECT_EQ(Category::Video, category_for_extension("ari"));
    EXPECT_EQ(Category::Archive, category_for_extension("gz"));
    EXPECT_EQ(Category::Archive, category_for_extension("xz"));
    EXPECT_EQ(Category::Archive, category_for_extension("jar"));
}

TEST(Query, FilterSortPageCorrectness) {
    tree::ResultTree t;
    auto* found = t.add_root(u"Found");
    auto* sub = add_file(t, found, "sub", 0, 0, 0, false, tree::Origin::Virtual);
    add_file(t, sub, "b.jpg", 100, 1000, 1, false, tree::Origin::Structured);
    add_file(t, sub, "a.txt", 200, 2000, 2, true, tree::Origin::Structured);
    add_file(t, found, "c.mp4", 300, 3000, 1, false, tree::Origin::Carved);
    add_file(t, found, "d.zip", 50, 4000, 3, true, tree::Origin::Carved);
    t.refresh_stats();

    /* all: "sub" carries no 0x10 attribute in this fixture, so it is a
     * size-0 file row - 5 rows total */
    {
        Result r = run(t, Query{});
        EXPECT_EQ(u64(5), r.total_matched);
        EXPECT_EQ(5u, r.rows.size()); /* default limit 100 */
    }
    /* size filter */
    {
        Query q;
        q.filter.size_min = 100;
        q.filter.size_max = 250;
        const Result r = run(t, q);
        EXPECT_EQ(u64(2), r.total_matched); /* b.jpg + a.txt */
    }
    /* deleted only */
    {
        Query q;
        q.filter.deleted_only = true;
        const Result r = run(t, q);
        EXPECT_EQ(u64(2), r.total_matched); /* a.txt + d.zip */
    }
    /* confidence min 2 (PartialRisk floor): a.txt(2), d.zip(3) pass;
     * sub(0), b.jpg(1), c.mp4(1) excluded */
    {
        Query q;
        q.filter.confidence_min = 2;
        const Result r = run(t, q);
        EXPECT_EQ(u64(2), r.total_matched);
    }
    /* category mask: images only */
    {
        Query q;
        q.filter.category_mask = 1u << u32(Category::Image);
        const Result r = run(t, q);
        ASSERT_EQ(u64(1), r.total_matched);
        EXPECT_EQ("b.jpg", r.rows[0].name_utf8);
        EXPECT_EQ(u32(Category::Image), r.rows[0].category);
        EXPECT_EQ(std::string("Found/sub/b.jpg"), r.rows[0].path_utf8);
    }
    /* name substring, case-insensitive */
    {
        Query q;
        q.filter.name_substring = ".MP";
        const Result r = run(t, q);
        ASSERT_EQ(u64(1), r.total_matched);
        EXPECT_EQ("c.mp4", r.rows[0].name_utf8);
    }
    /* extension list */
    {
        Query q;
        q.filter.extensions = {".jpg", ".zip"};
        const Result r = run(t, q);
        EXPECT_EQ(u64(2), r.total_matched);
    }
    /* count_only: no rows */
    {
        Query q;
        q.count_only = true;
        const Result r = run(t, q);
        EXPECT_EQ(u64(5), r.total_matched);
        EXPECT_TRUE(r.rows.empty());
    }
    /* sort by size desc + paging */
    {
        Query q;
        q.sort = SortKey::Size;
        q.dir = SortDir::Desc;
        q.offset = 1;
        q.limit = 2;
        const Result r = run(t, q);
        EXPECT_EQ(u64(5), r.total_matched);
        ASSERT_EQ(2u, r.rows.size());
        EXPECT_EQ("a.txt", r.rows[0].name_utf8); /* 200 */
        EXPECT_EQ("b.jpg", r.rows[1].name_utf8); /* 100 */
        /* order: c.mp4(300), a.txt(200), b.jpg(100), sub/d(50), d.zip(50) */
    }
    /* sort by name asc: stable, ties keep emission order */
    {
        Query q;
        q.sort = SortKey::Name;
        q.dir = SortDir::Asc;
        q.limit = 0; /* all */
        const Result r = run(t, q);
        ASSERT_EQ(5u, r.rows.size());
        EXPECT_EQ("a.txt", r.rows[0].name_utf8);
        EXPECT_EQ("b.jpg", r.rows[1].name_utf8);
        EXPECT_EQ("c.mp4", r.rows[2].name_utf8);
        EXPECT_EQ("d.zip", r.rows[3].name_utf8);
        EXPECT_EQ("sub", r.rows[4].name_utf8);
    }
    /* offset beyond end */
    {
        Query q;
        q.offset = 100;
        const Result r = run(t, q);
        EXPECT_EQ(u64(5), r.total_matched);
        EXPECT_TRUE(r.rows.empty());
    }
    /* origin mask: carved only */
    {
        Query q;
        q.filter.origin_mask = 1u << u32(tree::Origin::Carved);
        const Result r = run(t, q);
        EXPECT_EQ(u64(2), r.total_matched); /* c.mp4 + d.zip */
    }
}

/* og AdvanceFilterCondition path group parity (API-SURFACE-AUDIT op #20):
 * the row's FULL path (root role + ancestors + own name, '/'-joined) must
 * start with the prefix, ASCII-case-insensitive; the walk prunes subtrees
 * whose accumulated path is prefix-incompatible with the filter. */
TEST(Query, PathPrefixFilterMatchesAndPrunes) {
    tree::ResultTree t;
    auto* found = t.add_root(u"Found");
    auto* sub = add_file(t, found, "sub", 0, 0, 0, false, tree::Origin::Virtual);
    add_file(t, sub, "b.jpg", 100, 1000, 1, false, tree::Origin::Structured);
    add_file(t, sub, "a.txt", 200, 2000, 2, true, tree::Origin::Structured);
    auto* other = add_file(t, found, "other", 0, 0, 0, false, tree::Origin::Virtual);
    add_file(t, other, "e.png", 10, 10, 1, false, tree::Origin::Structured);
    add_file(t, found, "c.mp4", 300, 3000, 1, false, tree::Origin::Carved);
    t.refresh_stats();

    /* no filter: sub, b.jpg, a.txt, other, e.png, c.mp4 = 6 rows */
    {
        const Result r = run(t, Query{});
        EXPECT_EQ(u64(6), r.total_matched);
    }
    /* directory-prefix: the dir row itself + both children = 3 rows */
    {
        Query q;
        q.filter.path_prefix = "Found/sub";
        const Result r = run(t, q);
        EXPECT_EQ(u64(3), r.total_matched);
    }
    /* ASCII-case-insensitive on every segment */
    {
        Query q;
        q.filter.path_prefix = "found/SUB";
        const Result r = run(t, q);
        EXPECT_EQ(u64(3), r.total_matched);
    }
    /* mid-name prefix: "Found/s" matches sub (+children) but not other */
    {
        Query q;
        q.filter.path_prefix = "Found/s";
        const Result r = run(t, q);
        EXPECT_EQ(u64(3), r.total_matched);
    }
    /* the sibling branch */
    {
        Query q;
        q.filter.path_prefix = "Found/ot";
        const Result r = run(t, q);
        EXPECT_EQ(u64(2), r.total_matched); /* other + e.png */
    }
    /* full-file prefix narrows to exactly one row */
    {
        Query q;
        q.filter.path_prefix = "Found/sub/b";
        const Result r = run(t, q);
        ASSERT_EQ(u64(1), r.total_matched);
        EXPECT_EQ("b.jpg", r.rows[0].name_utf8);
    }
    /* root-segment prefix = the whole root's subtree */
    {
        Query q;
        q.filter.path_prefix = "Found";
        const Result r = run(t, q);
        EXPECT_EQ(u64(6), r.total_matched);
    }
    /* non-matching prefix: zero rows, and the pruned walk still runs clean */
    {
        Query q;
        q.filter.path_prefix = "Found/zzz";
        const Result r = run(t, q);
        EXPECT_EQ(u64(0), r.total_matched);
        EXPECT_TRUE(r.rows.empty());
    }
    /* composes with the other filter groups (deleted + prefix) */
    {
        Query q;
        q.filter.path_prefix = "Found/sub";
        q.filter.deleted_only = true;
        const Result r = run(t, q);
        EXPECT_EQ(u64(1), r.total_matched); /* a.txt only (b.jpg active) */
    }
}

TEST(Query, RandomizedAgainstReference) {
    std::mt19937_64 rng(12345);
    for (int round = 0; round < 20; ++round) {
        tree::ResultTree t;
        auto* root = t.add_root(u"Found");
        const int n = 200;
        for (int i = 0; i < n; ++i) {
            add_file(t, root, ("f" + std::to_string(i) + (i % 3 ? ".jpg" : ".dat")).c_str(),
                     u64(rng() % 10000), u64(rng() % 1000000), u32(rng() % 1000), (rng() & 1) != 0,
                     tree::Origin(rng() & 3));
        }
        t.refresh_stats();

        Query q;
        q.filter.size_min = 1000;
        q.filter.size_max = 8000;
        q.filter.confidence_min = 200;
        if (round % 2)
            q.filter.deleted_only = true;
        const Result r = run(t, q);

        const auto ref = brute(t, q.filter, false);
        EXPECT_EQ(u64(ref.size()), r.total_matched);
        /* rows (emission order == reference order, no sort) */
        for (size_t i = 0; i < ref.size() && i < r.rows.size(); ++i)
            EXPECT_EQ(ref[i].name, r.rows[i].name_utf8) << "round " << round << " row " << i;
    }
}

/* ------------------------------------------------------------------ */
/* M-029 (this leg's slice): the Type sort key + the modified-unit pin  */
/* ------------------------------------------------------------------ */

TEST(Query, TypeSortKeyOrdersByCategoryThenExtension) {
    /* M-029: the og's results-view Type column (column 3). Ordering:
     * category in the FileFilter.xml iconIndex display order
     * (Graphic/Document/Email/Audio/Video/Other/Archive/Bookmarks),
     * then extension, then name; no-category rows last. */
    tree::ResultTree t;
    auto* root = t.add_root(u"Found");
    auto add = [&t](tree::FileNode* parent, const char* nm) {
        auto* p = new tree::FileProperties();
        for (const char* q = nm; *q; ++q)
            p->name.push_back(u16(u8(*q)));
        p->logical_size = 10;
        auto* n = t.add_file(parent, p);
        p->release();
        return n;
    };
    /* emission order deliberately scrambled vs the expected sort */
    add(root, "zeta.mp3");       /* Audio -> slot 3 */
    add(root, "alpha.txt");      /* Document -> slot 1 */
    add(root, "omega.png");      /* Image(Graphic) -> slot 0 */
    add(root, "beta.htm");       /* Document, ext htm (before txt) */
    add(root, "gamma.mkv");      /* Video -> slot 4 */
    add(root, "noext");          /* no extension -> Other? no: no dot ->
                                  * cat_of -> Other (unknown ext) -> slot 5 */
    add(root, "delta.zip");      /* Archive -> slot 6 */
    add(root, "alpha2.txt");     /* Document, ext txt, name after beta.htm's
                                  * tie-break demo */
    query::Query q;
    q.sort = query::SortKey::Type;
    q.dir = query::SortDir::Asc;
    query::Result r;
    r = query::run(t, q);
    ASSERT_EQ(u32(r.rows.size()), 8u);
    /* expected: omega.png (Image) | beta.htm, alpha.txt, alpha2.txt
     * (Document: htm < txt, then name) | zeta.mp3 (Audio) | gamma.mkv
     * (Video) | noext (Other) | delta.zip (Archive) */
    EXPECT_EQ(r.rows[0].name_utf8, "omega.png");
    EXPECT_EQ(r.rows[1].name_utf8, "beta.htm");
    EXPECT_EQ(r.rows[2].name_utf8, "alpha.txt");
    EXPECT_EQ(r.rows[3].name_utf8, "alpha2.txt");
    EXPECT_EQ(r.rows[4].name_utf8, "zeta.mp3");
    EXPECT_EQ(r.rows[5].name_utf8, "gamma.mkv");
    EXPECT_EQ(r.rows[6].name_utf8, "noext");
    EXPECT_EQ(r.rows[7].name_utf8, "delta.zip");
}

TEST(Query, ModifiedUnitIsFiletimeEverywhere) {
    /* M-029: the frozen `tree --json` contract claimed `modified` is Unix
     * epoch SECONDS - false. The engines store FILETIME 100-ns since
     * 1601 (cr_fat_quick.cpp:256-259: unix*1e7 + 116444736000000000);
     * the query Row carries the node's value VERBATIM. This pin + the
     * ground-truth-contracts unit fix close the doc lie: a consumer
     * trusting the old line mis-parsed every timestamp by 369 years. */
    tree::ResultTree t;
    auto* root = t.add_root(u"Found");
    auto* p = new tree::FileProperties();
    for (const char* q = "dated.txt"; *q; ++q)
        p->name.push_back(u16(u8(*q)));
    p->logical_size = 7;
    /* a REAL FILETIME: 2025-02-06 12:00 UTC ~= 133856275200000000
     * (100-ns since 1601) - the same wire unit the FAT engine emits */
    p->times.modified = 133856275200000000ULL;
    (void)t.add_file(root, p);
    p->release();
    query::Query q;
    query::Result r;
    r = query::run(t, q);
    ASSERT_EQ(u64(r.rows.size()), 1u);
    EXPECT_EQ(r.rows[0].modified, 133856275200000000ULL)
        << "the Row carries the node's FILETIME verbatim - never Unix seconds";
}

/* ------------------------------------------------------------------ */
/* M-049: the err_text coverage pin (the surface was untested)          */
/* ------------------------------------------------------------------ */

#include "cr/cr_error.h"

TEST(Error, ErrTextCoversEveryCode) {
    /* M-049: every code must carry a distinct, non-empty text (the flat
     * code->text table was untested; unknown codes get the fallback). */
    const u32 codes[] = {
        ERR_OPEN_FAILED, ERR_READ_FAILED, ERR_WRITE_FAILED, ERR_RANGE_OUT_OF_BOUNDS,
        ERR_RETRY_EXHAUSTED, ERR_BAD_PARAMETER, ERR_NO_MEMORY, ERR_BAD_MAGIC,
        ERR_UNSUPPORTED, ERR_TRUNCATED, ERR_CANCELLED, ERR_SESSION_FORMAT,
        ERR_DEST_FULL, ERR_BAD_CHECKSUM, ERR_FVE_LOCKED, ERR_APPLY_CONSENT_DENIED,
        ERR_INTERNAL, ERR_IMAGE_CORRUPT,
    };
    std::set<std::string> texts;
    for (u32 c : codes) {
        const char* t = cr::err_text(c);
        ASSERT_NE(t, nullptr) << "code " << std::hex << c << " has no text";
        EXPECT_STRNE(t, "") << "code " << std::hex << c << " has empty text";
        texts.insert(t);
    }
    /* distinctness: two codes must not share text */
    EXPECT_EQ(texts.size(), sizeof(codes) / sizeof(codes[0]));
    /* unknown codes: the fallback, not a nullptr */
    EXPECT_STRNE(cr::err_text(0xDEADBEEFu), "");
    EXPECT_EQ(cr::err_text(0), std::string("OK")) << "0 is OK by contract";
}
