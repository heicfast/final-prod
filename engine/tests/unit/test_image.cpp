/* test_image.cpp - S-030 disk-image scan targets: flat / VMDK / VHD / VHDX.
 *
 * Two proof layers:
 *  1. Structure-level tests with in-memory builders (MemImageIo) - grain
 *     translation, BAT states, log overlay, error taxonomy, session
 *     roundtrip, MBR scan end-to-end.
 *  2. Real-world compatibility tests against images produced by qemu-img
 *     10.0.13 (the fixtures under tests/data/images, byte-written via
 *     qemu-io; generation commands in each test's comment) - proving the
 *     parsers read what real tools write, not just what we write.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/storage/cr_image.h"
#include "cr/orchestrator/cr_scan.h"
#include "cr/session/cr_records.h"
#include <gtest/gtest.h>
#include <cstring>
#include <map>
#include <string>
#include <vector>

namespace cr {
namespace storage {
namespace test {

using namespace cr::storage;

/* ------------------------------------------------------------------ */
/* in-memory ImageFileIo                                               */
/* ------------------------------------------------------------------ */

class MemImageIo final : public ImageFileIo {
public:
    u32 open(const std::u16string& path, u32* err) override {
        const auto it = files_.find(path);
        if (it == files_.end()) {
            if (err)
                *err = ERR_OPEN_FAILED;
            return 0;
        }
        next_ += 1;
        open_[next_] = path;
        return u32(next_); /* MSVC C4244: the fixture counter is tiny */
    }
    u32 read(u64 file, u64 offset, void* buf, u32 len) override {
        const auto h = open_.find(file);
        if (h == open_.end())
            return ERR_BAD_PARAMETER;
        const std::vector<u8>& data = files_[h->second];
        if (offset > data.size() || len > data.size() - offset)
            return ERR_TRUNCATED;
        std::memcpy(buf, data.data() + offset, len);
        return OK;
    }
    u64 size(u64 file) const override {
        const auto h = open_.find(file);
        if (h == open_.end())
            return 0;
        return files_.at(h->second).size();
    }
    void close(u64 file) override { open_.erase(file); }

    void add(const std::u16string& path, std::vector<u8> bytes) { files_[path] = std::move(bytes); }
    std::vector<u8>& bytes(const std::u16string& path) { return files_[path]; }

private:
    std::map<std::u16string, std::vector<u8>> files_;
    std::map<u64, std::u16string> open_;
    u64 next_ = 0;
};

std::u16string p(const char* s) {
    return std::u16string(s, s + std::strlen(s));
}

void put16(std::vector<u8>& v, u64 off, u16 x) {
    v[size_t(off)] = u8(x);
    v[size_t(off + 1)] = u8(x >> 8);
}
void put32(std::vector<u8>& v, u64 off, u32 x) {
    for (int i = 0; i < 4; i++)
        v[size_t(off + i)] = u8(x >> (8 * i));
}
void put64(std::vector<u8>& v, u64 off, u64 x) {
    for (int i = 0; i < 8; i++)
        v[size_t(off + i)] = u8(x >> (8 * i));
}
void be32put(std::vector<u8>& v, u64 off, u32 x) {
    for (int i = 0; i < 4; i++)
        v[size_t(off + i)] = u8(x >> (8 * (3 - i)));
}
void be64put(std::vector<u8>& v, u64 off, u64 x) {
    for (int i = 0; i < 8; i++)
        v[size_t(off + i)] = u8(x >> (8 * (7 - i)));
}

/* A minimal valid MBR with one FAT16 partition entry (the scan fixture). */
std::vector<u8> make_mbr(u64 part_start_lba, u64 part_sectors) {
    std::vector<u8> mbr(512, 0);
    mbr[0] = 0xEB;
    mbr[1] = 0x3C;
    mbr[2] = 0x90; /* boot code marker */
    std::memcpy(mbr.data() + 3, "MSWIN4.1", 8);
    mbr[0x1FE] = 0x55;
    mbr[0x1FF] = 0xAA;
    /* partition 1: type 0x06 FAT16 */
    mbr[0x1BE + 0] = 0x80;
    mbr[0x1BE + 4] = 0x06;
    put32(mbr, 0x1BE + 8, u32(part_start_lba));
    put32(mbr, 0x1BE + 12, u32(part_sectors));
    return mbr;
}

/* ------------------------------------------------------------------ */
/* flat                                                                */
/* ------------------------------------------------------------------ */

TEST(Image, FlatFileScansMbr) {
    MemImageIo io;
    std::vector<u8> disk(2 * 1024 * 1024, 0);
    const auto mbr = make_mbr(1, 2048);
    std::memcpy(disk.data(), mbr.data(), 512);
    io.add(p("disk.img"), std::move(disk));
    u32 err = 0;
    auto dev = open_image(io, p("disk.img"), &err);
    ASSERT_NE(dev, nullptr);
    EXPECT_EQ(err, OK);
    EXPECT_EQ(dev->geometry().logical_sector, 512u);
    EXPECT_EQ(dev->size(), 2u * 1024 * 1024);
    u8 buf[512] = {0};
    EXPECT_EQ(dev->read_at(510, buf + 510 - 510, 2), OK); /* sig bytes */
    /* full MBR read-back */
    u8 m[512] = {0};
    EXPECT_EQ(dev->read_at(0, m, 512), OK);
    EXPECT_EQ(m[0x1FE], 0x55);
    EXPECT_EQ(m[0x1FF], 0xAA);
    /* bounds */
    EXPECT_EQ(dev->read_at(dev->size() - 511, buf, 512), ERR_RANGE_OUT_OF_BOUNDS);
    /* the orchestrator accepts the image as a scan device (end-to-end) */
    orchestrator::ScanConfig cfg;
    cfg.run_carve = false;
    cfg.threads = 1;
    orchestrator::ScanStats stats;
    tree::ResultTree tree;
    orchestrator::ScanController ctrl(tree, cfg);
    const u32 rc = ctrl.run(dev.get(), nullptr, nullptr, nullptr, stats);
    EXPECT_EQ(rc, OK); /* scan completed over the image device */
}

