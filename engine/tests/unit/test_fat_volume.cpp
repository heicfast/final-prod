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
    /* N-010b: real FAT32 boot sectors carry the FS-name string at +0x52;
     * the og's engine-open validator (FUN_18002f970) requires it. */
    std::memcpy(b + 0x52, "FAT32   ", 8);
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

/* FAT-8 (AUDIT-FAT): the backup-boot fallback is FAT32-only. FAT12/16
 * volumes have NO backup boot sector - sector 6 there is FAT-area bytes.
 * A FAT16 image whose sector-6 content merely validates as a boot sector
 * must NOT be salvaged via the fallback: the damaged-primary scan fails
 * honestly instead of adopting FAT-area bytes as geometry. */
TEST(FatEngine, Fat16HasNoBackupBootFallback) {
    auto img = build_fat16_image();
    /* plant a full valid copy of the (FAT16) boot sector at sector 6 */
    std::memcpy(img.data() + 6 * 512, img.data(), 512);
    /* corrupt the PRIMARY boot sector entirely (zeros) */
    std::memset(img.data(), 0, 512);

    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    FatScanFlags flags;
    FatScanStats stats;
    u32 rc = scan_fat(&dev, flags, nullptr, nullptr, nullptr, tree, ra, nullptr, nullptr, stats);
    EXPECT_NE(rc, OK); /* refused: FAT16 flavor cannot use the backup path */
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
    /* N-010b: the engine-open gates require the FS-name string; real
     * backup boot sectors carry it (the backup is a full copy). */
    std::memcpy(b + 0x52, "FAT32   ", 8);
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

/* ---- FAT12 (PARITY-AUDIT F-A1-1 regression) ------------------------- */

/* 12-bit FAT entry write: entry i occupies bytes [i*3/2, i*3/2+2). */
void set_fat12_entry(std::vector<u8>& img, size_t fat_off, u32 cluster, u32 value) {
    size_t off = fat_off + cluster + cluster / 2;
    if (cluster & 1) {
        img[off] = u8((img[off] & 0x0F) | ((value & 0xF) << 4));
        img[off + 1] = u8((value >> 4) & 0xFF);
    } else {
        img[off] = u8(value & 0xFF);
        img[off + 1] = u8((img[off + 1] & 0xF0) | ((value >> 8) & 0xF));
    }
}

/* FAT12 fixture: 600 sectors x 512B, 1 sec/cluster -> 562 data clusters
 * (< 4085 => FAT12 flavor). Layout: reserved 2 | FAT 2x2 | root 32 | data.
 * first_data_sector = 2 + 2*2 + 32 = 38. */
std::vector<u8> build_fat12_image() {
    std::vector<u8> img(u64(600) * 512, 0);
    u8* b = img.data();
    std::memcpy(b + 3, "MSDOS5.0", 8);
    put16(img, 0x0B, 512); /* bytes/sector */
    b[0x0D] = 1;           /* sectors/cluster */
    put16(img, 0x0E, 2);   /* reserved */
    b[0x10] = 2;           /* FAT count */
    put16(img, 0x11, 512); /* root entries (32 sectors) */
    put16(img, 0x13, 600); /* total sectors 16 */
    b[0x15] = 0xF8;
    put16(img, 0x16, 2); /* sectors/FAT */
    put16(img, 0x18, 63);
    put16(img, 0x1A, 255);
    b[0x26] = 0x29;
    put32v(img, 0x27, 0x5678);
    std::memcpy(b + 0x2B, "FAT12VOL   ", 11);
    std::memcpy(b + 0x36, "FAT12   ", 8);
    b[510] = 0x55;
    b[511] = 0xAA;

    const size_t FAT1 = 2 * 512;
    const size_t FAT2 = 4 * 512;
    const size_t ROOT = 6 * 512;  /* 32 sectors */
    const size_t DATA = 38 * 512; /* cluster 2 */

    auto link = [&](u32 c, u32 v) {
        set_fat12_entry(img, FAT1, c, v);
        set_fat12_entry(img, FAT2, c, v); /* mirrored FAT2 */
    };
    /* reserved entries 0/1 (media / EOC) */
    img[FAT1 + 0] = 0xF8;
    img[FAT1 + 1] = 0xFF;
    img[FAT1 + 2] = 0xFF;
    img[FAT2 + 0] = 0xF8;
    img[FAT2 + 1] = 0xFF;
    img[FAT2 + 2] = 0xFF;

    /* live TWELVE.TXT: 2 -> 3 -> 4 (EOC 0xFFF), 3 clusters; deleted
     * _OLD.TXT: 6 -> 7 (EOC 0xFFF), 2 clusters, chain intact */
    link(2, 3);
    link(3, 4);
    link(4, 0xFFF);
    link(6, 7);
    link(7, 0xFFF);

    u8* root = img.data() + ROOT;
    make_dentry(root + 0 * 32, "TWELVE  TXT", 0x20, 2, 1536, 0x5A46, 0x6000, false);
    make_dentry(root + 1 * 32, "XOLD    TXT", 0x20, 6, 1024, 0x5A46, 0x6000, true);

    std::memcpy(img.data() + DATA + (2 - 2) * 512, "FAT12LIVEDATA", 13);
    std::memcpy(img.data() + DATA + (6 - 2) * 512, "FAT12DEL DATA", 13);
    return img;
}

TEST(FatEngine, Fat12ChainsCanonicalEocAndBad) {
    /* F-A1-1: FAT12 BAD (0xFF7) / EOC (>=0xFF8) must canonicalize like the
     * FAT16/32 branches. Before the fix the raw 0xFFF EOC value (4095) was
     * followed as a next cluster -> out of range -> chain "broken" -> every
     * FAT12 deleted file capped at PartialRisk + chains_broken inflated. */
    auto img = build_fat12_image();
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    FatScanFlags flags;
    FatScanStats stats;
    u32 rc = scan_fat(&dev, flags, nullptr, nullptr, nullptr, tree, ra, rd, nullptr, stats);
    ASSERT_EQ(rc, OK) << err_text(rc);

    auto all = collect_all(tree);
    tree::FileNode* twelve = nullptr;
    tree::FileNode* old = nullptr;
    for (auto* n : all) {
        if (n->name_utf8() == "TWELVE.TXT")
            twelve = n;
        if (n->name_utf8() == "_OLD.TXT")
            old = n;
    }
    ASSERT_NE(twelve, nullptr) << "live FAT12 file missing";
    ASSERT_NE(old, nullptr) << "deleted FAT12 file missing";

    /* live chain 2->3->4 walked to the 12-bit EOC: single contiguous extent */
    ASSERT_EQ(twelve->props()->extents.size(), 1u);
    EXPECT_EQ(twelve->props()->extents[0].offset, u64(38) * 512); /* cluster 2 */
    EXPECT_EQ(twelve->props()->extents[0].length, u64(3) * 512);
    EXPECT_EQ(twelve->deleted_state(), tree::DeletedState::Active);

    /* deleted chain 6->7 intact and complete for its size: Excellent, not
     * the broken-chain PartialRisk, and no broken-chain accounting */
    ASSERT_EQ(old->props()->extents.size(), 1u);
    EXPECT_EQ(old->props()->extents[0].offset, u64(38 + 4) * 512); /* cluster 6 */
    EXPECT_EQ(old->props()->extents[0].length, u64(2) * 512);
    EXPECT_EQ(old->deleted_state(), tree::DeletedState::Deleted);
    EXPECT_EQ(old->confidence(), tree::Confidence::Excellent);
    EXPECT_EQ(stats.chains_broken, 0u);
}

TEST(FatEngine, DeletedLfnFragmentOrder) {
    /* FAT-1 (AUDIT-FAT, P1): deleted LFN slots carry 0xE5 in the sequence
     * byte - the sort key collapses to 0x25 for every slot, disk order
     * (descending sequence) survives, and the fragments concatenated in
     * REVERSE produced garbage names for every deleted-LFN file while the
     * per-slot checksum still matched (so the mangled name beat the 8.3
     * fallback). Physical layout survives deletion: reverse the disk order. */
    auto img = build_fat16_image();
    u8* root = img.data() + 42 * 512;
    /* 26-char name, 3 slots, planted in DESCENDING disk order like VFAT
     * writes them; then deletion overwrites each slot's first byte. */
    u8 name11[11] = {0xE5, 'E', 'L', 'E', 'T', 'E', '~', '1', 'T', 'X', 'T'};
    const u8 chk = lfn_checksum(name11); /* over the RAW (0xE5-led) 8.3 bytes -
                                             what the walker computes on a deleted dentry */
    make_lfn_slot(root + 0 * 32, 0x43, chk, u"txt");
    make_lfn_slot(root + 1 * 32, 0x02, chk, u"_file_name.");
    make_lfn_slot(root + 2 * 32, 0x01, chk, u"deleted_long");
    root[0 * 32] = 0xE5; /* deletion marker over the sequence bytes */
    root[1 * 32] = 0xE5;
    root[2 * 32] = 0xE5;
    make_dentry(root + 3 * 32, "DELETE~1TXT", 0x20, 20, 1000, 0x5A46, 0x6000, true);

    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    FatScanFlags flags;
    FatScanStats stats;
    ASSERT_EQ(scan_fat(&dev, flags, nullptr, nullptr, nullptr, tree, ra, rd, nullptr, stats), OK);

    auto all = collect_all(tree);
    tree::FileNode* victim = nullptr;
    for (auto* n : all)
        if (n->name_utf8() == "deleted_long_file_name.txt")
            victim = n;
    ASSERT_NE(victim, nullptr) << "deleted LFN name must recover in fragment order";
    EXPECT_EQ(victim->deleted_state(), tree::DeletedState::Deleted);
    EXPECT_GE(stats.lfn_recovered, 1u);
}

/* ---- GAP-S6-1: deleted-LFN checksum oracle (bench 40079065019 + probe) ----
 *
 * PROBE GROUND TRUTH (mtools FAT32 image, mdel, raw dentry dump - session
 * worklog CORE-FINAL-1): deletion writes 0xE5 into byte 0 of EVERY 32-byte
 * entry in the chain - for LFN slots that is the SEQUENCE byte; the 13-char
 * name chunks are FULLY INTACT, and every slot carries the checksum that was
 * computed at WRITE TIME over the ORIGINAL (pre-deletion) 8.3 bytes. The old
 * code checksummed the 0xE5-led bytes and could never match -> every deleted
 * long name fell back to '_'-mangled 8.3 (bench: SEED\TINY\_000.JPG). */

TEST(FatEngine, DeletedLfnRealWorldChecksumRecoversFullName) {
    /* the exact probe shape: "averylongfilename000.txt" (24 chars, 2 slots),
     * alias AVERYL~1.TXT, checksum stored over the ORIGINAL 'A'-led bytes,
     * then deletion 0xE5's all three entries' first bytes. */
    auto img = build_fat16_image();
    u8* root = img.data() + 42 * 512;
    u8 name11[11] = {'A', 'V', 'E', 'R', 'Y', 'L', '~', '1', 'T', 'X', 'T'};
    const u8 chk = lfn_checksum(name11);                       /* over the ORIGINAL bytes - what
                                                                * Windows stores at write time */
    make_lfn_slot(root + 0 * 32, 0x42, chk, u"name000.txt");   /* seq2|last */
    make_lfn_slot(root + 1 * 32, 0x01, chk, u"averylongfile"); /* seq1 */
    make_dentry(root + 2 * 32, "AVERYL~1TXT", 0x20, 20, 1000, 0x5A46, 0x6000, true);
    /* deletion markers: sequence byte of each slot + the 8.3 first byte */
    root[0 * 32] = 0xE5;
    root[1 * 32] = 0xE5;
    root[2 * 32] = 0xE5;

    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    FatScanFlags flags;
    FatScanStats stats;
    ASSERT_EQ(scan_fat(&dev, flags, nullptr, nullptr, nullptr, tree, ra, rd, nullptr, stats), OK);

    auto all = collect_all(tree);
    tree::FileNode* victim = nullptr;
    for (auto* n : all)
        if (n->name_utf8().find("averylongfilename000.txt") != std::string::npos)
            victim = n;
    ASSERT_NE(victim, nullptr) << "the FULL long name must recover from intact "
                                  "name chunks + the checksum oracle";
    EXPECT_EQ(victim->name_utf8(), "averylongfilename000.txt");
    EXPECT_EQ(victim->deleted_state(), tree::DeletedState::Deleted);
    EXPECT_GE(stats.lfn_recovered, 1u);
}

TEST(FatEngine, DeletedLfnSingleSlotChecksumRecovery) {
    /* one-slot LFN ("report_v2.md" - base 9 chars > 8) with the REAL
     * write-time checksum; deletion destroys slot + 8.3 first bytes. */
    auto img = build_fat16_image();
    u8* root = img.data() + 42 * 512;
    u8 name11[11] = {'R', 'E', 'P', 'O', 'R', 'T', '~', '1', 'M', 'D'};
    name11[10] = ' ';
    const u8 chk = lfn_checksum(name11);
    make_lfn_slot(root + 0 * 32, 0x41, chk, u"report_v2.md");
    make_dentry(root + 1 * 32, "REPORT~1MD ", 0x20, 20, 1000, 0x5A46, 0x6000, true);
    root[0 * 32] = 0xE5;
    root[1 * 32] = 0xE5;

    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    FatScanFlags flags;
    FatScanStats stats;
    ASSERT_EQ(scan_fat(&dev, flags, nullptr, nullptr, nullptr, tree, ra, rd, nullptr, stats), OK);

    auto all = collect_all(tree);
    tree::FileNode* victim = nullptr;
    for (auto* n : all)
        if (n->name_utf8() == "report_v2.md")
            victim = n;
    ASSERT_NE(victim, nullptr) << "single-slot deleted LFN must recover";
    EXPECT_GE(stats.lfn_recovered, 1u);
}

TEST(FatEngine, DeletedLfnTornChecksumKeepsHonestFallback) {
    /* slots disagree on the stored checksum -> no oracle -> the '_'
     * convention fallback (never a fabricated long name). */
    auto img = build_fat16_image();
    u8* root = img.data() + 42 * 512;
    u8 name11[11] = {'A', 'V', 'E', 'R', 'Y', 'L', '~', '1', 'T', 'X', 'T'};
    const u8 chk = lfn_checksum(name11);
    make_lfn_slot(root + 0 * 32, 0x42, chk, u"name000.txt");
    make_lfn_slot(root + 1 * 32, 0x01, u8(chk ^ 0x5A), u"averylongfile"); /* torn */
    make_dentry(root + 2 * 32, "AVERYL~1TXT", 0x20, 20, 1000, 0x5A46, 0x6000, true);
    root[0 * 32] = 0xE5;
    root[1 * 32] = 0xE5;
    root[2 * 32] = 0xE5;

    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    FatScanFlags flags;
    FatScanStats stats;
    ASSERT_EQ(scan_fat(&dev, flags, nullptr, nullptr, nullptr, tree, ra, rd, nullptr, stats), OK);

    auto all = collect_all(tree);
    bool saw_fabricated_long_name = false;
    bool saw_fallback = false;
    for (auto* n : all) {
        if (n->name_utf8().find("averylongfilename") != std::string::npos)
            saw_fabricated_long_name = true;
        if (n->name_utf8() == "_VERYL~1.TXT")
            saw_fallback = true;
    }
    EXPECT_FALSE(saw_fabricated_long_name) << "torn slot set must not yield a long name";
    EXPECT_TRUE(saw_fallback) << "the '_'-convention 8.3 fallback must survive";
}

TEST(FatEngine, LfnDeletedChecksumBijection) {
    /* the oracle is exact: for every 11-byte alias the brute-force returns
     * the ORIGINAL first byte (spot-check across the byte space). */
    for (u32 orig = 1; orig <= 0xFF; orig += 7) {
        u8 name11[11] = {u8(orig), 'X', 'Y', '~', '1', 'T', 'X', 'T', ' ', ' ', ' '};
        const u8 chk = lfn_checksum(name11);
        LfnEntry slot = {};
        slot.sequence = 0x41;
        slot.attributes = ATTR_LFN;
        slot.type = 0;
        slot.checksum = chk;
        slot.first_cluster = 0;
        for (int i = 0; i < 5; ++i)
            slot.name1[i] = 0xFFFF;
        u8 raw[11] = {0xE5, 'X', 'Y', '~', '1', 'T', 'X', 'T', ' ', ' ', ' '};
        u8 first = 0, got_chk = 0;
        std::vector<const LfnEntry*> slots = {&slot};
        ASSERT_TRUE(lfn_deleted_checksum_recover(slots, raw, first, got_chk))
            << "recovery must succeed for orig=" << orig;
        EXPECT_EQ(first, u8(orig)) << "recovered byte must equal the original";
        EXPECT_EQ(got_chk, chk);
    }
}

/* ---- GAP-S6-2: unlinked deleted chains get contiguous-candidate runs ----
 *
 * Bench 40079065019: FAT32 volume-scan cells committed 10143 files 'done'
 * with manifest-matching sizes but only 46 hash-true - the classic
 * first-cluster-only failure (freed chains yield ONE cluster; the writer
 * zero-fills the rest at full size). The contiguous candidate completes the
 * run, validated against the FAT free-state, graded PartialRisk. */

TEST(FatEngine, DeletedUnlinkedChainGetsContiguousRun) {
    auto img = build_fat16_image();
    u8* root = img.data() + 42 * 512;
    const size_t DATA = 74 * 512;
    /* 4-cluster deleted-unlinked file at cluster 40 (FAT stays all-FREE -
     * the zeroed links are exactly the unlink signature); content markers
     * survive in the data area. */
    make_dentry(root + 6 * 32, "UNLINK  TXT", 0x20, 40, 2048, 0x5A46, 0x6000, true);
    for (u32 c = 40; c <= 43; ++c)
        std::memcpy(img.data() + DATA + (c - 2) * 512, "UNLINKED-DATA-MARKER", 20);

    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    FatScanFlags flags;
    FatScanStats stats;
    ASSERT_EQ(scan_fat(&dev, flags, nullptr, nullptr, nullptr, tree, ra, rd, nullptr, stats), OK);

    auto all = collect_all(tree);
    tree::FileNode* victim = nullptr;
    for (auto* n : all)
        if (n->name_utf8() == "_NLINK.TXT")
            victim = n;
    ASSERT_NE(victim, nullptr) << "unlinked deleted entry must surface";
    /* the FULL 4-cluster contiguous run (was: 1 cluster pre-fix) */
    ASSERT_EQ(victim->props()->extents.size(), 1u);
    EXPECT_EQ(victim->props()->extents[0].offset, u64(72 + 40) * 512);
    EXPECT_EQ(victim->props()->extents[0].length, u64(4) * 512);
    EXPECT_EQ(victim->props()->logical_size, 2048u);
    /* inferred chain: PartialRisk, never Excellent */
    EXPECT_EQ(victim->confidence(), tree::Confidence::PartialRisk);
}

TEST(FatEngine, DeletedUnlinkedChainTruncatesAtReusedCluster) {
    auto img = build_fat16_image();
    u8* root = img.data() + 42 * 512;
    const size_t FAT1 = 2 * 512;
    /* 4-cluster candidate at 50, but cluster 52 is reallocated to a LIVE
     * file (FAT entry non-zero) -> the run truncates to [50,51]; the
     * 2-cluster prefix is honest partial content. */
    make_dentry(root + 6 * 32, "TRUNC   TXT", 0x20, 50, 2048, 0x5A46, 0x6000, true);
    set_fat16_entry(img, FAT1, 52, 0xFFFF); /* live allocation inside the run */
    set_fat16_entry(img, 22 * 512, 52, 0xFFFF);

    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    FatScanFlags flags;
    FatScanStats stats;
    ASSERT_EQ(scan_fat(&dev, flags, nullptr, nullptr, nullptr, tree, ra, rd, nullptr, stats), OK);

    auto all = collect_all(tree);
    tree::FileNode* victim = nullptr;
    for (auto* n : all)
        if (n->name_utf8() == "_RUNC.TXT")
            victim = n;
    ASSERT_NE(victim, nullptr);
    ASSERT_EQ(victim->props()->extents.size(), 1u);
    EXPECT_EQ(victim->props()->extents[0].offset, u64(72 + 50) * 512);
    EXPECT_EQ(victim->props()->extents[0].length, u64(2) * 512); /* cut at 52 */
    EXPECT_EQ(victim->confidence(), tree::Confidence::PartialRisk);
}

TEST(FatEngine, DeletedLinkedChainStillPrefersFatTruth) {
    /* an INTACT linked chain (links survive deletion) must keep the
     * FAT-walked run and its Excellent grading - the contiguous heuristic
     * only completes runs that came up SHORT. (Chain 20->21 from the base
     * fixture, size 1000 = 2 clusters.) */
    auto img = build_fat16_image();
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    FatScanFlags flags;
    FatScanStats stats;
    ASSERT_EQ(scan_fat(&dev, flags, nullptr, nullptr, nullptr, tree, ra, rd, nullptr, stats), OK);
    auto all = collect_all(tree);
    tree::FileNode* del = nullptr;
    for (auto* n : all)
        if (n->name_utf8() == "_EL.TXT")
            del = n;
    ASSERT_NE(del, nullptr);
    ASSERT_EQ(del->props()->extents.size(), 1u);
    EXPECT_EQ(del->props()->extents[0].offset, u64(72 + 20) * 512);
    EXPECT_EQ(del->props()->extents[0].length, u64(2) * 512);
    EXPECT_EQ(del->confidence(), tree::Confidence::Excellent); /* FAT-linked truth */
}

/* ---- N-002: the og's tolerant EOC families -------------------------------- */

TEST(FatEngine, Fat32EocMask24AndAlternateFamiliesEndClean) {
    /* N-002 (diff-to-fix): the og's chain termination recognizes EOC
     * variants some BIOS/embedded formatters write (FATSearchDLL Next,
     * :8030-8048): the mask-24 family (dirty high bits, e.g. 0x27FFFFF8 /
     * 0xFFFFFFF8) and the reserved 0x0FFFFFF0..F7 range. Our core
     * followed the masked value as a cluster number -> out of range ->
     * chain "broken"; the og ends those chains cleanly. Unit-level on
     * FatTable + follow_chain per the finding's reproduction plan. */
    std::vector<u8> img(u64(66562) * 512, 0);
    u8* b = img.data();
    std::memcpy(b + 3, "MSDOS5.0", 8);
    put16(img, 0x0B, 512);
    b[0x0D] = 1;
    put16(img, 0x0E, 8);
    b[0x10] = 2;
    put16(img, 0x11, 0);
    put16(img, 0x13, 0);
    b[0x15] = 0xF8;
    put16(img, 0x16, 0); /* FAT32: spf16 == 0 */
    put32v(img, 0x20, 66562);
    put32v(img, 0x24, 512);
    put32v(img, 0x2C, 2);
    b[510] = 0x55;
    b[511] = 0xAA;
    const size_t FAT1 = 8 * 512, FAT2 = 520 * 512;
    auto link32 = [&](u32 c, u32 v) {
        put32v(img, FAT1 + c * 4, v);
        put32v(img, FAT2 + c * 4, v);
    };
    /* mask-24 family with high dirty bits (a real formatter artifact) */
    link32(5, 6);
    link32(6, 0x27FFFFF8);
    /* full-dirty canonical form (the og's raw 0xFFFFFFF8 check) */
    link32(20, 0xFFFFFFF8);
    /* alternate family: reserved 0x0FFFFFF5 (pins the existing branch) */
    link32(10, 0x0FFFFFF5);

    u8 boot[512];
    std::memcpy(boot, b, 512);
    MemDevice dev(std::move(img));
    FatParams p;
    ASSERT_EQ(validate_boot(boot, u64(66562) * 512, p), OK);
    ASSERT_EQ(p.flavor, Flavor::Fat32);
    FatTable t;
    ASSERT_EQ(t.load(&dev, p), OK);

    /* mask-24 chain: clean end, both clusters, no broken flag */
    ChainResult r1 = follow_chain(t, 5, 0x10000, nullptr);
    EXPECT_FALSE(r1.broken) << "mask-24 EOC variant must end the chain cleanly";
    ASSERT_EQ(2u, r1.clusters.size());
    EXPECT_EQ(5u, r1.clusters[0]);
    EXPECT_EQ(6u, r1.clusters[1]);

    /* full-dirty canonical: clean single-cluster chain */
    ChainResult r2 = follow_chain(t, 20, 0x10000, nullptr);
    EXPECT_FALSE(r2.broken);
    ASSERT_EQ(1u, r2.clusters.size());

    /* alternate family: clean end */
    ChainResult r3 = follow_chain(t, 10, 0x10000, nullptr);
    EXPECT_FALSE(r3.broken);
    ASSERT_EQ(1u, r3.clusters.size());
}

TEST(FatEngine, Fat16BadClusterStillCanonical) {
    /* I-002 (diff-to-fix, INFORMATIONAL): the DELIBERATE delta is FAT12-only
     * (og follows 0xFF7 raw, CORRECTIONS row 3; we truncate honestly). On
     * FAT16 the og canonicalizes 0xFFF7 to BAD like us - this pin guards
     * the boundary: nobody "fixing" FAT12 parity may break the FAT16
     * semantics both sides share. */
    auto img = build_fat16_image();
    /* link cluster 40 (currently free) -> BAD chain: 40 -> 41 -> 0xFFF7 */
    const size_t FAT1 = 2 * 512, FAT2 = 22 * 512;
    auto link16 = [&](u32 c, u16 v) {
        set_fat16_entry(img, FAT1, c, v);
        set_fat16_entry(img, FAT2, c, v);
    };
    link16(40, 41);
    link16(41, u16(0xFFF7));
    u8 boot[512];
    std::memcpy(boot, img.data(), 512);
    MemDevice dev(std::move(img));
    FatParams p;
    ASSERT_EQ(validate_boot(boot, u64(5074) * 512, p), OK);
    FatTable t;
    ASSERT_EQ(t.load(&dev, p), OK);
    ChainResult r = follow_chain(t, 40, 0x10000, nullptr);
    EXPECT_TRUE(r.broken) << "FAT16 BAD (0xFFF7) must canonicalize to CHAIN_BAD";
    ASSERT_EQ(2u, r.clusters.size());
}

/* ---- N-021: invalid/zero FAT dates yield the ZERO FileTime (og parity) -- */

namespace {

/* Build the standard FAT16 fixture and plant three extra live dentries with
 * invalid dates (own copy - the shared fixture is untouched). */
std::vector<u8> build_fat16_image_with_bad_dates(u16 zero_date_dentry_date, u16 month13_date,
                                                 u16 day0_date) {
    auto img = build_fat16_image();
    u8* root = img.data() + 42 * 512;
    /* cluster 15 already carries an EOC chain in the fixture; reuse it */
    make_dentry(root + 6 * 32, "ZERODAT TXT", 0x20, 15, 100, zero_date_dentry_date, 0x6000, false);
    make_dentry(root + 7 * 32, "MONTH13 TXT", 0x20, 15, 100, month13_date, 0x6000, false);
    make_dentry(root + 8 * 32, "DAYZERO TXT", 0x20, 15, 100, day0_date, 0x6000, false);
    return img;
}

tree::FileNode* find_node(tree::ResultTree& tree, const char* want) {
    for (auto* n : collect_all(tree)) {
        if (n->name_utf8() == want)
            return n;
    }
    return nullptr;
}


namespace {
/* M-011: a large-FAT32 boot-sector writer (spf above the old 16 MiB cap).
 * 34000 sectors/FAT = 17 MiB table; total sized so the data area holds
 * 66000 clusters (FAT32 by the cluster bracket). */
void build_large_fat32_boot(std::vector<u8>& img, u32 spf) {
    u8* b = img.data();
    put16(img, 0x0B, 512);
    b[0x0D] = 1;
    put16(img, 0x0E, 8);
    b[0x10] = 2;
    put16(img, 0x11, 0);
    put16(img, 0x13, 0);
    b[0x15] = 0xF8;
    put16(img, 0x16, 0);
    put32v(img, 0x20, 8 + 2 * spf + 66000);
    put32v(img, 0x24, spf);
    put32v(img, 0x2C, 2);
    std::memcpy(b + 0x52, "FAT32   ", 8);
    b[510] = 0x55;
    b[511] = 0xAA;
}
} // namespace

TEST(FatEngine, LargeFat32TableAccepted) {
    /* M-011: the og's FAT32 window is spf in [256, 0x96000] sectors
     * (FUN_180036e90 :44775) = 128 KiB..300 MiB whole-loaded. A 17 MiB FAT
     * (34000 sectors) scanned in the og; the core's flat 16 MiB cap
     * returned ERR_BAD_PARAMETER and the volume vanished. */
    const u32 spf = 34000;
    std::vector<u8> img(u64(8 + 2 * spf + 66000) * 512, 0);
    build_large_fat32_boot(img, spf);
    const size_t FAT1 = 8 * 512, FAT2 = (8 + spf) * 512;
    auto link32 = [&](u32 c, u32 v) {
        put32v(img, FAT1 + c * 4, v);
        put32v(img, FAT2 + c * 4, v);
    };
    /* FAT[0]/FAT[1] media words (family-tolerant form) */
    put32v(img, FAT1, 0x0FFFFF00 | 0xF8);
    put32v(img, FAT1 + 4, 0x0FFFFFF8);
    put32v(img, FAT2, 0x0FFFFF00 | 0xF8);
    put32v(img, FAT2 + 4, 0x0FFFFFF8);
    link32(2, 3);
    link32(3, 0x0FFFFFFF);
    link32(100, 0x0FFFFFFF);
    const u64 data_start = (8 + 2ull * spf) * 512;
    std::memcpy(img.data() + data_start + 8, "BIGFAT32DATA", 12);
    /* root dir cluster 2: one dentry pointing at cluster 100 */
    {
        u8* d = img.data() + data_start + 0 * 512 + 0 * 32; /* cluster 2 byte 0 */
        /* 8.3 name: "BIG     " + ext "TXT" (name d[0..7], ext d[8..10]) */
        std::memcpy(d, "BIG     TXT", 11);
        d[0x0B] = 0x20;   /* archive */
        d[0x1A] = 100;    /* first cluster low */
        d[0x1C] = 12;     /* size */
    }
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    FatScanFlags flags;
    FatScanStats stats;
    u32 rc = scan_fat(&dev, flags, nullptr, nullptr, nullptr, tree, ra, rd, nullptr, stats);
    ASSERT_EQ(rc, OK) << "17 MiB FAT32 table must scan (og window up to 300 MiB)";
    auto all = collect_all(tree);
    bool big = false;
    for (auto* n : all)
        if (n->name_utf8() == "BIG.TXT")
            big = true;
    EXPECT_TRUE(big) << "the large-table volume's file recovered";
}

TEST(FatEngine, SmallFat16TableDeclined) {
    /* M-011: the og's FAT16 window requires spf >= 12 sectors
     * (FUN_180036e90 :44743: 0xC/(bps>>9) <= spf). An spf=8 "FAT16" is
     * FAT12-window territory the og never scans as FAT16. */
    auto img = build_fat16_image();
    /* rewrite the BPB with spf=8, keeping the rest of the shape */
    put16(img, 0x16, 8);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    FatScanFlags flags;
    FatScanStats stats;
    u32 rc = scan_fat(&dev, flags, nullptr, nullptr, nullptr, tree, ra, rd, nullptr, stats);
    EXPECT_EQ(rc, ERR_BAD_PARAMETER) << "spf=8 FAT16 declined (og window)";
}


namespace {
/* a device that FAILS reads over a byte range (the FAT1 region) */
class HoleDevice final : public IStorageDevice {
public:
    HoleDevice(std::vector<u8> b, u64 hole_off, u64 hole_len)
        : data_(std::move(b)), off_(hole_off), len_(hole_len) {
        geom_.logical_sector = 512;
        geom_.physical_sector = 512;
        geom_.sector_count = data_.size() / 512;
        geom_.total_bytes = geom_.sector_count * 512;
    }
    u32 read_at(u64 o, void* buf, u32 len) override {
        if (o < off_ + len_ && o + len > off_)
            return ERR_READ_FAILED; /* any overlap with the hole fails */
        if (o > data_.size() || len > data_.size() - o)
            return ERR_RANGE_OUT_OF_BOUNDS;
        std::memcpy(buf, data_.data() + o, len);
        return OK;
    }
    const Geometry& geometry() const override { return geom_; }

private:
    std::vector<u8> data_;
    u64 off_, len_;
    Geometry geom_;
};
} // namespace

TEST(FatEngine, MirrorShiftedCopyRejected) {
    /* M-012(b): a mirror answer that is a shifted copy of the primary
     * layout (mirror(c+1) == alt+1) is REJECTED by the og's scrutiny
     * (FUN_180041de0 :54230-32); the old core followed it into bogus
     * backward chains. */
    auto img = build_fat16_image();
    /* FAT1 cluster 20 (FREE in the fixture's unused space); plant a torn
     * mirror: FAT2[20] = 19 (backward), FAT2[21] = 20 (= alt+1: shifted) */
    const size_t FAT2 = 22 * 512; /* fixture: reserved=1, spf=20? see below */
    (void)FAT2;
    MemDevice dev(std::move(img));
    /* direct FatTable-level check: construct params + table by hand */
    FatParams p;
    {
        auto img2 = build_fat16_image();
        const u8* b = img2.data();
        p.bytes_per_sector = u16_from_bytes_le(b + 0x0B);
        p.sectors_per_cluster = b[0x0D];
        p.cluster_bytes = u32(p.sectors_per_cluster) * p.bytes_per_sector;
        p.reserved_sectors = u16_from_bytes_le(b + 0x0E);
        p.fat_count = b[0x10];
        p.fat_sectors = u16_from_bytes_le(b + 0x16);
        p.fat_start = u64(p.reserved_sectors) * p.bytes_per_sector;
        p.fat2_start = p.fat_start + u64(p.fat_sectors) * p.bytes_per_sector;
        p.root_entries = u16_from_bytes_le(b + 0x11);
        p.flavor = Flavor::Fat16;
        p.cluster_count = 5000;
    }
    /* rebuild a table with a torn mirror directly */
    auto img3 = build_fat16_image();
    const size_t f1 = size_t(p.fat_start), f2 = size_t(p.fat2_start);
    /* FAT1[50] = 0 (free); FAT2[50] = 49 (backward); FAT2[51] = 50 (alt+1) */
    put16(img3, f1 + 50 * 2, 0);
    put16(img3, f2 + 50 * 2, 49);
    put16(img3, f2 + 51 * 2, 50);
    MemDevice dev3(std::move(img3));
    FatTable t;
    ASSERT_EQ(t.load(&dev3, p), OK);
    EXPECT_EQ(t.next(50), 0u) << "shifted-copy mirror answer rejected";
    /* contiguity-consistent backward answer IS believed */
    auto img4 = build_fat16_image();
    put16(img4, f1 + 60 * 2, 0);  /* primary free */
    put16(img4, f2 + 60 * 2, 59); /* backward */
    put16(img4, f2 + 59 * 2, 60); /* mirror(59) == 60 == alt+1: contiguous */
    put16(img4, f2 + 58 * 2, 0);  /* no cross-link claim */
    MemDevice dev4(std::move(img4));
    FatTable t2;
    ASSERT_EQ(t2.load(&dev4, p), OK);
    EXPECT_EQ(t2.next(60), 59u) << "contiguity-consistent backward answer believed";
}

TEST(FatEngine, MirrorCrossLinkRejected) {
    /* M-012(b): mirror(alt-1) == alt (the target is already claimed) is a
     * cross-link - rejected by the og's scrutiny (:54236-38). */
    auto img = build_fat16_image();
    FatParams p;
    {
        const u8* b = img.data();
        p.bytes_per_sector = u16_from_bytes_le(b + 0x0B);
        p.sectors_per_cluster = b[0x0D];
        p.cluster_bytes = u32(p.sectors_per_cluster) * p.bytes_per_sector;
        p.reserved_sectors = u16_from_bytes_le(b + 0x0E);
        p.fat_count = b[0x10];
        p.fat_sectors = u16_from_bytes_le(b + 0x16);
        p.fat_start = u64(p.reserved_sectors) * p.bytes_per_sector;
        p.fat2_start = p.fat_start + u64(p.fat_sectors) * p.bytes_per_sector;
        p.flavor = Flavor::Fat16;
        p.cluster_count = 5000;
    }
    const size_t f1 = size_t(p.fat_start), f2 = size_t(p.fat2_start);
    /* FAT1[70] = 0; FAT2[70] = 69 (backward); FAT2[68] = 69 (claimed!) */
    put16(img, f1 + 70 * 2, 0);
    put16(img, f2 + 70 * 2, 69);
    put16(img, f2 + 68 * 2, 69); /* cross-link: 69 already pointed at */
    put16(img, f2 + 69 * 2, 70); /* and contiguous - but the claim kills it */
    MemDevice dev(std::move(img));
    FatTable t;
    ASSERT_EQ(t.load(&dev, p), OK);
    EXPECT_EQ(t.next(70), 0u) << "cross-linked mirror answer rejected";
}

TEST(FatEngine, MirrorOnlyScanFat1Unreadable) {
    /* M-012(c): FAT1 unreadable, FAT2 intact - the og scans the volume on
     * the raw mirror (child1 == NULL); the core used to fail the whole
     * scan with ERR_READ_FAILED. */
    auto img = build_fat16_image();
    FatParams p;
    u64 fat1_len;
    {
        const u8* b = img.data();
        p.bytes_per_sector = u16_from_bytes_le(b + 0x0B);
        p.sectors_per_cluster = b[0x0D];
        p.cluster_bytes = u32(p.sectors_per_cluster) * p.bytes_per_sector;
        p.reserved_sectors = u16_from_bytes_le(b + 0x0E);
        p.fat_count = b[0x10];
        p.fat_sectors = u16_from_bytes_le(b + 0x16);
        p.fat_start = u64(p.reserved_sectors) * p.bytes_per_sector;
        p.fat2_start = p.fat_start + u64(p.fat_sectors) * p.bytes_per_sector;
        fat1_len = u64(p.fat_sectors) * p.bytes_per_sector;
        p.flavor = Flavor::Fat16;
        p.cluster_count = 5000;
        p.root_entries = u16_from_bytes_le(b + 0x11);
    }
    /* punch a hole over the whole FAT1 region */
    HoleDevice dev(std::move(img), p.fat_start, fat1_len);
    FatTable t;
    ASSERT_EQ(t.load(&dev, p), OK) << "mirror-only load succeeds";
    EXPECT_TRUE(t.mirror_only());
    /* the fixture's chains (10->11->12 EOC, mirrored in FAT2) resolve */
    EXPECT_EQ(t.next(10), 11u);
    /* and the full scan works on the mirror-only device */
    auto img2 = build_fat16_image();
    HoleDevice dev2(std::move(img2), p.fat_start, fat1_len);
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    FatScanFlags flags;
    FatScanStats stats;
    u32 rc = scan_fat(&dev2, flags, nullptr, nullptr, nullptr, tree, ra, rd, nullptr, stats);
    EXPECT_EQ(rc, OK) << "the whole volume scans on the raw mirror";
    EXPECT_EQ(stats.mirror_only_scan, 1u);
    auto all = collect_all(tree);
    bool readme = false;
    for (auto* n : all)
        if (n->name_utf8() == "readme_document_v1.txt")
            readme = true;
    EXPECT_TRUE(readme) << "live file recovered through the mirror";
}

} // namespace

TEST(FatEngine, InvalidDateYieldsZeroFileTime) {
    /* N-021: date 0x0000 = "date unknown" (all fields zero) - the
     * DosDateTimeToFileTime failure class. The og zeroes the props slot
     * (FATSearchDLL_decompiled.c:32135-32138); the core used to store
     * 1970-01-01 (the epoch delta added to 0). */
    auto img = build_fat16_image_with_bad_dates(0x0000, 0x01A1 /* month 13 */, 0x0020 /* day 0 */);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost");
    FatScanFlags flags;
    FatScanStats stats;
    ASSERT_EQ(scan_fat(&dev, flags, nullptr, nullptr, nullptr, tree, ra, rd, rl, stats), OK);
    tree::FileNode* z = find_node(tree, "ZERODAT.TXT");
    ASSERT_NE(z, nullptr);
    EXPECT_EQ(z->props()->times.created, 0ULL); /* zero FileTime = 1601-01-01 */
    EXPECT_EQ(z->props()->times.modified, 0ULL);
    EXPECT_EQ(z->props()->times.accessed, 0ULL);
}

TEST(FatEngine, InvalidMonthYieldsZeroFileTime) {
    auto img = build_fat16_image_with_bad_dates(0x0000, 0x01A1, 0x0020);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost");
    FatScanFlags flags;
    FatScanStats stats;
    ASSERT_EQ(scan_fat(&dev, flags, nullptr, nullptr, nullptr, tree, ra, rd, rl, stats), OK);
    tree::FileNode* m = find_node(tree, "MONTH13.TXT");
    ASSERT_NE(m, nullptr); /* month 13 is not a month */
    EXPECT_EQ(m->props()->times.created, 0ULL);
    EXPECT_EQ(m->props()->times.modified, 0ULL);
    EXPECT_EQ(m->props()->times.accessed, 0ULL);
}

TEST(FatEngine, InvalidDayYieldsZeroFileTime) {
    auto img = build_fat16_image_with_bad_dates(0x0000, 0x01A1, 0x0020);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost");
    FatScanFlags flags;
    FatScanStats stats;
    ASSERT_EQ(scan_fat(&dev, flags, nullptr, nullptr, nullptr, tree, ra, rd, rl, stats), OK);
    tree::FileNode* d = find_node(tree, "DAYZERO.TXT");
    ASSERT_NE(d, nullptr); /* day 0 is not a day */
    EXPECT_EQ(d->props()->times.created, 0ULL);
    EXPECT_EQ(d->props()->times.modified, 0ULL);
    EXPECT_EQ(d->props()->times.accessed, 0ULL);
}

TEST(FatEngine, ValidDateYieldsCorrectFileTime) {
    /* N-021 regression guard: valid dates are unchanged by the fix.
     * 0x5A46 = 2025-02-06, 0x6000 = 12:00 -> unix 1738843200. */
    auto img = build_fat16_image_with_bad_dates(0x0000, 0x01A1, 0x0020);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost");
    FatScanFlags flags;
    FatScanStats stats;
    ASSERT_EQ(scan_fat(&dev, flags, nullptr, nullptr, nullptr, tree, ra, rd, rl, stats), OK);
    tree::FileNode* r = find_node(tree, "readme_document_v1.txt");
    ASSERT_NE(r, nullptr);
    EXPECT_EQ(r->props()->times.modified, 1738843200ULL * 10000000ULL + 116444736000000000ULL);
}

/* ---- N-010a/N-010b/N-010c/N-022 (diff-to-fix session) ------------------ */

TEST(FatEngine, FlavorBoundary65524IsFat32) {
    /* N-010a: the og's FATSearchDLL boundary (FUN_180007e50 :5219-5231)
     * makes 0xFFF4 = 65524 data clusters FAT32 (lower bound inclusive);
     * the core's old `< 65525` read it as FAT16. The FAT32 fixture's
     * 65530 clusters stay FAT32; 65523 stays FAT16. */
    EXPECT_EQ(flavor_from_clusters(65524), Flavor::Fat32);
    EXPECT_EQ(flavor_from_clusters(65523), Flavor::Fat16);
    EXPECT_EQ(flavor_from_clusters(4085), Flavor::Fat16);
    EXPECT_EQ(flavor_from_clusters(4084), Flavor::Fat12);
}

TEST(FatEngine, BootStringGateDeclinesTornStringVolume) {
    /* N-010b: the og's engine-open validator (FUN_18002f970) requires
     * "FAT16   " at +0x36 / "FAT32   " at +0x52; a torn reserved area
     * declines the engine on the og (flavor 0xE) and must decline here
     * too - the volume routes to other engines / raw carving. */
    auto img = build_fat16_image();
    std::memset(img.data() + 0x36, 0, 8); /* tear the FS-name string */
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    FatScanFlags flags;
    FatScanStats stats;
    u32 rc = scan_fat(&dev, flags, nullptr, nullptr, nullptr, tree, ra, nullptr, nullptr, stats);
    EXPECT_NE(rc, OK) << "torn FS-name string must decline the FAT engine (og parity)";
}

TEST(FatEngine, BootStringCaseInsensitiveAccepted) {
    /* N-010b: the og uses _strnicmp - "fat16   " (lowercase, some
     * third-party formatters) must open. */
    auto img = build_fat16_image();
    std::memcpy(img.data() + 0x36, "fat16   ", 8);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    FatScanFlags flags;
    FatScanStats stats;
    EXPECT_EQ(scan_fat(&dev, flags, nullptr, nullptr, nullptr, tree, ra, nullptr, nullptr, stats),
              OK);
}

TEST(FatEngine, Fat16LowRootEntriesDeclined) {
    /* N-010b: root_entries >= 224 is the og's engine-open floor
     * (:38054 `0xdf < root`); 16-entry boots are declined. */
    auto img = build_fat16_image();
    put16(img, 0x11, 16);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    FatScanFlags flags;
    FatScanStats stats;
    u32 rc = scan_fat(&dev, flags, nullptr, nullptr, nullptr, tree, ra, nullptr, nullptr, stats);
    EXPECT_NE(rc, OK) << "root_entries < 224 must decline (og engine-open floor)";
}

TEST(FatEngine, Fat16RootEntriesNotSectorMultipleDeclined) {
    /* N-010b: root_entries*32 % bps == 0 (:38055). 225 entries x 32 B
     * is 7200 bytes - not a 512 multiple. */
    auto img = build_fat16_image();
    put16(img, 0x11, 225);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    FatScanFlags flags;
    FatScanStats stats;
    u32 rc = scan_fat(&dev, flags, nullptr, nullptr, nullptr, tree, ra, nullptr, nullptr, stats);
    EXPECT_NE(rc, OK) << "root_entries*32 % bps != 0 must decline";
}

TEST(FatEngine, NumFatsThreeDeclined) {
    /* N-010b: numFATs in {1,2} (:38049). Triple-FAT boots are declined
     * by the og's engine-open validator. */
    auto img = build_fat16_image();
    img[0x10] = 3;
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    FatScanFlags flags;
    FatScanStats stats;
    u32 rc = scan_fat(&dev, flags, nullptr, nullptr, nullptr, tree, ra, nullptr, nullptr, stats);
    EXPECT_NE(rc, OK) << "numFATs == 3 must decline (og {1,2} gate)";
}

TEST(FatEngine, SpcDomainMatchesOgSet) {
    /* N-010b/N-034 correction (verified this session): SPC is a u8 BPB
     * field, so the old any-pow2 check already accepted exactly
     * {1,2,4,8,16,32,64,128} - the og's set (FUN_18002f970 bitmask +
     * 0x80 case) over the same domain. The audit's "SPC 256 accepted"
     * sub-claim was a false positive (256 is unrepresentable in u8).
     * This pin walks the whole 0..255 domain so any future loosening
     * (e.g. a u16 field) fails loudly. */
    auto img = build_fat16_image();
    u8 boot[512];
    FatParams p;
    for (int v = 0; v <= 255; ++v) {
        img[0x0D] = u8(v);
        std::memcpy(boot, img.data(), 512);
        const bool pow2 = v >= 1 && (v & (v - 1)) == 0;
        const u32 rc = validate_boot(boot, u64(5074) * 512, p);
        if (pow2)
            EXPECT_EQ(rc, OK) << "SPC " << v << " must be accepted (og set)";
        else
            EXPECT_NE(rc, OK) << "SPC " << v << " must be rejected (og set)";
    }
}

TEST(FatEngine, DateSanityRejectedInStrictMode) {
    /* N-010c: FUN_1800365a0 :44298-44315 - a nonzero date field with
     * month > 12 or day == 0 rejects the dentry (strict mode). The
     * default flow keeps the N-021 zero-FileTime emission. */
    /* month=13 (0x0D << 5 | 6), day valid */
    const u16 month13 = u16((13u << 5) | 6u);
    /* day = 0, month valid */
    const u16 day0 = u16((7u << 5) | 0u);
    auto img = build_fat16_image_with_bad_dates(0, month13, day0);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    FatScanFlags flags;
    flags.strict_date_sanity = true;
    FatScanStats stats;
    ASSERT_EQ(scan_fat(&dev, flags, nullptr, nullptr, nullptr, tree, ra, nullptr, nullptr, stats),
              OK);
    EXPECT_EQ(find_node(tree, "MONTH13.TXT"), nullptr) << "month 13 dentry must be rejected";
    EXPECT_EQ(find_node(tree, "DAYZERO.TXT"), nullptr) << "day 0 dentry must be rejected";
    EXPECT_NE(find_node(tree, "ZERODAT.TXT"), nullptr) << "whole-zero date passes the gate";
    EXPECT_EQ(stats.date_rejected, 2u);
    /* default flow (flag off): N-021 semantics preserved - emitted
     * with zero FileTime, not rejected */
    MemDevice dev2(build_fat16_image_with_bad_dates(0, month13, day0));
    tree::ResultTree tree2;
    auto* ra2 = tree2.add_root(u"Found");
    FatScanFlags default_flags;
    FatScanStats stats2;
    ASSERT_EQ(scan_fat(&dev2, default_flags, nullptr, nullptr, nullptr, tree2, ra2, nullptr,
                       nullptr, stats2),
              OK);
    EXPECT_NE(find_node(tree2, "MONTH13.TXT"), nullptr) << "default flow keeps N-021 emission";
    EXPECT_EQ(stats2.date_rejected, 0u);
}

TEST(FatEngine, BadPrimaryServedWithoutMirrorConsult) {
    /* N-022: the og's CDoubleFATTable::GetNext consults the mirror only
     * on a primary FREE entry; a primary BAD truncates the chain even
     * when the mirror holds a live link (torn pair). */
    auto img = build_fat16_image();
    const size_t FAT1 = 2 * 512, FAT2 = 22 * 512;
    /* torn pair at cluster 10: primary BAD, mirror says 11 */
    set_fat16_entry(img, FAT1, 10, u16(0xFFF7));
    set_fat16_entry(img, FAT2, 10, 11);
    /* keep the rest of the chain intact on both */
    u8 boot[512];
    std::memcpy(boot, img.data(), 512);
    MemDevice dev(std::move(img));
    FatParams p;
    ASSERT_EQ(validate_boot(boot, u64(5074) * 512, p), OK);
    FatTable t;
    ASSERT_EQ(t.load(&dev, p), OK);
    /* default (og parity): the chain truncates at the BAD cluster */
    ChainResult r = follow_chain(t, 10, 0x10000, nullptr);
    EXPECT_TRUE(r.broken) << "primary BAD must truncate (og serves primary directly)";
    ASSERT_EQ(1u, r.clusters.size()) << "no mirror consult on primary BAD";
    /* opt-in salvage: the torn pair recovers through the mirror link */
    t.set_mirror_bad_salvage(true);
    ChainResult rs = follow_chain(t, 10, 0x10000, nullptr);
    EXPECT_FALSE(rs.broken) << "opt-in salvage continues through the mirror";
    ASSERT_EQ(3u, rs.clusters.size());
    EXPECT_EQ(rs.clusters[1], 11u);
    EXPECT_EQ(rs.clusters[2], 12u);
}

TEST(FatEngine, PrimaryFreeStillConsultsMirror) {
    /* N-022 companion pin: primary FREE + mirror link still serves the
     * mirror answer (unchanged by the fix - the og's gated-serve path). */
    auto img = build_fat16_image();
    const size_t FAT1 = 2 * 512, FAT2 = 22 * 512;
    set_fat16_entry(img, FAT1, 10, 0);  /* primary FREE */
    set_fat16_entry(img, FAT2, 10, 11); /* mirror alive */
    u8 boot[512];
    std::memcpy(boot, img.data(), 512);
    MemDevice dev(std::move(img));
    FatParams p;
    ASSERT_EQ(validate_boot(boot, u64(5074) * 512, p), OK);
    FatTable t;
    ASSERT_EQ(t.load(&dev, p), OK);
    EXPECT_EQ(t.next(10), 11u) << "primary FREE must consult the mirror (og gate)";
}