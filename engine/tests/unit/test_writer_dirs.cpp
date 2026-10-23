/* test_writer_dirs.cpp - G-08 acceptance: destination structure
 * reconstruction. The SafeWriter materializes the full directory tree of a
 * recovery: intermediate dirs are created (mkdir -p semantics, cached per
 * unique dir), directory NODES are enqueueable items (empty leaves
 * materialize as real directories), and re-runs are idempotent.
 *
 * Licensed under the MIT license. See LICENSE.
 */
#include <gtest/gtest.h>
#include "cr/recovery/cr_safewriter.h"
#include "cr/storage/cr_device.h"
#include <atomic>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <vector>

using namespace cr;
using namespace cr::recovery;
namespace fs = std::filesystem;

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

/* Filesystem-backed DestIo (std::filesystem = portable Linux/Windows).
 * The writer composes dest paths with u'\\' separators; normalize them to
 * the native separator before touching the fs. */
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
        /* MSVC: path::native() is wstring — the u16 suffix (ASCII) is
         * widened to the native value_type so '+' type-checks on both
         * std::libs (CI unblock for 91053b0's Windows leg). */
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
        /* already-exists is success; only real failures surface */
        return ec ? u32(ERR_DEST_UNWRITABLE) : u32(OK);
    }
    u32 write(u64 id, u64 off, const void* data, u32 len) override {
        auto it = open_.find(id);
        if (it == open_.end())
            return ERR_BAD_PARAMETER;
        std::ofstream f(it->second.temp, std::ios::binary | std::ios::in | std::ios::out);
        if (!f)
            return ERR_WRITE_FAILED;
        f.seekp(std::streamoff(off));
        f.write(static_cast<const char*>(data), len);
        return f ? OK : ERR_WRITE_FAILED;
    }
    u32 commit(u64 id) override {
        auto it = open_.find(id);
        if (it == open_.end())
            return ERR_BAD_PARAMETER;
        std::error_code ec;
        fs::rename(it->second.temp, it->second.final, ec);
        return ec ? u32(ERR_WRITE_FAILED) : u32(OK);
    }
    void close(u64 id) override {
        auto it = open_.find(id);
        if (it == open_.end())
            return;
        std::error_code ec;
        if (!fs::equivalent(it->second.temp, it->second.final, ec) && !ec) {
            /* un-committed temp: remove (atomic semantics) */
            fs::remove(it->second.temp, ec);
        } else if (ec) {
            fs::remove(it->second.temp, ec); /* temp != final (different) */
        }
        open_.erase(it);
    }
    bool exists(const std::u16string& p) override {
        std::error_code ec;
        return fs::exists(root_ / norm(p), ec);
    }

private:
    struct Open {
        fs::path temp, final;
    };
    fs::path root_;
    std::map<u64, Open> open_;
};

/* every created path under root (files and dirs), relative, '/'-joined */
std::set<std::string> walk_tree(const fs::path& root) {
    std::set<std::string> out;
    std::error_code ec;
    for (fs::recursive_directory_iterator it(root, ec), end; it != end; it.increment(ec)) {
        std::string rel = fs::relative(it->path(), root, ec).generic_string();
        if (!it->is_directory(ec))
            out.insert("f:" + rel);
        else
            out.insert("d:" + rel);
    }
    return out;
}

struct TmpDir {
    fs::path p;
    TmpDir() {
        static std::atomic<int> n{0};
        char buf[128];
#ifdef _WIN32
        const int pid = _getpid(); /* MSVC C4996: POSIX name deprecated */
#else
        const int pid = getpid();
#endif
        snprintf(buf, sizeof(buf), "/tmp/cr_wdirs_%d_%d", pid, n.fetch_add(1));
        p = buf;
        fs::create_directories(p);
    }
    ~TmpDir() {
        std::error_code ec;
        fs::remove_all(p, ec);
    }
};

tree::FileProperties* file_props(const char* name, u64 off, u64 len, bool dir = false) {
    auto* p = new tree::FileProperties();
    p->name = std::u16string();
    for (const char* q = name; *q; ++q)
        p->name.push_back(u16(u8(*q)));
    if (!dir)
        p->extents.push_back({off, len});
    p->logical_size = dir ? 0 : len;
    if (dir)
        p->attributes = 0x10;
    return p;
}

/* add_file adopts a reference on the props; the caller-side reference
 * from file_props() is released here (ref-count contract, cf. existing
 * SafeWriter tests) */
tree::FileNode* add_node(tree::ResultTree& t, tree::FileNode* parent, const char* name, u64 off,
                         u64 len, bool dir = false) {
    tree::FileProperties* p = file_props(name, off, len, dir);
    tree::FileNode* n = t.add_file(parent, p);
    p->release();
    return n;
}

/* fixture: Found/a/b/file1.txt, Found/a/empty_leaf/ (EMPTY),
 * Found/empty_top/ (EMPTY), Found/file0.txt */
void build_fixture_tree(tree::ResultTree& tree, std::vector<u8>& img) {
    std::memcpy(img.data(), "F0", 2);
    std::memcpy(img.data() + 512, "F1", 2);
    auto* found = tree.add_root(u"Found");
    (void)add_node(tree, found, "file0.txt", 0, 2);
    auto* a = add_node(tree, found, "a", 0, 0, true);
    auto* b = add_node(tree, a, "b", 0, 0, true);
    (void)add_node(tree, b, "file1.txt", 512, 2);
    (void)add_node(tree, a, "empty_leaf", 0, 0, true);
    (void)add_node(tree, found, "empty_top", 0, 0, true);
    tree.refresh_stats();
}

} // namespace

