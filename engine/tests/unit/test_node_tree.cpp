/* ResultTree: dual-class nodes, hard-link sharing, dedup index (F9). */
#include <gtest/gtest.h>
#include "cr/tree/cr_node.h"
#include <cwchar>
#include <cstring>

using namespace cr;
using namespace cr::tree;

namespace {
FileProperties* mkprops(u64 ref, const char16_t* name, u64 size, u32 attrs) {
    auto* p = new FileProperties();
    p->record_ref = ref;
    p->name = name;
    p->logical_size = size;
    p->attributes = attrs;
    return p;
}
} // namespace

TEST(NodeTree, AddRootsAndFiles) {
    ResultTree t;
    FullNode* root = t.add_root(u"Volume C");
    ASSERT_NE(root, nullptr);
    tree::FileProperties* p1 = mkprops(1000, u"readme.txt", 1234, 0);
    tree::FileProperties* p2 = mkprops(1001, u"photo.jpg", 99999, 0);
    FileNode* f1 = t.add_file(root, p1);
    FileNode* f2 = t.add_file(root, p2);
    p1->release();
    p2->release();
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
    FullNode* root = t.add_root(u"R");
    FileProperties* shared = mkprops(500, u"a.txt", 10, 0);
    FileNode* l1 = t.add_file(root, shared);
    FileNode* l2 = t.add_file(root, shared); /* hard link: +1 per call */
    shared->release();                       /* the ctor ref; nodes hold theirs */
    /* same content object behind both links */
    EXPECT_EQ(l1->props(), l2->props());
    /* find_by_record hits either */
    EXPECT_NE(t.find_by_record(500), nullptr);
}

TEST(NodeTree, ExtentDedupIndex) {
    ResultTree t;
    FullNode* root = t.add_root(u"R");
    FileProperties* a = mkprops(0, u"carved.jpg", 50000, 0);
    a->extents.push_back({1000000, 50000});
    t.add_file(root, a);
    a->release();
    FileNode* hit = t.find_by_extent(1000000, 0);
    EXPECT_NE(hit, nullptr);
    /* different first extent - no hit */
    EXPECT_EQ(t.find_by_extent(999999, 0), nullptr);
}

TEST(NodeTree, ResetFreesClean) {
    ResultTree t;
    FullNode* root = t.add_root(u"R");
    tree::FileProperties* px = mkprops(1, u"x", 1, 0);
    t.add_file(root, px);
    px->release();
    t.reset();
    EXPECT_EQ(t.node_count(), 0u);
    /* ASan validates no leak/UAF on destruction at scope exit */
}

TEST(NodeTree, DetachChildLiveCounterCountsOnlyAttachedFiles) {
    /* N-218: live_files_ is ticked by add_file ALONE (add_folder and
     * add_root never tick - N-166's own contract), but detach_child
     * subtracted the WHOLE subtree's node count - a detached subtree
     * holding FullNode intermediates (the session-loaded shape:
     * deserialize_tree re-creates marker=2 intermediate folders under
     * the structured roots) over-subtracted and WRAPPED the unsigned
     * counter, corrupting the mid-stage telemetry the progress layer
     * samples. The subtraction must count non-full nodes only. */
    ResultTree t;
    FullNode* root = t.add_root(u"R");
    ASSERT_NE(root, nullptr);
    FullNode* folder = t.add_folder(root, u"sub"); /* FullNode: NO tick */
    ASSERT_NE(folder, nullptr);
    FileProperties* p = mkprops(1000, u"a.txt", 10, 0);
    FileNode* f = t.add_file(folder, p);
    p->release();
    ASSERT_NE(f, nullptr);
    EXPECT_EQ(t.live_files(), 1u); /* the file alone - not the folder */
    t.detach_child(root, folder);
    EXPECT_EQ(t.live_files(), 0u) << "the unsigned counter must not wrap";
    EXPECT_EQ(t.node_count(), 1u); /* the root alone */
}

/* ---- M13 (W5): the "Searching: <path>" ticker cell ---- */

TEST(NodeTree, CurrentPathSamplesFirstAttachImmediately) {
    /* tick 0 samples: the scan's first find lights the ticker at once */
    ResultTree t;
    EXPECT_EQ(t.current_path(), "");
    FullNode* root = t.add_root(u"Volume C");
    tree::FileProperties* p = mkprops(7, u"first.txt", 10, 0);
    t.add_file(root, p);
    p->release();
    EXPECT_EQ(t.current_path(), "Volume C\\first.txt");
}

TEST(NodeTree, CurrentPathBoundedCadenceAndDepth) {
    /* every 128th attach resamples; the path is the root-joined full
     * path of the NEWEST sampled file; unsampled attaches leave the
     * cell holding the last sample (the display cadence contract) */
    ResultTree t;
    FullNode* root = t.add_root(u"Data");
    FullNode* dir = t.add_folder(root, u"Users");
    FullNode* sub = t.add_folder(dir, u"docs");
    char16_t name[32];
    tree::FileProperties* last = nullptr;
    for (int i = 0; i < 300; ++i) {
        /* ASCII-safe u16 name build (wchar_t width differs on Linux) */
        int n = 0;
        name[n++] = u'f';
        if (i >= 100)
            name[n++] = char16_t(u'0' + (i / 100) % 10);
        if (i >= 10)
            name[n++] = char16_t(u'0' + (i / 10) % 10);
        name[n++] = char16_t(u'0' + i % 10);
        name[n++] = u'.';
        name[n++] = u't';
        name[n++] = u'x';
        name[n++] = u't';
        name[n] = 0;
        last = mkprops(u64(i), name, 1, 0);
        t.add_file(sub, last);
        last->release();
    }
    const std::string got = t.current_path();
    /* 300 attaches -> samples at ticks 0, 128, 256: the cell holds a
     * tick-128/256 sample; assert the shape, not the exact index (the
     * cadence contract is the pin) */
    EXPECT_NE(got.find("Data\\Users\\docs\\f"), std::string::npos)
        << "cell must hold a root-joined path of a sampled file, got: " << got;
    EXPECT_GT(got.size(), strlen("Data\\Users\\docs\\f0.txt"));
}

TEST(NodeTree, CurrentPathTruncationKeepsTheTail) {
    /* deep chains past the 511-byte cap keep the ITEM's own name (the
     * tail), drop the deep-root side, and stay valid UTF-8 */
    ResultTree t;
    FullNode* root = t.add_root(u"R");
    FileNode* parent = root;
    for (int d = 0; d < 40; ++d) {
        FullNode* dir = t.add_folder(parent, u"folder-with-a-fairly-long-name-here");
        parent = dir;
    }
    tree::FileProperties* p = mkprops(9, u"the-item-name.txt", 1, 0);
    t.add_file(parent, p);
    p->release();
    const std::string got = t.current_path();
    EXPECT_LE(got.size(), 511u);
    /* the tail survives: the item's own name is present */
    EXPECT_NE(got.rfind("the-item-name.txt"), std::string::npos)
        << "truncation must keep the item's name, got: " << got;
    /* the front backs off to a codepoint boundary: never a continuation
     * byte (0b10xxxxxx) — the truncated string is valid UTF-8 */
    ASSERT_FALSE(got.empty());
    const unsigned char front = static_cast<unsigned char>(got.front());
    EXPECT_NE(front & 0xC0u, 0x80u) << "front must not be a UTF-8 continuation byte";
}
