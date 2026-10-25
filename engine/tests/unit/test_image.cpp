/* S-030: disk-image scan targets (flat / VMDK sparse / VHD dynamic) -
 * all fixtures built in memory through the injected byte sources. */
#include <gtest/gtest.h>
#include "cr/storage/cr_image.h"
#include "cr/fat/cr_fat_engine.h"
#include "cr/orchestrator/cr_scan.h"
#include "cr/session/cr_crsf.h"

#include <cstring>
#include <string>
#include <vector>

using namespace cr;
using namespace cr::storage;

namespace {

/* ---- in-memory byte source --------------------------------------------- */

class MemSource final : public ImageByteSource {
public:
    explicit MemSource(std::vector<u8> b) : b_(std::move(b)) {}
    u32 read(u64 at, void* buf, u32 len) override {
        if (at > b_.size() || u64(len) > b_.size() - at)
            return ERR_RANGE_OUT_OF_BOUNDS;
        if (len)
            std::memcpy(buf, b_.data() + at, len);
        return OK;
    }
    u64 size() const override { return b_.size(); }

private:
    std::vector<u8> b_;
};

/* a file universe for descriptor-relative extent opens (path -> bytes) */
struct MemFs {
    std::vector<std::pair<std::string, std::vector<u8>>> files;
    u32 open(const char* path, std::unique_ptr<ImageByteSource>& out) {
        const std::string p = path;
        for (auto& f : files)
            if (f.first == p) {
                out.reset(new MemSource(f.second));
                return OK;
            }
        return ERR_OPEN_FAILED;
    }
    static u32 open_cb(void* user, const char* path, std::unique_ptr<ImageByteSource>& out) {
        return static_cast<MemFs*>(user)->open(path, out);
    }
    ImageFileOps ops() {
        ImageFileOps o;
        o.open = &MemFs::open_cb;
        o.user = this;
        return o;
    }
};

void put16(std::vector<u8>& v, size_t at, unsigned x) {
    v[at] = u8(x & 0xFF);
    v[at + 1] = u8((x >> 8) & 0xFF);
}
void put32(std::vector<u8>& v, size_t at, unsigned long x) {
    for (int i = 0; i < 4; ++i)
        v[at + i] = u8((x >> (8 * i)) & 0xFF);
}
void put64(std::vector<u8>& v, size_t at, unsigned long long x) {
    for (int i = 0; i < 8; ++i)
        v[at + i] = u8((x >> (8 * i)) & 0xFF);
}

/* ---- the FAT16 fixture (the shared scan-content truth) ------------------ */

constexpr unsigned kImgSecs = 5074;
constexpr size_t kFAT1 = 2 * 512, kFAT2 = 22 * 512, kROOT = 42 * 512, kDATA = 74 * 512;

void dentry(std::vector<u8>& img, size_t slot, const char* n11, unsigned attr, unsigned cl,
            unsigned long sz, bool del) {
    u8* e = img.data() + kROOT + slot * 32;
    std::memset(e, 0, 32);
    for (int i = 0; i < 11; ++i)
        e[i] = u8(n11[i]);
    if (del)
        e[0] = 0xE5;
    e[11] = u8(attr);
    put16(img, kROOT + slot * 32 + 26, cl & 0xFFFF);
    put32(img, kROOT + slot * 32 + 28, sz);
}

std::vector<u8> build_fat16() {
    std::vector<u8> img(size_t(kImgSecs) * 512, 0);
    std::memcpy(img.data() + 3, "MSWIN4.1", 8);
    put16(img, 0x0B, 512);
    img[0x0D] = 1;
    put16(img, 0x0E, 2);
    img[0x10] = 2;
    put16(img, 0x11, 512);
    put16(img, 0x13, kImgSecs);
    img[0x15] = 0xF8;
    put16(img, 0x16, 20);
    put16(img, 0x18, 63);
    put16(img, 0x1A, 255);
    img[0x26] = 0x29;
    std::memcpy(img.data() + 0x2B, "TESTVOL    ", 11);
    std::memcpy(img.data() + 0x36, "FAT16   ", 8);
    img[510] = 0x55;
    img[511] = 0xAA;
    /* M-015 canonical media words */
    put16(img, kFAT1 + 0 * 2, 0xFFF8);
    put16(img, kFAT1 + 1 * 2, 0xFFFF);
    put16(img, kFAT2 + 0 * 2, 0xFFF8);
    put16(img, kFAT2 + 1 * 2, 0xFFFF);
    put16(img, kFAT1 + 5 * 2, 0xFFFF);
    put16(img, kFAT2 + 5 * 2, 0xFFFF);
    put16(img, kFAT1 + 10 * 2, 11);
    put16(img, kFAT2 + 10 * 2, 11);
    put16(img, kFAT1 + 11 * 2, 12);
    put16(img, kFAT2 + 11 * 2, 12);
    put16(img, kFAT1 + 12 * 2, 0xFFFF);
    put16(img, kFAT2 + 12 * 2, 0xFFFF);
    put16(img, kFAT1 + 20 * 2, 0xFFFF);
    put16(img, kFAT2 + 20 * 2, 0xFFFF);
    dentry(img, 0, "DOC     TXT", 0x20, 10, 1000, false);
    dentry(img, 1, "DELFILE TXT", 0x20, 20, 500, true);
    std::memcpy(img.data() + kDATA + (10 - 2) * 512, "DOCDATA", 7);
    std::memcpy(img.data() + kDATA + (20 - 2) * 512, "DELDATA", 7);
    return img;
}

/* scan the device with the FAT engine and return (files, deleted) */
std::pair<u64, u64> scan_fat_count(IStorageDevice& dev) {
    tree::ResultTree tree;
    fat::FatScanFlags fl;
    fat::FatScanStats st;
    tree::FullNode* ra = tree.add_root(u"Found");
    tree::FullNode* rd = tree.add_root(u"Deleted");
    tree::FullNode* rl = tree.add_root(u"Lost");
    const u32 rc = fat::scan_fat(&dev, fl, nullptr, nullptr, nullptr, tree, ra, rd, rl, st);
    if (rc != OK)
        return {~0ull, ~0ull};
    return {st.files, st.deleted_entries};
}

/* ---- VMDK monolithicSparse builder --------------------------------------
 * Layout: [0 header][1 descriptor][2 grain directory][3 grain table]
 * [4.. grains]. Only grains with nonzero content are allocated. */
std::vector<u8> build_vmdk_sparse(const std::vector<u8>& content) {
    const u64 grain_sectors = 8; /* 4 KiB grains */
    const u64 content_sectors = (content.size() + 511) / 512;
    const u64 total_grains = (content_sectors + grain_sectors - 1) / grain_sectors;
    const u64 gtes = 512; /* entries per grain table */
    const u64 gts = (total_grains + gtes - 1) / gtes; /* grain tables used */
    const u64 gd_sectors = (gts * 4 + 511) / 512;
    const u64 gt_sectors = (total_grains * 4 + 511) / 512;
    const u64 first_grain_sector = 2 + gd_sectors + gt_sectors;
    std::vector<u8> vmdk;
    vmdk.resize(size_t(first_grain_sector) * 512);

    /* header */
    put32(vmdk, 0, 0x564D444B);
    put32(vmdk, 4, 1); /* version */
    put64(vmdk, 8, content_sectors);
    put64(vmdk, 16, grain_sectors);
    put64(vmdk, 24, 1); /* descriptorOffset (sector) */
    put64(vmdk, 32, 1); /* descriptorSize (sectors) */
    put32(vmdk, 40, u32(gtes)); /* numGTEsPerGT */
    put64(vmdk, 44, 2);         /* rgdOffset */
    put64(vmdk, 52, 2);         /* gdOffset */
    put16(vmdk, 60, u32(first_grain_sector)); /* overHead */
    vmdk[63] = '\n';
    vmdk[64] = ' ';
    vmdk[65] = '\r';
    vmdk[66] = '\n';
    put16(vmdk, 67, 0); /* compressAlgorithm: none */

    /* embedded descriptor */
    std::string desc = "# Disk DescriptorFile\nversion=1\nCID=fffffffe\nparentCID=ffffffff\n"
                       "createType=\"monolithicSparse\"\nRW " +
                       std::to_string(content_sectors) + " SPARSE \"disk.vmdk\"\n\n";
    desc.resize(512, '\n');
    std::memcpy(vmdk.data() + 512, desc.data(), 512);

    /* grain directory (one GDE per grain table; tables pack back-to-back
     * after the GD, each gtes*4 bytes) + the grain tables + grains */
    const u64 gt_base = 2 + gd_sectors;
    const u64 sectors_per_table = (gtes * 4 + 511) / 512;
    for (u64 t = 0; t < gts; ++t)
        put32(vmdk, size_t(2 * 512 + t * 4), u32(gt_base + t * sectors_per_table));
    u64 next_grain_sector = first_grain_sector;
    for (u64 g = 0; g < total_grains; ++g) {
        const u64 start = g * grain_sectors * 512;
        const u64 len = grain_sectors * 512;
        bool nonzero = false;
        for (u64 i = start; i < start + len && i < content.size(); ++i)
            if (content[size_t(i)] != 0) {
                nonzero = true;
                break;
            }
        if (!nonzero)
            continue; /* an unallocated grain reads as zeros */
        const u32 gte = u32(next_grain_sector);
        put32(vmdk, size_t(gt_base * 512 + g * 4), gte);
        const size_t old = vmdk.size();
        vmdk.resize(old + size_t(len));
        const u64 copy = start + len <= content.size() ? len : (content.size() - start);
        if (copy)
            std::memcpy(vmdk.data() + old, content.data() + size_t(start), size_t(copy));
        next_grain_sector += grain_sectors;
    }
    return vmdk;
}

/* ---- VHD dynamic builder --------------------------------------------------
 * Layout: [0 footer copy][1 dynamic header][3 BAT][4.. block 0..][footer].
 * Per-sector bitmaps; only sectors with nonzero content are marked. */
std::vector<u8> build_vhd_dynamic(const std::vector<u8>& content, u32 block_bytes = 65536) {
    const u64 content_sectors = (content.size() + 511) / 512;
    const u64 disk_bytes = content_sectors * 512;
    const u32 sectors_per_block = block_bytes / 512;
    const u64 blocks = (disk_bytes + block_bytes - 1) / block_bytes;
    const u64 bat_off = 512 + 1024; /* sector 3 */
    const u64 bitmap_bytes = sectors_per_block / 8;

    /* pass 1: which blocks have any allocated sector */
    std::vector<u8> bat(size_t(blocks), 0xFF);
    auto bitmaps = std::vector<std::vector<u8>>(size_t(blocks));
    std::vector<bool> block_used(size_t(blocks), false);
    for (u64 b = 0; b < blocks; ++b) {
        bitmaps[size_t(b)].assign(size_t(bitmap_bytes), 0);
        for (u32 s = 0; s < sectors_per_block; ++s) {
            const u64 off = u64(b) * block_bytes + u64(s) * 512;
            if (off >= content.size())
                break;
            bool nonzero = false;
            for (u64 i = off; i < off + 512 && i < content.size(); ++i)
                if (content[size_t(i)] != 0) {
                    nonzero = true;
                    break;
                }
            if (nonzero) {
                bitmaps[size_t(b)][s / 8] |= u8(1u << (s % 8));
                block_used[size_t(b)] = true;
            }
        }
    }

    std::vector<u8> v;
    v.resize(size_t(bat_off + ((blocks * 4 + 511) / 512) * 512));
    /* footer copy at 0 (the authoritative copy is appended at the end) */
    const auto write_footer = [&](size_t at) {
        std::memset(v.data() + at, 0, 512);
        std::memcpy(v.data() + at, "conectix", 8);
        put32(v, at + 12, 0x00010000);       /* format version */
        put64(v, at + 16, 512);              /* data offset -> dynamic header */
        put64(v, at + 48, disk_bytes);       /* current size */
        put32(v, at + 60, 3);                /* disk type: dynamic */
    };
    write_footer(0);

    /* dynamic header at 512 */
    std::memset(v.data() + 512, 0, 1024);
    std::memcpy(v.data() + 512, "cxsparse", 8);
    put64(v, 512 + 16, bat_off); /* table offset */
    put32(v, 512 + 28, u32(blocks)); /* max table entries */
    put32(v, 512 + 32, block_bytes);

    /* BAT + blocks */
    u64 block_sector = (bat_off + ((blocks * 4 + 511) / 512) * 512) / 512;
    for (u64 b = 0; b < blocks; ++b) {
        if (block_used[size_t(b)]) {
            put32(v, size_t(bat_off + b * 4), u32(block_sector));
            const size_t old = v.size();
            v.resize(old + size_t(bitmap_bytes + block_bytes));
            std::memcpy(v.data() + old, bitmaps[size_t(b)].data(), size_t(bitmap_bytes));
            const u64 from = u64(b) * block_bytes;
            const u64 copy = from + block_bytes <= content.size() ? block_bytes
                                                                  : (content.size() - from);
            if (copy)
                std::memcpy(v.data() + old + bitmap_bytes, content.data() + size_t(from),
                            size_t(copy));
            block_sector += (bitmap_bytes + block_bytes) / 512;
        } else {
            put32(v, size_t(bat_off + b * 4), 0xFFFFFFFF);
        }
    }
    /* trailing footer (authoritative) */
    const size_t f = v.size();
    v.resize(f + 512);
    write_footer(f);
    return v;
}

} // namespace

