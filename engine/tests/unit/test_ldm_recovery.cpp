/* test_ldm_recovery.cpp - S-052 acceptance: the LDM basis-aware recovery.
 *
 * The scan reads dynamic volumes through table::LdmVolumeDevice; the
 * finding's defect was that the device DIED with the scan loop while the
 * tree nodes kept LAYOUT-RELATIVE extents - every recovery consumer then
 * read those offsets against a single physical disk (silently wrong
 * bytes for any multi-disk layout). This file proves the closed loop:
 * the tree carries per-root basis ids, the LdmBasisTable routes the
 * SafeWriter's reads through the retained virtual devices, the overlap
 * refusal compares PHYSICAL spans, and the layouts + basis stamps
 * survive the session file boundary.
 *
 * Licensed under the MIT license. See LICENSE.
 */
#include <gtest/gtest.h>
#include "test_fixtures.hpp"
#include "test_tmpdir.hpp"
#include "cr/recovery/cr_safewriter.h"
#include "cr/preview/cr_preview.h"
#include "cr/table/cr_ldm.h"
#include "cr/tree/cr_node.h"
#include "cr/session/cr_records.h"
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <string>
#include <vector>

using namespace cr;
using namespace cr::table;
using namespace cr::recovery;
using namespace cr::testfixture;
namespace fs = std::filesystem;

namespace {

struct TmpDir {
    fs::path path;
    TmpDir(const char* prefix) {
        path = cr_test::tmp_path(prefix, "");
        fs::create_directories(path);
    }
    ~TmpDir() {
        std::error_code ec;
        fs::remove_all(path, ec);
    }
};

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

/* filesystem-backed DestIo (the test_writer_dirs pattern) */
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
        fs::rename(it->second.temp, it->second.final_, ec);
        return ec ? u32(ERR_WRITE_FAILED) : u32(OK);
    }
    void close(u64 id) override { open_.erase(id); }
    u64 free_bytes(const std::u16string&) const override { return 1ull << 30; }
    bool exists(const std::u16string& path) override {
        std::error_code ec;
        return fs::exists(root_ / norm(path), ec);
    }

private:
    fs::path root_;
    struct Open {
        fs::path temp;
        fs::path final_;
    };
    std::map<u64, Open> open_;
};

/* ---- compact multi-disk fixture builders (test_ldm.cpp's shapes) ---- */

constexpr u64 SPAN_PART_START = 2048;
constexpr u64 SPAN_PART_SIZE = 4096; /* 2 MiB per member */

const char* SPAN_GUIDS[2] = {"01020304-0506-0708-090A-0B0C0D0E0F10",
                             "11121314-1516-1718-191A-1B1C1D1E1F20"};

std::vector<std::vector<u8>> build_spanned_disks() {
    std::vector<std::vector<u8>> imgs;
    for (u32 i = 0; i < 2; i++) {
        LdmFixture fx(16384);
        fx.set_disk(1000 + i, SPAN_GUIDS[i]);
        fx.add_dgr4(1, "TestPCDg0");
        fx.add_dsk4(1000, "Disk1", SPAN_GUIDS[0]);
        fx.add_dsk4(1001, "Disk2", SPAN_GUIDS[1]);
        fx.add_volume({2000, "Span1", 3, SPAN_PART_SIZE * 2, 1});
        fx.add_component({3000, "Span1-01", LDM_COMP_CONCAT, 2, 2000, 0, 0});
        fx.add_partition({4000, "Disk1-01", SPAN_PART_START, SPAN_PART_SIZE, 0, 3000, 0, 1000});
        fx.add_partition(
            {4001, "Disk2-01", SPAN_PART_START, SPAN_PART_SIZE, SPAN_PART_SIZE, 3000, 1, 1001});
        fx.finish();
        imgs.push_back(fx.take());
    }
    std::memset(imgs[0].data() + SPAN_PART_START * 512, 0x11, size_t(SPAN_PART_SIZE * 512));
    std::memset(imgs[1].data() + SPAN_PART_START * 512, 0x22, size_t(SPAN_PART_SIZE * 512));
    return imgs;
}

