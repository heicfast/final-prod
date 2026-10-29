/* test_writer_dirs.cpp - G-08 acceptance: destination structure
 * reconstruction. The SafeWriter materializes the full directory tree of a
 * recovery: intermediate dirs are created (mkdir -p semantics, cached per
 * unique dir), directory NODES are enqueueable items (empty leaves
 * materialize as real directories), and re-runs are idempotent.
 *
 * Licensed under the MIT license. See LICENSE.
 */
#include <gtest/gtest.h>
#include "test_tmpdir.hpp"
#include "cr/recovery/cr_safewriter.h"
#include "cr/recovery/cr_report.h"
#include "cr/storage/cr_device.h"
#include <atomic>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <set>
#ifdef _WIN32
/* S-060 (the win-build repair): the metadata apply's Windows branch rides
 * the production WinDestIo's own SDK surface (SetFileAttributesW /
 * SetFileTime) - the POSIX-only utimensat/chmod shape was the MSVC red */
#include <windows.h>
#else
#include <sys/stat.h>
#include <fcntl.h> /* S-060: utimensat/AT_FDCWD in the FsIo metadata apply */
#endif
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
    /* S-060: record the writer's post-commit metadata calls (the
     * restoration assertions read the recorded FileMeta set); the
     * inject flag forces a refusal for the non-fatal path pin. The
     * POSIX translation also APPLIES times/attributes via
     * utimensat/chmod, so the on-disk state is verifiable too. */
    u32 set_metadata(u64, const std::u16string& final_path, const FileMeta& meta) override {
        if (metadata_fail)
            return ERR_WRITE_FAILED;
        if (metadata_unsupported)
            return ERR_UNSUPPORTED;
        RecordedMeta rec;
        std::string p8;
        for (u16 c : final_path)
            p8.push_back(char(c & 0xFF));
        rec.path = p8;
        rec.meta = meta;
        recorded.push_back(rec);
        return FileDestIoLikeApply_(final_path, meta);
    }

    struct RecordedMeta {
        std::string path;
        FileMeta meta;
    };
    std::vector<RecordedMeta> recorded;
    bool metadata_fail = false;
    bool metadata_unsupported = false;

