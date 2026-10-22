/* FAT engine end-to-end tests: synthetic FAT16 + FAT32 volumes.
 *
 * FAT16 fixture (5074 sectors x 512B, 5000 clusters -> flavor FAT16):
 *   reserved 2 | FAT1 2x20 | FAT2 2x20 | root 32 | data
 *   root entries: LFN readme (clusters 10-12), _EL.TXT deleted-intact
 *   (20-21), _ONE.TXT deleted-unlinked (30 FREE), SUBDIR dir (cluster 5)
 *   containing INNER.TXT (cluster 15).
 *
 * FAT32 fixture (66562 sectors, 65530 clusters): root cluster chain 2->3,
 * one live photo.jpg; primary boot corrupted -> backup-boot fallback.
 */
#include <gtest/gtest.h>
#include "cr/fat/cr_fat_engine.h"
#include "cr/storage/cr_device.h"
#include <cstring>
#include <functional>
#include <vector>

using namespace cr;
using namespace cr::fat;

namespace {

class MemDevice final : public IStorageDevice {
public:
    explicit MemDevice(std::vector<u8> b) : data_(std::move(b)) {
        geom_.logical_sector = 512;
        geom_.physical_sector = 512;
        geom_.sector_count = data_.size() / 512;
        geom_.total_bytes = geom_.sector_count * 512;
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

/* FAT16 BPB for the fixture geometry. */
void build_fat16_boot(std::vector<u8>& img) {
    u8* b = img.data();
    std::memcpy(b + 3, "MSWIN4.1", 8); /* OEM (any) */
    put16(img, 0x0B, 512);             /* bytes/sector */
    b[0x0D] = 1;                       /* sectors/cluster */
    put16(img, 0x0E, 2);               /* reserved */
    b[0x10] = 2;                       /* FAT count */
    put16(img, 0x11, 512);             /* root entries (32 sectors) */
    put16(img, 0x13, 5074);            /* total sectors 16 */
    b[0x15] = 0xF8;
    put16(img, 0x16, 20); /* sectors/FAT */
    put16(img, 0x18, 63);
    put16(img, 0x1A, 255);
    put32v(img, 0x20, 0); /* total32 (use 16-bit) */
    b[0x26] = 0x29;       /* extended boot */
    put32v(img, 0x27, 0x1234);
    std::memcpy(b + 0x2B, "TESTVOLUME ", 11);
    std::memcpy(b + 0x36, "FAT16   ", 8);
    b[510] = 0x55;
    b[511] = 0xAA;
}

void set_fat16_entry(std::vector<u8>& img, size_t fat_byte_off, u32 cluster, u16 value) {
    put16(img, fat_byte_off + cluster * 2, value);
}

void make_lfn_slot(u8* slot, u8 seq, u8 checksum, const char16_t* fragment) {
    std::u16string f = fragment;
    LfnEntry* l = reinterpret_cast<LfnEntry*>(slot);
    l->sequence = seq;
    l->attributes = ATTR_LFN;
    l->type = 0;
    l->checksum = checksum;
    l->first_cluster = 0;
    const int counts[3] = {5, 6, 2};
    u16* parts[3] = {l->name1, l->name2, l->name3};
    size_t fi = 0;
    for (int p = 0; p < 3; ++p)
        for (int c = 0; c < counts[p]; ++c)
            parts[p][c] = (fi < f.size()) ? f[fi++] : 0xFFFF;
}

void make_dentry(u8* slot, const char* name8_3, u8 attrs, u32 first_cluster, u32 size,
                 u16 write_date, u16 write_time, bool deleted) {
    DirEntry* e = reinterpret_cast<DirEntry*>(slot);
    for (int i = 0; i < 11; ++i)
        e->name[i] = u8(name8_3[i]);
    if (deleted)
        e->name[0] = DENTRY_DELETED;
    e->attributes = attrs;
    e->nt_reserved = 0;
    e->creation_tenths = 0;
    e->creation_time = write_time;
    e->creation_date = write_date;
    e->access_date = write_date;
    e->first_cluster_high = u16(first_cluster >> 16);
    e->write_time = write_time;
    e->write_date = write_date;
    e->first_cluster_low = u16(first_cluster);
    e->file_size = size;
}

std::vector<u8> build_fat16_image() {
    std::vector<u8> img(u64(5074) * 512, 0);
    build_fat16_boot(img);

    const size_t FAT1 = 2 * 512;  /* sector 2 */
    const size_t FAT2 = 22 * 512; /* sector 22 */
    const size_t ROOT = 42 * 512; /* sector 42 (32 sectors) */
    const size_t DATA = 74 * 512; /* sector 74: cluster 2 */

    /* chains: 10->11->12 EOC (readme), 20->21 EOC (del, intact),
     * 30 FREE (gone), 5 EOC (subdir), 15 EOC (inner) */
    auto link = [&](u32 c, u16 v) {
        set_fat16_entry(img, FAT1, c, v);
        set_fat16_entry(img, FAT2, c, v); /* mirrored FAT2 */
    };
    link(10, 11);
    link(11, 12);
    link(12, 0xFFFF);
    link(20, 21);
    link(21, 0xFFFF);
    link(30, 0x0000);
    link(5, 0xFFFF);
    link(15, 0xFFFF);

    /* root entries at ROOT */
    u8* root = img.data() + ROOT;
    /* LFN for "readme_document_v1.txt" (23 chars):
     * slot seq2|0x40 = chars 14..23 "ment_v1.txt", slot seq1 = chars 1..13 */
    u8 name11[11] = {'R', 'E', 'A', 'D', 'M', 'E', '~', '1', 'T', 'X', 'T'};
    u8 chk = lfn_checksum(name11);
    make_lfn_slot(root + 0 * 32, 0x42, chk, u"ment_v1.txt"); /* seq2|last */
    make_lfn_slot(root + 1 * 32, 0x01, chk, u"readme_docu"); /* seq1 */
    make_dentry(root + 2 * 32, "README~1TXT", 0x20, 10, 1200, 0x5A46 /*2022-01-06*/, 0x6000, false);
    make_dentry(root + 3 * 32, "DEL     TXT", 0x20, 20, 1000, 0x5A46, 0x6000, true);
    make_dentry(root + 4 * 32, "GONE    TXT", 0x20, 30, 500, 0x5A46, 0x6000, true);
    make_dentry(root + 5 * 32, "SUBDIR     ", ATTR_DIRECTORY, 5, 0, 0x5A46, 0x6000, false);

    /* subdir content at cluster 5 (sector 74+3=77) */
    u8* sub = img.data() + DATA + (5 - 2) * 512;
    make_dentry(sub + 0 * 32, ".          ", ATTR_DIRECTORY, 5, 0, 0x5A46, 0x6000, false);
    make_dentry(sub + 1 * 32, "..         ", ATTR_DIRECTORY, 5, 0, 0x5A46, 0x6000, false);
    make_dentry(sub + 2 * 32, "INNER   TXT", 0x20, 15, 512, 0x5A46, 0x6000, false);

    /* file content markers */
    std::memcpy(img.data() + DATA + (10 - 2) * 512, "READMEDATA", 10);
    std::memcpy(img.data() + DATA + (15 - 2) * 512, "INNERDATA", 9);
    return img;
}

std::vector<tree::FileNode*> collect_all(tree::ResultTree& t) {
    std::vector<tree::FileNode*> all;
    std::function<void(tree::FileNode*)> collect = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            all.push_back(c);
            collect(c);
        }
    };
    for (auto* r : t.roots())
        collect(r);
    return all;
}

} // namespace