TEST(Image, FlatRejectsEmptyAndMissing) {
    MemImageIo io;
    io.add(p("empty.img"), std::vector<u8>());
    u32 err = 0;
    EXPECT_EQ(open_image(io, p("empty.img"), &err), nullptr);
    EXPECT_EQ(err, ERR_TRUNCATED);
    EXPECT_EQ(open_image(io, p("missing.img"), &err), nullptr);
    EXPECT_EQ(err, ERR_OPEN_FAILED);
}

TEST(Image, ImageRejectsTruncatedFile) {
    MemImageIo io;
    io.add(p("tiny.vhdx"), std::vector<u8>(1000, 0xAB)); /* < 1 MiB: no room for the header */
    u32 err = 0;
    EXPECT_EQ(open_image(io, p("tiny.vhdx"), &err), nullptr);
    EXPECT_EQ(err, ERR_TRUNCATED);
    io.add(p("tiny.vhd"), std::vector<u8>(200, 0xAB)); /* < 512: no footer */
    EXPECT_EQ(open_image(io, p("tiny.vhd"), &err), nullptr);
    EXPECT_EQ(err, ERR_TRUNCATED);
    io.add(p("tiny.vmdk"), std::vector<u8>(100, 0xAB)); /* < 512: no header */
    EXPECT_EQ(open_image(io, p("tiny.vmdk"), &err), nullptr);
    EXPECT_EQ(err, ERR_TRUNCATED);
}

TEST(Image, EdiskRefusedAsRegisteredExclusion) {
    MemImageIo io;
    io.add(p("backup.edisk"), std::vector<u8>(4096, 0xAB));
    u32 err = 0;
    EXPECT_EQ(open_image(io, p("backup.edisk"), &err), nullptr);
    EXPECT_EQ(err, ERR_UNSUPPORTED);
}

TEST(Image, BadMagicVmdkRefusedNotFlatFallback) {
    MemImageIo io;
    io.add(p("not-a.vmdk"), std::vector<u8>(8192, 0x41)); /* 'A's, no KDMV/descriptor */
    u32 err = 0;
    EXPECT_EQ(open_image(io, p("not-a.vmdk"), &err), nullptr);
    EXPECT_EQ(err, ERR_BAD_MAGIC); /* the og: corrupt VMDK is an error, never flat */
}

/* ------------------------------------------------------------------ */
/* VMDK builders                                                       */
/* ------------------------------------------------------------------ */

/* Builds a monolithicSparse image: capacity sectors, one grain directory
 * entry per 512-grain table, grains of grain_sectors. */
struct VmdkBuilt {
    std::vector<u8> bytes;
    u64 data_start_sector; /* first sector after metadata */
};

VmdkBuilt build_monolithic_sparse(u64 capacity_sectors, u64 grain_sectors,
                                  const std::map<u64, std::vector<u8>>& grains,
                                  const std::string& create_type = "monolithicSparse",
                                  const std::string& parent_hint = "") {
    const u64 grains_total = (capacity_sectors + grain_sectors - 1) / grain_sectors;
    const u32 gtes_per_gt = 512;
    const u64 gdes = (grains_total + gtes_per_gt - 1) / gtes_per_gt;
    const u64 desc_sector = 1, desc_sectors = 4;
    const u64 gd_sector = desc_sector + desc_sectors;
    const u64 gt_sector = gd_sector + gdes;
    const u64 data_sector = (gt_sector + gdes * 4 /* each GT is 512 entries */ + 7) & ~u64(7);

    std::string desc = "# Disk DescriptorFile\nversion=1\nCID=fffffffe\n";
    desc +=
        "parentCID=" + (parent_hint.empty() ? std::string("ffffffff") : std::string("fffffffd")) +
        "\n";
    desc += "createType=\"" + create_type + "\"\n\n";
    desc += "# Extent description\n";
    desc += "RW " + std::to_string(capacity_sectors) + " SPARSE \"img.vmdk\"\n\n";
    desc += "# The Disk Data Base\n#DDB\n\n";
    if (!parent_hint.empty())
        desc = "# Disk DescriptorFile\nversion=1\nCID=fffffffe\nparentCID=fffffffd\n"
               "createType=\"" +
               create_type + "\"\nparentFileNameHint=\"" + parent_hint +
               "\"\n\n# Extent description\nRW " + std::to_string(capacity_sectors) +
               " SPARSE \"img.vmdk\"\n\n#DDB\n\n";
    while (desc.size() < desc_sectors * 512)
        desc.push_back('\0');

    std::vector<u8> v(size_t(data_sector) * 512, 0);
    /* sparse extent header */
    put32(v, 0x00, 0x564D444B);
    put32(v, 0x04, 1); /* version */
    put32(v, 0x08, 3); /* flags: new line detection etc. - irrelevant on read */
    put64(v, 0x0C, capacity_sectors);
    put64(v, 0x14, grain_sectors);
    put64(v, 0x1C, desc_sector);
    put64(v, 0x24, desc_sectors);
    put32(v, 0x2C, gtes_per_gt);
    put64(v, 0x30, 0); /* rgd (absent) */
    put64(v, 0x38, gd_sector);
    put64(v, 0x40, data_sector); /* overhead */
    std::memcpy(v.data() + desc_sector * 512, desc.data(), desc.size());

    /* grain directory + tables */
    u64 next_grain_sector = data_sector;
    std::vector<u32> gte(grains_total, 0);
    for (u64 g = 0; g < grains_total; ++g) {
        const auto it = grains.find(g);
        if (it == grains.end())
            continue; /* unallocated */
        const u64 grain_len = grain_sectors * 512;
        v.resize(size_t(next_grain_sector * 512 + grain_len), 0);
        std::memcpy(v.data() + next_grain_sector * 512, it->second.data(),
                    std::min(grain_len, it->second.size()));
        gte[size_t(g)] = u32(next_grain_sector);
        next_grain_sector += grain_sectors;
    }
    /* GD entries point at per-table sectors; each table holds 512 u32 */
    for (u64 t = 0; t < gdes; ++t) {
        const u64 gt_off = (gt_sector + t * 4) * 512;
        put32(v, (gd_sector + t) * 512, u32(gt_sector + t * 4)); /* GDE -> GT sector */
        for (u32 e = 0; e < gtes_per_gt; ++e) {
            const u64 idx = t * gtes_per_gt + e;
            const u32 val = idx < gte.size() ? gte[size_t(idx)] : 0;
            put32(v, gt_off + u64(e) * 4, val);
        }
    }
    return {std::move(v), data_sector};
}

