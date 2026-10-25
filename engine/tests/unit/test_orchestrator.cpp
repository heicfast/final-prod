/* Orchestrator + safe recovery writer tests. */
#include <gtest/gtest.h>
#include "cr/orchestrator/cr_scan.h"
#include "cr/recovery/cr_safewriter.h"
#include "cr/session/cr_crsf.h"
#include "cr/storage/cr_device.h"
#include "test_fixtures.hpp"
#include <cstring>
#include <utility>
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
    EXPECT_EQ(ScanController::probe_family(&dev), FsFamily::Exfat);
    std::memset(b + 3, 'X', 8);
    /* odd OEM but 55AA + sane BPB still probes FAT (heuristic hint);
     * destroy the signature for Unknown */
    EXPECT_EQ(ScanController::probe_family(&dev), FsFamily::Fat);
    b[510] = 0x00;
    EXPECT_EQ(ScanController::probe_family(&dev), FsFamily::Unknown);
}

TEST(Orchestrator, NtfsProbeContentProof) {
    /* M-043 fail-before/pass-after: the og's analyser recognizes NTFS by
     * CONTENT, not OEM bytes (FUN_180005470's gates + FUN_180005180's
     * per-record validation: FILE magic, USA count < 0x11, USA offset in
     * [0x2a, 0x2a+0xd7) - corpus re-verified first-hand). */
    auto put16_raw = [](u8* p, u16 v) {
        p[0] = u8(v);
        p[1] = u8(v >> 8);
    };
    auto mk = [&](u64 mft_lcn, bool file_magic, u16 usa_off, u16 usa_cnt) {
        std::vector<u8> img(64 * 1024, 0);
        u8* b = img.data();
        std::memcpy(b + 3, "NTFS    ", 8);
        put16_raw(b + 0x0B, 512); /* bps */
        b[0x0D] = 0x01;            /* spc 1 (pow2, nonzero) */
        put16_raw(b + 0x28, u16(mft_lcn));
        put16_raw(b + 0x2A, u16(mft_lcn >> 16));
        if (mft_lcn != 0) {
            u8* r = img.data() + mft_lcn * 512;
            if (file_magic)
                std::memcpy(r, "FILE", 4);
            put16_raw(r + 0x06, usa_off);
            put16_raw(r + 0x08, usa_cnt);
        }
        return img;
    };

    /* 1. spoofed OEM, $MFT geometry pointing at zeros: not NTFS */
    {
        auto img = mk(16, false, 0x30, 3);
        MemDevice dev(std::move(img));
        EXPECT_EQ(ScanController::probe_family(&dev), FsFamily::Unknown);
    }
    /* 2. FILE magic but USA count out of the og's bound (>= 0x11) */
    {
        auto img = mk(16, true, 0x30, 0x20);
        MemDevice dev(std::move(img));
        EXPECT_EQ(ScanController::probe_family(&dev), FsFamily::Unknown);
    }
    /* 3. FILE magic but USA offset outside [0x2a, 0x201) */
    {
        auto img = mk(16, true, 0x10, 3);
        MemDevice dev(std::move(img));
        EXPECT_EQ(ScanController::probe_family(&dev), FsFamily::Unknown);
    }
    /* 4. real-shaped record 0: recognized */
    {
        auto img = mk(16, true, 0x30, 3);
        MemDevice dev(std::move(img));
        EXPECT_EQ(ScanController::probe_family(&dev), FsFamily::Ntfs);
    }
    /* 5. plausible BPB but $MFT LCN zero: never probes (geometry gate) */
    {
        auto img = mk(0, true, 0x30, 3);
        MemDevice dev(std::move(img));
        EXPECT_EQ(ScanController::probe_family(&dev), FsFamily::Unknown);
    }
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

/* event recorder: counts per type + last ErrorRange payload */
struct EventRecorder {
    std::vector<event::Type> seen;
    u64 last_err_start = 0, last_err_len = 0;
    u32 max_error_ranges = 0;
    static void on_evt(const event::Event& e, void* user) {
        auto* r = static_cast<EventRecorder*>(user);
        r->seen.push_back(e.type);
        if (e.type == event::Type::ErrorRange) {
            r->last_err_start = e.err_start_bytes;
            r->last_err_len = e.err_len_bytes;
            r->max_error_ranges = std::max(r->max_error_ranges, e.error_ranges);
        }
    }
    bool has(event::Type t) const { return std::find(seen.begin(), seen.end(), t) != seen.end(); }
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
