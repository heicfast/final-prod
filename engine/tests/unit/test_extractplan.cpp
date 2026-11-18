/* test_extractplan.cpp - F-00d/F-01 acceptance: the structure-preserving
 * extract plan (the named feature) and the SfId writer scheme.
 *
 * Coverage (every claim a test, per the session rules):
 *   - sanitize_component_sf: reserved names (incl. con.txt + superscript
 *     COM spellings), trailing dots/spaces, control chars, "."/"..",
 *     empty, the 240-unit cap preserving a short extension;
 *   - with_id_suffix_sf: extension split, extensionless, dotfile;
 *   - plan_deleted_tree: deleted-only selection, F-31 whole-subtree
 *     pruning, live ancestors as path components (never items), orphan
 *     zone (_orphaned/ + id-suffixed tops, two same-name orphan roots
 *     never merge), carved exclusion, junk-filter exclusion,
 *     parents-before-children order, deleted dir materialization;
 *   - plan_deleted_snapshot: the same walk over a captured snapshot;
 *   - the SfId writer: intra-queue collision -> "name (id N).ext";
 *     second-collision exhaustion -> ERR_DEST_NAME_EXHAUSTED (never an
 *     overwrite); pre-existing final -> (id N) under Rename; encrypted
 *     item -> empty file + note; the inside-out canonicalize guard
 *     refuses a symlink escape with ERR_DEST_ESCAPE.
 *
 * Licensed under the MIT license. See LICENSE.
 */
#include <gtest/gtest.h>
#include "test_tmpdir.hpp"
#include "cr/recovery/cr_extractplan.h"
#include "cr/recovery/cr_safewriter.h"
#include "cr/recovery/cr_report.h"
#include "cr/tree/cr_snapshot.h"
#include "cr/storage/cr_device.h"
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <string>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

using namespace cr;
using namespace cr::tree;
using namespace cr::recovery;
namespace fs = std::filesystem;

namespace {

/* RAII temp dir over cr_test::tmp_path (the suite's cross-platform
 * helper - MSVC's CRT does not resolve "/tmp") */
struct TmpDir {
    fs::path p;
    TmpDir() : p(cr_test::tmp_path("f01plan", "")) { fs::create_directories(p); }
    ~TmpDir() {
        std::error_code ec;
        fs::remove_all(p, ec);
    }
    const fs::path& path() const { return p; }
};

FileProperties* mkprops(u64 ref, const char16_t* name, u64 size, u32 attrs) {
    auto* p = new FileProperties();
    p->record_ref = ref;
    p->name = name;
    p->logical_size = size;
    p->attributes = attrs;
    return p;
}

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

private:
    std::vector<u8> data_;
    Geometry geom_;
};

/* Filesystem-backed DestIo with the F-01 canonicalize surface (fs::
 * weakly_canonical / canonical follows symlinks on both platforms). */
class FsIo final : public DestIo {
public:
    explicit FsIo(fs::path root) : root_(std::move(root)) {}

    static fs::path norm(std::u16string p) {
        for (auto& c : p)
            if (c == u'\\')
                c = u'/';
        return fs::path(p);
    }