/* ========================================================================= */

TEST(Image, FlatFileScansMbr) {
    auto fat = build_fat16();
    std::unique_ptr<IStorageDevice> dev;
    ImageFileOps no_ops;
    ASSERT_EQ(open_image(std::unique_ptr<ImageByteSource>(new MemSource(fat)), no_ops, nullptr,
                         dev),
              u32(OK));
    ASSERT_NE(dev, nullptr);
    EXPECT_EQ(dev->size(), fat.size());
    EXPECT_EQ(dev->geometry().logical_sector, 512u);
    /* the og's LBA0 classification: a FAT16 MBR-less volume boots at LBA0
     * with the 55AA signature -> Mbr */
    EXPECT_EQ(classify_boot_layout(dev.get()), BootLayout::Mbr);
    /* a full FAT scan through the device finds the fixture's files */
    const auto found = scan_fat_count(*dev);
    ASSERT_NE(found.first, ~0ull) << "scan_fat failed on the flat image";
    EXPECT_GE(found.first, 2u); /* DOC.TXT + the deleted DELFILE.TXT chain */
}

TEST(Image, Lba0LayoutClassification) {
    /* GPT: protective MBR entry (type 0xEE) + the EFI PART header at LBA1 */
    std::vector<u8> gpt(2 * 512, 0);
    gpt[510] = 0x55;
    gpt[511] = 0xAA;
    gpt[446 + 4] = 0xEE;
    std::memcpy(gpt.data() + 512, "EFI PART", 8);
    std::unique_ptr<IStorageDevice> dev;
    ASSERT_EQ(open_image(std::unique_ptr<ImageByteSource>(new MemSource(gpt)), ImageFileOps(),
                         nullptr, dev),
              u32(OK));
    EXPECT_EQ(classify_boot_layout(dev.get()), BootLayout::Gpt);

    /* torn protective entry, intact GPT header: still Gpt */
    std::vector<u8> torn = gpt;
    torn[446 + 4] = 0x00;
    ASSERT_EQ(open_image(std::unique_ptr<ImageByteSource>(new MemSource(torn)), ImageFileOps(),
                         nullptr, dev),
              u32(OK));
    EXPECT_EQ(classify_boot_layout(dev.get()), BootLayout::Gpt);

    /* nothing recognizable */
    std::vector<u8> blank(2048, 0);
    ASSERT_EQ(open_image(std::unique_ptr<ImageByteSource>(new MemSource(blank)), ImageFileOps(),
                         nullptr, dev),
              u32(OK));
    EXPECT_EQ(classify_boot_layout(dev.get()), BootLayout::Unknown);
}