private:
    /* the portable apply (utimensat + the readonly-bit chmod), best
     * effort - the recording above is the assertion surface */
    u32 FileDestIoLikeApply_(const std::u16string& final_path, const FileMeta& meta) {
        std::string path;
        for (u16 c : final_path)
            path.push_back(char(c & 0xFF));
        for (char& c : path)
            if (c == '\\')
                c = '/';
        const std::string full = (root_ / fs::path(path)).string();
        u32 rc = OK;
#ifdef _WIN32
        /* the Windows branch: the metadata word IS the FILE_ATTRIBUTE set
         * and the timestamps ARE FILETIME bits - the native SDK calls,
         * exactly the production WinDestIo's own apply */
        std::wstring wpath;
        for (u16 c : final_path)
            wpath.push_back(wchar_t(c));
        for (wchar_t& c : wpath)
            if (c == L'\\')
                c = L'/';
        const std::wstring wfull = (root_ / fs::path(wpath)).wstring();
        if (meta.set_attributes) {
            if (!SetFileAttributesW(wfull.c_str(), DWORD(meta.attributes)))
                rc = ERR_WRITE_FAILED;
        }
        if (meta.set_accessed || meta.set_modified) {
            HANDLE h = CreateFileW(wfull.c_str(), FILE_WRITE_ATTRIBUTES,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                                   OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (h == INVALID_HANDLE_VALUE) {
                rc = rc != OK ? rc : u32(ERR_WRITE_FAILED);
            } else {
                const auto to_ft = [](u64 ft) {
                    return FILETIME{DWORD(ft & 0xFFFFFFFFu), DWORD(ft >> 32)};
                };
                FILETIME fa = to_ft(meta.accessed), fm = to_ft(meta.modified);
                if (!SetFileTime(h, nullptr, meta.set_accessed ? &fa : nullptr,
                                 meta.set_modified ? &fm : nullptr))
                    rc = rc != OK ? rc : u32(ERR_WRITE_FAILED);
                CloseHandle(h);
            }
        }
#else
        if (meta.set_attributes) {
            struct stat st;
            if (::stat(full.c_str(), &st) == 0) {
                mode_t mode = st.st_mode & 07777;
                if (meta.attributes & 0x1)
                    mode &= ~mode_t(0200);
                else
                    mode |= 0200;
                if (::chmod(full.c_str(), mode) != 0)
                    rc = ERR_WRITE_FAILED;
            } else
                rc = ERR_WRITE_FAILED;
        }
        if (meta.set_accessed || meta.set_modified) {
            struct timespec ts[2];
            const auto to_ts = [](u64 ft, bool set) {
                struct timespec out;
                if (!set) {
                    out.tv_sec = 0;
                    out.tv_nsec = UTIME_OMIT;
                    return out;
                }
                const long long unix100 = (long long)ft - 116444736000000000LL;
                out.tv_sec = (time_t)(unix100 / 10000000LL);
                out.tv_nsec = (long)((unix100 % 10000000LL) * 100);
                if (out.tv_nsec < 0) {
                    out.tv_sec -= 1;
                    out.tv_nsec += 1000000000L;
                }
                return out;
            };
            ts[0] = to_ts(meta.accessed, meta.set_accessed);
            ts[1] = to_ts(meta.modified, meta.set_modified);
            if (::utimensat(AT_FDCWD, full.c_str(), ts, 0) != 0)
                rc = rc != OK ? rc : u32(ERR_WRITE_FAILED);
        }
#endif
        return rc;
    }

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
        /* GAP-CI-1: MSVC does not resolve "/tmp" - use the real temp dir */
        p = cr_test::tmp_path("cr_wdirs", "");
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

/* ---- S-060: the post-commit metadata restoration (the og's per-item
 * SetFileAttributesW + mask-gated SetFileTime tail, DRWUI :874686-718) ---- */

TEST(WriterDirs, RestoresTimestampsAndAttributes) {
    /* a committed file's DOS attributes (word & ~0x2) and the emitted
     * timestamp trio reach the destination: the og restores both after
     * every item; the core's writer now calls set_metadata post-commit
     * with the per-field enables mirroring what the engine emitted. */
    std::vector<u8> img(1024, 0);
    std::memcpy(img.data(), "F0", 2);
    tree::ResultTree tree;
    auto* found = tree.add_root(u"Found");
    tree::FileProperties* pr = file_props("file0.txt", 0, 2);
    pr->times.created = 130000000000000000ULL; /* 2013-03-09 class */
    pr->times.accessed = 131000000000000000ULL;
    pr->times.modified = 132000000000000000ULL;
    pr->times.changed = 133000000000000000ULL; /* NOT in the og's trio */
    pr->attributes = 0x23u;                    /* R|H|dirty: 0x2 cleared */
    tree::FileNode* n = tree.add_file(found, pr);
    (void)n;
    pr->release();
    tree.refresh_stats();
    MemDevice dev(img);

    TmpDir tmp;
    FsIo io(tmp.p);
    SafeWriter writer(io, &dev, WriterConfig{});
    ASSERT_EQ(OK, writer.enqueue(n, u"out\\"));
    WriterStats stats;
    ASSERT_EQ(OK, writer.process(nullptr, nullptr, nullptr, stats));
    ASSERT_EQ(u64(1), stats.done);

    ASSERT_EQ(size_t(1), io.recorded.size()) << "exactly one metadata call (the file)";
    const auto& rec = io.recorded[0];
    EXPECT_EQ(std::string("out\\file0.txt"), rec.path);
    EXPECT_EQ(130000000000000000ULL, rec.meta.created);
    EXPECT_EQ(131000000000000000ULL, rec.meta.accessed);
    EXPECT_EQ(132000000000000000ULL, rec.meta.modified);
    EXPECT_EQ(0x21u, rec.meta.attributes) << "the og clears bit 0x2 from the word";
    EXPECT_TRUE(rec.meta.set_created);
    EXPECT_TRUE(rec.meta.set_accessed);
    EXPECT_TRUE(rec.meta.set_modified);
    EXPECT_TRUE(rec.meta.set_attributes);
}

TEST(WriterDirs, ZeroTimeSlotsPassAsUnset) {
    /* a zero slot is the og's honest no-date (the N-021/A-014 zero
     * FileTime): the og passes NULL for the un-set fields, and the
     * core's per-field enables mirror exactly which slots the engine
     * emitted - a zero slot is never written as a fake date. */
    std::vector<u8> img(1024, 0);
    std::memcpy(img.data(), "F0", 2);
    tree::ResultTree tree;
    auto* found = tree.add_root(u"Found");
    tree::FileProperties* pr = file_props("onlymod.txt", 0, 2);
    pr->times.modified = 132000000000000000ULL; /* the only emitted slot */
    pr->attributes = 0x20u;                     /* archive bit */
    tree::FileNode* n = tree.add_file(found, pr);
    pr->release();
    tree.refresh_stats();
    MemDevice dev(img);

    TmpDir tmp;
    FsIo io(tmp.p);
    SafeWriter writer(io, &dev, WriterConfig{});
    ASSERT_EQ(OK, writer.enqueue(n, u"out\\"));
    WriterStats stats;
    ASSERT_EQ(OK, writer.process(nullptr, nullptr, nullptr, stats));
    ASSERT_EQ(size_t(1), io.recorded.size());
    const auto& rec = io.recorded[0];
    EXPECT_FALSE(rec.meta.set_created) << "a zero created slot passes as NULL";
    EXPECT_FALSE(rec.meta.set_accessed) << "a zero accessed slot passes as NULL";
    EXPECT_TRUE(rec.meta.set_modified);
    EXPECT_TRUE(rec.meta.set_attributes);
}

TEST(WriterDirs, MetadataFailureIsNonFatal) {
    /* the og traces both restoration failures and CONTINUES - the item
     * is done, the file exists; the core reports the code per row (the
     * report surface tells the truth when the destination refuses). */
    std::vector<u8> img(1024, 0);
    std::memcpy(img.data(), "F0", 2);
    tree::ResultTree tree;
    auto* found = tree.add_root(u"Found");
    tree::FileProperties* pr = file_props("file0.txt", 0, 2);
    pr->times.modified = 132000000000000000ULL;
    pr->attributes = 0x20u;
    tree::FileNode* n = tree.add_file(found, pr);
    pr->release();
    tree.refresh_stats();
    MemDevice dev(img);

    TmpDir tmp;
    FsIo io(tmp.p);
    io.metadata_fail = true; /* the injected refusal */
    SafeWriter writer(io, &dev, WriterConfig{});
    ASSERT_EQ(OK, writer.enqueue(n, u"out\\"));
    WriterStats stats;
    ASSERT_EQ(OK, writer.process(nullptr, nullptr, nullptr, stats));
    EXPECT_EQ(u64(1), stats.done) << "a metadata refusal never fails the item";
    EXPECT_EQ(u64(0), stats.errors);
    std::error_code ec;
    EXPECT_TRUE(fs::exists(tmp.p / "out" / "file0.txt", ec)) << "the file itself committed";
    const auto rows = writer.report_items();
    ASSERT_EQ(size_t(1), rows.size());
    EXPECT_EQ(report::Outcome::Done, rows[0].outcome);
    EXPECT_EQ(
        std::string("failed:0x") +
            [&] {
                char b[16];
                std::snprintf(b, sizeof(b), "%x", u32(ERR_WRITE_FAILED));
                return std::string(b);
            }(),
        rows[0].metadata)
        << "the report row carries the refusal code";
}

TEST(WriterDirs, MetadatalessBackendReportsUnsupported) {
    /* the default DestIo has no metadata surface - the writer still
     * commits (the pre-S-060 behavior) and the report row says
     * "unsupported" truthfully (never "restored"). */
    std::vector<u8> img(1024, 0);
    std::memcpy(img.data(), "F0", 2);
    tree::ResultTree tree;
    auto* found = tree.add_root(u"Found");
    tree::FileProperties* pr = file_props("file0.txt", 0, 2);
    pr->times.modified = 132000000000000000ULL;
    pr->attributes = 0x20u;
    tree::FileNode* n = tree.add_file(found, pr);
    pr->release();
    tree.refresh_stats();
    MemDevice dev(img);

    TmpDir tmp;
    /* the metadata-less backend posture: every op works but the
     * metadata surface refuses (the DestIo default's ERR_UNSUPPORTED,
     * standing in for a backend with no restoration surface) */
    FsIo io(tmp.p);
    io.metadata_unsupported = true;
    SafeWriter writer(io, &dev, WriterConfig{});
    ASSERT_EQ(OK, writer.enqueue(n, u"out\\"));
    WriterStats stats;
    ASSERT_EQ(OK, writer.process(nullptr, nullptr, nullptr, stats));
    EXPECT_EQ(u64(1), stats.done);
    const auto rows = writer.report_items();
    ASSERT_EQ(size_t(1), rows.size());
    EXPECT_EQ(
        std::string("failed:0x") +
            [&] {
                char b[16];
                std::snprintf(b, sizeof(b), "%x", u32(ERR_UNSUPPORTED));
                return std::string(b);
            }(),
        rows[0].metadata);
}

TEST(WriterDirs, InteriorHoleZeroFilledFullSize) {
    /* S-059 end-to-end: a 3 MiB file with a 1 MiB interior sparse hole
     * recovers at FULL logical size - the hole is zeros, the tail data
     * lands at its true offset (not left-shifted), no ERR_TRUNCATED.
     * (Pre-fix: the writer dropped the hole extent and either failed
     * the whole file or shifted the tail 1 MiB early - the finding's
     * falsifiable case.) */
    const u64 CL = 4096; /* one extent = one cluster for simplicity */
    std::vector<u8> img(4ull << 20, 0);
    /* a NONZERO device head (0xEE): a hole extent must NEVER read from
     * the device (offset 0 would emit 0xEE into the hole region) - the
     * og's type-2 chunk writes zeros with no read at all */
    std::memset(img.data(), 0xEE, 1 << 20);
    /* data run 1: [0, 1 MiB) at device 0x100000, pattern 0xAA */
    for (u64 i = 0; i < (1 << 20); i += CL)
        std::memset(img.data() + 0x100000 + i, 0xAA, CL);
    /* data run 2: [2 MiB, 3 MiB) at device 0x200000, pattern 0xBB */
    for (u64 i = 0; i < (1 << 20); i += CL)
        std::memset(img.data() + 0x200000 + i, 0xBB, CL);

    tree::ResultTree tree;
    auto* found = tree.add_root(u"Found");
    /* file_props(name, off, len) pre-populates ONE plain extent - clear
     * it: this fixture builds its own positioned extent list */
    tree::FileProperties* pr = file_props("holed.dat", 0, 3 << 20);
    pr->extents.clear();
    tree::FileProperties::Extent e1{};
    e1.offset = 0x100000;
    e1.length = 1 << 20;
    e1.logical = 0;
    pr->extents.push_back(e1);
    tree::FileProperties::Extent eh{};
    eh.offset = 0;
    eh.length = 1 << 20;
    eh.logical = 1 << 20;
    eh.hole = true;
    pr->extents.push_back(eh);
    tree::FileProperties::Extent e2{};
    e2.offset = 0x200000;
    e2.length = 1 << 20;
    e2.logical = 2 << 20;
    pr->extents.push_back(e2);
    tree::FileNode* n = tree.add_file(found, pr);
    (void)n;
    pr->release();
    tree.refresh_stats();
    MemDevice dev(img);

    TmpDir tmp;
    FsIo io(tmp.p);
    SafeWriter writer(io, &dev, WriterConfig{});
    ASSERT_EQ(OK, writer.enqueue(n, u"out\\"));
    WriterStats stats;
    ASSERT_EQ(OK, writer.process(nullptr, nullptr, nullptr, stats))
        << "the interior hole must not fail the file";
    ASSERT_EQ(u64(1), stats.done);
    /* the full 3 MiB materialized */
    std::error_code ec;
    const fs::path out = tmp.p / "out" / "holed.dat";
    ASSERT_TRUE(fs::exists(out, ec));
    EXPECT_EQ(u64(3) << 20, u64(fs::file_size(out, ec))) << "the FULL logical size";
    /* content: head 0xAA, middle zeros, tail 0xBB at the TRUE offsets */
    std::ifstream f(out, std::ios::binary);
    ASSERT_TRUE(f);
    auto check_span = [&](u64 at, u64 len, u8 want, const char* what) {
        std::vector<u8> buf(static_cast<size_t>(len), 0);
        f.seekg(std::streamoff(at));
        f.read(reinterpret_cast<char*>(buf.data()), std::streamsize(len));
        for (size_t i = 0; i < len; ++i)
            if (buf[i] != want) {
                EXPECT_TRUE(false) << what << ": byte " << i << " is " << std::hex
                                   << unsigned(buf[i]) << " want " << unsigned(want) << std::dec;
                return;
            }
    };
    check_span(0, 1 << 20, 0xAA, "the head data run");
    check_span(1 << 20, 1 << 20, 0x00, "the interior hole (zeros)");
    check_span(2 << 20, 1 << 20, 0xBB, "the tail data run at its TRUE offset");
}

TEST(WriterDirs, LegacyCumulativeExtentsUnchanged) {
    /* S-059 back-compat pin: extents with NO logical stamps and NO
     * holes (old sessions, unstamped engines) take the cumulative walk
     * - byte-identical to the pre-S-059 writer. */
    std::vector<u8> img(4096, 0);
    std::memcpy(img.data(), "AB", 2);
    std::memcpy(img.data() + 512, "CD", 2);
    tree::ResultTree tree;
    auto* found = tree.add_root(u"Found");
    tree::FileProperties* pr = file_props("legacy.dat", 0, 1024);
    tree::FileProperties::Extent a{};
    a.offset = 0;
    a.length = 512;
    pr->extents.push_back(a);
    tree::FileProperties::Extent b{};
    b.offset = 512;
    b.length = 512;
    pr->extents.push_back(b);
    tree::FileNode* n = tree.add_file(found, pr);
    (void)n;
    pr->release();
    tree.refresh_stats();
    MemDevice dev(img);
    TmpDir tmp;
    FsIo io(tmp.p);
    SafeWriter writer(io, &dev, WriterConfig{});
    ASSERT_EQ(OK, writer.enqueue(n, u"out\\"));
    WriterStats stats;
    ASSERT_EQ(OK, writer.process(nullptr, nullptr, nullptr, stats));
    std::ifstream f(tmp.p / "out" / "legacy.dat", std::ios::binary);
    ASSERT_TRUE(f);
    char head[3] = {};
    f.read(head, 2);
    EXPECT_EQ(std::string("AB"), std::string(head, 2)) << "extent 1 at logical 0";
    f.seekg(512);
    f.read(head, 2);
    EXPECT_EQ(std::string("CD"), std::string(head, 2)) << "extent 2 at logical 512 (cumulative)";
}

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
     * renames to the og's "name_1.txt" (N-065: DRWUI %s_%d%s form) even
     * under ConflictPolicy::Skip (intra-queue collisions always rename;
     * the destination is not consulted) */
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
    EXPECT_TRUE(fs::is_regular_file(tmp.p / "out/dup_1.txt"));
}