/* one file node under a basis-stamped root */
tree::FileNode* plant_file(tree::ResultTree& tree, tree::FullNode* root, const char* name16,
                           u64 ext_off, u64 ext_len) {
    auto* p = new tree::FileProperties();
    for (const char* q = name16; *q; ++q)
        p->name.push_back(u16(u8(*q)));
    p->logical_size = ext_len;
    tree::FileProperties::Extent e{ext_off, ext_len, 0, 0};
    p->extents.push_back(e);
    tree::FileNode* n = tree.add_file(root, p);
    p->release();
    return n;
}

std::vector<u8> read_out(const fs::path& p) {
    std::ifstream f(p, std::ios::binary);
    return std::vector<u8>((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

const LdmVolumeLayout* find_layout(const LdmInfo& info, u64 vol_obj_id) {
    for (const auto& l : info.layouts)
        if (l.vol_obj_id == vol_obj_id)
            return &l;
    return nullptr;
}

} // namespace

/* THE finding falsifiable: a file whose extents live on DISK 2 of a
 * spanned volume recovers byte-exact through the basis table. The
 * embedded fail-before: the SAME extent read against disk 1 (the
 * absolute source - exactly what the pre-fix writer did) serves 0x11
 * bytes, proving the old path produced silently wrong content. */
TEST(LdmRecovery, SpanRecoveryReadsThroughVirtualBasis) {
    TmpDir tmp("s052_span");
    auto imgs = build_spanned_disks();
    MemDevice d0(std::move(imgs[0]));
    MemDevice d1(std::move(imgs[1]));
    LdmInfo info;
    ASSERT_EQ(OK, parse_ldm_database(&d0, info));
    const LdmVolumeLayout* lay = find_layout(info, 2000);
    ASSERT_NE(nullptr, lay);
    ASSERT_EQ(LdmVolumeKind::Spanned, lay->kind);

    /* the basis registry: the scan's shape (device 1 = basis id 1) */
    auto vol = std::make_unique<LdmVolumeDevice>(*lay, [&](const LdmGuid& g) -> IStorageDevice* {
        if (g == info.disks[0].guid)
            return &d0;
        if (g == info.disks[1].guid)
            return &d1;
        return nullptr;
    });
    LdmBasisTable basis;
    basis.add(vol.get());

    /* the tree: a basis-1 root with a file whose extent lives on disk 2
     * (volume offset >= the first member's 2 MiB) */
    tree::ResultTree tree;
    tree::FullNode* root = tree.add_root(u"Dynamic Volume Span1");
    ASSERT_NE(nullptr, root);
    tree.set_root_basis(root, 1);
    const u64 ext_off = SPAN_PART_SIZE * 512 + 512; /* disk-2 region */
    const u64 ext_len = 8192;
    ASSERT_NE(nullptr, plant_file(tree, root, "REPORT.DAT", ext_off, ext_len));

    /* the FAIL-BEFORE half, executed: the pre-fix writer read this
     * extent against the single physical scan disk (d0) - those bytes
     * are 0x11, NOT the volume's 0x22 content. Wrong by construction. */
    {
        std::vector<u8> old_path(16, 0);
        ASSERT_EQ(OK, d0.read_at(ext_off, old_path.data(), u32(old_path.size())));
        for (u8 b : old_path)
            ASSERT_EQ(0x11, b); /* what the pre-fix recovery served */
        std::vector<u8> vol_bytes(16, 0);
        ASSERT_EQ(OK, vol->read_at(ext_off, vol_bytes.data(), u32(vol_bytes.size())));
        for (u8 b : vol_bytes)
            ASSERT_EQ(0x22, b); /* what the scan saw - and only the basis
                                 * routing now serves */
    }

    FsIo io(tmp.path);
    WriterConfig cfg;
    cfg.verify_hash = false; /* content pin below is the proof */
    SafeWriter w(io, &d0 /* the absolute source: disk 1 */, cfg, &basis);
    ASSERT_EQ(OK, w.enqueue(tree.roots()[0]->first_child(), u"out"));
    WriterStats stats;
    ASSERT_EQ(OK, w.process(nullptr, nullptr, nullptr, stats));
    ASSERT_EQ(1u, stats.done);
    const std::vector<u8> out = read_out(tmp.path / "out" / "REPORT.DAT");
    ASSERT_EQ(size_t(ext_len), out.size());
    for (u8 b : out)
        EXPECT_EQ(0x22, b) << "recovered bytes must be the disk-2 content";
}

/* the striped round-trip: a multi-chunk extent recovers through the
 * interleave (chunk k -> column k%2, row k/2 on the single fixture disk) */
TEST(LdmRecovery, StripeRecoveryRoundTrips) {
    TmpDir tmp("s052_stripe");
    StripeDisk sd;
    auto img = sd.fx.take();
    for (u64 row = 0; row < STRIPE_MEMBER_SIZE / CHUNK; row++) {
        std::memset(img.data() + (STRIPE_A_START + row * CHUNK) * 512, int(0xA0 + (row & 0x0F)),
                    size_t(CHUNK * 512));
        std::memset(img.data() + (STRIPE_B_START + row * CHUNK) * 512, int(0xB0 + (row & 0x0F)),
                    size_t(CHUNK * 512));
    }
    MemDevice dev(std::move(img));
    LdmInfo info;
    ASSERT_EQ(OK, parse_ldm_database(&dev, info));
    const LdmVolumeLayout* lay = find_layout(info, 2001);
    ASSERT_NE(nullptr, lay);
    ASSERT_EQ(LdmVolumeKind::Striped, lay->kind);

    auto vol = std::make_unique<LdmVolumeDevice>(*lay, [&](const LdmGuid& g) -> IStorageDevice* {
        return g == info.disks[0].guid ? &dev : nullptr;
    });
    LdmBasisTable basis;
    basis.add(vol.get());

    tree::ResultTree tree;
    tree::FullNode* root = tree.add_root(u"Dynamic Volume Stripe1");
    tree.set_root_basis(root, 1);
    /* 3 consecutive chunks (rows 0..1, both columns) */
    const u64 ext_len = 3 * CHUNK * 512;
    ASSERT_NE(nullptr, plant_file(tree, root, "STRIPES.BIN", 0, ext_len));

    FsIo io(tmp.path);
    WriterConfig cfg;
    cfg.verify_hash = false;
    SafeWriter w(io, &dev, cfg, &basis);
    ASSERT_EQ(OK, w.enqueue(tree.roots()[0]->first_child(), u"out"));
    WriterStats stats;
    ASSERT_EQ(OK, w.process(nullptr, nullptr, nullptr, stats));
    const std::vector<u8> out = read_out(tmp.path / "out" / "STRIPES.BIN");
    ASSERT_EQ(size_t(ext_len), out.size());
    for (u64 k = 0; k < 3; k++) {
        const u8 col = u8(k % 2);
        const u64 row = k / 2;
        const u8 expect = col == 0 ? u8(0xA0 + (row & 0x0F)) : u8(0xB0 + (row & 0x0F));
        for (u64 i = 0; i < CHUNK * 512; i++)
            EXPECT_EQ(expect, out[size_t(k * CHUNK * 512 + i)]) << "chunk " << k << " byte " << i;
    }
}

/* RAID-5 with one member absent: the virtual device reconstructs (the
 * scan-side semantics), and the recovery serves the reconstructed bytes */
TEST(LdmRecovery, Raid5RecoveryWithMissingMember) {
    TmpDir tmp("s052_r5");
    /* the raid5 fixture from test_ldm.cpp's shape, rebuilt here */
    constexpr u64 R5_PART_START = 2048, R5_PART_SIZE = 4096, R5_CHUNK = 64;
    const char* R5_GUIDS[3] = {"01020304-0506-0708-090A-0B0C0D0E0F10",
                               "11121314-1516-1718-191A-1B1C1D1E1F20",
                               "21222324-2526-2728-292A-2B2C2D2E2F30"};
    std::vector<std::vector<u8>> imgs;
    for (u32 i = 0; i < 3; i++) {
        LdmFixture fx(16384);
        fx.set_disk(1000 + i, R5_GUIDS[i]);
        fx.add_dgr4(1, "TestPCDg0");
        fx.add_dsk4(1000, "Disk1", R5_GUIDS[0]);
        fx.add_dsk4(1001, "Disk2", R5_GUIDS[1]);
        fx.add_dsk4(1002, "Disk3", R5_GUIDS[2]);
        fx.add_volume({2000, "Raid1", 4, R5_PART_SIZE / R5_CHUNK * R5_CHUNK * 2, 1});
        fx.add_component({3000, "Raid1-01", LDM_COMP_RAID, 3, 2000, R5_CHUNK, 3});
        fx.add_partition({4000, "Disk1-01", R5_PART_START, R5_PART_SIZE, 0, 3000, 0, 1000});
        fx.add_partition({4001, "Disk2-01", R5_PART_START, R5_PART_SIZE, 0, 3000, 1, 1001});
        fx.add_partition({4002, "Disk3-01", R5_PART_START, R5_PART_SIZE, 0, 3000, 2, 1002});
        fx.finish();
        imgs.push_back(fx.take());
    }
    const auto r5_fill = [](u64 row, u64 col) -> u8 {
        return u8((row * 5 + col * 37 + 11) & 0xFF);
    };
    const u64 rows = R5_PART_SIZE / R5_CHUNK;
    for (u64 row = 0; row < rows; row++) {
        const u64 parity = 2 - (row % 3);
        u32 data_cols[2] = {0, 0};
        u32 nd = 0;
        for (u32 c = 0; c < 3; c++)
            if (c != parity)
                data_cols[nd++] = c;
        const u8 pb = u8(r5_fill(row, data_cols[0]) ^ r5_fill(row, data_cols[1]));
        for (u32 c = 0; c < 3; c++)
            std::memset(imgs[c].data() + (R5_PART_START + row * R5_CHUNK) * 512,
                        c == parity ? pb : r5_fill(row, c), size_t(R5_CHUNK * 512));
    }
    MemDevice d0(std::move(imgs[0]));
    MemDevice d1(std::move(imgs[1]));
    MemDevice d2(std::move(imgs[2]));
    LdmInfo info;
    ASSERT_EQ(OK, parse_ldm_database(&d0, info));
    const LdmVolumeLayout* lay = find_layout(info, 2000);
    ASSERT_NE(nullptr, lay);
    ASSERT_EQ(LdmVolumeKind::Raid5, lay->kind);

    /* disk 3 ABSENT from the resolver: reads reconstruct from the two
     * surviving columns (the device's own documented semantics) */
    auto vol = std::make_unique<LdmVolumeDevice>(*lay, [&](const LdmGuid& g) -> IStorageDevice* {
        if (g == info.disks[0].guid)
            return &d0;
        if (g == info.disks[1].guid)
            return &d1;
        return nullptr; /* disk 3 missing */
    });
    LdmBasisTable basis;
    basis.add(vol.get());

    tree::ResultTree tree;
    tree::FullNode* root = tree.add_root(u"Dynamic Volume Raid1");
    tree.set_root_basis(root, 1);
    /* the first two data chunks of row 0 live on the two present columns
     * ... but a whole-row extent forces the mapping across the parity
     * rotation: rows 0..3 cover all three columns incl. the missing one */
    const u64 ext_len = 4 * (R5_CHUNK * 512); /* 4 data chunks */
    ASSERT_NE(nullptr, plant_file(tree, root, "R5RECOVER.BIN", 0, ext_len));

    FsIo io(tmp.path);
    WriterConfig cfg;
    cfg.verify_hash = false;
    SafeWriter w(io, &d0, cfg, &basis);
    ASSERT_EQ(OK, w.enqueue(tree.roots()[0]->first_child(), u"out"));
    WriterStats stats;
    ASSERT_EQ(OK, w.process(nullptr, nullptr, nullptr, stats));
    const std::vector<u8> out = read_out(tmp.path / "out" / "R5RECOVER.BIN");
    ASSERT_EQ(size_t(ext_len), out.size());
    /* expected: the data-chunk sequence (k = row*(N-1) + i) */
    for (u64 k = 0; k < 4; k++) {
        const u64 row = k / 2;
        const u64 i = k % 2;
        const u64 parity = 2 - (row % 3);
        const u32 col = u32((parity + 1 + i) % 3);
        for (u64 b = 0; b < R5_CHUNK * 512; b++)
            EXPECT_EQ(r5_fill(row, col), out[size_t(k * R5_CHUNK * 512 + b)])
                << "data chunk " << k << " byte " << b;
    }
}

/* the overlap refusal maps LDM extents to PHYSICAL spans:
 *  (a) a node whose physical span on the scan disk intersects the
 *      destination is REFUSED (the true-positive the old numeric check
 *      could MISS when the layout-relative offset differed);
 *  (b) a node whose layout-relative offset NUMERICALLY hits the
 *      destination span but physically lives on the OTHER member disk
 *      is NOT refused (the old check's false-positive class). */
TEST(LdmRecovery, OverlapRefusalMapsToPhysicalSpans) {
    auto imgs = build_spanned_disks();
    MemDevice d0(std::move(imgs[0]));
    MemDevice d1(std::move(imgs[1]));
    LdmInfo info;
    ASSERT_EQ(OK, parse_ldm_database(&d0, info));
    const LdmVolumeLayout* lay = find_layout(info, 2000);
    ASSERT_NE(nullptr, lay);

    auto vol = std::make_unique<LdmVolumeDevice>(*lay, [&](const LdmGuid& g) -> IStorageDevice* {
        if (g == info.disks[0].guid)
            return &d0;
        if (g == info.disks[1].guid)
            return &d1;
        return nullptr;
    });
    LdmBasisTable basis;
    basis.add(vol.get());

    /* (a) TRUE positive: a volume extent inside the DISK-1 member maps to
     * d0's [SPAN_PART_START*512 ..) physical span; arm the destination
     * over a slice of exactly that region -> must refuse. The old
     * layout-relative numeric check compared volume offset 512 against
     * physical dest LBAs and MISSED the overlap (the finding's
     * false-negative class). */
    {
        TmpDir tmp("s052_ovl_a");
        tree::ResultTree tree;
        tree::FullNode* root = tree.add_root(u"Dynamic Volume Span1");
        tree.set_root_basis(root, 1);
        plant_file(tree, root, "HIT.DAT", 512, 4096); /* volume offset 512 =
                                                           d0 phys 2048*512+512 */
        FsIo io(tmp.path);
        WriterConfig cfg;
        SafeWriter w(io, &d0, cfg, &basis);
        /* the destination volume sits at d0's 1 MiB + 2 KiB - inside the
         * member's physical region */
        w.set_destination_spans({{(SPAN_PART_START * 512) + 2048, 8192}});
        EXPECT_EQ(ERR_DEST_OVERLAP, w.enqueue(tree.roots()[0]->first_child(), u"out"));
    }

    /* (b) FALSE positive fixed: the disk-2 member's physical region on d1
     * starts at the SAME LBA as disk 1's (2048), and the volume-relative
     * extent 2 MiB+512 maps to d1's physical 2048*512+512. The OLD
     * numeric check compared the LAYOUT-RELATIVE offset (2 MiB + 512)
     * against the physical destination span - arming the destination at
     * exactly that numeric range refused an innocent node whose bytes
     * live on ANOTHER DISK. The physical mapping must not. */
    {
        TmpDir tmp("s052_ovl_b");
        tree::ResultTree tree;
        tree::FullNode* root = tree.add_root(u"Dynamic Volume Span1");
        tree.set_root_basis(root, 1);
        plant_file(tree, root, "INNOCENT.DAT", SPAN_PART_SIZE * 512 + 512, 4096);
        FsIo io(tmp.path);
        WriterConfig cfg;
        SafeWriter w(io, &d0, cfg, &basis);
        /* armed at the LAYOUT-RELATIVE numeric range (the old check's
         * comparison basis) - physically this is d1's region, disjoint
         * from the destination disk by hardware */
        w.set_destination_spans({{SPAN_PART_SIZE * 512 + 512, 4096}});
        EXPECT_EQ(OK, w.enqueue(tree.roots()[0]->first_child(), u"out"));
        WriterStats stats;
        ASSERT_EQ(OK, w.process(nullptr, nullptr, nullptr, stats));
        const std::vector<u8> out = read_out(tmp.path / "out" / "INNOCENT.DAT");
        ASSERT_EQ(size_t(4096), out.size());
        for (u8 b : out)
            EXPECT_EQ(0x22, b); /* recovered fine - no false refusal */
    }
}

/* open question 1 pinned: a SIMPLE dynamic volume round-trips through
 * the basis wiring (the identity mapping case) */
TEST(LdmRecovery, SimpleVolumeRecoveryRoundTrip) {
    TmpDir tmp("s052_simple");
    StripeDisk sd;
    auto img = sd.fx.take();
    for (u64 i = 0; i < SIMPLE_SIZE * 512; i++)
        img[size_t(SIMPLE_START * 512) + size_t(i)] = u8((i * 7 + 3) & 0xFF);
    MemDevice dev(std::move(img));
    LdmInfo info;
    ASSERT_EQ(OK, parse_ldm_database(&dev, info));
    const LdmVolumeLayout* lay = find_layout(info, 2000);
    ASSERT_NE(nullptr, lay);
    ASSERT_EQ(LdmVolumeKind::Simple, lay->kind);

    auto vol = std::make_unique<LdmVolumeDevice>(*lay, [&](const LdmGuid& g) -> IStorageDevice* {
        return g == info.disks[0].guid ? &dev : nullptr;
    });
    LdmBasisTable basis;
    basis.add(vol.get());

    tree::ResultTree tree;
    tree::FullNode* root = tree.add_root(u"Dynamic Volume Volume1");
    tree.set_root_basis(root, 1);
    const u64 len = 64 * 1024;
    ASSERT_NE(nullptr, plant_file(tree, root, "MARKER.BIN", 777, len));

    FsIo io(tmp.path);
    WriterConfig cfg;
    cfg.verify_hash = false;
    SafeWriter w(io, &dev, cfg, &basis);
    ASSERT_EQ(OK, w.enqueue(tree.roots()[0]->first_child(), u"out"));
    WriterStats stats;
    ASSERT_EQ(OK, w.process(nullptr, nullptr, nullptr, stats));
    const std::vector<u8> out = read_out(tmp.path / "out" / "MARKER.BIN");
    ASSERT_EQ(size_t(len), out.size());
    for (u64 i = 0; i < len; i++)
        EXPECT_EQ(u8(((777 + i) * 7 + 3) & 0xFF), out[size_t(i)]) << "byte " << i;
}

/* preview bytes == recovery bytes for an LDM-basis node (the finding's
 * preview-matches-recovery contract, core level: the SAME basis device
 * feeds the PreviewReader the writer read through) */
TEST(LdmRecovery, PreviewMatchesRecovery) {
    TmpDir tmp("s052_prev");
    auto imgs = build_spanned_disks();
    MemDevice d0(std::move(imgs[0]));
    MemDevice d1(std::move(imgs[1]));
    LdmInfo info;
    ASSERT_EQ(OK, parse_ldm_database(&d0, info));
    const LdmVolumeLayout* lay = find_layout(info, 2000);
    ASSERT_NE(nullptr, lay);

    auto vol = std::make_unique<LdmVolumeDevice>(*lay, [&](const LdmGuid& g) -> IStorageDevice* {
        if (g == info.disks[0].guid)
            return &d0;
        if (g == info.disks[1].guid)
            return &d1;
        return nullptr;
    });
    LdmBasisTable basis;
    basis.add(vol.get());

    tree::ResultTree tree;
    tree::FullNode* root = tree.add_root(u"Dynamic Volume Span1");
    tree.set_root_basis(root, 1);
    const u64 ext_off = SPAN_PART_SIZE * 512 + 512;
    const u64 ext_len = 8192;
    tree::FileNode* node = plant_file(tree, root, "SAME.BIN", ext_off, ext_len);
    ASSERT_NE(nullptr, node);

    /* the preview reader through the basis device */
    preview::PreviewReader reader(vol.get(), node->props());
    std::vector<u8> pv(ext_len, 0);
    ASSERT_EQ(u32(ext_len), reader.read_at(0, pv.data(), u32(ext_len)));

    FsIo io(tmp.path);
    WriterConfig cfg;
    cfg.verify_hash = false;
    SafeWriter w(io, &d0, cfg, &basis);
    ASSERT_EQ(OK, w.enqueue(node, u"out"));
    WriterStats stats;
    ASSERT_EQ(OK, w.process(nullptr, nullptr, nullptr, stats));
    const std::vector<u8> out = read_out(tmp.path / "out" / "SAME.BIN");
    ASSERT_EQ(size_t(ext_len), out.size());
    ASSERT_EQ(size_t(ext_len), pv.size());
    for (size_t i = 0; i < out.size(); i++)
        ASSERT_EQ(out[i], pv[i]) << "preview byte != recovery byte at " << i;
    for (u8 b : out)
        EXPECT_EQ(0x22, b);
}

/* the session round-trip: the layouts record + the tree's root basis
 * stamps survive serialize -> deserialize (the resume/recovery input) */
TEST(LdmRecovery, SessionRoundTripsLayoutsAndRootBasis) {
    auto imgs = build_spanned_disks();
    MemDevice d0(std::move(imgs[0]));
    MemDevice d1(std::move(imgs[1]));
    LdmInfo info;
    ASSERT_EQ(OK, parse_ldm_database(&d0, info));
    const LdmVolumeLayout* lay = find_layout(info, 2000);
    ASSERT_NE(nullptr, lay);

    tree::ResultTree tree;
    tree::FullNode* root = tree.add_root(u"Dynamic Volume Span1");
    tree.set_root_basis(root, 7); /* an arbitrary nonzero stamp */
    plant_file(tree, root, "PERSIST.DAT", 512, 4096);

    std::vector<u8> tree_blob;
    ASSERT_EQ(OK, session::serialize_tree(tree, tree_blob));
    std::vector<u8> lay_blob;
    ASSERT_EQ(OK, session::serialize_ldm_layouts(info.layouts, lay_blob));

    tree::ResultTree back;
    ASSERT_EQ(OK, session::deserialize_tree(tree_blob.data(), tree_blob.size(), back));
    ASSERT_EQ(1u, back.roots().size());
    EXPECT_EQ(7u, back.roots()[0]->basis()) << "the root basis stamp round-trips";

    std::vector<LdmVolumeLayout> lays;
    ASSERT_EQ(OK, session::deserialize_ldm_layouts(lay_blob.data(), lay_blob.size(), lays));
    ASSERT_EQ(info.layouts.size(), lays.size());
    ASSERT_EQ(lay->kind, lays[0].kind);
    EXPECT_EQ(lay->vol_obj_id, lays[0].vol_obj_id);
    EXPECT_EQ(lay->total_sectors, lays[0].total_sectors);
    ASSERT_EQ(lay->copies.size(), lays[0].copies.size());
    ASSERT_EQ(lay->copies[0].size(), lays[0].copies[0].size());
    EXPECT_EQ(lay->copies[0][1].disk_guid, lays[0].copies[0][1].disk_guid);
    EXPECT_EQ(lay->copies[0][1].vol_offset, lays[0].copies[0][1].vol_offset);

    /* the re-armed device over the ROUND-TRIPPED layout reads the same
     * bytes (the resume path's core guarantee) */
    LdmVolumeDevice vol2(lays[0], [&](const LdmGuid& g) -> IStorageDevice* {
        if (g == info.disks[0].guid)
            return &d0;
        if (g == info.disks[1].guid)
            return &d1;
        return nullptr;
    });
    std::vector<u8> b(16, 0);
    ASSERT_EQ(OK, vol2.read_at(SPAN_PART_SIZE * 512 + 512, b.data(), u32(b.size())));
    for (u8 x : b)
        EXPECT_EQ(0x22, x);
}