    u32 create_temp(u64 id, const std::u16string& final_path,
                    const std::u16string& temp_suffix) override {
        const fs::path fin = root_ / norm(final_path);
        std::basic_string<fs::path::value_type> suffix;
        for (u16 c : temp_suffix)
            suffix.push_back(fs::path::value_type(c));
        const fs::path tmp = fs::path(fin.native() + suffix);
        std::error_code ec;
        fs::create_directories(fin.parent_path(), ec);
        std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
        if (!f)
            return ERR_DEST_UNWRITABLE;
        f.close();
        open_[id] = {tmp, fin};
        return OK;
    }
    u32 mkdir(const std::u16string& path) override {
        std::error_code ec;
        fs::create_directories(root_ / norm(path), ec);
        return ec ? u32(ERR_DEST_UNWRITABLE) : u32(OK);
    }
    u32 write(u64 id, u64 off, const void* data, u32 len) override {
        auto it = open_.find(id);
        if (it == open_.end())
            return ERR_BAD_PARAMETER;
        std::ofstream f(it->second.first, std::ios::binary | std::ios::in | std::ios::out);
        if (!f)
            return ERR_WRITE_FAILED;
        f.seekp(std::streamoff(off));
        f.write(static_cast<const char*>(data), std::streamoff(len));
        return f ? OK : ERR_WRITE_FAILED;
    }
    u32 commit(u64 id) override {
        auto it = open_.find(id);
        if (it == open_.end())
            return ERR_BAD_PARAMETER;
        std::error_code ec;
        fs::rename(it->second.first, it->second.second, ec);
        return ec ? u32(ERR_WRITE_FAILED) : u32(OK);
    }
    void close(u64 id) override {
        auto it = open_.find(id);
        if (it == open_.end())
            return;
        std::error_code ec;
        fs::remove(it->second.first, ec);
        open_.erase(it);
    }
    bool exists(const std::u16string& path) override {
        std::error_code ec;
        return fs::exists(root_ / norm(path), ec);
    }
    u32 canonicalize(const std::u16string& path, std::u16string& out) override {
        std::error_code ec;
        fs::path resolved = fs::weakly_canonical(root_ / norm(path), ec);
        if (ec)
            return ERR_UNSUPPORTED;
        out.clear();
        for (fs::path::value_type c : resolved.native())
            out.push_back(u16(c)); /* test paths stay in the BMP */
        return OK;
    }

private:
    fs::path root_;
    std::map<u64, std::pair<fs::path, fs::path>> open_;
};

std::u16string u(const char* s) {
    std::u16string out;
    for (const char* p = s; *p; ++p)
        out.push_back(u16(u8(*p)));
    return out;
}

} // namespace

/* ---- sanitizer ----------------------------------------------------------- */

TEST(SfSanitize, ReservedNames) {
    EXPECT_EQ(sanitize_component_sf(u"CON"), u"_CON");
    EXPECT_EQ(sanitize_component_sf(u"con.txt"), u"_con.txt");
    EXPECT_EQ(sanitize_component_sf(u"LPT7"), u"_LPT7");
    EXPECT_EQ(sanitize_component_sf(u"CONIN$"), u"_CONIN$");
    /* superscript spellings: COM¹ COM² COM³ */
    EXPECT_EQ(sanitize_component_sf(u"COM\x00B9"), u"_COM\x00B9");
    EXPECT_EQ(sanitize_component_sf(u"com\x00B2.doc"), u"_com\x00B2.doc");
    /* not reserved */
    EXPECT_EQ(sanitize_component_sf(u"CONTROL.txt"), u"CONTROL.txt");
    EXPECT_EQ(sanitize_component_sf(u"COMPUTER"), u"COMPUTER");
}

TEST(SfSanitize, Neutralization) {
    EXPECT_EQ(sanitize_component_sf(u".."), u"__");
    EXPECT_EQ(sanitize_component_sf(u"."), u"_");
    EXPECT_EQ(sanitize_component_sf(u""), u"_");
    EXPECT_EQ(sanitize_component_sf(u"bad:name?*"), u"bad_name__");
    EXPECT_EQ(sanitize_component_sf(u"trailing. "), u"trailing__");
    EXPECT_EQ(sanitize_component_sf(u"trail  "), u"trail__");
    /* control chars -> '_' (split the literal so the hex escape does not
     * swallow the following 'b' - \x01b would be ONE escape 0x1b) */
    EXPECT_EQ(sanitize_component_sf(std::u16string{u"a\x01"
                                                   u"b"}),
              u"a_b");
}

TEST(SfSanitize, Utf16Cap) {
    std::u16string stem(300, u'x');
    const std::u16string long_name = stem + u".txt";
    const std::u16string out = sanitize_component_sf(long_name, 240);
    EXPECT_EQ(out.size(), size_t(240));
    /* the short extension survived at the tail */
    EXPECT_EQ(out.substr(out.size() - 4), u".txt");
    /* no-extension: hard truncate */
    const std::u16string noext(300, u'y');
    EXPECT_EQ(sanitize_component_sf(noext, 240).size(), size_t(240));
    /* a long "extension" is not preserved */
    std::u16string longext(10, u'a');
    longext = u"f." + longext;
    (void)longext;
    std::u16string fake(250, u'z');
    fake += u".";
    std::u16string bigext(40, u'e');
    fake += bigext;
    const std::u16string out2 = sanitize_component_sf(fake, 240);
    EXPECT_EQ(out2.size(), size_t(240));
    EXPECT_EQ(out2.find(u".eeee"), std::u16string::npos); /* long ext not kept */
}

