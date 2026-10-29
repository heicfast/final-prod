/* test_folderfilter.cpp - M-073 acceptance: the scan-time folder filter
 * (the og's PushFolderFilter surface, UIUX/31:142). The prune must remove
 * out-of-prefix subtrees at the stage boundaries (before checkpoints), the
 * counters must follow the pruned tree, the carve/pathless roots must be
 * exempt, the filter must survive the session round-trip, and the empty
 * filter must be a behavior-unchanged pin.
 *
 * Licensed under the MIT license. See LICENSE.
 */
#include <gtest/gtest.h>
#include "cr/orchestrator/cr_folderfilter.h"
#include "cr/orchestrator/cr_scan.h"
#include "cr/session/cr_crsf.h"
#include "cr/session/cr_records.h"
#include "cr/storage/cr_device.h"
#include <cstring>
#include <functional>
#include <set>
#include <vector>

using namespace cr;
using namespace cr::orchestrator;

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

const u8 ATTR_DIRECTORY = 0x10;

/* A FAT16 volume with TWO top-level folders and root-level files, so a
 * folder-prefix filter has an in-scope subtree, an out-of-scope subtree,
 * and out-of-scope root files to prune:
 *   \INSCOPE\KEEP.TXT     (cluster 15)  - in scope under "INSCOPE"
 *   \INSCOPE\SUB\DEEP.TXT (cluster 16)  - nested in scope
 *   \OTHER\DROP.TXT       (cluster 17)  - out of scope (whole folder)
 *   \ROOTFILE.TXT         (cluster 18)  - out of scope (root level)
 *   \GONE.TXT (deleted)   (cluster 19)  - out of scope, deleted-state
 */
std::vector<u8> build_filtered_fat16() {
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
    std::memcpy(b + 0x2B, "FILTERVOL  ", 11);
    std::memcpy(b + 0x36, "FAT16   ", 8);
    b[510] = 0x55;
    b[511] = 0xAA;

    const size_t FAT1 = 2 * 512, FAT2 = 22 * 512, ROOT = 42 * 512, DATA = 74 * 512;
    auto link = [&](u32 c, u16 v) {
        put16(img, FAT1 + c * 2, v);
        put16(img, FAT2 + c * 2, v);
    };
    put16(img, FAT1 + 0 * 2, 0xFFF8);
    put16(img, FAT1 + 1 * 2, 0xFFFF);
    put16(img, FAT2 + 0 * 2, 0xFFF8);
    put16(img, FAT2 + 1 * 2, 0xFFFF);
    link(5, 0xFFFF);  /* \INSCOPE dir */
    link(6, 0xFFFF);  /* \INSCOPE\SUB dir */
    link(15, 0xFFFF); /* KEEP.TXT */
    link(16, 0xFFFF); /* DEEP.TXT */
    link(17, 0xFFFF); /* DROP.TXT */
    link(18, 0xFFFF); /* ROOTFILE.TXT */
    link(19, 0xFFFF); /* GONE.TXT (deleted, intact chain) */

    auto dentry = [&](u8* slot, const char* n11, u8 attrs, u32 cluster, u32 size, bool deleted) {
        std::memset(slot, 0, 32);
        for (int i = 0; i < 11 && n11[i]; ++i)
            slot[i] = u8(n11[i]);
        for (int i = 0; i < 11; ++i)
            if (slot[i] == 0)
                slot[i] = u8(' ');
        if (deleted)
            slot[0] = 0xE5;
        slot[11] = attrs;
        /* valid FAT date/time (2025-02-06 12:00 - the dentry contract the
         * emission walk validates; zero dates are the invalid class) */
        put16(img, size_t(slot - img.data()) + 14, 0x6000);
        put16(img, size_t(slot - img.data()) + 16, 0x5A46);
        put16(img, size_t(slot - img.data()) + 18, 0x5A46);
        put16(img, size_t(slot - img.data()) + 22, 0x6000);
        put16(img, size_t(slot - img.data()) + 24, 0x5A46);
        put16(img, size_t(slot - img.data()) + 26, u16(cluster));
        put32v(img, size_t(slot - img.data()) + 28, size);
    };

    u8* root = img.data() + ROOT;
    dentry(root + 0 * 32, "INSCOPE    ", ATTR_DIRECTORY, 5, 0, false);
    dentry(root + 1 * 32, "OTHER      ", ATTR_DIRECTORY, 7, 0, false);
    dentry(root + 2 * 32, "ROOTFILETXT", 0x20, 18, 64, false);
    dentry(root + 3 * 32, "GONE    TXT", 0x20, 19, 32, true);

    u8* inscope = img.data() + DATA + (5 - 2) * 512;
    dentry(inscope + 0 * 32, ".          ", ATTR_DIRECTORY, 5, 0, false);
    dentry(inscope + 1 * 32, "..         ", ATTR_DIRECTORY, 5, 0, false);
    dentry(inscope + 2 * 32, "SUB        ", ATTR_DIRECTORY, 6, 0, false);
    dentry(inscope + 3 * 32, "KEEP    TXT", 0x20, 15, 128, false);

    u8* sub = img.data() + DATA + (6 - 2) * 512;
    dentry(sub + 0 * 32, ".          ", ATTR_DIRECTORY, 6, 0, false);
    dentry(sub + 1 * 32, "..         ", ATTR_DIRECTORY, 5, 0, false);
    dentry(sub + 2 * 32, "DEEP    TXT", 0x20, 16, 96, false);

    u8* other = img.data() + DATA + (7 - 2) * 512;
    dentry(other + 0 * 32, ".          ", ATTR_DIRECTORY, 7, 0, false);
    dentry(other + 1 * 32, "..         ", ATTR_DIRECTORY, 7, 0, false);
    dentry(other + 2 * 32, "DROP    TXT", 0x20, 17, 64, false);

    std::memcpy(img.data() + DATA + (15 - 2) * 512, "KEEPDATA", 8);
    std::memcpy(img.data() + DATA + (16 - 2) * 512, "DEEPDATA", 8);
    return img;
}

