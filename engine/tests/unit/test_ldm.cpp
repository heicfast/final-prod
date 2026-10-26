/* test_ldm.cpp - LDM full-implementation tests.
 *
 * The fixture builder emits byte-exact kernel/libldm-true layouts:
 *   - MBR with a type-0x42 span; PRIVHEAD at LBA 6 (+2 backups)
 *   - TOCBLOCK at config+1/+2 (bitmap slots "config"/"log")
 *   - VMDB at config+17 (vblk array at +512, 128-byte records)
 *   - VBLK object graph: DGR4, DSK4, volumes, components, partitions
 *   - composite (multi-fragment) records
 *   - striped volume with 64 KiB chunks; RAID-5 with left-symmetric
 *     parity; mirrored copies; spanned concatenation
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/table/cr_ldm.h"
#include "cr/table/cr_table_parse.h"
#include "test_fixtures.hpp"
#include <gtest/gtest.h>
#include <cstring>
#include <cstdlib>
#include <string>

using namespace cr::testfixture;

namespace cr {
namespace table {
namespace {

/* ---------------------------------------------------------------- */
/* In-memory device                                                  */
/* ---------------------------------------------------------------- */

class MemDevice : public IStorageDevice {
public:
    explicit MemDevice(std::vector<u8> d) : d_(std::move(d)) {
        g_.logical_sector = 512;
        g_.physical_sector = 512;
        g_.total_bytes = d_.size();
        g_.sector_count = d_.size() / 512;
    }
    u32 read_at(u64 off, void* buf, u32 len) override {
        if (off > d_.size() || len > d_.size() - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        memcpy(buf, d_.data() + off, len);
        return OK;
    }
    const Geometry& geometry() const override { return g_; }

private:
    Geometry g_{};
    std::vector<u8> d_;
};

/* ---------------------------------------------------------------- */
/* Tests                                                             */
/* ---------------------------------------------------------------- */

TEST(Ldm, ParsesDatabaseAndGraph) {
    StripeDisk sd;
    MemDevice dev(sd.fx.take());
    LdmInfo info;
    ASSERT_EQ(OK, parse_ldm_database(&dev, info));
    ASSERT_TRUE(info.found);
    EXPECT_EQ(u16(2), info.ph.ver_major);
    EXPECT_EQ(u16(11), info.ph.ver_minor);
    EXPECT_EQ(u64(2048), info.ph.config_size);
    EXPECT_EQ(u64(42), info.vm.committed_seq);
    EXPECT_EQ(u32(VBLK_SIZE), info.vm.vblk_size);
    EXPECT_EQ(u32(512), info.vm.vblk_offset);
    EXPECT_STREQ("TestPCDg0", info.vm.disk_group_name.c_str());
    EXPECT_EQ(size_t(1), info.groups.size());
    EXPECT_EQ(size_t(1), info.disks.size());
    EXPECT_EQ(size_t(2), info.volumes.size());
    EXPECT_EQ(size_t(2), info.components.size());
    EXPECT_EQ(size_t(3), info.partitions.size());
    EXPECT_EQ(u64(3), info.stats.privheads_matched); /* primary + 2 backups */
    EXPECT_EQ(u32(9), info.stats.vblks_parsed);      /* 9 objects (composite = 1) */
    EXPECT_EQ(u32(0), info.stats.vblks_bad);
    /* the DSK4 binary GUID equals the parsed PRIVHEAD text GUID */
    ASSERT_EQ(size_t(1), info.disks.size());
    LdmGuid expect_guid = {};
    ASSERT_TRUE(ldm_parse_guid_text("01234567-89AB-CDEF-0123-456789ABCDEF", expect_guid));
    EXPECT_EQ(expect_guid, info.disks[0].guid);
    EXPECT_TRUE(info.disks[0].guid_valid);
}

const LdmVolumeLayout* find_layout(const LdmInfo& info, u64 vol_obj_id) {
    for (const auto& l : info.layouts)
        if (l.vol_obj_id == vol_obj_id)
            return &l;
    return nullptr;
}

TEST(Ldm, ResolvesSimpleVolume) {
    StripeDisk sd;
    MemDevice dev(sd.fx.take());
    LdmInfo info;
    ASSERT_EQ(OK, parse_ldm_database(&dev, info));
    ASSERT_EQ(size_t(2), info.layouts.size());
    /* composites commit after the walk, so walk order puts Stripe1 first;
     * look the layouts up by volume object id instead of index */
    const LdmVolumeLayout* lay = find_layout(info, 2000);
    ASSERT_NE(nullptr, lay);
    EXPECT_EQ(LdmVolumeKind::Simple, lay->kind);
    EXPECT_TRUE(lay->complete);
    EXPECT_EQ(u64(SIMPLE_SIZE), lay->total_sectors);
    ASSERT_EQ(size_t(1), lay->copies.size());
    ASSERT_EQ(size_t(1), lay->copies[0].size());
    const LdmMember& m = lay->copies[0][0];
    EXPECT_EQ(u64(SIMPLE_START), m.disk_lba);
    EXPECT_EQ(u64(SIMPLE_SIZE), m.sectors);
}

TEST(Ldm, SimpleVolumeDeviceReads) {
    StripeDisk sd;
    /* distinct marker bytes inside the simple volume */
    auto img = sd.fx.take();
    for (u64 i = 0; i < SIMPLE_SIZE * 512; i++)
        img[size_t(SIMPLE_START * 512) + size_t(i)] = u8((i * 7 + 3) & 0xFF);
    MemDevice dev(std::move(img));
    LdmInfo info;
    ASSERT_EQ(OK, parse_ldm_database(&dev, info));
    const LdmVolumeLayout* lay = find_layout(info, 2000);
    ASSERT_NE(nullptr, lay);
    LdmVolumeDevice vol(*lay, [&](const LdmGuid& g) -> IStorageDevice* {
        return g == info.disks[0].guid ? &dev : nullptr;
    });
    EXPECT_EQ(u64(SIMPLE_SIZE * 512), vol.size());
    /* aligned whole-volume read */
    std::vector<u8> buf(SIMPLE_SIZE * 512);
    ASSERT_EQ(OK, vol.read_at(0, buf.data(), u32(buf.size())));
    for (u64 i = 0; i < SIMPLE_SIZE * 512; i++)
        ASSERT_EQ(u8((i * 7 + 3) & 0xFF), buf[size_t(i)]) << "byte " << i;
    /* unaligned head+tail read crossing a sector */
    u8 part[1000];
    ASSERT_EQ(OK, vol.read_at(100, part, sizeof(part)));
    for (u32 i = 0; i < sizeof(part); i++)
        EXPECT_EQ(u8(((100 + i) * 7 + 3) & 0xFF), part[i]) << "byte " << i;
    /* out of range */
    EXPECT_EQ(ERR_RANGE_OUT_OF_BOUNDS, vol.read_at(vol.size() - 100, part, 200));
}

TEST(Ldm, StripedVolumeInterleave) {
    StripeDisk sd;
    auto img = sd.fx.take();
    /* member A: chunks tagged 0xA0 + row; member B: 0xB0 + row */
    for (u64 row = 0; row < STRIPE_MEMBER_SIZE / CHUNK; row++) {
        memset(img.data() + (STRIPE_A_START + row * CHUNK) * 512, int(0xA0 + (row & 0x0F)),
               size_t(CHUNK * 512));
        memset(img.data() + (STRIPE_B_START + row * CHUNK) * 512, int(0xB0 + (row & 0x0F)),
               size_t(CHUNK * 512));
    }
    MemDevice dev(std::move(img));
    LdmInfo info;
    ASSERT_EQ(OK, parse_ldm_database(&dev, info));
    const LdmVolumeLayout* lay = find_layout(info, 2001);
    ASSERT_NE(nullptr, lay);
    ASSERT_EQ(LdmVolumeKind::Striped, lay->kind);
    EXPECT_EQ(u64(CHUNK), lay->chunk_sectors);
    EXPECT_EQ(u32(2), lay->columns);
    EXPECT_TRUE(lay->complete);
    LdmVolumeDevice vol(*lay, [&](const LdmGuid& g) -> IStorageDevice* {
        return g == info.disks[0].guid ? &dev : nullptr;
    });
    /* volume chunk k lands on column k%2, row k/2 */
    const u64 rows = STRIPE_MEMBER_SIZE / CHUNK;
    EXPECT_EQ(u64(rows * CHUNK * 2 * 512), vol.size());
    for (u64 k = 0; k < rows * 2; k++) {
        u8 first = 0;
        ASSERT_EQ(OK, vol.read_at(k * CHUNK * 512, &first, 1));
        const u8 col = u8(k % 2);
        const u64 row = k / 2;
        const u8 expect = col == 0 ? u8(0xA0 + (row & 0x0F)) : u8(0xB0 + (row & 0x0F));
        EXPECT_EQ(expect, first) << "chunk " << k;
    }
}

TEST(Ldm, CompositeVblkRecord) {
    StripeDisk sd;
    MemDevice dev(sd.fx.take());
    LdmInfo info;
    ASSERT_EQ(OK, parse_ldm_database(&dev, info));
    /* Volume1 was emitted as a 2-fragment composite spanning slots */
    const LdmVolumeObj* v1 = nullptr;
    for (const auto& v : info.volumes)
        if (v.name == "Volume1-CompositeSpanTest-0123456789")
            v1 = &v;
    ASSERT_NE(nullptr, v1);
    EXPECT_EQ(u64(2000), v1->obj_id);
    EXPECT_EQ(u64(SIMPLE_SIZE), v1->size);
    EXPECT_EQ(u64(1), v1->n_comps);
    EXPECT_EQ(u32(1), info.stats.composites);
    EXPECT_EQ(u32(0), info.stats.composites_incomplete);
}

TEST(Ldm, NonLdmDiskRejected) {
    std::vector<u8> d(8ull << 20, 0x55);
    d[510] = 0x55;
    MemDevice dev(std::move(d));
    LdmInfo info;
    EXPECT_EQ(ERR_BAD_MAGIC, parse_ldm_database(&dev, info));
    EXPECT_FALSE(info.found);
}

/* ---------------------------------------------------------------- */
/* Multi-disk fixtures: RAID-5 / mirror / spanned                    */
/* ---------------------------------------------------------------- */

constexpr u64 R5_PART_START = 2048;
constexpr u64 R5_PART_SIZE = 4096;
constexpr u64 R5_CHUNK = 64; /* sectors per stripe unit */
constexpr u64 MIRROR_PART_SIZE = 4096;
constexpr u64 SPAN_PART_SIZE = 4096;

const u8 R5_GUIDS[3][16] = {{0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C,
                             0x0D, 0x0E, 0x0F, 0x10},
                            {0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C,
                             0x1D, 0x1E, 0x1F, 0x20},
                            {0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C,
                             0x2D, 0x2E, 0x2F, 0x30}};
const char* R5_GUID_TEXTS[3] = {"01020304-0506-0708-090A-0B0C0D0E0F10",
                                "11121314-1516-1718-191A-1B1C1D1E1F20",
                                "21222324-2526-2728-292A-2B2C2D2E2F30"};

u8 r5_fill(u64 row, u64 col) {
    return u8((row * 5 + col * 37 + 11) & 0xFF);
}

std::vector<std::vector<u8>> build_raid5_disks() {
    std::vector<std::vector<u8>> imgs;
    for (u32 i = 0; i < 3; i++) {
        LdmFixture fx(16384); /* 8 MiB per disk */
        fx.set_disk(1000 + i, R5_GUID_TEXTS[i]);
        fx.add_dgr4(1, "TestPCDg0");
        fx.add_dsk4(1000, "Disk1", R5_GUID_TEXTS[0]);
        fx.add_dsk4(1001, "Disk2", R5_GUID_TEXTS[1]);
        fx.add_dsk4(1002, "Disk3", R5_GUID_TEXTS[2]);
        fx.add_volume(
            {2000, "Raid1", 4 /* _VOLUME_TYPE_RAID5 */, R5_PART_SIZE / R5_CHUNK * R5_CHUNK * 2, 1});
        fx.add_component({3000, "Raid1-01", LDM_COMP_RAID, 3, 2000, R5_CHUNK, 3});
        fx.add_partition({4000, "Disk1-01", R5_PART_START, R5_PART_SIZE, 0, 3000, 0, 1000});
        fx.add_partition({4001, "Disk2-01", R5_PART_START, R5_PART_SIZE, 0, 3000, 1, 1001});
        fx.add_partition({4002, "Disk3-01", R5_PART_START, R5_PART_SIZE, 0, 3000, 2, 1002});
        fx.finish();
        imgs.push_back(fx.take());
    }
    /* fill the member areas: data columns get the row/column pattern,
     * the parity column gets the real XOR of the row's data columns
     * (left-symmetric: P(row) = N-1-(row mod N)) */
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
            memset(imgs[c].data() + (R5_PART_START + row * R5_CHUNK) * 512,
                   c == parity ? pb : r5_fill(row, c), size_t(R5_CHUNK * 512));
    }
    return imgs;
}