TEST(Image, VmdkSparseExtentReads) {
    MemImageIo io;
    const u64 grain = 128; /* 64 KiB grains */
    std::map<u64, std::vector<u8>> grains;
    std::vector<u8> g0(64 * 1024, 0xAB);
    std::vector<u8> g3(64 * 1024, 0xCD);
    grains[0] = std::move(g0);
    grains[3] = std::move(g3);
    const auto built = build_monolithic_sparse(8192, grain, grains);
    io.add(p("img.vmdk"), built.bytes);
    u32 err = 0;
    auto dev = open_image(io, p("img.vmdk"), &err);
    ASSERT_NE(dev, nullptr);
    EXPECT_EQ(err, OK);
    EXPECT_EQ(dev->size(), 8192u * 512);

    u8 buf[64 * 1024];
    /* grain 0: pattern at LBA0 */
    EXPECT_EQ(dev->read_at(0, buf, 1024), OK);
    EXPECT_EQ(buf[0], 0xAB);
    EXPECT_EQ(buf[1023], 0xAB);
    /* grain 1 (unallocated): zeros */
    EXPECT_EQ(dev->read_at(64 * 1024, buf, 1024), OK);
    EXPECT_EQ(buf[0], 0);
    /* grain 3: pattern at 3*64KiB */
    EXPECT_EQ(dev->read_at(3 * 64 * 1024, buf, 1024), OK);
    EXPECT_EQ(buf[0], 0xCD);
    /* straddling read: end of grain 3 + start of grain 4 (zeros) */
    EXPECT_EQ(dev->read_at(3 * 64 * 1024 + 64 * 1024 - 512, buf, 1024), OK);
    EXPECT_EQ(buf[0], 0xCD);
    EXPECT_EQ(buf[512], 0);
    /* bounds */
    EXPECT_EQ(dev->read_at(dev->size() - 1, buf, 2), ERR_RANGE_OUT_OF_BOUNDS);
}

TEST(Image, VmdkDifferencingChainReadsParent) {
    /* parent: grain 0 = 0xAB, grain 2 = 0xCD; child: grain 2 = 0xEF only */
    MemImageIo io;
    std::map<u64, std::vector<u8>> pg, cg;
    pg[0] = std::vector<u8>(64 * 1024, 0xAB);
    pg[2] = std::vector<u8>(64 * 1024, 0xCD);
    cg[2] = std::vector<u8>(64 * 1024, 0xEF);
    io.add(p("parent.vmdk"), build_monolithic_sparse(8192, 128, pg).bytes);
    /* the child references the parent via parentFileNameHint (relative) */
    io.add(p("child.vmdk"),
           build_monolithic_sparse(8192, 128, cg, "monolithicSparse", "parent.vmdk").bytes);
    u32 err = 0;
    auto dev = open_image(io, p("child.vmdk"), &err);
    ASSERT_NE(dev, nullptr);
    EXPECT_EQ(err, OK);
    u8 buf[512];
    /* grain 0 unallocated in the child -> parent's 0xAB */
    EXPECT_EQ(dev->read_at(0, buf, 512), OK);
    EXPECT_EQ(buf[0], 0xAB);
    /* grain 2 allocated in the child -> 0xEF overrides the parent */
    EXPECT_EQ(dev->read_at(2 * 64 * 1024, buf, 512), OK);
    EXPECT_EQ(buf[0], 0xEF);
    /* grain 1 nowhere: zeros */
    EXPECT_EQ(dev->read_at(64 * 1024, buf, 512), OK);
    EXPECT_EQ(buf[0], 0);
}

TEST(Image, VmdkStreamOptimizedRefused) {
    MemImageIo io;
    const auto built = build_monolithic_sparse(1024, 128, {}, "streamOptimized");
    io.add(p("stream.vmdk"), built.bytes);
    u32 err = 0;
    EXPECT_EQ(open_image(io, p("stream.vmdk"), &err), nullptr);
    EXPECT_EQ(err, ERR_UNSUPPORTED);
}

/* ------------------------------------------------------------------ */
/* VHD builders                                                        */
/* ------------------------------------------------------------------ */

u32 vhd_checksum(const std::vector<u8>& blk, size_t len, size_t csum_off) {
    std::vector<u8> tmp(blk.begin(), blk.begin() + len);
    std::memset(tmp.data() + csum_off, 0, 4);
    u32 sum = 0;
    for (size_t i = 0; i < len; ++i)
        sum += tmp[i];
    return ~sum;
}

/* dynamic VHD: block_size bytes per block, blocks map {index: {in_block:
 * bytes}}, current_size bytes total. */
