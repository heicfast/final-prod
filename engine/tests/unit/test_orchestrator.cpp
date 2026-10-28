/* Orchestrator + safe recovery writer tests. */
#include <gtest/gtest.h>
#include "cr/orchestrator/cr_scan.h"
#include "cr/orchestrator/cr_filterfolder.h"
#include "cr/query/cr_query.h"
#include "cr/cr_errmod.h"
#include "cr/recovery/cr_safewriter.h"
#include "cr/session/cr_crsf.h"
#include "cr/storage/cr_device.h"
#include "test_fixtures.hpp"
#include "test_tmpdir.hpp"
#include <algorithm>
#include <cstring>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "cr/cr_log.h"

using namespace cr;
using namespace cr::orchestrator;
using namespace cr::recovery;

namespace {

class MemDevice final : public IStorageDevice {
public:
    explicit MemDevice(std::vector<u8> b) : data_(std::move(b)) {
        geom_.logical_sector = 512;
        geom_.physical_sector = 512;
        geom_.sector_count = data_.size() / 512;
        geom_.total_bytes = data_.size();
    }
    u32 read_at(u64 off, void* buf, u32 len) override {
        if (off > data_.size() || len > data_.size() - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        std::memcpy(buf, data_.data() + off, len);
        return OK;
    }
    const Geometry& geometry() const override { return geom_; }
    std::vector<u8>& raw() { return data_; }

private:
    std::vector<u8> data_;
    Geometry geom_;
};

void put16(std::vector<u8>& v, size_t at, u16 x) {
    v[at] = u8(x);
    v[at + 1] = u8(x >> 8);
}
void put32v(std::vector<u8>& v, size_t at, u32 x) {
    v[at] = u8(x);
    v[at + 1] = u8(x >> 8);
    v[at + 2] = u8(x >> 16);
    v[at + 3] = u8(x >> 24);
}

/* minimal FAT16 volume: 5074 sectors, clusters 5(root),10(file),20(file) */
std::vector<u8> build_fat16() {
    std::vector<u8> img(u64(5074) * 512, 0);
    u8* b = img.data();
    std::memcpy(b + 3, "MSWIN4.1", 8);
    put16(img, 0x0B, 512);
    b[0x0D] = 1;
    put16(img, 0x0E, 2);
    b[0x10] = 2;
    put16(img, 0x11, 512);
    put16(img, 0x13, 5074);
    b[0x15] = 0xF8;
    put16(img, 0x16, 20);
    put16(img, 0x18, 63);
    put16(img, 0x1A, 255);
    b[0x26] = 0x29;
    std::memcpy(b + 0x2B, "TESTVOL    ", 11);
    std::memcpy(b + 0x36, "FAT16   ", 8);
    b[510] = 0x55;
    b[511] = 0xAA;

    const size_t FAT1 = 2 * 512, FAT2 = 22 * 512, ROOT = 42 * 512, DATA = 74 * 512;
    /* M-015: canonical media words (real-volume shape; the whole-table
     * validator requires u32@FAT[0] == 0xFFFFFFF8 on FAT16) */
    put16(img, FAT1 + 0 * 2, 0xFFF8);
    put16(img, FAT1 + 1 * 2, 0xFFFF);
    put16(img, FAT2 + 0 * 2, 0xFFF8);
    put16(img, FAT2 + 1 * 2, 0xFFFF);
    put16(img, FAT1 + 5 * 2, 0xFFFF);
    put16(img, FAT2 + 5 * 2, 0xFFFF); /* root dir */
    put16(img, FAT1 + 10 * 2, 11);
    put16(img, FAT2 + 10 * 2, 11);
    put16(img, FAT1 + 11 * 2, 12);
    put16(img, FAT2 + 11 * 2, 12);
    put16(img, FAT1 + 12 * 2, 0xFFFF);
    put16(img, FAT2 + 12 * 2, 0xFFFF); /* doc */
    put16(img, FAT1 + 20 * 2, 0xFFFF);
    put16(img, FAT2 + 20 * 2, 0xFFFF); /* del file */

    u8* root = img.data() + ROOT;
    /* dentry writer: name 11 chars + attrs + cluster + size */
    auto dentry = [&](size_t slot, const char* n11, u8 attr, u32 cl, u32 sz, bool del) {
        u8* e = root + slot * 32;
        std::memset(e, 0, 32);
        for (int i = 0; i < 11; ++i)
            e[i] = u8(n11[i]);
        if (del)
            e[0] = 0xE5;
        e[11] = attr;
        put16(img, ROOT + slot * 32 + 20, u16(cl >> 16));
        put16(img, ROOT + slot * 32 + 26, u16(cl));
        put32v(img, ROOT + slot * 32 + 28, sz);
    };
    dentry(0, "DOC     TXT", 0x20, 10, 1000, false);
    dentry(1, "DELFILE TXT", 0x20, 20, 500, true);
    /* content markers */
    std::memcpy(img.data() + DATA + (10 - 2) * 512, "DOCDATA", 7);
    std::memcpy(img.data() + DATA + (20 - 2) * 512, "DELDATA", 7);
    return img;
}

/* Mock DestIo */
struct MockIo final : DestIo {
    std::vector<std::u16string> created;
    std::vector<std::u16string> committed;
    std::map<u64, std::vector<u8>> files;  /* item_id -> content */
    std::map<u64, std::u16string> finals;  /* item_id -> final path */
    std::set<std::u16string> existing;     /* for exists() */
    std::vector<std::u16string> made_dirs; /* G-08 mkdir log */

    u32 create_temp(u64 id, const std::u16string& final_path,
                    const std::u16string& temp_suffix) override {
        (void)temp_suffix;
        created.push_back(final_path);
        finals[id] = final_path;
        files[id] = {};
        return 0;
    }
    u32 mkdir(const std::u16string& p) override {
        made_dirs.push_back(p);
        existing.insert(p); /* a made dir also exists */
        return 0;
    }
    u32 write(u64 id, u64 off, const void* data, u32 len) override {
        auto& f = files[id];
        if (f.size() < off + len)
            f.resize(size_t(off + len));
        std::memcpy(f.data() + off, data, len);
        return 0;
    }
    u32 commit(u64 id) override {
        committed.push_back(finals[id]);
        existing.insert(finals[id]);
        return 0;
    }
    void close(u64) override {}
    bool exists(const std::u16string& p) override { return existing.count(p) != 0; }

    std::string content_of(u64 id) const {
        auto it = files.find(id);
        if (it == files.end())
            return {};
        return std::string(it->second.begin(), it->second.end());
    }
};

std::u16string u16_(const char* s) {
    std::u16string o;
    for (const char* p = s; *p; ++p)
        o.push_back(u16(u8(*p)));
    return o;
}

TEST(Orchestrator, DynamicDiskVolumeScan) {
    /* a FAT16 volume living inside an LDM simple volume on an MBR dynamic
     * disk: the orchestrator must parse the LDM database, build the
     * virtual volume device and scan it like any candidate */
    auto fat = build_fat16(); /* 5074 sectors */
    constexpr u64 PART_START = 2048;
    constexpr u64 PART_SIZE = 5074;
    cr::testfixture::LdmFixture fx(40960); /* 20 MiB disk */
    fx.set_disk(1000, "01234567-89AB-CDEF-0123-456789ABCDEF");
    fx.add_dgr4(1, "TestPCDg0");
    fx.add_dsk4(1000, "Disk1", "01234567-89AB-CDEF-0123-456789ABCDEF");
    fx.add_volume({2000, "Volume1", 3, PART_SIZE, 1});
    fx.add_component({3000, "Volume1-01", cr::table::LDM_COMP_CONCAT, 1, 2000, 0, 0});
    fx.add_partition({4000, "Disk1-01", PART_START, PART_SIZE, 0, 3000, 0, 1000});
    fx.finish();
    auto img = fx.take();
    std::memcpy(img.data() + PART_START * 512, fat.data(), fat.size());

    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    ScanConfig cfg;
    cfg.run_quick = true;
    cfg.run_deep = true;
    cfg.run_carve = false;
    cfg.whole_disk = true;
    ScanController ctrl(tree, cfg);
    ScanStats stats;
    u32 rc = ctrl.run(&dev, nullptr, nullptr, nullptr, stats);
    ASSERT_EQ(rc, OK) << err_text(rc);
    EXPECT_GE(stats.files_found, 1u);   /* DOC.TXT inside the dynamic volume */
    EXPECT_GE(stats.deleted_found, 1u); /* DELFILE.TXT */

    /* the volume was scanned under a "Dynamic Volume Volume1" root */
    tree::FileNode* doc = nullptr;
    std::function<void(tree::FileNode*)> collect = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            if (c->name_utf8() == "DOC.TXT")
                doc = c;
            collect(c);
        }
    };
    for (tree::FileNode* r : tree.roots())
        collect(r);
    ASSERT_NE(nullptr, doc);
}

/* ---- S-051 (diff-to-fix): the whole-disk flow's partition-list targets ----
 *
 * The og's disk-level flow (architecture/04-recovery-workflow.md §2/§3.4:
 * CDoScanPartition -> CGenericRecoveryController) reads the partition list
 * at scan setup and makes every table volume a scan target. The core
 * previously probed LBA 0, saw the MBR, dispatched Unknown and completed
 * rc=0 with ZERO files on any healthy partitioned disk (the executed RUN 2,
 * evidence/executions/s10-wholedisk-scan-targets.md). These tests pin the
 * fixed target construction; M-104's display-name and known-vs-lost
 * distinctions ride the same mechanism. */

namespace {

/* count nodes by exact name anywhere in the tree */
u64 count_named(tree::ResultTree& tree, const char* utf8_name) {
    u64 n = 0;
    std::function<void(tree::FileNode*)> walk = [&](tree::FileNode* node) {
        if (node->name_utf8() == utf8_name)
            ++n;
        for (tree::FileNode* c = node->first_child(); c; c = c->next_sibling())
            walk(c);
    };
    for (auto* r : tree.roots())
        walk(r);
    return n;
}

/* collect root display names (role for full nodes, name for the rest) */
std::vector<std::string> root_names(tree::ResultTree& tree) {
    std::vector<std::string> out;
    for (auto* r : tree.roots())
        out.push_back(r->name_utf8());
    return out;
}

} // namespace

TEST(Orchestrator, WholeDiskScansTableVolumes) {
    /* the executed evidence's RUN 2 shape: a plain whole-disk target (no
     * --whole-disk, Unknown hint) on a healthy two-volume MBR disk MUST
     * find both volumes' files - previously rc=0 with an empty tree. */
    auto img = testfixture::build_two_volume_mbr_disk("VOLONE", "VOLTWO");
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    ScanConfig cfg;
    cfg.run_quick = true;
    cfg.run_deep = true;
    cfg.run_carve = false;
    cfg.whole_disk = false;
    ScanController ctrl(tree, cfg);
    ScanStats stats;
    u32 rc = ctrl.run(&dev, nullptr, nullptr, nullptr, stats);
    ASSERT_EQ(rc, OK) << err_text(rc);
    EXPECT_EQ(stats.files_found, 4u) << "both volumes' DOC.TXT + the deleted files' entries";
    EXPECT_GE(stats.deleted_found, 2u);
    EXPECT_EQ(stats.table_volume_targets, 2u);
    EXPECT_EQ(stats.table_volume_skipped, 0u);
    /* each volume's DOC.TXT found exactly once - under its own root */
    EXPECT_EQ(count_named(tree, "DOC.TXT"), 2u);
}

TEST(Orchestrator, WholeDiskGptTwin) {
    /* the same flow on the GPT twin: the GPT entry array (basic-data
     * partitions) drives the same per-volume targets. */
    auto img = testfixture::build_two_volume_gpt_disk("GPTVOL1", "GPTVOL2");
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    ScanConfig cfg;
    cfg.run_quick = true;
    cfg.run_carve = false;
    ScanController ctrl(tree, cfg);
    ScanStats stats;
    u32 rc = ctrl.run(&dev, nullptr, nullptr, nullptr, stats);
    ASSERT_EQ(rc, OK) << err_text(rc);
    EXPECT_EQ(stats.files_found, 4u);
    EXPECT_EQ(count_named(tree, "DOC.TXT"), 2u);
    const auto names = root_names(tree);
    EXPECT_NE(std::find(names.begin(), names.end(), "GPTVOL1"), names.end())
        << "the first volume's display-name root";
    EXPECT_NE(std::find(names.begin(), names.end(), "GPTVOL2"), names.end())
        << "the second volume's display-name root";
}