TEST(Ldm, Raid5LeftSymmetricReads) {
    auto imgs = build_raid5_disks();
    std::vector<MemDevice> devs;
    devs.emplace_back(std::move(imgs[0]));
    devs.emplace_back(std::move(imgs[1]));
    devs.emplace_back(std::move(imgs[2]));
    LdmInfo info;
    ASSERT_EQ(OK, parse_ldm_database(&devs[0], info));
    const LdmVolumeLayout* lay = find_layout(info, 2000);
    ASSERT_NE(nullptr, lay);
    ASSERT_EQ(LdmVolumeKind::Raid5, lay->kind);
    EXPECT_EQ(u64(R5_CHUNK), lay->chunk_sectors);
    EXPECT_EQ(u32(3), lay->columns);
    EXPECT_TRUE(lay->complete);
    const u64 rows = R5_PART_SIZE / R5_CHUNK;
    LdmVolumeDevice vol(*lay, [&](const LdmGuid& g) -> IStorageDevice* {
        for (u32 i = 0; i < 3; i++)
            if (g == info.disks[i].guid)
                return &devs[i];
        return nullptr;
    });
    EXPECT_EQ(u64(rows * R5_CHUNK * 2 * 512), vol.size());
    /* data chunk k -> row k/(N-1), i k%(N-1), parity P=(N-1)-(row%N),
     * column (P+1+i)%N (md left-symmetric; libldm "raid5_ls") */
    for (u64 k = 0; k < rows * 2; k++) {
        const u64 row = k / 2;
        const u64 i = k % 2;
        const u64 parity = 2 - (row % 3);
        const u64 col = (parity + 1 + i) % 3;
        u8 first = 0;
        ASSERT_EQ(OK, vol.read_at(k * R5_CHUNK * 512, &first, 1)) << "chunk " << k;
        EXPECT_EQ(r5_fill(row, col), first) << "chunk " << k;
    }
    /* one large multi-chunk read through the batching path */
    std::vector<u8> buf(size_t(R5_CHUNK * 6 * 512));
    ASSERT_EQ(OK, vol.read_at(R5_CHUNK * 3 * 512, buf.data(), u32(buf.size())));
    for (u64 k = 3; k < 9; k++) {
        const u64 row = k / 2;
        const u64 i = k % 2;
        const u64 parity = 2 - (row % 3);
        const u64 col = (parity + 1 + i) % 3;
        EXPECT_EQ(r5_fill(row, col), buf[size_t((k - 3) * R5_CHUNK * 512)]) << "bulk chunk " << k;
    }
}

