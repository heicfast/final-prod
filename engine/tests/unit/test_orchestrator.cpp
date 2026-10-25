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
    pvd[0] = 1;  /* DescriptorTag.TagIdentifier = 1 (PVD) */
    pvd[1] = 1;  /* DescriptorVersion */
    pvd[0x10] = 0x0B; /* VolumeIdentifier dstring: 11 chars */
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
    b[2] = 0x90;                     /* FAT jump code */
    std::memcpy(b + 3, "MSDOS5.0", 8); /* OEM */
    put16(img, 0x0B, 512);           /* pow2 bps */
    b[0x10] = 2;                     /* numFATs (M-045's gate) */
    b[510] = 0x55;
    b[511] = 0xAA;
    MemDevice dev(std::move(img));
    EXPECT_EQ(ScanController::probe_family(&dev), FsFamily::Fat);
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
    session::Reader r(+[](void* u, void* b, u32 len) -> u32 {
        auto* m = static_cast<MemFile*>(u);
        if (m->pos + len > m->data.size())
            return 1;
        std::memcpy(b, m->data.data() + m->pos, len);
        m->pos += len;
        return 0;
    }, MemFile::sk, +[](void* u) -> u64 { return static_cast<MemFile*>(u)->data.size(); }, &mf);
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