std::vector<u8> build_dynamic_vhd(u64 current_size, u32 block_size,
                                  const std::map<u64, std::map<u64, std::vector<u8>>>& blocks,
                                  u32 disk_type = 3, const std::u16string* parent_path = nullptr) {
    const u32 max_entries = u32((current_size + block_size - 1) / block_size) + 1;
    const u64 bat_off = 2048;
    const u64 data_off = (bat_off + u64(max_entries) * 4 + 511) & ~u64(511);

    std::vector<u8> v;
    /* footer copy at 0 (qemu layout), then dynamic header at 512 */
    v.resize(data_off, 0);

    /* dynamic disk header at offset 512 (footer->dataOffset points here) */
    std::memcpy(v.data() + 512, "cxsparse", 8);
    be64put(v, 512 + 0x08, 0xFFFFFFFFFFFFFFFFULL);
    be64put(v, 512 + 0x10, bat_off);    /* table offset */
    be32put(v, 512 + 0x18, 0x00010000); /* version */
    be32put(v, 512 + 0x1C, max_entries);
    be32put(v, 512 + 0x20, block_size);
    be32put(v, 512 + 0x24, 0); /* checksum, fixed below */

    /* BAT: all 0xFFFFFFFF (unallocated), filled below */
    for (u32 i = 0; i < max_entries; ++i)
        be32put(v, bat_off + u64(i) * 4, 0xFFFFFFFF);

    u64 next_block = data_off;
    for (const auto& kv : blocks) {
        const u64 block = kv.first;
        /* 512-byte sector bitmap: mark every written sector */
        std::vector<u8> bitmap(512, 0);
        std::vector<u8> data(block_size, 0);
        for (const auto& wr : kv.second) {
            std::memcpy(data.data() + wr.first, wr.second.data(), wr.second.size());
            const u64 first_sec = wr.first / 512;
            const u64 nsec = (wr.second.size() + 511) / 512;
            for (u64 s = 0; s < nsec; ++s)
                bitmap[size_t((first_sec + s) / 8)] |= u8(1u << ((first_sec + s) % 8));
        }
        v.resize(size_t(next_block) + 512 + block_size, 0);
        std::memcpy(v.data() + next_block, bitmap.data(), 512);
        std::memcpy(v.data() + next_block + 512, data.data(), block_size);
        be32put(v, bat_off + block * 4, u32(next_block / 512));
        next_block += 512 + block_size;
    }
    /* for differencing: parent locator (W2ku, UTF-16BE) appended after blocks */
    if (parent_path) {
        const std::u16string& pp = *parent_path;
        std::vector<u8> loc(pp.size() * 2);
        for (size_t i = 0; i < pp.size(); ++i) {
            loc[2 * i] = u8(pp[i] >> 8);
            loc[2 * i + 1] = u8(pp[i]);
        }
        v.resize(size_t(next_block) + loc.size(), 0);
        std::memcpy(v.data() + next_block, loc.data(), loc.size());
        /* locator entry 0: W2ku, data at next_block */
        be32put(v, 512 + 0x240, 0x57326B75); /* W2ku */
        be32put(v, 512 + 0x248, u32(loc.size()));
        be64put(v, 512 + 0x250, next_block);
        next_block += loc.size();
    }

    /* the footer (tail) + head copy */
    auto write_footer = [&](u64 at) {
        std::memcpy(v.data() + at, "conectix", 8);
        be32put(v, at + 0x08, 0x00000002); /* features */
        be32put(v, at + 0x0C, 0x00010000); /* format version */
        be64put(v, at + 0x10, 512);        /* data offset -> dynamic header */
        be32put(v, at + 0x18, 0);          /* timestamp */
        std::memcpy(v.data() + at + 0x1C, "qemu", 4);
        be32put(v, at + 0x20, 0x00050003);
        std::memcpy(v.data() + at + 0x24, "Wi2k", 4);
        be64put(v, at + 0x28, current_size);
        be64put(v, at + 0x30, current_size);
        be32put(v, at + 0x38, 0x00790411); /* geometry */
        be32put(v, at + 0x3C, disk_type);
        be32put(v, at + 0x40, 0); /* checksum, fixed below */
    };
    v.resize(size_t(next_block) + 512, 0);
    write_footer(0);
    write_footer(next_block);
    /* checksums: each footer over its own 512 bytes, the dynamic header
     * over its 1024 (checksum fields zeroed during computation) */
    be32put(v, 0x40, vhd_checksum(v, 512, 0x40));
    be32put(v, next_block + 0x40,
            vhd_checksum(std::vector<u8>(v.begin() + next_block, v.end()), 512, 0x40));
    be32put(v, 512 + 0x24,
            vhd_checksum(std::vector<u8>(v.begin() + 512, v.begin() + 512 + 1024), 1024, 0x24));
    return v;
}

TEST(Image, VhdDynamicDisk) {
    MemImageIo io;
    /* 4 MiB disk, 2 MiB blocks: block0 has 0xAB @0, block1 has 0xEF @1MiB */
    std::map<u64, std::map<u64, std::vector<u8>>> blocks;
    blocks[0][0] = std::vector<u8>(4096, 0xAB);
    blocks[1][1024 * 1024] = std::vector<u8>(2048, 0xEF);
    io.add(p("d.vhd"), build_dynamic_vhd(4 * 1024 * 1024, 2 * 1024 * 1024, blocks));
    u32 err = 0;
    auto dev = open_image(io, p("d.vhd"), &err);
    ASSERT_NE(dev, nullptr) << "err=" << err;
    EXPECT_EQ(err, OK);
    EXPECT_EQ(dev->size(), 4u * 1024 * 1024);
    u8 buf[4096];
    EXPECT_EQ(dev->read_at(0, buf, 4096), OK);
    EXPECT_EQ(buf[0], 0xAB);
    /* unwritten region inside block 0: zeros */
    EXPECT_EQ(dev->read_at(64 * 1024, buf, 512), OK);
    EXPECT_EQ(buf[0], 0);
    /* block 1 written region: in-block offset 1 MiB = disk offset 3 MiB */
    EXPECT_EQ(dev->read_at(3 * 1024 * 1024, buf, 512), OK);
    EXPECT_EQ(buf[0], 0xEF);
    /* block 1's unwritten head (2 MiB): zeros inside an allocated block */
    EXPECT_EQ(dev->read_at(2 * 1024 * 1024, buf, 512), OK);
    EXPECT_EQ(buf[0], 0);
    /* bounds */
    EXPECT_EQ(dev->read_at(dev->size() - 256, buf, 512), ERR_RANGE_OUT_OF_BOUNDS);
}

TEST(Image, VhdDifferencingReadsParentOnClearBitmap) {
    MemImageIo io;
    /* parent: block 0 = 0xAB everywhere */
    std::map<u64, std::map<u64, std::vector<u8>>> pb;
    pb[0][0] = std::vector<u8>(2 * 1024 * 1024, 0xAB);
    io.add(p("base.vhd"), build_dynamic_vhd(2 * 1024 * 1024, 2 * 1024 * 1024, pb));
    /* child (differencing): bitmap marks only sector 1 as child-owned */
    std::map<u64, std::map<u64, std::vector<u8>>> cb;
    cb[0][512] = std::vector<u8>(512, 0xCD);
    const std::u16string parent_path = p("base.vhd");
    io.add(p("diff.vhd"), build_dynamic_vhd(2 * 1024 * 1024, 2 * 1024 * 1024, cb, 4, &parent_path));
    u32 err = 0;
    auto dev = open_image(io, p("diff.vhd"), &err);
    ASSERT_NE(dev, nullptr);
    EXPECT_EQ(err, OK);
    u8 buf[512];
    EXPECT_EQ(dev->read_at(0, buf, 512), OK); /* parent (clear bit) */
    EXPECT_EQ(buf[0], 0xAB);
    EXPECT_EQ(dev->read_at(512, buf, 512), OK); /* child (set bit) */
    EXPECT_EQ(buf[0], 0xCD);
    EXPECT_EQ(dev->read_at(1024, buf, 512), OK); /* parent again */
    EXPECT_EQ(buf[0], 0xAB);
}

