/* test_grading.cpp - S-023 acceptance: the post-scan CRecoverability
 * grading pass (the og's DRWUI-side 3-grade model) - grade storage
 * semantics, the decision tree, assistant statistics, session round-trip,
 * and the report's scan-analysis block.
 *
 * Licensed under the MIT license. See LICENSE.
 */
#include <gtest/gtest.h>
#include "cr/tree/cr_grading.h"
#include "cr/orchestrator/cr_scan.h"
#include "cr/recovery/cr_report.h"
#include "cr/session/cr_crsf.h"
#include "cr/session/cr_records.h"
#include <functional>

using namespace cr;
using namespace cr::tree;

namespace {

FileNode* add_file(ResultTree& t, FileNode* parent, const char* name, u64 size, u32 fs_kind,
                   DeletedState deleted = DeletedState::Active, Origin origin = Origin::Structured,
                   u64 first_extent = 0, u64 extent_len = 0) {
    auto* p = new FileProperties();
    for (const char* q = name; *q; ++q)
        p->name.push_back(u16(u8(*q)));
    p->logical_size = size;
    p->fs_kind = fs_kind;
    if (extent_len) {
        FileProperties::Extent e;
        e.offset = first_extent;
        e.length = extent_len;
        p->extents.push_back(e);
    }
    t.add_file(parent, p);
    p->release();
    FileNode* n = parent->first_child();
    while (n->next_sibling())
        n = n->next_sibling();
    n->set_deleted_state(deleted);
    n->set_origin(origin);
    return n;
}

grading::GradeContext single_disk_ctx() {
    grading::GradeContext ctx;
    grading::DeviceSlot slot;
    slot.base = 0;
    slot.disk_ordinal = 0;
    ctx.devices.push_back(slot);
    return ctx;
}

} // namespace

/* ---- grade storage semantics (the og setter FUN_140089630) ---- */

TEST(Grading, MonotonicMaxNeverDowngrades) {
    ResultTree t;
    auto* root = t.add_root(u"Found");
    auto* n = add_file(t, root, "a.txt", 100, u32(FsKindCode::Fat16));
    n->set_grade(Grade::High);
    n->set_grade(Grade::Average); /* 1 < 2: refused */
    EXPECT_EQ(Grade::High, n->grade());
    n = add_file(t, root, "b.txt", 100, u32(FsKindCode::Fat16));
    n->set_grade(Grade::Low);
    n->set_grade(Grade::Average); /* 0 -> 1: raise ok */
    EXPECT_EQ(Grade::Average, n->grade());
    n->set_grade(Grade::High); /* 1 -> 2: raise ok */
    EXPECT_EQ(Grade::High, n->grade());
}

TEST(Grading, Grade2Sticky) {
    ResultTree t;
    auto* root = t.add_root(u"Found");
    auto* n = add_file(t, root, "c.txt", 100, u32(FsKindCode::Fat16));
    n->set_grade(Grade::High);
    n->set_grade(Grade::Low); /* a grade-2 node is never cleared */
    EXPECT_EQ(Grade::High, n->grade());
    /* 1 -> 0 IS allowed (the og clears non-2 grades) */
    n = add_file(t, root, "d.txt", 100, u32(FsKindCode::Fat16));
    n->set_grade(Grade::Average);
    n->set_grade(Grade::Low);
    EXPECT_EQ(Grade::Low, n->grade());
}

TEST(Grading, DefaultGradeIsAverageAtEmit) {
    /* the og engines write grade=1 at node construction
     * (NTFSSearchDLL :39242/:39779) - OQ-S2M-1 */
    ResultTree t;
    auto* root = t.add_root(u"Found");
    auto* n = add_file(t, root, "e.txt", 100, u32(FsKindCode::Fat16));
    EXPECT_EQ(Grade::Average, n->grade());
    EXPECT_FALSE(n->overlap());
}

/* ---- the decision tree ---- */