TEST(Ldm, Raid5ReconstructsMissingColumn) {
    auto imgs = build_raid5_disks();
    std::vector<MemDevice> devs;
    devs.emplace_back(std::move(imgs[0]));
    devs.emplace_back(std::move(imgs[1])); /* disk 3 intentionally absent */
    LdmInfo info;
    ASSERT_EQ(OK, parse_ldm_database(&devs[0], info));
    const LdmVolumeLayout* lay = find_layout(info, 2000);
    ASSERT_NE(nullptr, lay);
    ASSERT_EQ(LdmVolumeKind::Raid5, lay->kind);
    LdmVolumeDevice vol(*lay, [&](const LdmGuid& g) -> IStorageDevice* {
        for (u32 i = 0; i < 2; i++)
            if (g == info.disks[i].guid)
                return &devs[i];
        return nullptr; /* Disk3 missing: 1 of 3 columns gone */
    });
    const u64 rows = R5_PART_SIZE / R5_CHUNK;
    for (u64 k = 0; k < rows * 2; k++) {
        const u64 row = k / 2;
        const u64 i = k % 2;
        const u64 parity = 2 - (row % 3);
        const u64 col = (parity + 1 + i) % 3;
        u8 first = 0;
        ASSERT_EQ(OK, vol.read_at(k * R5_CHUNK * 512, &first, 1)) << "chunk " << k;
        EXPECT_EQ(r5_fill(row, col), first) << "reconstructed chunk " << k;
    }
}

TEST(Ldm, Raid5TwoMissingColumnsUnrecoverable) {
    auto imgs = build_raid5_disks();
    std::vector<MemDevice> devs;
    devs.emplace_back(std::move(imgs[0])); /* disks 2+3 absent */
    LdmInfo info;
    ASSERT_EQ(OK, parse_ldm_database(&devs[0], info));
    const LdmVolumeLayout* lay = find_layout(info, 2000);
    ASSERT_NE(nullptr, lay);
    LdmVolumeDevice vol(*lay, [&](const LdmGuid& g) -> IStorageDevice* {
        return g == info.disks[0].guid ? &devs[0] : nullptr;
    });
    u8 first = 0;
    /* chunk 0 (row 0, col 0) lives on the surviving column: direct read */
    EXPECT_EQ(OK, vol.read_at(0, &first, 1));
    EXPECT_EQ(r5_fill(0, 0), first);
    /* chunk 2 (row 1, i 0 -> col 2) is on a missing column and its
     * reconstruction needs the other missing column: unrecoverable */
    EXPECT_EQ(ERR_READ_FAILED, vol.read_at(2 * R5_CHUNK * 512, &first, 1));
}

/* Failing device: fails reads intersecting [lo, hi). */
class HoleDevice final : public MemDevice {
public:
    HoleDevice(std::vector<u8> d, u64 lo, u64 hi) : MemDevice(std::move(d)), lo_(lo), hi_(hi) {}
    u32 read_at(u64 off, void* buf, u32 len) override {
        if (off < hi_ && lo_ < off + len)
            return ERR_READ_FAILED;
        return MemDevice::read_at(off, buf, len);
    }

private:
    u64 lo_, hi_;
};