/* WRITER-1 (AUDIT-PIPELINE, F-A4-8): per-item failures are collected, not
 * fatal - the og's ExportThread continues and reports "%1 files/%2
 * failed" at the end. A 3-item queue with a poisoned middle item must
 * still deliver items 1 and 3 and return OK. */
namespace {
class FaultIo final : public DestIo {
public:
    FaultIo(fs::path root, std::u16string poison)
        : inner_(std::move(root)), poison_(std::move(poison)) {}
    u32 create_temp(u64 id, const std::u16string& final_path,
                    const std::u16string& temp_suffix) override {
        if (final_path.find(poison_) != std::u16string::npos)
            return ERR_DEST_UNWRITABLE; /* injected fault */
        return inner_.create_temp(id, final_path, temp_suffix);
    }
    u32 mkdir(const std::u16string& path) override { return inner_.mkdir(path); }
    u32 write(u64 id, u64 off, const void* data, u32 len) override {
        return inner_.write(id, off, data, len);
    }
    u32 commit(u64 id) override { return inner_.commit(id); }
    void close(u64 id) override { inner_.close(id); }
    bool exists(const std::u16string& path) override { return inner_.exists(path); }

private:
    FsIo inner_;
    std::u16string poison_;
};
} // namespace

TEST(WriterDirs, PerItemErrorCollectedQueueContinues) {
    std::vector<u8> img(2048, 0);
    std::memcpy(img.data(), "A!", 2);
    std::memcpy(img.data() + 512, "B!", 2);
    std::memcpy(img.data() + 1024, "C!", 2);
    tree::ResultTree tree;
    auto* found = tree.add_root(u"Found");
    (void)add_node(tree, found, "a.txt", 0, 2);
    (void)add_node(tree, found, "b.txt", 512, 2); /* poisoned */
    (void)add_node(tree, found, "c.txt", 1024, 2);
    MemDevice dev(img);

    TmpDir tmp;
    FaultIo io(tmp.p, u"b.txt");
    SafeWriter writer(io, &dev, WriterConfig{});
    for (tree::FileNode* c = found->first_child(); c; c = c->next_sibling())
        ASSERT_EQ(OK, writer.enqueue(c, u"out\\"));

    WriterStats stats;
    /* the poisoned item fails but the pass COMPLETES: rc OK, one error
     * collected, the other two files delivered */
    ASSERT_EQ(OK, writer.process(nullptr, nullptr, nullptr, stats));
    EXPECT_EQ(u64(1), stats.errors);
    EXPECT_EQ(u64(2), stats.done);
    const std::set<std::string> got = walk_tree(tmp.p);
    EXPECT_TRUE(got.count("f:out/a.txt"));
    EXPECT_TRUE(got.count("f:out/c.txt"));
    EXPECT_FALSE(got.count("f:out/b.txt"));
}