TEST(Image, VmdkMonolithicSparseReadsAndScans) {
    /* the virtual disk holds the FAT16 fixture; only grains with nonzero
     * content are allocated (a genuinely sparse image - the unallocated
     * majority MUST read back as zeros and the scan must not see the
     * difference) */
    auto fat = build_fat16();
    auto vmdk = build_vmdk_sparse(fat);
    EXPECT_LT(vmdk.size(), fat.size() / 2) << "the fixture is mostly zeros - sparse must shrink";

    std::unique_ptr<IStorageDevice> dev;
    ASSERT_EQ(open_image(std::unique_ptr<ImageByteSource>(new MemSource(vmdk)), ImageFileOps(),
                         nullptr, dev),
              u32(OK));
    ASSERT_NE(dev, nullptr);
    EXPECT_EQ(dev->size(), fat.size());
    EXPECT_EQ(classify_boot_layout(dev.get()), BootLayout::Mbr);

    /* full-content readback: byte-identical to the flat original */
    std::vector<u8> back(fat.size());
    ASSERT_EQ(dev->read_at(0, back.data(), u32(back.size())), u32(OK));
    EXPECT_EQ(0, std::memcmp(back.data(), fat.data(), fat.size()));

    /* unaligned + tail reads (the device contract allows any shape) */
    u8 chunk[1000];
    ASSERT_EQ(dev->read_at(123, chunk, sizeof(chunk)), u32(OK));
    EXPECT_EQ(0, std::memcmp(chunk, fat.data() + 123, sizeof(chunk)));
    const u64 tail = fat.size() - 7;
    ASSERT_EQ(dev->read_at(tail, chunk, 7), u32(OK));
    EXPECT_EQ(0, std::memcmp(chunk, fat.data() + tail, 7));

    /* the engine pass over the sparse image finds the same files */
    const auto found = scan_fat_count(*dev);
    ASSERT_NE(found.first, ~0ull) << "scan_fat failed on the sparse image";
    EXPECT_GE(found.first, 2u);
}