TEST(Orchestrator, WholeDiskMixedFamilies) {
    /* one FAT16 volume + one NTFS-probeable volume (the boot + record-0
     * shape): per-target family probing dispatches each through its own
     * engine; the NTFS target (no $Volume label decode yet - N-133's
     * landing) takes the FS-marker tier, never the anonymous form. */
    u64 s1 = 0, s2 = 0;
    auto img = testfixture::build_two_volume_mbr_disk("FATSIDE", "VOLTWO", &s1, &s2);
    /* plant the NTFS probe shape over the second volume's boot sector */
    u8* b = img.data() + s2 * 512;
    std::memset(b, 0, 512);
    std::memcpy(b + 3, "NTFS    ", 8);
    put16(img, s2 * 512 + 0x0B, 512);
    b[0x0D] = 1;
    b[0x15] = 0xF8;
    img[s2 * 512 + 0x28] = 200;
    img[s2 * 512 + 0x30] = 100; /* $MFT LCN 100 */
    img[s2 * 512 + 0x38] = 108;
    b[0x40] = 2;
    b[0x44] = 2;
    b[510] = 0x55;
    b[511] = 0xAA;
    /* the $MFT record 0 at volume-relative LCN 100 */
    const u64 rec = (s2 + 100) * 512;
    std::memset(img.data() + rec, 0, 1024);
    put32v(img, rec, 0x454C4946); /* 'FILE' */
    put16(img, rec + 4, 0x30);
    put16(img, rec + 6, 3);
    put16(img, rec + 0x14, 0x38);
    put16(img, rec + 0x18, 1);
    img[rec + 510] = 0x34;
    img[rec + 511] = 0x12;
    img[rec + 1022] = 0x34;
    img[rec + 1023] = 0x12;

    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    ScanConfig cfg;
    cfg.run_quick = true;
    cfg.run_carve = false;
    ScanController ctrl(tree, cfg);
    ScanStats stats;
    u32 rc = ctrl.run(&dev, nullptr, nullptr, nullptr, stats);
    ASSERT_EQ(rc, OK) << err_text(rc);
    EXPECT_EQ(stats.table_volume_targets, 2u);
    EXPECT_EQ(stats.table_volume_skipped, 0u);
    EXPECT_GE(stats.files_found, 2u) << "the FAT volume's DOC.TXT + deleted file";
    const auto names = root_names(tree);
    EXPECT_NE(std::find(names.begin(), names.end(), "FATSIDE"), names.end());
    /* the NTFS tier-2 name: the FS marker + the position, never anonymous */
    std::string expected = "NTFS @ " + std::to_string(s2);
    EXPECT_NE(std::find(names.begin(), names.end(), expected), names.end())
        << "expected the FS-marker root '" << expected << "'";
}

TEST(Orchestrator, WholeDiskDamagedVolumeSkippedHonestly) {
    /* one healthy volume + one table entry whose boot sector is dead: the
     * damaged target is skipped HONESTLY (counted in table_volume_skipped,
     * rc stays OK, no root emitted) - never silently nor fatally. */
    u64 s1 = 0, s2 = 0;
    auto img = testfixture::build_two_volume_mbr_disk("HEALTHY", "DEADBOOT", &s1, &s2);
    std::memset(img.data() + s2 * 512, 0, 512); /* no FS signature at all */
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    ScanConfig cfg;
    cfg.run_quick = true;
    cfg.run_carve = false;
    ScanController ctrl(tree, cfg);
    ScanStats stats;
    u32 rc = ctrl.run(&dev, nullptr, nullptr, nullptr, stats);
    ASSERT_EQ(rc, OK) << err_text(rc);
    EXPECT_EQ(stats.table_volume_targets, 2u);
    EXPECT_EQ(stats.table_volume_skipped, 1u);
    EXPECT_GE(stats.files_found, 2u) << "the healthy volume still scans";
    EXPECT_EQ(count_named(tree, "DOC.TXT"), 1u);
}

TEST(Orchestrator, WholeDiskSuperfloppyUnchanged) {
    /* no partition table (a bare FAT volume at LBA 0): the single-slice
     * dispatch keeps the volume-scan behavior - files under Found. */
    auto img = build_fat16(); /* TESTVOL, no table */
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    ScanConfig cfg;
    cfg.run_quick = true;
    cfg.run_carve = false;
    cfg.whole_disk = false;
    ScanController ctrl(tree, cfg);
    ScanStats stats;
    u32 rc = ctrl.run(&dev, nullptr, nullptr, nullptr, stats);
    ASSERT_EQ(rc, OK) << err_text(rc);
    EXPECT_EQ(stats.table_volume_targets, 0u) << "no table: no per-volume targets";
    EXPECT_GE(stats.files_found, 1u);
    /* the volume-target path did not fire: DOC.TXT sits under Found */
    bool doc_under_found = false;
    if (tree.roots().size() > 0) {
        for (tree::FileNode* c = tree.roots()[0]->first_child(); c; c = c->next_sibling())
            if (c->name_utf8() == "DOC.TXT")
                doc_under_found = true;
    }
    EXPECT_TRUE(doc_under_found);
}

TEST(Orchestrator, WholeDiskNoDoubleScan) {
    /* whole-disk mode (discovery ON): the table volumes are scanned ONCE
     * as primary targets; discovery's table-match candidates for them
     * are dropped - no duplicate "Candidate @ <LBA>" roots, and
     * lost_partitions reports the genuinely-lost set only. */
    auto img = testfixture::build_two_volume_mbr_disk("VOLONE", "VOLTWO");
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    ScanConfig cfg;
    cfg.run_quick = true;
    cfg.run_deep = true;
    cfg.run_carve = false;
    cfg.whole_disk = true;
    ScanController ctrl(tree, cfg);
    ScanStats stats;
    u32 rc = ctrl.run(&dev, nullptr, nullptr, nullptr, stats);
    ASSERT_EQ(rc, OK) << err_text(rc);
    EXPECT_EQ(stats.files_found, 4u);
    EXPECT_EQ(count_named(tree, "DOC.TXT"), 2u) << "each volume scanned exactly once";
    EXPECT_EQ(stats.lost_partitions, 0u)
        << "the table's own volumes are not lost-partition candidates anymore";
    for (const auto& n : root_names(tree))
        EXPECT_EQ(n.find("Candidate @"), std::string::npos)
            << "no anonymous candidate root for table volumes: " << n;
}

TEST(Orchestrator, WholeDiskVolumeRootsCarryDisplayNames) {
    /* M-104: the per-volume roots carry the volumes' on-disk labels
     * (VOLONE/VOLTWO from the boot sectors), not anonymous LBA strings;
     * the live and deleted files of one volume share its single root
     * (deleted as a node flag, the og's one-tree-per-volume shape). */
    auto img = testfixture::build_two_volume_mbr_disk("VOLONE", "VOLTWO");
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    ScanConfig cfg;
    cfg.run_quick = true;
    cfg.run_carve = false;
    ScanController ctrl(tree, cfg);
    ScanStats stats;
    ASSERT_EQ(ctrl.run(&dev, nullptr, nullptr, nullptr, stats), OK);

    const auto names = root_names(tree);
    ASSERT_NE(std::find(names.begin(), names.end(), "VOLONE"), names.end());
    ASSERT_NE(std::find(names.begin(), names.end(), "VOLTWO"), names.end());
    for (const auto& n : names)
        EXPECT_EQ(n.find("Candidate @"), std::string::npos) << "anonymous root: " << n;

    /* the deleted file lives under the SAME volume root, flagged deleted
     * (the FAT engine renders the 0xE5 first byte; both nodes under
     * VOLTWO's subtree with deleted_state set on the deleted one) */
    tree::FileNode* vol2 = nullptr;
    for (auto* r : tree.roots())
        if (r->name_utf8() == "VOLTWO")
            vol2 = r;
    ASSERT_NE(vol2, nullptr);
    u64 del_flagged = 0;
    for (tree::FileNode* c = vol2->first_child(); c; c = c->next_sibling())
        if (c->deleted_state() == tree::DeletedState::Deleted)
            ++del_flagged;
    EXPECT_GE(del_flagged, 1u) << "the deleted file under its volume root, flagged";
}

TEST(Orchestrator, WholeDiskKnownVsLostDistinguishable) {
    /* M-104: one in-table volume + one OUT-OF-TABLE volume (a genuinely
     * lost partition - a valid FAT boot outside the table). The known
     * volume surfaces under its label root; the lost one as a discovery
     * candidate - distinguishable by naming convention, both scannable. */
    u64 s1 = 0;
    auto img = testfixture::build_two_volume_mbr_disk("KNOWNVOL", "VOLTWO", &s1);
    /* wipe the SECOND table entry (its volume data stays on disk) -> the
     * second volume becomes a genuinely-lost partition for discovery. */
    u8* mbr = img.data();
    std::memset(mbr + 446 + 16, 0, 16); /* entry 2 gone; entry 1 stays */
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    ScanConfig cfg;
    cfg.run_quick = true;
    cfg.run_deep = true;
    cfg.run_carve = false;
    cfg.whole_disk = true;
    ScanController ctrl(tree, cfg);
    ScanStats stats;
    u32 rc = ctrl.run(&dev, nullptr, nullptr, nullptr, stats);
    ASSERT_EQ(rc, OK) << err_text(rc);
    /* the in-table volume: display-name root; the out-of-table volume:
     * exactly one discovery candidate */
    const auto names = root_names(tree);
    EXPECT_NE(std::find(names.begin(), names.end(), "KNOWNVOL"), names.end());
    EXPECT_EQ(stats.lost_partitions, 1u);
    u64 candidate_roots = 0;
    for (const auto& n : names)
        if (n.find("Candidate @") != std::string::npos)
            ++candidate_roots;
    EXPECT_EQ(candidate_roots, 1u);
    EXPECT_EQ(count_named(tree, "DOC.TXT"), 2u) << "both volumes' files found";
}

TEST(Orchestrator, WholeDiskVolumeRootNamesRoundTripSession) {
    /* M-104: the per-volume display-name roots persist through the
     * session round-trip (the og's own persistence mechanism for
     * multi-volume identity - the volume root IS the record). */
    auto img = testfixture::build_two_volume_mbr_disk("VOLONE", "VOLTWO");
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    ScanConfig cfg;
    cfg.run_quick = true;
    cfg.run_carve = false;
    ScanController ctrl(tree, cfg);
    ScanStats stats;
    ASSERT_EQ(ctrl.run(&dev, nullptr, nullptr, nullptr, stats), OK);

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
    } mf;
    {
        session::Writer w(MemFile::wr, MemFile::tl, MemFile::sk, &mf);
        ASSERT_EQ(w.open(7), OK);
        ASSERT_EQ(ctrl.save_session(w, stats), OK);
        ASSERT_EQ(w.finalize(), OK);
    }
    tree::ResultTree tree2;
    ScanStats stats2;
    session::Reader r(
        +[](void* u, void* b, u32 len) -> u32 {
            auto* m = static_cast<MemFile*>(u);
            if (m->pos + len > m->data.size())
                return 1;
            std::memcpy(b, m->data.data() + m->pos, len);
            m->pos += len;
            return 0;
        },
        MemFile::sk, +[](void* u) -> u64 { return static_cast<MemFile*>(u)->data.size(); }, &mf);
    ASSERT_EQ(r.open(nullptr), OK);
    ASSERT_EQ(ScanController::load_session(r, tree2, stats2), OK);
    /* the volume roots reload with their display names intact */
    const auto names = root_names(tree2);
    EXPECT_NE(std::find(names.begin(), names.end(), "VOLONE"), names.end());
    EXPECT_NE(std::find(names.begin(), names.end(), "VOLTWO"), names.end());
    EXPECT_EQ(count_named(tree2, "DOC.TXT"), 2u);
}

/* ---- M-018: the FilterFolder result-set junk filter ---- */

TEST(FilterFolder, CorpusListsPinned) {
    /* the shipped FilterFolder.ini sets, byte-exact from the corpus file
     * (count pins = regeneration-drift guard, the filefilter precedent) */
    EXPECT_EQ(4u, filterfolder::data::kFolderCount);
    EXPECT_EQ(7u, filterfolder::data::kFileCount);
    EXPECT_EQ(14u, filterfolder::data::kFatNameCount);
    /* every pool must terminate within its own bytes: the consumer reads
     * them as C strings, so a pool without a NUL is a global-buffer
     * overflow (the merge regression this pin guards against; under ASan
     * the strlen walks past the array and aborts) */
    EXPECT_LT(std::strlen(reinterpret_cast<const char*>(filterfolder::data::kFolderPool)), 4096u);
    EXPECT_LT(std::strlen(reinterpret_cast<const char*>(filterfolder::data::kFilePool)), 4096u);
    EXPECT_LT(std::strlen(reinterpret_cast<const char*>(filterfolder::data::kFatNamePool)), 4096u);
    /* the byte content is the og's list verbatim: the trailing byte of
     * each pool is the terminator, the one before it the delimiter */
    EXPECT_EQ(u8('\0'),
              *(filterfolder::data::kFolderPool +
                std::strlen(reinterpret_cast<const char*>(filterfolder::data::kFolderPool))));
    EXPECT_EQ(u8(';'),
              *(filterfolder::data::kFolderPool +
                std::strlen(reinterpret_cast<const char*>(filterfolder::data::kFolderPool)) - 1));
    /* FolderName: upgrade/setup scaffolding */
    EXPECT_TRUE(filterfolder::is_junk_name(u"Config.Msi", true));
    EXPECT_TRUE(filterfolder::is_junk_name(u"$Windows.~BT", true));
    EXPECT_TRUE(filterfolder::is_junk_name(u"$Windows.~WS", true));
    EXPECT_TRUE(filterfolder::is_junk_name(u"$Windows.~TMP", true));
    /* FileName: metadata noise (ASCII-fold: FAT 8.3 renders uppercase) */
    EXPECT_TRUE(filterfolder::is_junk_name(u"desktop.ini", false));
    EXPECT_TRUE(filterfolder::is_junk_name(u"DESKTOP.INI", false));
    EXPECT_TRUE(filterfolder::is_junk_name(u"._.DS_Store", false));
    EXPECT_TRUE(filterfolder::is_junk_name(u".DS_Store", false));
    EXPECT_TRUE(filterfolder::is_junk_name(u"$UsnJrnl", false));
    EXPECT_TRUE(filterfolder::is_junk_name(u"$USNJRNL", false));
    EXPECT_TRUE(filterfolder::is_junk_name(u"IndexerVolumeGuid", false));
    EXPECT_TRUE(filterfolder::is_junk_name(u"WPSettings.dat", false));
    EXPECT_TRUE(filterfolder::is_junk_name(u"WPSETTINGS.DAT", false));
    EXPECT_TRUE(filterfolder::is_junk_name(u"MountPointManagerRemoteDatabase", false));
    /* FatFilterName: the localized New Folder family + SVI + BitLocker */
    EXPECT_TRUE(filterfolder::is_junk_name(u"New Folder", true));
    EXPECT_TRUE(filterfolder::is_junk_name(u"\u65B0\u5EFA\u6587\u4EF6\u5939", true)); /* zh-CN */
    EXPECT_TRUE(filterfolder::is_junk_name(u"\u65B0\u5EFA\u8CC7\u6599\u593E", true)); /* zh-TW */
    EXPECT_TRUE(filterfolder::is_junk_name(u"\u65B0\u3057\u3044\u30D5\u30A9\u30EB\u30C0\u30FC",
                                           true));                         /* ja */
    EXPECT_TRUE(filterfolder::is_junk_name(u"\uC0C8 \uD3F4\uB354", true)); /* ko */
    EXPECT_TRUE(filterfolder::is_junk_name(u"Neuer Ordner", true));
    EXPECT_TRUE(filterfolder::is_junk_name(u"Nova pasta", true));
    EXPECT_TRUE(filterfolder::is_junk_name(u"Nuova cartella", true));
    EXPECT_TRUE(filterfolder::is_junk_name(u"Nueva carpeta", true));
    EXPECT_TRUE(filterfolder::is_junk_name(u"Nouveau dossier", true));
    EXPECT_TRUE(filterfolder::is_junk_name(u"System Volume Information", true));
    EXPECT_TRUE(filterfolder::is_junk_name(u"AadRecoveryPasswordDelete", false));
    EXPECT_TRUE(filterfolder::is_junk_name(u"ClientRecoveryPasswordRotation", false));
    EXPECT_TRUE(filterfolder::is_junk_name(u"FveDecryptedVolumeFolder", false));
    /* negatives: real files never filtered */
    EXPECT_FALSE(filterfolder::is_junk_name(u"DOC.TXT", false));
    EXPECT_FALSE(filterfolder::is_junk_name(u"resume 2026.pdf", false));
    EXPECT_FALSE(filterfolder::is_junk_name(u"Photos", true));
    EXPECT_FALSE(filterfolder::is_junk_name(u"", false));
    /* the folder list does not hit files and vice versa */
    EXPECT_FALSE(filterfolder::is_junk_name(u"Config.Msi", false));
    EXPECT_FALSE(filterfolder::is_junk_name(u"desktop.ini", true));
}