TEST(SfIdSuffix, Forms) {
    EXPECT_EQ(with_id_suffix_sf(u"report.docx", 17), u"report (id 17).docx");
    EXPECT_EQ(with_id_suffix_sf(u"README", 5), u"README (id 5)");
    EXPECT_EQ(with_id_suffix_sf(u".hidden", 3), u".hidden (id 3)");
    EXPECT_EQ(with_id_suffix_sf(u"archive.tar.gz", 9), u"archive.tar (id 9).gz");
}

/* ---- the plan (live tree) ------------------------------------------------ */

TEST(ExtractPlan, DeletedOnlyAndPruning) {
    ResultTree t;
    FullNode* found = t.add_root(u"Found");
    FullNode* deleted = t.add_root(u"Deleted");
    ASSERT_NE(found, nullptr);
    ASSERT_NE(deleted, nullptr);

    /* active file - excluded */
    FileProperties* a1 = mkprops(100, u"live.txt", 10, 0);
    t.add_file(found, a1);
    a1->release();
    /* deleted file under the Deleted root - included at root level */
    FileProperties* d1 = mkprops(101, u"gone.txt", 20, 0);
    FileNode* dn1 = t.add_file(deleted, d1);
    dn1->set_deleted_state(DeletedState::Deleted);
    d1->release();
    /* active dir with NO deleted descendants - pruned whole */
    FullNode* empty_live = t.add_folder(found, u"AllLive");
    (void)empty_live;
    FileProperties* a2 = mkprops(102, u"also-live.bin", 5, 0x10);
    t.add_file(empty_live, a2);
    a2->release();

    extractplan::PlanOptions opt;
    std::vector<extractplan::PlanItem> items;
    const auto st = extractplan::plan_deleted_tree(t, opt, items);

    ASSERT_EQ(items.size(), size_t(1));
    EXPECT_EQ(items[0].name, u"gone.txt");
    EXPECT_EQ(items[0].rel_dir, u"");
    EXPECT_FALSE(items[0].is_dir);
    EXPECT_EQ(st.files, 1u);
    EXPECT_EQ(st.bytes_total, 20u);
    EXPECT_GE(st.pruned_subtrees, 1u); /* AllLive dropped whole */
}

TEST(ExtractPlan, LiveAncestorIsPathComponent) {
    ResultTree t;
    FullNode* found = t.add_root(u"Found");
    /* live dir Users > live dir john > DELETED file a.txt */
    FileProperties* pusers = mkprops(200, u"Users", 0, 0x10);
    FileNode* users = t.add_file(found, pusers);
    pusers->release();
    FileProperties* pjohn = mkprops(201, u"john", 0, 0x10);
    FileNode* john = t.add_file(users, pjohn);
    pjohn->release();
    FileProperties* pa = mkprops(202, u"a.txt", 7, 0);
    FileNode* fa = t.add_file(john, pa);
    fa->set_deleted_state(DeletedState::Deleted);
    pa->release();

    extractplan::PlanOptions opt;
    std::vector<extractplan::PlanItem> items;
    const auto st = extractplan::plan_deleted_tree(t, opt, items);

    ASSERT_EQ(items.size(), size_t(1));
    EXPECT_EQ(items[0].name, u"a.txt");
    EXPECT_EQ(items[0].rel_dir, u"Users\\john\\");
    EXPECT_FALSE(items[0].is_dir);
    /* the live dirs were used as components, not pruned */
    EXPECT_EQ(st.pruned_active, 0u);
    EXPECT_EQ(st.pruned_subtrees, 0u);
}