/* ---- GAP-S6-7: metadata-only items skip, never zero-fill ----
 *
 * Bench 40073093997 permanent_delete: 991 MetadataOnly 'Lost' items
 * (remnant/orphan reconstruction, no extents) were enqueued by
 * recover-all and committed as FULL-SIZE ZERO files - false positives
 * against the manifest. The zero-fill Skip policy exists for PARTIAL
 * spans of real files; a whole no-extent item has nothing to recover
 * (the og emits no content for metadata-only entries). */

TEST(WriterDirs, MetadataOnlyItemSkipsNotZeroFills) {
    TmpDir tmp;
    FsIo io(tmp.p);
    MemDevice src(std::vector<u8>(64 * 1024, 0));
    SafeWriter writer(io, &src, WriterConfig{});
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Found");
    auto* meta = tree.add_root(u"Lost");
    /* a metadata-only node: positive size, NO extents, NO resident data */
    tree::FileProperties* p1 = new tree::FileProperties();
    p1->name = u"ghost.txt";
    p1->logical_size = 4096;
    tree::FileNode* ghost = tree.add_file(meta, p1);
    p1->release();
    ASSERT_NE(ghost, nullptr);
    /* a real content node: resident data (recoverable) */
    tree::FileProperties* p2 = new tree::FileProperties();
    p2->name = u"real.txt";
    p2->logical_size = 4;
    p2->resident_data = {0x41, 0x42, 0x43, 0x44};
    tree::FileNode* real = tree.add_file(root, p2);
    p2->release();
    ASSERT_NE(real, nullptr);
    /* an empty file: size 0 is REAL content - must still commit */
    tree::FileProperties* p3 = new tree::FileProperties();
    p3->name = u"empty.txt";
    p3->logical_size = 0;
    tree::FileNode* empty = tree.add_file(root, p3);
    p3->release();
    ASSERT_NE(empty, nullptr);

    ASSERT_EQ(writer.enqueue(ghost, u""), OK);
    ASSERT_EQ(writer.enqueue(real, u""), OK);
    ASSERT_EQ(writer.enqueue(empty, u""), OK);

    WriterStats stats;
    ASSERT_EQ(writer.process(nullptr, nullptr, nullptr, stats), OK);
    EXPECT_EQ(stats.nothing_to_recover, 1u);
    EXPECT_EQ(stats.done, 2u); /* real + empty committed */
    /* NO zero-filled ghost file on disk */
    EXPECT_FALSE(fs::exists(tmp.p / FsIo::norm(u"ghost.txt")))
        << "metadata-only items must not materialize as zero files";
    EXPECT_TRUE(fs::exists(tmp.p / FsIo::norm(u"real.txt")));
    EXPECT_TRUE(fs::exists(tmp.p / FsIo::norm(u"empty.txt")));

    /* the report tells the truth: outcome nothing_to_recover + totals */
    const auto items = writer.report_items();
    ASSERT_EQ(items.size(), 3u);
    EXPECT_EQ(items[0].outcome, report::Outcome::NothingToRecover);
    EXPECT_EQ(items[0].bytes, 0u);
    EXPECT_EQ(items[1].outcome, report::Outcome::Done);
    EXPECT_EQ(items[2].outcome, report::Outcome::Done);
    report::Totals t = writer.report_totals("src", "dst", 0, stats);
    EXPECT_EQ(t.nothing_to_recover, 1u);
    /* reconciliation: queued == done + refused + errors + resumed +
     * nothing_to_recover + pending */
    EXPECT_EQ(t.queued, t.done + t.refused + t.errors + t.resumed + t.nothing_to_recover +
                            t.pending + t.dirs);
    /* the JSON round-trips the new outcome + totals field (validator) */
    const std::string doc = report::build_report(items, t);
    EXPECT_NE(doc.find("nothing_to_recover"), std::string::npos);
}