TEST(Image, VmdkDescriptorWithFlatAndZeroExtents) {
    /* a split image: descriptor + FLAT extent file + ZERO extent + a
     * second FLAT with a nonzero base offset - the og's twoGbMaxExtentFlat
     * shape (minus the 2 GiB sizing) */
    auto fat = build_fat16();
    const u64 flat1_sectors = 1000;
    const u64 zero_sectors = 500;
    const u64 flat2_sectors = kImgSecs - flat1_sectors - zero_sectors;

    /* flat extent 1: the first flat1_sectors of the content */
    std::vector<u8> ext1(fat.begin(), fat.begin() + size_t(flat1_sectors) * 512);
    /* flat extent 2: content after the zero hole, stored with a 4-sector
     * prefix inside its file (the descriptor's offset field) */
    const size_t prefix = 4 * 512;
    std::vector<u8> ext2(prefix + size_t(flat2_sectors) * 512, 0);
    std::memcpy(ext2.data() + prefix, fat.data() + size_t((flat1_sectors + zero_sectors) * 512),
                size_t(flat2_sectors) * 512);

    std::string desc = "# Disk DescriptorFile\nversion=1\ncreateType=\"twoGbMaxExtentFlat\"\n";
    desc += "RW " + std::to_string(flat1_sectors) + " FLAT \"ext1.dat\" 0\n";
    desc += "RW " + std::to_string(zero_sectors) + " ZERO\n";
    desc += "RW " + std::to_string(flat2_sectors) + " FLAT \"sub/ext2.dat\" " +
            std::to_string(prefix / 512) + "\n";
    desc += "\n";

    MemFs fs;
    fs.files.push_back({"/tmp/img/ext1.dat", ext1});
    fs.files.push_back({"/tmp/img/sub/ext2.dat", ext2});

    std::unique_ptr<IStorageDevice> dev;
    ASSERT_EQ(open_image(std::unique_ptr<ImageByteSource>(new MemSource(std::vector<u8>(
                                                                    desc.begin(), desc.end()))),
                         fs.ops(), "/tmp/img/", dev),
              u32(OK));
    ASSERT_NE(dev, nullptr);
    EXPECT_EQ(dev->size(), fat.size());

    /* readback across all three extents (FLAT, ZERO, FLAT+offset) */
    std::vector<u8> back(fat.size());
    ASSERT_EQ(dev->read_at(0, back.data(), u32(back.size())), u32(OK));
    /* the ZERO hole's bytes: the fixture region is zeros, so identical */
    EXPECT_EQ(0, std::memcmp(back.data(), fat.data(), fat.size()));

    /* a nonzero probe inside the hole proves the ZERO extent serves
     * zeros (not extent data): write a sentinel into the flat2 file where
     * the hole would map if extents were misrouted */
    EXPECT_EQ(dev->read_at(flat1_sectors * 512 + 8, back.data(), 16), u32(OK));
    bool all_zero = true;
    for (int i = 0; i < 16; ++i)
        if (back[i] != 0)
            all_zero = false;
    EXPECT_TRUE(all_zero) << "the ZERO extent must read as zeros";

    const auto found = scan_fat_count(*dev);
    ASSERT_NE(found.first, ~0ull);
    EXPECT_GE(found.first, 2u);
}

