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
#include "test_tmpdir.hpp"
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

TEST(Grading, RefsAndBtrfsDeletedFilesGradeHigh) {
    /* S-061: the og's insert-time hook grades every non-existing ReFS
     * (kind 10) and Btrfs (kind 0xB) file High unconditionally
     * (FUN_14038a470 :605481-497 - FUN_140089630(node,2) + the gauge
     * ticks DAT_141be5830/5828 for both kinds, verified first-hand).
     * The pre-fix core returned early without touching the grade -
     * the whole ReFS/Btrfs population sat at Average. */
    ResultTree t;
    auto* root = t.add_root(u"Found");
    auto* refs_del =
        add_file(t, root, "notes.txt", 900, u32(FsKindCode::Refs), DeletedState::Deleted);
    auto* btrfs_del =
        add_file(t, root, "photo.jpg", 900, u32(FsKindCode::Btrfs), DeletedState::Deleted);
    /* control: the same shape on ext4 stays Average (no insert rule) */
    auto* ext4_del =
        add_file(t, root, "notes.txt", 900, u32(FsKindCode::Ext4), DeletedState::Deleted);
    grading::AssistantStats st;
    grading::grade_tree(t, single_disk_ctx(), st);
    EXPECT_EQ(Grade::High, refs_del->grade()) << "ReFS deleted -> insert-time High";
    EXPECT_EQ(Grade::High, btrfs_del->grade()) << "Btrfs deleted -> insert-time High";
    EXPECT_NE(Grade::High, ext4_del->grade()) << "ext4 deleted keeps the generic path";
}