TEST(WriterDirs, StructureReconstructionWithEmptyLeaves) {
    std::vector<u8> img(1024, 0);
    tree::ResultTree tree;
    build_fixture_tree(tree, img); /* content written BEFORE the device copies */
    MemDevice dev(img);

    TmpDir tmp;
    FsIo io(tmp.p);
    SafeWriter writer(io, &dev, WriterConfig{});

    /* enqueue: the dir nodes themselves + the files (mirrors the CLI walk) */
    std::function<void(tree::FileNode*, const std::u16string&)> plan =
        [&](tree::FileNode* n, const std::u16string& path) {
            for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
                ASSERT_EQ(OK, writer.enqueue(c, path));
                if ((c->props()->attributes & 0x10) != 0)
                    plan(c, path + c->props()->name + u"\\");
            }
        };
    for (auto* r : tree.roots())
        plan(r, u"out\\");

    WriterStats stats;
    ASSERT_EQ(OK, writer.process(nullptr, nullptr, nullptr, stats));
    EXPECT_EQ(u64(4), stats.dirs); /* a, a\b, a\empty_leaf, empty_top */
    EXPECT_EQ(u64(2), stats.done); /* file0.txt, file1.txt */

    /* the full structure materialized - INCLUDING the empty leaves */
    const std::set<std::string> expect = {
        "d:out",           "d:out/a",         "d:out/a/b",           "d:out/a/empty_leaf",
        "d:out/empty_top", "f:out/file0.txt", "f:out/a/b/file1.txt",
    };
    EXPECT_EQ(expect, walk_tree(tmp.p));
}

TEST(WriterDirs, DeepDestChainCreatedAndIdempotentRerun) {
    std::vector<u8> img(512, 0);
    std::memcpy(img.data(), "XY", 2);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* found = tree.add_root(u"Found");
    (void)add_node(tree, found, "deep.txt", 0, 2);
    tree.refresh_stats();

    TmpDir tmp;
    FsIo io(tmp.p);
    SafeWriter writer(io, &dev, WriterConfig{});
    /* a dest_dir chain that does not pre-exist anywhere */
    ASSERT_EQ(OK, writer.enqueue(tree.roots()[0]->first_child(), u"new1\\new2\\new3\\"));

    WriterStats stats;
    ASSERT_EQ(OK, writer.process(nullptr, nullptr, nullptr, stats));
    EXPECT_EQ(u64(1), stats.done);
    EXPECT_TRUE(fs::is_regular_file(tmp.p / "new1/new2/new3/deep.txt"));
    /* content round-trips */
    {
        std::ifstream f(tmp.p / "new1/new2/new3/deep.txt", std::ios::binary);
        char buf[2] = {0, 0};
        f.read(buf, 2);
        EXPECT_EQ(0, std::memcmp(buf, "XY", 2));
    }

    /* idempotent re-run with ConflictPolicy::Skip: the same queue
     * re-processed - file already final (resumed), dirs re-materialized
     * tolerantly, structure unchanged */
    SafeWriter again(io, &dev, [] {
        WriterConfig c;
        c.conflict = ConflictPolicy::Skip;
        return c;
    }());
    ASSERT_EQ(OK, again.enqueue(tree.roots()[0]->first_child(), u"new1\\new2\\new3\\"));
    WriterStats st2;
    ASSERT_EQ(OK, again.process(nullptr, nullptr, nullptr, st2));
    EXPECT_EQ(u64(1), st2.resumed);
    EXPECT_EQ(u64(0), st2.errors);
    EXPECT_EQ(u64(0), st2.done); /* skipped, not re-written */
    EXPECT_TRUE(fs::is_regular_file(tmp.p / "new1/new2/new3/deep.txt"));

    /* no temp leftovers after either run */
    for (const auto& rel : walk_tree(tmp.p))
        EXPECT_EQ(std::string::npos, rel.find(".part-")) << "temp leftover: " << rel;
}

TEST(WriterDirs, IntraQueueDuplicatesRenameUnderSkip) {
    /* two queued nodes with the same name in the same dir: the second
     * renames to "name (2).txt" even under ConflictPolicy::Skip (intra-
     * queue collisions always rename; the destination is not consulted) */
    std::vector<u8> img(512, 0);
    std::memcpy(img.data(), "AA", 2);
    MemDevice dev(img);
    tree::ResultTree tree;
    auto* found = tree.add_root(u"Found");
    (void)add_node(tree, found, "dup.txt", 0, 2);
    (void)add_node(tree, found, "dup.txt", 0, 2);
    tree.refresh_stats();

    TmpDir tmp;
    FsIo io(tmp.p);
    WriterConfig c;
    c.conflict = ConflictPolicy::Skip;
    SafeWriter writer(io, &dev, c);
    tree::FileNode* n1 = found->first_child();
    ASSERT_NE(nullptr, n1);
    tree::FileNode* n2 = n1->next_sibling();
    ASSERT_NE(nullptr, n2);
    ASSERT_EQ(OK, writer.enqueue(n1, u"out\\"));
    ASSERT_EQ(OK, writer.enqueue(n2, u"out\\"));

    WriterStats stats;
    ASSERT_EQ(OK, writer.process(nullptr, nullptr, nullptr, stats));
    EXPECT_EQ(u64(2), stats.done);
    EXPECT_TRUE(fs::is_regular_file(tmp.p / "out/dup.txt"));
    EXPECT_TRUE(fs::is_regular_file(tmp.p / "out/dup (2).txt"));
}