TEST(Image, VhdBadChecksumRefused) {
    MemImageIo io;
    auto v = build_dynamic_vhd(1024 * 1024, 1024 * 1024, {});
    /* corrupt current size in BOTH footer copies (the tail damage alone is
     * spec-recoverable via the head mirror - the fallback stays; a doubly
     * corrupt footer is refused) */
    v[v.size() - 512 + 0x30] ^= 0xFF;
    v[0x30] ^= 0xFF;
    io.add(p("bad.vhd"), std::move(v));
    u32 err = 0;
    EXPECT_EQ(open_image(io, p("bad.vhd"), &err), nullptr);
    EXPECT_EQ(err, ERR_BAD_CHECKSUM);
}

/* ------------------------------------------------------------------ */
/* VHDX builders                                                       */
/* ------------------------------------------------------------------ */

/* minimal CRC-32C for builders (the production one is in cr_image.cpp) */
struct TestCrc32c {
    u32 t[256];
    TestCrc32c() {
        for (u32 i = 0; i < 256; ++i) {
            u32 c = i;
            for (int k = 0; k < 8; k++)
                c = (c >> 1) ^ (0x82F63B78u & (0u - (c & 1)));
            t[i] = c;
        }
    }
};
u32 test_crc32c(const u8* p, size_t len, size_t skip4_at) {
    static TestCrc32c tab;
    u32 crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < len; ++i) {
        const u8 b = (i >= skip4_at && i < skip4_at + 4) ? 0 : p[i];
        crc = tab.t[(crc ^ b) & 0xFF] ^ (crc >> 8);
    }
    return ~crc;
}

std::vector<u8> build_vhdx(u64 virt_size, u32 block_size, u32 logical_sector,
                           const std::map<u64, std::vector<u8>>& payload,
                           const std::vector<u8>* log_entry = nullptr) {
    /* layout: header section 1 MiB, log region 1 MiB @1MiB, BAT region
     * 1 MiB @2 MiB, metadata region 1 MiB @3 MiB, then payload blocks
     * (1 MiB-aligned). Region/BAT offsets mirror qemu's layout. */
    const u64 log_off = 1 * 1024 * 1024;
    const u64 bat_off = 2 * 1024 * 1024;
    const u64 meta_off = 3 * 1024 * 1024;
    const u64 data_off = 4 * 1024 * 1024;

    const u32 data_blocks = u32((virt_size + block_size - 1) / block_size);
    const u64 chunk_ratio = (u64(1) << 23) * logical_sector / block_size;
    u32 crb = 0;
    while ((u64(1) << (crb + 1)) <= chunk_ratio)
        ++crb;
    const u32 bat_entries = data_blocks + ((data_blocks ? data_blocks - 1 : 0) >> crb);

    std::vector<u8> v;
    v.resize(size_t(data_off));
    /* file identifier */
    std::memcpy(v.data(), "vhdxfile", 8);

    /* header 1 @0x10000 (header 2 left zeroed = invalid CRC -> ignored) */
    {
        std::vector<u8> h(4096, 0);
        put32(h, 0x00, 0x64616568); /* "head" */
        put64(h, 0x08, 7);          /* sequence */
        /* log guid (nonzero so the log machinery engages when present) */
        for (int i = 0; i < 16; i++)
            h[0x30 + i] = u8(0x10 + i);
        put16(h, 0x40, 0);                               /* log version */
        put16(h, 0x42, 1);                               /* file version */
        put32(h, 0x44, log_entry ? 1 * 1024 * 1024 : 0); /* log length */
        put64(h, 0x48, log_off);
        put32(h, 0x04, test_crc32c(h.data(), 4096, 4));
        std::memcpy(v.data() + 0x10000, h.data(), 4096);
    }

    /* region table @0x30000 */
    {
        std::vector<u8> rt(64 * 1024, 0);
        put32(rt, 0x00, 0x69676572); /* "regi" */
        put32(rt, 0x08, 2);          /* entries */
        /* BAT region GUID 2DC27766-F623-4200-... (LE fields) */
        put32(rt, 16 + 0, 0x2DC27766);
        put16(rt, 16 + 4, 0xF623);
        put16(rt, 16 + 6, 0x4200);
        const u8 tail[8] = {0x9D, 0x64, 0x11, 0x5E, 0x9B, 0xFD, 0x4A, 0x08};
        std::memcpy(rt.data() + 16 + 8, tail, 8);
        put64(rt, 16 + 16, bat_off);
        put32(rt, 16 + 24, 1 * 1024 * 1024);
        put32(rt, 16 + 28, 1); /* required */
        /* metadata region GUID 8B7CA206-4790-B9AB-... */
        put32(rt, 48 + 0, 0x8B7CA206);
        put16(rt, 48 + 4, 0x4790);
        put16(rt, 48 + 6, 0x4B9A);
        const u8 mtail[8] = {0xB8, 0xFE, 0x57, 0x5F, 0x05, 0x0F, 0x88, 0x6E};
        std::memcpy(rt.data() + 48 + 8, mtail, 8);
        put64(rt, 48 + 16, meta_off);
        put32(rt, 48 + 24, 1 * 1024 * 1024);
        put32(rt, 48 + 28, 1);
        put32(rt, 4, test_crc32c(rt.data(), 64 * 1024, 4));
        std::memcpy(v.data() + 0x30000, rt.data(), 64 * 1024);
    }

    /* metadata region: header + 3 entries (file params, virtual size,
     * logical sector size) at +64 KiB */
    {
        std::vector<u8> m(64 * 1024 + 32, 0);  /* header + entries + 64 KiB values */
        put64(m, 0x00, 0x617461646174656DULL); /* "metadata" */
        put16(m, 0x0A, 3);                     /* entry count */
        auto entry = [&](size_t slot, u32 g1, u16 g2, u16 g3, u32 moff, u32 mlen) {
            put32(m, 32 + slot * 32 + 0, g1);
            put16(m, 32 + slot * 32 + 4, g2);
            put16(m, 32 + slot * 32 + 6, g3);
            put32(m, 32 + slot * 32 + 16, moff);
            put32(m, 32 + slot * 32 + 20, mlen);
        };
        entry(0, 0xCAA16737, 0xFA36, 0x4D43, 64 * 1024, 8);      /* file params */
        entry(1, 0x2FA54224, 0xCD1B, 0x4876, 64 * 1024 + 8, 8);  /* virt size */
        entry(2, 0x8141BF1D, 0xA96F, 0x4709, 64 * 1024 + 16, 4); /* logical */
        put32(m, 64 * 1024 + 0, block_size);
        put32(m, 64 * 1024 + 4, 0); /* data bits: no parent */
        put64(m, 64 * 1024 + 8, virt_size);
        put32(m, 64 * 1024 + 16, logical_sector);
        std::memcpy(v.data() + meta_off, m.data(), m.size());
    }

    /* BAT */
    {
        std::vector<u8> bat(u64(bat_entries) * 8, 0);
        u64 next_payload = data_off;
        for (u32 b = 0; b < data_blocks; ++b) {
            const auto it = payload.find(b);
            if (it == payload.end())
                continue; /* NOT_PRESENT: zeros */
            v.resize(size_t(next_payload) + block_size, 0);
            std::memcpy(v.data() + next_payload, it->second.data(),
                        std::min<u64>(block_size, it->second.size()));
            const u32 slot = b + (b >> crb);
            put64(bat, u64(slot) * 8, next_payload | 6 /* FULLY_PRESENT */);
            next_payload += block_size;
        }
        std::memcpy(v.data() + bat_off, bat.data(), bat.size());
    }

    /* log region (one pre-built entry, when supplied); extend-only: the
     * region itself is already inside the file, never truncate past it */
    if (log_entry) {
        if (v.size() < size_t(log_off) + log_entry->size())
            v.resize(size_t(log_off) + log_entry->size(), 0);
        std::memcpy(v.data() + log_off, log_entry->data(), log_entry->size());
    }
    return v;
}