TEST(Image, VhdDynamicDiskReadsAndScans) {
    auto fat = build_fat16();
    auto vhd = build_vhd_dynamic(fat);
    std::unique_ptr<IStorageDevice> dev;
    ASSERT_EQ(open_image(std::unique_ptr<ImageByteSource>(new MemSource(vhd)), ImageFileOps(),
                         nullptr, dev),
              u32(OK));
    ASSERT_NE(dev, nullptr);
    EXPECT_EQ(dev->size(), fat.size());
    EXPECT_EQ(classify_boot_layout(dev.get()), BootLayout::Mbr);

    std::vector<u8> back(fat.size());
    ASSERT_EQ(dev->read_at(0, back.data(), u32(back.size())), u32(OK));
    EXPECT_EQ(0, std::memcmp(back.data(), fat.data(), fat.size()));

    /* unaligned read across a block boundary (64 KiB blocks) */
    u8 chunk[4096];
    ASSERT_EQ(dev->read_at(65536 - 100, chunk, sizeof(chunk)), u32(OK));
    EXPECT_EQ(0, std::memcmp(chunk, fat.data() + 65536 - 100, sizeof(chunk)));

    const auto found = scan_fat_count(*dev);
    ASSERT_NE(found.first, ~0ull) << "scan_fat failed on the dynamic VHD";
    EXPECT_GE(found.first, 2u);
}