TEST(Grading, SizeFloorIs1) {
    /* size < 1 -> grade 0 (FUN_14038ac90 :605726-605727) */
    ResultTree t;
    auto* root = t.add_root(u"Found");
    auto* zero = add_file(t, root, "zero.dat", 0, u32(FsKindCode::Fat16), DeletedState::Active);
    auto* one = add_file(t, root, "one.dat", 1, u32(FsKindCode::Fat16), DeletedState::Active);
    grading::AssistantStats st;
    grading::grade_tree(t, single_disk_ctx(), st);
    EXPECT_EQ(Grade::Low, zero->grade());
    EXPECT_EQ(Grade::High, one->grade()); /* existing + size>=1 -> 2 */
}

TEST(Grading, EmailCategoryFastPath) {
    /* og category 0xC (email) -> grade 2 (FUN_14039aa40's rewrite feeds
     * FUN_14038ac90 :605731-605732); our analog: query::Category::Email */
    ResultTree t;
    auto* root = t.add_root(u"Found");
    auto* deleted_pst =
        add_file(t, root, "archive.pst", 5000, u32(FsKindCode::Fat16), DeletedState::Deleted);
    auto* deleted_txt =
        add_file(t, root, "notes.txt", 5000, u32(FsKindCode::Fat16), DeletedState::Deleted);
    grading::AssistantStats st;
    grading::grade_tree(t, single_disk_ctx(), st);
    EXPECT_EQ(Grade::High, deleted_pst->grade());
    EXPECT_NE(Grade::High, deleted_txt->grade()); /* not email, not twin */
}

TEST(Grading, ExistingFilesHighDeletedNot) {
    /* the insert rule (FUN_14037a8a0): existing + size >= 1 -> 2 */
    ResultTree t;
    auto* root = t.add_root(u"Found");
    auto* live = add_file(t, root, "live.txt", 100, u32(FsKindCode::Ext4), DeletedState::Active);
    auto* dead = add_file(t, root, "dead.txt", 100, u32(FsKindCode::Ext4), DeletedState::Deleted);
    grading::AssistantStats st;
    grading::grade_tree(t, single_disk_ctx(), st);
    EXPECT_EQ(Grade::High, live->grade());
    EXPECT_EQ(Grade::Average, dead->grade()); /* no rule fired -> default 1 */
}

TEST(Grading, SmallFileUnder1024High) {
    /* the carve-device rule (FUN_14038b0e0 vt+0xe8 branch): carved small
     * < 0x400 -> High; large -> Low */
    ResultTree t;
    auto* root = t.add_root(u"Found");
    auto* small = add_file(t, root, "small.jpg", 100, u32(FsKindCode::Raw), DeletedState::Deleted,
                           Origin::Carved, 0x10000, 100);
    auto* large = add_file(t, root, "large.jpg", 100000, u32(FsKindCode::Raw),
                           DeletedState::Deleted, Origin::Carved, 0x20000, 100000);
    grading::AssistantStats st;
    grading::grade_tree(t, single_disk_ctx(), st);
    EXPECT_EQ(Grade::High, small->grade());
    EXPECT_EQ(Grade::Low, large->grade());
    EXPECT_EQ(u64(2), st.raw_count);
}

TEST(Grading, NtfsDeviceIndexRuleUpgrades) {
    /* NTFS + device ordinal in [0,9) -> 2 + QuickScanDeleteExtend tick
     * (FUN_14038b0e0 :605897-605900, DAT_141be5818) */
    ResultTree t;
    auto* root = t.add_root(u"Found");
    auto* dead = add_file(t, root, "gone.txt", 5000, u32(FsKindCode::Ntfs), DeletedState::Deleted);
    grading::AssistantStats st;
    grading::grade_tree(t, single_disk_ctx(), st);
    EXPECT_EQ(Grade::High, dead->grade());
    EXPECT_EQ(u64(1), st.quick_scan_delete_extend);
}