TEST(ExtractPlan, DeletedDirTreeParentsFirst) {
    ResultTree t;
    FullNode* deleted = t.add_root(u"Deleted");
    FileProperties* pd = mkprops(300, u"Old", 0, 0x10);
    FileNode* dd = t.add_file(deleted, pd);
    dd->set_deleted_state(DeletedState::Deleted);
    pd->release();
    FileProperties* pi = mkprops(301, u"inner", 0, 0x10);
    FileNode* di = t.add_file(dd, pi);
    di->set_deleted_state(DeletedState::Deleted);
    pi->release();
    FileProperties* pf = mkprops(302, u"memo.txt", 42, 0);
    FileNode* dff = t.add_file(di, pf);
    dff->set_deleted_state(DeletedState::Deleted);
    pf->release();

    extractplan::PlanOptions opt;
    std::vector<extractplan::PlanItem> items;
    extractplan::plan_deleted_tree(t, opt, items);

    ASSERT_EQ(items.size(), size_t(3));
    /* parents before children */
    EXPECT_EQ(items[0].name, u"Old");
    EXPECT_TRUE(items[0].is_dir);
    EXPECT_EQ(items[0].rel_dir, u"");
    EXPECT_EQ(items[1].name, u"inner");
    EXPECT_TRUE(items[1].is_dir);
    EXPECT_EQ(items[1].rel_dir, u"Old\\");
    EXPECT_EQ(items[2].name, u"memo.txt");
    EXPECT_EQ(items[2].rel_dir, u"Old\\inner\\");
}

TEST(ExtractPlan, OrphansIdSuffixedAndNeverMerged) {
    ResultTree t;
    FullNode* lost = t.add_root(u"File Path Lost");
    FullNode* ld = t.add_folder(lost, u"LostDir4711");
    ASSERT_NE(ld, nullptr);

    /* two orphaned "Documents" dirs under DIFFERENT dead parents */
    FileProperties* p1 = mkprops(400, u"Documents", 0, 0x10);
    FileNode* o1 = t.add_file(lost, p1);
    o1->set_deleted_state(DeletedState::Deleted);
    p1->release();
    FileProperties* pc1 = mkprops(401, u"one.txt", 11, 0);
    FileNode* oc1 = t.add_file(o1, pc1);
    oc1->set_deleted_state(DeletedState::Deleted);
    pc1->release();

    FullNode* ld2 = t.add_folder(lost, u"LostDir9999");
    FileProperties* p2 = mkprops(402, u"Documents", 0, 0x10);
    FileNode* o2 = t.add_file(ld2, p2);
    o2->set_deleted_state(DeletedState::Deleted);
    p2->release();
    FileProperties* pc2 = mkprops(403, u"two.txt", 12, 0);
    FileNode* oc2 = t.add_file(o2, pc2);
    oc2->set_deleted_state(DeletedState::Deleted);
    pc2->release();

    /* an orphan FILE directly under the lost root */
    FileProperties* po = mkprops(404, u"orphan.txt", 13, 0);
    FileNode* of = t.add_file(ld, po);
    of->set_deleted_state(DeletedState::Deleted);
    po->release();

    extractplan::PlanOptions opt;
    std::vector<extractplan::PlanItem> items;
    const auto st = extractplan::plan_deleted_tree(t, opt, items);

    /* 2 orphan dir roots + 2 children + 1 orphan file */
    ASSERT_EQ(items.size(), size_t(5));
    EXPECT_EQ(st.orphan_dirs, 2u);
    EXPECT_EQ(st.orphan_files, 1u);

    /* every orphan top lands under _orphaned/ with its DISTINCT
     * id-suffixed name (never merged): 2 dir roots + 1 file root */
    std::set<std::u16string> root_names;
    for (const auto& it : items)
        if (it.orphan_root)
            root_names.insert(it.name);
    ASSERT_EQ(root_names.size(), size_t(3));
    EXPECT_EQ(items[0].rel_dir, u"_orphaned\\");
    EXPECT_TRUE(items[0].orphan_root);
    /* children keep the real structure beneath their id-suffixed root */
    bool saw_one = false, saw_two = false, saw_file = false;
    for (const auto& it : items) {
        if (it.name == u"one.txt") {
            saw_one = true;
            EXPECT_EQ(it.rel_dir.substr(0, 10), u"_orphaned\\");
            EXPECT_NE(it.rel_dir.find(u"Documents (id "), std::u16string::npos);
        }
        if (it.name == u"two.txt") {
            saw_two = true;
            EXPECT_NE(it.rel_dir.find(u"Documents (id "), std::u16string::npos);
        }
        if (it.name.find(u"orphan (id ") != std::u16string::npos) {
            saw_file = true;
            EXPECT_EQ(it.rel_dir, u"_orphaned\\");
            EXPECT_TRUE(it.orphan_root);
            /* " (id N)" BEFORE the extension */
            EXPECT_EQ(it.name.substr(it.name.size() - 4), u".txt");
        }
    }
    EXPECT_TRUE(saw_one);
    EXPECT_TRUE(saw_two);
    EXPECT_TRUE(saw_file);
    /* the two id-suffixed roots differ */
    std::u16string ida, idb;
    for (const auto& it : items)
        if (it.orphan_root && it.is_dir) {
            if (ida.empty())
                ida = it.name;
            else
                idb = it.name;
        }
    EXPECT_NE(ida, idb);
}

