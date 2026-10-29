/* test_m101_branches.cpp - the M-101 branch-axis battery: the Versions
 * node (NTFS multi-version record siblings, one pathname -> one group)
 * and the Tags Extracted branch (the runtime grouping flag), plus the
 * session round-trip and the three-landed-branches regression pin.
 */

#include "cr/tree/cr_node.h"
#include "cr/query/cr_query.h"
#include "cr/session/cr_records.h"
#include "cr/cr_error.h"
#include <gtest/gtest.h>
#include <cstring>
#include <functional>
#include <vector>

using namespace cr;

namespace {

tree::FileNode* add_file(tree::ResultTree& t, tree::FileNode* parent, const char16_t* name,
                         u64 size, u64 version_group, bool tags) {
    tree::FileProperties* p = new tree::FileProperties();
    p->name = name;
    p->logical_size = size;
    p->attributes = 0x20;
    p->version_group = version_group; /* M-101: the stamped axis */
    p->tags_extracted = tags;
    tree::FileNode* n = t.add_file(parent, p);
    p->release(); /* add_file takes a reference, not ownership */
    return n;
}

} // namespace

/* ---- the Versions axis: the grouping semantics ---------------------- */

TEST(M101Branches, VersionsNodeGroupsTornSiblings) {
    /* The finding's falsification: two records for the same $FILE_NAME
     * in the same directory -> ONE group. The stamping side is pinned
     * end-to-end in test_ntfs_volume (NtfsWalkStampsVersionGroup); here
     * the branch semantics: nodes sharing a nonzero id are the group's
     * membership, single-member ids never surface. */
    tree::ResultTree t;
    auto* root = t.add_root(u"Found");
    auto* v1 = add_file(t, root, u"report.docx", 100, 0xAABBCCDD11223344ull, false);
    auto* v2 = add_file(t, root, u"report.docx", 240, 0xAABBCCDD11223344ull, false);
    add_file(t, root, u"other.txt", 10, 0x1234ull, false); /* single-member */
    add_file(t, root, u"plain.bin", 10, 0, false);         /* no group */

    const auto groups = query::version_groups(t);
    ASSERT_EQ(groups.size(), 1u) << "only the multi-member id surfaces";
    EXPECT_EQ(groups[0].id, 0xAABBCCDD11223344ull);
    EXPECT_EQ(groups[0].count, 2u);
    EXPECT_EQ(groups[0].name, u"report.docx") << "the members share the name by construction";
    EXPECT_NE(v1, nullptr);
    EXPECT_NE(v2, nullptr);

    /* the per-group filter: exactly the group's members */
    query::Query q;
    q.filter.version_group = 0xAABBCCDD11223344ull;
    const query::Result r = query::run(t, q);
    ASSERT_EQ(r.rows.size(), 2u);
    for (const auto& row : r.rows) {
        EXPECT_EQ(row.version_group, 0xAABBCCDD11223344ull);
        EXPECT_EQ(row.name_utf8, "report.docx");
    }
}

TEST(M101Branches, VersionsGroupComposesWithOtherFilters) {
    /* The finding's own composition note (composes with M-072): the
     * group axis ANDs with the state/size axes - the deleted state of a
     * torn sibling is the point of the Versions node (the recoverable
     * PRIOR version). */
    tree::ResultTree t;
    auto* root = t.add_root(u"Found");
    add_file(t, root, u"report.docx", 100, 0xAABBCCDD11223344ull, false);
    auto* old = add_file(t, root, u"report.docx", 240, 0xAABBCCDD11223344ull, false);
    old->set_deleted_state(tree::DeletedState::Deleted);
    add_file(t, root, u"noise.dat", 500, 0xAABBCCDD11223344ull, false);

    query::Query q;
    q.filter.version_group = 0xAABBCCDD11223344ull;
    q.filter.state_mask = 1u << u32(tree::DeletedState::Deleted); /* the deleted sibling */
    const query::Result r = query::run(t, q);
    ASSERT_EQ(r.rows.size(), 1u);
    EXPECT_EQ(r.rows[0].size, 240u);
}

/* ---- the Tags Extracted axis: the runtime grouping ------------------ */