std::vector<std::vector<u8>> build_mirror_disks() {
    std::vector<std::vector<u8>> imgs;
    for (u32 i = 0; i < 2; i++) {
        LdmFixture fx(16384);
        fx.set_disk(1000 + i, R5_GUID_TEXTS[i]);
        fx.add_dgr4(1, "TestPCDg0");
        fx.add_dsk4(1000, "Disk1", R5_GUID_TEXTS[0]);
        fx.add_dsk4(1001, "Disk2", R5_GUID_TEXTS[1]);
        /* mirrored volume: two components, one part each */
        fx.add_volume({2000, "Mirror1", 3, MIRROR_PART_SIZE, 2});
        fx.add_component({3000, "Mirror1-01", LDM_COMP_CONCAT, 1, 2000, 0, 0});
        fx.add_component({3001, "Mirror1-02", LDM_COMP_CONCAT, 1, 2000, 0, 0});
        fx.add_partition({4000, "Disk1-01", 2048, MIRROR_PART_SIZE, 0, 3000, 0, 1000});
        fx.add_partition({4001, "Disk2-01", 2048, MIRROR_PART_SIZE, 0, 3001, 0, 1001});
        fx.finish();
        imgs.push_back(fx.take());
    }
    /* distinct content per copy */
    memset(imgs[0].data() + 2048 * 512, 0x5A, size_t(MIRROR_PART_SIZE * 512));
    memset(imgs[1].data() + 2048 * 512, 0xA5, size_t(MIRROR_PART_SIZE * 512));
    return imgs;
}

TEST(Ldm, MirroredReadFailover) {
    auto imgs = build_mirror_disks();
    /* copy 0 (Disk1) has an unrecoverable hole */
    HoleDevice d0(std::move(imgs[0]), 2048 * 512, (2048 + MIRROR_PART_SIZE) * 512);
    MemDevice d1(std::move(imgs[1]));
    LdmInfo info;
    ASSERT_EQ(OK, parse_ldm_database(&d0, info));
    const LdmVolumeLayout* lay = find_layout(info, 2000);
    ASSERT_NE(nullptr, lay);
    ASSERT_EQ(LdmVolumeKind::Mirrored, lay->kind);
    EXPECT_EQ(size_t(2), lay->copies.size());
    LdmVolumeDevice vol(*lay, [&](const LdmGuid& g) -> IStorageDevice* {
        if (g == info.disks[0].guid)
            return &d0;
        if (g == info.disks[1].guid)
            return &d1;
        return nullptr;
    });
    /* copy 0 is wholly unreadable: every byte must come from copy 1 */
    std::vector<u8> buf(4096);
    for (u64 off = 0; off < MIRROR_PART_SIZE * 512; off += 4096) {
        ASSERT_EQ(OK, vol.read_at(off, buf.data(), u32(buf.size())));
        for (u8 b : buf)
            EXPECT_EQ(0xA5, b) << "mirror failover at " << off;
    }
}

TEST(Ldm, MirroredBothCopiesFail) {
    auto imgs = build_mirror_disks();
    HoleDevice d0(std::move(imgs[0]), 2048 * 512, (2048 + MIRROR_PART_SIZE) * 512);
    HoleDevice d1(std::move(imgs[1]), 2048 * 512, (2048 + MIRROR_PART_SIZE) * 512);
    LdmInfo info;
    ASSERT_EQ(OK, parse_ldm_database(&d0, info));
    const LdmVolumeLayout* lay = find_layout(info, 2000);
    ASSERT_NE(nullptr, lay);
    LdmVolumeDevice vol(*lay, [&](const LdmGuid& g) -> IStorageDevice* {
        if (g == info.disks[0].guid)
            return &d0;
        if (g == info.disks[1].guid)
            return &d1;
        return nullptr;
    });
    u8 b = 0;
    EXPECT_EQ(ERR_READ_FAILED, vol.read_at(0, &b, 1));
}

std::vector<std::vector<u8>> build_spanned_disks() {
    std::vector<std::vector<u8>> imgs;
    for (u32 i = 0; i < 2; i++) {
        LdmFixture fx(16384);
        fx.set_disk(1000 + i, R5_GUID_TEXTS[i]);
        fx.add_dgr4(1, "TestPCDg0");
        fx.add_dsk4(1000, "Disk1", R5_GUID_TEXTS[0]);
        fx.add_dsk4(1001, "Disk2", R5_GUID_TEXTS[1]);
        fx.add_volume({2000, "Span1", 3, SPAN_PART_SIZE * 2, 1});
        fx.add_component({3000, "Span1-01", LDM_COMP_CONCAT, 2, 2000, 0, 0});
        fx.add_partition({4000, "Disk1-01", 2048, SPAN_PART_SIZE, 0, 3000, 0, 1000});
        fx.add_partition({4001, "Disk2-01", 2048, SPAN_PART_SIZE, SPAN_PART_SIZE, 3000, 1, 1001});
        fx.finish();
        imgs.push_back(fx.take());
    }
    memset(imgs[0].data() + 2048 * 512, 0x11, size_t(SPAN_PART_SIZE * 512));
    memset(imgs[1].data() + 2048 * 512, 0x22, size_t(SPAN_PART_SIZE * 512));
    return imgs;
}

