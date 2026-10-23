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
#include "test_fixtures.hpp"
#include <gtest/gtest.h>
#include <cstring>

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
            {2000, "Raid1", 4 /* read policy RAID */, R5_PART_SIZE / R5_CHUNK * R5_CHUNK * 2, 1});
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
        fx.add_volume({2000, "Mirror1", 1, MIRROR_PART_SIZE, 2});
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
        fx.add_volume({2000, "Span1", 1, SPAN_PART_SIZE * 2, 1});
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