TEST(Orchestrator, JunkFilteredFromResults) {
    /* a FAT16 volume carrying the og's hidden set: DESKTOP.INI,
     * $USNJRNL (8.3), an LFN "System Volume Information" dir and an LFN
     * zh-CN New Folder dir; KEEP.ME is the negative control. The scan
     * must flag exactly the four junk nodes (stats + flags) and the
     * query surface must exclude them while the real files stay. */
    auto img = build_fat16();
    constexpr size_t ROOT = 42 * 512;
    u8* root = img.data() + ROOT;
    auto dentry = [&](size_t slot, const char* n11, u8 attr, u32 cl, u32 sz) {
        u8* e = root + slot * 32;
        std::memset(e, 0, 32);
        for (int i = 0; i < 11; ++i)
            e[i] = u8(n11[i]);
        e[11] = attr;
        put16(img, ROOT + slot * 32 + 20, u16(cl >> 16));
        put16(img, ROOT + slot * 32 + 26, u16(cl));
        put32v(img, ROOT + slot * 32 + 28, sz);
    };
    /* LFN chain writer: fragments physically DESCENDING (last first),
     * 13 UTF-16 chars per slot, checksum ties to the 8.3 alias that
     * follows (the engine's lfn_assemble contract). */
    auto lfn_chain = [&](size_t slot, const std::u16string& name, const char* alias11) {
        u8 alias[11];
        std::memcpy(alias, alias11, 11);
        u32 sum = 0;
        for (int i = 0; i < 11; ++i)
            sum = ((sum & 1) << 7) + (sum >> 1) + u32(alias[i]);
        const u8 chk = u8(sum & 0xFF);
        const size_t frags = (name.size() + 12) / 13;
        for (size_t f = frags; f >= 1; --f) {
            u8* e = root + (slot + (frags - f)) * 32;
            std::memset(e, 0xFF, 32);
            e[0] = u8(f == frags ? 0x40 | f : f);
            e[11] = 0x0F; /* LFN */
            e[12] = 0;
            e[13] = chk;
            e[26] = 0;
            e[27] = 0;
            static const size_t off[3] = {1, 14, 28};
            static const int cnt[3] = {5, 6, 2};
            size_t pos = (f - 1) * 13;
            for (int pi = 0; pi < 3; ++pi) {
                for (int ci = 0; ci < cnt[pi]; ++ci) {
                    const u16 ch =
                        pos < name.size() ? name[pos] : (pos == name.size() ? 0x0000 : 0xFFFF);
                    e[off[pi] + 2 * size_t(ci)] = u8(ch & 0xFF);
                    e[off[pi] + 2 * size_t(ci) + 1] = u8(ch >> 8);
                    ++pos;
                }
            }
        }
        return slot + frags; /* the alias entry's slot */
    };
    dentry(2, "DESKTOP INI", 0x20, 11, 100);
    dentry(3, "$USNJRNL   ", 0x20, 11, 282);
    dentry(4, "KEEP    ME ", 0x20, 11, 10);
    { /* System Volume Information (dir) */
        const size_t alias_slot = lfn_chain(5, u"System Volume Information", "SYSTEM~1   ");
        dentry(alias_slot, "SYSTEM~1   ", 0x10, 0, 0);
    }
    { /* zh-CN New Folder (dir) */
        const size_t alias_slot = lfn_chain(8, u"\u65B0\u5EFA\u6587\u4EF6\u5939", "NEWFOL~1   ");
        dentry(alias_slot, "NEWFOL~1   ", 0x10, 0, 0);
    }

    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    ScanConfig cfg;
    cfg.run_quick = true;
    cfg.run_carve = false;
    ScanController ctrl(tree, cfg);
    ScanStats stats;
    ASSERT_EQ(OK, ctrl.run(&dev, nullptr, nullptr, nullptr, stats)) << err_text(u32(0));
    EXPECT_EQ(4u, stats.junk_filtered)
        << "DESKTOP.INI + $USNJRNL + System Volume Information + the zh-CN New Folder";

    /* the flags are on exactly the junk nodes */
    std::map<std::string, bool> flagged;
    std::function<void(tree::FileNode*)> collect = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            if (c->props())
                flagged[c->name_utf8()] = (c->props()->flags & tree::NODE_FLAG_JUNK_FILTERED) != 0;
            collect(c);
        }
    };
    for (tree::FileNode* r : tree.roots())
        collect(r);
    EXPECT_TRUE(flagged.count("DESKTOP.INI") && flagged["DESKTOP.INI"]);
    EXPECT_TRUE(flagged.count("$USNJRNL") && flagged["$USNJRNL"]);
    EXPECT_TRUE(flagged.count("System Volume Information") && flagged["System Volume Information"]);
    EXPECT_TRUE(flagged.count("\u65B0\u5EFA\u6587\u4EF6\u5939") &&
                flagged["\u65B0\u5EFA\u6587\u4EF6\u5939"]);
    ASSERT_TRUE(flagged.count("DOC.TXT"));
    EXPECT_FALSE(flagged["DOC.TXT"]) << "real files are never junk-filtered";
    ASSERT_TRUE(flagged.count("KEEP.ME"));
    EXPECT_FALSE(flagged["KEEP.ME"]) << "the negative control stays visible";

    /* the query surface excludes the junk set (the og's presented tree) */
    query::Query q;
    q.limit = 1000;
    query::Result res = query::run(tree, q);
    std::set<std::string> names;
    for (const auto& row : res.rows)
        names.insert(row.name_utf8);
    EXPECT_FALSE(names.count("DESKTOP.INI"));
    EXPECT_FALSE(names.count("$USNJRNL"));
    EXPECT_FALSE(names.count("System Volume Information"));
    EXPECT_TRUE(names.count("DOC.TXT"));
    EXPECT_TRUE(names.count("KEEP.ME"));
    EXPECT_TRUE(names.count("_ELFILE.TXT"))
        << "deleted files are results too (the FAT undelete convention renders 0xE5 as _)";
}

/* ---- M-071: the file-backed junk-gate fixture ----
 *
 * tools/gen_junk_gate_fixture.py generates
 * tests/data/images/junk-gate-fat16.raw (same geometry as build_fat16()
 * above, with the SVI dir carrying an inner TRACKER.TXT so the
 * subtree semantics are observable). This test proves the SHIPPED FILE
 * scans with the same semantics the in-memory builder asserts - the
 * Windows CI junk-gate contract step (rrecov scan/tree/recover over
 * this exact file) rides a fixture the Linux suite has already proven
 * parseable, flagged and query-gated. The two-surface semantic is
 * pinned here: TRACKER.TXT is a query RESULT (the flag is per-node;
 * children of hidden dirs stay results per the og's "hides entries,
 * not contents") while the CLI tree/recover-all walks skip the whole
 * flagged subtree (a tree view cannot show children of a hidden node;
 * the M-018/M-071 gates live in rrecov.cpp's walks). */
TEST(FilterFolder, JunkGateFixtureFileScansAndFlagsHiddenSet) {
    const std::string raw = cr_test::read_repo_file("tests/data/images/junk-gate-fat16.raw");
    ASSERT_FALSE(raw.empty());
    ASSERT_EQ(size_t(5074) * 512, raw.size()) << "regenerate via tools/gen_junk_gate_fixture.py";

    MemDevice dev(std::vector<u8>(raw.begin(), raw.end()));
    tree::ResultTree tree;
    ScanConfig cfg;
    cfg.run_quick = true;
    cfg.run_carve = false;
    ScanController ctrl(tree, cfg);
    ScanStats stats;
    ASSERT_EQ(OK, ctrl.run(&dev, nullptr, nullptr, nullptr, stats)) << err_text(u32(0));

    EXPECT_EQ(4u, stats.junk_filtered)
        << "DESKTOP.INI + $USNJRNL + System Volume Information + the zh-CN New Folder";

    std::map<std::string, bool> flagged;
    std::function<void(tree::FileNode*)> collect = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            if (c->props())
                flagged[c->name_utf8()] = (c->props()->flags & tree::NODE_FLAG_JUNK_FILTERED) != 0;
            collect(c);
        }
    };
    for (tree::FileNode* r : tree.roots())
        collect(r);
    /* the hidden set, flagged on the file image exactly as in-memory */
    EXPECT_TRUE(flagged.count("DESKTOP.INI") && flagged["DESKTOP.INI"]);
    EXPECT_TRUE(flagged.count("$USNJRNL") && flagged["$USNJRNL"]);
    EXPECT_TRUE(flagged.count("System Volume Information") && flagged["System Volume Information"]);
    EXPECT_TRUE(flagged.count("\u65B0\u5EFA\u6587\u4EF6\u5939") &&
                flagged["\u65B0\u5EFA\u6587\u4EF6\u5939"]);
    /* the controls, never flagged */
    ASSERT_TRUE(flagged.count("DOC.TXT"));
    EXPECT_FALSE(flagged["DOC.TXT"]);
    ASSERT_TRUE(flagged.count("KEEP.ME"));
    EXPECT_FALSE(flagged["KEEP.ME"]);
    /* the inner file is a result (per-node flag; children stay) */
    ASSERT_TRUE(flagged.count("TRACKER.TXT"));
    EXPECT_FALSE(flagged["TRACKER.TXT"]);

    /* the query surface excludes the junk set, keeps everything else */
    query::Query q;
    q.limit = 1000;
    query::Result res = query::run(tree, q);
    std::set<std::string> names;
    for (const auto& row : res.rows)
        names.insert(row.name_utf8);
    EXPECT_FALSE(names.count("DESKTOP.INI"));
    EXPECT_FALSE(names.count("$USNJRNL"));
    EXPECT_FALSE(names.count("System Volume Information"));
    EXPECT_TRUE(names.count("DOC.TXT"));
    EXPECT_TRUE(names.count("KEEP.ME"));
    EXPECT_TRUE(names.count("TRACKER.TXT"));
}

} // namespace

/* ---- N-048: the engine logging seam ---- */

namespace {
struct LogRecorder {
    std::mutex mu;
    std::vector<std::pair<cr::log::Level, std::string>> lines;
    static void sink(cr::log::Level lvl, const char* line, void* user) {
        auto* r = static_cast<LogRecorder*>(user);
        std::lock_guard<std::mutex> lock(r->mu);
        r->lines.push_back({lvl, line});
    }
    size_t count(const char* needle) const {
        size_t n = 0;
        for (const auto& l : lines)
            if (l.second.find(needle) != std::string::npos)
                ++n;
        return n;
    }
};
} // namespace

TEST(Orchestrator, LogSilentByDefault) {
    /* N-048: the pre-fix world was SILENT - no sink registered, no line
     * formatted, zero overhead. The seam must default to that world and
     * return to it after an explicit clear. */
    cr::log::set_sink(nullptr, nullptr);
    EXPECT_FALSE(cr::log::enabled());
    /* the silent write is a no-op (nothing to observe except that it
     * returns; the enabled() fast path is the contract) */
    cr::log::write(cr::log::Level::Info, "silence %d", 42);
    EXPECT_FALSE(cr::log::enabled());
    /* a registered-then-cleared sink goes silent again */
    LogRecorder rec;
    cr::log::set_sink(&LogRecorder::sink, &rec);
    EXPECT_TRUE(cr::log::enabled());
    cr::log::set_sink(nullptr, nullptr);
    EXPECT_FALSE(cr::log::enabled());
    cr::log::write(cr::log::Level::Error, "must not arrive");
    EXPECT_EQ(rec.lines.size(), 0u) << "a cleared sink receives nothing";
}