/* one carvable JPEG in the tail (the carve-root exemption pin needs the
 * carve stage to find something) */
void plant_jpeg_tail(std::vector<u8>& img) {
    static const u8 jpg[] = {
        0xFF, 0xD8, 0xFF, 0xE0, 0x00, 0x10, 'J',  'F',  'I',  'F',  0x00, 0x01, 0x01, 0x00,
        0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0xFF, 0xDB, 0x00, 0x43, 0x00, 0x08, 0x06, 0x06,
        0x07, 0x06, 0x05, 0x08, 0x07, 0x07, 0x07, 0x09, 0x09, 0x08, 0x0A, 0x0C, 0x14, 0x0D,
        0x0C, 0x0B, 0x0B, 0x0C, 0x18, 0x10, 0x13, 0x14, 0x0F, 0x18, 0x17, 0x1A, 0x1A, 0x1A,
        0xFF, 0xC0, 0x00, 0x0B, 0x08, 0x00, 0x04, 0x00, 0x04, 0x01, 0x01, 0x11, 0x00, 0xFF,
        0xC4, 0x00, 0x1F, 0x00, 0x00, 0x01, 0x05, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0xFF, 0xDA, 0x00, 0x08, 0x01, 0x01, 0x00, 0x00, 0x3F, 0x00,
        0x7B, 0x94, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xD9};
    std::memcpy(img.data() + img.size() - 2 * 512, jpg, sizeof(jpg));
    /* a nonzero stream: the carve walker needs the jpeg extent to be
     * nonzero data, not a zero tail */
    for (size_t k = sizeof(jpg); k < 512; ++k)
        img[img.size() - 2 * 512 + k] = u8(0x41 + (k % 26));
}

/* directory-ness for the walk: FullNode folders AND light nodes with the
 * DOS directory attribute (the FAT engines' shape - see the prune's own
 * node_is_dir note) */
bool is_dir_node(const tree::FileNode* n) {
    if (n->is_full())
        return true;
    const tree::FileProperties* p = n->props();
    return p != nullptr && (p->attributes & 0x10) != 0;
}