TEST(FatEngine, Fat16EndToEnd) {
    auto img = build_fat16_image();
    MemDevice dev(std::move(img));

    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost");
    FatScanFlags flags;
    FatScanStats stats;
    u32 rc = scan_fat(&dev, flags, nullptr, nullptr, nullptr, tree, ra, rd, rl, stats);
    ASSERT_EQ(rc, OK) << err_text(rc);

    EXPECT_GE(stats.files, 4u); /* readme, del, gone, inner */
    EXPECT_GE(stats.dirs, 1u);  /* subdir (dots skipped) */
    EXPECT_GE(stats.deleted_entries, 2u);

    auto all = collect_all(tree);
    tree::FileNode *readme = nullptr, *del = nullptr, *gone = nullptr, *subdir = nullptr,
                   *inner = nullptr;
    for (auto* n : all) {
        std::string nm = n->name_utf8();
        if (nm == "readme_document_v1.txt")
            readme = n; /* LFN applied */
        else if (nm == "_EL.TXT")
            del = n; /* deleted 8.3, '_' restored */
        else if (nm == "_ONE.TXT")
            gone = n;
        else if (nm == "SUBDIR")
            subdir = n;
        else if (nm == "INNER.TXT")
            inner = n;
    }
    ASSERT_NE(readme, nullptr);
    ASSERT_NE(del, nullptr);
    ASSERT_NE(gone, nullptr);
    ASSERT_NE(subdir, nullptr);
    ASSERT_NE(inner, nullptr);

    /* LFN correctness + fallback corruption case */
    EXPECT_STREQ(readme->name_utf8().c_str(), "readme_document_v1.txt");

    /* structure: inner under subdir */
    EXPECT_EQ(inner->parent(), subdir);

    /* extents: readme clusters 10-12 -> one contiguous extent at sec 82 */
    ASSERT_EQ(readme->props()->extents.size(), 1u);
    EXPECT_EQ(readme->props()->extents[0].offset, u64(82) * 512);
    EXPECT_EQ(readme->props()->extents[0].length, u64(3) * 512);
    EXPECT_EQ(readme->props()->logical_size, 1200u);
    EXPECT_EQ(readme->confidence(), tree::Confidence::Excellent);

    /* deleted-intact chain: Excellent; deleted-unlinked: capped PartialRisk */
    EXPECT_EQ(del->confidence(), tree::Confidence::Excellent);
    EXPECT_EQ(del->deleted_state(), tree::DeletedState::Deleted);
    EXPECT_EQ(gone->confidence(), tree::Confidence::PartialRisk);

    /* times: 0x5A46 = 2025-02-06, 0x6000 = 12:00 -> unix 1738843200 */
    EXPECT_EQ(readme->props()->times.modified, 1738843200ULL * 10000000ULL + 116444736000000000ULL);
}