TEST(Ldm, SpannedConcatenation) {
    auto imgs = build_spanned_disks();
    MemDevice d0(std::move(imgs[0]));
    MemDevice d1(std::move(imgs[1]));
    LdmInfo info;
    ASSERT_EQ(OK, parse_ldm_database(&d0, info));
    const LdmVolumeLayout* lay = find_layout(info, 2000);
    ASSERT_NE(nullptr, lay);
    ASSERT_EQ(LdmVolumeKind::Spanned, lay->kind);
    ASSERT_EQ(size_t(1), lay->copies.size());
    ASSERT_EQ(size_t(2), lay->copies[0].size());
    /* members ordered by vol_offset: Disk1's part first */
    EXPECT_EQ(u64(0), lay->copies[0][0].vol_offset);
    EXPECT_EQ(u64(SPAN_PART_SIZE), lay->copies[0][1].vol_offset);
    LdmVolumeDevice vol(*lay, [&](const LdmGuid& g) -> IStorageDevice* {
        if (g == info.disks[0].guid)
            return &d0;
        if (g == info.disks[1].guid)
            return &d1;
        return nullptr;
    });
    EXPECT_EQ(u64(SPAN_PART_SIZE * 2 * 512), vol.size());
    std::vector<u8> buf(512);
    /* first half from Disk1, second half from Disk2 */
    ASSERT_EQ(OK, vol.read_at(512, buf.data(), 512));
    for (u8 b : buf)
        EXPECT_EQ(0x11, b);
    ASSERT_EQ(OK, vol.read_at(SPAN_PART_SIZE * 512 + 512, buf.data(), 512));
    for (u8 b : buf)
        EXPECT_EQ(0x22, b);
    /* a read crossing the member boundary */
    std::vector<u8> cross(1024);
    ASSERT_EQ(OK, vol.read_at(SPAN_PART_SIZE * 512 - 512, cross.data(), u32(cross.size())));
    for (u64 i = 0; i < 512; i++)
        EXPECT_EQ(0x11, cross[size_t(i)]);
    for (u64 i = 512; i < 1024; i++)
        EXPECT_EQ(0x22, cross[size_t(i)]);
    /* missing second member: reads inside it fail, reads in the first
     * half still succeed (honest partial availability) */
    LdmVolumeDevice half(*lay, [&](const LdmGuid& g) -> IStorageDevice* {
        return g == info.disks[0].guid ? &d0 : nullptr;
    });
    ASSERT_EQ(OK, half.read_at(512, buf.data(), 512));
    EXPECT_EQ(ERR_READ_FAILED, half.read_at(SPAN_PART_SIZE * 512 + 512, buf.data(), 512));
}
} // namespace
} // namespace table
} // namespace cr