TEST(WriterDirs, ProgressExcludesErrorItems) {
    /* N-009b (diff-to-fix): progress honesty - an item that ends in Error
     * never counts toward done_bytes (the og's ExportThread counts
     * WRITTEN bytes). The old unconditional add reached done == total on
     * error-heavy runs - "100% done, N errors". */
    std::vector<u8> img(2048, 0);
    std::memcpy(img.data(), "A!", 2);
    std::memcpy(img.data() + 512, "B!", 2);
    std::memcpy(img.data() + 1024, "C!", 2);
    tree::ResultTree tree;
    auto* found = tree.add_root(u"Found");
    (void)add_node(tree, found, "a.txt", 0, 2);
    (void)add_node(tree, found, "b.txt", 512, 2); /* poisoned */
    (void)add_node(tree, found, "c.txt", 1024, 2);
    MemDevice dev(img);

    TmpDir tmp;
    FaultIo io(tmp.p, u"b.txt");
    SafeWriter writer(io, &dev, WriterConfig{});
    for (tree::FileNode* c = found->first_child(); c; c = c->next_sibling())
        ASSERT_EQ(OK, writer.enqueue(c, u"out\\"));

    auto progress = [](u64 done, u64 total, void* user) -> bool {
        auto* ctx = static_cast<std::pair<u64, u64>*>(user);
        ctx->first = std::max(ctx->first, done);
        ctx->second = total;
        return true;
    };
    std::pair<u64, u64> ctx{0, 0};
    WriterStats stats;
    ASSERT_EQ(OK, writer.process(nullptr, progress, &ctx, stats));
    EXPECT_EQ(u64(1), stats.errors);
    EXPECT_EQ(u64(2), stats.done);
    EXPECT_EQ(ctx.first, 4u) << "done must reach only the two written items' bytes (2+2)";
    EXPECT_EQ(ctx.second, 6u) << "total still counts every queued item";
}