std::vector<std::string> collect_paths(tree::ResultTree& t) {
    std::vector<std::string> paths;
    std::function<void(tree::FileNode*, std::string)> walk = [&](tree::FileNode* n,
                                                                 const std::string& acc) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            std::string name = c->name_utf8();
            std::string path = acc.empty() ? name : acc + "/" + name;
            if (is_dir_node(c))
                walk(c, path);
            else
                paths.push_back(path);
        }
    };
    for (auto* r : t.roots())
        walk(r, std::string());
    return paths;
}

/* the in-memory sink pair the session writer/reader need (the container
 * is pure-core: callbacks only). A REAL seekable sink: the writer's
 * finalize() patches header fields in place, so an append-only sink
 * corrupts the container (the test_orchestrator MemFile pattern). */
struct MemSink {
    std::vector<u8> bytes;
    u64 pos = 0;
    static u32 write_fn(void* user, const void* p, u32 len) {
        auto* s = static_cast<MemSink*>(user);
        const u8* b = static_cast<const u8*>(p);
        if (s->pos + len > s->bytes.size())
            s->bytes.resize(size_t(s->pos + len));
        std::memcpy(s->bytes.data() + s->pos, b, len);
        s->pos += len;
        return 0;
    }
    static u64 tell_fn(void* user) { return static_cast<MemSink*>(user)->pos; }
    static u32 seek_fn(void* user, u64 abs) {
        static_cast<MemSink*>(user)->pos = abs;
        return 0;
    }
    static u32 read_fn(void* user, void* buf, u32 len) {
        auto* s = static_cast<MemSink*>(user);
        if (s->pos + len > s->bytes.size())
            return u32(0x50030001);
        std::memcpy(buf, s->bytes.data() + s->pos, len);
        s->pos += len;
        return 0;
    }
    static u64 size_fn(void* user) { return static_cast<MemSink*>(user)->bytes.size(); }
    static u32 rseek_fn(void* user, u64 abs) {
        static_cast<MemSink*>(user)->pos = abs;
        return 0;
    }
};

} // namespace

/* ---- the pure prune module -------------------------------------------- */

TEST(FolderFilter, NormalizePrefixFoldsSeparatorsAndCase) {
    std::string out;
    EXPECT_TRUE(folderfilter::normalize_prefix("\\Users\\Alice\\", out));
    EXPECT_EQ(out, "users/alice");
    EXPECT_TRUE(folderfilter::normalize_prefix("/INSCOPE", out));
    EXPECT_EQ(out, "inscope");
    EXPECT_FALSE(folderfilter::normalize_prefix("", out));
    EXPECT_FALSE(folderfilter::normalize_prefix("/", out));
    EXPECT_FALSE(folderfilter::normalize_prefix("\\", out));
    EXPECT_FALSE(folderfilter::normalize_prefix("//", out));
}

TEST(FolderFilter, FileAndFolderMatchSemantics) {
    /* file: raw string prefix (the og filter-bar family, cr_query's
     * starts_with_ci) */
    EXPECT_TRUE(folderfilter::file_path_matches("users/alice/a.txt", "users/alice"));
    EXPECT_TRUE(folderfilter::file_path_matches("users", "users"));
    EXPECT_FALSE(folderfilter::file_path_matches("other/a.txt", "users/alice"));
    EXPECT_FALSE(folderfilter::file_path_matches("users.txt", "users/alice"));
    /* folder: prefix-compatible in either direction (path_compatible) */
    EXPECT_TRUE(folderfilter::folder_path_compatible("users", "users/alice"));
    EXPECT_TRUE(folderfilter::folder_path_compatible("users/alice/docs", "users/alice"));
    EXPECT_FALSE(folderfilter::folder_path_compatible("other", "users/alice"));
    EXPECT_FALSE(folderfilter::folder_path_compatible("users2", "users/alice"));
}