TEST(Orchestrator, LogSinkReceivesScanBrackets) {
    /* N-048/N-061: the og's run-log channel (EPMLOG.log + the qa-logs
     * semantics: CScanMgr::doScan entry, quick/deep ranges,
     * UpdateScanFinishedInfo) maps to the core seam: scan START/END
     * brackets + stage boundaries; the retry pipe's exhaust adds the
     * og's read-error trail (PartitionRecoveryD PRD:21962). */
    auto fat = build_fat16();
    MemDevice dev(std::move(fat));
    tree::ResultTree tree;
    ScanConfig cfg;
    cfg.run_quick = true;
    cfg.run_deep = false;
    cfg.run_carve = false;
    LogRecorder rec;
    cr::log::set_sink(&LogRecorder::sink, &rec);
    ScanController ctrl(tree, cfg);
    ScanStats stats;
    u32 rc = ctrl.run(&dev, nullptr, nullptr, nullptr, stats);
    cr::log::set_sink(nullptr, nullptr); /* detach before the recorder dies */
    ASSERT_EQ(rc, OK) << err_text(rc);
    ASSERT_FALSE(rec.lines.empty());
    EXPECT_GE(rec.count("scan start: extent="), 1u) << "the START bracket";
    EXPECT_GE(rec.count("scan end: rc="), 1u) << "the END bracket with counters";
    EXPECT_GE(rec.count("stage 0 begin"), 1u) << "the stage-boundary trail";
    /* the scan-end line carries the truthful counters */
    {
        std::lock_guard<std::mutex> lock(rec.mu);
        auto it = std::find_if(rec.lines.begin(), rec.lines.end(), [](const auto& l) {
            return l.second.find("scan end: rc=") != std::string::npos;
        });
        ASSERT_NE(it, rec.lines.end());
        char buf[32];
        snprintf(buf, sizeof(buf), "files=%llu", (unsigned long long)stats.files_found);
        EXPECT_NE(it->second.find(buf), std::string::npos)
            << "the END bracket carries the files counter: " << it->second;
        snprintf(buf, sizeof(buf), "elapsed_ms=%llu", (unsigned long long)stats.elapsed_ms);
        EXPECT_NE(it->second.find(buf), std::string::npos)
            << "the END bracket carries the pause-compensated duration";
    }
}

TEST(Orchestrator, MemoryMonitorEmitsUnderPressure) {
    /* N-062 (the og's CMemMonitorManager pressure events on the same
     * sink - Log.md census): the pure threshold check emits a Warn line
     * at/above the threshold; below it and when the monitor is OFF
     * (threshold 0 - the conservative library default; the og's
     * always-on monitor is its UI process's choice) it stays silent.
     * Never coupled to scheduling (the finding's own rule - no corpus
     * evidence the og throttles; the manager feeds logs/UI). */
    LogRecorder rec;
    cr::log::set_sink(&LogRecorder::sink, &rec);
    /* a synthetic low threshold always exceeded */
    cr::log::check_memory_pressure(1u, 2u);
    cr::log::set_sink(nullptr, nullptr);
    ASSERT_EQ(rec.lines.size(), 1u);
    EXPECT_EQ(rec.lines[0].first, cr::log::Level::Warn);
    EXPECT_NE(rec.lines[0].second.find("memory pressure: working_set=2 MB threshold=1 MB"),
              std::string::npos);
    /* below the threshold: silent */
    LogRecorder rec2;
    cr::log::set_sink(&LogRecorder::sink, &rec2);
    cr::log::check_memory_pressure(100u, 2u);
    cr::log::set_sink(nullptr, nullptr);
    EXPECT_EQ(rec2.lines.size(), 0u);
    /* the monitor OFF (0): silent even at an enormous working set */
    LogRecorder rec3;
    cr::log::set_sink(&LogRecorder::sink, &rec3);
    cr::log::check_memory_pressure(0u, 1u << 20u);
    cr::log::set_sink(nullptr, nullptr);
    EXPECT_EQ(rec3.lines.size(), 0u);
}

TEST(Orchestrator, WeightedProgress) {
    StageWeights w{40, 35, 25};
    EXPECT_EQ(weighted_progress(w, 0, 0, 0), 0u);
    EXPECT_EQ(weighted_progress(w, 1000, 0, 0), 400u);
    EXPECT_EQ(weighted_progress(w, 1000, 1000, 0), 750u);
    EXPECT_EQ(weighted_progress(w, 1000, 1000, 1000), 1000u);
    EXPECT_EQ(weighted_progress(w, 500, 500, 500), 500u);
    EXPECT_EQ(weighted_progress(w, 2000, 0, 0), 400u); /* clamped */
}

TEST(Orchestrator, ProbeFamily) {
    auto img = build_fat16();
    MemDevice dev(std::move(img));
    EXPECT_EQ(ScanController::probe_family(&dev), FsFamily::Fat);
    u8* b = dev.raw().data();
    std::memcpy(b + 3, "NTFS    ", 8);
    /* M-043: the fail-before leg of this test used to EXPECT Ntfs here -
     * a spoofed OEM on a FAT volume dispatched the NTFS engine. The og's
     * analyser requires a content proof (FILE magic + USA bounds at the
     * $MFT LCN) before recognition; a FAT image's BPB 0x28 does not point
     * at an MFT record, so the proof fails and the 55AA FAT signature
     * underneath wins - exactly the og's multi-analyser outcome. */
    EXPECT_EQ(ScanController::probe_family(&dev), FsFamily::Fat);
    std::memcpy(b + 3, "EXFAT   ", 8);
    /* N-050: a spoofed EXFAT OEM on a FAT volume fails the exFAT
     * boot-region content proof (the FAT BPB's nonzero bytes at
     * 0x0B..0x3F fail the og's MustBeZero sweep) and the FAT gates +
     * root proof underneath win - exactly the og's multi-analyser
     * outcome (the engine is never dispatched on the spoofed family). */
    EXPECT_EQ(ScanController::probe_family(&dev), FsFamily::Fat);
    std::memset(b + 3, 'X', 8);
    /* odd OEM but 55AA + sane BPB still probes FAT (heuristic hint);
     * destroy the signature for Unknown */
    EXPECT_EQ(ScanController::probe_family(&dev), FsFamily::Fat);
    b[510] = 0x00;
    EXPECT_EQ(ScanController::probe_family(&dev), FsFamily::Unknown);
}

/* ---- N-050: two-stage recognition (boot gate -> content proof) ------- */

TEST(Orchestrator, SpoofedExfatOemRejectedBeforeEngine) {
    /* the finding's named proof: a volume with a spoofed family OEM but
     * no matching content never dispatches the engine. probe_family is
     * the dispatch gate (the fs_hint switch keys on it), so family !=
     * Exfat means the exFAT engine's invocation counter stays zero. */
    auto img = build_fat16();
    std::memcpy(img.data() + 3, "EXFAT   ", 8);
    MemDevice dev(std::move(img));
    EXPECT_EQ(ScanController::probe_family(&dev), FsFamily::Fat)
        << "spoofed EXFAT OEM over a FAT volume: the content proof must "
           "refuse the exFAT family (the og's analyser does)";
}

TEST(Orchestrator, SpoofedFatBootUnreadableRootRejected) {
    /* FAT half: the boot BYTES pass (55AA + numFATs + pow2 bps + sane
     * spf16) but the root directory's first sector is unreachable
     * (truncated image) - the og's root verification (FUN_180002180 ->
     * FUN_180001f70) reads the root and fails; recognition refused. */
    auto img = build_fat16();
    img.resize(42 * 512); /* the root starts at sector 42: unreachable */
    MemDevice dev(std::move(img));
    EXPECT_EQ(ScanController::probe_family(&dev), FsFamily::Unknown)
        << "a FAT boot whose root directory cannot be read is not FAT by "
           "proof (the og's analyser refuses it)";
}

TEST(Orchestrator, RealExfatBootContentVerified) {
    /* the positive pin: a structurally valid exFAT boot region (the
     * engine's own gates: OEM + 55AA + shift bounds + MustBeZero sweep
     * + FAT count/length + heap fit; wire-struct offsets per
     * cr_exfat_wire.h BootSectorMain) proves the family - the cheap
     * form of the og analyser's recognition, before any engine
     * dispatch. 2 MiB image, 512-byte sectors, 2 sectors/cluster. */
    const u64 VOL_SECTORS = 4096;
    std::vector<u8> img(size_t(VOL_SECTORS) * 512, 0);
    u8* b = img.data();
    b[0] = 0xEB;
    b[1] = 0x76;
    b[2] = 0x90;
    std::memcpy(b + 3, "EXFAT   ", 8);
    /* volume_length u64 @0x48 */
    for (int i = 0; i < 8; ++i)
        b[0x48 + i] = u8(VOL_SECTORS >> (8 * i));
    /* fat_offset u32 @0x50 (>= 24: past the boot region) */
    b[0x50] = 24;
    /* fat_length u32 @0x54 */
    b[0x54] = 8;
    /* cluster_heap_offset u32 @0x58 (>= fat_offset+fat_length) */
    b[0x58] = 40;
    /* cluster_count u32 @0x5C (heap fit: (n-1)*2+40 <= 4096) */
    b[0x5C] = 0xE4; /* 2020 */
    b[0x5D] = 0x07;
    /* first_cluster_of_root u32 @0x60 (>= 2, < count+2) */
    b[0x60] = 2;
    /* bytes_per_sector_shift u8 @0x6C = 9 (512) */
    b[0x6C] = 9;
    /* sectors_per_cluster_shift u8 @0x6D = 1 */
    b[0x6D] = 1;
    /* fat_count u8 @0x6E = 1 */
    b[0x6E] = 1;
    b[510] = 0x55;
    b[511] = 0xAA;
    MemDevice dev(std::move(img));
    EXPECT_EQ(ScanController::probe_family(&dev), FsFamily::Exfat)
        << "a structurally valid exFAT boot region proves the family";
}

/* ---- M-044: UDF recognition (the og's analyser result 0xc) -----------
 * The probe is decompile-exact (FATFileSystemAnalyser_decompiled.c
 * :1413-1435): boot's first 3 bytes NOT all nonzero -> anchor at sector
 * 0x100 -> LE-u32@anchor+0x14 (the main VDS location) -> "UDF" at
 * [0x19..0x1b] of that sector. The og has no UDF structured engine -
 * classification only; content rides the carve family. */
static void udf_write_anchor(std::vector<u8>& img, u32 vds_sector) {
    u8* a = img.data() + 0x100 * 512;
    std::memset(a, 0, 512);
    a[0] = 0x02;
    a[1] = 0x00; /* tag: AVDP (LE-dialect form, as the og loads it) */
    /* main VDS location at +0x14, little-endian (the og's compiled load) */
    a[0x14] = u8(vds_sector);
    a[0x15] = u8(vds_sector >> 8);
    a[0x16] = u8(vds_sector >> 16);
    a[0x17] = u8(vds_sector >> 24);
}

TEST(Orchestrator, FsFamilyValuesMatchContract) {
    /* N-076 pin: the documented wire contract (FB-L03's held slot, the
     * cr_ui ABI hint cases, S-019's "BitLocker = 10", M-044's Udf = 11).
     * The pre-fix enum failed Hfsplus/BitLocker (5 and 9). */
    EXPECT_EQ(u32(FsFamily::Unknown), 0u);
    EXPECT_EQ(u32(FsFamily::Ntfs), 1u);
    EXPECT_EQ(u32(FsFamily::Fat), 2u);
    EXPECT_EQ(u32(FsFamily::Exfat), 3u);
    EXPECT_EQ(u32(FsFamily::Ext4), 4u);
    EXPECT_EQ(u32(FsFamily::Hfsplus), 6u) << "FB-L03: 5 stays reserved";
    EXPECT_EQ(u32(FsFamily::Btrfs), 7u);
    EXPECT_EQ(u32(FsFamily::Zfs), 8u);
    EXPECT_EQ(u32(FsFamily::Refs), 9u);
    EXPECT_EQ(u32(FsFamily::BitLocker), 10u) << "S-019's documented value";
    EXPECT_EQ(u32(FsFamily::Udf), 11u) << "M-044";
}

TEST(Orchestrator, UdfProbeFailsOnMkudffsShapeLikeTheOg) {
    /* parity pin (the honest one): a fresh mkudffs (exfatprogs-era
     * udftools, Debian) image has a blank boot, its anchor's LE-loaded
     * main-VDS location lands on the real VDS (sector 96 on the 8 MiB
     * reference layout), and the VDS's first descriptor is a PVD whose
     * [0x19..0x1b] reads "Lin" (the "LinuxUDF" implementation string) -
     * the og's exact algorithm therefore does NOT classify it as UDF and
     * the volume rides the RAW/carve family. Our probe matches the og
     * byte-for-byte: not UDF here either. (Probed first-hand on a real
     * mkudffs 2.x image this session; the shape is reproduced in-memory.) */
    std::vector<u8> img(1024 * 1024, 0);
    udf_write_anchor(img, 96);
    /* the mkudffs PVD: tag=1, then the implementation bytes - 08 at 0x18,
     * "LinuxUDF" at 0x19 */
    u8* pvd = img.data() + 96 * 512;
    pvd[0] = 0x01;
    pvd[0x18] = 0x08;
    std::memcpy(pvd + 0x19, "LinuxUDF", 8);
    MemDevice dev(std::move(img));
    EXPECT_EQ(ScanController::probe_family(&dev), FsFamily::Unknown)
        << "the og's probe reads 'Lin' here - not UDF; parity means the "
           "same answer, the volume carves";
}