TEST(FatEngine, LfnCorruptFallsBackToShort) {
    /* corrupt the LFN checksum -> 8.3 name used */
    auto img = build_fat16_image();
    img[42 * 512 + 0 * 32 + 13] = 0x99; /* slot1 checksum field */
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    FatScanFlags flags;
    FatScanStats stats;
    ASSERT_EQ(scan_fat(&dev, flags, nullptr, nullptr, nullptr, tree, ra, nullptr, nullptr, stats),
              OK);
    auto all = collect_all(tree);
    tree::FileNode* readme = nullptr;
    for (auto* n : all)
        if (n->name_utf8() == "README~1.TXT")
            readme = n;
    ASSERT_NE(readme, nullptr); /* 8.3 fallback survived the corrupt LFN */
}

TEST(FatEngine, Fat32RootChainAndBackupBoot) {
    /* FAT32: 66562 sectors, reserved 8, FAT 2x512 sectors, first data 1032
     * -> cluster_count 65530 (FAT32). Root chain 2->3, one live entry. */
    std::vector<u8> img(u64(66562) * 512, 0);
    u8* b = img.data();
    std::memcpy(b + 3, "MSDOS5.0", 8);
    put16(img, 0x0B, 512);
    b[0x0D] = 1;
    put16(img, 0x0E, 8); /* reserved (backup boot at 6) */
    b[0x10] = 2;
    put16(img, 0x11, 0); /* FAT32: no fixed root */
    put16(img, 0x13, 0); /* use 32-bit total */
    b[0x15] = 0xF8;
    put16(img, 0x16, 0); /* FAT32 marker: spf16 = 0 */
    put16(img, 0x18, 63);
    put16(img, 0x1A, 255);
    put32v(img, 0x20, 66562);
    put32v(img, 0x24, 512); /* sectors/FAT */
    put16(img, 0x28, 0);
    put32v(img, 0x2C, 2); /* root cluster */
    put16(img, 0x32, 6);  /* backup boot sector */
    b[510] = 0x55;
    b[511] = 0xAA;
    std::memcpy(img.data() + 6 * 512, b, 512); /* backup copy */

    /* FAT1 at sector 8, FAT2 at 520 */
    const size_t FAT1 = 8 * 512, FAT2 = 520 * 512;
    auto link32 = [&](u32 c, u32 v) {
        put32v(img, FAT1 + c * 4, v);
        put32v(img, FAT2 + c * 4, v);
    };
    link32(2, 3);          /* root chain */
    link32(3, 0x0FFFFFFF); /* EOC */
    link32(100, 0x0FFFFFFF);

    /* root dir content: cluster 2 at sector 1032, cluster 3 at 1033.
     * Cluster 2 must be FULL (no 0x00 slot) or the dir ends there - fill
     * slots 1..15 with volume-label entries (skipped by the walker). */
    u8* root = img.data() + 1032 * 512;
    make_dentry(root + 0 * 32, "PHOTO   JPG", 0x20, 100, 1024, 0x5A46, 0x6000, false);
    for (int i = 1; i < 16; ++i) {
        char nm[12] = "VOLLABEL   ";
        nm[8] = char('0' + i / 10);
        nm[9] = char('0' + i % 10);
        make_dentry(root + u32(i) * 32, "VOLLABEL   ", ATTR_VOLUME_ID, 0, 0, 0x5A46, 0x6000, false);
        (void)nm;
    }
    u8* root2 = img.data() + 1033 * 512;
    make_dentry(root2 + 0 * 32, "SECOND  TXT", 0x20, 100, 16, 0x5A46, 0x6000, false);

    {
        MemDevice dev(std::move(img)); /* move in, keep a copy for corrupt case */
        /* NOTE: MemDevice owns the buffer; for the corruption phase we need
         * a second image. Rebuild below instead. */
        tree::ResultTree tree;
        auto* ra = tree.add_root(u"Found");
        FatScanFlags flags;
        FatScanStats stats;
        u32 rc =
            scan_fat(&dev, flags, nullptr, nullptr, nullptr, tree, ra, nullptr, nullptr, stats);
        ASSERT_EQ(rc, OK) << err_text(rc);
        auto all = collect_all(tree);
        tree::FileNode *photo = nullptr, *second = nullptr;
        for (auto* n : all) {
            if (n->name_utf8() == "PHOTO.JPG")
                photo = n;
            if (n->name_utf8() == "SECOND.TXT")
                second = n;
        }
        ASSERT_NE(photo, nullptr);  /* root cluster 2 */
        ASSERT_NE(second, nullptr); /* root chain continuation cluster 3 */
        EXPECT_EQ(photo->props()->logical_size, 1024u);
    }
}