TEST(FolderFilter, PruneRootRemovesOutOfPrefixAndKeepsInPrefix) {
    tree::ResultTree t;
    auto* r = t.add_root(u"Found");
    auto* in = t.add_folder(r, u"INSCOPE");
    auto* keep = t.add_folder(in, u"SUB");
    auto* other = t.add_folder(r, u"OTHER");
    auto* windows = t.add_folder(r, u"Windows");
    auto add_file = [&](tree::FileNode* parent, const char16_t* name, bool deleted) {
        auto* p = new tree::FileProperties();
        p->name = name;
        tree::FileNode* n = t.add_file(parent, p);
        p->release();
        if (deleted)
            n->set_deleted_state(tree::DeletedState::Deleted);
        return n;
    };
    add_file(keep, u"DEEP.TXT", false);
    add_file(in, u"KEEP.TXT", false);
    add_file(other, u"DROP.TXT", false);
    add_file(other, u"DROP2.TXT", true);
    add_file(windows, u"NOTEPAD.EXE", false);
    add_file(r, u"ROOTFILE.TXT", false); /* root-level file: out of scope */

    const folderfilter::PruneStats ps = folderfilter::prune_root(t, r, {"inscope"});
    /* pruned: DROP.TXT, DROP2.TXT (deleted), NOTEPAD.EXE, ROOTFILE.TXT */
    EXPECT_EQ(ps.files, 4u);
    EXPECT_EQ(ps.deleted, 1u);

    const std::vector<std::string> paths = collect_paths(t);
    std::set<std::string> got(paths.begin(), paths.end());
    EXPECT_EQ(got.count("INSCOPE/KEEP.TXT"), 1u);
    EXPECT_EQ(got.count("INSCOPE/SUB/DEEP.TXT"), 1u);
    EXPECT_EQ(got.size(), 2u) << "out-of-prefix files must be gone";
}

TEST(FolderFilter, PruneRootMultiplePrefixesAndCase) {
    tree::ResultTree t;
    auto* r = t.add_root(u"Found");
    auto* a = t.add_folder(r, u"Users");
    auto* b = t.add_folder(r, u"ProgData");
    auto* c = t.add_folder(r, u"Windows");
    auto add_file = [&](tree::FileNode* parent, const char16_t* name) {
        auto* p = new tree::FileProperties();
        p->name = name;
        t.add_file(parent, p);
        p->release();
    };
    add_file(a, u"A.TXT");
    add_file(b, u"B.DAT");
    add_file(c, u"C.EXE");
    /* two prefixes, mixed case + backslash form */
    const folderfilter::PruneStats ps = folderfilter::prune_root(t, r, {"USERS", "progdata\\"});
    EXPECT_EQ(ps.files, 1u); /* only C.EXE is out of both scopes */
    const std::vector<std::string> paths = collect_paths(t);
    EXPECT_EQ(paths.size(), 2u);
}

TEST(FolderFilter, PruneRootEmptyPrefixesAreOff) {
    tree::ResultTree t;
    auto* r = t.add_root(u"Found");
    auto* p = new tree::FileProperties();
    p->name = u"X.TXT";
    t.add_file(r, p);
    p->release();
    /* every prefix normalizes empty -> the filter is OFF, nothing pruned */
    const folderfilter::PruneStats ps = folderfilter::prune_root(t, r, {"", "/", "\\"});
    EXPECT_EQ(ps.files, 0u);
    EXPECT_EQ(collect_paths(t).size(), 1u);
}

TEST(FolderFilter, PruneRootKeepsParentOfScope) {
    tree::ResultTree t;
    auto* r = t.add_root(u"Found");
    auto* users = t.add_folder(r, u"Users");        /* ancestor of scope: stays */
    auto* alice = t.add_folder(users, u"alice");    /* the scope */
    auto* docs = t.add_folder(alice, u"Documents"); /* inside: stays */
    auto* bob = t.add_folder(users, u"bob");        /* sibling: pruned */
    auto add_file = [&](tree::FileNode* parent, const char16_t* name) {
        auto* p = new tree::FileProperties();
        p->name = name;
        t.add_file(parent, p);
        p->release();
    };
    add_file(docs, u"D.TXT");
    add_file(bob, u"B.TXT");
    const folderfilter::PruneStats ps = folderfilter::prune_root(t, r, {"users/alice"});
    EXPECT_EQ(ps.files, 1u); /* bob's file */
    const std::vector<std::string> paths = collect_paths(t);
    ASSERT_EQ(paths.size(), 1u);
    EXPECT_EQ(paths[0], "Users/alice/Documents/D.TXT");
}