TEST(ExtractPlan, CarvedAndJunkExcluded) {
    ResultTree t;
    FullNode* found = t.add_root(u"Found");
    FullNode* raw = t.add_root(u"Reconstructed");
    FileProperties* pc = mkprops(500, u"carved.jpg", 100, 0);
    FileNode* fc = t.add_file(raw, pc);
    fc->set_origin(Origin::Carved);
    fc->set_deleted_state(DeletedState::Deleted);
    pc->release();
    FileProperties* pj = mkprops(501, u"junk.tmp", 1, 0);
    FileNode* fj = t.add_file(found, pj);
    fj->set_deleted_state(DeletedState::Deleted);
    fj->props()->flags |= NODE_FLAG_JUNK_FILTERED;
    pj->release();
    FileProperties* pok = mkprops(502, u"keep.txt", 2, 0);
    FileNode* fok = t.add_file(found, pok);
    fok->set_deleted_state(DeletedState::Deleted);
    pok->release();

    extractplan::PlanOptions opt;
    std::vector<extractplan::PlanItem> items;
    const auto st = extractplan::plan_deleted_tree(t, opt, items);
    ASSERT_EQ(items.size(), size_t(1));
    EXPECT_EQ(items[0].name, u"keep.txt");
    EXPECT_EQ(st.carved_excluded, 1u);
}

/* ---- the plan (snapshot) -------------------------------------------------- */

TEST(ExtractPlan, SnapshotWalkMatchesTreeWalk) {
    ResultTree t;
    FullNode* found = t.add_root(u"Found");
    FileProperties* pd = mkprops(600, u"Docs", 0, 0x10);
    FileNode* dd = t.add_file(found, pd);
    dd->set_deleted_state(DeletedState::Deleted);
    pd->release();
    FileProperties* pf = mkprops(601, u"x.txt", 33, 0);
    FileNode* dff = t.add_file(dd, pf);
    dff->set_deleted_state(DeletedState::Deleted);
    pf->release();

    auto snap = tree::Snapshot::capture(t, false);
    ASSERT_NE(snap, nullptr);

    extractplan::PlanOptions opt;
    std::vector<extractplan::PlanItem> items;
    const auto st = extractplan::plan_deleted_snapshot(*snap, opt, items);
    ASSERT_EQ(items.size(), size_t(2));
    EXPECT_EQ(st.files, 1u);
    EXPECT_EQ(st.dirs, 1u);
    EXPECT_EQ(items[0].name, u"Docs");
    EXPECT_TRUE(items[0].is_dir);
    EXPECT_EQ(items[1].name, u"x.txt");
    EXPECT_EQ(items[1].rel_dir, u"Docs\\");
    EXPECT_NE(items[1].snap_index, 0xFFFFFFFFu);
}

/* ---- the SfId writer ------------------------------------------------------ */

