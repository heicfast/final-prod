/* Orchestrator + safe recovery writer tests. */
#include <gtest/gtest.h>
#include "cr/orchestrator/cr_scan.h"
#include "cr/recovery/cr_safewriter.h"
#include "cr/session/cr_crsf.h"
#include "cr/storage/cr_device.h"
#include <cstring>
#include <vector>

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
    u32 read_at(u64 off, void *buf, u32 len) override {
        if (off > data_.size() || len > data_.size() - off) return ERR_RANGE_OUT_OF_BOUNDS;
        std::memcpy(buf, data_.data() + off, len);
        return OK;
    }
    const Geometry &geometry() const override { return geom_; }
    std::vector<u8> &raw() { return data_; }

private:
    std::vector<u8> data_;
    Geometry geom_;
};

void put16(std::vector<u8> &v, size_t at, u16 x) {
    v[at] = u8(x); v[at + 1] = u8(x >> 8);
}
void put32v(std::vector<u8> &v, size_t at, u32 x) {
    v[at] = u8(x); v[at + 1] = u8(x >> 8); v[at + 2] = u8(x >> 16); v[at + 3] = u8(x >> 24);
}

/* minimal FAT16 volume: 5074 sectors, clusters 5(root),10(file),20(file) */
std::vector<u8> build_fat16() {
    std::vector<u8> img(u64(5074) * 512, 0);
    u8 *b = img.data();
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
    b[510] = 0x55; b[511] = 0xAA;

    const size_t FAT1 = 2 * 512, FAT2 = 22 * 512, ROOT = 42 * 512, DATA = 74 * 512;
    put16(img, FAT1 + 5 * 2, 0xFFFF);  put16(img, FAT2 + 5 * 2, 0xFFFF);   /* root dir */
    put16(img, FAT1 + 10 * 2, 11);      put16(img, FAT2 + 10 * 2, 11);
    put16(img, FAT1 + 11 * 2, 12);      put16(img, FAT2 + 11 * 2, 12);
    put16(img, FAT1 + 12 * 2, 0xFFFF);  put16(img, FAT2 + 12 * 2, 0xFFFF); /* doc */
    put16(img, FAT1 + 20 * 2, 0xFFFF);  put16(img, FAT2 + 20 * 2, 0xFFFF); /* del file */

    u8 *root = img.data() + ROOT;
    /* dentry writer: name 11 chars + attrs + cluster + size */
    auto dentry = [&](size_t slot, const char *n11, u8 attr, u32 cl, u32 sz, bool del) {
        u8 *e = root + slot * 32;
        std::memset(e, 0, 32);
        for (int i = 0; i < 11; ++i) e[i] = u8(n11[i]);
        if (del) e[0] = 0xE5;
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
    std::map<u64, std::vector<u8>> files; /* item_id -> content */
    std::map<u64, std::u16string> finals; /* item_id -> final path */
    std::set<std::u16string> existing;    /* for exists() */

    u32 create_temp(u64 id, const std::u16string &final_path) override {
        created.push_back(final_path);
        finals[id] = final_path;
        files[id] = {};
        return 0;
    }
    u32 write(u64 id, u64 off, const void *data, u32 len) override {
        auto &f = files[id];
        if (f.size() < off + len) f.resize(size_t(off + len));
        std::memcpy(f.data() + off, data, len);
        return 0;
    }
    u32 commit(u64 id) override {
        committed.push_back(finals[id]);
        existing.insert(finals[id]);
        return 0;
    }
    void close(u64) override {}
    bool exists(const std::u16string &p) override { return existing.count(p) != 0; }

    std::string content_of(u64 id) const {
        auto it = files.find(id);
        if (it == files.end()) return {};
        return std::string(it->second.begin(), it->second.end());
    }
};

std::u16string u16_(const char *s) {
    std::u16string o;
    for (const char *p = s; *p; ++p) o.push_back(u16(u8(*p)));
    return o;
}

} // namespace

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
    u8 *b = dev.raw().data();
    std::memcpy(b + 3, "NTFS    ", 8);
    EXPECT_EQ(ScanController::probe_family(&dev), FsFamily::Ntfs);
    std::memcpy(b + 3, "EXFAT   ", 8);
    EXPECT_EQ(ScanController::probe_family(&dev), FsFamily::Exfat);
    std::memset(b + 3, 'X', 8);
    /* odd OEM but 55AA + sane BPB still probes FAT (heuristic hint);
     * destroy the signature for Unknown */
    EXPECT_EQ(ScanController::probe_family(&dev), FsFamily::Fat);
    b[510] = 0x00;
    EXPECT_EQ(ScanController::probe_family(&dev), FsFamily::Unknown);
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
    tree::FileNode *doc = nullptr;
    std::function<void(tree::FileNode *)> collect = [&](tree::FileNode *n) {
        for (tree::FileNode *c = n->first_child(); c; c = c->next_sibling()) {
            if (c->name_utf8() == "DOC.TXT") doc = c;
            collect(c);
        }
    };
    for (auto *r : tree.roots()) collect(r);
    ASSERT_NE(doc, nullptr);

    /* session round-trip */
    struct MemFile {
        std::vector<u8> data;
        u64 pos = 0;
        static u32 wr(void *u, const void *b, u32 len) {
            auto *m = static_cast<MemFile *>(u);
            if (m->pos + len > m->data.size()) m->data.resize(size_t(m->pos + len));
            std::memcpy(m->data.data() + m->pos, b, len);
            m->pos += len;
            return 0;
        }
        static u64 tl(void *u) { return static_cast<MemFile *>(u)->pos; }
        static u32 sk(void *u, u64 abs) {
            static_cast<MemFile *>(u)->pos = abs;
            return 0;
        }
        static u32 rd(void *u, void *b, u32 len) {
            auto *m = static_cast<MemFile *>(u);
            if (m->pos + len > m->data.size()) return 1;
            std::memcpy(b, m->data.data() + m->pos, len);
            m->pos += len;
            return 0;
        }
        static u64 sz(void *u) { return static_cast<MemFile *>(u)->data.size(); }
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
    tree::FileNode *doc2 = nullptr;
    std::function<void(tree::FileNode *)> collect2 = [&](tree::FileNode *n) {
        for (tree::FileNode *c = n->first_child(); c; c = c->next_sibling()) {
            if (c->name_utf8() == "DOC.TXT") doc2 = c;
            collect2(c);
        }
    };
    for (auto *r2 : tree2.roots()) collect2(r2);
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
    tree::FileNode *doc = nullptr;
    std::function<void(tree::FileNode *)> collect = [&](tree::FileNode *n) {
        for (tree::FileNode *c = n->first_child(); c; c = c->next_sibling()) {
            if (c->name_utf8() == "DOC.TXT") doc = c;
            collect(c);
        }
    };
    for (auto *r : tree.roots()) collect(r);
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

TEST(SafeWriter, CollisionAndSanitize) {
    MockIo io;
    std::vector<u8> img(64 * 512, 0);
    MemDevice dev(std::move(img));
    SafeWriter writer(io, &dev, WriterConfig{});

    io.existing.insert(u16_("R:\\out\\photo.jpg"));
    std::u16string n1 = writer.resolve_collision(u16_("R:\\out"), u16_("photo.jpg"));
    EXPECT_EQ(n1, u16_("photo (2).jpg"));

    /* sanitize: forbidden chars replaced (corpus rule) */
    EXPECT_EQ(SafeWriter::sanitize_name(u16_("a<b>:?/|*.txt")), u16_("a_b______.txt"));
    EXPECT_EQ(SafeWriter::sanitize_name(u16_("..")), u16_("_.."));
    EXPECT_EQ(SafeWriter::sanitize_name(u16_("")), u16_("_"));
}