/* ---- the scan-pipeline integration ------------------------------------ */

TEST(ScanTimeFolderFilter, PrunesOutOfPrefixNodes) {
    /* THE acceptance: a seeded volume scanned with a folder filter keeps
     * only the in-prefix subtree - out-of-prefix folders, their files and
     * out-of-scope root files never appear in the tree (the og's
     * "constrains incremental tree insertion" observable). */
    MemDevice dev(build_filtered_fat16());
    tree::ResultTree tree;
    ScanConfig cfg;
    cfg.run_carve = false;
    cfg.filter_folders = {"INSCOPE"};
    ScanController ctrl(tree, cfg);
    ScanStats stats;
    ASSERT_EQ(OK, ctrl.run(&dev, nullptr, nullptr, nullptr, stats));

    const std::vector<std::string> paths = collect_paths(tree);
    std::set<std::string> got(paths.begin(), paths.end());
    EXPECT_EQ(got.count("INSCOPE/KEEP.TXT"), 1u);
    EXPECT_EQ(got.count("INSCOPE/SUB/DEEP.TXT"), 1u);
    EXPECT_EQ(got.count("OTHER/DROP.TXT"), 0u) << "the out-of-scope folder must be pruned";
    EXPECT_EQ(got.count("ROOTFILE.TXT"), 0u) << "the out-of-scope root file must be pruned";
    EXPECT_EQ(got.count("#ONE.TXT"), 0u)
        << "the out-of-scope deleted file must be pruned (the ' #' placeholder name)";
    EXPECT_EQ(got.size(), 2u);
    /* the counters describe the pruned tree (the presented truth) */
    EXPECT_EQ(stats.files_found, 2u);
    EXPECT_EQ(stats.folder_filtered, 3u); /* DROP.TXT + ROOTFILE.TXT + GONE.TXT */
}

TEST(ScanTimeFolderFilter, EmptyIsOff) {
    /* the behavior-unchanged pin: the same fixture without a filter keeps
     * every node (the filter is what removes them, not the fixture). */
    MemDevice dev(build_filtered_fat16());
    tree::ResultTree tree;
    ScanConfig cfg;
    cfg.run_carve = false;
    ScanController ctrl(tree, cfg);
    ScanStats stats;
    ASSERT_EQ(OK, ctrl.run(&dev, nullptr, nullptr, nullptr, stats));
    const std::vector<std::string> paths = collect_paths(tree);
    std::set<std::string> got(paths.begin(), paths.end());
    EXPECT_EQ(got.count("INSCOPE/KEEP.TXT"), 1u);
    EXPECT_EQ(got.count("INSCOPE/SUB/DEEP.TXT"), 1u);
    EXPECT_EQ(got.count("OTHER/DROP.TXT"), 1u);
    EXPECT_EQ(got.count("ROOTFILE.TXT"), 1u);
    EXPECT_EQ(got.count("#ONE.TXT"), 1u); /* the deleted 8.3 placeholder name (N-172) */
    EXPECT_EQ(stats.folder_filtered, 0u);
    EXPECT_EQ(stats.files_found, 5u);
}

TEST(ScanTimeFolderFilter, CarveRootIsExempt) {
    /* the scoping decision: pathless content (the carve root) is never
     * pruned by a folder filter - the planted JPEG survives a filter that
     * matches no folder. */
    std::vector<u8> img = build_filtered_fat16();
    plant_jpeg_tail(img);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    ScanConfig cfg;
    cfg.filter_folders = {"INSCOPE"};
    ScanController ctrl(tree, cfg);
    ScanStats stats;
    ASSERT_EQ(OK, ctrl.run(&dev, nullptr, nullptr, nullptr, stats));
    EXPECT_GT(stats.carved_found, 0u) << "the carve stage must find the planted JPEG";
    /* a carved-origin file node survives in the tree (the carve root is
     * exempt: pathless content is never pruned by a folder filter) */
    bool carved_seen = false;
    std::function<void(tree::FileNode*)> scan = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            if (!is_dir_node(c) && c->origin() == tree::Origin::Carved)
                carved_seen = true;
            scan(c);
        }
    };
    for (auto* r : tree.roots())
        scan(r);
    EXPECT_TRUE(carved_seen) << "the carved JPEG must survive the folder filter";
}