TEST(Grading, ExtensionTableGatesDeepValidation) {
    /* only table extensions enter the twin detection: two same-key files
     * with in-table extensions -> twins; out-of-table -> no twin */
    ResultTree t;
    auto* root = t.add_root(u"Found");
    auto* a1 = add_file(t, root, "a.doc", 500, u32(FsKindCode::Fat16), DeletedState::Deleted,
                        Origin::Structured, 0x5000, 0x400);
    auto* a2 = add_file(t, root, "b.doc", 500, u32(FsKindCode::Fat16), DeletedState::Deleted,
                        Origin::Structured, 0x5000, 0x400);
    auto* c1 = add_file(t, root, "c.zzqx", 500, u32(FsKindCode::Fat16), DeletedState::Deleted,
                        Origin::Structured, 0x9000, 0x400);
    auto* c2 = add_file(t, root, "d.zzqx", 500, u32(FsKindCode::Fat16), DeletedState::Deleted,
                        Origin::Structured, 0x9000, 0x400);
    grading::AssistantStats st;
    grading::grade_tree(t, single_disk_ctx(), st);
    EXPECT_TRUE(a1->overlap()) << "in-table (DOC) twins must be flagged";
    EXPECT_TRUE(a2->overlap());
    EXPECT_EQ(Grade::High, a1->grade());
    EXPECT_EQ(Grade::High, a2->grade());
    EXPECT_FALSE(c1->overlap()) << "out-of-table extensions never enter the deep path";
    EXPECT_FALSE(c2->overlap());
}

TEST(Grading, TwinAddressFlagsBoth) {
    /* twins must be same-extent-end AND same category (DRWUI :605774-
     * 605781 matches keys AND categories before FUN_14038ba60 flags) */
    ResultTree t;
    auto* root = t.add_root(u"Found");
    /* same extent-end + same category (doc/doc) -> twins */
    auto* x1 = add_file(t, root, "x.doc", 500, u32(FsKindCode::Fat16), DeletedState::Deleted,
                        Origin::Structured, 0x5000, 0x400);
    auto* x2 = add_file(t, root, "y.doc", 0x400, u32(FsKindCode::Fat16), DeletedState::Deleted,
                        Origin::Structured, 0x5100, 0x300);
    /* same extent-end + DIFFERENT category (doc/mp3) -> not twins */
    auto* y1 = add_file(t, root, "p1.mp3", 500, u32(FsKindCode::Fat16), DeletedState::Deleted,
                        Origin::Structured, 0xA000, 0x400);
    auto* y2 = add_file(t, root, "p2.doc", 500, u32(FsKindCode::Fat16), DeletedState::Deleted,
                        Origin::Structured, 0xA000, 0x400);
    grading::AssistantStats st;
    grading::grade_tree(t, single_disk_ctx(), st);
    EXPECT_TRUE(x1->overlap());
    EXPECT_TRUE(x2->overlap());
    EXPECT_FALSE(y1->overlap());
    EXPECT_FALSE(y2->overlap());
}

TEST(Grading, TableMembershipAndCount) {
    /* the dumped 169-entry table (first key "Raw") */
    EXPECT_TRUE(grading::extension_in_grading_table("raw"));
    EXPECT_TRUE(grading::extension_in_grading_table("doc"));
    EXPECT_TRUE(grading::extension_in_grading_table("pst"));
    EXPECT_TRUE(grading::extension_in_grading_table("cr3"));
    EXPECT_FALSE(grading::extension_in_grading_table("zzqx"));
    EXPECT_FALSE(grading::extension_in_grading_table(""));
    EXPECT_EQ(size_t(169), grading::grading_table_entry_count());
}

/* ---- per-selection re-grade ---- */

