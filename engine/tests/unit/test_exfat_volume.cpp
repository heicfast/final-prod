/* exFAT engine end-to-end tests (spec-exact fixture).
 *
 * Layout: 512B sectors/clusters, volume 130 sectors:
 *   boot 0..23 (main 0, backup 12) | FAT sector 24 (128 entries) | heap 25..
 *   root = cluster 2 (sector 25): bitmap(0x81@50), upcase(0x82), label(0x83),
 *   photo.jpg (NoFatChain, clusters 10-11), doc.txt (FAT chain 20-22),
 *   gone.dat DELETED (cluster 30, bitmap free -> Excellent),
 *   over.dat DELETED (cluster 40, bitmap SET -> Overwritten),
 *   subdir (0x85 dir, cluster 70) containing inner.txt (NoFatChain @80).
 */
#include <gtest/gtest.h>
#include "cr/exfat/cr_exfat_engine.h"
#include "cr/storage/cr_device.h"
#include <algorithm>
#include <cstring>
#include <functional>
#include <vector>

using namespace cr;
using namespace cr::exfat;

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
void put64(std::vector<u8>& v, size_t at, u64 x) {
    for (int i = 0; i < 8; ++i)
        v[at + i] = u8(x >> (8 * i));
}

/* published algorithm (spec): 16-bit rotate-add */

constexpr u32 EXFAT_TEST_TS = (0x5A46u << 16) | 0x6000u; /* 2025-02-06 12:00:
    every field in range - M-014's timestamp gates reject zero-day/garbage
    stamps, so planted sets carry a realistic stamp like real media */

struct SetBuilder {
    std::vector<u8> entries;
    size_t name_entry_first_ = 0; /* S-013: first name-entry offset (multi-chunk flip) */

    void file_entry(u16 attribute, u32 ts, u8 tz = 0) {
        u8* e = add();
        e[0] = 0x85;
        e[1] = 2;                         /* secondary count: stream + 1 name */
        put16(entries, cur_off() + 2, 0); /* checksum filled later */
        put16(entries, cur_off() + 4, attribute);
        put32v(entries, cur_off() + 8, ts);  /* create */
        put32v(entries, cur_off() + 12, ts); /* modify */
        put32v(entries, cur_off() + 16, ts); /* access */
        e[20] = 0;
        e[21] = 0; /* 10ms */
        /* tz bytes at +0x16/+0x17/+0x18 (create/modify/access). S-011
         * fixtures set them to prove the engine does NOT apply them (og
         * parity: the bytes are parsed but ignored). */
        e[22] = tz;
        e[23] = tz;
        e[24] = tz;
    }
    void file_entry_deleted(u16 attribute, u32 ts) {
        file_entry(attribute, ts);
        entries[cur_off()] = 0x05;
    }
    void stream_entry(u8 flags, u8 name_len, u32 first_cluster, u64 data_len) {
        u8* e = add();
        e[0] = 0xC0;
        e[1] = flags;
        e[3] = name_len;
        put64(entries, cur_off() + 8, data_len); /* valid data len */
        put32v(entries, cur_off() + 20, first_cluster);
        put64(entries, cur_off() + 24, data_len);
    }
    void stream_entry_deleted(u8 flags, u8 name_len, u32 first_cluster, u64 data_len) {
        stream_entry(flags, name_len, first_cluster, data_len);
        entries[cur_off()] = 0x40;
    }
    void name_entry(const char16_t* nm) {
        std::u16string n = nm;
        /* S-013 fixture: names longer than 15 chars span ceil(len/15)
         * name entries (spec 7.6.3 chunked UTF-16), 15 chars each */
        const size_t chunks = (n.size() + 14) / 15;
        name_entry_first_ = entries.size();
        for (size_t c = 0; c < chunks; ++c) {
            u8* e = add();
            e[0] = 0xC1;
            for (size_t i = 0; i < 15; ++i) {
                const size_t idx = c * 15 + i;
                put16(entries, cur_off() + 2 + i * 2, idx < n.size() ? n[idx] : 0);
            }
        }
    }
    void name_entry_deleted(const char16_t* nm) {
        name_entry(nm);
        /* flip EVERY name entry of this name (multi-chunk sets) */
        for (size_t o = name_entry_first_; o < entries.size(); o += 32)
            entries[o] = 0x41;
    }
    /* fill SetCheckSum over the built set (excluding the +2 field) */
    void finish_set(std::vector<u8>& root, size_t set_at) {
        /* normalize the secondary-count byte to the real entry count
         * (S-013: multi-name sets change the count; single-name callers
         * are unaffected: 2 == 3 entries - 1) */
        entries[1] = u8(entries.size() / 32 - 1);
        /* compute over the set bytes, skipping the +2 checksum field */
        u16 sum = 0;
        for (size_t b = 0; b < entries.size() * 32; ++b) {
            if (b >= 2 && b < 4)
                continue;
            sum = static_cast<u16>(((sum & 1) ? 0x8000u : 0u) + (sum >> 1) +
                                   *(root.data() + set_at + b));
        }
        put16(root, set_at + 2, sum);
    }

    size_t set_off = 0; /* offset within entries where the SET starts */

private:
    u8* add() {
        entries.resize(entries.size() + 32, 0);
        return entries.data() + entries.size() - 32;
    }
    size_t cur_off() const { return entries.size() - 32; }
};

/* N-067 helper: the fixture's 64-entry upcase table (a-z folded) and its
 * exact checksum32 stamp - the dentry and the stream stay consistent by
 * construction. Mirrors the bytes build_exfat_image plants at cluster 60. */
u32 exfat_test_upcase_table_checksum() {
    std::vector<u8> tbl(128, 0);
    for (u32 i = 0; i < 64; ++i) {
        const u32 ch = (i >= 'a' && i <= 'z') ? (i - 0x20) : i;
        tbl[i * 2] = u8(ch);
        tbl[i * 2 + 1] = u8(ch >> 8);
    }
    return checksum32(tbl.data(), u32(tbl.size()), 0);
}

