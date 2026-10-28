/* FAT engine end-to-end tests: synthetic FAT16 + FAT32 volumes.
 *
 * FAT16 fixture (5074 sectors x 512B, 5000 clusters -> flavor FAT16):
 *   reserved 2 | FAT1 2x20 | FAT2 2x20 | root 32 | data
 *   root entries: LFN readme (clusters 10-12), #EL.TXT deleted-intact
 *   (20-21), #ONE.TXT deleted-unlinked (30 FREE), SUBDIR dir (cluster 5)
 *   containing INNER.TXT (cluster 15).
 *
 * FAT32 fixture (66562 sectors, 65530 clusters): root cluster chain 2->3,
 * one live photo.jpg; primary boot corrupted -> backup-boot fallback.
 */
#include <gtest/gtest.h>
#include "cr/fat/cr_fat_engine.h"
#include "cr/cr_text.h"
#include "cr/storage/cr_device.h"
#include <cstdio>
#include <cstdlib>
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
/* N-018/UBSan: byte-wise u16 store for raw (possibly unaligned) buffers */
inline void put_u16(u8* p, u16 x) {
    p[0] = u8(x);
    p[1] = u8(x >> 8);
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
    /* N-018/UBSan: the on-disk LFN layout puts u16 name arrays at ODD
     * offsets (name1 at +1); storing through u16* members is unaligned
     * UB flagged by the UBSan lane. Same bytes, byte-wise stores. */
    std::u16string f = fragment;
    slot[0] = seq; /* sequence */
    slot[11] = ATTR_LFN;
    slot[12] = 0; /* type */
    slot[13] = checksum;
    put_u16(slot + 26, 0); /* first_cluster */
    const int counts[3] = {5, 6, 2};
    const size_t offs[3] = {1, 14, 28}; /* name1/name2/name3 */
    size_t fi = 0;
    for (int p = 0; p < 3; ++p)
        for (int c = 0; c < counts[p]; ++c)
            put_u16(slot + offs[p] + 2 * size_t(c), (fi < f.size()) ? f[fi++] : 0xFFFF);
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
    /* M-015: canonical media words (real FAT16 volumes always carry
     * FAT[0]=0xFFF8, FAT[1]=0xFFFF; the whole-table validator the og
     * applies requires the exact pair, u32@FAT[0] == 0xFFFFFFF8) */
    set_fat16_entry(img, FAT1, 0, 0xFFF8);
    set_fat16_entry(img, FAT1, 1, 0xFFFF);
    set_fat16_entry(img, FAT2, 0, 0xFFF8);
    set_fat16_entry(img, FAT2, 1, 0xFFFF);
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

TEST(FatEngine, FreeMapExcludesVirtualClustersAndMatchesFAT) {
    /* S-009 hardening: the FAT free map must never claim the VIRTUAL
     * reserved clusters 0/1. raw() answers 0 for them (out of table), so
     * the first landing's fill loop (c from 0) set their bits FREE and
     * the linear base math painted [data_base - 2*cb, data_base) - the
     * LAST TWO ROOT-DIRECTORY SECTORS on this fixture - as free space.
     * This test pins the exclusion + the exact free-run truth.
     * Fixture chains: 10->11->12 (readme), 20->21 (del), 30 FREE (gone),
     * 5 (subdir), 15 (inner); max_cluster 5001. Free runs: 2..4, 6..9,
     * 13..14, 16..19, 22..5001 (30 is free - one run). */
    auto img = build_fat16_image();
    MemDevice dev(std::move(img));

    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost");
    FatScanFlags flags;
    FatScanStats stats;
    storage::FreeMap fm;
    u32 rc = scan_fat(&dev, flags, nullptr, nullptr, nullptr, tree, ra, rd, rl, stats, &fm);
    ASSERT_EQ(rc, OK) << err_text(rc);

    const u64 DATA = 74 * 512;       /* cluster 2 = sector 74 */
    const u64 VIRT = DATA - 2 * 512; /* the virtual clusters' byte span */
    ASSERT_TRUE(fm.valid());
    EXPECT_EQ(fm.base_byte, VIRT);
    EXPECT_EQ(fm.cluster_bytes, 512u);
    EXPECT_EQ(fm.cluster_count, 5002u); /* 0..5001 (0/1 virtual) */

    /* THE DEFECT PIN: the pre-data region (root-dir tail here) is NOT
     * free - the virtual clusters are excluded from the free set */
    EXPECT_FALSE(fm.range_free(VIRT, 1024))
        << "virtual clusters 0/1 must never read free (root-dir/FAT region)";
    EXPECT_FALSE(fm.is_free(0));
    EXPECT_FALSE(fm.is_free(1));

    /* exact free-run truth, expressed as per-cluster checks */
    auto cl_free = [&](u32 c) { return fm.range_free(DATA + u64(c - 2) * 512, 512); };
    for (u32 c : {2u, 3u, 4u, 6u, 7u, 8u, 9u, 13u, 14u, 16u, 17u, 18u, 19u, 22u, 29u, 30u, 31u,
                  100u, 5001u})
        EXPECT_TRUE(cl_free(c)) << "cluster " << c << " must be free";
    for (u32 c : {5u, 10u, 11u, 12u, 15u, 20u, 21u})
        EXPECT_FALSE(cl_free(c)) << "cluster " << c << " is FAT-linked (not free)";
}

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
        else if (nm == "#EL.TXT")
            del = n; /* deleted 8.3 - the og's '#' placeholder (N-172) */
        else if (nm == "#ONE.TXT")
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

TEST(FatEngine, LfnIllegalCharFallsBackTo83) {
    /* N-176 (diff-to-fix): an assembled LFN containing any of the nine
     * characters a real Windows name can never carry is discarded and
     * the 8.3 alias used (FATSearchDLL :32084-32099, the strncpy of
     * "?*|\\/:\"<>" + the nested compare loop). Checksum-valid
     * corrupt remnants are the recovery population this shapes. */
    const char16_t illegal[9] = {u'?', u'*', u'|', u'\\', u'/', u':', u'"', u'<', u'>'};
    for (int k = 0; k < 9; ++k) {
        auto img = build_fat16_image();
        u8* root = img.data() + 42 * 512;
        /* two slots: "re<illegal>adme.txt" style - 14 chars, seq 1|2 */
        char16_t longname[20];
        int ln = 0;
        longname[ln++] = u'r';
        longname[ln++] = u'e';
        longname[ln++] = illegal[k];
        longname[ln++] = u'a';
        longname[ln++] = u'd';
        longname[ln++] = u'm';
        longname[ln++] = u'e';
        longname[ln++] = u'_';
        longname[ln++] = u'1';
        longname[ln++] = u'.';
        longname[ln++] = u't';
        longname[ln++] = u'x';
        longname[ln++] = u't';
        longname[ln] = 0;
        u8 name11[11] = {'R', 'E', 'A', 'D', 'M', 'E', '~', '2', 'T', 'X', 'T'};
        u8 chk = lfn_checksum(name11);
        std::u16string nm(longname);
        const size_t half = nm.size() > 13 ? 13 : nm.size();
        /* slot 2 = tail (13 chars max), slot 1 = head */
        std::u16string tail = nm.size() > 13 ? nm.substr(13) : std::u16string();
        std::u16string head = nm.substr(0, half);
        while (head.size() < 13)
            head.push_back(char16_t(0));
        make_lfn_slot(root + 0 * 32, u8(0x42), chk, tail.c_str() + 0); /* seq2|last */
        make_lfn_slot(root + 1 * 32, u8(0x01), chk, head.c_str() + 0); /* seq1 */
        make_dentry(root + 6 * 32, "README~2TXT", 0x20, 40, 100, 0x5A46, 0x6000, false);
        /* link cluster 40 EOC for the planted file */
        set_fat16_entry(img, 2 * 512, 40, 0xFFFF);
        set_fat16_entry(img, 22 * 512, 40, 0xFFFF);
        MemDevice dev(std::move(img));
        tree::ResultTree tree;
        auto* ra = tree.add_root(u"Found");
        FatScanFlags flags;
        FatScanStats stats;
        ASSERT_EQ(
            scan_fat(&dev, flags, nullptr, nullptr, nullptr, tree, ra, nullptr, nullptr, stats),
            OK);
        auto all = collect_all(tree);
        bool saw_alias = false, saw_illegal = false;
        for (auto* n : all) {
            if (n->name_utf8() == "README~2.TXT")
                saw_alias = true;
            for (char c : n->name_utf8())
                if (c == char(illegal[k]))
                    saw_illegal = true;
        }
        EXPECT_TRUE(saw_alias) << "the 8.3 alias must serve (illegal char index " << k << ")";
        EXPECT_FALSE(saw_illegal) << "the corrupt LFN must be discarded (index " << k << ")";
    }
}

TEST(FatEngine, LfnValidNameUnchanged) {
    /* N-176 regression pin: valid LFNs (all legal characters) still
     * ride the LFN path - the fixture's own "readme_document_v1.txt". */
    auto img = build_fat16_image();
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    FatScanFlags flags;
    FatScanStats stats;
    ASSERT_EQ(scan_fat(&dev, flags, nullptr, nullptr, nullptr, tree, ra, nullptr, nullptr, stats),
              OK);
    auto all = collect_all(tree);
    bool saw_lfn = false;
    for (auto* n : all)
        if (n->name_utf8() == "readme_document_v1.txt")
            saw_lfn = true;
    EXPECT_TRUE(saw_lfn) << "valid LFNs are unaffected by the illegal-char gate";
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

TEST(FatEngine, ShortNameOemBytesDecoded) {
    /* N-087: an 8.3 name with high-half OEM bytes decodes through CP437
     * at the boundary - byte 0x93 is 'o'-circumflex (U+00F4), not the
     * identity-mapped U+0093 control the pre-fix code produced. The
     * og's own behavior: the raw bytes persist and the mounting
     * codepage supplies the glyphs (ifileproperties-part1:121); the
     * core's UTF-16 carrier needs the conversion. */
    auto img = build_fat16_image();
    /* a new dentry: bytes {0x93, 'A', 'B'} + ext "TXT", chain 15 (EOO) */
    u8* root = img.data() + 42 * 512;
    u8 oem[11] = {0x93, 'A', 'B', ' ', ' ', ' ', ' ', ' ', 'T', 'X', 'T'};
    make_dentry(root + 6 * 32, reinterpret_cast<const char*>(oem), 0x20, 15, 100, 0x5A46, 0x6000,
                false);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    FatScanFlags flags;
    FatScanStats stats;
    ASSERT_EQ(scan_fat(&dev, flags, nullptr, nullptr, nullptr, tree, ra, nullptr, nullptr, stats),
              OK);
    auto all = collect_all(tree);
    /* CP437 0x93 = U+00F4 (o-circumflex) + "AB.TXT" */
    bool found = false;
    for (auto* n : all) {
        if (n->props() == nullptr)
            continue;
        const std::u16string& nm = n->props()->name;
        if (nm.size() == 7 && nm[0] == 0xF4 && nm[1] == u'A' && nm[2] == u'B' && nm[3] == u'.' &&
            nm[4] == u'T' && nm[5] == u'X' && nm[6] == u'T') {
            found = true;
            /* and NOT the identity-mapped control U+0093 */
            EXPECT_NE(u16(0x93), nm[0]);
        }
        for (u16 c : nm)
            ASSERT_NE(u16(0x93), c) << "the pre-fix identity map must be gone";
    }
    EXPECT_TRUE(found) << "the OEM name must decode through CP437";
}

TEST(FatEngine, ShortNameAsciiUnchanged) {
    /* N-087: ASCII 8.3 names are byte-identical before/after (the
     * conversion touches only the high half) */
    auto img = build_fat16_image();
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    FatScanFlags flags;
    FatScanStats stats;
    ASSERT_EQ(scan_fat(&dev, flags, nullptr, nullptr, nullptr, tree, ra, nullptr, nullptr, stats),
              OK);
    auto all = collect_all(tree);
    bool saw_del = false, saw_inner = false;
    for (auto* n : all) {
        if (n->name_utf8() == "#EL.TXT") /* the og placeholder convention (N-172) */
            saw_del = true;
        if (n->name_utf8() == "INNER.TXT")
            saw_inner = true;
    }
    EXPECT_TRUE(saw_del);
    EXPECT_TRUE(saw_inner);
}

TEST(FatEngine, ShortNameKaniMarkerPathUnchanged) {
    /* N-087: the 0x05-first-byte KANJI escape (cr_fat_quick.cpp:202-203
     * rewrites 0x05 -> 0xE5 before the name build) and the deleted
     * 0xE5 first byte both still reach the marker logic - the decode
     * happens after, so 0xE5-as-name (CP437 alpha) decodes to U+00D1
     * when the marker was consumed. A live 0x05-led dentry name */
    auto img = build_fat16_image();
    u8* root = img.data() + 42 * 512;
    /* 0x05 lead byte (the KANJI lead-in for a real 0xE5 first char) */
    u8 kani[11] = {0x05, 'X', ' ', ' ', ' ', ' ', ' ', ' ', 'T', 'X', 'T'};
    make_dentry(root + 6 * 32, reinterpret_cast<const char*>(kani), 0x20, 15, 100, 0x5A46, 0x6000,
                false);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    FatScanFlags flags;
    FatScanStats stats;
    ASSERT_EQ(scan_fat(&dev, flags, nullptr, nullptr, nullptr, tree, ra, nullptr, nullptr, stats),
              OK);
    auto all = collect_all(tree);
    bool saw_kani = false;
    for (auto* n : all) {
        if (n->props() == nullptr || n->props()->name.empty())
            continue;
        if (n->props()->name[0] == 0x05) {
            ADD_FAILURE() << "the KANJI escape must be consumed (rewritten to 0xE5)";
        }
        /* the caller rewrites 0x05 -> 0xE5; CP437 0xE5 = U+03C3 (sigma)
         * - the decoded name is sigma + "X.TXT" (the Latin-1 identity
         * map would have produced U+00E5 - the pre-fix bug) */
        const std::u16string& nm = n->props()->name;
        if (nm.size() == 6 && nm[0] == 0x3C3 && nm[1] == u'X' && nm[2] == u'.' && nm[3] == u'T' &&
            nm[4] == u'X' && nm[5] == u'T')
            saw_kani = true;
    }
    EXPECT_TRUE(saw_kani) << "0x05 -> 0xE5 -> CP437 U+00D1, the full marker path";
}

TEST(FatEngine, NtResCaseFoldFourWay) {
    /* N-175 (diff-to-fix): Windows stores the display case of 8.3 names
     * in the NT reserved byte; the og's name builder applies it
     * (FATSearchDLL :44191-44216, the four-way exact-value switch).
     * A mixed-case on-disk name "File001 Txt" separates all five paths:
     *   0x00 -> FILE001.TXT   0x08 -> file001.TXT
     *   0x10 -> FILE001.txt   0x18 -> file001.txt
     *   0x01 -> File001.Txt   (raw - the og's switch is exact, not a mask)
     */
    struct Case {
        u8 ntres;
        const char* expect;
    };
    const Case cases[] = {
        {0x00, "FILE001.TXT"}, {0x08, "file001.TXT"}, {0x10, "FILE001.txt"},
        {0x18, "file001.txt"}, {0x01, "File001.Txt"},
    };
    for (const auto& c : cases) {
        auto img = build_fat16_image();
        u8* root = img.data() + 42 * 512;
        /* slot 6, own EOC cluster 40 (unlinked-elsewhere, linked here) */
        set_fat16_entry(img, 2 * 512, 40, 0xFFFF);
        set_fat16_entry(img, 22 * 512, 40, 0xFFFF);
        /* mixed-case on-disk bytes separate all five render paths */
        make_dentry(root + 6 * 32, "File001 Txt", 0x20, 40, 100, 0x5A46, 0x6000, false);
        root[6 * 32 + 12] = c.ntres; /* the NTRes byte */
        MemDevice dev(std::move(img));
        tree::ResultTree tree;
        auto* ra = tree.add_root(u"Found");
        FatScanFlags flags;
        FatScanStats stats;
        ASSERT_EQ(
            scan_fat(&dev, flags, nullptr, nullptr, nullptr, tree, ra, nullptr, nullptr, stats),
            OK);
        auto all = collect_all(tree);
        bool found = false;
        for (auto* n : all) {
            if (n->props() == nullptr || n->props()->name.empty())
                continue;
            if (n->name_utf8().rfind("File001", 0) == 0 ||
                n->name_utf8().rfind("FILE001", 0) == 0 ||
                n->name_utf8().rfind("file001", 0) == 0) {
                EXPECT_EQ(n->name_utf8(), c.expect) << "NTRes=0x" << std::hex << int(c.ntres);
                found = true;
            }
        }
        EXPECT_TRUE(found) << "the planted dentry must surface (NTRes=0x" << std::hex
                           << int(c.ntres) << ")";
    }
}

TEST(FatEngine, NtResFoldLeavesLfnCaseUntouched) {
    /* N-175 regression pin: the fold applies ONLY to the 8.3 alias path -
     * LFN names carry their true case and must never be re-folded. The
     * fixture's LFN "readme_document_v1.txt" must surface exactly as
     * stored, not uppercased by the NTRes=0 dentry beside it. */
    auto img = build_fat16_image();
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    FatScanFlags flags;
    FatScanStats stats;
    ASSERT_EQ(scan_fat(&dev, flags, nullptr, nullptr, nullptr, tree, ra, nullptr, nullptr, stats),
              OK);
    auto all = collect_all(tree);
    bool saw_lfn = false;
    for (auto* n : all) {
        if (n->name_utf8() == "readme_document_v1.txt")
            saw_lfn = true;
    }
    EXPECT_TRUE(saw_lfn) << "the LFN keeps its true case";
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
        if (n->name_utf8() == "#OLD.TXT")
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

TEST(FatEngine, FatOriginClusterCarriedForChainlessDeleted) {
    /* OQ-SA-1 (resolved, scan-engine lane): the og's CFATFileProperties
     * +0x3e first-cluster anchor is stamped from the DENTRY pair
     * (+0x14/+0x1A) independent of chain resolution — for deleted entries
     * the reconstruction helper FUN_180012e60 ALWAYS closes with
     * props->SetFirstCluster(start & 0xffffffff), where start = the
     * dentry's OWN first-cluster when the reconstruction produced nothing
     * (FATSearchDLL_decompiled.c:13702-13738). A chainless deleted file
     * (the FAT entry at its origin cluster is FREE) therefore KEEPS the
     * origin datum in the og's record; pre-fix the core reconstructed
     * nothing and dropped it (RED: fat_origin_cluster read 0). */
    auto img = build_fat12_image();
    /* a second deleted dentry: _GONE.TXT, first cluster 9, FAT[9] FREE
     * (the zero-initialized FAT never links it - chainless by
     * construction); its content marker sits in the data area as the
     * only witness the dentry points at */
    u8* root = img.data() + 6 * 512;
    make_dentry(root + 2 * 32, "GGONE   TXT", 0x20, 9, 512, 0x5A46, 0x6000, true);
    std::memcpy(img.data() + 38 * 512 + (9 - 2) * 512, "FAT12GONE", 9);

    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    FatScanFlags flags; /* quick semantics: no deleted-chain reconstruction */
    FatScanStats stats;
    u32 rc = scan_fat(&dev, flags, nullptr, nullptr, nullptr, tree, ra, rd, nullptr, stats);
    ASSERT_EQ(rc, OK) << err_text(rc);

    auto all = collect_all(tree);
    tree::FileNode* gone = nullptr;
    tree::FileNode* twelve = nullptr;
    for (auto* n : all) {
        if (n->name_utf8() == "#GONE.TXT")
            gone = n;
        if (n->name_utf8() == "TWELVE.TXT")
            twelve = n;
    }
    ASSERT_NE(gone, nullptr) << "chainless deleted dentry not emitted";
    ASSERT_NE(twelve, nullptr) << "live FAT12 file missing";

    /* the chainless deleted file: FAT[9] is FREE, so the FAT verified
     * NOTHING - the default-flags pass reconstructs a single-cluster
     * contiguous candidate (GAP-S6-2) capped at PartialRisk, never
     * Excellent. Whatever the chain machinery produced, the dentry's
     * origin cluster survives as the passive anchor datum */
    EXPECT_EQ(gone->props()->extents.size(), 1u);                   /* the inferred [9,10) run */
    EXPECT_EQ(gone->props()->extents[0].offset, u64(38 + 7) * 512); /* cluster 9 */
    EXPECT_EQ(gone->confidence(), tree::Confidence::PartialRisk);   /* honesty cap */
    EXPECT_EQ(gone->props()->fat_origin_cluster, 9u);
    EXPECT_EQ(gone->deleted_state(), tree::DeletedState::Deleted);

    /* the live file carries the anchor too (the og stamps every FAT
     * record's +0x3e; the census observed it on all-chain-resolved
     * records - TWELVE.TXT is exactly that shape) */
    EXPECT_EQ(twelve->props()->fat_origin_cluster, 2u);
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
        if (n->name_utf8() == "#VERYL~1.TXT")
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
        /* N-018/UBSan: byte-wise stores - name1 sits at an odd offset */
        u8 slotbuf[32] = {};
        slotbuf[0] = 0x41; /* sequence */
        slotbuf[11] = ATTR_LFN;
        slotbuf[12] = 0;
        slotbuf[13] = chk;
        put_u16(slotbuf + 26, 0); /* first_cluster */
        for (int i = 0; i < 5; ++i)
            put_u16(slotbuf + 1 + 2 * i, 0xFFFF);
        LfnEntry slot = {};
        std::memcpy(&slot, slotbuf, sizeof(slot)); /* plain C++ object; the
            engine reads it through le16/le8 - no alignment UB on OUR side */
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
        if (n->name_utf8() == "#NLINK.TXT")
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
        if (n->name_utf8() == "#RUNC.TXT")
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
        if (n->name_utf8() == "#EL.TXT")
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
        d[0x0B] = 0x20; /* archive */
        d[0x1A] = 100;  /* first cluster low */
        d[0x1C] = 12;   /* size */
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

TEST(FatEngine, CrossLinkedFat1RejectedAndFat2Used) {
    /* M-015: a FAT1 containing a cross-link (two chains claiming one
     * cluster) is wholesale-rejected by the og's claims walk
     * (FUN_180041b00 :54084-141) and the factory retries the next copy -
     * chains must come from the clean FAT2. */
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
    const size_t f1 = size_t(p.fat_start);
    /* FAT1 cross-link: 40->41, 41->EOC AND 45->41 (41 claimed by two
     * roots) - FAT2 stays the clean fixture copy */
    set_fat16_entry(img, f1, 40, 41);
    set_fat16_entry(img, f1, 41, 0xFFFF);
    set_fat16_entry(img, f1, 45, 41);
    MemDevice dev(std::move(img));
    FatTable t;
    ASSERT_EQ(t.load(&dev, p), OK) << "the clean mirror keeps the volume scannable";
    EXPECT_TRUE(t.fat_copy_rejected()) << "FAT1 was wholesale-rejected";
    /* chains resolve through the promoted FAT2 (the fixture's links) */
    EXPECT_EQ(t.next(10), 11u);
    EXPECT_EQ(t.next(40), 0u) << "the corrupt FAT1 links are gone";
}

TEST(FatEngine, MediaByteCanonicalEnforcedFat16) {
    /* M-015: FAT16 requires the exact media pair u32@FAT[0] ==
     * 0xFFFFFFF8 (FUN_180041b00 :54052 `*piVar2 == -8`). A zeroed pair
     * rejects the copy; the clean mirror carries the volume. */
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
    const size_t f1 = size_t(p.fat_start);
    set_fat16_entry(img, f1, 0, 0); /* zero the media word in FAT1 */
    MemDevice dev(std::move(img));
    FatTable t;
    ASSERT_EQ(t.load(&dev, p), OK) << "mirror still valid";
    EXPECT_TRUE(t.fat_copy_rejected());
    /* and when BOTH copies carry the zeroed pair, the table is refused */
    auto img2 = build_fat16_image();
    const size_t f2 = size_t(p.fat2_start);
    set_fat16_entry(img2, f1, 0, 0);
    set_fat16_entry(img2, f2, 0, 0);
    MemDevice dev2(std::move(img2));
    FatTable t2;
    EXPECT_EQ(t2.load(&dev2, p), ERR_BAD_MAGIC) << "no valid copy: the og declines";
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

/* ---- A-014: out-of-range FAT time words yield the ZERO FileTime --------- */

namespace {

/* Plant three live dentries whose DATE is valid (0x5A46 = 2025-02-06) but
 * whose TIME words carry out-of-range components: hour 24 (0xC000), minute
 * 60 (0x6780), 2-second field 30 (= 60 s, 0x601E). DosDateTimeToFileTime
 * rejects all three component classes (FATSearchDLL :32135-54 zero-on-fail);
 * the pre-fix core folded them into shifted plausible timestamps. */
std::vector<u8> build_fat16_image_with_bad_times() {
    auto img = build_fat16_image();
    u8* root = img.data() + 42 * 512;
    make_dentry(root + 6 * 32, "HOUR24  TXT", 0x20, 15, 100, 0x5A46, 0xC000, false);
    make_dentry(root + 7 * 32, "MIN60   TXT", 0x20, 15, 100, 0x5A46, 0x6780, false);
    make_dentry(root + 8 * 32, "SEC60   TXT", 0x20, 15, 100, 0x5A46, 0x601E, false);
    return img;
}

} // namespace

TEST(FatEngine, TimeWordHour24YieldsZeroFileTime) {
    /* A-014: hour bits 24 flow unvalidated into the epoch math and add a
     * full day - the fabricated "date + 1 day 00:00" timestamp. The og's
     * DosDateTimeToFileTime fails, the node slot is zeroed. */
    EXPECT_EQ(fat_date_to_unix(0x5A46, 0xC000, 0), 0u);
    EXPECT_EQ(fat_date_to_unix(0x5A46, 0xF800, 0), 0u) << "hour 31: same class";
}

TEST(FatEngine, TimeWordMinute60YieldsZeroFileTime) {
    /* A-014: minute bits 60 (the 6-bit field can hold 60..63). */
    EXPECT_EQ(fat_date_to_unix(0x5A46, 0x6780, 0), 0u);
}

TEST(FatEngine, TimeWordSecondField30YieldsZeroFileTime) {
    /* A-014: the 2-second field 30 decodes 60 s (the 5-bit field can hold
     * 30/31 = 60/62 s); the og's accept set tops out at field 29 = 58 s. */
    EXPECT_EQ(fat_date_to_unix(0x5A46, 0x601E, 0), 0u);
    EXPECT_EQ(fat_date_to_unix(0x5A46, 0x601F, 0), 0u) << "field 31 = 62 s: same class";
}

TEST(FatEngine, ValidTimeWordBoundaryUnchanged) {
    /* A-014 regression guard: the boundary-legal maximum time word
     * 23:59:58 (0xBF7D) still converts - the accept set shrinks by nothing.
     * 2025-02-06 23:59:58 = unix 1738886398. */
    EXPECT_EQ(fat_date_to_unix(0x5A46, 0xBF7D, 0), 1738886398ULL);
    EXPECT_EQ(fat_date_to_unix(0x5A46, 0x6000, 0), 1738843200ULL) << "12:00:00 unchanged";
}

TEST(FatEngine, InvalidTimeStampsPersistAsZeroInTree) {
    /* A-014 end-to-end: corrupt-dentry time words (the deleted population
     * recovery targets) surface as the honest zero FileTime in the results
     * tree, never as a shifted plausible date. */
    auto img = build_fat16_image_with_bad_times();
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost");
    FatScanFlags flags;
    FatScanStats stats;
    ASSERT_EQ(scan_fat(&dev, flags, nullptr, nullptr, nullptr, tree, ra, rd, rl, stats), OK);
    for (const char* name : {"HOUR24.TXT", "MIN60.TXT", "SEC60.TXT"}) {
        tree::FileNode* n = find_node(tree, name);
        ASSERT_NE(n, nullptr) << name << " must be scanned";
        EXPECT_EQ(n->props()->times.created, 0ULL) << name << " created";
        EXPECT_EQ(n->props()->times.modified, 0ULL) << name << " modified";
    }
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

/* ---- M-045 (diff-to-fix): the three validate_boot gates the og's        */
/* classifier enforces and the core's engine gate lacked. (b) SPC and       */
/* (c) the FAT32 label already landed via N-010b/N-034 (boot_passes_open_   */
/* gates + lost_boot_flavor); these are the (a)/(d)/(e) residuals.          */

TEST(FatEngine, FatBps1536Accepted) {
    /* M-045(a): the og's classifier accepts ANY 512-multiple bps
     * (FUN_180004610 :2542, (bps & 0x1ff) == 0) - 1536 is a rare-but-real
     * geometry. Fail-before: the pow2-in-[512,4096] gate rejected it,
     * making a lost_boot_flavor-accepted candidate invisible to
     * probe_fat (validate_boot runs as its FIRST gate). */
    auto img = build_fat16_image();
    put16(img, 0x0B, 1536);
    u8 boot[512];
    std::memcpy(boot, img.data(), 512);
    FatParams p;
    const u32 rc = validate_boot(boot, u64(5074) * 1536, p);
    ASSERT_EQ(rc, OK) << "any 512-multiple bps must classify (og parity)";
    EXPECT_EQ(p.bytes_per_sector, 1536u);
    /* the arithmetic is bps-generic: root sectors ceil(512*32/1536)=11 */
    EXPECT_EQ(p.first_data_sector, u64(2) + 2 * 20 + 11);
}

TEST(FatEngine, FatBps32768CeilingAndNonMultipleRejected) {
    /* M-045(a): the documented deliberate bound - 32768 is the last
     * accepted size; 33280 (a 512-multiple above the ceiling) and 1000
     * (not a multiple) are rejected. */
    auto img = build_fat16_image();
    u8 boot[512];
    FatParams p;
    put16(img, 0x0B, 32768);
    std::memcpy(boot, img.data(), 512);
    EXPECT_EQ(validate_boot(boot, u64(5074) * 32768, p), OK)
        << "32768 is the documented ceiling (accepted)";
    put16(img, 0x0B, 33280);
    std::memcpy(boot, img.data(), 512);
    EXPECT_NE(validate_boot(boot, u64(5074) * 33280, p), OK)
        << "above the ceiling: rejected (documented deliberate bound)";
    put16(img, 0x0B, 1000);
    std::memcpy(boot, img.data(), 512);
    EXPECT_NE(validate_boot(boot, u64(5074) * 1000, p), OK)
        << "not a 512-multiple: rejected (og :2542 gate)";
}

TEST(FatEngine, FatZeroNumFatsRejected) {
    /* M-045(d): numFATs == 0 is a hard reject - the og's every gate
     * requires it nonzero (FUN_1800048f0/4a40/4b70 first checks); the
     * old code let fat_area = 0 through (latent nonsense geometry).
     * Fail-before: validate_boot returned OK. */
    auto img = build_fat16_image();
    img[0x10] = 0;
    u8 boot[512];
    std::memcpy(boot, img.data(), 512);
    FatParams p;
    EXPECT_NE(validate_boot(boot, u64(5074) * 512, p), OK)
        << "numFATs == 0 must be rejected before any geometry is derived";
}

TEST(FatEngine, FatClusterCountOverflowRejected) {
    /* M-045(e): cluster counts >= 0xFFFFFF0 are rejected outright - the
     * og classifier's final band else-branch (FUN_180004610 :2595-2598);
     * the spec's reserved max is 0x0FFFFFF6 so only overflow noise is
     * cut. Fail-before: classified FAT32 and passed. */
    auto img = build_fat16_image();
    put16(img, 0x11, 0);                 /* FAT32 shape: no root dir */
    put16(img, 0x16, 0);                 /* spf16 = 0 */
    put32v(img, 0x24, 64);               /* spf32 */
    put16(img, 0x13, 0);                 /* use total32 */
    put32v(img, 0x20, 130 + 0xFFFFFF0u); /* first_data 130 + cap clusters */
    std::memcpy(img.data() + 0x52, "FAT32   ", 8);
    u8 boot[512];
    std::memcpy(boot, img.data(), 512);
    FatParams p;
    EXPECT_NE(validate_boot(boot, u64(0x100) * 512, p), OK)
        << "cluster_count >= 0xFFFFFF0 must be rejected (og final-band gate)";
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
/* ---- M-052 (diff-to-fix): the FAT recycle-bin $I/$R pairing ---------- */

namespace {

/* a filled $RECYCLE.BIN on the FAT16 fixture: <bin-root>\S-1-5-21\ holds a
 * $I7ABC12 metadata record (v2, 544 B, cluster 50) + its $R7ABC12.JPG
 * content (2048 B, cluster 51). bin_root_8_3 selects the root name so the
 * legacy roots (Recycler/Recycled) reuse the same body. */
std::vector<u8> build_fat16_recycle_image(const char* bin_root_8_3) {
    auto img = build_fat16_image();
    const size_t FAT1 = 2 * 512;
    const size_t FAT2 = 22 * 512;
    const size_t ROOT = 42 * 512;
    const size_t DATA = 74 * 512;
    auto link = [&](u32 c, u16 v) {
        set_fat16_entry(img, FAT1, c, v);
        set_fat16_entry(img, FAT2, c, v);
    };
    link(40, 0xFFFF); /* bin root dir */
    link(41, 0xFFFF); /* SID subfolder */
    link(50, 0xFFFF); /* $I record */
    link(51, 0xFFFF); /* $R content */

    /* root: the bin directory (fixture used slots 0..5) */
    u8* root = img.data() + ROOT;
    make_dentry(root + 6 * 32, bin_root_8_3, ATTR_DIRECTORY, 40, 0, 0x5A46, 0x6000, false);

    /* bin root content (cluster 40): . .. SID */
    u8* bin = img.data() + DATA + (40 - 2) * 512;
    make_dentry(bin + 0 * 32, ".          ", ATTR_DIRECTORY, 40, 0, 0x5A46, 0x6000, false);
    make_dentry(bin + 1 * 32, "..         ", ATTR_DIRECTORY, 0, 0, 0x5A46, 0x6000, false);
    make_dentry(bin + 2 * 32, "S-1-5-21   ", ATTR_DIRECTORY, 41, 0, 0x5A46, 0x6000, false);

    /* SID content (cluster 41): . .. $I $R */
    u8* sid = img.data() + DATA + (41 - 2) * 512;
    make_dentry(sid + 0 * 32, ".          ", ATTR_DIRECTORY, 41, 0, 0x5A46, 0x6000, false);
    make_dentry(sid + 1 * 32, "..         ", ATTR_DIRECTORY, 40, 0, 0x5A46, 0x6000, false);
    make_dentry(sid + 2 * 32, "$I7ABC12   ", 0x20, 50, 544, 0x5A46, 0x6000, false);
    make_dentry(sid + 3 * 32, "$R7ABC12JPG", 0x20, 51, 2048, 0x5A46, 0x6000, false);

    /* $I record (cluster 50): v2 fixed form - version, size, FILETIME,
     * pathlen, original path "C:\vacation.jpg" (15 chars) */
    u8* irec = img.data() + DATA + (50 - 2) * 512;
    put32v(img, DATA + (50 - 2) * 512, 2); /* version 2 */
    put32v(img, DATA + (50 - 2) * 512 + 4, 0);
    put32v(img, DATA + (50 - 2) * 512 + 8, 4096); /* original size */
    put32v(img, DATA + (50 - 2) * 512 + 12, 0);
    put32v(img, DATA + (50 - 2) * 512 + 16, 0);  /* FILETIME lo */
    put32v(img, DATA + (50 - 2) * 512 + 20, 30); /* FILETIME hi -> 30<<32 */
    put32v(img, DATA + (50 - 2) * 512 + 24, 15); /* pathlen */
    const char16_t* path = u"C:\\vacation.jpg";
    for (int i = 0; i < 15; ++i)
        put_u16(irec + 28 + i * 2, path[i]);

    /* $R content (cluster 51): a recognizable marker */
    std::memcpy(img.data() + DATA + (51 - 2) * 512, "RECYCLED-CONTENT", 16);
    return img;
}

struct FatScanOut {
    tree::ResultTree tree;
    std::vector<tree::FileNode*> all;
};
void fat_scan_collect(std::vector<u8> img, FatScanStats* stats, FatScanOut& out) {
    MemDevice dev(std::move(img));
    auto* ra = out.tree.add_root(u"Found");
    auto* rd = out.tree.add_root(u"Deleted");
    auto* rl = out.tree.add_root(u"Lost");
    FatScanFlags flags;
    if (scan_fat(&dev, flags, nullptr, nullptr, nullptr, out.tree, ra, rd, rl, *stats) != OK)
        return;
    std::function<void(tree::FileNode*)> collect = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            out.all.push_back(c);
            collect(c);
        }
    };
    for (auto* r : out.tree.roots())
        collect(r);
}

} // namespace

TEST(FatEngine, RecyclePairingRestoresOriginalName) {
    /* M-052 falsifiable claim: a FAT32/FAT16 volume holding
     * $RECYCLE.BIN\S-1-5-21-...\$R7ABC12.jpg + $I7ABC12 - the og emits a
     * recycle-surfaced result carrying the original name/path/deletion
     * date from the $I record (IRecycleBinNode2UI,
     * fat-exfat-engine-internals.md :37-40). Fail-before: the pre-fix
     * core emitted two raw-named files with no pairing. */
    FatScanStats stats;
    FatScanOut out;
    fat_scan_collect(build_fat16_recycle_image("$RECYCLEBIN"), &stats, out);
    EXPECT_EQ(stats.recycle_info, 1u) << "the $I record must parse";
    EXPECT_EQ(stats.recycle_remapped, 1u) << "the $R must remap";
    tree::FileNode* vac = nullptr;
    int raw_r = 0, raw_i = 0;
    for (auto* n : out.all) {
        const std::string nm = n->name_utf8();
        if (nm == "vacation.jpg")
            vac = n;
        if (nm == "$R7ABC12.JPG")
            ++raw_r;
        if (nm == "$I7ABC12")
            ++raw_i;
    }
    ASSERT_NE(vac, nullptr) << "the $R must surface as its original name";
    EXPECT_EQ(vac->props()->logical_size, 4096u) << "size from the $I record";
    EXPECT_EQ(vac->props()->times.modified, (u64(30) << 32)) << "deletion FILETIME";
    EXPECT_EQ(vac->deleted_state(), tree::DeletedState::Deleted);
    ASSERT_EQ(vac->props()->extents.size(), 1u);
    EXPECT_EQ(vac->props()->extents[0].offset, u64(74 + 49) * 512) << "$R content cluster";
    EXPECT_EQ(raw_i, 0) << "bin metadata never surfaces";
    EXPECT_EQ(raw_r, 0) << "the raw $R name is remapped away";
}

TEST(FatEngine, RecycleLegacyRoots) {
    /* M-052: the og matches all bin roots byte-exact (_stricmp,
     * FATSearchDLL :1668-1677): Recycler (NT4-2000) and Recycled (Win9x)
     * get the same pairing as $RECYCLE.BIN. */
    for (const char* root : {"RECYCLER   ", "RECYCLED   "}) {
        FatScanStats stats;
        FatScanOut out;
        fat_scan_collect(build_fat16_recycle_image(root), &stats, out);
        EXPECT_EQ(stats.recycle_info, 1u) << root << ": $I must parse";
        EXPECT_EQ(stats.recycle_remapped, 1u) << root << ": $R must remap";
        tree::FileNode* vac = nullptr;
        for (auto* n : out.all)
            if (n->name_utf8() == "vacation.jpg")
                vac = n;
        ASSERT_NE(vac, nullptr) << root << ": the legacy root must pair";
    }
}

TEST(FatEngine, NonRecycleDollarFilesStayRaw) {
    /* M-052 pin: out-of-bin $I/$R-named files are ordinary content (the
     * og scopes the bin handling to the recognized roots). The fixture
     * plants the SAME names at the volume root - outside any bin. */
    auto img = build_fat16_recycle_image("$RECYCLEBIN");
    /* rename the bin-root dentry in place to a non-bin name (all other
     * fields - attributes, cluster, dates - stay untouched) */
    const size_t ROOT = 42 * 512;
    const char plain[11] = {'P', 'L', 'A', 'I', 'N', 'D', 'I', 'R', ' ', ' ', ' '};
    std::memcpy(img.data() + ROOT + 6 * 32, plain, 11);
    FatScanStats stats;
    FatScanOut out;
    fat_scan_collect(std::move(img), &stats, out);
    EXPECT_EQ(stats.recycle_info, 0u) << "no bin root: the $I must not be treated as metadata";
    EXPECT_EQ(stats.recycle_remapped, 0u);
    int raw_r = 0, raw_i = 0, vac = 0;
    for (auto* n : out.all) {
        const std::string nm = n->name_utf8();
        if (nm == "$R7ABC12.JPG")
            ++raw_r;
        if (nm == "$I7ABC12")
            ++raw_i;
        if (nm == "vacation.jpg")
            ++vac;
    }
    EXPECT_EQ(raw_i, 1u) << "out-of-bin $I-named file stays a raw result";
    EXPECT_EQ(raw_r, 1u) << "out-of-bin $R-named file stays a raw result";
    EXPECT_EQ(vac, 0u) << "no remap outside the bin";
}

/* ---- S-051/M-104 (diff-to-fix): the boot-sector volume label ---- */

namespace {

/* a minimal FAT16 boot sector with the given 11-byte label */
std::vector<u8> fat16_boot_image(const char* label) {
    std::vector<u8> img(200 * 512, 0);
    u8* b = img.data();
    b[0] = 0xEB;
    b[1] = 0x3C;
    b[2] = 0x90;
    std::memcpy(b + 3, "MSDOS5.0", 8);
    b[0x0B] = 0x00;
    b[0x0C] = 0x02; /* bps 512 */
    b[0x0D] = 1;    /* spc */
    b[0x0E] = 1;    /* reserved */
    b[0x10] = 1;    /* one FAT */
    b[0x11] = 0x00;
    b[0x12] = 0x02; /* 512 root entries */
    b[0x13] = 200;  /* total16 = 200 sectors */
    b[0x15] = 0xF8;
    b[0x16] = 2; /* spf16 */
    char lbl[11] = {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '};
    for (int i = 0; i < 11 && label[i]; ++i)
        lbl[i] = label[i];
    std::memcpy(b + 0x2B, lbl, 11);
    std::memcpy(b + 0x36, "FAT16   ", 8);
    b[510] = 0x55;
    b[511] = 0xAA;
    return img;
}

} // namespace

TEST(FatEngine, ReadVolumeLabelDecodesBootSector) {
    /* the FAT12/16 label sits at +0x2B (11 bytes, space padded); the trim
     * keeps interior spaces ("MY DISK") and drops the pad. */
    std::u16string label;
    MemDevice dev1(fat16_boot_image("MY DISK"));
    ASSERT_EQ(read_volume_label(&dev1, label), OK);
    std::string utf8;
    text::lossless_u16_to_utf8(label, utf8);
    EXPECT_EQ(utf8, "MY DISK");

    MemDevice dev2(fat16_boot_image("VOLONE"));
    ASSERT_EQ(read_volume_label(&dev2, label), OK);
    text::lossless_u16_to_utf8(label, utf8);
    EXPECT_EQ(utf8, "VOLONE");
}

TEST(FatEngine, ReadVolumeLabelRejectsLabelLessAndNonFat) {
    /* an all-space label is the honest "no label" error - the caller's
     * fallback tier takes over (never a blank display name); a non-FAT
     * slice is rejected by validate_boot before the field is read. */
    std::u16string label;
    MemDevice blank(fat16_boot_image("           "));
    EXPECT_EQ(read_volume_label(&blank, label), ERR_BAD_PARAMETER);
    /* a label of 10 spaces + '1' (all 11 field bytes): the trim keeps
     * every interior space - only TRAILING pad is stripped */
    MemDevice sparse(fat16_boot_image("          1"));
    ASSERT_EQ(read_volume_label(&sparse, label), OK);
    std::string utf8;
    text::lossless_u16_to_utf8(label, utf8);
    EXPECT_EQ(utf8, "          1");

    /* zeroed sector: no FAT structure at all */
    std::vector<u8> zeros(512, 0);
    MemDevice zerosec(std::move(zeros));
    EXPECT_EQ(read_volume_label(&zerosec, label), ERR_BAD_MAGIC);
    /* null device */
    EXPECT_EQ(read_volume_label(nullptr, label), ERR_BAD_PARAMETER);
}

TEST(FatEngine, ClusterOffsetBeyond2TiB) {
    /* N-162: the og computes (cluster-2) x spc as a 32-bit product at four
     * verified sites (FATSearchDLL :11239/:13194 - the (uint) cast and the
     * 32-bit operand promotion, both re-read first-hand; :28172/:39068-39080
     * the geometry twins) - it wraps on data areas beyond 2^32 sectors (a
     * 2 TiB displacement at the spec-max cluster with spc=32). The core is
     * full u64 (cr_chain.cpp's cluster_offset) - the deliberate
     * non-adoption of an og arithmetic defect, registered as
     * PARITY-DELTAS D-119 (the D-56/D-68 numeric-hygiene family's FAT
     * member). This pin holds the u64 path so a future parity session
     * cannot "fix" the core DOWN to the wrapping form. */
    FatParams p;
    p.bytes_per_sector = 512;
    p.sectors_per_cluster = 32;
    p.cluster_bytes = u64(32) * 512; /* the derived field the math reads */
    p.first_data_sector = 100;
    /* the spec-max cluster 0x0FFFFFF4: the 32-bit product would wrap to
     * 0xFFFFFE40 sectors (~2 TiB displacement); the u64 product is exact */
    const u32 cluster = 0x0FFFFFF4u;
    const u64 off = FatTable::cluster_offset(p, cluster);
    const u64 want = (u64(cluster) - 2) * u64(32 * 512) + u64(100 * 512);
    EXPECT_EQ(off, want) << "the u64 product is exact at the spec-max cluster";
    EXPECT_GT(off, u64(1) << 32) << "the address is beyond the 32-bit wrap band";
    /* the wrapped form the og would compute (the down-fix guard) */
    const u64 wrapped = u64((u32(cluster) - 2u) * u32(32 * 512)) + u64(100 * 512);
    EXPECT_NE(off, wrapped) << "the u64 path must NOT equal the og's wrapping form";
}

TEST(FatEngine, DeletedFirstCharPlaceholderIsHash) {
    /* N-172: the og's deleted-entry placeholder is '#' (FATSearchDLL
     * FUN_180036350 :44166-44172, re-read first-hand: the 0xE5 first
     * byte - -0x1b signed - renders as 0x23). The no-oracle branch
     * aligned for bench comparability; the LFN-oracle branch stays the
     * registered superset (PARITY-DELTAS D-120: the checksum bijection
     * restores the ORIGINAL byte where the og shows the placeholder). */
    /* the shape is pinned end-to-end by Fat16EndToEnd's #EL.TXT / #ONE.TXT
     * rows (the re-pinned set); this unit pin holds the convention by
     * name so the N-172 mapping is discoverable from the test list. */
    const char og_placeholder = '#'; /* 0x23, the decompile's uVar8 */
    EXPECT_EQ(int(u8(og_placeholder)), 0x23);
}

TEST(FatEngine, DeletedLfnChecksumRecoversOriginalByte) {
    /* N-172's superset pin: the LFN-checksum bijection restores the
     * destroyed first byte where the og renders '#' - the deterministic
     * brute-force over 256 candidates validated against the stored
     * checksum (GAP-S6-1, bench 40079065019; the stored checksum is
     * computed at WRITE time over the ORIGINAL 11 bytes). The
     * per-candidate loop shape is pinned at LfnDeletedChecksumRecovers
     * (above); this pin holds the PRESENTATION contract: an
     * oracle-validated recovery yields the original byte, never the
     * placeholder. */
    const u8 orig11[11] = {'H', 'E', 'L', 'L', 'O', ' ', ' ', ' ', 'T', 'X', 'T'};
    u8 raw[11];
    std::memcpy(raw, orig11, 11);
    raw[0] = 0xE5; /* the deletion marker destroyed the 'H' */
    const u8 orig = 'H';
    const u8 chk = lfn_checksum(orig11); /* the STORED value: over the original */
    LfnEntry slot = {};
    slot.checksum = chk; /* the surviving LFN slot's oracle */
    u8 first = 0, got_chk = 0;
    std::vector<const LfnEntry*> slots = {&slot};
    ASSERT_TRUE(lfn_deleted_checksum_recover(slots, raw, first, got_chk));
    EXPECT_EQ(first, orig) << "the oracle branch restores the ORIGINAL byte";
    EXPECT_NE(first, u8('#')) << "never the placeholder";
    EXPECT_EQ(got_chk, chk);
}

TEST(FatEngine, Date2107ConvertsFullyNoClamp) {
    /* N-173: the FAT year field reaches 2107 (year 127); the og's
     * DosDateTimeToFileTime (FATSearchDLL :32135/:32144/:32151) converts
     * the full range with no 2106 cutoff. The pre-fix core clamped unix
     * seconds to u32 - dates past 2106-02-07 06:28:15 collapsed to 0
     * (the invalid 1601 FileTime). The clamp is gone; the u64 path
     * converts the FAT-legal maximum. (The finding's example word
     * 0xFFBF decodes 2107-11-31 - day 31 of November, an INVALID date;
     * the true 2107-12-31 word is 0xFF9F - the correction recorded in
     * the Resolution.) */
    /* 2107-12-31 12:00:00 = unix 4354776000 (past u32) */
    EXPECT_EQ(fat_date_to_unix(0xFF9F, 0x6000, 0), 4354776000ULL);
    /* the exact u32 boundary: 2106-02-07 06:28:14 = 0xFFFFFFFE */
    EXPECT_EQ(fat_date_to_unix(0xFC47, 0x3387, 0), 0xFFFFFFFEULL);
    /* one step past the boundary: 2106-02-07 06:28:16 = 0x100000000 -
     * the pre-fix clamp returned 0 here (the regression pin) */
    EXPECT_EQ(fat_date_to_unix(0xFC47, 0x3388, 0), 0x100000000ULL);
    /* the N-021 invalid class is UNCHANGED: 0 stays exactly the
     * zero-date/month-13/day-0 class, never a clamped valid date */
    EXPECT_EQ(fat_date_to_unix(0x0000, 0x6000, 0), 0u);
    EXPECT_EQ(fat_date_to_unix(0x01A1, 0x6000, 0), 0u); /* month 13 */
    EXPECT_EQ(fat_date_to_unix(0x0020, 0x6000, 0), 0u); /* day 0 */
}

TEST(FatEngine, TimeWordOutOfRangeYieldsZeroTimestamp) {
    /* A-014: the og's DosDateTimeToFileTime (FATSearchDLL
     * :32135-32154, three zero-on-fail slots) rejects hour >= 24,
     * minute >= 60 and second >= 60; the rejected slot becomes the
     * zero FileTime. The core's shared converter used to fold those
     * into the epoch math - fabricating shifted plausible timestamps
     * (time 0xC000 = hour 24 = exactly +1 day). Every out-of-range
     * time word on a valid date now yields 0 (the invalid class). */
    /* hour 24 (0xC000): the finding's worked example - was date+1 day.
     * (The finding's example date word 0x65C4 decodes month 14 - an
     * invalid date; the clean 2024-06-12 word 0x58CC is used here so
     * the test isolates the TIME-word class exactly.) */
    EXPECT_EQ(fat_date_to_unix(0x58CC, 0xC000, 0), 0u) << "hour 24";
    /* hour 31 (0xF800): the 5-bit field's maximum */
    EXPECT_EQ(fat_date_to_unix(0x58CC, 0xF800, 0), 0u) << "hour 31";
    /* minute 60 (0x0780): the 6-bit field past the bound */
    EXPECT_EQ(fat_date_to_unix(0x58CC, 0x0780, 0), 0u) << "minute 60";
    /* minute 63 (0x07E0): the 6-bit field's maximum */
    EXPECT_EQ(fat_date_to_unix(0x58CC, 0x07E0, 0), 0u) << "minute 63";
    /* 2s field 30 (0x001E) = 60 s: the >59 second class */
    EXPECT_EQ(fat_date_to_unix(0x58CC, 0x001E, 0), 0u) << "second field 30";
    /* 2s field 31 (0x001F) = 62 s: the field's maximum */
    EXPECT_EQ(fat_date_to_unix(0x58CC, 0x001F, 0), 0u) << "second field 31";
}

TEST(FatEngine, ValidTimeWordBoundaryConvertsUnchanged) {
    /* A-014 regression edge: the boundary-legal values 23:59:58
     * (0xBF7D) and 00:00:00 (0x0000) still convert - the gate accepts
     * exactly the DosDateTimeToFileTime range. */
    /* 2024-06-12 23:59:58 */
    EXPECT_EQ(fat_date_to_unix(0x58CC, 0xBF7D, 0), 1718236798ULL);
    /* midnight on the same date */
    EXPECT_EQ(fat_date_to_unix(0x58CC, 0x0000, 0), 1718150400ULL);
    /* 2s field 29 (0x001D) = 58 s is the last legal second */
    EXPECT_EQ(fat_date_to_unix(0x58CC, 0x001D, 0), 1718150458ULL);
}

TEST(FatEngine, InvalidTimeStampsPersistAsZero) {
    /* A-014 end-to-end: a live dentry whose write time word carries
     * hour 24 gets the ZERO FileTime on the record (the og's
     * zero-on-fail slot), not a day-shifted plausible value. */
    auto img = build_fat16_image();
    u8* root = img.data() + 42 * 512;
    /* plant a dentry whose CREATE time is valid (12:00) but whose WRITE
     * time carries hour 24 - the og zeroes the failing slot only
     * (per-slot zeroing, FATSearchDLL :32144-:32148 vs :32135-:32138) */
    /* slot 6 = the first free slot (0-5 used; a zero slot would end the
     * directory walk before this dentry is reached) */
    DirEntry* bad = reinterpret_cast<DirEntry*>(root + 6 * 32);
    const char nm[11] = {'B', 'A', 'D', 'T', 'I', 'M', 'E', ' ', 'T', 'X', 'T'};
    for (int i = 0; i < 11; ++i)
        bad->name[i] = u8(nm[i]);
    bad->attributes = 0x20;
    bad->nt_reserved = 0;
    bad->creation_tenths = 0;
    bad->creation_time = 0x6000; /* 12:00 - valid */
    bad->creation_date = 0x58CC; /* 2024-06-12 */
    bad->access_date = 0x58CC;
    bad->first_cluster_high = 0;
    bad->first_cluster_low = 15;
    bad->write_time = 0xC000; /* hour 24 - invalid */
    bad->write_date = 0x58CC;
    bad->file_size = 100;
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost");
    FatScanFlags flags;
    FatScanStats stats;
    ASSERT_EQ(scan_fat(&dev, flags, nullptr, nullptr, nullptr, tree, ra, rd, rl, stats), OK);
    tree::FileNode* z = find_node(tree, "BADTIME.TXT");
    ASSERT_NE(z, nullptr) << "the invalid-time dentry is still a result";
    EXPECT_EQ(z->props()->times.modified, 0u)
        << "hour 24 persists as the zero FileTime, not date+1 day";
    /* creation (time 0x6000 = 12:00 valid) stays real: only the
     * failing slot is zeroed - the og zeroes per-slot, not per-file */
    EXPECT_NE(z->props()->times.created, 0u);
}

<<<<<<< HEAD
TEST(FatEngine, InvalidTimeWordAccessSlotStaysReal) {
    /* A-014 convergence pin (the S52 lane's supplementary): the og
     * passes time word 0 for the access slot (FATSearchDLL
     * :32151-32154 - access has no time field in FAT), so a dentry
     * whose write/create time words carry hour 24 keeps a REAL
     * accessed timestamp while created/modified/changed zero out;
     * changed mirrors modified (cr_fat_quick.cpp:362). Neither remote
     * test family asserted the accessed slot. */
    auto img = build_fat16_image();
    u8* root = img.data() + 42 * 512;
    make_dentry(root + 6 * 32, "ACCREAL TXT", 0x20, 15, 100, 0x58CC, 0xC000, false);
=======
/* ---- M-130: the dentry shape-validator family -------------------------- */

namespace {
/* Plant a raw 32-byte dentry at a root slot (fields the make_dentry
 * helper cannot express: raw attribute bytes, crafted metadata quads). */
void plant_raw(std::vector<u8>& img, size_t slot, const DirEntry& e) {
    std::memcpy(img.data() + 42 * 512 + slot * 32, &e, sizeof(e));
}
DirEntry raw_dentry(const char* name8_3, u8 attr, u16 high, u16 low, u32 size) {
    DirEntry e = {};
    for (int i = 0; i < 11; ++i)
        e.name[i] = u8(name8_3[i]);
    e.attributes = attr;
    e.creation_time = 0x6000;
    e.creation_date = 0x58CC;
    e.access_date = 0x58CC;
    e.write_time = 0x6000;
    e.write_date = 0x58CC;
    e.first_cluster_high = high;
    e.first_cluster_low = low;
    e.file_size = size;
    return e;
}
} // namespace

TEST(FatEngine, DentryAttrReservedBitsStopWalk) {
    /* M-130: an attribute byte with reserved bits (>= 0x40) fails the
     * og's validator (FUN_1800365a0 :44295-96 - the u32 at +0x08's top
     * byte IS the attribute byte; the audit's "ext-tail" attribution
     * corrected) and the walk BREAKS (:8792-93): the dentries after the
     * corrupt slot are lost - parity means matching the loss. */
    auto img = build_fat16_image();
    u8* root = img.data() + 42 * 512;
    make_dentry(root + 6 * 32, "GOOD1   TXT", 0x20, 10, 100, 0x58CC, 0x6000, false);
    plant_raw(img, 7, raw_dentry("RESERVEDTXT", 0x40, 0, 10, 100));
    make_dentry(root + 8 * 32, "AFTER   TXT", 0x20, 10, 100, 0x58CC, 0x6000, false);
>>>>>>> 994e0bb0bf9d80c69e7c47ace4e7541ecb4255f1
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost");
    FatScanFlags flags;
    FatScanStats stats;
    ASSERT_EQ(scan_fat(&dev, flags, nullptr, nullptr, nullptr, tree, ra, rd, rl, stats), OK);
<<<<<<< HEAD
    tree::FileNode* n = find_node(tree, "ACCREAL.TXT");
    ASSERT_NE(n, nullptr) << "the invalid-time dentry is still a result";
    EXPECT_EQ(n->props()->times.created, 0u) << "hour 24 -> the zeroed slot";
    EXPECT_EQ(n->props()->times.modified, 0u) << "hour 24 -> the zeroed slot";
    EXPECT_EQ(n->props()->times.changed, 0u) << "changed mirrors modified";
    EXPECT_EQ(n->props()->times.accessed,
              fat_date_to_unix(0x58CC, 0, 0) * 10000000ULL + 116444736000000000ULL)
        << "access (time word 0) keeps the real date";
=======
    EXPECT_NE(find_node(tree, "GOOD1.TXT"), nullptr) << "before the corrupt slot";
    EXPECT_EQ(find_node(tree, "AFTER.TXT"), nullptr) << "the walk stopped at the 0x40 attr";
    EXPECT_EQ(find_node(tree, "RESERVED.TXT"), nullptr);
    EXPECT_GE(stats.shape_rejected, 1u);
}

TEST(FatEngine, DentryErasedPatternQuadStopsWalk) {
    /* M-130: the extension+attribute quad equal to the
     * ntres/tenths/creation-time quad (and not 0x20) is the erased /
     * repeated-pattern class (:44293-94) - including the all-zero
     * dentry. The walk stops. */
    auto img = build_fat16_image();
    u8* root = img.data() + 42 * 512;
    make_dentry(root + 6 * 32, "GOOD1   TXT", 0x20, 10, 100, 0x58CC, 0x6000, false);
    DirEntry pat = raw_dentry("PATTERN TXT", 0x20, 0, 10, 100);
    pat.nt_reserved = 'T';      /* 0x54 */
    pat.creation_tenths = 'X';  /* 0x58 */
    pat.creation_time = 0x2054; /* 'T' | 0x20 << 8 */
    plant_raw(img, 7, pat);
    make_dentry(root + 8 * 32, "AFTER   TXT", 0x20, 10, 100, 0x58CC, 0x6000, false);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost");
    FatScanFlags flags;
    FatScanStats stats;
    ASSERT_EQ(scan_fat(&dev, flags, nullptr, nullptr, nullptr, tree, ra, rd, rl, stats), OK);
    EXPECT_NE(find_node(tree, "GOOD1.TXT"), nullptr);
    EXPECT_EQ(find_node(tree, "PATTERN.TXT"), nullptr) << "the erased-pattern quad rejects";
    EXPECT_EQ(find_node(tree, "AFTER.TXT"), nullptr) << "the walk stopped";
}

TEST(FatEngine, DentryHistogramEleventhRepeatStopsWalk) {
    /* M-130: the byte-histogram gate (:44356-69) - 21-byte metadata
     * window, nonzero bytes; the ELEVENTH occurrence of any value
     * rejects (the pre-increment compare is > 9; the audit's ">9
     * occurrences" boundary corrected to 10-pass/11-reject). */
    auto img = build_fat16_image();
    u8* root = img.data() + 42 * 512;
    make_dentry(root + 6 * 32, "GOOD1   TXT", 0x20, 10, 100, 0x58CC, 0x6000, false);
    DirEntry eleven = raw_dentry("HISTO   TXT", 0x20, 0, 10, 100);
    eleven.nt_reserved = 0x42;
    eleven.creation_tenths = 0x42;
    eleven.creation_time = 0x4242;
    eleven.creation_date = 0x4242;
    eleven.access_date = 0x4242;
    eleven.write_time = 0x4242;
    eleven.write_date = 0x4242; /* the 11th 0x42 */
    plant_raw(img, 7, eleven);
    make_dentry(root + 8 * 32, "AFTER   TXT", 0x20, 10, 100, 0x58CC, 0x6000, false);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost");
    FatScanFlags flags;
    FatScanStats stats;
    ASSERT_EQ(scan_fat(&dev, flags, nullptr, nullptr, nullptr, tree, ra, rd, rl, stats), OK);
    EXPECT_NE(find_node(tree, "GOOD1.TXT"), nullptr);
    EXPECT_EQ(find_node(tree, "HISTO.TXT"), nullptr) << "11 repeats reject";
    EXPECT_EQ(find_node(tree, "AFTER.TXT"), nullptr) << "the walk stopped";
}

TEST(FatEngine, DentryHistogramTenRepeatsStillEmit) {
    /* M-130 boundary: TEN occurrences of the same nonzero byte PASS -
     * the emitted dentry is a real result (the og's pre-increment
     * compare fires on the 11th, not the 10th). */
    auto img = build_fat16_image();
    DirEntry ten = raw_dentry("HIST10  TXT", 0x20, 0, 10, 100);
    ten.nt_reserved = 0x42;
    ten.creation_tenths = 0x42;
    ten.creation_time = 0x4242;
    ten.creation_date = 0x4242;
    ten.access_date = 0x4242;
    ten.write_time = 0x4242; /* exactly ten 0x42 bytes */
    plant_raw(img, 6, ten);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost");
    FatScanFlags flags;
    FatScanStats stats;
    ASSERT_EQ(scan_fat(&dev, flags, nullptr, nullptr, nullptr, tree, ra, rd, rl, stats), OK);
    EXPECT_NE(find_node(tree, "HIST10.TXT"), nullptr) << "10 repeats are legal";
}

TEST(FatEngine, DentryImpureLabelStopsWalk) {
    /* M-130: label purity (:44298-44301) - an entry with the volume bit
     * that is NOT a pure label (attr in {8, 0x28}, hidden clear) is
     * corrupt and the walk stops; a PURE label is skipped as a result
     * and the walk continues (the pin below). */
    auto img = build_fat16_image();
    u8* root = img.data() + 42 * 512;
    make_dentry(root + 6 * 32, "GOOD1   TXT", 0x20, 10, 100, 0x58CC, 0x6000, false);
    plant_raw(img, 7, raw_dentry("IMPLABEL   ", 0x18, 0, 5, 0)); /* dir+volume */
    make_dentry(root + 8 * 32, "AFTER   TXT", 0x20, 10, 100, 0x58CC, 0x6000, false);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost");
    FatScanFlags flags;
    FatScanStats stats;
    ASSERT_EQ(scan_fat(&dev, flags, nullptr, nullptr, nullptr, tree, ra, rd, rl, stats), OK);
    EXPECT_NE(find_node(tree, "GOOD1.TXT"), nullptr);
    EXPECT_EQ(find_node(tree, "AFTER.TXT"), nullptr) << "the impure label stopped the walk";
}

TEST(FatEngine, PureVolumeLabelSkippedWalkContinues) {
    /* M-130 pin: a PURE volume label (attr 0x08) passes the validator
     * and is skipped as a result - the walk continues past it. */
    auto img = build_fat16_image();
    u8* root = img.data() + 42 * 512;
    plant_raw(img, 6, raw_dentry("VOLLABEL   ", 0x08, 0, 0, 0));
    make_dentry(root + 7 * 32, "AFTER   TXT", 0x20, 10, 100, 0x58CC, 0x6000, false);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost");
    FatScanFlags flags;
    FatScanStats stats;
    ASSERT_EQ(scan_fat(&dev, flags, nullptr, nullptr, nullptr, tree, ra, rd, rl, stats), OK);
    EXPECT_EQ(find_node(tree, "VOLLABEL"), nullptr) << "a label is never a result";
    EXPECT_NE(find_node(tree, "AFTER.TXT"), nullptr) << "the walk continued past the label";
}

TEST(FatEngine, SizeChainXorSkippedButWalkContinues) {
    /* M-130: the emission-walk shape gates (:44344-48, the param_2 = 1
     * site) - a zero-size file WITH a chain (and a sized file with NO
     * chain) are inconsistent shapes: NOT emitted, but the walk does
     * NOT stop (this is the skip class, not the break class). */
    auto img = build_fat16_image();
    u8* root = img.data() + 42 * 512;
    make_dentry(root + 6 * 32, "ZEROSIZETXT", 0x20, 10, 0, 0x58CC, 0x6000, false);
    make_dentry(root + 7 * 32, "NOCHAIN TXT", 0x20, 0, 500, 0x58CC, 0x6000, false);
    make_dentry(root + 8 * 32, "AFTER   TXT", 0x20, 10, 100, 0x58CC, 0x6000, false);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost");
    FatScanFlags flags;
    FatScanStats stats;
    ASSERT_EQ(scan_fat(&dev, flags, nullptr, nullptr, nullptr, tree, ra, rd, rl, stats), OK);
    EXPECT_EQ(find_node(tree, "ZEROSIZE.TXT"), nullptr) << "zero size with a chain: skipped";
    EXPECT_EQ(find_node(tree, "NOCHAIN.TXT"), nullptr) << "sized with no chain: skipped";
    EXPECT_NE(find_node(tree, "AFTER.TXT"), nullptr) << "the walk continued (skip, not break)";
    EXPECT_GE(stats.shape_rejected, 2u);
}

TEST(FatEngine, DirSizeAndHighClusterSkipped) {
    /* M-130: the dir shape gate (:44350) - a directory with size != 0
     * AND a nonzero high cluster word is the corrupt-dir shape: not
     * emitted, walk continues. */
    auto img = build_fat16_image();
    u8* root = img.data() + 42 * 512;
    plant_raw(img, 6, raw_dentry("BADDIR     ", ATTR_DIRECTORY, 0x1234, 5, 777));
    make_dentry(root + 7 * 32, "AFTER   TXT", 0x20, 10, 100, 0x58CC, 0x6000, false);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost");
    FatScanFlags flags;
    FatScanStats stats;
    ASSERT_EQ(scan_fat(&dev, flags, nullptr, nullptr, nullptr, tree, ra, rd, rl, stats), OK);
    EXPECT_EQ(find_node(tree, "BADDIR"), nullptr) << "the corrupt dir shape: skipped";
    EXPECT_NE(find_node(tree, "AFTER.TXT"), nullptr) << "the walk continued";
>>>>>>> 994e0bb0bf9d80c69e7c47ace4e7541ecb4255f1
}

TEST(FatEngine, Date2107EndToEndFileTimeIsReal) {
    /* N-173 end-to-end: a live dentry dated 2107-12-31 carries the REAL
     * FileTime (unix 4354776000 * 1e7 + the epoch), not the zero/1601
     * emission the pre-fix clamp produced. */
    auto img = build_fat16_image_with_bad_dates(0xFF9F, 0x01A1, 0x0020);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost");
    FatScanFlags flags;
    FatScanStats stats;
    ASSERT_EQ(scan_fat(&dev, flags, nullptr, nullptr, nullptr, tree, ra, rd, rl, stats), OK);
    tree::FileNode* z = find_node(tree, "ZERODAT.TXT");
    ASSERT_NE(z, nullptr) << "the 2107-dated dentry must be a result";
    EXPECT_EQ(z->props()->times.modified, 4354776000ULL * 10000000ULL + 116444736000000000ULL)
        << "the 2107 date converts fully (the og has no cutoff)";
}
