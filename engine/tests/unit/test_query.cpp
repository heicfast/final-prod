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