/* ---- N-065: the og's collision policy (DRWUI :357116-357131) ---------- */

TEST(WriterDirs, CollisionNameUnderscoreFormat) {
    /* the og renames to name_1.ext / name_2.ext (%s_%d%s, counter from
     * 1) - NOT the old "name (N).ext" space-parens form. Three same-name
     * nodes in one directory: the original + two og-form renames. */
    std::vector<u8> img(512, 0);
    std::memcpy(img.data(), "AA", 2);
    MemDevice dev(img);
    tree::ResultTree tree;
    auto* found = tree.add_root(u"Found");
    (void)add_node(tree, found, "rep.docx", 0, 2);
    (void)add_node(tree, found, "rep.docx", 0, 2);
    (void)add_node(tree, found, "rep.docx", 0, 2);
    tree.refresh_stats();

    TmpDir tmp;
    FsIo io(tmp.p);
    WriterConfig c;
    SafeWriter writer(io, &dev, c);
    tree::FileNode* n = found->first_child();
    ASSERT_NE(nullptr, n);
    for (int k = 0; k < 3; ++k) {
        ASSERT_EQ(OK, writer.enqueue(n, u"out\\"));
        n = n->next_sibling();
    }
    WriterStats stats;
    ASSERT_EQ(OK, writer.process(nullptr, nullptr, nullptr, stats));
    EXPECT_EQ(u64(3), stats.done);
    EXPECT_TRUE(fs::is_regular_file(tmp.p / "out/rep.docx"));
    EXPECT_TRUE(fs::is_regular_file(tmp.p / "out/rep_1.docx")) << "the og's %s_%d%s form";
    EXPECT_TRUE(fs::is_regular_file(tmp.p / "out/rep_2.docx"));
    /* extensionless names take the og's %s_%d branch */
    {
        tree::ResultTree t2;
        auto* f2 = t2.add_root(u"Found");
        (void)add_node(t2, f2, "README", 0, 2);
        (void)add_node(t2, f2, "README", 0, 2);
        t2.refresh_stats();
        TmpDir tmp2;
        FsIo io2(tmp2.p);
        SafeWriter w2(io2, &dev, c);
        tree::FileNode* r = f2->first_child();
        ASSERT_EQ(OK, w2.enqueue(r, u"out\\"));
        r = r->next_sibling();
        ASSERT_EQ(OK, w2.enqueue(r, u"out\\"));
        WriterStats st2;
        ASSERT_EQ(OK, w2.process(nullptr, nullptr, nullptr, st2));
        EXPECT_TRUE(fs::is_regular_file(tmp2.p / "out/README"));
        EXPECT_TRUE(fs::is_regular_file(tmp2.p / "out/README_1")) << "the %s_%d branch";
    }
    /* the default cap is the og's 10,000 (was 64) */
    EXPECT_EQ(10000u, WriterConfig{}.rename_attempts);
}