/* ---- M-044: UDF volume recognition (the og's analyser result 0xc) ----
 *
 * Minimal ECMA-167 recognition chain, og-arithmetic-exact (the probe
 * decoded at FATFileSystemAnalyser_decompiled.c FUN_180002580:1413-1435):
 *  - LBA 0: the reserved area (first three bytes zero - the og's
 *    fast-reject gate: all-nonzero boot bytes = jump code -> never UDF);
 *  - LBA 0x100: AnchorVolumeDescriptorPointer; the Main Volume
 *    Descriptor Sequence extent location sits at +0x14 (ECMA-167 AVDP
 *    layout: 16-byte DescriptorTag, then ExtentDescriptor pairs);
 *  - the Main VDS start descriptor (Primary Volume Descriptor shape)
 *    carrying "UDF" at [0x19..0x1b] - a CS0 dstring volume identifier
 *    "12345678UDF" (len byte at +0x10, chars from +0x11) lands exactly
 *    there, the position the og's three byte-compares read. */
std::vector<u8> build_udf(size_t vds_lba = 0x101) {
    std::vector<u8> img((vds_lba + 2) * 512, 0);
    put32v(img, 0x100 * 512 + 0x14, u32(vds_lba)); /* AVDP main VDS location */
    u8* pvd = img.data() + vds_lba * 512;
    pvd[0] = 1;                                 /* DescriptorTag.TagIdentifier = 1 (PVD) */
    pvd[1] = 1;                                 /* DescriptorVersion */
    pvd[0x10] = 0x0B;                           /* VolumeIdentifier dstring: 11 chars */
    std::memcpy(pvd + 0x11, "12345678UDF", 11); /* "UDF" at +0x19..0x1b */
    return img;
}

TEST(Orchestrator, ProbesUdfVolume) {
    MemDevice dev(build_udf());
    EXPECT_EQ(ScanController::probe_family(&dev), FsFamily::Udf);
    /* the descriptor chain may live anywhere the AVDP points */
    MemDevice dev2(build_udf(0x140));
    EXPECT_EQ(ScanController::probe_family(&dev2), FsFamily::Udf);
}

TEST(Orchestrator, UdfProbeRequiresBothDescriptorReads) {
    /* truncated device: the AVDP at +0x100 sectors is unreachable ->
     * the og's second read fails -> the fail path, never UDF (and no
     * crash: reads are bounded by the device contract) */
    MemDevice dev(std::vector<u8>(0x100 * 512, 0));
    EXPECT_EQ(ScanController::probe_family(&dev), FsFamily::Unknown);
    /* AVDP present but the VDS descriptor does not carry "UDF" at
     * [0x19..0x1b] -> Unknown (the og's three byte-compares fail) */
    auto img = build_udf();
    std::memcpy(img.data() + 0x101 * 512 + 0x11, "12345678XYZ", 11);
    MemDevice dev2(std::move(img));
    EXPECT_EQ(ScanController::probe_family(&dev2), FsFamily::Unknown);
}

TEST(Orchestrator, UdfNotConfusedWithFat) {
    /* a boot sector with jump code (all-nonzero first three bytes) skips
     * the UDF probe entirely (the og's ||-short-circuit) even when a
     * complete UDF descriptor chain sits behind it - and the 55AA+BPB
     * gate then classifies FAT */
    auto img = build_udf();
    u8* b = img.data();
    b[0] = 0xEB;
    b[1] = 0x3C;
    b[2] = 0x90;                       /* FAT jump code */
    std::memcpy(b + 3, "MSDOS5.0", 8); /* OEM */
    put16(img, 0x0B, 512);             /* pow2 bps */
    b[0x10] = 2;                       /* numFATs (M-045's gate) */
    b[510] = 0x55;
    b[511] = 0xAA;
    MemDevice dev(std::move(img));
    /* N-050: the painted FAT boot passes the byte gates but the root
     * proof refuses it (no valid FAT geometry: spc == 0 and the 0x2C
     * field is UDF descriptor bytes, not a root cluster >= 2) - the
     * og's analyser fails the same root verification, so the volume is
     * neither UDF (jump code skips the probe) nor FAT: Unknown. */
    EXPECT_EQ(ScanController::probe_family(&dev), FsFamily::Unknown);
}

TEST(Orchestrator, UdfSurfacesAsCarveFamily) {
    /* a UDF volume has no structured engine on either side (no UDF
     * SearchDLL exists in the og's 9-engine set); quick/deep yield
     * nothing and carve recovers content by signature, with the family
     * recorded as Udf */
    auto img = build_udf();
    img.resize(0x400 * 512, 0);
    /* plant a JPEG in the untracked space behind the descriptors */
    {
        u8* at = img.data() + 0x200 * 512;
        at[0] = 0xFF;
        at[1] = 0xD8; /* SOI */
        at[2] = 0xFF;
        at[3] = 0xE0; /* APP0 */
        at[4] = 0;
        at[5] = 16;
        std::memcpy(at + 6, "JFIF", 4);
        at[10] = 1;
        at[11] = 1;
        at[14] = 1;
        at[15] = 0;
        at[16] = 0;
        at[17] = 0; /* APP0 done: 2+2+16 */
        size_t p = 20;
        at[p] = 0xFF;
        at[p + 1] = 0xFE; /* COM */
        at[p + 2] = 0x01;
        at[p + 3] = 0xD2; /* 466: big payload (grammar floor) */
        for (int i = 0; i < 462; ++i)
            at[p + 4 + i] = u8('A' + (i % 26));
        p += 2 + 466;
        at[p] = 0xFF;
        at[p + 1] = 0xC0; /* SOF0 */
        at[p + 2] = 0;
        at[p + 3] = 17;
        at[p + 4] = 8;
        at[p + 5] = 0;
        at[p + 6] = 64;
        at[p + 7] = 0;
        at[p + 8] = 48;
        at[p + 9] = 1;
        at[p + 10] = 1;
        at[p + 11] = 0x22;
        at[p + 12] = 0;
        p += 2 + 17;
        at[p] = 0xFF;
        at[p + 1] = 0xDA; /* SOS */
        at[p + 2] = 0;
        at[p + 3] = 12;
        for (int i = 4; i < 14; ++i)
            at[p + i] = 0;
        p += 2 + 12;
        at[p] = 0x00;
        at[p + 1] = 0x11;
        at[p + 2] = 0x22;
        at[p + 3] = 0x33;
        at[p + 4] = 0xFF;
        at[p + 5] = 0xD9; /* EOI */
    }
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    ScanConfig cfg;
    cfg.run_quick = true;
    cfg.run_deep = true;
    cfg.run_carve = true;
    cfg.whole_disk = false;
    ScanController ctrl(tree, cfg);
    ScanStats stats;
    const u32 rc = ctrl.run(&dev, nullptr, nullptr, nullptr, stats);
    ASSERT_EQ(rc, OK) << err_text(rc);
    /* quick/deep produced nothing structured (files_found counts engine
     * emissions only); the JPEG came from the carve stage */
    EXPECT_EQ(stats.files_found, 0u);
    EXPECT_GE(stats.carved_found, 1u);
    /* the family the auto-probe picked is recorded as Udf on the session
     * VolumeInfo (fs_kind carries FsFamily on the wire - D-1 truth) */
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
    } mf;
    {
        session::Writer w(MemFile::wr, MemFile::tl, MemFile::sk, &mf);
        ASSERT_EQ(w.open(7), OK);
        ASSERT_EQ(ctrl.save_session(w, stats), OK);
        ASSERT_EQ(w.finalize(), OK);
    }
    session::Reader r(
        +[](void* u, void* b, u32 len) -> u32 {
            auto* m = static_cast<MemFile*>(u);
            if (m->pos + len > m->data.size())
                return 1;
            std::memcpy(b, m->data.data() + m->pos, len);
            m->pos += len;
            return 0;
        },
        MemFile::sk, +[](void* u) -> u64 { return static_cast<MemFile*>(u)->data.size(); }, &mf);
    ASSERT_EQ(r.open(nullptr), OK);
    bool saw_vi = false;
    for (u32 i = 0; i < r.record_count(); ++i) {
        session::Record rec;
        ASSERT_EQ(r.read_record(i, rec), OK);
        if (rec.type != session::REC_VOLUME_INFO)
            continue;
        session::VolumeInfo vi;
        ASSERT_EQ(session::deserialize_volume_info(rec.data.data(), rec.data.size(), vi), OK);
        EXPECT_EQ(vi.fs_kind, u32(FsFamily::Udf));
        saw_vi = true;
    }
    EXPECT_TRUE(saw_vi);
}

TEST(Orchestrator, CarveSingleNamingFamily) {
    /* N-014b: one scan emits ONE carved-name family. The grammar layer
     * and the raw-sign layer both name through the registered chain
     * (og per-analyser embedded_name, then the File%06u default); the
     * old orchestrator-side "Carved%06u" lambda produced a second family
     * inside the same scan. */
    auto img = build_fat16();
    /* plant INSIDE the volume's free clusters (carving runs on the
     * volume slice, free-map gated): clusters 100 and 300 are untouched
     * (DATA starts at sector 74; cluster N lives at DATA+(N-2)*512) */
    const size_t DATA = 74 * 512;
    {
        u8* at = img.data() + DATA + (100 - 2) * 512;
        at[0] = 0xFF;
        at[1] = 0xD8;
        at[2] = 0xFF;
        at[3] = 0xE0;
        at[4] = 0;
        at[5] = 16;
        std::memcpy(at + 6, "JFIF", 4);
        at[10] = 1;
        at[11] = 1;
        at[14] = 1;
        at[15] = 0;
        at[16] = 0;
        at[17] = 0;
        size_t p = 20;
        at[p] = 0xFF;
        at[p + 1] = 0xFE;
        at[p + 2] = 0x01;
        at[p + 3] = 0xD2; /* 466: big payload (grammar floor) */
        for (int i = 0; i < 462; ++i)
            at[p + 4 + i] = u8('A' + (i % 26));
        p += 2 + 466;
        at[p] = 0xFF;
        at[p + 1] = 0xC0;
        at[p + 2] = 0;
        at[p + 3] = 17;
        at[p + 4] = 8;
        at[p + 5] = 0;
        at[p + 6] = 64;
        at[p + 7] = 0;
        at[p + 8] = 48;
        at[p + 9] = 1;
        at[p + 10] = 1;
        at[p + 11] = 0x22;
        p += 2 + 17;
        at[p] = 0xFF;
        at[p + 1] = 0xDA; /* SOS */
        at[p + 2] = 0;
        at[p + 3] = 8;
        for (int i = 0; i < 8; ++i)
            at[p + 4 + i] = 0x11;
        p += 2 + 12;
        at[p] = 0x00;
        at[p + 1] = 0x11;
        at[p + 2] = 0x22;
        at[p + 3] = 0x33;
        at[p + 4] = 0xFF;
        at[p + 5] = 0xD9; /* EOI */
    }
    /* raw-sign-layer candidate: a PSD in another free cluster */
    {
        u8* at = img.data() + DATA + (300 - 2) * 512;
        std::memcpy(at, "8BPS", 4);
        at[4] = 0;
        at[5] = 1; /* version BE */
        at[12] = 0;
        at[13] = 1; /* channels BE */
        at[14] = 0;
        at[15] = 0;
        at[16] = 0;
        at[17] = 64; /* height BE */
        at[18] = 0;
        at[19] = 0;
        at[20] = 0;
        at[21] = 48; /* width BE */
        at[22] = 0;
        at[23] = 8; /* depth BE */
        at[24] = 0;
        at[25] = 1; /* color mode */
    }
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    ScanConfig cfg;
    cfg.run_quick = true;
    cfg.run_deep = false;
    cfg.run_carve = true;
    cfg.whole_disk = false;
    ScanController ctrl(tree, cfg);
    ScanStats stats;
    ASSERT_EQ(ctrl.run(&dev, nullptr, nullptr, nullptr, stats), OK) << "scan ran";
    ASSERT_GE(stats.carved_found, 2u) << "both the grammar and raw-sign candidates carved";

    /* every carved node must carry the single File%06u family */
    std::vector<std::string> carved_names;
    std::function<void(tree::FileNode*)> collect = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            if (c->origin() == tree::Origin::Carved)
                carved_names.push_back(c->name_utf8());
            collect(c);
        }
    };
    for (tree::FileNode* r : tree.roots())
        collect(r);
    ASSERT_GE(carved_names.size(), size_t(2));
    bool saw_jpg = false, saw_psd = false;
    for (const std::string& n : carved_names) {
        EXPECT_EQ(std::string::npos, n.find("Carved")) << "the split family is gone: " << n;
        /* the registered default: FileNNNNNN.ext (6 digits) */
        ASSERT_GE(n.size(), size_t(10)) << n;
        EXPECT_EQ(0u, n.compare(0, 4, "File")) << n;
        for (int i = 4; i < 10; ++i)
            EXPECT_TRUE(n[i] >= '0' && n[i] <= '9') << "index digits: " << n;
        if (n.size() > 10 && n.compare(n.size() - 4, 4, ".jpg") == 0)
            saw_jpg = true;
        if (n.size() > 10 && n.compare(n.size() - 4, 4, ".psd") == 0)
            saw_psd = true;
    }
    EXPECT_TRUE(saw_jpg) << "the grammar-layer candidate is in the family";
    EXPECT_TRUE(saw_psd) << "the raw-sign-layer candidate is in the family";
}