std::vector<u8> build_exfat_image(u8 tz = 0) {
    /* 2048 sectors (1MB) - spec minimum volume size; layout keeps the
     * small fixture geometry inside (heap 25..124, rest unused) */
    std::vector<u8> img(u64(2048) * 512, 0);

    /* ---- boot sector (main at 0, backup at 12) ---- */
    u8* b = img.data();
    b[0] = 0xEB;
    b[1] = 0x76;
    b[2] = 0x90;
    std::memcpy(b + 3, "EXFAT   ", 8);
    /* MustBeZero 11..63 stays zero */
    put64(img, 64, 0);    /* partition offset */
    put64(img, 72, 2048); /* volume length (spec min 1MB) */
    put32v(img, 80, 24);  /* fat offset */
    put32v(img, 84, 1);   /* fat length */
    put32v(img, 88, 25);  /* cluster heap offset */
    put32v(img, 92, 100); /* cluster count */
    put32v(img, 96, 2);   /* root first cluster */
    put32v(img, 100, 0x12345678);
    put16(img, 104, 0x0100); /* fs revision 1.00 */
    put16(img, 106, 0);      /* volume flags */
    b[108] = 9;              /* 512B sectors */
    b[109] = 0;              /* 1 sector per cluster */
    b[110] = 1;              /* 1 FAT */
    b[111] = 0x80;
    b[112] = 0xFF; /* percent in use: n/a */
    b[510] = 0x55;
    b[511] = 0xAA;
    std::memcpy(img.data() + 12 * 512, b, 512); /* backup copy */

    /* extended boot sectors 1..8 / 13..20: 0xAA550000 at +508 */
    for (u32 s = 1; s <= 8; ++s) {
        img[s * 512 + 508] = 0x00;
        img[s * 512 + 509] = 0x00;
        img[s * 512 + 510] = 0x55;
        img[s * 512 + 511] = 0xAA;
        img[(12 + s) * 512 + 508] = 0x00;
        img[(12 + s) * 512 + 509] = 0x00;
        img[(12 + s) * 512 + 510] = 0x55;
        img[(12 + s) * 512 + 511] = 0xAA;
    }
    /* boot-region checksum: over sectors 0..10, bytes 106/107/112 of
     * sector 0 skipped entirely (spec 3.1.14.1); FILL-REPEATED at 11/23 */
    u32 chk = 0;
    for (u32 s = 0; s <= 10; ++s) {
        for (u32 i = 0; i < 512; ++i) {
            if (s == 0 && ((i >= 106 && i < 108) || i == 112))
                continue;
            chk = ((chk & 1) ? 0x80000000u : 0u) + (chk >> 1) + img[s * 512 + i];
        }
    }
    for (u32 k = 0; k < 128; ++k) {
        put32v(img, 11 * 512 + k * 4, chk);
        put32v(img, 23 * 512 + k * 4, chk);
    }

    /* ---- FAT (sector 24) ---- */
    const size_t FAT = 24 * 512;
    put32v(img, FAT + 2 * 4, 3);          /* root chain: 2 -> 3 */
    put32v(img, FAT + 3 * 4, 0xFFFFFFFF); /* root EOC */
    put32v(img, FAT + 20 * 4, 21);
    put32v(img, FAT + 21 * 4, 22);
    put32v(img, FAT + 22 * 4, 0xFFFFFFFF); /* doc.txt 20-22 */
    put32v(img, FAT + 50 * 4, 0xFFFFFFFF); /* bitmap */
    put32v(img, FAT + 60 * 4, 0xFFFFFFFF); /* upcase table */
    put32v(img, FAT + 70 * 4, 0xFFFFFFFF); /* subdir */

    /* ---- allocation bitmap at cluster 50 (sector 75) ----
     * M-054: the on-disk bitmap's bit i describes cluster i+2 (MS exFAT
     * spec section 7.1) - setbit(c) writes the bit FOR cluster c. */
    {
        const size_t BM = 25 * 512 + (50 - 2) * 512;
        auto setbit = [&](u32 c) {
            const u32 bit = c - 2; /* cluster c -> bit c-2 (spec) */
            img[BM + bit / 8] |= u8(1u << (bit % 8));
        };
        setbit(2);
        setbit(3); /* root (2-cluster chain) */
        setbit(10);
        setbit(11); /* photo.jpg */
        setbit(20);
        setbit(21);
        setbit(22); /* doc.txt */
        setbit(50); /* bitmap itself */
        setbit(60); /* upcase table */
        setbit(70); /* subdir */
        setbit(80); /* inner.txt */
        setbit(40); /* over.dat's cluster: REUSED */
        /* 30 (gone.dat) NOT set */
    }

    /* ---- root dir at cluster 2 (sector 25) ---- */
    const size_t ROOT = 25 * 512;
    size_t at = 0;
    const u32 TS = (0x5A46u << 16) | 0x6000u; /* 2025-02-06 12:00 local */

    /* 0x81 bitmap dentry */
    {
        u8* e = img.data() + ROOT;
        std::memset(e, 0, 32);
        e[0] = 0x81;
        e[1] = 0;
        put32v(img, ROOT + 20, 50);
        put64(img, ROOT + 24, 13); /* (100+7)/8 */
        at += 32;
    }
    /* 0x82 upcase: a REAL 64-entry table at cluster 60 with its exact
     * checksum32 stamp (N-067: the volume-integrity pass validates this
     * stream; corrupt-checksum variants mutate the dentry/bytes). */
    {
        u8* e = img.data() + ROOT + at;
        std::memset(e, 0, 32);
        e[0] = 0x82;
        put32v(img, ROOT + at + 4, exfat_test_upcase_table_checksum());
        put32v(img, ROOT + at + 20, 60);
        put64(img, ROOT + at + 24, 128);
        /* the table bytes at cluster 60 = sector 83: 64 u16 upcase
         * entries, ASCII a-z folded (identical bytes to the checksum
         * helper above) */
        for (u32 i = 0; i < 64; ++i) {
            const u32 ch = (i >= 'a' && i <= 'z') ? (i - 0x20) : i;
            put16(img, 83 * 512 + i * 2, char16_t(ch));
        }
        at += 32;
    }
    /* 0x83 volume label */
    {
        u8* e = img.data() + ROOT + at;
        std::memset(e, 0, 32);
        e[0] = 0x83;
        e[1] = 7;
        const char16_t* lbl = u"TESTVOL";
        for (int i = 0; i < 7; ++i)
            put16(img, ROOT + at + 2 + i * 2, lbl[i]);
        at += 32;
    }
    /* photo.jpg: live NoFatChain (clusters 10-11, 1024B). Carries the
     * caller's tz bytes (S-011: non-zero values prove the engine leaves
     * them unapplied). */
    {
        SetBuilder sb;
        size_t set_at = ROOT + at;
        sb.file_entry(0x20, TS, tz);
        sb.stream_entry(0x03 /*AllocPossible|NoFatChain*/, 9, 10, 1024);
        sb.name_entry(u"photo.jpg");
        std::memcpy(img.data() + set_at, sb.entries.data(), sb.entries.size());
        sb.finish_set(img, set_at);
        at += u32(sb.entries.size());
    }
    /* doc.txt: live FAT chain 20-22 (1536B) */
    {
        SetBuilder sb;
        size_t set_at = ROOT + at;
        sb.file_entry(0x20, TS);
        sb.stream_entry(0x01 /*AllocPossible, FAT chain*/, 7, 20, 1536);
        sb.name_entry(u"doc.txt");
        std::memcpy(img.data() + set_at, sb.entries.data(), sb.entries.size());
        sb.finish_set(img, set_at);
        at += u32(sb.entries.size());
    }
    /* gone.dat: DELETED NoFatChain (cluster 30, 512B; bitmap free).
     * M-013: the checksum IS stamped (finish_set) - a real deleted set
     * keeps its original SetChecksum field (deletion only clears the
     * InUse bits; the og's dual-form gate expects the restored form to
     * match it). The old "no checksum validation" comment encoded
     * N-003's refuted premise. */
    {
        SetBuilder sb;
        size_t set_at = ROOT + at;
        sb.file_entry_deleted(0x20, TS);
        sb.stream_entry_deleted(0x03, 8, 30, 512);
        sb.name_entry_deleted(u"gone.dat");
        std::memcpy(img.data() + set_at, sb.entries.data(), sb.entries.size());
        sb.finish_set(img, set_at);
        at += u32(sb.entries.size());
    }
    /* over.dat: DELETED NoFatChain (cluster 40; bitmap SET -> Overwritten) */
    {
        SetBuilder sb;
        size_t set_at = ROOT + at;
        sb.file_entry_deleted(0x20, TS);
        sb.stream_entry_deleted(0x03, 8, 40, 512);
        sb.name_entry_deleted(u"over.dat");
        std::memcpy(img.data() + set_at, sb.entries.data(), sb.entries.size());
        sb.finish_set(img, set_at);
        at += u32(sb.entries.size());
    }
    /* subdir: live dir (FAT chain, cluster 70, 512B) */
    {
        SetBuilder sb;
        size_t set_at = ROOT + at;
        sb.file_entry(0x10 /*Directory*/, TS);
        sb.stream_entry(0x01, 6, 70, 512);
        sb.name_entry(u"subdir");
        std::memcpy(img.data() + set_at, sb.entries.data(), sb.entries.size());
        sb.finish_set(img, set_at);
        at += u32(sb.entries.size());
    }
    /* end of directory: a zero entry follows (image is zero-filled) */

    /* ---- subdir content at cluster 70 (sector 93) ---- */
    {
        const size_t SUB = 25 * 512 + (70 - 2) * 512;
        SetBuilder sb;
        size_t set_at = SUB;
        sb.file_entry(0x20, TS);
        sb.stream_entry(0x03, 9, 80, 256);
        sb.name_entry(u"inner.txt");
        std::memcpy(img.data() + set_at, sb.entries.data(), sb.entries.size());
        sb.finish_set(img, set_at);
    }
    /* content markers */
    std::memcpy(img.data() + 25 * 512 + (10 - 2) * 512, "PHOTODATA", 9);
    std::memcpy(img.data() + 25 * 512 + (80 - 2) * 512, "INNERDATA", 9);
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

/* ---- S-011: exFAT timestamps match the og's tz-ignored semantics ------- */

namespace {

/* Scan a fixture built with the given photo.jpg tz bytes into the CALLER-
 * owned tree (the tree owns the nodes) and return the photo.jpg node (the
 * fixture's other sets always carry tz = 0x00). */
tree::FileNode* scan_photo_with_tz(u8 tz, tree::ResultTree& tree) {
    auto img = build_exfat_image(tz);
    MemDevice dev(std::move(img));
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost");
    ExfatScanFlags flags;
    ExfatScanStats stats;
    if (scan_exfat(&dev, flags, nullptr, nullptr, nullptr, tree, ra, rd, rl, stats) != OK)
        return nullptr;
    for (auto* n : collect_all(tree))
        if (n->name_utf8() == "photo.jpg")
            return n;
    return nullptr;
}

} // namespace

TEST(ExfatEngine, TimestampsMatchOgLocalAsUtc) {
    /* S-011: the og's exFATSearchDLL never reads the dentry tz bytes
     * (imports census: zero TZ-conversion APIs; six DosDateTimeToFileTime
     * call sites at exFATSearchDLL_decompiled.c:5215-5235 / :45174-45194,
     * date/time words only; zero quarter-hour arithmetic in the DLL).
     * A file whose entries carry tz = 0x88 (7-bit two's-complement, +2h)
     * must store the ON-DISK time as the FILETIME, exactly as a tz byte
     * of 0x00 would - the spec-correct -2h subtraction is the divergence
     * this test pins out (pre-fix core stored 10:00; the og stores 12:00
     * on every host, matching the on-disk wall clock through the shared
     * FAT-family datetime core). */
    tree::ResultTree tree;
    tree::FileNode* photo = scan_photo_with_tz(0x88, tree);
    ASSERT_NE(photo, nullptr);
    /* 2025-02-06 12:00 on-disk, carried through as-is (og parity) */
    const u64 want = 1738843200ULL * 10000000ULL + 116444736000000000ULL;
    EXPECT_EQ(photo->props()->times.created, want);
    EXPECT_EQ(photo->props()->times.modified, want);
    EXPECT_EQ(photo->props()->times.accessed, want);
}

TEST(ExfatEngine, TimestampsZeroTzPassThrough) {
    /* S-011 regression guard: tz = 0x00 is the spec's "offset not
     * specified" sentinel - both sides pass the on-disk time through
     * unchanged. (EndToEnd pins the same value via the default fixture;
     * this is the named guard for the S-011 fix.) */
    tree::ResultTree tree;
    tree::FileNode* photo = scan_photo_with_tz(0x00, tree);
    ASSERT_NE(photo, nullptr);
    const u64 want = 1738843200ULL * 10000000ULL + 116444736000000000ULL;
    EXPECT_EQ(photo->props()->times.created, want);
    EXPECT_EQ(photo->props()->times.modified, want);
    EXPECT_EQ(photo->props()->times.accessed, want);
}

TEST(ExfatEngine, TimestampsMatchExplorerDisplay) {
    /* S-011: tz = 0x20 (+8h - the East-Asia camera/SD-card case, where
     * consumer devices DO write the offset): the stored FILETIME is the
     * on-disk wall-clock time, which is what Explorer shows for exFAT on
     * the volume's home host and what the og's .rsf carries. UTC->local
     * display conversion is the consumer's job (the DRWUI layer); the
     * engine contract is the stored value, pinned here. */
    tree::ResultTree tree;
    tree::FileNode* photo = scan_photo_with_tz(0x20, tree);
    ASSERT_NE(photo, nullptr);
    const u64 want = 1738843200ULL * 10000000ULL + 116444736000000000ULL;
    EXPECT_EQ(photo->props()->times.created, want);
    EXPECT_EQ(photo->props()->times.modified, want);
    EXPECT_EQ(photo->props()->times.accessed, want);
}

TEST(ExfatEngine, TimestampsWestOffsetAlsoIgnored) {
    /* S-011 negative-direction pin: tz = 0x7C (7-bit two's-complement
     * 124 -> -4 quarter-hours = UTC-1). The pre-fix core's subtraction
     * flipped sign on west offsets (stored wall clock + 1 h); the pin
     * covers the negative two's-complement decode path the east/zero
     * cases do not exercise. */
    tree::ResultTree tree;
    tree::FileNode* photo = scan_photo_with_tz(0x7C, tree);
    ASSERT_NE(photo, nullptr);
    const u64 want = 1738843200ULL * 10000000ULL + 116444736000000000ULL;
    EXPECT_EQ(photo->props()->times.created, want);
    EXPECT_EQ(photo->props()->times.modified, want);
    EXPECT_EQ(photo->props()->times.accessed, want);
}

TEST(ExfatEngine, FreeMapExcludesVirtualClustersAndMatchesBitmap) {
    /* S-009 hardening: the exFAT allocation bitmap's bits 0/1 are the
     * VIRTUAL reserved clusters (spec-zero). The first landing inverted
     * them to FREE and the linear base math painted [heap_base - 2*cb,
     * heap_base) - the boot-checksum/FAT tail sectors here - as free
     * space. This test pins the exclusion + the fixture's free truth.
     * Fixture bits set: 2,3 (root), 10,11 (photo), 20,21,22 (doc), 40
     * (over.dat), 50 (bitmap), 70 (subdir), 80 (inner); cluster_count
     * 100 -> data clusters 2..101; gone.dat's cluster 30 is free. */
    auto img = build_exfat_image();
    MemDevice dev(std::move(img));

    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost");
    ExfatScanFlags flags;
    ExfatScanStats stats;
    storage::FreeMap fm;
    u32 rc = scan_exfat(&dev, flags, nullptr, nullptr, nullptr, tree, ra, rd, rl, stats, &fm);
    ASSERT_EQ(rc, OK) << err_text(rc);

    const u64 HEAP = 25 * 512;       /* cluster 2 = sector 25 */
    const u64 VIRT = HEAP - 2 * 512; /* virtual clusters' byte span */
    ASSERT_TRUE(fm.valid());
    EXPECT_EQ(fm.base_byte, VIRT);
    EXPECT_EQ(fm.cluster_bytes, 512u);

    /* THE DEFECT PIN: the pre-heap region (boot checksum + FAT tail
     * here) is NOT free - virtual clusters excluded */
    EXPECT_FALSE(fm.range_free(VIRT, 1024))
        << "virtual clusters 0/1 must never read free (boot/FAT region)";
    EXPECT_FALSE(fm.is_free(0));
    EXPECT_FALSE(fm.is_free(1));

    auto cl_free = [&](u32 c) { return fm.range_free(HEAP + u64(c - 2) * 512, 512); };
    for (u32 c : {4u, 9u, 12u, 19u, 23u, 30u, 39u, 41u, 49u, 51u, 69u, 71u, 79u, 81u, 101u})
        EXPECT_TRUE(cl_free(c)) << "cluster " << c << " must be free";
    for (u32 c : {2u, 3u, 10u, 11u, 20u, 21u, 22u, 40u, 50u, 60u, 70u, 80u})
        EXPECT_FALSE(cl_free(c)) << "cluster " << c << " is bitmap-allocated";
}

namespace {
/* S-009 hardening helper: fails every read touching one byte range (the
 * allocation-bitmap cluster on the fixture) - a torn bitmap load. */
class RangeFaultDevice final : public IStorageDevice {
public:
    RangeFaultDevice(std::vector<u8> b, u64 fault_off, u64 fault_len)
        : data_(std::move(b)), f0_(fault_off), f1_(fault_off + fault_len) {
        geom_.logical_sector = 512;
        geom_.physical_sector = 512;
        geom_.sector_count = data_.size() / 512;
        geom_.total_bytes = data_.size();
    }
    u32 read_at(u64 off, void* buf, u32 len) override {
        if (off > data_.size() || len > data_.size() - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        if (off < f1_ && off + len > f0_)
            return ERR_READ_FAILED;
        std::memcpy(buf, data_.data() + off, len);
        return OK;
    }
    const Geometry& geometry() const override { return geom_; }

private:
    std::vector<u8> data_;
    u64 f0_, f1_;
    Geometry geom_;
};
} // namespace

TEST(ExfatEngine, TornBitmapReadYieldsNoFreeMap) {
    /* S-009 hardening: a FAILED allocation-bitmap read leaves zeros in
     * the loader buffer; zeros read "not allocated" and the inversion
     * would paint the whole covered range FREE - a false-free map over a
     * lie. The honest answer is NO map (invalid) - the disk map then
     * renders Unknown cells, never false-free ones. The fixture's
     * bitmap lives at cluster 50 (sector 75). */
    auto img = build_exfat_image();
    const u64 bitmap_off = (25 * 512) + (50 - 2) * 512;
    RangeFaultDevice dev(std::move(img), bitmap_off, 512);

    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost");
    ExfatScanFlags flags;
    ExfatScanStats stats;
    storage::FreeMap fm;
    u32 rc = scan_exfat(&dev, flags, nullptr, nullptr, nullptr, tree, ra, rd, rl, stats, &fm);
    ASSERT_EQ(rc, OK) << err_text(rc) << " (a torn bitmap is not a scan error)";
    EXPECT_FALSE(fm.valid()) << "a torn bitmap read must yield NO free map - zeros are not truth";
}

TEST(ExfatEngine, EndToEnd) {
    auto img = build_exfat_image();
    MemDevice dev(std::move(img));

    ExfatParams params;
    ASSERT_EQ(validate_boot_region(&dev, params), OK);
    EXPECT_TRUE(params.boot_main_ok);
    EXPECT_TRUE(params.boot_backup_ok);
    EXPECT_TRUE(params.boot_checksum_ok);
    EXPECT_EQ(params.cluster_count, 100u);

    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost");
    ExfatScanFlags flags;
    ExfatScanStats stats;
    u32 rc = scan_exfat(&dev, flags, nullptr, nullptr, nullptr, tree, ra, rd, rl, stats);
    ASSERT_EQ(rc, OK) << err_text(rc);
    EXPECT_EQ(stats.sets_damaged, 0u);
    EXPECT_EQ(stats.no_fat_chains, 4u); /* photo, gone, over, inner (dirs never NoFatChain) */

    auto all = collect_all(tree);
    tree::FileNode *photo = nullptr, *doc = nullptr, *gone = nullptr, *over = nullptr,
                   *subdir = nullptr, *inner = nullptr;
    for (auto* n : all) {
        std::string nm = n->name_utf8();
        if (nm == "photo.jpg")
            photo = n;
        else if (nm == "doc.txt")
            doc = n;
        else if (nm == "gone.dat")
            gone = n;
        else if (nm == "over.dat")
            over = n;
        else if (nm == "subdir")
            subdir = n;
        else if (nm == "inner.txt")
            inner = n;
    }
    ASSERT_NE(photo, nullptr);
    ASSERT_NE(doc, nullptr);
    ASSERT_NE(gone, nullptr);
    ASSERT_NE(over, nullptr);
    ASSERT_NE(subdir, nullptr);
    ASSERT_NE(inner, nullptr);

    /* NoFatChain extents: photo clusters 10-11 -> heap offset 25 + (10-2) */
    ASSERT_EQ(photo->props()->extents.size(), 1u);
    EXPECT_EQ(photo->props()->extents[0].offset, (25 + 8) * 512);
    EXPECT_EQ(photo->props()->extents[0].length, 1024u);
    EXPECT_EQ(photo->props()->logical_size, 1024u);
    EXPECT_EQ(photo->confidence(), tree::Confidence::Excellent);

    /* FAT chain: doc 20-22 contiguous */
    ASSERT_EQ(doc->props()->extents.size(), 1u);
    EXPECT_EQ(doc->props()->extents[0].offset, (25 + 18) * 512);
    EXPECT_EQ(doc->props()->extents[0].length, 1536u);

    /* grading: gone (bitmap free) Excellent; over (bitmap set) Overwritten */
    EXPECT_EQ(gone->confidence(), tree::Confidence::Excellent);
    EXPECT_EQ(gone->deleted_state(), tree::DeletedState::Deleted);
    EXPECT_EQ(over->confidence(), tree::Confidence::Overwritten);
    EXPECT_EQ(over->deleted_state(), tree::DeletedState::Deleted);

    /* subdir/inner */
    EXPECT_EQ(inner->parent(), subdir);
    EXPECT_EQ(inner->props()->logical_size, 256u);
    EXPECT_EQ(inner->props()->extents[0].offset, (25 + 78) * 512);

    /* times: 2025-02-06 12:00 local (tz unused) -> unix 1738843200 */
    EXPECT_EQ(photo->props()->times.created, 1738843200ULL * 10000000ULL + 116444736000000000ULL);
}

/* N-067: the og's volume analyzer validates the upcase table as part of
 * volume integrity (boot + upcase checks, CExFatVolumeAnalyer - corpus
 * fat-exfat-engine-internals.md:63-64). Grade, not reject: a corrupt
 * table flags the integrity signal; the scan itself continues (the og's
 * corrupt-volume posture, the EXFAT-2 salvage lineage). The upcase
 * dentry sits right after the bitmap dentry at the root start. */
static size_t count_found_files(tree::FileNode* found_root) {
    size_t n = 0;
    for (tree::FileNode* c = found_root->first_child(); c; c = c->next_sibling())
        ++n;
    return n;
}

TEST(ExfatEngine, UpcaseTableChecksumValidated) {
    /* clean fixture: the table's checksum32 stamp matches its stream ->
     * validated, zero mismatches */
    {
        auto img = build_exfat_image();
        MemDevice dev(std::move(img));
        tree::ResultTree tree;
        auto* ra = tree.add_root(u"Found");
        auto* rd = tree.add_root(u"Deleted");
        auto* rl = tree.add_root(u"Lost");
        ExfatScanFlags flags;
        ExfatScanStats stats;
        ASSERT_EQ(
            scan_exfat(&dev, flags, nullptr, nullptr, nullptr, tree, ra, rd, rl, stats, nullptr),
            OK);
        EXPECT_GE(stats.upcase_validated, 1u) << "the in-use upcase dentry must be checked";
        EXPECT_EQ(stats.upcase_mismatches, 0u) << "the clean table must not flag";
    }
    /* corrupt TableChecksum: the flag fires and the scan still finds the
     * same files (grade, not reject) */
    {
        auto img = build_exfat_image();
        const size_t ROOT = 25 * 512;
        put32v(img, ROOT + 32 + 4, 0xDEADBEEF); /* the upcase dentry's stamp */
        MemDevice dev(std::move(img));
        tree::ResultTree tree;
        auto* ra = tree.add_root(u"Found");
        auto* rd = tree.add_root(u"Deleted");
        auto* rl = tree.add_root(u"Lost");
        ExfatScanFlags flags;
        ExfatScanStats stats;
        ASSERT_EQ(
            scan_exfat(&dev, flags, nullptr, nullptr, nullptr, tree, ra, rd, rl, stats, nullptr),
            OK)
            << "a corrupt upcase checksum must never stop the scan";
        EXPECT_EQ(stats.upcase_validated, 0u) << "a corrupt table does not count as validated";
        EXPECT_EQ(stats.upcase_mismatches, 1u) << "the corrupt stamp must flag";
        EXPECT_NE(count_found_files(ra), 0u) << "files are still found";
    }
}

TEST(ExfatEngine, UpcaseLengthSanity) {
    const size_t ROOT = 25 * 512;
    /* DataLength = 0: a contentless table cannot satisfy the mandatory
     * mapping - flagged, scan continues */
    {
        auto img = build_exfat_image();
        put64(img, ROOT + 32 + 24, 0);
        MemDevice dev(std::move(img));
        tree::ResultTree tree;
        auto* ra = tree.add_root(u"Found");
        auto* rd = tree.add_root(u"Deleted");
        auto* rl = tree.add_root(u"Lost");
        ExfatScanFlags flags;
        ExfatScanStats stats;
        ASSERT_EQ(
            scan_exfat(&dev, flags, nullptr, nullptr, nullptr, tree, ra, rd, rl, stats, nullptr),
            OK);
        EXPECT_EQ(stats.upcase_mismatches, 1u) << "zero-length table must flag";
        EXPECT_NE(count_found_files(ra), 0u);
    }
    /* DataLength absurd (4 GiB): flagged WITHOUT the over-read - the
     * sanity bar refuses before any stream access */
    {
        auto img = build_exfat_image();
        put64(img, ROOT + 32 + 24, 0x100000000ull);
        MemDevice dev(std::move(img));
        tree::ResultTree tree;
        auto* ra = tree.add_root(u"Found");
        auto* rd = tree.add_root(u"Deleted");
        auto* rl = tree.add_root(u"Lost");
        ExfatScanFlags flags;
        ExfatScanStats stats;
        ASSERT_EQ(
            scan_exfat(&dev, flags, nullptr, nullptr, nullptr, tree, ra, rd, rl, stats, nullptr),
            OK)
            << "an absurd length must never hang or fault the scan";
        EXPECT_EQ(stats.upcase_mismatches, 1u) << "the absurd length must flag";
        EXPECT_NE(count_found_files(ra), 0u);
    }
}

TEST(ExfatEngine, FreeMapProducerMatchesSpecBitMapping) {
    /* M-054 pin: the S-009 free-map producer must reflect the SPEC bit
     * mapping (on-disk bit i = cluster i+2, MS exFAT spec section 7.1).
     * The pre-fix engine's Bitmap::test read bit c for cluster c, so both
     * the deleted-viability grading AND the free map consulted cluster
     * c+2's allocation state - a two-cluster shift on every real volume.
     * Fail-before proof of record: with the fixtures spec-correct and the
     * engine body reverted, gone/over grading swaps (EndToEnd red) and
     * this producer's free/used answers shift by two clusters. */
    auto img = build_exfat_image();
    MemDevice dev(std::move(img));

    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost");
    ExfatScanFlags flags;
    ExfatScanStats stats;
    storage::FreeMap fm;
    ASSERT_EQ(scan_exfat(&dev, flags, nullptr, nullptr, nullptr, tree, ra, rd, rl, stats, &fm), OK)
        << err_text(0);
    ASSERT_TRUE(fm.valid());
    EXPECT_EQ(fm.cluster_bytes, 512u);
    /* the 0ba21d8 hardening: cluster numbers 0..cluster_count+1 (the two
     * virtual + ALL data clusters - the first landing dropped the last
     * two data clusters out of coverage); FreeMap::cluster_count is the
     * SLOT-COVERAGE count (cr_freemap.h - is_free bounds cluster <
     * cluster_count), so the map covers 2 virtual + 100 data slots = 102.
     * The M-054 pin initially raced the hardening (expected the raw spec
     * field 100); re-pinned by both lanes convergently. */
    EXPECT_EQ(fm.cluster_count, 102u);
    /* heap base = sector 25; the map's virtual cluster 0 sits 2 clusters
     * below it (the landed base's linear-cluster convention) */
    EXPECT_EQ(fm.base_byte, u64(25 - 2) * 512);
    /* in-use clusters read NOT free: the structural set (root 2-3, photo
     * 10-11, doc 20-22, bitmap 50, upcase table 60, subdir 70, inner 80)
     * + over.dat's REUSED cluster 40 (the M-054 sentinel: pre-fix, bit 40 was read for
     * cluster 40 - the bit that actually belongs to cluster 42) */
    for (u32 c : {2u, 3u, 10u, 11u, 20u, 21u, 22u, 40u, 50u, 60u, 70u, 80u})
        EXPECT_FALSE(fm.is_free(c)) << "cluster " << c << " must read in-use";
    /* free clusters read free: gone.dat's cluster 30 (the deleted-free
     * sentinel), untouched heap clusters, and the LAST two data clusters
     * (100/101 - the 0ba21d8 under-coverage fix's sentinel) */
    for (u32 c : {30u, 4u, 5u, 12u, 19u, 35u, 90u, 100u, 101u})
        EXPECT_TRUE(fm.is_free(c)) << "cluster " << c << " must read free";
    /* byte-granular truth at the sentinel: over.dat's cluster-40 byte
     * range is NOT range-free; gone.dat's cluster-30 range IS */
    const u64 heap = u64(25) * 512;
    EXPECT_FALSE(fm.range_free(heap + u64(40 - 2) * 512, 512));
    EXPECT_TRUE(fm.range_free(heap + u64(30 - 2) * 512, 512));
}

static std::vector<u8> exfat_image_with_torn_doc_checksum() {
    auto img = build_exfat_image();
    /* corrupt doc.txt's stream first_cluster low byte (breaks the set
     * checksum without breaking the grammar) */
    const size_t ROOT = 25 * 512;
    /* find doc.txt set: bitmap(1) + upcase(1) + label(1) + photo(3) = 5th
     * entry onward: entries 5,6,7 = doc set; stream is entry 6 */
    img[ROOT + 6 * 32 + 20] ^= 0x01;
    return img;
}

TEST(ExfatEngine, SetChecksumMismatchGradesExcellentInOgProfile) {
    /* N-003 (diff-to-fix): the og engine performs NO entry-set checksum
     * validation (verified absence) - default flags grade a live file with
     * a torn SetChecksum normally, exactly like the og. */
    MemDevice dev(exfat_image_with_torn_doc_checksum());
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    ExfatScanFlags flags; /* defaults: og-parity profile */
    ExfatScanStats stats;
    ASSERT_EQ(scan_exfat(&dev, flags, nullptr, nullptr, nullptr, tree, ra, nullptr, nullptr, stats),
              OK);
    EXPECT_EQ(0u, stats.sets_damaged); /* not even counted in og mode */
    auto all = collect_all(tree);
    tree::FileNode* doc = nullptr;
    for (auto* n : all)
        if (n->name_utf8() == "doc.txt")
            doc = n;
    ASSERT_NE(doc, nullptr);
    EXPECT_EQ(doc->confidence(), tree::Confidence::Excellent);
}

TEST(ExfatEngine, SetChecksumMismatchMarksDamaged) {
    /* honest-recovery profile: validate_entry_sets=true keeps the
     * deliberate strictness - the file is still emitted (never dropped),
     * downgraded to PartialRisk (pins the pre-N-003 behavior) */
    MemDevice dev(exfat_image_with_torn_doc_checksum());
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    ExfatScanFlags flags;
    flags.validate_entry_sets = true; /* honest mode */
    ExfatScanStats stats;
    ASSERT_EQ(scan_exfat(&dev, flags, nullptr, nullptr, nullptr, tree, ra, nullptr, nullptr, stats),
              OK);
    EXPECT_GE(stats.sets_damaged, 1u);
    auto all = collect_all(tree);
    tree::FileNode* doc = nullptr;
    for (auto* n : all)
        if (n->name_utf8() == "doc.txt")
            doc = n;
    ASSERT_NE(doc, nullptr);
    EXPECT_EQ(doc->confidence(), tree::Confidence::PartialRisk); /* live but set damaged */
}

/* ---- AUDIT-FAT regression batch (EXFAT-1/2/3/4) ---- */

/* additional sets are planted after the fixture's own 576 bytes of root
 * entries (3 singles + 5 three-entry sets), still inside the 2-cluster
 * root chain (1024 bytes) */
TEST(ExfatEngine, InvalidFirstClusterRejected) {
    /* EXFAT-1: og rejects first_cluster == 1 and >= 0xFFFFFFF7
     * (FUN_180003090:1857-1859). Before the fix, 0xFFFFFFF7+ passed the
     * `>= 2` gate and produced a garbage chain at an absurd offset. */
    auto img = build_exfat_image();
    const size_t ROOT = 25 * 512;
    const size_t AT = ROOT + 576;
    {
        SetBuilder sb;
        sb.file_entry(0x20, 0x5A466000u << 0 | 0x5A466000);
        sb.stream_entry(0x01, 8, 1, 512); /* fc = 1: reserved */
        sb.name_entry(u"bad1.dat");
        std::memcpy(img.data() + AT, sb.entries.data(), sb.entries.size());
        sb.finish_set(img, AT);
    }
    {
        SetBuilder sb;
        size_t at = AT + 96;
        sb.file_entry(0x20, EXFAT_TEST_TS);
        sb.stream_entry(0x01, 10, 0xFFFFFFF9u, 512); /* fc in EOC family */
        sb.name_entry(u"badeoc.dat");
        std::memcpy(img.data() + at, sb.entries.data(), sb.entries.size());
        sb.finish_set(img, at);
    }
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    ExfatScanFlags flags;
    ExfatScanStats stats;
    ASSERT_EQ(scan_exfat(&dev, flags, nullptr, nullptr, nullptr, tree, ra, nullptr, nullptr, stats),
              OK);
    EXPECT_GE(stats.sets_damaged, 2u);
    auto all = collect_all(tree);
    tree::FileNode *bad1 = nullptr, *badeoc = nullptr;
    for (auto* n : all) {
        if (n->name_utf8() == "bad1.dat")
            bad1 = n;
        if (n->name_utf8() == "badeoc.dat")
            badeoc = n;
    }
    ASSERT_NE(bad1, nullptr) << "metadata-only emission expected";
    ASSERT_NE(badeoc, nullptr) << "metadata-only emission expected";
    EXPECT_TRUE(bad1->props()->extents.empty());
    EXPECT_TRUE(badeoc->props()->extents.empty()); /* no garbage chain */
}

TEST(ExfatEngine, EocFamilyTerminatesChain) {
    /* EXFAT-3: EOC family is 0xFFFFFFF8..0xFFFFFFFF. Terminator 0xFFFFFFFA
     * must end the chain cleanly (before: followed as a cluster number ->
     * garbage extent beyond the device). */
    auto img = build_exfat_image();
    const size_t FAT = 24 * 512;
    put32v(img, FAT + 90 * 4, 91);
    put32v(img, FAT + 91 * 4, 0xFFFFFFFAu); /* family member, not 0xFFFFFFFF */
    const size_t ROOT = 25 * 512;
    const size_t AT = ROOT + 576;
    SetBuilder sb;
    sb.file_entry(0x20, EXFAT_TEST_TS);
    sb.stream_entry(0x01, 7, 90, 1024);
    sb.name_entry(u"eoc.txt");
    std::memcpy(img.data() + AT, sb.entries.data(), sb.entries.size());
    sb.finish_set(img, AT);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    ExfatScanFlags flags;
    ExfatScanStats stats;
    ASSERT_EQ(scan_exfat(&dev, flags, nullptr, nullptr, nullptr, tree, ra, nullptr, nullptr, stats),
              OK);
    auto all = collect_all(tree);
    tree::FileNode* eoc = nullptr;
    for (auto* n : all)
        if (n->name_utf8() == "eoc.txt")
            eoc = n;
    ASSERT_NE(eoc, nullptr);
    ASSERT_EQ(eoc->props()->extents.size(), 1u);
    EXPECT_EQ(eoc->props()->extents[0].offset, u64(25 + 88) * 512); /* cluster 90 */
    EXPECT_EQ(eoc->props()->extents[0].length, 1024u);
}

TEST(ExfatEngine, FreeEntryContiguousSalvage) {
    /* EXFAT-2: og FUN_180021c60:28158-28163 continues with cluster+1 on a
     * FREE (zero) FAT entry - the salvage heuristic for FAT-zeroed volumes
     * (deletion zeroes the chain). Before: hard break -> truncated extent. */
    auto img = build_exfat_image();
    /* FAT entries for clusters 100/101 stay ZERO (never planted) */
    const size_t ROOT = 25 * 512;
    const size_t AT = ROOT + 576;
    SetBuilder sb;
    sb.file_entry_deleted(0x20, EXFAT_TEST_TS);
    sb.stream_entry_deleted(0x01, 10, 100, 1024); /* 2 clusters expected */
    sb.name_entry_deleted(u"zeroed.dat");
    std::memcpy(img.data() + AT, sb.entries.data(), sb.entries.size());
    sb.finish_set(img, AT); /* M-013: the dual-form gate reads this field */
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rl = tree.add_root(u"Lost");
    ExfatScanFlags flags;
    ExfatScanStats stats;
    ASSERT_EQ(scan_exfat(&dev, flags, nullptr, nullptr, nullptr, tree, ra, nullptr, rl, stats), OK);
    EXPECT_GE(stats.assumed_contiguous, 1u);
    auto all = collect_all(tree);
    tree::FileNode* z = nullptr;
    for (auto* n : all)
        if (n->name_utf8() == "zeroed.dat")
            z = n;
    ASSERT_NE(z, nullptr);
    /* full contiguous extent [100,101] - NOT the truncated single cluster */
    ASSERT_EQ(z->props()->extents.size(), 1u);
    EXPECT_EQ(z->props()->extents[0].offset, u64(25 + 98) * 512); /* cluster 100 */
    EXPECT_EQ(z->props()->extents[0].length, 1024u);
    /* assumed chain never grades above PartialRisk */
    EXPECT_EQ(z->confidence(), tree::Confidence::PartialRisk);
}

TEST(ExfatEngine, BootGatesMatchOg) {
    /* EXFAT-4: the four og boot gates we lacked (FUN_180021e80) */
    auto reject = [](std::vector<u8> img) {
        MemDevice dev(std::move(img));
        ExfatParams p;
        EXPECT_EQ(validate_boot_region(&dev, p), ERR_BAD_PARAMETER);
    };
    { /* (a) MustBeZero sweep +0x0B..+0x3F */
        auto img = build_exfat_image();
        img[0x20] = 1;
        reject(std::move(img));
    }
    { /* (b) fat_count in [1,2] */
        auto img = build_exfat_image();
        img[110] = 0;
        reject(std::move(img));
    }
    { /* (c) fat_length != 0 */
        auto img = build_exfat_image();
        put32v(img, 84, 0);
        reject(std::move(img));
    }
    { /* (d) cluster-heap fit: (count-1)<<spc + heap > volume_length */
        auto img = build_exfat_image();
        put32v(img, 92, 2100); /* (2099)<<0 + 25 = 2124 > 2048 */
        reject(std::move(img));
    }
}

/* ---- S-013: FATs over 32 MiB run windowed; chains never truncate ------ */

namespace {

/* Large-FAT fixture: 512 B sectors/clusters, FAT = 65537 sectors =
 * 33,563,264 bytes (> 32 MiB - the S-013 trigger). Heap at sector 65561;
 * cluster_count 0x40010 so a second-window cluster (0x40001+) is a
 * legal FAT entry (FAT byte 0x100000+ = window 1 - the refill path).
 * ~160 MiB device (boot validator requires volume_length <= device). */
std::vector<u8> build_exfat_large_fat_image() {
    const u32 BPS = 512, SPC = 0; /* 512 B clusters */
    const u32 FAT_OFF = 24, FAT_LEN = 65537;
    const u32 HEAP = FAT_OFF + FAT_LEN; /* 65561 */
    const u32 CCOUNT = 0x40010;
    const u64 VLEN = 328000;
    std::vector<u8> img(VLEN * BPS, 0);
    const size_t heap_bytes = size_t(HEAP) * BPS;
    auto clus_off = [&](u32 c) -> size_t { return heap_bytes + size_t(c - 2) * BPS; };

    /* boot: main + backup + extended + checksum (same recipe as the
     * small fixture, with the large geometry) */
    u8* b = img.data();
    b[0] = 0xEB;
    b[1] = 0x76;
    b[2] = 0x90;
    std::memcpy(b + 3, "EXFAT   ", 8);
    put64(img, 72, VLEN);
    put32v(img, 80, FAT_OFF);
    put32v(img, 84, FAT_LEN);
    put32v(img, 88, HEAP);
    put32v(img, 92, CCOUNT);
    put32v(img, 96, 2); /* root cluster */
    put32v(img, 100, 0x12345678);
    put16(img, 104, 0x0100);
    put16(img, 106, 0);
    b[108] = 9;
    b[109] = SPC;
    b[110] = 1;
    b[111] = 0x80;
    b[112] = 0xFF;
    b[510] = 0x55;
    b[511] = 0xAA;
    std::memcpy(img.data() + 12 * BPS, b, BPS);
    for (u32 s = 1; s <= 8; ++s) {
        img[s * BPS + 510] = 0x55;
        img[s * BPS + 511] = 0xAA;
        img[(12 + s) * BPS + 510] = 0x55;
        img[(12 + s) * BPS + 511] = 0xAA;
    }
    u32 chk = 0;
    for (u32 s = 0; s <= 10; ++s)
        for (u32 i = 0; i < BPS; ++i) {
            if (s == 0 && ((i >= 106 && i < 108) || i == 112))
                continue;
            chk = ((chk & 1) ? 0x80000000u : 0u) + (chk >> 1) + img[s * BPS + i];
        }
    for (u32 k = 0; k < 128; ++k) {
        put32v(img, 11 * BPS + k * 4, chk);
        put32v(img, 23 * BPS + k * 4, chk);
    }

    /* FAT region: window-0 entries (bytes < 1 MiB) + window-1 entries
     * (cluster 0x40001 -> FAT byte 0x100004) */
    const size_t FAT = size_t(FAT_OFF) * BPS;
    auto fe = [&](u32 c, u32 v) { put32v(img, FAT + size_t(c) * 4, v); };
    fe(2, 0xFFFFFFFF); /* root: single cluster */
    fe(20, 21);
    fe(21, 22);
    fe(22, 0xFFFFFFFF);            /* doc.txt 20-22 (window 0) */
    for (u32 c = 50; c < 114; ++c) /* allocation bitmap chain 50..113 */
        fe(c, c + 1);
    fe(113, 0xFFFFFFFF);
    fe(0x40001, 0x40002); /* far.dat - WINDOW 1 */
    fe(0x40002, 0xFFFFFFFF);
    fe(0x40005, 0x40006); /* fardir - WINDOW 1, 2 clusters */
    fe(0x40006, 0xFFFFFFFF);

    /* allocation bitmap at cluster 50: bits for the used low clusters
     * (M-054: bit c-2 is cluster c's bit - spec section 7.1) */
    {
        const size_t BM = clus_off(50);
        auto setbit = [&](u32 c) {
            const u32 bit = c - 2;
            img[BM + bit / 8] |= u8(1u << (bit % 8));
        };
        setbit(2);
        setbit(20);
        setbit(21);
        setbit(22);
        for (u32 c = 50; c < 114; ++c)
            setbit(c);
        /* far clusters (0x40001/2/5/6) left FREE in the bitmap: chain
         * length is what the tests assert; grading is not */
    }

    /* root directory at cluster 2 */
    const u32 TS = (0x5A46u << 16) | 0x6000u;
    const size_t ROOT = clus_off(2);
    size_t at = 0;
    {
        u8* e = img.data() + ROOT;
        std::memset(e, 0, 32);
        e[0] = 0x81; /* allocation bitmap */
        put32v(img, ROOT + 20, 50);
        put64(img, ROOT + 24, 32786); /* (0x40010+7)/8 */
        at += 32;
    }
    {
        SetBuilder sb;
        size_t set_at = ROOT + at;
        sb.file_entry(0x20, TS);
        sb.stream_entry(0x01, 7, 20, 1536); /* FAT chain, window 0 */
        sb.name_entry(u"doc.txt");
        std::memcpy(img.data() + set_at, sb.entries.data(), sb.entries.size());
        sb.finish_set(img, set_at);
        at += sb.entries.size();
    }
    {
        SetBuilder sb;
        size_t set_at = ROOT + at;
        sb.file_entry(0x20, TS);
        sb.stream_entry(0x01, 7, 0x40001, 1024); /* FAT chain, WINDOW 1 */
        sb.name_entry(u"far.dat");
        std::memcpy(img.data() + set_at, sb.entries.data(), sb.entries.size());
        sb.finish_set(img, set_at);
        at += sb.entries.size();
    }
    {
        SetBuilder sb;
        size_t set_at = ROOT + at;
        sb.file_entry(0x10, TS);
        sb.stream_entry(0x01, 6, 0x40005, 1024); /* dir FAT chain, WINDOW 1 */
        sb.name_entry(u"fardir");
        std::memcpy(img.data() + set_at, sb.entries.data(), sb.entries.size());
        sb.finish_set(img, set_at);
        at += sb.entries.size();
    }

    /* fardir content: cluster 1 (0x40005) is filled with EXACTLY 16
     * valid entries (3 padding sets of 5+5+6 entries - names sized so
     * the sets tile the 512-byte cluster with no zero gap, because the
     * entry walk terminates at the first zero entry); inner.txt's set
     * lands in fardir's SECOND cluster (0x40006) - pre-S-013 the
     * 1-cluster truncation read only 0x40005 and never found it. */
    {
        const size_t FAR1 = clus_off(0x40005);
        const std::u16string pad1 =
            u"padding-file-with-a-really-long-name-number-one-aaaaa"; /* 53 -> 4 names -> 6 entries
                                                                       */
        const std::u16string pad2 =
            u"padding-file-with-a-really-long-name-number-two-bbbbb"; /* 53 -> 4 names -> 6 entries
                                                                       */
        const std::u16string pad3 =
            u"padding-file-three-cccc"; /* 24 -> 2 names -> 4 entries (6+6+4 = 16 exact) */
        size_t at2 = 0;
        for (const auto& nm : {pad1, pad2, pad3}) {
            SetBuilder sb;
            size_t set_at = FAR1 + at2;
            sb.file_entry(0x20, TS);
            sb.stream_entry(0x03, u8(nm.size()), 90, 512); /* NoFatChain; true name len */
            sb.name_entry(nm.c_str());
            std::memcpy(img.data() + set_at, sb.entries.data(), sb.entries.size());
            sb.finish_set(img, set_at);
            at2 += sb.entries.size();
        }
        EXPECT_EQ(at2, size_t(16 * 32))
            << "padding sets must tile the cluster exactly (got " << at2 << " bytes)";
        const size_t FAR2 = clus_off(0x40006);
        SetBuilder sb;
        sb.file_entry(0x20, TS);
        sb.stream_entry(0x03, 9, 80, 256); /* NoFatChain, data never read */
        sb.name_entry(u"inner.txt");
        std::memcpy(img.data() + FAR2, sb.entries.data(), sb.entries.size());
        sb.finish_set(img, FAR2);
    }
    return img;
}


/* N-067 empirical pin: the checksum algorithm is whatever REAL volume
 * writers use. tests/data/exfat/upcase-table-mkfs-exfat-1.2.9.bin is the
 * 5836-byte spec up-case table extracted from a fresh mkfs.exfat
 * (exfatprogs 1.2.9) image, whose on-disk TableChecksum is 0xE619D30D.
 * A second lane's first cut computed the MS-spec seeded CRC-32 instead
 * (0x673C42A0) - spec-memory, wrong in reality; this pin holds the
 * algorithm to the interoperable truth (Windows mounts exfatprogs
 * images, so the value real tools write IS the algorithm). */
TEST(ExfatEngine, UpcaseAlgorithmMatchesRealMkfsExfat) {
    FILE* f = fopen("data/exfat/upcase-table-mkfs-exfat-1.2.9.bin", "rb");
    if (!f) {
        GTEST_SKIP() << "fixture data/exfat/upcase-table-mkfs-exfat-1.2.9.bin not present";
        return;
    }
    std::vector<u8> table;
    u8 chunk[4096];
    size_t got = 0;
    while ((got = fread(chunk, 1, sizeof(chunk), f)) > 0)
        table.insert(table.end(), chunk, chunk + got);
    fclose(f);
    ASSERT_EQ(table.size(), 5836u) << "the spec up-case table is exactly 5836 bytes";
    EXPECT_EQ(checksum32(table.data(), u32(table.size()), 0), 0xE619D30Du)
        << "must match mkfs.exfat 1.2.9's on-disk TableChecksum";
}

} // namespace

TEST(ExfatEngine, LargeFatFullChains) {
    /* S-013: a >32 MiB FAT (33,563,264 B) used to silently set have_fat
     * false and truncate EVERY FAT-chained file to one cluster. Now the
     * whole-FAT fast path falls back to the og's windowed accessor
     * (FUN_180021c60: 1 MiB windows, no size cap): doc.txt (window-0
     * chain) and far.dat (window-1 chain - a refill) both recover full
     * length. */
    auto img = build_exfat_large_fat_image();
    MemDevice dev(std::move(img));
    tree::ResultTree t;
    auto* ra = t.add_root(u"Found");
    auto* rd = t.add_root(u"Deleted");
    auto* rl = t.add_root(u"Lost Files");
    ExfatScanFlags fl;
    ExfatScanStats st;
    ASSERT_EQ(scan_exfat(&dev, fl, nullptr, nullptr, nullptr, t, ra, rd, rl, st), OK);
    tree::FileNode* doc = nullptr;
    tree::FileNode* far_n = nullptr;
    for (tree::FileNode* n : collect_all(t)) {
        const std::string nm = n->name_utf8();
        if (nm == "doc.txt")
            doc = n;
        if (nm == "far.dat")
            far_n = n;
    }
    ASSERT_NE(doc, nullptr) << "window-0 FAT-chained file missing";
    ASSERT_NE(far_n, nullptr) << "window-1 FAT-chained file missing";
    /* logical_size mirrors the stream entry on BOTH sides of the fix; the
     * S-013 truncation is in EXTENT COVERAGE - pre-fix the chain was a
     * single 512-byte cluster, so recovery of the tail was silently
     * impossible. Assert the extents cover the full chain. */
    EXPECT_EQ(doc->props()->logical_size, u64(1536));
    EXPECT_EQ(far_n->props()->logical_size, u64(1024));
    ASSERT_EQ(doc->props()->extents.size(), size_t(1));
    ASSERT_EQ(far_n->props()->extents.size(), size_t(1));
    EXPECT_EQ(doc->props()->extents.front().length, u64(1536));   /* was 512 pre-fix */
    EXPECT_EQ(far_n->props()->extents.front().length, u64(1024)); /* was 512 pre-fix */
    EXPECT_GE(st.fat_window_refills, 1u); /* the far chain crossed windows */
}

TEST(ExfatEngine, LargeFatDirectoryWalk) {
    /* S-013 directory half: a directory whose FAT chain lives in window 1
     * walks BOTH clusters - inner.txt sits in fardir's SECOND cluster, so
     * the pre-fix one-cluster truncation never found it. */
    auto img = build_exfat_large_fat_image();
    MemDevice dev(std::move(img));
    tree::ResultTree t;
    auto* ra = t.add_root(u"Found");
    auto* rd = t.add_root(u"Deleted");
    auto* rl = t.add_root(u"Lost Files");
    ExfatScanFlags fl;
    ExfatScanStats st;
    ASSERT_EQ(scan_exfat(&dev, fl, nullptr, nullptr, nullptr, t, ra, rd, rl, st), OK);
    tree::FileNode* inner = nullptr;
    for (tree::FileNode* n : collect_all(t))
        if (n->name_utf8() == "inner.txt")
            inner = n;
    ASSERT_NE(inner, nullptr) << "fardir's second cluster was never walked";
    EXPECT_EQ(inner->props()->logical_size, u64(256));
    /* the fardir node itself: pre-fix its chain was 1 cluster */
    tree::FileNode* fardir_n = nullptr;
    for (tree::FileNode* n : collect_all(t))
        if (n->name_utf8() == "fardir")
            fardir_n = n;
    ASSERT_NE(fardir_n, nullptr);
    ASSERT_EQ(fardir_n->props()->extents.size(), size_t(1));
    EXPECT_EQ(fardir_n->props()->extents.front().length, u64(1024)); /* was 512 pre-fix */
}

/* ---- N-010e (diff-to-fix session): og byte-OR reserved-field form ------ */

TEST(ExfatEngine, StreamReservedPartiallyNonzeroPassesOgByteOr) {
    /* N-010e: the og's stream-entry gate (FUN_180003090 :1843-1846) is
     * BYTE-granular with an OR form - reserved2 passes when either byte
     * is 0, reserved3 when any of its four bytes is 0 (single-byte loads
     * in the decompile, not a u16/u32 word test). A partially-nonzero
     * reserved field the old core word-test flagged as damaged is NOT
     * damaged on the og. reserved1 (byte +2) stays strictly zero. */
    auto img = build_exfat_image();
    const size_t ROOT = 25 * 512;
    const size_t AT = ROOT + 576;
    {
        SetBuilder sb;
        sb.file_entry(0x20, EXFAT_TEST_TS);
        sb.stream_entry(0x01, 11, 60, 512);
        sb.name_entry(u"partres.dat");
        std::memcpy(img.data() + AT, sb.entries.data(), sb.entries.size());
        sb.finish_set(img, AT);
        /* reserved2 (u16 @+6): high byte set, low byte zero -> og passes */
        img[AT + 32 + 7] = 0x01;
        /* reserved3 (u32 @+0x10): one nonzero byte, three zero -> og passes */
        img[AT + 32 + 0x11] = 0x02;
    }
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    ExfatScanFlags flags;
    ExfatScanStats stats;
    ASSERT_EQ(scan_exfat(&dev, flags, nullptr, nullptr, nullptr, tree, ra, nullptr, nullptr, stats),
              OK);
    EXPECT_EQ(stats.sets_damaged, 0u)
        << "partially-nonzero reserved2/3 must pass the og byte-OR form";
    auto all = collect_all(tree);
    tree::FileNode* partres = nullptr;
    for (auto* n : all)
        if (n->name_utf8() == "partres.dat")
            partres = n;
    ASSERT_NE(partres, nullptr) << "the set must still be emitted undamaged";
}

TEST(ExfatEngine, StreamReservedAllBytesNonzeroStillDamaged) {
    /* N-010e companion: reserved2 with BOTH bytes nonzero fails the og's
     * byte-OR form (no zero byte in the field) - still damaged. */
    auto img = build_exfat_image();
    const size_t ROOT = 25 * 512;
    const size_t AT = ROOT + 576;
    {
        SetBuilder sb;
        sb.file_entry(0x20, EXFAT_TEST_TS);
        sb.stream_entry(0x01, 11, 60, 512);
        sb.name_entry(u"fullres.dat");
        std::memcpy(img.data() + AT, sb.entries.data(), sb.entries.size());
        sb.finish_set(img, AT);
        img[AT + 32 + 6] = 0x01;
        img[AT + 32 + 7] = 0x02;
    }
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    ExfatScanFlags flags;
    ExfatScanStats stats;
    ASSERT_EQ(scan_exfat(&dev, flags, nullptr, nullptr, nullptr, tree, ra, nullptr, nullptr, stats),
              OK);
    EXPECT_GE(stats.sets_damaged, 1u) << "all-nonzero reserved2 must stay damaged";
}

TEST(ExfatEngine, StreamReserved1ByteStrictlyZero) {
    /* N-010e: reserved1 (byte @+2) is a strict zero test on the og
     * (param_1[2] == 0 with no OR escape) - any nonzero value damages. */
    auto img = build_exfat_image();
    const size_t ROOT = 25 * 512;
    const size_t AT = ROOT + 576;
    {
        SetBuilder sb;
        sb.file_entry(0x20, EXFAT_TEST_TS);
        sb.stream_entry(0x01, 8, 60, 512);
        sb.name_entry(u"res1.dat");
        std::memcpy(img.data() + AT, sb.entries.data(), sb.entries.size());
        sb.finish_set(img, AT);
        img[AT + 32 + 2] = 0x01; /* reserved1 */
    }
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    ExfatScanFlags flags;
    ExfatScanStats stats;
    ASSERT_EQ(scan_exfat(&dev, flags, nullptr, nullptr, nullptr, tree, ra, nullptr, nullptr, stats),
              OK);
    EXPECT_GE(stats.sets_damaged, 1u) << "reserved1 != 0 must damage (strict og test)";
}

/* ---- M-013 (diff-to-fix): the dual-form SetChecksum + the oracle ------ */

TEST(ExfatEngine, DeletedSetChecksumRejectsTornSet) {
    /* M-013 falsifiable claim (a): a deleted set with a torn
     * checksum-mismatched entry is REJECTED (the og's dual-form gate,
     * FUN_18001eba0/FUN_18001fef0) - the pre-fix core accepted it
     * unconditionally (the FP surface). Fail-before: with the gate
     * reverted, gone-corrupt IS emitted and sets_rejected stays 0. */
    auto img = build_exfat_image();
    const size_t ROOT = 25 * 512;
    /* gone.dat's set starts after bitmap(1)+upcase(1)+label(1)+photo(3)
     * +doc(3) = 9 entries */
    const size_t GONE = ROOT + 9 * 32;
    /* corrupt the name entry's first name byte ("g" -> "X"): the raw and
     * restored checksums both fail now */
    img[GONE + 2 * 32 + 2] = 'X';
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost");
    ExfatScanFlags flags; /* defaults: the deleted gate is unconditional og parity */
    ExfatScanStats stats;
    ASSERT_EQ(scan_exfat(&dev, flags, nullptr, nullptr, nullptr, tree, ra, rd, rl, stats), OK);
    EXPECT_EQ(stats.sets_rejected, 1u);
    EXPECT_EQ(stats.sets_reconstructed, 0u); /* no orphan tail to rebuild from */
    auto all = collect_all(tree);
    for (auto* n : all)
        EXPECT_NE(n->name_utf8(), "Xone.dat") << "the torn deleted set must not be emitted";
}

TEST(ExfatEngine, DeletedSetOracleReassembly) {
    /* M-013 falsifiable claim (c): a parked deleted set recoverable only
     * by oracle-guided reassembly (FUN_1800203a0's suffix substitution
     * re-tested against the stored checksum). The head's name entry is
     * torn; the CORRECT name entry survives as an orphan run elsewhere in
     * the directory - the reconstruction overlays it at the name slot,
     * the dual checksum matches, the file is emitted. Fail-before: with
     * the oracle reverted, the file is absent and sets_reconstructed is
     * 0. */
    auto img = build_exfat_image();
    const size_t ROOT = 25 * 512;
    const size_t AT = ROOT + 576; /* past the fixture's own sets */
    /* the parked head: the correct set with a TORN name entry */
    SetBuilder head;
    head.file_entry_deleted(0x20, EXFAT_TEST_TS);
    head.stream_entry_deleted(0x01, 9, 100, 512); /* "right.dat" = 9 chars */
    head.name_entry_deleted(u"right.dat");
    std::memcpy(img.data() + AT, head.entries.data(), head.entries.size());
    head.finish_set(img, AT); /* stamped over the CORRECT name bytes */
    img[AT + 2 * 32 + 2] = 'X'; /* tear the name's first byte post-stamp */

    /* the orphan tail: the CORRECT name entry, parked one entry later */
    SetBuilder tail;
    tail.name_entry_deleted(u"right.dat");
    std::memcpy(img.data() + AT + 3 * 32, tail.entries.data(), tail.entries.size());

    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost");
    ExfatScanFlags flags;
    ExfatScanStats stats;
    ASSERT_EQ(scan_exfat(&dev, flags, nullptr, nullptr, nullptr, tree, ra, rd, rl, stats), OK);
    EXPECT_EQ(stats.sets_rejected, 1u); /* the torn head was parked */
    EXPECT_EQ(stats.sets_reconstructed, 1u); /* the oracle rebuilt it */
    auto all = collect_all(tree);
    tree::FileNode* right = nullptr;
    for (auto* n : all)
        if (n->name_utf8() == "right.dat")
            right = n;
    ASSERT_NE(right, nullptr) << "the oracle must recover the torn set";
    EXPECT_EQ(right->props()->extents.size(), 1u);
    EXPECT_EQ(right->props()->logical_size, 512u);
}

TEST(ExfatEngine, LiveCompleteSetBadChecksumUngraded) {
    /* M-013 falsifiable claim (b): the inverse asymmetry - a COMPLETE
     * LIVE set with a bad checksum grades normally in the default (og)
     * profile: the og's complete-set path applies per-entry validators
     * without a checksum test. The deleted-set gate NEVER touches live
     * sets (sets_rejected stays 0). */
    MemDevice dev(exfat_image_with_torn_doc_checksum());
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    ExfatScanFlags flags; /* og profile */
    ExfatScanStats stats;
    ASSERT_EQ(scan_exfat(&dev, flags, nullptr, nullptr, nullptr, tree, ra, nullptr, nullptr,
                         stats),
              OK);
    EXPECT_EQ(stats.sets_rejected, 0u); /* the gate is deleted-sets-only */
    EXPECT_EQ(stats.sets_damaged, 0u);  /* og profile: no live downgrade either */
    auto all = collect_all(tree);
    tree::FileNode* doc = nullptr;
    for (auto* n : all)
        if (n->name_utf8() == "doc.txt")
            doc = n;
    ASSERT_NE(doc, nullptr);
    EXPECT_EQ(doc->confidence(), tree::Confidence::Excellent);
}

/* ---- M-014 (diff-to-fix): the og's structural entry gates -------------- */

namespace {

/* scan helper: returns every emitted node name (UTF-8) for gate tests */
std::vector<std::string> m014_scan_names(std::vector<u8> img, ExfatScanStats* stats_out) {
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    auto* rd = tree.add_root(u"Deleted");
    auto* rl = tree.add_root(u"Lost");
    ExfatScanFlags flags;
    ExfatScanStats stats;
    if (scan_exfat(&dev, flags, nullptr, nullptr, nullptr, tree, ra, rd, rl, stats) != OK) {
        if (stats_out)
            *stats_out = stats;
        return {};
    }
    if (stats_out)
        *stats_out = stats;
    std::vector<std::string> names;
    for (auto* n : collect_all(tree))
        names.push_back(n->name_utf8());
    return names;
}

} // namespace

TEST(ExfatEngine, EmptyFileSuppressedLikeOg) {
    /* M-014 falsifiable claim (a): the og's stream emission gate
     * (FUN_180029280 :34562-34564) requires data_length != 0 &&
     * first_cluster != 0 && name_length != 0 - a zero-length file never
     * appears in og results. Fail-before: the pre-fix core emitted it. */
    auto img = build_exfat_image();
    const size_t ROOT = 25 * 512;
    const size_t AT = ROOT + 576;
    {
        SetBuilder sb;
        sb.file_entry(0x20, EXFAT_TEST_TS);
        sb.stream_entry(0x01, 8, 0, 0); /* empty file: fc=0, len=0 */
        sb.name_entry(u"empty.dat");
        std::memcpy(img.data() + AT, sb.entries.data(), sb.entries.size());
        sb.finish_set(img, AT);
    }
    {
        /* len=0 but fc nonzero: still suppressed (data_length gate) */
        SetBuilder sb;
        size_t at = AT + 96;
        sb.file_entry(0x20, EXFAT_TEST_TS);
        sb.stream_entry(0x01, 8, 60, 0);
        sb.name_entry(u"empty2.dat");
        std::memcpy(img.data() + at, sb.entries.data(), sb.entries.size());
        sb.finish_set(img, at);
    }
    ExfatScanStats stats;
    const auto names = m014_scan_names(std::move(img), &stats);
    EXPECT_EQ(stats.sets_gated, 2u);
    for (const auto& n : names) {
        EXPECT_NE(n, "empty.dat") << "zero-length file must not be emitted (og parity)";
        EXPECT_NE(n, "empty2.dat") << "zero-length file must not be emitted (og parity)";
    }
}

TEST(ExfatEngine, GarbageTimestampRejected) {
    /* M-014 falsifiable claim (b): the og validates the create, modify
     * AND access words field-by-field (FUN_180002f40 via FUN_180002f90)
     * - month 0/13, day 0, hour 24 each reject the set. The all-zero
     * stamp fails too (day==0): the finding's fix-plan guess ("treat
     * all-zero as valid") is corrected by the decompile. */
    auto img = build_exfat_image();
    /* extend the root chain 2->3->4->5 (2048 B) so six planted sets fit
     * past the fixture's own 576 bytes (clusters 4/5 are free space in
     * the base fixture - nothing else lives there) */
    const size_t FATOFF = 24 * 512;
    put32v(img, FATOFF + 3 * 4, 4);
    put32v(img, FATOFF + 4 * 4, 5);
    put32v(img, FATOFF + 5 * 4, 0xFFFFFFFF);
    const size_t ROOT = 25 * 512;
    const size_t AT = ROOT + 576;
    struct Bad {
        const char* name;
        u32 ts;
    };
    const Bad bad[] = {
        {"tsmonth0.dat", (0x16u << 16) | 0x6000u},        /* month=0 (day=22) */
        {"tsmonth13.dat", (0x1A6u << 16) | 0x6000u},     /* month=13 (day=6) */
        {"tsday0.dat", (0x60u << 16) | 0x6000u},         /* day=0, month=3 */
        {"tshour24.dat", (0x5A46u << 16) | 0xC000u},     /* hour=24 */
        {"tszero.dat", 0},                               /* all zero */
        {"tsmin60.dat", (0x5A46u << 16) | 0x780u},       /* minute=60 */
    };
    size_t at = AT;
    for (const auto& b : bad) {
        std::u16string nm;
        for (const char* p = b.name; *p; ++p)
            nm.push_back(*p);
        SetBuilder sb;
        sb.file_entry(0x20, b.ts);
        sb.stream_entry(0x01, u8(nm.size()), 60, 512);
        sb.name_entry(nm.c_str());
        std::memcpy(img.data() + at, sb.entries.data(), sb.entries.size());
        sb.finish_set(img, at);
        at += sb.entries.size();
    }
    ExfatScanStats stats;
    const auto names = m014_scan_names(std::move(img), &stats);
    EXPECT_EQ(stats.sets_gated, 6u);
    for (const auto& b : bad) {
        bool found = false;
        for (const auto& n : names)
            if (n == b.name)
                found = true;
        EXPECT_FALSE(found) << b.name << " (garbage timestamp) must not be emitted";
    }
}

TEST(ExfatEngine, SecondaryCountCapped) {
    /* M-014 falsifiable claim (c): secondary_count is bounded to [2,18]
     * ((u8)(sc-2) < 0x11, FUN_180002f90 :1787) - sc=40 (and sc=1) reject
     * the set at the head, so a corrupt header cannot drag the walker
     * through arbitrary following entries. */
    auto img = build_exfat_image();
    const size_t ROOT = 25 * 512;
    const size_t AT = ROOT + 576;
    {
        SetBuilder sb;
        sb.file_entry(0x20, EXFAT_TEST_TS);
        sb.stream_entry(0x01, 8, 60, 512);
        sb.name_entry(u"wide.dat");
        std::memcpy(img.data() + AT, sb.entries.data(), sb.entries.size());
        sb.finish_set(img, AT);
        img[AT + 1] = 40; /* secondary_count = 40: out of range */
    }
    {
        SetBuilder sb;
        size_t at = AT + 96;
        sb.file_entry(0x20, EXFAT_TEST_TS);
        sb.stream_entry(0x01, 8, 60, 512);
        sb.name_entry(u"thin.dat");
        std::memcpy(img.data() + at, sb.entries.data(), sb.entries.size());
        sb.finish_set(img, at);
        img[at + 1] = 1; /* secondary_count = 1: below range */
    }
    ExfatScanStats stats;
    const auto names = m014_scan_names(std::move(img), &stats);
    EXPECT_EQ(stats.sets_gated, 2u);
    for (const auto& n : names) {
        EXPECT_NE(n, "wide.dat") << "sc=40 set must be skipped at the head";
        EXPECT_NE(n, "thin.dat") << "sc=1 set must be skipped at the head";
    }
}

TEST(ExfatEngine, BadNameFlagsRejected) {
    /* M-014 falsifiable claim (d): a name entry with flags&3 != 0 (or
     * flags >= 4) is rejected (FUN_180003030 :1814-1817: (flags & 3) == 0
     * && flags < 4); the first needed fragment must also carry a nonzero
     * first char. Fail-before: the pre-fix core accepted both. */
    auto img = build_exfat_image();
    const size_t ROOT = 25 * 512;
    const size_t AT = ROOT + 576;
    {
        SetBuilder sb;
        sb.file_entry(0x20, EXFAT_TEST_TS);
        sb.stream_entry(0x01, 8, 60, 512);
        sb.name_entry(u"flagbad.dat");
        std::memcpy(img.data() + AT, sb.entries.data(), sb.entries.size());
        sb.finish_set(img, AT);
        img[AT + 2 * 32 + 1] = 0x03; /* name flags & 3 != 0 */
    }
    {
        SetBuilder sb;
        size_t at = AT + 96;
        sb.file_entry(0x20, EXFAT_TEST_TS);
        sb.stream_entry(0x01, 8, 60, 512);
        sb.name_entry(u"flaghi.dat");
        std::memcpy(img.data() + at, sb.entries.data(), sb.entries.size());
        sb.finish_set(img, at);
        img[at + 2 * 32 + 1] = 0x04; /* name flags >= 4 */
    }
    {
        /* first char of the first needed fragment is zero */
        SetBuilder sb;
        size_t at = AT + 192;
        sb.file_entry(0x20, EXFAT_TEST_TS);
        sb.stream_entry(0x01, 8, 60, 512);
        sb.name_entry(u"zfirst.dat");
        std::memcpy(img.data() + at, sb.entries.data(), sb.entries.size());
        sb.finish_set(img, at);
        img[at + 2 * 32 + 2] = 0;
        img[at + 2 * 32 + 3] = 0;
    }
    ExfatScanStats stats;
    const auto names = m014_scan_names(std::move(img), &stats);
    EXPECT_EQ(stats.sets_gated, 3u);
    for (const auto& n : names) {
        EXPECT_NE(n, "flagbad.dat") << "flags&3!=0 name must reject the set";
        EXPECT_NE(n, "flaghi.dat") << "flags>=4 name must reject the set";
        EXPECT_NE(n, "zfirst.dat") << "zero first char must reject the set";
    }
}

TEST(ExfatEngine, FileReservedNonzeroRejected) {
    /* M-014: FUN_180002f90's reserved gates - reserved1 (u16 +6) and the
     * reserved2 tail (+0x19..+0x1F) must be zero on the FILE dentry (a
     * stricter test than the STREAM-side N-010e byte-OR form: the file
     * side has no OR escape). A failing set is skipped, not damaged. */
    auto img = build_exfat_image();
    const size_t ROOT = 25 * 512;
    const size_t AT = ROOT + 576;
    {
        SetBuilder sb;
        sb.file_entry(0x20, EXFAT_TEST_TS);
        sb.stream_entry(0x01, 8, 60, 512);
        sb.name_entry(u"fres1.dat");
        std::memcpy(img.data() + AT, sb.entries.data(), sb.entries.size());
        sb.finish_set(img, AT);
        img[AT + 6] = 0x01; /* reserved1 low byte */
    }
    {
        SetBuilder sb;
        size_t at = AT + 96;
        sb.file_entry(0x20, EXFAT_TEST_TS);
        sb.stream_entry(0x01, 8, 60, 512);
        sb.name_entry(u"fres2.dat");
        std::memcpy(img.data() + at, sb.entries.data(), sb.entries.size());
        sb.finish_set(img, at);
        img[at + 0x1A] = 0x02; /* reserved2 byte */
    }
    ExfatScanStats stats;
    const auto names = m014_scan_names(std::move(img), &stats);
    EXPECT_EQ(stats.sets_gated, 2u);
    for (const auto& n : names) {
        EXPECT_NE(n, "fres1.dat") << "file reserved1 != 0 must reject the set";
        EXPECT_NE(n, "fres2.dat") << "file reserved2 != 0 must reject the set";
    }
}

TEST(ExfatEngine, ValidBoundaryTimestampsStillPass) {
    /* M-014 regression guard: the boundary-VALID stamps must keep
     * passing - month 1 and 12, day 1 and 31, hour 0 and 23, minute 0
     * and 59, 2sec 0 and 29 (FUN_180002f40's exact bounds). Guards
     * against over-rejection from an off-by-one in the field masks. */
    auto img = build_exfat_image();
    const size_t ROOT = 25 * 512;
    const size_t AT = ROOT + 576;
    struct Ok {
        const char16_t* name;
        u32 ts;
    };
    const Ok ok[] = {
        {u"okmin.dat", (1u << 16) | (1u << 21)},                 /* Jan 1 00:00:00 */
        {u"okmax.dat", (31u << 16) | (12u << 21) | (23u << 11) | (59u << 5) | 29u}, /* Dec 31 23:59:58 */
        {u"oksec29.dat", (6u << 16) | (2u << 21) | 29u},         /* 2sec field = 29 */
    };
    size_t at = AT;
    for (const auto& o : ok) {
        std::u16string nm;
        for (const char16_t* p = o.name; *p; ++p)
            nm.push_back(*p);
        SetBuilder sb;
        sb.file_entry(0x20, o.ts);
        sb.stream_entry(0x01, u8(nm.size()), 60, 512);
        sb.name_entry(o.name);
        std::memcpy(img.data() + at, sb.entries.data(), sb.entries.size());
        sb.finish_set(img, at);
        at += sb.entries.size();
    }
    ExfatScanStats stats;
    const auto names = m014_scan_names(std::move(img), &stats);
    EXPECT_EQ(stats.sets_gated, 0u);
    std::string n0, n1, n2;
    for (const auto& n : names) {
        if (n == "okmin.dat")
            n0 = n;
        if (n == "okmax.dat")
            n1 = n;
        if (n == "oksec29.dat")
            n2 = n;
    }
    EXPECT_FALSE(n0.empty()) << "Jan 1 00:00:00 must pass";
    EXPECT_FALSE(n1.empty()) << "Dec 31 23:59:58 must pass";
    EXPECT_FALSE(n2.empty()) << "2sec=29 must pass";
}

/* ---- M-014 completion (DFX24): the UTF-16 name-validity table ------- */
/* The first landing left PTR_180045a00's table unimplemented on the
 * premise that its bytes were "not recoverable from the corpus". The
 * premise was wrong: og-codebase/exFATSearchDLL.dll ships the binary,
 * and the 256 bytes at .rdata RVA 0x45A00 (file offset 0x44600 via the
 * PE section map) are the table FUN_180003030 indexes with the HIGH
 * byte of any char whose LOW byte is zero (byte order pinned by the
 * raw disassembly at VA 0x180003030). These two tests prove the
 * recovered table behaves as the og's gate. */

TEST(ExfatEngine, NameUtf16TableInvalidCharRejected) {
    /* U+0100 (low byte 0, high byte 0x01 - the table's zero at index 1)
     * is invalid per FUN_180003030's table check. Fail-before: with the
     * table check absent (the first landing), this set was emitted. */
    auto img = build_exfat_image();
    const size_t ROOT = 25 * 512;
    const size_t AT = ROOT + 576;
    SetBuilder sb;
    sb.file_entry(0x20, EXFAT_TEST_TS);
    sb.stream_entry(0x01, 10, 90, 512);
    sb.name_entry(u"tab\u0100bad.dat"); /* 10 chars - U+0100 is ONE char */
    std::memcpy(img.data() + AT, sb.entries.data(), sb.entries.size());
    sb.finish_set(img, AT);
    ExfatScanStats stats;
    const auto names = m014_scan_names(std::move(img), &stats);
    EXPECT_EQ(names.end(), std::find(names.begin(), names.end(), "tab\xE1\x80\x80" "bad.dat"))
        << "U+0100 (table[0x01] == 0) must reject the set";
    EXPECT_GE(stats.sets_gated, 1u);
}

TEST(ExfatEngine, NameUtf16TableAllowedCharAccepted) {
    /* Boundary pin: U+1A00 (low byte 0, high byte 0x1A - table[0x1A] ==
     * 1) passes the same gate. Proves the check is the og's table, not
     * a blanket zero-low-byte reject (U+1A00 and U+0100 differ only in
     * the table's verdict). */
    auto img = build_exfat_image();
    const size_t ROOT = 25 * 512;
    const size_t AT = ROOT + 576;
    SetBuilder sb;
    sb.file_entry(0x20, EXFAT_TEST_TS);
    sb.stream_entry(0x01, 10, 90, 512);
    sb.name_entry(u"tab\u1A00ok.dat");
    std::memcpy(img.data() + AT, sb.entries.data(), sb.entries.size());
    sb.finish_set(img, AT);
    ExfatScanStats stats;
    const auto names = m014_scan_names(std::move(img), &stats);
    EXPECT_NE(names.end(), std::find(names.begin(), names.end(), "tab\xE1\xA8\x80" "ok.dat"))
        << "U+1A00 (table[0x1A] == 1) must survive the gate";
    EXPECT_EQ(stats.sets_gated, 0u);
}

/* ---- M-052 (diff-to-fix): the recycle-bin pairing mirrored on exFAT --- */

TEST(ExfatEngine, RecyclePairingMirrored) {
    /* M-052: the exFAT engine carries the same $I/$R pairing as FAT/NTFS
     * (the corpus documents the mirror - fat-exfat-engine-internals.md
     * :71). A $RECYCLE.BIN tree on the fixture volume: bin root @90,
     * SID dir @91, the $I v2 record @92->93 (544 B, two clusters), the
     * $R content @94. Fail-before: with the pairing absent, the $R
     * surfaces as the raw "$R7ABC12.docx" name with no original-name
     * restoration. */
    auto img = build_exfat_image();
    const size_t FAT = 24 * 512;
    put32v(img, FAT + 90 * 4, 0xFFFFFFFF); /* bin root */
    put32v(img, FAT + 91 * 4, 0xFFFFFFFF); /* SID dir */
    put32v(img, FAT + 92 * 4, 93);         /* $I: 544 B spans two clusters */
    put32v(img, FAT + 93 * 4, 0xFFFFFFFF);
    put32v(img, FAT + 94 * 4, 0xFFFFFFFF); /* $R content */

    const size_t ROOT = 25 * 512;
    size_t at = ROOT + 576; /* past the fixture's own 18 root entries */
    const auto plant_at = [&](size_t dst, u8 attr, u8 name_len, u32 fc, u64 dl,
                              const char16_t* nm) {
        SetBuilder sb;
        sb.file_entry(attr, EXFAT_TEST_TS);
        sb.stream_entry(0x01, name_len, fc, dl);
        sb.name_entry(nm);
        std::memcpy(img.data() + dst, sb.entries.data(), sb.entries.size());
        sb.finish_set(img, dst);
    };
    /* the bin root set lives in the VOLUME root; the SID subdir set
     * lives INSIDE the bin root's cluster (90); the $I/$R sets live
     * INSIDE the SID dir's cluster (91) */
    plant_at(at, 0x10 /*dir*/, 12, 90, 512, u"$RECYCLE.BIN");
    const size_t BIN = 25 * 512 + (90 - 2) * 512;
    plant_at(BIN, 0x10, 7, 91, 512, u"S-1-5-2");
    const size_t SID = 25 * 512 + (91 - 2) * 512;
    plant_at(SID + 0 * 32, 0x20, 8, 92, 544, u"$I7ABC12");
    plant_at(SID + 3 * 32, 0x20, 13, 94, 512, u"$R7ABC12.docx");

    /* the $I v2 record at cluster 92 (2 clusters, 544 bytes) */
    const size_t IREC = 25 * 512 + (92 - 2) * 512;
    const u64 want_time = 1740830400ULL * 10000000ULL + 116444736000000000ULL;
    put32v(img, IREC + 0, 2); /* version 2 (Vista+ fixed form) */
    for (int i = 0; i < 8; ++i)
        img[IREC + 8 + i] = u8((12345ull) >> (8 * i));
    for (int i = 0; i < 8; ++i)
        img[IREC + 16 + i] = u8(want_time >> (8 * i));
    put32v(img, IREC + 24, 26); /* pathlen */
    const char16_t* path = u"C:\\users\\test\\report.docx";
    for (int i = 0; path[i]; ++i)
        put16(img, IREC + 28 + i * 2, path[i]);
    std::memcpy(img.data() + 25 * 512 + (94 - 2) * 512, "RECYCLEDBYTES", 14);

    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"Found");
    ExfatScanFlags flags;
    ExfatScanStats stats;
    ASSERT_EQ(scan_exfat(&dev, flags, nullptr, nullptr, nullptr, tree, ra, nullptr, nullptr,
                         stats),
              OK);
    EXPECT_GE(stats.recycle_info, 1u);
    EXPECT_GE(stats.recycle_remapped, 1u);
    tree::FileNode* report = nullptr;
    bool raw_r = false, raw_i = false;
    for (auto* n : collect_all(tree)) {
        if (n->name_utf8() == "report.docx")
            report = n;
        if (n->name_utf8().rfind("$R7ABC12", 0) == 0)
            raw_r = true;
        if (n->name_utf8().rfind("$I7ABC12", 0) == 0)
            raw_i = true;
    }
    ASSERT_NE(report, nullptr) << "the $R must surface under its original name";
    EXPECT_FALSE(raw_r) << "the raw $R name must not appear";
    EXPECT_FALSE(raw_i) << "the $I metadata must never surface";
    EXPECT_EQ(report->props()->logical_size, 12345u);
    EXPECT_EQ(report->props()->times.modified, want_time);
    EXPECT_EQ(report->deleted_state(), tree::DeletedState::Deleted);
    ASSERT_FALSE(report->props()->extents.empty());
}