TEST(WriterDirs, CollisionExhaustionFailsTheItem) {
    /* the og's cap-break (DRWUI :357131 `if (10000 < counter) break`):
     * when every rename candidate is taken the file is FAILED - never a
     * synthesized "_dup" name (the old core's og-foreign fallback, which
     * could itself collide). Small cap + a fully-colliding destination
     * proves the semantics without 10,000 files. */
    std::vector<u8> img(512, 0);
    std::memcpy(img.data(), "AA", 2);
    MemDevice dev(img);
    tree::ResultTree tree;
    auto* found = tree.add_root(u"Found");
    (void)add_node(tree, found, "rep.txt", 0, 2);
    tree.refresh_stats();

    TmpDir tmp;
    FsIo io(tmp.p);
    WriterConfig c;
    c.rename_attempts = 2; /* name_1 and name_2 both pre-exist */
    fs::create_directories(tmp.p / "out");
    for (const char* pre : {"rep.txt", "rep_1.txt", "rep_2.txt"})
        std::ofstream(tmp.p / "out" / pre).put('x');
    SafeWriter writer(io, &dev, c);
    tree::FileNode* n = found->first_child();
    ASSERT_NE(nullptr, n);
    ASSERT_EQ(OK, writer.enqueue(n, u"out\\"));

    WriterStats stats;
    ASSERT_EQ(OK, writer.process(nullptr, nullptr, nullptr, stats));
    EXPECT_EQ(u64(1), stats.errors) << "the exhausted item FAILS (og cap-break)";
    EXPECT_EQ(u64(0), stats.done);
    /* no _dup file was ever synthesized */
    bool saw_dup = false;
    for (const auto& rel : walk_tree(tmp.p))
        if (rel.find("_dup") != std::string::npos)
            saw_dup = true;
    EXPECT_FALSE(saw_dup) << "the og never produces a _dup name";
    /* the report row carries the honest reason */
    const auto rows = writer.report_items();
    ASSERT_EQ(1u, rows.size());
    EXPECT_EQ(report::Outcome::Error, rows[0].outcome);
    EXPECT_EQ(u32(ERR_DEST_NAME_EXHAUSTED), rows[0].reason);
}