namespace cr {
namespace table {

namespace {
void put32le(u8* p, u32 v) {
    p[0] = u8(v);
    p[1] = u8(v >> 8);
    p[2] = u8(v >> 16);
    p[3] = u8(v >> 24);
}
void put64le(u8* p, u64 v) {
    for (int i = 0; i < 8; i++)
        p[i] = u8(v >> (8 * i));
}
} // namespace

namespace {

/* A-002 de-self-reference: the REAL Microsoft PARTITION_LDM_METADATA_GUID /
 * PARTITION_LDM_DATA_GUID, hard-coded as mixed-endian disk bytes exactly as
 * dmio writes them into GPT entries on real Windows dynamic disks (winioctl.h
 * / Microsoft Learn PARTITION_INFORMATION_GPT; same bytes the og binaries
 * compare against - LdmManager.mo DAT_1800c4960, Partition.mo
 * DAT_180036630/DAT_180036640). The fixtures below write THESE literals,
 * never the constants under test, so a corrupted constant fails the test
 * instead of quietly validating itself. */
constexpr u8 REAL_LDM_METADATA_GUID[16] = {0xAA, 0xC8, 0x08, 0x58, 0x8F, 0x7E, 0xE0, 0x42,
                                           0x85, 0xD2, 0xE1, 0xE9, 0x04, 0x34, 0xCF, 0xB3};
constexpr u8 REAL_LDM_DATA_GUID[16] = {0xA0, 0x60, 0x9B, 0xAF, 0x31, 0x14, 0x62, 0x4F,
                                       0xBC, 0x68, 0x33, 0x11, 0x71, 0x4A, 0x69, 0xAD};

/* A-002 negative fixture: the fabricated 1cb6a43 values that exist in no
 * og binary, no Microsoft source, and no real disk - must be rejected. */
constexpr u8 FABRICATED_LDM_METADATA_GUID[16] = {0xAA, 0xC8, 0x08, 0x58, 0x9F, 0x8E, 0x87, 0x4C,
                                                 0xAE, 0xA2, 0xAF, 0x0A, 0xA3, 0xC4, 0x16, 0x87};

/* shared builder for the GPT-dynamic-disk fixtures (protective MBR +
 * primary/backup GPT with CRCs + two entries + PRIVHEAD at the metadata
 * partition's last block, the og FUN_1800724f0 position). */
std::vector<u8> make_gpt_dynamic_disk(const u8* meta_guid, const u8* data_guid) {
    StripeDisk sd;
    const u64 total_secs = 65536; /* matches the fixture's 32 MiB disk */
    const u64 cfg_start = total_secs - 2048;
    const u64 ldm_meta_last = 65532; /* metadata partition [cfg_start, 65532] */
    sd.fx.write_privhead(ldm_meta_last);
    std::vector<u8> disk = sd.fx.take();
    u8* mbr = disk.data();
    mbr[0x1BE + 4] = 0xEE;
    p32be(mbr + 0x1BE + 8, 1);
    p32be(mbr + 0x1BE + 12, u32(total_secs - 1));
    u8* h = disk.data() + 512;
    std::memcpy(h, "EFI PART", 8);
    put32le(h + 8, 0x00010000);
    put32le(h + 12, 92);
    put64le(h + 24, 1);
    put64le(h + 32, 65535);
    put64le(h + 40, 2);
    put64le(h + 48, 65531);
    put64le(h + 72, 2);
    put32le(h + 80, 2);
    put32le(h + 84, 128);
    std::vector<u8> entries(2 * 128, 0);
    std::memcpy(entries.data() + 0, meta_guid, 16);
    put64le(entries.data() + 32, cfg_start);
    put64le(entries.data() + 40, ldm_meta_last);
    std::memcpy(entries.data() + 128, data_guid, 16);
    put64le(entries.data() + 128 + 32, 2048);
    put64le(entries.data() + 128 + 40, 4096);
    put32le(h + 88, crc32(entries.data(), entries.size()));
    std::memcpy(disk.data() + 2 * 512, entries.data(), entries.size());
    put32le(h + 16, crc32(h, 92));
    std::memcpy(disk.data() + 65534 * 512, entries.data(), entries.size());
    u8* bh = disk.data() + 65535 * 512;
    std::memcpy(bh, h, 512);
    put32le(bh + 16, 0);
    put64le(bh + 24, 65535);
    put64le(bh + 32, 1);
    put64le(bh + 72, 65534);
    put32le(bh + 16, crc32(bh, 92));
    return disk;
}

} // namespace

/* ---- A-001/A-002: GPT dynamic disk with canonical type GUIDs ----------- */

TEST(Ldm, GptDynamicDiskCanonicalGuidsParse) {
    /* A-002: this fixture writes the REAL Microsoft GUID bytes (hard-coded
     * literals above), NOT the constants under test - the previous version
     * was self-referential (constants copied into the fixture), which
     * validated any constant value including the fabricated 1cb6a43 bytes.
     * With the real GUIDs in the entries, a GPT dynamic disk parses
     * end-to-end only while cr_table_wire.h carries Microsoft's actual
     * PARTITION_LDM_METADATA_GUID (the og gate: LdmManager FUN_1800724f0
     * memcmp vs DAT_1800c4960, PRIVHEAD at the entry's last LBA). */
    MemDevice dev(make_gpt_dynamic_disk(REAL_LDM_METADATA_GUID, REAL_LDM_DATA_GUID));
    LdmInfo info;
    ASSERT_EQ(OK, parse_ldm_database(&dev, info));
    EXPECT_TRUE(info.is_gpt);
    EXPECT_EQ(size_t(2), info.layouts.size());
    EXPECT_EQ(u64(2), info.stats.privheads_matched); /* partition tail (65532)
                                                        + config+1856 (65344);
                                                        config+2047 carries the
                                                        backup GPT header here */
}

TEST(Ldm, GptDynamicDiskFabricatedGuidsRejected) {
    /* A-002 negative guard: the fabricated 1cb6a43 GUIDs (5808c8aa-8e9f-… /
     * af9b60a0-384e-… family) appear in no og binary, no Microsoft source,
     * and no real disk; a disk carrying them must NOT be treated as a GPT
     * dynamic disk. Before the A-002 restore this fixture parsed end-to-end
     * - the exact wrongness the fabricated constants embodied. */
    MemDevice dev(make_gpt_dynamic_disk(FABRICATED_LDM_METADATA_GUID, REAL_LDM_DATA_GUID));
    LdmInfo info;
    EXPECT_EQ(ERR_BAD_MAGIC, parse_ldm_database(&dev, info));
}

TEST(Ldm, GptDynamicDiskTableClassifiesLdmEntries) {
    /* the table layer sees the same GPT: LDM data classifies Ldm, the
     * metadata partition classifies Reserved (A-001 classification).
     * A-002: built from the shared REAL-GUID fixture - classification is
     * proven against the Microsoft GUIDs a real disk carries, not against
     * whatever the constants currently hold. */
    MemDevice dev(make_gpt_dynamic_disk(REAL_LDM_METADATA_GUID, REAL_LDM_DATA_GUID));
    DiskTable t;
    ASSERT_EQ(parse_disk_table(&dev, t), OK);
    EXPECT_EQ(t.layout, TableLayout::Gpt);
    ASSERT_EQ(t.partitions.size(), 2u);
    EXPECT_EQ(t.partitions[0].fs, FsKind::Reserved); /* LDM metadata */
    EXPECT_EQ(t.partitions[1].fs, FsKind::Ldm);      /* LDM data */
}

namespace {

/* Mixed-endian disk bytes derived from the Microsoft winioctl TEXT form -
 * an encoding path INDEPENDENT of the byte literals above (A-002: the two
 * paths must agree; a hex typo in either fails the paired test below). */
std::vector<u8> winioctl_guid(const char* text) {
    std::string t;
    for (const char* p = text; *p; ++p)
        if (*p != '-' && *p != '{' && *p != '}')
            t += *p;
    auto hx = [&](size_t i) { return u8(strtoul(t.substr(i, 2).c_str(), nullptr, 16)); };
    std::vector<u8> b(16);
    b[0] = hx(6);
    b[1] = hx(4);
    b[2] = hx(2);
    b[3] = hx(0); /* Data1 LE */
    b[4] = hx(10);
    b[5] = hx(8); /* Data2 LE */
    b[6] = hx(14);
    b[7] = hx(12); /* Data3 LE */
    for (int i = 0; i < 8; i++)
        b[8 + i] = hx(16 + 2 * size_t(i)); /* Data4 raw */
    return b;
}

} // namespace

TEST(Ldm, GptDynamicDiskRealCanonicalGuidsParse) {
    /* A-002 cross-encoding guard (added in the reconciliation of the two
     * independently-landed A-002 fixes): the same disk as
     * GptDynamicDiskCanonicalGuidsParse, but with the entry GUIDs derived
     * independently from the Microsoft winioctl TEXT forms. The literal
     * path and the text path are two independent encodings of ground
     * truth and must agree on every byte - a hex typo in either the byte
     * literals or the constants fails exactly one of the paired tests. */
    const std::vector<u8> meta = winioctl_guid("5808c8aa-7e8f-42e0-85d2-e1e90434cfb3");
    const std::vector<u8> data = winioctl_guid("af9b60a0-1431-4f62-bc68-3311714a69ad");
    EXPECT_EQ(0, std::memcmp(REAL_LDM_METADATA_GUID, meta.data(), 16));
    EXPECT_EQ(0, std::memcmp(REAL_LDM_DATA_GUID, data.data(), 16));
    MemDevice dev(make_gpt_dynamic_disk(meta.data(), data.data()));
    LdmInfo info;
    ASSERT_EQ(OK, parse_ldm_database(&dev, info));
    EXPECT_TRUE(info.is_gpt);
    EXPECT_EQ(size_t(2), info.layouts.size());
}

/* ===================================================================== */
/* M-046 (diff-to-fix): the LDM GPT-path lookup must use the DEVICE      */
/* geometry — on 4Kn the GPT header lives at byte 4096 (LBA 1), the      */
/* entries at LBA 2, and the metadata partition's last block at          */
/* last_lba*4096. The hardcoded *512 read the header mid-sector-0 and    */
/* made LDM-over-GPT undiscoverable on every 4Kn disk.                   */
/* ===================================================================== */

namespace {

class MemDevice4k : public IStorageDevice {
public:
    explicit MemDevice4k(std::vector<u8> d) : d_(std::move(d)) {
        g_.logical_sector = 4096;
        g_.physical_sector = 4096;
        g_.total_bytes = d_.size();
        g_.sector_count = d_.size() / 4096;
    }
    u32 read_at(u64 off, void* buf, u32 len) override {
        if (off > d_.size() || len > d_.size() - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        memcpy(buf, d_.data() + off, len);
        return OK;
    }
    const Geometry& geometry() const override { return g_; }

private:
    Geometry g_{};
    std::vector<u8> d_;
};

/* The standard 512-byte GPT dynamic-disk fixture re-framed for 4Kn:
 * same LDM database (LDM's own coordinates stay 512-byte LBAs — kernel
 * sector semantics), GPT header at byte 4096, entries at byte 8192, and
 * the metadata partition's primary PRIVHEAD at an 8-aligned 512-LBA so
 * its byte position is exactly last_lba * 4096. */
std::vector<u8> make_gpt_dynamic_disk_4kn(const u8* meta_guid, const u8* data_guid) {
    StripeDisk sd;
    const u64 total_secs = 65536; /* 32 MiB in 512-LBAs (the fixture's disk) */
    const u64 cfg_start = total_secs - 2048;
    const u64 ldm_meta_last = 65528; /* 8-aligned: 4Kn LBA 8191 exactly */
    sd.fx.write_privhead(ldm_meta_last);
    std::vector<u8> disk = sd.fx.take();
    u8* mbr = disk.data();
    mbr[0x1BE + 4] = 0xEE;
    p32be(mbr + 0x1BE + 8, 1);
    p32be(mbr + 0x1BE + 12, u32(total_secs - 1));
    /* GPT header at 4Kn LBA 1 = byte 4096 */
    u8* h = disk.data() + 4096;
    std::memcpy(h, "EFI PART", 8);
    put32le(h + 8, 0x00010000);
    put32le(h + 12, 92);
    put64le(h + 24, 1);   /* first usable (4Kn LBAs) */
    put64le(h + 32, 8191);/* last usable */
    put64le(h + 40, 2);   /* entries at 4Kn LBA 2 */
    put64le(h + 48, 8190);
    put64le(h + 72, 2);   /* 2 entries */
    put32le(h + 80, 2);
    put32le(h + 84, 128);
    std::vector<u8> entries(2 * 128, 0);
    std::memcpy(entries.data() + 0, meta_guid, 16);
    put64le(entries.data() + 32, cfg_start / 8);  /* 512-LBA 63488 -> 4Kn 7936 */
    put64le(entries.data() + 40, ldm_meta_last / 8); /* 65528 -> 4Kn 8191 */
    std::memcpy(entries.data() + 128, data_guid, 16);
    put64le(entries.data() + 128 + 32, 2048 / 8);
    put64le(entries.data() + 128 + 40, 4096 / 8);
    put32le(h + 88, crc32(entries.data(), entries.size()));
    std::memcpy(disk.data() + 2 * 4096, entries.data(), entries.size());
    put32le(h + 16, crc32(h, 92));
    return disk;
}

} // namespace

TEST(Ldm, GptPathFindsMetadata4Kn) {
    /* Fail-before: the hardcoded *512 GPT-path lookup read the header at
     * byte 512 (mid-sector-0 zeros) -> ERR_BAD_MAGIC; no dynamic-disk
     * recovery on any 4Kn GPT disk while cr_table_parse succeeded. */
    MemDevice4k dev(make_gpt_dynamic_disk_4kn(REAL_LDM_METADATA_GUID, REAL_LDM_DATA_GUID));
    LdmInfo info;
    const u32 rc = parse_ldm_database(&dev, info);
    ASSERT_EQ(OK, rc) << err_text(rc);
    EXPECT_TRUE(info.is_gpt);
    EXPECT_TRUE(info.found);
    EXPECT_EQ(size_t(2), info.layouts.size());
    /* partition tail (8191*4096) + config+1856; config+2047 carries the
     * stripe fixture's own bytes here */
    EXPECT_GE(info.stats.privheads_matched, u64(2));
}

/* ---------------------------------------------------------------- */
/* M-016: TOCBLOCK candidate set/order (og: {config+2, config+0,     */
/* config+1}, first valid wins — LdmManager FUN_180072ad0)           */
/* ---------------------------------------------------------------- */

namespace {

constexpr u64 STRIPE_CFG_START = 65536 - 2048; /* StripeDisk's config LBA */

void m016_write_tocblock(std::vector<u8>& d, u64 cfg_off, u64 bitmap1_start) {
    u8* toc = d.data() + (STRIPE_CFG_START + cfg_off) * 512;
    memset(toc, 0, 512);
    memcpy(toc, "TOCBLOCK", 8);
    memcpy(toc + 0x24, "config", 6);
    p64be(toc + 0x2E, bitmap1_start); /* bitmap1 start -> VMDB anchor */
    p64be(toc + 0x36, 1600);
    memcpy(toc + 0x46, "log", 3);
    p64be(toc + 0x50, 1617);
    p64be(toc + 0x58, 400);
}

void m016_erase_tocblock(std::vector<u8>& d, u64 cfg_off) {
    memset(d.data() + (STRIPE_CFG_START + cfg_off) * 512, 0, 512);
}

} // namespace

TEST(Ldm, TocblockAtConfigPlus0) {
    /* M-016(a): the og probes config+0 as its second TOCBLOCK candidate
     * (FUN_180072ad0 :59584). A disk whose only valid TOCBLOCK sits at
     * config+0 must parse; the old candidate list {1,2,2045,2046} never
     * probed offset 0 and the database failed with ERR_BAD_CHECKSUM —
     * every volume on such a disk was missed. */
    StripeDisk sd;
    auto d = sd.fx.take();
    m016_erase_tocblock(d, 1);
    m016_erase_tocblock(d, 2);
    m016_write_tocblock(d, 0, 17);
    MemDevice dev(std::move(d));
    LdmInfo info;
    const u32 rc0 = parse_ldm_database(&dev, info);
    ASSERT_EQ(OK, rc0) << err_text(rc0);
    EXPECT_TRUE(info.found);
    EXPECT_EQ(u64(17), info.toc.bitmap1_start);
    EXPECT_EQ(size_t(2), info.layouts.size());
}

TEST(Ldm, TocblockPrefersPlus2OnConflict) {
    /* M-016(c): on two valid-but-conflicting copies at +1 and +2 the og
     * follows the +2 copy (probed first). The +2 copy anchors the VMDB
     * at config+18 (committed_seq 99); the +1 copy anchors at +17
     * (committed_seq 42). The core's old order {1,2,...} picked +1 and
     * followed the stale VMDB — different database state on torn-write
     * disks. */
    StripeDisk sd;
    auto d = sd.fx.take();
    /* VMDB@17 already written by the fixture (committed_seq 42) */
    u8* vmdb18 = d.data() + (STRIPE_CFG_START + 18) * 512;
    memcpy(vmdb18, d.data() + (STRIPE_CFG_START + 17) * 512, 512);
    p64be(vmdb18 + 0x75, 99); /* distinct committed sequence */
    m016_write_tocblock(d, 1, 17); /* stale copy first in the old order */
    m016_write_tocblock(d, 2, 18); /* og-preferred copy */
    MemDevice dev(std::move(d));
    LdmInfo info;
    const u32 rc1 = parse_ldm_database(&dev, info);
    ASSERT_EQ(OK, rc1) << err_text(rc1);
    EXPECT_TRUE(info.found);
    EXPECT_EQ(u64(18), info.toc.bitmap1_start);
    EXPECT_EQ(u64(99), info.vm.committed_seq);
}

TEST(Ldm, TocblockTailCopies) {
    /* M-016(b) pin: the end-of-config tail copies (config+2045/+2046)
     * are a core superset the og does not probe; a disk whose only
     * TOCBLOCK sits at config+2045 must keep parsing after the candidate
     * reorder (kernel ldm.c tail-copy semantics preserved). */
    StripeDisk sd;
    auto d = sd.fx.take();
    m016_erase_tocblock(d, 1);
    m016_erase_tocblock(d, 2);
    m016_write_tocblock(d, 2045, 17);
    MemDevice dev(std::move(d));
    LdmInfo info;
    const u32 rc2 = parse_ldm_database(&dev, info);
    ASSERT_EQ(OK, rc2) << err_text(rc2);
    EXPECT_TRUE(info.found);
    EXPECT_EQ(size_t(2), info.layouts.size());
}

TEST(Ldm, OversizeConfigRejected) {
    /* N-012a: the og's LdmManager gate - a PRIVHEAD declaring a config
     * region over 1 MiB (>= 0x801 sectors) aborts the disk
     * ("ConfigSize[%I64d] larger than 1m ?." -> rc 0xfffffff8,
     * LdmManager_decompiled.c:59748-59759). The core used to clamp the
     * read to 4 MiB and keep walking; parity closes on the gate. */
    StripeDisk sd;
    auto d = sd.fx.take();
    /* patch every PRIVHEAD copy (LBA 6 + the two backups) to declare
     * 0x801 sectors - the smallest size the og refuses */
    const u64 cfg_start = 65536 - 2048;
    for (u64 lba : {u64(6), cfg_start + 1856, cfg_start + 2047}) {
        u8* p = d.data() + lba * 512;
        ASSERT_EQ(0, memcmp(p, "PRIVHEAD", 8));
        p64be(p + 0x133, 0x801);
    }
    MemDevice dev(std::move(d));
    LdmInfo info;
    EXPECT_EQ(ERR_BAD_CHECKSUM, parse_ldm_database(&dev, info));
    EXPECT_FALSE(info.found);
}

TEST(Ldm, ConfigSizeExactlyOneMebibyteAccepted) {
    /* N-012a boundary pin: 2048 sectors (1 MiB exactly) is the largest
     * size the og accepts (`< 0x801`); the standard fixture value must
     * keep parsing after the gate lands. */
    StripeDisk sd;
    MemDevice dev(sd.fx.take());
    LdmInfo info;
    ASSERT_EQ(OK, parse_ldm_database(&dev, info)) << "1 MiB config must parse";
    EXPECT_TRUE(info.found);
    EXPECT_EQ(size_t(2), info.layouts.size());
}

TEST(Ldm, ZeroInterleaveDefaultsTo64K) {
    /* N-012b: the og defaults a zero interleave to 64 KiB (interleave
     * 0 -> 0x10000 bytes, LdmManager_decompiled.c:47665 + :68631-68637;
     * the sector form falls back to 0x80 = 128 sectors, :68638-68641).
     * A component VBLK with chunk_size 0 must yield a readable
     * default-geometry striped volume, not a refusal. */
    LdmFixture fx(65536);
    fx.set_disk(1000, "01234567-89AB-CDEF-0123-456789ABCDEF");
    fx.add_dgr4(1, "TestPCDg0");
    fx.add_dsk4(1000, "Disk1", "01234567-89AB-CDEF-0123-456789ABCDEF");
    fx.add_volume({2001, "Stripe1", 3, STRIPE_MEMBER_SIZE * 2, 1});
    fx.add_component({3001, "Stripe1-01", LDM_COMP_STRIPE, 2, 2001, 0, 2});
    fx.add_partition({4001, "Disk1-02", STRIPE_A_START, STRIPE_MEMBER_SIZE, 0, 3001, 0, 1000});
    fx.add_partition({4002, "Disk1-03", STRIPE_B_START, STRIPE_MEMBER_SIZE, 0, 3001, 1, 1000});
    fx.finish();
    /* marker pattern on both columns: row-major like the interleave test */
    auto img = fx.take();
    for (u64 row = 0; row < STRIPE_MEMBER_SIZE / 128; row++) {
        memset(img.data() + (STRIPE_A_START + row * 128) * 512, int(0xA0 + (row & 0x0F)),
               size_t(128 * 512));
        memset(img.data() + (STRIPE_B_START + row * 128) * 512, int(0xB0 + (row & 0x0F)),
               size_t(128 * 512));
    }
    MemDevice dev(std::move(img));
    LdmInfo info;
    ASSERT_EQ(OK, parse_ldm_database(&dev, info)) << err_text(OK);
    ASSERT_TRUE(info.found);
    const LdmVolumeLayout* lay = find_layout(info, 2001);
    ASSERT_NE(nullptr, lay);
    EXPECT_EQ(LdmVolumeKind::Striped, lay->kind);
    EXPECT_TRUE(lay->complete) << "zero-interleave volume must be complete on the 64 KiB default";
    EXPECT_EQ(u64(128), lay->chunk_sectors) << "og default: 0x80 sectors = 64 KiB";
    /* the read path must resolve through the defaulted geometry */
    LdmVolumeDevice vol(*lay, [&](const LdmGuid& g) -> IStorageDevice* {
        return g == info.disks[0].guid ? &dev : nullptr;
    });
    u8 first = 0;
    ASSERT_EQ(OK, vol.read_at(0, &first, 1));
    EXPECT_EQ(0xA0, first) << "volume byte 0 comes from column 0, row 0";
    ASSERT_EQ(OK, vol.read_at(128 * 512, &first, 1));
    EXPECT_EQ(0xB0, first) << "second chunk comes from column 1 (128-sector interleave)";
}

} // namespace table
} // namespace cr