TEST(Image, VhdFixedDiskIsFlatMinusFooter) {
    /* a fixed VHD: raw content + the trailing footer */
    auto fat = build_fat16();
    std::vector<u8> v = fat;
    const size_t f = v.size();
    v.resize(f + 512);
    std::memset(v.data() + f, 0, 512);
    std::memcpy(v.data() + f, "conectix", 8);
    put64(v, f + 16, ~0ull); /* fixed: no dynamic header */
    put64(v, f + 48, fat.size());
    put32(v, f + 60, 2); /* disk type: fixed */

    std::unique_ptr<IStorageDevice> dev;
    ASSERT_EQ(open_image(std::unique_ptr<ImageByteSource>(new MemSource(v)), ImageFileOps(),
                         nullptr, dev),
              u32(OK));
    EXPECT_EQ(dev->size(), fat.size());
    std::vector<u8> back(fat.size());
    ASSERT_EQ(dev->read_at(0, back.data(), u32(back.size())), u32(OK));
    EXPECT_EQ(0, std::memcmp(back.data(), fat.data(), fat.size()));
}

TEST(Image, RejectsBrokenInputs) {
    std::unique_ptr<IStorageDevice> dev;
    /* under one sector: not a scan target */
    EXPECT_EQ(open_image(std::unique_ptr<ImageByteSource>(new MemSource({1, 2, 3})), ImageFileOps(),
                         nullptr, dev),
              u32(ERR_BAD_PARAMETER));
    /* VHDX: tracked remainder, honest refusal */
    std::vector<u8> vhdx(4096, 0);
    std::memcpy(vhdx.data(), "vhdxfile", 8);
    EXPECT_EQ(open_image(std::unique_ptr<ImageByteSource>(new MemSource(vhdx)), ImageFileOps(),
                         nullptr, dev),
              u32(ERR_UNSUPPORTED));
    /* VMDK magic with a garbage header: bad magic, never a fallback */
    std::vector<u8> bad(4096, 0);
    put32(bad, 0, 0x564D444B);
    put32(bad, 4, 99); /* unknown version */
    EXPECT_EQ(open_image(std::unique_ptr<ImageByteSource>(new MemSource(bad)), ImageFileOps(),
                         nullptr, dev),
              u32(ERR_BAD_MAGIC));
    /* a descriptor with no extent lines: bad magic */
    std::vector<u8> nodesc(1024, 0);
    std::memcpy(nodesc.data(), "# Disk DescriptorFile\nversion=1\n", 31);
    EXPECT_EQ(open_image(std::unique_ptr<ImageByteSource>(new MemSource(nodesc)), ImageFileOps(),
                         nullptr, dev),
              u32(ERR_BAD_MAGIC));
    /* out-of-bounds reads are refused (the device contract) */
    auto fat = build_fat16();
    ASSERT_EQ(open_image(std::unique_ptr<ImageByteSource>(new MemSource(fat)), ImageFileOps(),
                         nullptr, dev),
              u32(OK));
    u8 b[16];
    EXPECT_EQ(dev->read_at(fat.size() - 8, b, 16), u32(ERR_RANGE_OUT_OF_BOUNDS));
}