TEST(ScanTimeFolderFilter, SurvivesSessionRoundTrip) {
    /* save with a filter -> the session carries REC_SCAN_FILTER -> the
     * reload hands it back in VolumeSessionInfo and resume adopts it (the
     * og's AdvanceFilter state persists across saves). */
    MemDevice dev(build_filtered_fat16());
    tree::ResultTree tree;
    ScanConfig cfg;
    cfg.run_carve = false;
    cfg.filter_folders = {"\\INSCOPE\\"}; /* raw form: round-trips verbatim */
    ScanController ctrl(tree, cfg);
    ScanStats stats;
    ASSERT_EQ(OK, ctrl.run(&dev, nullptr, nullptr, nullptr, stats));

    MemSink sink;
    session::Writer w(MemSink::write_fn, MemSink::tell_fn, MemSink::seek_fn, &sink);
    ASSERT_EQ(OK, w.open(0x52435253));
    ASSERT_EQ(OK, ctrl.save_session(w, stats));
    ASSERT_EQ(OK, w.finalize());

    /* the record is present and round-trips byte-exact */
    {
        session::Reader r(MemSink::read_fn, MemSink::rseek_fn, MemSink::size_fn, &sink);
        tree::ResultTree tree2;
        ScanStats stats2;
        session::VolumeSessionInfo info;
        ASSERT_EQ(OK, ScanController::load_session_summary(r, tree2, stats2, info));
        ASSERT_EQ(info.filter_folders.size(), 1u);
        EXPECT_EQ(info.filter_folders[0], "\\INSCOPE\\");
    }
    /* resume adopts the stored filter when the host config carries none */
    {
        session::Reader r(MemSink::read_fn, MemSink::rseek_fn, MemSink::size_fn, &sink);
        tree::ResultTree tree2;
        ScanStats stats2;
        session::VolumeSessionInfo info;
        std::unique_ptr<ScanController> resumed;
        ScanConfig fresh; /* no filter: the session's must come back */
        fresh.run_quick = false;
        fresh.run_deep = false;
        fresh.run_carve = false;
        ASSERT_EQ(OK, ScanController::resume(r, tree2, fresh, stats2, info, resumed));
        /* every stage already done: the resumed run is a no-op, but the
         * filter is observable through the controller's saved config - the
         * save below re-emits the same REC_SCAN_FILTER */
        MemSink sink2;
        session::Writer w2(MemSink::write_fn, MemSink::tell_fn, MemSink::seek_fn, &sink2);
        ASSERT_EQ(OK, w2.open(0x52435253));
        ScanStats stats3;
        ASSERT_EQ(OK, resumed->save_session(w2, stats3));
        ASSERT_EQ(OK, w2.finalize());
        session::Reader r2(MemSink::read_fn, MemSink::rseek_fn, MemSink::size_fn, &sink2);
        tree::ResultTree tree3;
        ScanStats stats4;
        session::VolumeSessionInfo info2;
        ASSERT_EQ(OK, ScanController::load_session_summary(r2, tree3, stats4, info2));
        ASSERT_EQ(info2.filter_folders.size(), 1u);
        EXPECT_EQ(info2.filter_folders[0], "\\INSCOPE\\");
    }
}

