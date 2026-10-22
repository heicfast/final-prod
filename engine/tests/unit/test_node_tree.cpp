/* ResultTree: dual-class nodes, hard-link sharing, dedup index (F9). */
#include <gtest/gtest.h>
#include "cr/tree/cr_node.h"

using namespace cr;
using namespace cr::tree;

namespace {
FileProperties *mkprops(u64 ref, const char16_t *name, u64 size, u32 attrs) {
    auto *p = new FileProperties();
    p->record_ref = ref;
    p->name = name;
    p->logical_size = size;
    p->attributes = attrs;
    return p;
}
} // namespace

TEST(NodeTree, AddRootsAndFiles) {
    ResultTree t;
    FullNode *root = t.add_root(u"Volume C");
    ASSERT_NE(root, nullptr);
    FileNode *f1 = t.add_file(root, mkprops(1000, u"readme.txt", 1234, 0));
    FileNode *f2 = t.add_file(root, mkprops(1001, u"photo.jpg", 99999, 0));
    ASSERT_NE(f1, nullptr);
    ASSERT_NE(f2, nullptr);
    EXPECT_EQ(root->first_child(), f1);
    EXPECT_EQ(f1->next_sibling(), f2);
    EXPECT_EQ(f2->prev_sibling(), f1);
    EXPECT_EQ(f2->parent(), root);
    EXPECT_EQ(t.node_count(), 3u); /* root + 2 */
    t.refresh_stats();
    EXPECT_EQ(root->stats().file_count, 2u);
    EXPECT_EQ(root->stats().total_logical, 1234u + 99999u);
}

TEST(NodeTree, HardLinkSharesProps) {
    ResultTree t;
    FullNode *root = t.add_root(u"R");
    FileProperties *shared = mkprops(500, u"a.txt", 10, 0);
    shared->add_ref(); /* caller + tree */
    FileNode *l1 = t.add_file(root, shared);
    FileNode *l2 = t.add_file(root, shared);
    /* same content object behind both links */
    EXPECT_EQ(l1->props(), l2->props());
    /* find_by_record hits either */
    EXPECT_NE(t.find_by_record(500), nullptr);
}

TEST(NodeTree, ExtentDedupIndex) {
    ResultTree t;
    FullNode *root = t.add_root(u"R");
    FileProperties *a = mkprops(0, u"carved.jpg", 50000, 0);
    a->extents.push_back({1000000, 50000});
    t.add_file(root, a);
    FileNode *hit = t.find_by_extent(1000000, 0);
    EXPECT_NE(hit, nullptr);
    /* different first extent - no hit */
    EXPECT_EQ(t.find_by_extent(999999, 0), nullptr);
}

TEST(NodeTree, ResetFreesClean) {
    ResultTree t;
    FullNode *root = t.add_root(u"R");
    t.add_file(root, mkprops(1, u"x", 1, 0));
    t.reset();
    EXPECT_EQ(t.node_count(), 0u);
    /* ASan validates no leak/UAF on destruction at scope exit */
}