TEST(Orchestrator, StagedRunAndSessionRoundTrip) {
    auto img = build_fat16();
    MemDevice dev(std::move(img));

    tree::ResultTree tree;
    ScanConfig cfg;
    cfg.run_quick = true;
    cfg.run_deep = true;
    cfg.run_carve = false; /* carving covered by its own suite */
    cfg.whole_disk = false;
    ScanController ctrl(tree, cfg);
    ScanStats stats;
    u32 rc = ctrl.run(&dev, nullptr, nullptr, nullptr, stats);
    ASSERT_EQ(rc, OK) << err_text(rc);
    EXPECT_GE(stats.files_found, 1u);   /* DOC.TXT */
    EXPECT_GE(stats.deleted_found, 1u); /* DELFILE.TXT (deleted entry) */

    /* node exists with extents + content readable through the slice */
    tree::FileNode* doc = nullptr;
    std::function<void(tree::FileNode*)> collect = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            if (c->name_utf8() == "DOC.TXT")
                doc = c;
            collect(c);
        }
    };
    for (auto* r : tree.roots())
        collect(r);
    ASSERT_NE(doc, nullptr);

    /* session round-trip */
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
        ASSERT_EQ(w.open(7), OK);
        ASSERT_EQ(ctrl.save_session(w, stats), OK);
        ASSERT_EQ(w.finalize(), OK);
    }
    tree::ResultTree tree2;
    ScanStats stats2;
    session::Reader r(MemFile::rd, MemFile::sk, MemFile::sz, &mf);
    ASSERT_EQ(ScanController::load_session(r, tree2, stats2), OK);
    EXPECT_GE(stats2.files_found, stats.files_found);
    /* restored tree contains DOC.TXT again */
    tree::FileNode* doc2 = nullptr;
    std::function<void(tree::FileNode*)> collect2 = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            if (c->name_utf8() == "DOC.TXT")
                doc2 = c;
            collect2(c);
        }
    };
    for (auto* r2 : tree2.roots())
        collect2(r2);
    ASSERT_NE(doc2, nullptr);
    EXPECT_EQ(doc2->props()->logical_size, doc->props()->logical_size);
}

TEST(SafeWriter, OverlapRefusal) {
    auto img = build_fat16();
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    ScanConfig cfg;
    cfg.run_carve = false;
    ScanController ctrl(tree, cfg);
    ScanStats stats;
    ASSERT_EQ(ctrl.run(&dev, nullptr, nullptr, nullptr, stats), OK);
    tree::FileNode* doc = nullptr;
    std::function<void(tree::FileNode*)> collect = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            if (c->name_utf8() == "DOC.TXT")
                doc = c;
            collect(c);
        }
    };
    for (auto* r : tree.roots())
        collect(r);
    ASSERT_NE(doc, nullptr);

    MockIo io;
    SafeWriter writer(io, &dev, WriterConfig{});
    /* destination volume occupying the SAME disk region as the source
     * file's extents -> hard refusal (F12) */
    writer.set_destination(0, dev.size());
    u32 rc = writer.enqueue(doc, u16_("R:\\out"));
    EXPECT_EQ(rc, ERR_DEST_OVERLAP);
    EXPECT_EQ(writer.queue().size(), 1u);
    EXPECT_EQ(writer.queue()[0].state, ItemState::Refused);

    /* disjoint destination: accepted */
    SafeWriter writer2(io, &dev, WriterConfig{});
    writer2.set_destination(dev.size(), dev.size()); /* past the disk: disjoint */
    EXPECT_EQ(writer2.enqueue(doc, u16_("R:\\out")), OK);
    WriterStats ws;
    ASSERT_EQ(writer2.process(nullptr, nullptr, nullptr, ws), OK);
    EXPECT_EQ(ws.done, 1u);
    EXPECT_EQ(writer2.queue().size(), 1u);
    EXPECT_EQ(writer2.queue()[0].state, ItemState::Done);
    /* content marker survived the copy: cluster 10 = DATA+(10-2)*512 */
    std::string got = io.content_of(writer2.queue()[0].item_id);
    ASSERT_GE(got.size(), 7u);
    EXPECT_EQ(got.substr(0, 7), "DOCDATA");
}

/* M-071 design pin: the junk gate lives in the RESULT-SURFACE walks
 * (query passes(); rrecov.cpp's CLI tree print + recover-all plan walk),
 * NOT in SafeWriter::enqueue - an explicit selection (the flat-C API
 * consumer's equivalent of checking the box for a hidden file) still
 * recovers a junk-flagged node. This is the documented M-018 contract;
 * pinning it here keeps a future "defense-in-depth" enqueue gate from
 * silently breaking explicit selection. */
TEST(SafeWriter, ExplicitSelectionStillRecoversJunkFlaggedNode) {
    const std::string raw = cr_test::read_repo_file("tests/data/images/junk-gate-fat16.raw");
    ASSERT_FALSE(raw.empty());
    MemDevice dev(std::vector<u8>(raw.begin(), raw.end()));
    tree::ResultTree tree;
    ScanConfig cfg;
    cfg.run_carve = false;
    ScanController ctrl(tree, cfg);
    ScanStats stats;
    ASSERT_EQ(ctrl.run(&dev, nullptr, nullptr, nullptr, stats), OK);
    EXPECT_EQ(stats.junk_filtered, 4u);

    tree::FileNode* junk = nullptr;
    std::function<void(tree::FileNode*)> collect = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            if (c->name_utf8() == "DESKTOP.INI")
                junk = c;
            collect(c);
        }
    };
    for (auto* r : tree.roots())
        collect(r);
    ASSERT_NE(junk, nullptr);
    ASSERT_NE(junk->props(), nullptr);
    ASSERT_NE(junk->props()->flags & tree::NODE_FLAG_JUNK_FILTERED, 0u)
        << "fixture precondition: DESKTOP.INI must arrive flagged";

    MockIo io;
    SafeWriter writer(io, &dev, WriterConfig{});
    writer.set_destination(dev.size(), dev.size()); /* disjoint destination */
    EXPECT_EQ(writer.enqueue(junk, u16_("R:\\out")), OK);
    WriterStats ws;
    ASSERT_EQ(writer.process(nullptr, nullptr, nullptr, ws), OK);
    EXPECT_EQ(ws.done, 1u);
    EXPECT_EQ(writer.queue().size(), 1u);
    EXPECT_EQ(writer.queue()[0].state, ItemState::Done);
    std::string got = io.content_of(writer.queue()[0].item_id);
    ASSERT_GE(got.size(), 15u);
    EXPECT_EQ(got.substr(0, 15), "DESKTOPINI-JUNK");
}

TEST(SafeWriter, CollisionAndSanitize) {
    MockIo io;
    std::vector<u8> img(64 * 512, 0);
    MemDevice dev(std::move(img));
    SafeWriter writer(io, &dev, WriterConfig{});

    io.existing.insert(u16_("R:\\out\\photo.jpg"));
    std::u16string n1 = writer.resolve_collision(u16_("R:\\out"), u16_("photo.jpg"));
    EXPECT_EQ(n1, u16_("photo_1.jpg"));

    /* sanitize: forbidden chars replaced (corpus rule) */
    EXPECT_EQ(SafeWriter::sanitize_name(u16_("a<b>:?/|*.txt")), u16_("a_b______.txt"));
    EXPECT_EQ(SafeWriter::sanitize_name(u16_("..")), u16_("_.."));
    EXPECT_EQ(SafeWriter::sanitize_name(u16_("")), u16_("_"));
}

TEST(SafeWriter, ResidentPrefixNotOverwritten) {
    /* Regression: the writer used to restart streaming at offset 0 after
     * emitting the resident prefix, zero/extent-overwriting the payload.
     * Files: resident 16-byte prefix + a 496-byte extent carrying the rest,
     * and a resident-only file padded by logical_size. */
    MockIo io;
    std::vector<u8> img(64 * 512, 0);
    /* extent content for the mixed file: post-resident bytes */
    std::memcpy(img.data() + 512 * 20, "EXTENTDATA", 10);
    MemDevice dev(std::move(img));
    SafeWriter writer(io, &dev, WriterConfig{});
    writer.set_destination(dev.size(), dev.size()); /* disjoint from source */

    tree::ResultTree tree;
    auto* ra = tree.add_root(u"R");

    /* mixed: 16B resident + extent 496B */
    {
        auto* pr = new tree::FileProperties();
        pr->name = u16_("mixed.bin");
        pr->logical_size = 512;
        const char* payload = "RESIDENT-PAYLOAD"; /* exactly 16 chars */
        pr->resident_data.assign(payload, payload + 16);
        tree::FileProperties::Extent ex;
        ex.offset = 512 * 20;
        ex.length = 496;
        pr->extents.push_back(ex);
        tree::FileNode* n = tree.add_file(ra, pr);
        pr->release();
        ASSERT_NE(nullptr, n);
        ASSERT_EQ(writer.enqueue(n, u16_("R:\\out")), OK);
    }
    /* resident-only: 8B payload, logical size 32 (zero-padded tail) */
    {
        auto* pr = new tree::FileProperties();
        pr->name = u16_("tiny.bin");
        pr->logical_size = 32;
        const char* payload = "TINYPAYL"; /* exactly 8 chars */
        pr->resident_data.assign(payload, payload + 8);
        tree::FileNode* n = tree.add_file(ra, pr);
        pr->release();
        ASSERT_NE(nullptr, n);
        ASSERT_EQ(writer.enqueue(n, u16_("R:\\out")), OK);
    }
    WriterStats ws;
    ASSERT_EQ(writer.process(nullptr, nullptr, nullptr, ws), OK);
    ASSERT_EQ(2u, ws.done);

    for (const auto& item : writer.queue()) {
        const std::string got = io.content_of(item.item_id);
        if (item.final_name == u16_("R:\\out\\mixed.bin")) {
            ASSERT_GE(got.size(), 512u);
            EXPECT_EQ(0, std::memcmp("RESIDENT-PAYLOAD", got.data(), 16))
                << "resident prefix was overwritten";
            EXPECT_EQ(0, std::memcmp("EXTENTDATA", got.data() + 16, 10))
                << "extent content must follow the resident prefix";
        } else if (item.final_name == u16_("R:\\out\\tiny.bin")) {
            ASSERT_GE(got.size(), 32u);
            EXPECT_EQ(0, std::memcmp("TINYPAYL", got.data(), 8));
            for (size_t i = 8; i < 32; ++i)
                EXPECT_EQ(0, got[i]) << "tail byte " << i << " must be zero-padded";
        }
    }
}

/* ---- S-008: pre-scan repair wiring (consent-gated, corpus RawFixer
 * position: repair, then scan) ---- */
#include "cr/repair/cr_rawfixer.h"