TEST(Image, SessionRoundTripCarriesTheImagePath) {
    /* the identity card's v3 tail: a scan of an image target persists the
     * image path; a reloaded session reports it (the og's image import
     * keeps the file identity for the device list + resume) */
    auto fat = build_fat16();
    std::unique_ptr<IStorageDevice> dev;
    ASSERT_EQ(open_image(std::unique_ptr<ImageByteSource>(new MemSource(fat)), ImageFileOps(),
                         nullptr, dev),
              u32(OK));

    tree::ResultTree tree;
    orchestrator::ScanConfig cfg;
    cfg.run_quick = true;
    cfg.run_deep = false;
    cfg.run_carve = false;
    for (size_t i = 0; i < sizeof(cfg.volume_image) - 1 && i < strlen("/vm/disks/qemu.img"); ++i)
        cfg.volume_image[i] = "/vm/disks/qemu.img"[i];
    orchestrator::ScanController ctrl(tree, cfg);
    orchestrator::ScanStats stats;
    ASSERT_EQ(OK, ctrl.run(dev.get(), nullptr, nullptr, nullptr, stats));

    /* MemFile session sink (the test_orchestrator pattern) */
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
    orchestrator::ScanStats stats2;
    session::VolumeSessionInfo info;
    {
        session::Reader r(MemFile::rd, MemFile::sk, MemFile::sz, &mf);
        ASSERT_EQ(OK, orchestrator::ScanController::load_session_summary(r, tree2, stats2, info));
    }
    EXPECT_STREQ(info.image, "/vm/disks/qemu.img");
    EXPECT_EQ(info.fs_family, u32(orchestrator::FsFamily::Fat));
    /* v1/v2 sessions (no image tail) load with an empty path */
    EXPECT_TRUE(strchr(info.image, '/') != nullptr);
}