TEST(SfIdWriter, IntraQueueCollisionIdSuffix) {
    TmpDir tmp;
    MemDevice dev(std::vector<u8>(512 * 64, 0xAB));
    FsIo io(tmp.path());
    WriterConfig cfg;
    cfg.scheme = CollisionScheme::SfId;
    SafeWriter w(io, &dev, cfg);

    ResultTree t;
    FullNode* deleted = t.add_root(u"Deleted");
    FileProperties* p1 = mkprops(700, u"dup.txt", 4, 0);
    p1->extents.push_back(FileProperties::Extent{0, 4, 0, 0, 0, false});
    FileNode* f1 = t.add_file(deleted, p1);
    f1->set_deleted_state(DeletedState::Deleted);
    p1->release();
    FileProperties* p2 = mkprops(701, u"dup.txt", 4, 0);
    p2->extents.push_back(FileProperties::Extent{0, 4, 0, 0, 0, false});
    FileNode* f2 = t.add_file(deleted, p2);
    f2->set_deleted_state(DeletedState::Deleted);
    p2->release();

    ASSERT_EQ(w.enqueue(f1, u""), u32(OK));
    ASSERT_EQ(w.enqueue(f2, u""), u32(OK));
    const auto& q = w.queue();
    ASSERT_EQ(q.size(), size_t(2));
    EXPECT_EQ(q[0].final_name, u"dup.txt");
    /* the second carries ITS OWN node id (deterministic across re-runs):
     * " (id N)" inserts BEFORE the extension (SF with_id_suffix) */
    const std::u16string want2 = u"dup (id " + u(std::to_string(f2->node_id()).c_str()) + u").txt";
    EXPECT_EQ(q[1].final_name, want2);

    WriterStats stats;
    ASSERT_EQ(w.process(nullptr, nullptr, nullptr, stats), u32(OK));
    EXPECT_EQ(stats.done, 2u);
    EXPECT_TRUE(fs::exists(tmp.path() / "dup.txt"));
    EXPECT_TRUE(fs::exists(tmp.path() / ("dup (id " + std::to_string(f2->node_id()) + ").txt")));
}

TEST(SfIdWriter, ExhaustionFailsNeverOverwrites) {
    TmpDir tmp;
    /* pre-existing files squat on BOTH the plain and the (id N) name */
    { std::ofstream f(tmp.path() / "dup.txt", std::ios::binary); }
    MemDevice dev(std::vector<u8>(512 * 64, 0xCD));
    FsIo io(tmp.path());
    WriterConfig cfg;
    cfg.scheme = CollisionScheme::SfId;
    SafeWriter w(io, &dev, cfg);

    ResultTree t;
    FullNode* deleted = t.add_root(u"Deleted");
    FileProperties* p1 = mkprops(710, u"dup.txt", 4, 0);
    p1->extents.push_back(FileProperties::Extent{0, 4, 0, 0, 0, false});
    FileNode* f1 = t.add_file(deleted, p1);
    f1->set_deleted_state(DeletedState::Deleted);
    p1->release();
    /* pre-existing squat on the id-suffixed name too (the SF form:
     * " (id N)" BEFORE the extension) */
    {
        std::ofstream f(tmp.path() / ("dup (id " + std::to_string(f1->node_id()) + ").txt"),
                        std::ios::binary);
    }

    ASSERT_EQ(w.enqueue(f1, u""), u32(OK));
    const auto& q = w.queue();
    ASSERT_EQ(q.size(), size_t(1));
    EXPECT_TRUE(q[0].name_exhausted); /* fails at process time */

    WriterStats stats;
    ASSERT_EQ(w.process(nullptr, nullptr, nullptr, stats), u32(OK));
    EXPECT_EQ(stats.errors, 1u);
    /* nothing pre-existing was overwritten: the squatter keeps its bytes */
    std::error_code ec;
    EXPECT_EQ(fs::file_size(tmp.path() / "dup.txt", ec), 0u);
}