TEST(Grading, RefsExistingStillHighAndSmallStillLow) {
    /* rule-order guards: the Active rule precedes the ReFS/Btrfs rule
     * (identical outcome - High), and the size<1 Low rule precedes both
     * (the og's insert hook is gated on size via the completion pass's
     * :605726-27 - a zero-size node grades Low on every FS). */
    ResultTree t;
    auto* root = t.add_root(u"Found");
    auto* refs_live =
        add_file(t, root, "live.txt", 100, u32(FsKindCode::Refs), DeletedState::Active);
    auto* refs_zero =
        add_file(t, root, "zero.txt", 0, u32(FsKindCode::Refs), DeletedState::Deleted);
    grading::AssistantStats st;
    grading::grade_tree(t, single_disk_ctx(), st);
    EXPECT_EQ(Grade::High, refs_live->grade());
    EXPECT_EQ(Grade::Low, refs_zero->grade());
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

TEST(Grading, GaugePopulationIsMemberSet) {
    /* N-183 (diff-to-fix): the og's recoverability gauge counts the
     * 20-format member set (the .rdata ctor map, DRWUI:605502-513) plus
     * the auto-High classes - never every file. The core's all-files
     * view (total/matched) stays as data beside it. */
    ResultTree t;
    auto* root = t.add_root(u"Found");
    /* member-set files (jpg x2, docx, pdf) */
    (void)add_file(t, root, "a.jpg", 500, u32(FsKindCode::Fat16), DeletedState::Deleted);
    (void)add_file(t, root, "b.jpg", 500, u32(FsKindCode::Fat16), DeletedState::Active);
    (void)add_file(t, root, "c.docx", 500, u32(FsKindCode::Fat16), DeletedState::Deleted);
    (void)add_file(t, root, "d.pdf", 500, u32(FsKindCode::Fat16), DeletedState::Deleted);
    /* non-member files (txt x2, zzqx) */
    (void)add_file(t, root, "e.txt", 500, u32(FsKindCode::Fat16), DeletedState::Deleted);
    (void)add_file(t, root, "f.txt", 500, u32(FsKindCode::Fat16), DeletedState::Active);
    (void)add_file(t, root, "g.zzqx", 500, u32(FsKindCode::Fat16), DeletedState::Deleted);
    grading::AssistantStats st;
    grading::grade_tree(t, single_disk_ctx(), st);
    EXPECT_EQ(u64(7), st.total_file_count) << "the all-files view keeps every file";
    EXPECT_EQ(u64(7), st.total) << "total stays the all-files aggregate";
    EXPECT_EQ(u64(4), st.gauge_total) << "only the member-set population (jpg/docx/pdf)";
}

TEST(Grading, ScoreFormulaRoundsLikeOg) {
    /* N-183: the og's verdict score int((matched/total + 0.05) * 10)
     * (FUN_14021b030 :374451-481). Boundaries: 0.50 -> 5; 0.85 -> 9;
     * 0.95 -> 10 (the int() truncation); 0 matched -> 0 (the 0.05 floor
     * rounds to 0); an empty population -> 0. Small results (<100
     * files): the gauge zeroes matched, the 0.05 floor still yields 0. */
    ResultTree t;
    auto* root = t.add_root(u"Found");
    /* 200 member files, 100 High (carve<0x400 promotes) + 100
     * too-large: 100/200 = 0.50 -> int(5.5) = 5; 200 >= 100 clears the
     * small-result floor. */
    for (int i = 0; i < 100; ++i) {
        char nm[32];
        snprintf(nm, sizeof(nm), "small%03d.jpg", i);
        (void)add_file(t, root, nm, 100, u32(FsKindCode::Raw), DeletedState::Deleted,
                       Origin::Carved);
    }
    for (int i = 0; i < 100; ++i) {
        char nm[32];
        snprintf(nm, sizeof(nm), "large%03d.jpg", i);
        (void)add_file(t, root, nm, 100000, u32(FsKindCode::Raw), DeletedState::Deleted,
                       Origin::Carved);
    }
    grading::AssistantStats st;
    grading::grade_tree(t, single_disk_ctx(), st);
    EXPECT_EQ(u64(200), st.gauge_total);
    EXPECT_EQ(u64(100), st.gauge_matched);
    EXPECT_FALSE(st.small_result) << "200 >= 100: the gauge shows";
    EXPECT_EQ(5u, st.score) << "int((100/200 + 0.05) * 10) = int(5.5) = 5";
}

TEST(Grading, QSDENotTheNtfsIndexCount) {
    /* N-183 (sub-gap c): QSDE (+0x1b8, DRWUI:331164-170) counts the
     * NON-EXISTING population per node; the NTFS fixer-index rule's tick
     * is DAT_141be5818 (the gauge-matched global) - two different
     * counters the pre-fix code had fused. */
    ResultTree t;
    auto* root = t.add_root(u"Found");
    /* a deleted NTFS node at a low device ordinal (the rule's firing
     * population - the existing-rule shadow matches the og's insert-time
     * High for live nodes): the fixer-index tick fires (grade High +
     * ntfs_device_index_high) AND the node is non-existing (one QSDE
     * tick). */
    (void)add_file(t, root, "gone.txt", 5000, u32(FsKindCode::Ntfs), DeletedState::Deleted);
    /* two deleted FAT nodes: QSDE ticks; the NTFS index never. */
    (void)add_file(t, root, "dead1.txt", 500, u32(FsKindCode::Fat16), DeletedState::Deleted);
    (void)add_file(t, root, "dead2.txt", 500, u32(FsKindCode::Fat16), DeletedState::Deleted);
    /* an existing FAT node: neither counter (not non-existing, not NTFS). */
    (void)add_file(t, root, "live.txt", 500, u32(FsKindCode::Fat16), DeletedState::Active);
    grading::AssistantStats st;
    grading::grade_tree(t, single_disk_ctx(), st);
    EXPECT_EQ(u64(1), st.ntfs_device_index_high) << "the fixer-index tick (the NTFS node)";
    EXPECT_EQ(u64(3), st.quick_scan_delete_extend)
        << "the non-existing population (3 deleted nodes)";
    /* the fusion test: the two counts differ -> neither is an alias */
    EXPECT_NE(st.ntfs_device_index_high, st.quick_scan_delete_extend);
}

TEST(Grading, NtfsDeviceIndexRuleUpgrades) {
    /* NTFS + device ordinal in [0,9) -> grade 2 + the DAT_141be5818 tick
     * (FUN_14038b0e0 :605897-605900). N-183 re-pin: DAT_141be5818 is the
     * GAUGE-MATCHED global, not QSDE - the rule's counter is
     * ntfs_device_index_high; QSDE is the per-node NON-EXISTING count
     * (the +0x1b8 field, DRWUI :331164-170), which this deleted node
     * also ticks. Both asserted. */
    ResultTree t;
    auto* root = t.add_root(u"Found");
    auto* dead = add_file(t, root, "gone.txt", 5000, u32(FsKindCode::Ntfs), DeletedState::Deleted);
    grading::AssistantStats st;
    grading::grade_tree(t, single_disk_ctx(), st);
    EXPECT_EQ(Grade::High, dead->grade());
    EXPECT_EQ(u64(1), st.ntfs_device_index_high) << "the fixer-index tick (DAT_141be5818)";
    EXPECT_EQ(u64(1), st.quick_scan_delete_extend) << "the non-existing population count";
}

TEST(Grading, ExtensionTableGatesDeepValidation) {
    /* S-058 re-pin: the og's gate is NOT-IN-TABLE (FUN_1401bebe0 is the
     * iterator INEQUALITY comparator - the :605743 deep-validation block
     * runs exactly when the 169-key set lookup MISSES; the control case
     * at :585885-905 calibrates the direction). Two same-key files with
     * OUT-OF-TABLE extensions -> twins; in-table -> the engine-default
     * grade stands, no twin machinery. (The pre-fix pin ran the twins
     * for the in-table pair - the s5 evidence's inverted polarity line,
     * which this test had pinned. The db82b5e6/81b4cdaf hot-window merge
     * left two generations of conflict markers here - collapsed to this
     * single form, the fixtures matching the asserts.) */
    ResultTree t;
    auto* root = t.add_root(u"Found");
    /* the in-table DOC pair at one key (skips the deep path) */
    auto* a1 = add_file(t, root, "a.doc", 500, u32(FsKindCode::Fat16), DeletedState::Deleted,
                        Origin::Structured, 0x5000, 0x400);
    auto* a2 = add_file(t, root, "b.doc", 500, u32(FsKindCode::Fat16), DeletedState::Deleted,
                        Origin::Structured, 0x5000, 0x400);
    /* the out-of-table ZZQX pair at another key (enters the deep path) */
    auto* c1 = add_file(t, root, "c.zzqx", 500, u32(FsKindCode::Fat16), DeletedState::Deleted,
                        Origin::Structured, 0x9000, 0x400);
    auto* c2 = add_file(t, root, "d.zzqx", 500, u32(FsKindCode::Fat16), DeletedState::Deleted,
                        Origin::Structured, 0x9000, 0x400);
    /* the dot-less pair at a third key: the runtime set's "" quirk makes
     * dot-less files IN-table -> skip the deep path too */
    auto* e1 = add_file(t, root, "noext1", 500, u32(FsKindCode::Fat16), DeletedState::Deleted,
                        Origin::Structured, 0xD000, 0x400);
    auto* e2 = add_file(t, root, "noext2", 500, u32(FsKindCode::Fat16), DeletedState::Deleted,
                        Origin::Structured, 0xD000, 0x400);
    grading::AssistantStats st;
    grading::grade_tree(t, single_disk_ctx(), st);
    EXPECT_FALSE(a1->overlap()) << "in-table (DOC) extensions skip the deep path";
    EXPECT_FALSE(a2->overlap());
    EXPECT_NE(Grade::High, a1->grade()) << "no twin-induced promotion for in-table rows";
    EXPECT_TRUE(c1->overlap()) << "out-of-table (.zzqx) extensions enter the deep path";
    EXPECT_TRUE(c2->overlap());
    EXPECT_EQ(Grade::High, c1->grade()) << "the twin pair promotes";
    EXPECT_EQ(Grade::High, c2->grade());
    EXPECT_FALSE(e1->overlap()) << "dot-less names skip the twin path (the \"\" quirk)";
    EXPECT_FALSE(e2->overlap());
}

TEST(Grading, DotlessNamesSkipTwinDetection) {
    /* the og's runtime set contains "" (the ctor's 2000-iteration init
     * loop inserts the empty string after the 169 real keys), so
     * dot-less files are IN-TABLE -> skip the deep path under the
     * corrected polarity. Pins the "" coincidence so the polarity flip
     * cannot regress it. */
    ResultTree t;
    auto* root = t.add_root(u"Found");
    auto* a1 = add_file(t, root, "noext1", 500, u32(FsKindCode::Fat16), DeletedState::Deleted,
                        Origin::Structured, 0x5000, 0x400);
    auto* a2 = add_file(t, root, "noext2", 500, u32(FsKindCode::Fat16), DeletedState::Deleted,
                        Origin::Structured, 0x5000, 0x400);
    grading::AssistantStats st;
    grading::grade_tree(t, single_disk_ctx(), st);
    EXPECT_FALSE(a1->overlap()) << "dot-less names skip the twin path (the \"\" quirk)";
    EXPECT_FALSE(a2->overlap());
}

TEST(Grading, RefsDeletedFilesGradeHigh) {
    /* S-061: the og's insert-time hook (FUN_14038a470 :605492-496) sets
     * grade 2 (High) for every non-existing ReFS node with the gauge
     * ticks - the completion pass excludes FS 10 from the generic path
     * precisely because insert already graded it. */
    ResultTree t;
    auto* root = t.add_root(u"Found");
    auto* dead = add_file(t, root, "notes.txt", 500, u32(FsKindCode::Refs), DeletedState::Deleted);
    grading::AssistantStats st;
    grading::grade_tree(t, single_disk_ctx(), st);
    EXPECT_EQ(Grade::High, dead->grade()) << "deleted ReFS rows grade High at insert";
}

TEST(Grading, BtrfsDeletedFilesGradeHigh) {
    /* S-061 twin: FS 0xB gets the same insert-time grade-2 rule
     * (:605485-489). */
    ResultTree t;
    auto* root = t.add_root(u"Found");
    auto* dead = add_file(t, root, "photo.raw", 500, u32(FsKindCode::Btrfs), DeletedState::Deleted);
    grading::AssistantStats st;
    grading::grade_tree(t, single_disk_ctx(), st);
    EXPECT_EQ(Grade::High, dead->grade()) << "deleted Btrfs rows grade High at insert";
}

TEST(Grading, RefsExistingStillHigh) {
    /* S-061 guard: existing (Active) ReFS files already grade High via
     * the Active rule that PRECEDES the ReFS/Btrfs rule - identical
     * observable outcome, pinned so the rule ordering cannot regress. */
    ResultTree t;
    auto* root = t.add_root(u"Found");
    auto* live = add_file(t, root, "live.txt", 500, u32(FsKindCode::Refs), DeletedState::Active);
    grading::AssistantStats st;
    grading::grade_tree(t, single_disk_ctx(), st);
    EXPECT_EQ(Grade::High, live->grade());
}

TEST(Grading, TwinAddressFlagsBoth) {
    /* twins must be same-extent-end AND same category (DRWUI :605774-
     * 605781 matches keys AND categories before FUN_14038ba60 flags) */
    ResultTree t;
    auto* root = t.add_root(u"Found");
    /* same extent-end + same category -> twins. S-058: the twin
     * fixtures ride OUT-OF-TABLE extensions (.zzqx/.ipynb) - the
     * corrected og gate runs the twin machinery only when the 169-key
     * lookup MISSES; the pre-fix fixtures used .doc/.mp3 (in-table)
     * which now skip the deep path. */
    auto* x1 = add_file(t, root, "x.zzqx", 500, u32(FsKindCode::Fat16), DeletedState::Deleted,
                        Origin::Structured, 0x5000, 0x400);
    auto* x2 = add_file(t, root, "y.zzqx", 0x400, u32(FsKindCode::Fat16), DeletedState::Deleted,
                        Origin::Structured, 0x5100, 0x300);
    /* same extent-end + DIFFERENT category (zzqx = Other vs ace =
     * Archive; both out-of-grading-table so the gate passes) -> not
     * twins */
    auto* y1 = add_file(t, root, "p1.zzqx", 500, u32(FsKindCode::Fat16), DeletedState::Deleted,
                        Origin::Structured, 0xA000, 0x400);
    auto* y2 = add_file(t, root, "p2.ace", 500, u32(FsKindCode::Fat16), DeletedState::Deleted,
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

/* ---- M-133: the twin-key device-family shift set ----------------------- */

namespace {
/* Two slots one kDeviceUnit apart: slot 0 (ordinal 0, the unshifted
 * neighbor) and slot 1 (ordinal 1, the device under test). Files at the
 * SAME in-slot extent end collide iff the slot-1 kind shifts: key_X =
 * U + end + (1 - s)U vs key_Y = end + 0 -> equal iff s == 2. This is
 * the exact arithmetic the og's kind set drives (DRWUI :605752-756:
 * kind 1 [FAT16] and kind 4 [EXT2/3] shift; everything else does not). */
grading::GradeContext two_slot_ctx(u32 slot1_kind) {
    grading::GradeContext ctx;
    grading::DeviceSlot s0;
    s0.base = 0;
    s0.disk_ordinal = 0;
    s0.fs_kind =
        u32(FsKindCode::Hfsplus); /* the unshifted neighbor: a
                                   * kind that reaches the twin path (NTFS/ReFS/Btrfs are
                                   * intercepted by solo_rules before twin collection - "twins are a
                                   * FAT-family path" is the og's own comment) */
    ctx.devices.push_back(s0);
    grading::DeviceSlot s1;
    s1.base = grading::kDeviceUnit;
    s1.disk_ordinal = 1;
    s1.fs_kind = slot1_kind;
    ctx.devices.push_back(s1);
    return ctx;
}

/* A deleted out-of-table (.doc fixture name kept for the M-133 lookup;
 * the S-058 gate needs out-of-table exts to reach the twin machinery -
 * zzqx below) pair at the same in-slot extent end: one on
 * slot 1 (the kind under test), one on slot 0 (NTFS, never shifted). */
std::pair<FileNode*, FileNode*> add_twin_pair(ResultTree& t, u32 slot1_kind) {
    auto* root = t.add_root(u"Found");
    add_file(t, root, "x.zzqx", 500, slot1_kind, DeletedState::Deleted, Origin::Structured,
             grading::kDeviceUnit + 0x5000, 0x400);
    add_file(t, root, "y.zzqx", 500, u32(FsKindCode::Hfsplus), DeletedState::Deleted,
             Origin::Structured, 0x5000, 0x400);
    FileNode* x = nullptr;
    FileNode* y = nullptr;
    for (auto* n = root->first_child(); n; n = n->next_sibling()) {
        if (n->name_utf8() == "x.zzqx")
            x = n;
        else if (n->name_utf8() == "y.zzqx")
            y = n;
    }
    return {x, y};
}
} // namespace

TEST(Grading, TwinKeyFat16StillShifts) {
    /* M-133 pin: FAT16 (og kind 1) KEEPS the -2 shift - the one FAT
     * member the og shifts. Same in-slot extent end + shift 2 -> the
     * keys coincide -> twins. Passes pre- and post-fix (the pin). */
    ResultTree t;
    auto [x, y] = add_twin_pair(t, u32(FsKindCode::Fat16));
    ASSERT_NE(x, nullptr);
    ASSERT_NE(y, nullptr);
    grading::AssistantStats st;
    grading::grade_tree(t, two_slot_ctx(u32(FsKindCode::Fat16)), st);
    EXPECT_TRUE(x->overlap()) << "FAT16 keeps the og's -2 shift (kind 1)";
    EXPECT_TRUE(y->overlap());
}

TEST(Grading, TwinKeyFat32DoesNotShift) {
    /* M-133 RED: the og shifts kinds {1, 4} ONLY - FAT32 (kind 2) is
     * NOT shifted. The pre-fix core shifted the whole FAT family: a
     * FAT32 device's files collided with the neighbor ordinal's. */
    ResultTree t;
    auto [x, y] = add_twin_pair(t, u32(FsKindCode::Fat32));
    ASSERT_NE(x, nullptr);
    ASSERT_NE(y, nullptr);
    grading::AssistantStats st;
    grading::grade_tree(t, two_slot_ctx(u32(FsKindCode::Fat32)), st);
    EXPECT_FALSE(x->overlap()) << "FAT32 does not shift (og kind 2)";
    EXPECT_FALSE(y->overlap());
}

TEST(Grading, TwinKeyExfatDoesNotShift) {
    /* M-133 RED: exFAT (og kind 7) is NOT shifted. */
    ResultTree t;
    auto [x, y] = add_twin_pair(t, u32(FsKindCode::Exfat));
    ASSERT_NE(x, nullptr);
    ASSERT_NE(y, nullptr);
    grading::AssistantStats st;
    grading::grade_tree(t, two_slot_ctx(u32(FsKindCode::Exfat)), st);
    EXPECT_FALSE(x->overlap()) << "exFAT does not shift (og kind 7)";
    EXPECT_FALSE(y->overlap());
}

TEST(Grading, TwinKeyFat12DoesNotShift) {
    /* M-133 RED: FAT12 (og kind 0) is NOT shifted. */
    ResultTree t;
    auto [x, y] = add_twin_pair(t, u32(FsKindCode::Fat12));
    ASSERT_NE(x, nullptr);
    ASSERT_NE(y, nullptr);
    grading::AssistantStats st;
    grading::grade_tree(t, two_slot_ctx(u32(FsKindCode::Fat12)), st);
    EXPECT_FALSE(x->overlap()) << "FAT12 does not shift (og kind 0)";
    EXPECT_FALSE(y->overlap());
}

TEST(Grading, TwinKeyExt4NtfsNeverShift) {
    /* M-133 pins: the unshifted majority - EXT4 (9, NOT the shifted
     * code 4) and ZFS (13) never shift; both reach the twin path so the
     * no-collision verdict is meaningful, not vacuous. */
    for (u32 kind : {u32(FsKindCode::Ext4), u32(FsKindCode::Zfs)}) {
        ResultTree t;
        add_twin_pair(t, kind);
        auto* root = t.roots()[0];
        FileNode *x = nullptr, *y = nullptr;
        for (auto* n = root->first_child(); n; n = n->next_sibling())
            (n->name_utf8() == "x.zzqx" ? x : y) = n;
        grading::AssistantStats st;
        grading::grade_tree(t, two_slot_ctx(kind), st);
        EXPECT_FALSE(x->overlap()) << "kind " << kind << " never shifts";
        EXPECT_FALSE(y->overlap());
    }
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

TEST(Report, ScanAnalysisCarriesPerFormat) {
    /* N-140: the per-format carve breakdown rides the scan_analysis
     * block - the og's GetRawUserInfoMap post-scan rendering (DRWUI
     * :227538-227625). The rows are nonzero-only in unified id order;
     * zero rows stay absent; slots==0 omits the counts (an absent
     * carve stage). RED pre-fix: the fields did not exist. */
    recovery::report::ScanAnalysis a;
    a.total_file_count = 7;
    a.carve_format_slots = 300; /* a live bound (grammar+raw+usersign) */
    a.carve_format_counts[3] = 405; /* e.g. the JPG grammar row */
    a.carve_format_counts[200] = 42; /* a raw-sign row */
    a.carve_format_counts[299] = 7;  /* the user-sign aggregate slot */
    recovery::report::Totals t;
    const std::string doc = recovery::report::build_report({}, t, &a);
    EXPECT_NE(std::string::npos, doc.find("\"carve_formats\""));
    EXPECT_NE(std::string::npos, doc.find("{\"id\": 3, \"count\": 405}"));
    EXPECT_NE(std::string::npos, doc.find("{\"id\": 200, \"count\": 42}"));
    EXPECT_NE(std::string::npos, doc.find("{\"id\": 299, \"count\": 7}"));
    EXPECT_NE(std::string::npos, doc.find("\"carve_format_overflow\":0"));
    /* the zero rows never render: id 4 has count 0 */
    EXPECT_EQ(std::string::npos, doc.find("{\"id\": 4,"));
    /* no carve stage: slots 0 -> an empty array, no fabricated rows */
    recovery::report::ScanAnalysis b;
    b.carve_format_slots = 0;
    const std::string plain = recovery::report::build_report({}, t, &b);
    EXPECT_NE(std::string::npos, plain.find("\"carve_formats\": []"));
    /* the overflow counter is surfaced, never silently dropped */
    b.carve_format_slots = 2;
    b.carve_format_counts[0] = 1;
    b.carve_format_overflow = 3;
    const std::string ofl = recovery::report::build_report({}, t, &b);
    EXPECT_NE(std::string::npos, ofl.find("\"carve_format_overflow\":3"));
}

/* ===================================================================== */
/* M-134 (diff-to-fix): the og's deep-read grade validation              */
/* (FUN_14038c470 :606291-619, decoded) - a bounded ONE-SECTOR content  */
/* read that flips the grade on actual disk bytes: addr==0 -> first-32- */
/* zero or first-4-0xFF grades Low; addr>0 -> the previous sector's     */
/* last-16-zero grades High; the overlap flag skips the read; the       */
/* graded bit makes the per-selection re-grade a one-shot.              */
/* ===================================================================== */

namespace {
/* a controllable reader device for the content rules (512-byte sectors) */
class GradeMemDevice final : public IStorageDevice {
public:
    explicit GradeMemDevice(u64 bytes, u32 sector = 512) : data_(size_t(bytes), 0) {
        geom_.logical_sector = sector;
        geom_.physical_sector = sector;
        geom_.sector_count = bytes / sector;
        geom_.total_bytes = geom_.sector_count * sector;
    }
    u32 read_at(u64 off, void* buf, u32 len) override {
        if (off > data_.size() || len > data_.size() - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        std::memcpy(buf, data_.data() + off, len);
        return OK;
    }
    const Geometry& geometry() const override { return geom_; }
    std::vector<u8> data_;

private:
    Geometry geom_;
};
} // namespace

TEST(Grading, DeepReadZeroedFirstSectorGradesLow) {
    /* M-146 re-pin: the addr==0 class is the EXTENTLESS node (the twin
     * key's end term is zero) - the og reads the reader's sector 0; its
     * first 32 bytes ZERO -> Low. (The pre-M-145 pin read the head of a
     * sector-0-STARTING file - the START-address misreading; an unshifted
     * kind [Fat32] so the index term is zero and the read proceeds.) */
    GradeMemDevice dev(u64(64) * 512);
    grading::GradeContext ctx = single_disk_ctx();
    ctx.reader.dev = &dev;
    ResultTree t;
    auto* root = t.add_root(u"Found");
    auto* f = add_file(t, root, "gone.zzqx", 500, u32(FsKindCode::Fat32), DeletedState::Deleted,
                       Origin::Structured, 0, 0);
    dev.data_[0] = 0; /* the whole first sector stays zeroed */
    grading::AssistantStats st;
    grading::grade_tree(t, ctx, st);
    EXPECT_EQ(Grade::Low, f->grade()) << "the extentless node over a zeroed sector 0 -> Low";
}

TEST(Grading, DeepRead0xFFHeadGradesLow) {
    /* M-146 re-pin: the extentless class again - the reader's sector 0
     * starting with 4x 0xFF (the erased-media signature) -> Low. */
    GradeMemDevice dev(u64(64) * 512);
    std::fill(dev.data_.begin(), dev.data_.begin() + 8, 0xFF);
    grading::GradeContext ctx = single_disk_ctx();
    ctx.reader.dev = &dev;
    ResultTree t;
    auto* root = t.add_root(u"Found");
    auto* f = add_file(t, root, "erased.zzqx", 500, u32(FsKindCode::Fat32), DeletedState::Deleted,
                       Origin::Structured, 0, 0);
    grading::AssistantStats st;
    grading::grade_tree(t, ctx, st);
    EXPECT_EQ(Grade::Low, f->grade()) << "the 0xFF head of sector 0 demotes the extentless node";
}

TEST(Grading, DeepReadOwnLastSectorTailZeroGradesHigh) {
    /* M-146 re-pin: the read is at the twin key - 1 = the extent's LAST
     * sector (the key helper's out-triple at DRWUI :322666-68: START +
     * LENGTH + index*unit). A file at sector 2 spanning sectors 2-3
     * whose OWN final sector (3) ends in 16 zero bytes - the intact-file
     * padding signature - grades High. (The pre-M-145 pin read sector 1,
     * the sector BEFORE the file - the START-address misreading.) */
    GradeMemDevice dev(u64(64) * 512);
    /* sector 3: nonzero head so the sector is real, zero last 16 */
    dev.data_[3 * 512] = 0x77;
    dev.data_[3 * 512 + 100] = 0x33;
    std::fill(dev.data_.begin() + 3 * 512 + 512 - 16, dev.data_.begin() + 4 * 512, 0);
    grading::GradeContext ctx = single_disk_ctx();
    ctx.reader.dev = &dev;
    ResultTree t;
    auto* root = t.add_root(u"Found");
    auto* f = add_file(t, root, "padded.zzqx", 500, u32(FsKindCode::Fat32), DeletedState::Deleted,
                       Origin::Structured, 2 * 512, 2 * 512);
    grading::AssistantStats st;
    grading::grade_tree(t, ctx, st);
    EXPECT_EQ(Grade::High, f->grade()) << "the zero tail of the file's OWN last sector -> High";
}

TEST(Grading, DeepReadPrevSectorPaddingNotHigh) {
    /* M-146 (the core's old false positive): a file whose PREVIOUS
     * sector ends in 16 zero bytes but whose OWN last sector does NOT -
     * the og reads the file's final sector and leaves the grade alone.
     * The pre-M-145 core read the preceding sector and promoted. */
    GradeMemDevice dev(u64(64) * 512);
    /* sector 1 (before the file): nonzero head, zero tail */
    dev.data_[512] = 0x77;
    std::fill(dev.data_.begin() + 512 + 512 - 16, dev.data_.begin() + 2 * 512, 0);
    /* sectors 2-3 (the file's own): all nonzero */
    std::fill(dev.data_.begin() + 2 * 512, dev.data_.begin() + 4 * 512, 0x5A);
    grading::GradeContext ctx = single_disk_ctx();
    ctx.reader.dev = &dev;
    ResultTree t;
    auto* root = t.add_root(u"Found");
    auto* f = add_file(t, root, "afterpad.zzqx", 500, u32(FsKindCode::Fat32), DeletedState::Deleted,
                       Origin::Structured, 2 * 512, 2 * 512);
    grading::AssistantStats st;
    grading::grade_tree(t, ctx, st);
    EXPECT_NE(Grade::High, f->grade()) << "previous-sector padding never promotes";
}

TEST(Grading, DeepReadSector0StartFileNotDemotedByHead) {
    /* M-146 (the second false class): a file that STARTS at sector 0 is
     * NOT the addr==0 class - its extent end is nonzero, the og reads
     * its OWN last sector. A zeroed first sector does not demote it. */
    GradeMemDevice dev(u64(64) * 512);
    /* sector 0: zeroed head (would have demoted under the old reading);
     * the file spans sectors 0-1, its last sector (1) is nonzero */
    std::fill(dev.data_.begin() + 512, dev.data_.begin() + 2 * 512, 0x5A);
    grading::GradeContext ctx = single_disk_ctx();
    ctx.reader.dev = &dev;
    ResultTree t;
    auto* root = t.add_root(u"Found");
    auto* f = add_file(t, root, "atzero.zzqx", 500, u32(FsKindCode::Fat32), DeletedState::Deleted,
                       Origin::Structured, 0, 2 * 512);
    grading::AssistantStats st;
    grading::grade_tree(t, ctx, st);
    EXPECT_NE(Grade::Low, f->grade()) << "a sector-0 start with a live last sector stays put";
}

TEST(Grading, DeepReadShiftedKindSkippedLikeOgFailedRead) {
    /* M-146 (the failed-read class): FAT16 carries the M-133 -2 index
     * term - the og's read address is out of any reader, the read FAILS,
     * the grade never changes. The core skips the same population; the
     * zero-padded own last sector must NOT promote a Fat16 node. */
    GradeMemDevice dev(u64(64) * 512);
    dev.data_[3 * 512] = 0x77;
    std::fill(dev.data_.begin() + 3 * 512 + 512 - 16, dev.data_.begin() + 4 * 512, 0);
    grading::GradeContext ctx = single_disk_ctx();
    ctx.reader.dev = &dev;
    ResultTree t;
    auto* root = t.add_root(u"Found");
    auto* f = add_file(t, root, "shifted.zzqx", 500, u32(FsKindCode::Fat16), DeletedState::Deleted,
                       Origin::Structured, 2 * 512, 2 * 512);
    grading::AssistantStats st;
    grading::grade_tree(t, ctx, st);
    EXPECT_EQ(Grade::Average, f->grade()) << "the shifted kind's read fails: no content grade";
}

TEST(Grading, DeepReadSkipsOverlapFlagged) {
    /* the overlap flag skips the content read (FUN_1401c5da0 != 0 ->
     * return): twins at zeroed content keep their twin-induced High -
     * the content rule never demotes them */
    GradeMemDevice dev(u64(64) * 512); /* all-zero content */
    grading::GradeContext ctx = single_disk_ctx();
    ctx.reader.dev = &dev;
    ResultTree t;
    auto* root = t.add_root(u"Found");
    auto* a = add_file(t, root, "twin1.zzqx", 500, u32(FsKindCode::Fat32), DeletedState::Deleted,
                       Origin::Structured, 0x5000, 0x400);
    auto* b = add_file(t, root, "twin2.zzqx", 500, u32(FsKindCode::Fat32), DeletedState::Deleted,
                       Origin::Structured, 0x5000, 0x400);
    /* the only file NOT twinned sits at byte 0 over zeroed content */
    auto* solo = add_file(t, root, "solo.zzqx", 500, u32(FsKindCode::Fat32), DeletedState::Deleted,
                          Origin::Structured, 0, 0);
    grading::AssistantStats st;
    grading::grade_tree(t, ctx, st);
    EXPECT_TRUE(a->overlap());
    EXPECT_TRUE(b->overlap());
    EXPECT_EQ(Grade::High, a->grade()) << "the twins keep their High - no content read";
    EXPECT_EQ(Grade::High, b->grade());
    EXPECT_EQ(Grade::Low, solo->grade()) << "the unflagged EXTENTLESS solo over zeroed sector 0";
}

TEST(Grading, RegradeRunsFullTreeOnce) {
    /* the graded bit makes the per-selection re-grade a true one-shot
     * (FUN_1401d7110's grade==1 && graded-bit-clear gate). Fail-before:
     * regrade re-ran forever (idempotent but unbounded). */
    ResultTree t;
    auto* root = t.add_root(u"Found");
    auto* avg = add_file(t, root, "avg.zzqx", 5000, u32(FsKindCode::Fat16), DeletedState::Deleted,
                         Origin::Structured, 0x9000, 0x400);
    grading::AssistantStats st;
    /* no reader: the completion pass marks graded only through the deep
     * read loop - without a reader the node stays ungraded (the ABI
     * re-grade path). With a reader it would be marked here already. */
    grading::grade_tree(t, single_disk_ctx(), st);
    ASSERT_EQ(Grade::Average, avg->grade());
    EXPECT_FALSE(avg->graded()) << "no reader: the completion pass left the bit clear";
    EXPECT_TRUE(grading::regrade_node(avg, single_disk_ctx(), &t))
        << "first call: eligible (grade 1, bit clear)";
    EXPECT_TRUE(avg->graded()) << "the re-grade marks the one-shot bit";
    EXPECT_FALSE(grading::regrade_node(avg, single_disk_ctx(), &t))
        << "second call: the one-shot gate refuses";
}

TEST(Grading, DeepReadReaderAbsentChangesNothing) {
    /* the structural back-compat pin: no reader -> the content rules are
     * skipped exactly as pre-M-134 (the ABI session re-grade path) */
    GradeMemDevice dev(u64(64) * 512);
    ResultTree t;
    auto* root = t.add_root(u"Found");
    auto* f = add_file(t, root, "zeroed.zzqx", 500, u32(FsKindCode::Fat16), DeletedState::Deleted,
                       Origin::Structured, 0, 0x400);
    (void)dev;
    grading::AssistantStats st;
    grading::grade_tree(t, single_disk_ctx(), st);
    EXPECT_NE(Grade::Low, f->grade())
        << "without a reader the zeroed content cannot demote (the registered seam)";
}

/* ---- N-205 (diff-to-fix): the rawCount population ------------------ */

TEST(Grading, RawTailZeroTicksForcedProbePopulation) {
    /* N-205: the og's rawCount is the VALIDATED-RECOVERABLE count - the
     * forced observe-only tail-zero probe (FUN_14038c470's param_3=1
     * form) over grade-1 NTFS nodes with 0 < key < device-size, ANDed
     * with the 0x23-default size arm (<= 0x77fffff), ticked at
     * quick-scan populate (FUN_1401c7d80, DRWUI:331183-311). The core's
     * raw_tail_zero carries that population. A node whose final sector
     * ends in the 16-byte zero padding signature ticks; the same-shape
     * node with a nonzero tail does not; a >0x77fffff node does not. */
    GradeMemDevice dev(u64(64) * 512);
    grading::GradeContext ctx = single_disk_ctx();
    ctx.reader.dev = &dev;
    ResultTree t;
    auto* root = t.add_root(u"Found");
    /* A: extent [512,1024) - the final sector (1) carries real head +
     * the zero tail signature -> ticks */
    dev.data_[1 * 512] = 0x55;
    dev.data_[1 * 512 + 100] = 0x33;
    add_file(t, root, "tick.bin", 600, u32(FsKindCode::Ntfs), DeletedState::Deleted,
             Origin::Structured, 512, 512);
    /* B: extent [1536,2048) - the final sector (3) ends NONZERO -> no tick */
    dev.data_[3 * 512] = 0x55;
    std::fill(dev.data_.begin() + 4 * 512 - 16, dev.data_.begin() + 4 * 512, 0xAB);
    add_file(t, root, "dirty.bin", 600, u32(FsKindCode::Ntfs), DeletedState::Deleted,
             Origin::Structured, 1536, 512);
    /* C: extent [2048,3072) - the zero tail signature but size 0x7800000
     * > 0x77fffff (the 0x23 default arm refuses) -> no tick */
    dev.data_[5 * 512] = 0x44;
    add_file(t, root, "huge.bin", 0x7800000ull, u32(FsKindCode::Ntfs), DeletedState::Deleted,
             Origin::Structured, 2048, 1024);
    grading::AssistantStats st;
    grading::grade_tree(t, ctx, st);
    EXPECT_EQ(u64(1), st.raw_tail_zero)
        << "exactly the tail-zero, size-legal NTFS node ticks (A yes; B no tail; C no size arm)";
}

TEST(Grading, RawTailZeroSizeArmBoundary) {
    /* N-205: the 0x23-default size arm's exact boundary - 0x77fffff is
     * the last ticking size, 0x7800000 the first refused (FUN_1401c76e0's
     * <= form). Both nodes carry the identical zero-tail final sector. */
    GradeMemDevice dev(u64(64) * 512);
    grading::GradeContext ctx = single_disk_ctx();
    ctx.reader.dev = &dev;
    ResultTree t;
    auto* root = t.add_root(u"Found");
    add_file(t, root, "edge_in.bin", 0x77fffffull, u32(FsKindCode::Ntfs), DeletedState::Deleted,
             Origin::Structured, 512, 512);
    add_file(t, root, "edge_out.bin", 0x7800000ull, u32(FsKindCode::Ntfs), DeletedState::Deleted,
             Origin::Structured, 1536, 512);
    grading::AssistantStats st;
    grading::grade_tree(t, ctx, st);
    EXPECT_EQ(u64(1), st.raw_tail_zero)
        << "0x77fffff ticks, 0x7800000 does not (the arm's boundary)";
}

TEST(Grading, RawCountSemanticsRegistered) {
    /* N-205 option (b): raw_count keeps the landed Origin::Carved census
     * (the report field's shipped meaning); the deliberate divergence
     * from the og's validated-recoverable rawCount is REGISTERED (the
     * D-146 row [renumbered D-143->D-144->D-145->D-146 across the push-rebases: the
     * racing windows' N-196/N-198/N-202 keep the lower seats]) with raw_tail_zero
     * carrying the og-parity population.
     * The pin: the register row + the header's mapping comment exist and
     * name the divergence. */
    const std::string deltas = cr_test::read_repo_file("docs/PARITY-DELTAS.md");
    ASSERT_FALSE(deltas.empty());
    EXPECT_NE(deltas.find("### D-146"), std::string::npos)
        << "the raw_count semantics divergence must carry its D-row";
    const std::string hdr = cr_test::read_repo_file("include/cr/tree/cr_grading.h");
    ASSERT_FALSE(hdr.empty());
    EXPECT_NE(hdr.find("raw_count semantics (N-205"), std::string::npos)
        << "the header's counter-mapping block must state the registered divergence";
    /* the population split is real: a carved node ticks raw_count but
     * never raw_tail_zero; the tail-zero NTFS node above ticks
     * raw_tail_zero but not raw_count */
    GradeMemDevice dev(u64(64) * 512);
    grading::GradeContext ctx = single_disk_ctx();
    ctx.reader.dev = &dev;
    ResultTree t;
    auto* root = t.add_root(u"Found");
    dev.data_[1 * 512] = 0x55;
    add_file(t, root, "carved.bin", 50, u32(FsKindCode::Raw), DeletedState::Deleted,
             Origin::Carved);
    add_file(t, root, "tick.bin", 600, u32(FsKindCode::Ntfs), DeletedState::Deleted,
             Origin::Structured, 512, 512);
    grading::AssistantStats st;
    grading::grade_tree(t, ctx, st);
    EXPECT_EQ(u64(1), st.raw_count);
    EXPECT_EQ(u64(1), st.raw_tail_zero)
        << "the two counters count disjoint populations (provenance vs validated)";
}