TEST(M101Branches, TagsExtractedGrouping) {
    /* The finding's test name: EXIF-bearing files group under the
     * branch. The flag is the runtime datum (the host's preview pass
     * stamps it; NOT persisted - the corpus shows the og persisting no
     * tag state; a reloaded session re-derives). */
    tree::ResultTree t;
    auto* root = t.add_root(u"Found");
    add_file(t, root, u"img1.jpg", 100, 0, true);   /* extracted tags */
    add_file(t, root, u"img2.jpg", 120, 0, true);   /* extracted tags */
    add_file(t, root, u"readme.txt", 10, 0, false); /* no tags */
    add_file(t, root, u"clip.mp4", 900, 0, true);   /* media tags count */

    query::Query q;
    q.filter.tags_only = true;
    const query::Result r = query::run(t, q);
    ASSERT_EQ(r.rows.size(), 3u);
    u32 tagged = 0;
    for (const auto& row : r.rows)
        tagged += row.tags_extracted;
    EXPECT_EQ(tagged, 3u) << "the branch rows carry the flag";
    /* the complement: no filter = all four */
    query::Query all;
    EXPECT_EQ(query::run(t, all).rows.size(), 4u);
}

/* ---- the session round-trip: the group id persists ------------------- */

TEST(M101Branches, SessionVersionsRoundTrip) {
    tree::ResultTree t;
    auto* root = t.add_root(u"V");
    add_file(t, root, u"report.docx", 100, 0xAABBCCDD11223344ull, false);
    add_file(t, root, u"report.docx", 240, 0xAABBCCDD11223344ull, true);
    add_file(t, root, u"plain.bin", 10, 0, true);
    t.refresh_stats();

    std::vector<u8> blob;
    ASSERT_EQ(cr::session::serialize_tree(t, blob), u32(OK));
    tree::ResultTree t2;
    ASSERT_EQ(cr::session::deserialize_tree(blob.data(), blob.size(), t2), u32(OK));
    ASSERT_EQ(t2.roots().size(), 1u);
    u32 with_group = 0;
    std::function<void(tree::FileNode*)> walk = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            if (c->props() && c->props()->version_group == 0xAABBCCDD11223344ull)
                ++with_group;
            /* the tags flag is RUNTIME-ONLY by contract: it may not ride
             * the wire (the finding's defer note - the og persists no
             * tag state) */
            walk(c);
        }
    };
    walk(t2.roots()[0]);
    EXPECT_EQ(with_group, 2u) << "the namespace id round-trips (nflags bit 14)";
    /* the branch enumeration survives the reload */
    const auto groups = query::version_groups(t2);
    ASSERT_EQ(groups.size(), 1u);
    EXPECT_EQ(groups[0].count, 2u);
}

/* ---- the regression pin: the three landed branches keep their shapes - */

TEST(M101Branches, ExistingBranchesUnchanged) {
    /* The junk filter (M-018's Unimportant bucket) still hides flagged
     * nodes from every query INCLUDING the new axes; the raw-category
     * Name-Lost/Path-Lost branches (M-029) are unaffected by the new
     * fields; the group helper never counts junk-flagged members. */
    tree::ResultTree t;
    auto* root = t.add_root(u"Found");
    tree::FileNode* junk = add_file(t, root, u"thumb.db", 100, 0x5555ull, false);
    junk->props()->flags |= tree::NODE_FLAG_JUNK_FILTERED; /* M-018 */
    add_file(t, root, u"good.txt", 10, 0, false);

    query::Query q;
    const query::Result r = query::run(t, q);
    ASSERT_EQ(r.rows.size(), 1u) << "the junk bucket still hides";
    EXPECT_EQ(r.rows[0].name_utf8, "good.txt");

    /* the junk node never joins a version group either */
    auto* root2 = t.add_root(u"R2");
    tree::FileNode* j2 = add_file(t, root2, u"x.dat", 1, 0x7777ull, false);
    j2->props()->flags |= tree::NODE_FLAG_JUNK_FILTERED;
    (void)j2;
    EXPECT_TRUE(query::version_groups(t).empty())
        << "a junk-flagged member alone never surfaces a group";
}