TEST(ScanTimeFolderFilter, SessionWithoutFilterLoadsCleanly) {
    /* pre-M-073 sessions (no REC_SCAN_FILTER) load with an empty filter -
     * the optional-record rule; nothing changes for them. */
    MemDevice dev(build_filtered_fat16());
    tree::ResultTree tree;
    ScanConfig cfg;
    cfg.run_carve = false;
    ScanController ctrl(tree, cfg);
    ScanStats stats;
    ASSERT_EQ(OK, ctrl.run(&dev, nullptr, nullptr, nullptr, stats));
    MemSink sink;
    session::Writer w(MemSink::write_fn, MemSink::tell_fn, MemSink::seek_fn, &sink);
    ASSERT_EQ(OK, w.open(0x52435253));
    ASSERT_EQ(OK, ctrl.save_session(w, stats));
    ASSERT_EQ(OK, w.finalize());
    session::Reader r(MemSink::read_fn, MemSink::rseek_fn, MemSink::size_fn, &sink);
    tree::ResultTree tree2;
    ScanStats stats2;
    session::VolumeSessionInfo info;
    ASSERT_EQ(OK, ScanController::load_session_summary(r, tree2, stats2, info));
    EXPECT_TRUE(info.filter_folders.empty());
}

TEST(ScanTimeFolderFilter, TornScanFilterRecordDegradesToNoFilter) {
    /* a malformed record (lengths beyond the payload) yields NO filter,
     * never a failed load (the honest-degrade rule). */
    std::vector<u8> blob;
    ASSERT_EQ(OK, session::serialize_scan_filter({"users/alice", "docs"}, blob));
    /* truncate mid-payload: the second prefix's bytes are gone */
    blob.resize(blob.size() - 3);
    std::vector<std::string> out;
    EXPECT_NE(OK, session::deserialize_scan_filter(blob.data(), blob.size(), out));
    EXPECT_TRUE(out.empty());
    /* and the well-formed form round-trips */
    ASSERT_EQ(OK, session::serialize_scan_filter({"users/alice", "docs"}, blob));
    EXPECT_EQ(OK, session::deserialize_scan_filter(blob.data(), blob.size(), out));
    ASSERT_EQ(out.size(), 2u);
    EXPECT_EQ(out[0], "users/alice");
    EXPECT_EQ(out[1], "docs");
}

TEST(ScanTimeFolderFilter, WholeDiskVolumeRootsArePruned) {
    /* a whole-disk scan's per-volume roots carry real folder structure:
     * the filter applies to them too (scan --disk N --whole-disk with a
     * folder filter scopes every volume's tree). */
    std::vector<u8> img = build_filtered_fat16();
    /* wrap the volume in an MBR: partition at LBA 2048 */
    const size_t vol_bytes = img.size();
    std::vector<u8> disk(vol_bytes + 2048 * 512, 0);
    std::memcpy(disk.data() + 2048 * 512, img.data(), vol_bytes);
    u8* mbr = disk.data();
    mbr[510] = 0x55;
    mbr[511] = 0xAA;
    u8* pte = mbr + 446; /* first partition entry */
    pte[0] = 0x80;       /* bootable */
    pte[4] = 0x06;       /* FAT16 */
    put32v(disk, size_t(pte - disk.data()) + 8, 2048);
    put32v(disk, size_t(pte - disk.data()) + 12, u32(vol_bytes / 512));

    MemDevice dev(std::move(disk));
    tree::ResultTree tree;
    ScanConfig cfg;
    cfg.whole_disk = true;
    cfg.run_carve = false;
    cfg.filter_folders = {"INSCOPE"};
    ScanController ctrl(tree, cfg);
    ScanStats stats;
    ASSERT_EQ(OK, ctrl.run(&dev, nullptr, nullptr, nullptr, stats));
    /* every file path in the tree is under INSCOPE (the volume root's
     * display name prefixes the paths; collect_paths is root-relative so
     * the assert is on the leaf structure) */
    /* collect_paths is TREE-root-relative (the volume root is itself a
     * tree root): every surviving file row must be inside INSCOPE */
    const std::vector<std::string> paths = collect_paths(tree);
    for (const auto& p : paths) {
        if (p.find('.') == std::string::npos)
            continue; /* folder rows (the volume root's children) */
        const bool in_scope = p == "INSCOPE/KEEP.TXT" || p == "INSCOPE/SUB/DEEP.TXT" ||
                              p == "KEEP.TXT" || p == "DEEP.TXT";
        EXPECT_TRUE(in_scope) << "unexpected surviving row: " << p;
    }
    EXPECT_EQ(stats.folder_filtered, 3u);
}
