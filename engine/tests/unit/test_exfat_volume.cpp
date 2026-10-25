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
    put32v(img, FAT + 70 * 4, 0xFFFFFFFF); /* subdir */

    /* ---- allocation bitmap at cluster 50 (sector 75) ---- */
    {
        const size_t BM = 25 * 512 + (50 - 2) * 512;
        auto setbit = [&](u32 c) { img[BM + c / 8] |= u8(1u << (c % 8)); };
        setbit(2);
        setbit(3); /* root (2-cluster chain) */
        setbit(10);
        setbit(11); /* photo.jpg */
        setbit(20);
        setbit(21);
        setbit(22); /* doc.txt */
        setbit(50); /* bitmap itself */
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
    /* 0x82 upcase (skipped by engine; present for realism) */
    {
        u8* e = img.data() + ROOT + at;
        std::memset(e, 0, 32);
        e[0] = 0x82;
        put32v(img, ROOT + at + 4, 0xDEADBEEF);
        put32v(img, ROOT + at + 20, 60);
        put64(img, ROOT + at + 24, 128);
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
    /* gone.dat: DELETED NoFatChain (cluster 30, 512B; bitmap free) */
    {
        SetBuilder sb;
        size_t set_at = ROOT + at;
        sb.file_entry_deleted(0x20, TS);
        sb.stream_entry_deleted(0x03, 8, 30, 512);
        sb.name_entry_deleted(u"gone.dat");
        std::memcpy(img.data() + set_at, sb.entries.data(), sb.entries.size());
        /* deleted sets: no checksum validation (InUse clear) */
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
        sb.file_entry(0x20, 0);
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
    sb.file_entry(0x20, 0);
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
    sb.file_entry_deleted(0x20, 0);
    sb.stream_entry_deleted(0x01, 10, 100, 1024); /* 2 clusters expected */
    sb.name_entry_deleted(u"zeroed.dat");
    std::memcpy(img.data() + AT, sb.entries.data(), sb.entries.size());
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
    fe(2, 0xFFFFFFFF);               /* root: single cluster */
    fe(20, 21);
    fe(21, 22);
    fe(22, 0xFFFFFFFF);             /* doc.txt 20-22 (window 0) */
    for (u32 c = 50; c < 114; ++c)   /* allocation bitmap chain 50..113 */
        fe(c, c + 1);
    fe(113, 0xFFFFFFFF);
    fe(0x40001, 0x40002);            /* far.dat - WINDOW 1 */
    fe(0x40002, 0xFFFFFFFF);
    fe(0x40005, 0x40006);            /* fardir - WINDOW 1, 2 clusters */
    fe(0x40006, 0xFFFFFFFF);

    /* allocation bitmap at cluster 50: bits for the used low clusters */
    {
        const size_t BM = clus_off(50);
        auto setbit = [&](u32 c) { img[BM + c / 8] |= u8(1u << (c % 8)); };
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
        const std::u16string pad1 = u"padding-file-with-a-really-long-name-number-one-aaaaa";  /* 53 -> 4 names -> 6 entries */
        const std::u16string pad2 = u"padding-file-with-a-really-long-name-number-two-bbbbb";  /* 53 -> 4 names -> 6 entries */
        const std::u16string pad3 = u"padding-file-three-cccc";  /* 24 -> 2 names -> 4 entries (6+6+4 = 16 exact) */
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