TEST(Grading, RegradeOnlyGrade1) {
    ResultTree t;
    auto* root = t.add_root(u"Found");
    auto* avg = add_file(t, root, "avg.txt", 5000, u32(FsKindCode::Fat16), DeletedState::Deleted);
    auto* high = add_file(t, root, "hi.pst", 5000, u32(FsKindCode::Fat16), DeletedState::Deleted);
    auto* low = add_file(t, root, "lo.dat", 0, u32(FsKindCode::Fat16), DeletedState::Deleted);
    grading::AssistantStats st;
    grading::grade_tree(t, single_disk_ctx(), st);
    ASSERT_EQ(Grade::Average, avg->grade());
    ASSERT_EQ(Grade::High, high->grade());
    ASSERT_EQ(Grade::Low, low->grade());

    /* re-grade only processes grade-1 nodes (FUN_1401d7110's gate) */
    EXPECT_TRUE(grading::regrade_node(avg, single_disk_ctx()));
    EXPECT_FALSE(grading::regrade_node(high, single_disk_ctx()));
    EXPECT_FALSE(grading::regrade_node(low, single_disk_ctx()));
}

/* ---- assistant statistics ---- */

TEST(Grading, AssistantCountersMixedFixture) {
    ResultTree t;
    auto* found = t.add_root(u"Found");
    auto* lost = t.add_root(u"Lost Files");
    add_file(t, found, "live.jpg", 100, u32(FsKindCode::Ext4), DeletedState::Active);
    add_file(t, found, "dead.txt", 200, u32(FsKindCode::Ext4), DeletedState::Deleted);
    add_file(t, found, "carved.jpg", 50, u32(FsKindCode::Raw), DeletedState::Deleted,
             Origin::Carved);
    add_file(t, lost, "remnant.doc", 300, u32(FsKindCode::Ntfs), DeletedState::Deleted,
             Origin::Lost);
    add_file(t, lost, "lostlive.doc", 300, u32(FsKindCode::Ntfs), DeletedState::Active,
             Origin::Lost);
    grading::AssistantStats st;
    grading::grade_tree(t, single_disk_ctx(), st);
    EXPECT_EQ(u64(5), st.total_file_count);
    EXPECT_EQ(u64(1), st.raw_count);
    EXPECT_EQ(u64(1), st.lost_partition_exist_file_count);
    EXPECT_EQ(u64(1), st.lost_partition_delete_file_count);
    EXPECT_EQ(u64(1), st.quick_scan_exist_file_count);
    EXPECT_EQ(u64(1), st.quick_scan_delete_file_count);
    EXPECT_EQ(u64(2), st.deep_scan_file_count); /* both under Lost Files */
    EXPECT_EQ(u64(2), st.category_counts[0]);   /* image: live.jpg + carved.jpg */
    /* small-result heuristic: 5 < 100 -> matched forced 0 */
    EXPECT_TRUE(st.small_result);
    EXPECT_EQ(u64(0), st.matched);
    EXPECT_EQ(u64(5), st.total);
}

/* ---- session round-trip ---- */