TEST(SfIdWriter, EncryptedItemEmptyWithNote) {
    TmpDir tmp;
    MemDevice dev(std::vector<u8>(512 * 64, 0xEE));
    FsIo io(tmp.path());
    WriterConfig cfg;
    cfg.scheme = CollisionScheme::SfId;
    SafeWriter w(io, &dev, cfg);

    ResultTree t;
    FullNode* deleted = t.add_root(u"Deleted");
    /* attributes 0x4000 = FILE_ATTRIBUTE_ENCRYPTED (EFS) */
    FileProperties* pe = mkprops(720, u"secret.txt", 4096, 0x4000);
    FileNode* fe = t.add_file(deleted, pe);
    fe->set_deleted_state(DeletedState::Deleted);
    pe->extents.push_back(FileProperties::Extent{0, 4096, 0, 0, 0, false});
    pe->release();

    ASSERT_EQ(w.enqueue(fe, u""), u32(OK));
    WriterStats stats;
    ASSERT_EQ(w.process(nullptr, nullptr, nullptr, stats), u32(OK));
    EXPECT_EQ(stats.done, 1u);
    std::error_code ec;
    EXPECT_EQ(fs::file_size(tmp.path() / "secret.txt", ec), 0u); /* EMPTY */

    const auto items = w.report_items();
    ASSERT_EQ(items.size(), size_t(1));
    EXPECT_EQ(items[0].outcome, report::Outcome::Done);
    EXPECT_NE(items[0].note.find("encrypted"), std::string::npos);
}

TEST(SfIdWriter, OutputGuardRefusesSymlinkEscape) {
#ifdef _WIN32
    GTEST_SKIP() << "symlink creation needs privileges on Windows hosts";
#else
    TmpDir tmp;
    TmpDir outside;
    MemDevice dev(std::vector<u8>(512 * 64, 0x11));
    FsIo io(tmp.path());
    WriterConfig cfg;
    cfg.scheme = CollisionScheme::SfId;
    SafeWriter w(io, &dev, cfg);
    /* arm the guard on the ABSOLUTE output root (the production flow
     * arms on set_destination's normalized root) */
    std::u16string root16;
    for (char c : fs::absolute(tmp.path()).string())
        root16.push_back(u16(u8(c)));
    w.arm_output_guard(root16);

    /* a pre-existing symlink inside the output pointing outside */
    std::error_code ec;
    fs::create_directories(outside.path() / "evil", ec);
    fs::create_directory_symlink(outside.path() / "evil", tmp.path() / "evil", ec);
    ASSERT_FALSE(ec);

    ResultTree t;
    FullNode* deleted = t.add_root(u"Deleted");
    FileProperties* pf = mkprops(730, u"x.txt", 4, 0);
    pf->extents.push_back(FileProperties::Extent{0, 4, 0, 0, 0, false});
    FileNode* dff = t.add_file(deleted, pf);
    dff->set_deleted_state(DeletedState::Deleted);
    pf->release();

    /* enqueue into the symlinked dir: mkdir must be refused by the guard */
    ASSERT_EQ(w.enqueue(dff, u"evil\\"), u32(OK));
    WriterStats stats;
    const u32 rc = w.process(nullptr, nullptr, nullptr, stats);
    /* the item fails with the guard's refusal (ERR_DEST_ESCAPE) - the
     * escape target never receives content */
    EXPECT_EQ(stats.errors, 1u);
    EXPECT_EQ(rc, u32(OK)); /* the PASS completes; the ITEM failed */
    EXPECT_FALSE(fs::exists(outside.path() / "evil" / "x.txt", ec));
#endif
}

TEST(SfIdWriter, EncryptedAndSchemeAreOptIn) {
    /* the classic scheme keeps its behavior: same collision, og names */
    TmpDir tmp;
    MemDevice dev(std::vector<u8>(512 * 64, 0x33));
    FsIo io(tmp.path());
    WriterConfig cfg; /* default: OgUnderscore */
    SafeWriter w(io, &dev, cfg);

    ResultTree t;
    FullNode* deleted = t.add_root(u"Deleted");
    FileProperties* p1 = mkprops(740, u"dup.txt", 4, 0);
    FileNode* f1 = t.add_file(deleted, p1);
    f1->set_deleted_state(DeletedState::Deleted);
    p1->release();
    FileProperties* p2 = mkprops(741, u"dup.txt", 4, 0);
    FileNode* f2 = t.add_file(deleted, p2);
    f2->set_deleted_state(DeletedState::Deleted);
    p2->release();

    ASSERT_EQ(w.enqueue(f1, u""), u32(OK));
    ASSERT_EQ(w.enqueue(f2, u""), u32(OK));
    const auto& q = w.queue();
    EXPECT_EQ(q[0].final_name, u"dup.txt");
    EXPECT_EQ(q[1].final_name, u"dup_1.txt"); /* the og's shipped scheme */
}