TEST(Image, VhdxDynamicReads) {
    MemImageIo io;
    std::map<u64, std::vector<u8>> payload;
    payload[0] = std::vector<u8>(64 * 1024, 0xAB);
    payload[2] = std::vector<u8>(64 * 1024, 0xCD);
    io.add(p("d.vhdx"), build_vhdx(3 * 1024 * 1024, 1024 * 1024, 512, payload));
    u32 err = 0;
    auto dev = open_image(io, p("d.vhdx"), &err);
    ASSERT_NE(dev, nullptr);
    EXPECT_EQ(err, OK);
    EXPECT_EQ(dev->size(), 3u * 1024 * 1024);
    u8 buf[4096];
    EXPECT_EQ(dev->read_at(0, buf, 512), OK);
    EXPECT_EQ(buf[0], 0xAB);
    EXPECT_EQ(dev->read_at(2 * 1024 * 1024, buf, 512), OK);
    EXPECT_EQ(buf[0], 0xCD);
    /* unallocated block 1: zeros */
    EXPECT_EQ(dev->read_at(1024 * 1024, buf, 512), OK);
    EXPECT_EQ(buf[0], 0);
    /* bounds */
    EXPECT_EQ(dev->read_at(dev->size() - 100, buf, 512), ERR_RANGE_OUT_OF_BOUNDS);
}