TEST(FatEngine, Fat32BackupBootFallback) {
    std::vector<u8> img(u64(66562) * 512, 0);
    u8* b = img.data();
    std::memcpy(b + 3, "MSDOS5.0", 8);
    put16(img, 0x0B, 512);
    b[0x0D] = 1;
    put16(img, 0x0E, 8);
    b[0x10] = 2;
    put16(img, 0x13, 0);
    b[0x15] = 0xF8;
    put16(img, 0x16, 0);
    put16(img, 0x18, 63);
    put16(img, 0x1A, 255);
    put32v(img, 0x20, 66562);
    put32v(img, 0x24, 512);
    put32v(img, 0x2C, 2);
    put16(img, 0x32, 6);
    b[510] = 0x55;
    b[511] = 0xAA;
    std::memcpy(img.data() + 6 * 512, b, 512);
    const size_t FAT1 = 8 * 512;
    put32v(img, FAT1 + 2 * 4, 3);
    put32v(img, FAT1 + 3 * 4, 0x0FFFFFFF);
    put32v(img, FAT1 + 100 * 4, 0x0FFFFFFF);
    u8* root = img.data() + 1032 * 512;
    make_dentry(root + 0 * 32, "PHOTO   JPG", 0x20, 100, 1024, 0x5A46, 0x6000, false);

    /* corrupt the PRIMARY boot sector entirely (zeros) - backup must save it */
    std::memset(img.data(), 0, 512);

    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    FatScanFlags flags;
    FatScanStats stats;
    u32 rc = scan_fat(&dev, flags, nullptr, nullptr, nullptr, tree, ra, nullptr, nullptr, stats);
    ASSERT_EQ(rc, OK) << err_text(rc);
    auto all = collect_all(tree);
    tree::FileNode* photo = nullptr;
    for (auto* n : all)
        if (n->name_utf8() == "PHOTO.JPG")
            photo = n;
    ASSERT_NE(photo, nullptr);
}

TEST(FatEngine, ChainCycleIsCut) {
    /* FAT loop: 40 -> 41 -> 40 (cycle guard must terminate, fix F7). The
     * test completing at all is the primary assertion. */
    auto img = build_fat16_image();
    const size_t FAT1 = 2 * 512;
    const size_t FAT2 = 22 * 512;
    set_fat16_entry(img, FAT1, 40, 41);
    set_fat16_entry(img, FAT2, 40, 41);
    set_fat16_entry(img, FAT1, 41, 40);
    set_fat16_entry(img, FAT2, 41, 40);
    u8* root = img.data() + 42 * 512;
    make_dentry(root + 6 * 32, "LOOPY   TXT", 0x20, 40, 4096, 0x5A46, 0x6000, false);

    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    FatScanFlags flags;
    FatScanStats stats;
    u32 rc = scan_fat(&dev, flags, nullptr, nullptr, nullptr, tree, ra, nullptr, nullptr, stats);
    ASSERT_EQ(rc, OK) << err_text(rc);
    auto all = collect_all(tree);
    tree::FileNode* loopy = nullptr;
    for (auto* n : all)
        if (n->name_utf8() == "LOOPY.TXT")
            loopy = n;
    ASSERT_NE(loopy, nullptr);
    /* cycle cut: extent covers the visited clusters only (40,41 contiguous) */
    EXPECT_EQ(loopy->props()->extents.size(), 1u);
    EXPECT_EQ(loopy->props()->extents[0].offset, u64(74 + 38) * 512); /* cluster 40 */
}