TEST(WriterDirs, MkdirSemanticsMatchShCreateDirectoryEx) {
    /* N-065's mkdir half, pinned with the close-reading classification:
     * the og's SHCreateDirectoryExW gate (accept 0 / 0xB7 - DRWUI
     * :870436-438) is SEMANTICALLY the exist-as-directory contract, not a
     * narrower error filter: the API itself returns ERROR_ALREADY_EXISTS
     * for pre-existing paths (that is WHY the og accepts 0xB7), so "dir
     * pre-exists -> proceed" is og parity on both sides. The finding's
     * ACCESS_DENIED premise mis-models the API (a pre-existing directory
     * yields 0xB7 there, which the og ACCEPTS). Pins: (a) an existing
     * directory component proceeds; (b) a FILE in the way fails the
     * item (the true divergence-free strictness); both sides refuse. */
    std::vector<u8> img(512, 0);
    MemDevice dev(img);
    tree::ResultTree tree;
    auto* found = tree.add_root(u"Found");
    (void)add_node(tree, found, "a", 0, 0, true); /* the mkdir carrier */
    tree.refresh_stats();

    TmpDir tmp;
    FsIo io(tmp.p);
    WriterConfig c;
    /* (a) pre-existing directory chain: proceeds (the 0xB7 contract) */
    fs::create_directories(tmp.p / "dest/a");
    SafeWriter w1(io, &dev, c);
    ASSERT_EQ(OK, w1.enqueue(found->first_child(), u"dest\\"));
    WriterStats s1;
    EXPECT_EQ(OK, w1.process(nullptr, nullptr, nullptr, s1));
    EXPECT_EQ(u64(1), s1.dirs) << "existing directory = the og's accepted 0xB7";

    /* (b) mkdir FAILURE propagates to the item (both sides refuse hard
     * mkdir errors: SHCreateDirectoryExW accepts only 0/0xB7, WinDestIo
     * refuses non-directory obstructions - pinned on the MSVC lane,
     * test_destio_win.cpp:65 - and the writer must FAIL the item, not
     * proceed). Proven through a fault shim: mkdir refuses the chain. */
    TmpDir tmp2;
    /* a minimal delegating shim: everything through FsIo, mkdir refused
     * (FsIo is final; the wrapper implements DestIo directly) */
    class RefusingMkdirIo final : public DestIo {
    public:
        explicit RefusingMkdirIo(FsIo& inner) : inner_(inner) {}
        u32 create_temp(u64 id, const std::u16string& fin, const std::u16string& suf) override {
            return inner_.create_temp(id, fin, suf);
        }
        u32 mkdir(const std::u16string&) override { return ERR_DEST_UNWRITABLE; }
        u32 write(u64 id, u64 off, const void* data, u32 len) override {
            return inner_.write(id, off, data, len);
        }
        u32 commit(u64 id) override { return inner_.commit(id); }
        void close(u64 id) override { inner_.close(id); }
        bool exists(const std::u16string& p) override { return inner_.exists(p); }

    private:
        FsIo& inner_;
    };
    FsIo io2(tmp2.p);
    RefusingMkdirIo rio(io2);
    SafeWriter w2(rio, &dev, c);
    ASSERT_EQ(OK, w2.enqueue(found->first_child(), u"dest2\\"));
    WriterStats s2;
    EXPECT_EQ(OK, w2.process(nullptr, nullptr, nullptr, s2)); /* collected, not fatal */
    EXPECT_GE(s2.errors, u64(1)) << "a hard mkdir failure fails the item on both sides";
}