TEST(Image, VhdxLogOverlayServesUnflushedWrites) {
    /* a "crashed" VHDX: the log journals a write to block 0's data sector
     * that the file body does not carry; the overlay must serve it */
    const u64 file_data_off = 4 * 1024 * 1024; /* block 0 payload */
    std::map<u64, std::vector<u8>> payload;
    payload[0] = std::vector<u8>(1024 * 1024, 0xAB);

    /* log entry: header + 1 data descriptor + 1 data sector (8 KiB) */
    std::vector<u8> entry(8192, 0);
    put32(entry, 0x00, 0x65676F6C); /* "loge" */
    put32(entry, 0x08, 8192);       /* entry length */
    put64(entry, 0x10, 1);          /* sequence */
    put32(entry, 0x18, 1);          /* descriptor count */
    for (int i = 0; i < 16; i++)
        entry[0x20 + i] = u8(0x10 + i); /* log guid (matches header) */
    /* flushed/last file offsets must be <= the real file size (a larger
     * value means the image was truncated - qemu's refusal rule) */
    put64(entry, 0x30, 5 * 1024 * 1024); /* flushed */
    put64(entry, 0x38, 5 * 1024 * 1024); /* last */
    /* VHDXLogDescriptor layout (qemu/spec): sig@0x00, trailing u32@0x04,
     * leading u64@0x08, file_offset u64@0x10, sequence u64@0x18 */
    const u8 new_data[16] = {0xEF, 0xEF, 0xEF, 0xEF, 0xEF, 0xEF, 0xEF, 0xEF,
                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    put32(entry, 0x40 + 0, 0x63736564);                     /* "desc" */
    std::memcpy(entry.data() + 0x40 + 4, new_data + 12, 4); /* trailing */
    std::memcpy(entry.data() + 0x40 + 8, new_data, 8);      /* leading */
    put64(entry, 0x40 + 0x10, file_data_off);               /* file offset (4KB aligned) */
    put64(entry, 0x40 + 0x18, 1);                           /* sequence */
    /* data sector @4096: "data" + seq hi + 4084 bytes + seq lo */
    put32(entry, 4096 + 0, 0x61746164);
    put32(entry, 4096 + 4, 0);
    put32(entry, 4096 + 4092, 1);
    std::memset(entry.data() + 4096 + 8, 0xEE, 4084);
    put32(entry, 4, test_crc32c(entry.data(), 8192, 4));

    MemImageIo io;
    io.add(p("crashed.vhdx"), build_vhdx(1024 * 1024, 1024 * 1024, 512, payload, &entry));
    u32 err = 0;
    auto dev = open_image(io, p("crashed.vhdx"), &err);
    ASSERT_NE(dev, nullptr) << "err=" << err;
    EXPECT_EQ(err, OK);
    u8 buf[512];
    /* the logged 0xEF/0xEE pattern must win over the stale 0xAB body */
    EXPECT_EQ(dev->read_at(0, buf, 512), OK);
    EXPECT_EQ(buf[0], 0xEF);
    EXPECT_EQ(buf[8], 0xEE);
    /* past the 4 KiB logged page, the body serves again */
    EXPECT_EQ(dev->read_at(4096, buf, 512), OK);
    EXPECT_EQ(buf[0], 0xAB);
}

TEST(Image, VhdxDifferencingRefusedPrecisely) {
    MemImageIo io;
    auto v = build_vhdx(1024 * 1024, 1024 * 1024, 512, {});
    /* flip file-params data bits to HAS_PARENT */
    v[3 * 1024 * 1024 + 64 * 1024 + 4] = 0x02;
    io.add(p("diff.vhdx"), std::move(v));
    u32 err = 0;
    EXPECT_EQ(open_image(io, p("diff.vhdx"), &err), nullptr);
    EXPECT_EQ(err, ERR_UNSUPPORTED);
}

/* ------------------------------------------------------------------ */
/* classification + session                                            */
/* ------------------------------------------------------------------ */

TEST(Image, Lba0LayoutClassification) {
    std::vector<u8> lba0(4096, 0);
    EXPECT_EQ(classify_lba0(lba0.data(), lba0.size()), Lba0Layout::Unknown);
    lba0[510] = 0x55;
    lba0[511] = 0xAA;
    EXPECT_EQ(classify_lba0(lba0.data(), lba0.size()), Lba0Layout::Mbr);
    std::memcpy(lba0.data() + 512, "EFI PART", 8);
    EXPECT_EQ(classify_lba0(lba0.data(), lba0.size()), Lba0Layout::Gpt);
    EXPECT_EQ(classify_lba0(lba0.data(), 256), Lba0Layout::Unknown); /* short buffer */
}

TEST(Image, ExtensionClassificationMatchesOgDispatch) {
    EXPECT_EQ(classify_extension(p("a.VMDK")), ImageFormat::Vmdk);
    EXPECT_EQ(classify_extension(p("b.vhd")), ImageFormat::Vhd);
    EXPECT_EQ(classify_extension(p("c.VHDX")), ImageFormat::Vhdx);
    EXPECT_EQ(classify_extension(p("d.img")), ImageFormat::Flat);
    EXPECT_EQ(classify_extension(p("e.dmg")), ImageFormat::Flat); /* og: raw fallback */
    EXPECT_EQ(classify_extension(p("f.raw")), ImageFormat::Flat);
    EXPECT_EQ(classify_extension(p("noext")), ImageFormat::Flat);
}

TEST(Image, SessionRoundTripPersistsImagePath) {
    /* the identity-card v3 tail: a scan over an image persists the path;
     * a reloaded session reports it (the resume path warns on mismatch) */
    MemImageIo io;
    std::vector<u8> disk(1024 * 1024, 0);
    const auto mbr = make_mbr(1, 1024);
    std::memcpy(disk.data(), mbr.data(), 512);
    io.add(p("evidence.img"), std::move(disk));
    u32 err = 0;
    auto dev = open_image(io, p("evidence.img"), &err);
    ASSERT_NE(dev, nullptr);

    orchestrator::ScanConfig cfg;
    cfg.run_carve = false;
    cfg.threads = 1;
    std::strcpy(cfg.volume_image, "/evidence/evidence.img");
    orchestrator::ScanStats stats;
    tree::ResultTree tree;
    orchestrator::ScanController ctrl(tree, cfg);
    ASSERT_EQ(ctrl.run(dev.get(), nullptr, nullptr, nullptr, stats), OK);

    /* serialize + reload the volume-info record (the same record shape the
     * controller's save_session emits with the identity card filled) */
    std::vector<u8> blob;
    session::VolumeInfo vi;
    /* the orchestrator's own save path fills the card; here we exercise
     * the record roundtrip directly with the card set like run() does */
    vi.fs_kind = 0;
    vi.logical_sector = 512;
    vi.physical_sector = 512;
    vi.slice_bytes = 1024 * 1024;
    std::strcpy(vi.image, "/evidence/evidence.img");
    ASSERT_EQ(session::serialize_volume_info(vi, blob), OK);
    session::VolumeInfo back;
    ASSERT_EQ(session::deserialize_volume_info(blob.data(), blob.size(), back), OK);
    EXPECT_STREQ(back.image, "/evidence/evidence.img");
    /* v2 records (no tail) reload with an empty path - old sessions stay
     * loadable and simply skip the image-mismatch warning. The v2 prefix
     * sits under the v3 image tail (256) AND the N-066 v4 tail (608). */
    std::vector<u8> v2(blob.begin(), blob.end() - 256 - 608);
    session::VolumeInfo old;
    ASSERT_EQ(session::deserialize_volume_info(v2.data(), v2.size(), old), OK);
    EXPECT_EQ(old.image[0], '\0');
    /* a v3-length record (with image, without the N-066 axes) also loads:
     * the v4 fields stay zeroed - the pre-N-066 file contract */
    std::vector<u8> v3(blob.begin(), blob.end() - 608);
    session::VolumeInfo oldv3;
    ASSERT_EQ(session::deserialize_volume_info(v3.data(), v3.size(), oldv3), OK);
    EXPECT_STREQ(oldv3.image, "/evidence/evidence.img");
    EXPECT_EQ(oldv3.firmware[0], '\0');
    EXPECT_EQ(oldv3.boot_snapshot[0], 0);
}

/* ------------------------------------------------------------------ */
/* real-world compatibility: qemu-img 10.0.13 fixtures                  */
/* ------------------------------------------------------------------ */

namespace {

/* loads a committed fixture; skips (with a clear message) when the data
 * file is absent so the structure tests still run everywhere */
bool load_fixture(const char* name, std::vector<u8>* out) {
    FILE* f = fopen((std::string("data/images/") + name).c_str(), "rb");
    if (!f)
        return false;
    fseek(f, 0, SEEK_END);
    const long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (n <= 0) {
        fclose(f);
        return false;
    }
    out->resize(size_t(n));
    const size_t got = fread(out->data(), 1, out->size(), f);
    fclose(f);
    return got == out->size();
}

class FileImageIo final : public ImageFileIo {
public:
    /* registers a fixture under a name; opens stream from the bytes (the
     * committed fixture IS the file - this exercises the exact on-disk
     * bytes qemu wrote, without depending on CWD layout) */
    void add(const std::u16string& path, std::vector<u8> bytes) { files_[path] = std::move(bytes); }
    u32 open(const std::u16string& path, u32* err) override {
        if (!files_.count(path)) {
            if (err)
                *err = ERR_OPEN_FAILED;
            return 0;
        }
        next_ += 1;
        open_[next_] = path;
        return u32(next_); /* MSVC C4244: the fixture counter is tiny */
    }
    u32 read(u64 file, u64 offset, void* buf, u32 len) override {
        if (!open_.count(file))
            return ERR_BAD_PARAMETER;
        const std::vector<u8>& d = files_[open_[file]];
        if (offset > d.size() || len > d.size() - offset)
            return ERR_TRUNCATED;
        std::memcpy(buf, d.data() + offset, len);
        return OK;
    }
    u64 size(u64 file) const override {
        if (!open_.count(file))
            return 0;
        return files_.at(open_.at(file)).size();
    }
    void close(u64 file) override { open_.erase(file); }

private:
    std::map<std::u16string, std::vector<u8>> files_;
    std::map<u64, std::u16string> open_;
    u64 next_ = 0;
};

} // namespace

/* qemu-img create -f vmdk disk1.vmdk 4M
 * qemu-io -c "write -P 0xAB 0 64k" -c "write -P 0xCD 1M 64k"
 *          -c "write -P 0xEF 3M 8k" -f vmdk disk1.vmdk */
TEST(Image, QemuVmdkCompat) {
    std::vector<u8> raw;
    if (!load_fixture("qemu-disk1.vmdk", &raw)) {
        GTEST_SKIP() << "fixture data/images/qemu-disk1.vmdk not present";
        return;
    }
    FileImageIo io;
    io.add(p("disk1.vmdk"), std::move(raw));
    u32 err = 0;
    auto dev = open_image(io, p("disk1.vmdk"), &err);
    ASSERT_NE(dev, nullptr) << "err=" << err;
    EXPECT_EQ(dev->size(), 4u * 1024 * 1024);
    u8 buf[8192];
    ASSERT_EQ(dev->read_at(0, buf, 8192), OK);
    EXPECT_EQ(buf[0], 0xAB);
    ASSERT_EQ(dev->read_at(1024 * 1024, buf, 8192), OK);
    EXPECT_EQ(buf[0], 0xCD);
    ASSERT_EQ(dev->read_at(3 * 1024 * 1024, buf, 8192), OK);
    EXPECT_EQ(buf[0], 0xEF);
    /* untouched region: zeros */
    ASSERT_EQ(dev->read_at(512 * 1024, buf, 512), OK);
    EXPECT_EQ(buf[0], 0);
}

/* qemu-img create -f vpc disk1.vhd 4M (+ same writes) */
TEST(Image, QemuVhdCompat) {
    std::vector<u8> raw;
    if (!load_fixture("qemu-disk1.vhd", &raw)) {
        GTEST_SKIP() << "fixture data/images/qemu-disk1.vhd not present";
        return;
    }
    FileImageIo io;
    io.add(p("disk1.vhd"), std::move(raw));
    u32 err = 0;
    auto dev = open_image(io, p("disk1.vhd"), &err);
    ASSERT_NE(dev, nullptr) << "err=" << err;
    EXPECT_EQ(dev->size(), 4212736u); /* CHS-rounded current size (qemu: 130/16/63) */
    u8 buf[8192];
    ASSERT_EQ(dev->read_at(0, buf, 8192), OK);
    EXPECT_EQ(buf[0], 0xAB);
    ASSERT_EQ(dev->read_at(1024 * 1024, buf, 8192), OK);
    EXPECT_EQ(buf[0], 0xCD);
    ASSERT_EQ(dev->read_at(3 * 1024 * 1024, buf, 8192), OK);
    EXPECT_EQ(buf[0], 0xEF);
    ASSERT_EQ(dev->read_at(512 * 1024, buf, 512), OK);
    EXPECT_EQ(buf[0], 0);
}

/* qemu-img create -f vhdx disk1.vhdx 4M (+ same writes) */
TEST(Image, QemuVhdxCompat) {
    std::vector<u8> raw;
    if (!load_fixture("qemu-disk1.vhdx", &raw)) {
        GTEST_SKIP() << "fixture data/images/qemu-disk1.vhdx not present";
        return;
    }
    FileImageIo io;
    io.add(p("disk1.vhdx"), std::move(raw));
    u32 err = 0;
    auto dev = open_image(io, p("disk1.vhdx"), &err);
    ASSERT_NE(dev, nullptr) << "err=" << err;
    EXPECT_EQ(dev->size(), 4u * 1024 * 1024);
    u8 buf[8192];
    ASSERT_EQ(dev->read_at(0, buf, 8192), OK);
    EXPECT_EQ(buf[0], 0xAB);
    ASSERT_EQ(dev->read_at(1024 * 1024, buf, 8192), OK);
    EXPECT_EQ(buf[0], 0xCD);
    ASSERT_EQ(dev->read_at(3 * 1024 * 1024, buf, 8192), OK);
    EXPECT_EQ(buf[0], 0xEF);
    ASSERT_EQ(dev->read_at(512 * 1024, buf, 512), OK);
    EXPECT_EQ(buf[0], 0);
}

} // namespace test
} // namespace storage
} // namespace cr