TEST(Grading, SessionRoundTripPreservesGrade) {
    ResultTree t;
    auto* root = t.add_root(u"Found");
    auto* low = add_file(t, root, "z.dat", 0, u32(FsKindCode::Fat16));
    auto* high = add_file(t, root, "y.pst", 100, u32(FsKindCode::Fat16));
    grading::AssistantStats st;
    grading::grade_tree(t, single_disk_ctx(), st);
    ASSERT_EQ(Grade::Low, low->grade());
    ASSERT_EQ(Grade::High, high->grade());
    high->set_overlap();

    struct MemFile {
        std::vector<u8> data;
        u64 pos = 0;
        static u32 wr(void* u, const void* b, u32 len) {
            auto* m = static_cast<MemFile*>(u);
            if (m->pos + len > m->data.size())
                m->data.resize(size_t(m->pos + len));
            std::memcpy(m->data.data() + m->pos, b, len);
            m->pos += len;
            return 0;
        }
        static u64 tl(void* u) { return static_cast<MemFile*>(u)->pos; }
        static u32 sk(void* u, u64 abs) {
            static_cast<MemFile*>(u)->pos = abs;
            return 0;
        }
        static u32 rd(void* u, void* b, u32 len) {
            auto* m = static_cast<MemFile*>(u);
            if (m->pos + len > m->data.size())
                return 1;
            std::memcpy(b, m->data.data() + m->pos, len);
            m->pos += len;
            return 0;
        }
        static u64 sz(void* u) { return static_cast<MemFile*>(u)->data.size(); }
    } mf;
    {
        session::Writer w(MemFile::wr, MemFile::tl, MemFile::sk, &mf);
        ASSERT_EQ(w.open(9), OK);
        std::vector<u8> blob;
        ASSERT_EQ(OK, session::serialize_tree(t, blob));
        ASSERT_EQ(OK, w.add_record(session::REC_RESULT_TREE, blob.data(), u32(blob.size())));
        ASSERT_EQ(w.finalize(), OK);
    }
    ResultTree t2;
    {
        session::Reader r(MemFile::rd, MemFile::sk, MemFile::sz, &mf);
        u32 count = 0;
        ASSERT_EQ(OK, r.open(&count));
        ASSERT_GE(count, 1u);
        session::Record rec;
        ASSERT_EQ(OK, r.read_record(0, rec));
        ASSERT_EQ(session::REC_RESULT_TREE, rec.type);
        ASSERT_EQ(OK, session::deserialize_tree(rec.data.data(), rec.data.size(), t2));
    }
    /* find the two nodes again */
    FileNode *l2 = nullptr, *h2 = nullptr;
    std::function<void(FileNode*)> collect = [&](FileNode* n) {
        for (FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            if (c->name_utf8() == "z.dat")
                l2 = c;
            if (c->name_utf8() == "y.pst")
                h2 = c;
            collect(c);
        }
    };
    for (auto* rr : t2.roots())
        collect(rr);
    ASSERT_NE(l2, nullptr);
    ASSERT_NE(h2, nullptr);
    EXPECT_EQ(Grade::Low, l2->grade());
    EXPECT_EQ(Grade::High, h2->grade());
    EXPECT_TRUE(h2->overlap());
}

/* ---- report block ---- */

TEST(Report, ContainsAssistantStatistics) {
    recovery::report::ScanAnalysis a;
    a.total_file_count = 10;
    a.raw_count = 3;
    a.lost_partition_exist_file_count = 1;
    a.lost_partition_delete_file_count = 2;
    a.quick_scan_exist_file_count = 2;
    a.quick_scan_delete_file_count = 1;
    a.deep_scan_file_count = 1;
    a.quick_scan_delete_extend = 1;
    for (size_t i = 0; i < 8; ++i)
        a.category_counts[i] = i;
    a.matched = 4;
    a.total = 10;
    a.small_result = false;
    recovery::report::Totals t;
    const std::string doc = recovery::report::build_report({}, t, &a);
    EXPECT_NE(std::string::npos, doc.find("\"scan_analysis\""));
    EXPECT_NE(std::string::npos, doc.find("\"total_file_count\":10"));
    EXPECT_NE(std::string::npos, doc.find("\"quick_scan_delete_extend\":1"));
    EXPECT_NE(std::string::npos, doc.find("\"bookmarks\": 7"));
    EXPECT_NE(std::string::npos, doc.find("\"matched\":4"));
    /* without the block: absent */
    const std::string plain = recovery::report::build_report({}, t);
    EXPECT_EQ(std::string::npos, plain.find("scan_analysis"));
    /* small-result truth: the flag lands in the document */
    a.small_result = true;
    a.matched = 0;
    const std::string small = recovery::report::build_report({}, t, &a);
    EXPECT_NE(std::string::npos, small.find("\"small_result\":1"));
}