namespace {

/* Write view over the SAME memory the scan reads (the platform layer's RW
 * handle stand-in for tests). */
class SharedRamWrite final : public repair::IWriteDevice {
public:
    explicit SharedRamWrite(MemDevice& dev) : d_(dev.raw()) {}
    u32 read_at(u64 off, void* buf, u32 len) override {
        if (off > d_.size() || len > d_.size() - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        std::memcpy(buf, d_.data() + off, len);
        return OK;
    }
    u32 write_at(u64 off, const void* buf, u32 len) override {
        if (off > d_.size() || len > d_.size() - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        std::memcpy(d_.data() + off, buf, len);
        ++writes;
        return OK;
    }
    u64 size() const override { return d_.size(); }
    u32 writes = 0;

private:
    std::vector<u8>& d_;
};

struct RepairCtx {
    bool consent = false;
    int consent_calls = 0;
    SharedRamWrite* dev = nullptr;
    std::vector<std::pair<u32, u64>> events; /* (outcome, actions) */
};

bool test_consent(const repair::RepairPlan& plan, void* user) {
    auto* ctx = static_cast<RepairCtx*>(user);
    ++ctx->consent_calls;
    EXPECT_FALSE(plan.refused);
    EXPECT_FALSE(plan.actions.empty());
    return ctx->consent;
}
repair::IWriteDevice* test_open_writer(void* user) {
    return static_cast<RepairCtx*>(user)->dev;
}
void test_close_writer(repair::IWriteDevice* dev, void*) {
    (void)dev;
}
void test_event_fn(const event::Event& evt, void* user) {
    if (evt.type == event::Type::Repair)
        static_cast<RepairCtx*>(user)->events.emplace_back(evt.repair_outcome, evt.repair_actions);
}

/* build_fat16 + the FAT16 entry signature on both copies, FAT1 sector 0 torn */
struct TornFatFixture {
    std::vector<u8> img;
    static constexpr u64 FAT1 = 2 * 512, FAT2 = 22 * 512;
    TornFatFixture() : img(build_fat16()) {
        for (u64 fat : {FAT1, FAT2}) {
            img[fat + 0] = 0xF8;
            img[fat + 1] = 0xFF;
            img[fat + 2] = 0xFF;
            img[fat + 3] = 0xFF;
        }
        std::memset(img.data() + FAT1, 0xAB, 512); /* torn signature sector */
    }
};

TEST(Orchestrator, PreScanRepairRestoresTornFatAndScans) {
    /* finding claim (2) end-to-end: a FAT16 volume with a torn FAT1
     * signature sector is repaired pre-scan after consent; the engines
     * observe the repaired bytes and the structured scan completes. */
    TornFatFixture fx;
    MemDevice dev(std::move(fx.img));
    SharedRamWrite wdev(dev);
    RepairCtx ctx;
    ctx.consent = true;
    ctx.dev = &wdev;

    tree::ResultTree tree;
    ScanConfig cfg;
    cfg.run_quick = true;
    cfg.run_deep = false;
    cfg.run_carve = false;
    cfg.pre_scan_repair = true;
    cfg.repair.consent = test_consent;
    cfg.repair.open_writer = test_open_writer;
    cfg.repair.close_writer = test_close_writer;
    cfg.repair.user = &ctx;
    cfg.event_fn = test_event_fn;
    cfg.event_user = &ctx;
    ScanController ctrl(tree, cfg);
    ScanStats stats;
    u32 rc = ctrl.run(&dev, nullptr, nullptr, nullptr, stats);
    ASSERT_EQ(rc, OK) << err_text(rc);

    /* the repair executed exactly one transactional action */
    EXPECT_EQ(1u, ctx.consent_calls);
    EXPECT_TRUE(stats.repair_ran);
    EXPECT_EQ("fat", std::string(stats.repair_family));
    EXPECT_EQ(1u, stats.repair_actions_planned);
    EXPECT_EQ(1u, stats.repair_actions_applied);
    EXPECT_FALSE(stats.repair_consent_denied);
    EXPECT_EQ(1u, wdev.writes);
    ASSERT_EQ(1u, stats.repair_undo.entries.size());
    /* FAT1's signature sector restored from the sibling copy */
    EXPECT_EQ(0xF8, dev.raw()[TornFatFixture::FAT1 + 0]);
    EXPECT_EQ(0xFF, dev.raw()[TornFatFixture::FAT1 + 1]);
    EXPECT_EQ(0, std::memcmp(dev.raw().data() + TornFatFixture::FAT1,
                             dev.raw().data() + TornFatFixture::FAT2, 512));
    /* the structured scan proceeded on the repaired volume */
    EXPECT_GE(stats.files_found, 1u);
    /* event stream: ready-for-consent then applied */
    ASSERT_EQ(2u, ctx.events.size());
    EXPECT_EQ(0u, ctx.events[0].first);
    EXPECT_EQ(1u, ctx.events[0].second);
    EXPECT_EQ(1u, ctx.events[1].first);
    EXPECT_EQ(1u, ctx.events[1].second);
}

TEST(Orchestrator, PreScanRepairDeniedNeverWrites) {
    TornFatFixture fx;
    MemDevice dev(std::move(fx.img));
    SharedRamWrite wdev(dev);
    RepairCtx ctx;
    ctx.consent = false;
    ctx.dev = &wdev;

    tree::ResultTree tree;
    ScanConfig cfg;
    cfg.run_quick = true;
    cfg.run_deep = false;
    cfg.run_carve = false;
    cfg.pre_scan_repair = true;
    cfg.repair.consent = test_consent;
    cfg.repair.open_writer = test_open_writer;
    cfg.repair.close_writer = test_close_writer;
    cfg.repair.user = &ctx;
    ScanController ctrl(tree, cfg);
    ScanStats stats;
    u32 rc = ctrl.run(&dev, nullptr, nullptr, nullptr, stats);
    ASSERT_EQ(rc, OK) << err_text(rc);
    /* consent asked and denied: nothing written, the volume is untouched,
     * and the scan still completes (falls through unrepaired) */
    EXPECT_EQ(1u, ctx.consent_calls);
    EXPECT_EQ(0u, wdev.writes);
    EXPECT_EQ(0u, stats.repair_actions_applied);
    EXPECT_TRUE(stats.repair_consent_denied);
    EXPECT_EQ(0xAB, dev.raw()[TornFatFixture::FAT1 + 0]);
    EXPECT_GE(stats.files_found, 0u);
}

TEST(Orchestrator, PreScanRepairWithoutHooksNeverWrites) {
    /* S1: no consent hook at all = the plan is surfaced in stats but can
     * never execute (default-deny) */
    TornFatFixture fx;
    MemDevice dev(std::move(fx.img));

    tree::ResultTree tree;
    ScanConfig cfg;
    cfg.run_quick = true;
    cfg.run_deep = false;
    cfg.run_carve = false;
    cfg.pre_scan_repair = true; /* hooks left null */
    ScanController ctrl(tree, cfg);
    ScanStats stats;
    u32 rc = ctrl.run(&dev, nullptr, nullptr, nullptr, stats);
    ASSERT_EQ(rc, OK) << err_text(rc);
    EXPECT_TRUE(stats.repair_ran);
    EXPECT_EQ(1u, stats.repair_actions_planned);
    EXPECT_EQ(0u, stats.repair_actions_applied);
    EXPECT_TRUE(stats.repair_consent_denied);
    EXPECT_EQ(0xAB, dev.raw()[TornFatFixture::FAT1 + 0]);
}

} // namespace

/* ---- A-004: ErrorRange must fire through the LIVE sink (set_event_sink),
 * not the config copy (null-call crash / mis-delivery before the fix) ----- */

namespace {

/* device wrapper that fails every read touching [fail_lba, fail_lba+1).
 * FatTable::load reads BOTH FAT copies wholesale (cr_chain.cpp:94/:100),
 * so a failing sector inside the FAT2 span is deterministically read;
 * the retry pipe isolates it to 512 bytes, zero-fills (Skip mode), and the
 * boundary emitter fires an ErrorRange event for the recorded span. The
 * healthy primary FAT keeps the walk itself green. */
class FailingSectorDevice final : public IStorageDevice {
public:
    FailingSectorDevice(std::vector<u8> b, u64 fail_lba)
        : data_(std::move(b)), fail_off_(fail_lba * 512) {
        geom_.logical_sector = 512;
        geom_.physical_sector = 512;
        geom_.sector_count = data_.size() / 512;
        geom_.total_bytes = data_.size();
    }
    u32 read_at(u64 off, void* buf, u32 len) override {
        if (off > data_.size() || len > data_.size() - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        if (off < fail_off_ + 512 && off + len > fail_off_)
            return ERR_READ_FAILED;
        std::memcpy(buf, data_.data() + off, len);
        return OK;
    }
    const Geometry& geometry() const override { return geom_; }

private:
    std::vector<u8> data_;
    u64 fail_off_;
    Geometry geom_;
};

/* event recorder: counts per type + last ErrorRange payload.
 * Thread-safe by the documented event contract (cr_scan.h :110-113):
 * events may fire from engine worker threads and the core does not
 * serialize deliveries - the sink must be. */
struct EventRecorder {
    std::vector<event::Type> seen;
    u64 last_err_start = 0, last_err_len = 0;
    u32 max_error_ranges = 0;
    /* M-049: the error-context triple on the last ErrorRange event */
    u32 last_err_module = 0xFFFFFFFFu, last_err_operation = 0xFFFFFFFFu, last_err_attempts = 0;
    mutable std::mutex m;
    static void on_evt(const event::Event& e, void* user) {
        auto* r = static_cast<EventRecorder*>(user);
        std::lock_guard<std::mutex> lock(r->m);
        r->seen.push_back(e.type);
        if (e.type == event::Type::ErrorRange) {
            r->last_err_start = e.err_start_bytes;
            r->last_err_len = e.err_len_bytes;
            r->max_error_ranges = std::max(r->max_error_ranges, e.error_ranges);
            r->last_err_module = e.err_module;
            r->last_err_operation = e.err_operation;
            r->last_err_attempts = e.err_attempts;
        }
    }
    bool has(event::Type t) const {
        std::lock_guard<std::mutex> lock(m);
        return std::find(seen.begin(), seen.end(), t) != seen.end();
    }
};

} // namespace

TEST(ScanEvents, ErrorRangeGoesToLiveSink) {
    /* the documented embedder pattern: ScanConfig carries NO event fn;
     * subscription happens post-construction via set_event_sink (the
     * GAP-S6-10 resume flow). Before the A-004 fix the guard passed on the
     * live sink but ErrorRange was invoked through cfg_.event_fn (null) -
     * a null function-pointer call mid-scan; with the fix the event
     * arrives at the live sink and the scan completes. */
    FailingSectorDevice dev(build_fat16(),
                            30); /* sector 30: FAT2 span, away from the used low-cluster entries */
    tree::ResultTree tree;
    ScanConfig cfg; /* event_fn null by default */
    cfg.run_quick = true;
    cfg.run_deep = true;
    cfg.run_carve = false;
    cfg.whole_disk = false;
    ScanController ctrl(tree, cfg);
    EventRecorder rec;
    ASSERT_EQ(OK, ctrl.set_event_sink(&EventRecorder::on_evt, &rec));
    ScanStats stats;
    ASSERT_EQ(OK, ctrl.run(&dev, nullptr, nullptr, nullptr, stats));
    /* the isolated failing sector produced at least one ErrorRange event,
     * delivered to the live sink with the recorded span payload */
    ASSERT_TRUE(rec.has(event::Type::ErrorRange));
    EXPECT_EQ(30u * 512u, rec.last_err_start);
    EXPECT_EQ(512u, rec.last_err_len);
    EXPECT_GE(rec.max_error_ranges, 1u);
}

TEST(ScanEvents, SinkSwitchDeliversAllTypes) {
    /* contract: after set_event_sink(B), EVERY event type - including
     * ErrorRange - arrives at B only; the config sink A receives nothing.
     * Before the A-004 fix ErrorRange mis-delivered to A. */
    FailingSectorDevice dev(build_fat16(),
                            30); /* sector 30: FAT2 span, away from the used low-cluster entries */
    tree::ResultTree tree;
    ScanConfig cfg; /* A: the config seed */
    cfg.run_quick = true;
    cfg.run_deep = true;
    cfg.run_carve = false;
    cfg.whole_disk = false;
    EventRecorder a, b;
    cfg.event_fn = &EventRecorder::on_evt;
    cfg.event_user = &a;
    ScanController ctrl(tree, cfg);
    ASSERT_EQ(OK, ctrl.set_event_sink(&EventRecorder::on_evt, &b));
    ScanStats stats;
    ASSERT_EQ(OK, ctrl.run(&dev, nullptr, nullptr, nullptr, stats));
    /* every type the run produces went to the LIVE sink B */
    EXPECT_TRUE(b.has(event::Type::ScanStage));
    EXPECT_TRUE(b.has(event::Type::ScanProgress));
    EXPECT_TRUE(b.has(event::Type::FilesFound));
    ASSERT_TRUE(b.has(event::Type::ErrorRange));
    EXPECT_EQ(30u * 512u, b.last_err_start);
    /* the superseded config sink A received NOTHING */
    EXPECT_TRUE(a.seen.empty());
}

TEST(ScanEvents, SingleBoundaryAtTeardown) {
    /* N-016 (diff-to-fix): the run()-exit stamp emitted
     * emit_boundary_events_ TWICE back-to-back (a 56cce87 merge artifact)
     * - FilesFound double-fired at teardown. The event stream must never
     * carry two ADJACENT FilesFound events (a fresh boundary carries new
     * counters between any two legitimate emits). */
    struct SeqRecorder {
        std::vector<event::Type> seq;
        static void on_evt(const event::Event& e, void* user) {
            static_cast<SeqRecorder*>(user)->seq.push_back(e.type);
        }
    };
    FailingSectorDevice dev(build_fat16(), 30);
    tree::ResultTree tree;
    ScanConfig cfg;
    cfg.run_quick = true;
    cfg.run_deep = true;
    cfg.run_carve = false;
    cfg.whole_disk = false;
    ScanController ctrl(tree, cfg);
    SeqRecorder rec;
    ASSERT_EQ(OK, ctrl.set_event_sink(&SeqRecorder::on_evt, &rec));
    ScanStats stats;
    ASSERT_EQ(OK, ctrl.run(&dev, nullptr, nullptr, nullptr, stats));
    size_t files_found = 0;
    for (size_t i = 0; i < rec.seq.size(); ++i)
        if (rec.seq[i] == event::Type::FilesFound)
            ++files_found;
    /* quick + deep on this fixture: exactly one boundary emit per stage
     * completion plus the run()-exit stamp = 3. The pre-fix double-fire
     * at the stamp produced 4 (the N-016 merge artifact). */
    EXPECT_EQ(files_found, 3u) << "one FilesFound per boundary - no teardown double-fire";
}

/* ---- M-049: the error-context triple (module/operation/retry state) on
 * the ErrorRange event - the engine-side subset of the og's error-object
 * architecture (per-module registrars; the MOM-bus manager layer stays out
 * of core scope) ---- */

namespace {
/* M-049 fixture: build_fat16 plus a SUBDIRECTORY whose cluster sits in
 * the SECOND 256 KiB cache block (sector 532 = cluster 460) - beyond
 * every probe read (they all touch only block 0 on this geometry) but
 * squarely inside the FAT engine's directory traversal. A dead sector
 * there is recorded while the ENGINE-pass thread stamp is active: the
 * per-sub-pass attribution this test proves. */
std::vector<u8> build_fat16_with_far_subdir() {
    std::vector<u8> img = build_fat16();
    const size_t FAT1 = 2 * 512, FAT2 = 22 * 512, ROOT = 42 * 512;
    const u32 SUB_CL = 460; /* sector 74 + 458 = 532: cache block 1 */
    put16(img, FAT1 + SUB_CL * 2, 0xFFFF);
    put16(img, FAT2 + SUB_CL * 2, 0xFFFF);
    u8* root = img.data() + ROOT;
    u8* e = root + 2 * 32; /* slot 2: the first free slot - a zero name[0]
                            * there is the END marker, so the entry must
                            * land at 2 to be walked at all */
    std::memset(e, 0, 32);
    const char* name = "FARSUB     ";
    for (int i = 0; i < 11; ++i)
        e[i] = u8(name[i]);
    e[11] = 0x10; /* ATTR_DIRECTORY */
    put16(img, ROOT + 2 * 32 + 20, 0);
    put16(img, ROOT + 2 * 32 + 26, u16(SUB_CL));
    put32v(img, ROOT + 2 * 32 + 28, 0);
    /* the subdir cluster itself: all-zero dentries (empty directory) */
    return img;
}
} // namespace

TEST(ScanEvents, ErrorRangeCarriesModuleOperationAttempts) {
    /* a persistent read failure inside the FAT engine's own directory
     * traversal (the far subdir cluster - first touched by the engine,
     * never by the probe) must arrive attributed to the FAT module,
     * operation READ, with the retry pipe's attempt count - the triple
     * the og's error objects would have carried as module/proc context. */
    FailingSectorDevice dev(build_fat16_with_far_subdir(), 532);
    tree::ResultTree tree;
    ScanConfig cfg;
    cfg.run_quick = true;
    cfg.run_deep = true;
    cfg.run_carve = false;
    cfg.whole_disk = false;
    ScanController ctrl(tree, cfg);
    EventRecorder rec;
    ASSERT_EQ(OK, ctrl.set_event_sink(&EventRecorder::on_evt, &rec));
    ScanStats stats;
    ASSERT_EQ(OK, ctrl.run(&dev, nullptr, nullptr, nullptr, stats));
    ASSERT_TRUE(rec.has(event::Type::ErrorRange)) << "the engine traversed the far subdir";
    EXPECT_EQ(u32(errmod::kFat), rec.last_err_module) << "attributed to the FAT engine pass";
    EXPECT_EQ(errmod::op_read, rec.last_err_operation) << "the error-range family is reads";
    EXPECT_EQ(3u, rec.last_err_attempts) << "the retry policy's attempt count";
}

TEST(ScanEvents, ProbeBlockFailureAttributesToOrchestrator) {
    /* the SAME dead sector in cache block 0 (the boot/FAT/root region)
     * is first touched by the PROBE's cache-block read - attributed to
     * the orchestrator module: the attribution tracks WHO was reading
     * when the range was recorded, not a stage constant. (The existing
     * A-004 fixtures' sector-30 failures ride this path.) */
    FailingSectorDevice dev(build_fat16(), 30);
    tree::ResultTree tree;
    ScanConfig cfg;
    cfg.run_quick = true;
    cfg.run_deep = true;
    cfg.run_carve = false;
    cfg.whole_disk = false;
    ScanController ctrl(tree, cfg);
    EventRecorder rec;
    ASSERT_EQ(OK, ctrl.set_event_sink(&EventRecorder::on_evt, &rec));
    ScanStats stats;
    ASSERT_EQ(OK, ctrl.run(&dev, nullptr, nullptr, nullptr, stats));
    ASSERT_TRUE(rec.has(event::Type::ErrorRange));
    EXPECT_EQ(u32(errmod::kOrchestrator), rec.last_err_module)
        << "the probe's block read was the first (and only) reader";
    EXPECT_EQ(errmod::op_read, rec.last_err_operation);
}

TEST(ScanEvents, CarveErrorRangeAttributesToCarveModule) {
    /* the same failing sector read by the CARVE sweep attributes to the
     * carve module - the attribution is per-sub-pass, not a constant: the
     * module id distinguishes WHO was reading when the sector died. */
    FailingSectorDevice dev(build_fat16(), 30);
    tree::ResultTree tree;
    ScanConfig cfg;
    cfg.run_quick = false;
    cfg.run_deep = false;
    cfg.run_carve = true;
    cfg.whole_disk = false;
    ScanController ctrl(tree, cfg);
    EventRecorder rec;
    ASSERT_EQ(OK, ctrl.set_event_sink(&EventRecorder::on_evt, &rec));
    ScanStats stats;
    ASSERT_EQ(OK, ctrl.run(&dev, nullptr, nullptr, nullptr, stats));
    ASSERT_TRUE(rec.has(event::Type::ErrorRange)) << "the carve window sweep read the dead sector";
    EXPECT_EQ(u32(errmod::kCarve), rec.last_err_module) << "attributed to the carve sweep";
    EXPECT_EQ(errmod::op_read, rec.last_err_operation);
    EXPECT_EQ(3u, rec.last_err_attempts);
}

TEST(ScanConfig, DeepStageDefaultsOnAndWholeDiskOff) {
    /* N-044: the og's "Deep Scan" toggle means the complete-searcher pass;
     * the core's deep stage is DEFAULT-ON (ScanConfig.run_deep), the flag
     * named --deep in the CLI actually toggled whole-disk lost-partition
     * discovery - a naming hazard. The CLI flag is now --whole-disk
     * (with --deep as a deprecated alias). These config defaults pin the
     * semantics the usage text states: deep stage always on until
     * --quick-only; whole_disk opt-in. */
    orchestrator::ScanConfig cfg{};
    EXPECT_TRUE(cfg.run_deep) << "the deep engine stage is default-on";
    EXPECT_FALSE(cfg.whole_disk) << "lost-partition discovery is opt-in";
    EXPECT_TRUE(cfg.run_quick);
    /* the deprecated alias maps ONLY to whole_disk - never the stages */
    cfg.whole_disk = true; /* what --deep/--whole-disk sets */
    EXPECT_TRUE(cfg.run_deep) << "the alias never touches the deep stage";
    EXPECT_TRUE(cfg.run_carve) << "the alias never touches the carve stage";
}

/* ---- N-016b (N-029 residual): restore filename derived from raw ------- */

TEST(SafeWriter, InvalidUtf8NameRestoredFromRaw) {
    /* The og's restore path names the output file from the ORIGINAL
     * on-disk bytes via MultiByteToWideChar(CP_UTF8, flags=0) - DRWUI
     * :264664-694, invoked at :357529 - which is the maximal-subpart
     * policy (N-016a). A node carrying raw bytes must therefore restore
     * under the FFFD-substituted name, NOT the private-unit display form
     * (the tree/report keep the lossless U+E000+byte display per D-53;
     * the created FILE matches what the og would create). Fail-before:
     * final_name carried the U+E0FF private-use unit. */
    MockIo io;
    std::vector<u8> img(64 * 512, 0);
    const char* payload = "RAWDATA";
    std::memcpy(img.data() + 512, payload, 7);
    MemDevice dev(std::move(img));
    SafeWriter writer(io, &dev, WriterConfig{});
    writer.set_destination(dev.size(), dev.size());

    tree::ResultTree tree;
    auto* ra = tree.add_root(u"R");
    {
        auto* pr = new tree::FileProperties();
        /* display name: the lossless mapping of "\xff(bad" (M-010) */
        pr->name = u"\uE0FF(bad";
        pr->name_raw = "\xff(bad"; /* the original on-disk bytes */
        pr->logical_size = 7;
        tree::FileProperties::Extent ex;
        ex.offset = 512;
        ex.length = 7;
        pr->extents.push_back(ex);
        tree::FileNode* n = tree.add_file(ra, pr);
        pr->release();
        ASSERT_NE(nullptr, n);
        ASSERT_EQ(writer.enqueue(n, u16_("R:\\out")), OK);
    }
    WriterStats ws;
    ASSERT_EQ(writer.process(nullptr, nullptr, nullptr, ws), OK);
    ASSERT_EQ(1u, ws.done);

    /* the og's restore name: 0xFF is an invalid lead -> ONE U+FFFD
     * (maximal subpart), then "(bad" verbatim. resolve_collision's
     * contract returns the BASENAME (the dest_dir join happens at the io
     * boundary) - so the assertion pins the substituted basename. */
    ASSERT_EQ(1u, writer.queue().size());
    EXPECT_EQ(writer.queue()[0].final_name, std::u16string(u"\uFFFD(bad"))
        << "restore filename must substitute per the og's conversion, not "
           "carry private-use units";
}

/* ---- M-058: the read-pipe construction is a pinned contract -------------- */

TEST(Orchestrator, ReadPipeBuildsCacheOverRetryForValidGeometry) {
    MemDevice dev(build_fat16());
    ReadPipe rp = build_read_pipe(&dev, 8ull << 20, 256 * 1024);
    /* the healthy path: engines read through the cache, the live error-map
     * source is the cache's inner retry device, the chain owns it all */
    ASSERT_NE(nullptr, rp.pipe);
    ASSERT_NE(nullptr, rp.chain);
    ASSERT_NE(nullptr, rp.retry);
    EXPECT_NE(rp.pipe, &dev);
    u8 boot[512] = {};
    ASSERT_EQ(rp.pipe->read_at(0, boot, sizeof(boot)), OK);
    EXPECT_EQ(0x55u, boot[510]);
    EXPECT_EQ(0xAAu, boot[511]);
}

TEST(Orchestrator, ReadPipeDegradesToBareSliceWhenCacheCreationFails) {
    MemDevice dev(build_fat16());
    /* cache_bytes < block_bytes -> CachedReader::create refuses AND destroys
     * the retry device it took by value. The pre-M-058 inline construction
     * fell back to the destroyed retry pointer here (use-after-free); the
     * pinned contract is the BARE SLICE. */
    ReadPipe rp = build_read_pipe(&dev, 4096, 8192);
    ASSERT_EQ(rp.pipe, &dev) << "degraded pipe must be the caller's slice";
    EXPECT_EQ(nullptr, rp.chain) << "no cache exists on this path";
    EXPECT_EQ(nullptr, rp.retry) << "no live error-map source on this path";
    /* the degraded pipe must be READABLE (it is the slice itself) */
    u8 boot[512] = {};
    ASSERT_EQ(rp.pipe->read_at(0, boot, sizeof(boot)), OK);
    EXPECT_EQ(0x55u, boot[510]);
    EXPECT_EQ(0xAAu, boot[511]);
}

TEST(Orchestrator, ReadPipeZeroCapacityDegradesToBareSlice) {
    MemDevice dev(build_fat16());
    /* capacity == 0 (cache_bytes == block_bytes is fine, but a zero cache
     * budget must round to zero blocks) - same pinned degradation */
    ReadPipe rp = build_read_pipe(&dev, 0, 256 * 1024);
    ASSERT_EQ(rp.pipe, &dev);
    EXPECT_EQ(nullptr, rp.chain);
    EXPECT_EQ(nullptr, rp.retry);
}

TEST(Orchestrator, LogSinkRegistrySurvivesHeavyChurn) {
    /* N-079 additive pin (the DFX23 convergent landing's test, ported
     * onto the canonical deque-pool fix): the seam must keep delivery
     * on the CURRENT sink through heavy registration churn, superseded
     * registrations must go silent, and the null-unregister path must
     * leave the registry usable without burning pool capacity. The
     * canonical fix's pool is unbounded so 20 registrations exercise the
     * swap bookkeeping; the pin holds for either registry design. */
    LogRecorder current, stale;
    cr::log::set_sink(&LogRecorder::sink, &stale);
    for (int i = 0; i < 20; ++i) /* heavy churn past any small pool */
        cr::log::set_sink(&LogRecorder::sink, &current);
    EXPECT_TRUE(cr::log::enabled());
    cr::log::write(cr::log::Level::Info, "after churn %d", 1);
    cr::log::set_sink(nullptr, nullptr);
    EXPECT_FALSE(cr::log::enabled());
    EXPECT_EQ(stale.lines.size(), 0u) << "superseded registrations go silent";
    ASSERT_FALSE(current.lines.empty());
    EXPECT_NE(current.lines[0].second.find("after churn"), std::string::npos)
        << "delivery follows the CURRENT sink through the churn";
    current.lines.clear();
    cr::log::set_sink(&LogRecorder::sink, &current);
    cr::log::write(cr::log::Level::Warn, "revived");
    cr::log::set_sink(nullptr, nullptr);
    ASSERT_EQ(current.lines.size(), 1u);
    EXPECT_NE(current.lines[0].second.find("revived"), std::string::npos)
        << "the null-publish unregister leaves the registry usable";
}

TEST(Orchestrator, IdentityCardV4SnapshotAndAxes) {
    /* N-066 end-to-end: run() captures the full 512-byte boot-sector image
     * alongside the CRC32 (the og record-3's +0x518 snapshot) and emits it
     * into the VolumeInfo v4 tail; the config's record-3 axes (firmware,
     * WWN, interface) flow through to the card. */
    auto img = build_fat16();
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    ScanConfig cfg;
    cfg.run_quick = true;
    cfg.run_deep = false;
    cfg.run_carve = false;
    std::memcpy(cfg.volume_firmware, "01.02D-04", 9);
    std::memcpy(cfg.volume_wwn, "50014ee2b8a1c2d3", 16);
    std::memcpy(cfg.volume_interface, "USB Device", 10);
    ScanController ctrl(tree, cfg);
    ScanStats stats;
    ASSERT_EQ(ctrl.run(&dev, nullptr, nullptr, nullptr, stats), OK) << err_text(u32(0));
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
    } mf;
    {
        session::Writer w(MemFile::wr, MemFile::tl, MemFile::sk, &mf);
        ASSERT_EQ(w.open(7), OK);
        ASSERT_EQ(ctrl.save_session(w, stats), OK);
        ASSERT_EQ(w.finalize(), OK);
    }
    session::Reader r(
        +[](void* u, void* b, u32 len) -> u32 {
            auto* m = static_cast<MemFile*>(u);
            if (m->pos + len > m->data.size())
                return 1;
            std::memcpy(b, m->data.data() + m->pos, len);
            m->pos += len;
            return 0;
        },
        MemFile::sk, +[](void* u) -> u64 { return static_cast<MemFile*>(u)->data.size(); }, &mf);
    ASSERT_EQ(r.open(nullptr), OK);
    bool saw_vi = false;
    for (u32 i = 0; i < r.record_count(); ++i) {
        session::Record rec;
        ASSERT_EQ(r.read_record(i, rec), OK);
        if (rec.type != session::REC_VOLUME_INFO)
            continue;
        session::VolumeInfo vi;
        ASSERT_EQ(session::deserialize_volume_info(rec.data.data(), rec.data.size(), vi), OK);
        saw_vi = true;
        EXPECT_EQ(0, std::strcmp(vi.firmware, "01.02D-04"));
        EXPECT_EQ(0, std::strcmp(vi.wwn, "50014ee2b8a1c2d3"));
        EXPECT_EQ(0, std::strcmp(vi.interface_desc, "USB Device"));
        /* the snapshot is the fixture's first 512 bytes and the hash is
         * crc32 of the same - two views of ONE read (N-066's contract) */
        u8 boot[512] = {};
        ASSERT_EQ(dev.read_at(0, boot, sizeof(boot)), u32(OK));
        EXPECT_EQ(0, std::memcmp(vi.boot_snapshot, boot, 512));
        EXPECT_EQ(table::crc32(boot, sizeof(boot)), vi.boot_hash);
        break;
    }
    EXPECT_TRUE(saw_vi) << "the VolumeInfo record must be present";
}
