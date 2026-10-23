/* Carving engine tests: grammar validation, specificity, dedup, hashing. */
#include <gtest/gtest.h>
#include "cr/carve/cr_carve.h"
#include "cr/storage/cr_device.h"
#include <cstring>
#include <vector>

using namespace cr;
using namespace cr::carve;

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
void putbe16(std::vector<u8>& v, size_t at, u16 x) {
    v[at] = u8(x >> 8);
    v[at + 1] = u8(x);
}
void put32v(std::vector<u8>& v, size_t at, u32 x) {
    v[at] = u8(x);
    v[at + 1] = u8(x >> 8);
    v[at + 2] = u8(x >> 16);
    v[at + 3] = u8(x >> 24);
}
void putbe32(std::vector<u8>& v, size_t at, u32 x) {
    v[at] = u8(x >> 24);
    v[at + 1] = u8(x >> 16);
    v[at + 2] = u8(x >> 8);
    v[at + 3] = u8(x);
}

/* a grammar-complete JPEG at `at` (padded with a COM segment to >= 512) */
size_t plant_jpeg(std::vector<u8>& img, size_t at) {
    img[at] = 0xFF;
    img[at + 1] = 0xD8; /* SOI */
    img[at + 2] = 0xFF;
    img[at + 3] = 0xE0; /* APP0 */
    putbe16(img, at + 4, 16);
    std::memcpy(img.data() + at + 6, "JFIF", 4);
    img[at + 10] = 1;
    img[at + 11] = 1;
    img[at + 12] = 0;
    img[at + 13] = 0;
    img[at + 14] = 1;
    img[at + 15] = 0;
    img[at + 16] = 1;
    img[at + 17] = 1;
    img[at + 18] = 0;
    img[at + 19] = 0; /* APP0 done: 2+2+16 */
    size_t p = at + 20;
    img[p] = 0xFF;
    img[p + 1] = 0xFE;        /* COM (comment) */
    putbe16(img, p + 2, 460); /* big payload */
    for (int i = 0; i < 456; ++i)
        img[p + 4 + i] = u8('A' + (i % 26));
    p += 2 + 460; /* SOF0 next */
    img[p] = 0xFF;
    img[p + 1] = 0xC0; /* SOF0 */
    putbe16(img, p + 2, 17);
    img[p + 4] = 8;
    putbe16(img, p + 5, 64);
    putbe16(img, p + 7, 48);
    img[p + 9] = 1;
    img[p + 10] = 1;
    img[p + 11] = 0x22;
    img[p + 12] = 0;
    p += 2 + 17;
    img[p] = 0xFF;
    img[p + 1] = 0xDA; /* SOS */
    putbe16(img, p + 2, 12);
    img[p + 4] = 1;
    img[p + 5] = 0;
    img[p + 6] = 0;
    img[p + 7] = 0;
    img[p + 8] = 0;
    img[p + 9] = 0;
    img[p + 10] = 0;
    img[p + 11] = 0;
    img[p + 12] = 0;
    img[p + 13] = 0;
    p += 2 + 12;
    img[p] = 0x00;
    img[p + 1] = 0x11;
    img[p + 2] = 0x22;
    img[p + 3] = 0x33;
    img[p + 4] = 0xFF;
    img[p + 5] = 0xD9;   /* EOI */
    return (p + 6) - at; /* 2+18+462+19+14+4+2 = 519 */
}

size_t plant_png(std::vector<u8>& img, size_t at) {
    const u8 magic[8] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
    std::memcpy(img.data() + at, magic, 8);
    /* IHDR: len@8 type@12 data 16..28 (13) crc 29..32 */
    putbe32(img, at + 8, 13);
    std::memcpy(img.data() + at + 12, "IHDR", 4);
    putbe32(img, at + 16, 4); /* width */
    putbe32(img, at + 20, 4); /* height */
    img[at + 24] = 8;
    img[at + 25] = 6;
    img[at + 26] = 0;
    img[at + 27] = 0;
    img[at + 28] = 0;                  /* 13th byte */
    putbe32(img, at + 29, 0xDEADBEEF); /* crc */
    /* IDAT: len@33 type@37 data 41..48 (8) crc 49..52 */
    putbe32(img, at + 33, 8);
    std::memcpy(img.data() + at + 37, "IDAT", 4);
    for (int i = 0; i < 8; ++i)
        img[at + 41 + i] = u8(i + 1);
    putbe32(img, at + 49, 0x11223344);
    /* IEND: len@53(0) type@57 crc 61..64 */
    putbe32(img, at + 53, 0);
    std::memcpy(img.data() + at + 57, "IEND", 4);
    putbe32(img, at + 61, 0xCAFEBABE);
    return 65;
}

size_t plant_zip(std::vector<u8>& img, size_t at, const char* name, size_t name_len) {
    /* local header 30 + name + no data content + central dir + EOCD */
    std::memcpy(img.data() + at, "PK\x03\x04", 4);
    put16(img, at + 4, 20);      /* version */
    put16(img, at + 6, 0);       /* flags */
    put16(img, at + 8, 0);       /* method */
    put16(img, at + 10, 0);      /* time */
    put16(img, at + 12, 0x5A46); /* date */
    put32v(img, at + 14, 0);     /* crc */
    put32v(img, at + 18, 0);     /* csize */
    put32v(img, at + 22, 0);     /* usize */
    put16(img, at + 26, u16(name_len));
    put16(img, at + 28, 0); /* extra */
    std::memcpy(img.data() + at + 30, name, name_len);
    size_t local_len = 30 + name_len;
    /* central directory */
    size_t cd_at = at + local_len;
    std::memcpy(img.data() + cd_at, "PK\x01\x02", 4);
    put16(img, cd_at + 4, 20);
    put16(img, cd_at + 6, 0);
    put16(img, cd_at + 8, 0);
    put16(img, cd_at + 10, 0);
    put16(img, cd_at + 12, 0x5A46);
    put32v(img, cd_at + 14, 0);
    put32v(img, cd_at + 18, 0);
    put32v(img, cd_at + 22, 0);
    put16(img, cd_at + 26, u16(name_len));
    put16(img, cd_at + 28, 0);
    std::memcpy(img.data() + cd_at + 46, name, name_len);
    size_t cd_len = 46 + name_len;
    /* EOCD */
    size_t eocd_at = cd_at + cd_len;
    std::memcpy(img.data() + eocd_at, "PK\x05\x06", 4);
    put16(img, eocd_at + 4, 0);
    put16(img, eocd_at + 6, 0);
    put16(img, eocd_at + 8, 1);
    put16(img, eocd_at + 10, 1);
    put32v(img, eocd_at + 12, u32(cd_len));    /* cd size */
    put32v(img, eocd_at + 16, u32(local_len)); /* cd offset (file-relative) */
    put16(img, eocd_at + 20, 0);               /* comment */
    return (eocd_at + 22) - at;
}

std::vector<u8> build_carve_image() {
    std::vector<u8> img(32 * 1024, 0);
    plant_jpeg(img, 512);
    plant_png(img, 2048);
    /* plain ZIP: the name is long enough that the archive clears the
     * registry's 128-byte min_size on its OWN extent (140 B). The old
     * last-EOCD-wins rule inflated this plant to 232 B by absorbing the
     * DOCX plant's EOCD - the test used to pass on that cross-file bug */
    plant_zip(img, 4096, "backup_archive_v1.txt", 21); /* plain ZIP: 140 B */
    plant_zip(img, 8192, "[Content_Types].xml", 19);   /* OOXML shape: 136 B */
    /* noise: JPEG magic, no EOI/SOF/SOS - planted clear of the real files
     * (the jpeg@512 spans 512..1033; keep noise away from its tail) */
    img[12288] = 0xFF;
    img[12289] = 0xD8;
    img[12290] = 0xFF;
    img[12291] = 0xE0;
    putbe16(img, 12292, 16);
    std::memcpy(img.data() + 12294, "JFIF", 4);
    return img;
}

} // namespace

TEST(CarveEngine, GrammarValidationAndSpecificity) {
    auto img = build_carve_image();
    MemDevice dev(std::move(img));

    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats stats;
    u32 rc =
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats);
    ASSERT_EQ(rc, OK) << err_text(rc);

    /* validated: jpg, png, zip, docx; noise rejected; no duplicates yet */
    EXPECT_EQ(stats.validated, 4u);
    EXPECT_GE(stats.rejected, 1u);
    EXPECT_EQ(stats.duplicates, 0u);
    EXPECT_TRUE(stats.bytes_scanned >= 32 * 1024 - 4096);

    /* collect and classify */
    std::vector<tree::FileNode*> all;
    std::function<void(tree::FileNode*)> collect = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            all.push_back(c);
            collect(c);
        }
    };
    collect(root);

    int jpg = 0, png = 0, zip = 0, docx = 0;
    tree::FileNode *jpg_node = nullptr, *docx_node = nullptr, *zip_node = nullptr;
    for (auto* n : all) {
        const u32 fmt = n->props()->carve_format;
        if (n->props()->extents.front().offset == 512) {
            jpg++;
            jpg_node = n;
        } else if (n->props()->extents.front().offset == 2048)
            png++;
        else if (n->props()->extents.front().offset == 8192) {
            docx++;
            docx_node = n;
        } else if (n->props()->extents.front().offset == 4096) {
            zip++;
            zip_node = n;
        } else if (n->props()->extents.front().offset == 12288) {
            /* the raw-sign layer 1 carves the JPEG noise plant as a jfif
             * candidate (corpus sign 0,ffd8ffe000104a464946) - the og's
             * layer-1 semantics: header signs accept what grammar rejects */
            EXPECT_GE(n->props()->carve_format, 0xFFFF0000u);
            EXPECT_EQ(n->props()->logical_size,
                      size_t(32768 - 12288)); /* jfif max, device-clamped */
        } else
            FAIL() << "unexpected carved node at " << n->props()->extents.front().offset;
        switch (fmt) {
        case FMT_JPG:
            EXPECT_EQ(n->name_utf8().substr(n->name_utf8().size() - 4), ".jpg");
            break;
        case FMT_PNG:
            EXPECT_EQ(n->name_utf8().substr(n->name_utf8().size() - 4), ".png");
            break;
        case FMT_DOCX:
            EXPECT_EQ(n->name_utf8().substr(n->name_utf8().size() - 5), ".docx");
            break;
        case FMT_ZIP:
            EXPECT_EQ(n->name_utf8().substr(n->name_utf8().size() - 4), ".zip");
            break;
        default:
            break;
        }
        EXPECT_TRUE(n->props()->hash_valid); /* all files under threshold */
    }
    EXPECT_EQ(jpg, 1);
    EXPECT_EQ(png, 1);
    EXPECT_EQ(zip, 1);
    EXPECT_EQ(docx, 1); /* the OOXML-shaped plant classified DOCX, not ZIP */

    /* sizes from grammar ends - exact extents, no cross-file absorption:
     * the ZIP ends at its OWN EOCD (first-consistent-wins), not at the
     * DOCX plant's EOCD 4 KB later */
    ASSERT_NE(jpg_node, nullptr);
    EXPECT_EQ(jpg_node->props()->logical_size, size_t(521));
    ASSERT_NE(zip_node, nullptr);
    EXPECT_EQ(zip_node->props()->logical_size, size_t(140)); /* 30+21 + 46+21 + 22 */
    ASSERT_NE(docx_node, nullptr);
    EXPECT_EQ(docx_node->props()->logical_size,
              size_t(30u + 19u + 46u + 19u + 22u)); /* 136: own EOCD */
}

TEST(CarveEngine, SecondPassFullyDeduped) {
    auto img = build_carve_image();
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats s1, s2;
    ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, s1),
              OK);
    const size_t nodes_after_p1 = tree.node_count();
    ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, s2),
              OK);
    EXPECT_EQ(s2.validated, 0u);
    /* THE F9 invariant: pass 2 adds zero nodes (every re-found candidate
     * - grammar and raw-sign alike - dedups against pass 1) */
    EXPECT_EQ(tree.node_count(), nodes_after_p1);
    EXPECT_GE(s2.duplicates, 4u);          /* at least the 4 grammar files */
    EXPECT_GT(s1.sign_hits, 0u);           /* the sign layer is live */
    EXPECT_EQ(s2.sign_hits, s1.sign_hits); /* same fixture, same probes */
}

TEST(CarveEngine, RegistryF8Gate) {
    std::vector<const Signature*> bad;
    EXPECT_EQ(validate_registry(&bad), 0u); /* the shipped table is clean */
    EXPECT_EQ(registry_size(), FMT_COUNT);
}

TEST(CarveEngine, PstAndRar5Grammar) {
    auto img = std::vector<u8>(2ull << 20, 0);
    /* --- Unicode PST: !BDN + MS + 0x17 + size@0xB8 + nonzero AMAP@0x1000 */
    memcpy(img.data() + 4096, "!BDN", 4);
    img[4096 + 4] = 0x4D; /* "MS" */
    img[4096 + 5] = 0x53;
    img[4096 + 0x0A] = 0x17;                       /* Unicode */
    put32v(img, 4096 + 0xB8, 128 * 1024);          /* size 128K */
    memset(img.data() + 4096 + 0x1000, 0xAA, 512); /* AMAP */

    /* --- RAR5: sig + MAIN + FILE + ENDARC with vints */
    size_t rar_at = 1ull << 20;
    memcpy(img.data() + rar_at, "Rar!\x1A\x07\x01\x00", 8);
    size_t p = rar_at + 8;
    auto vint = [&](u64 v) {
        while (v >= 0x80) {
            img[p++] = u8(v | 0x80);
            v >>= 7;
        }
        img[p++] = u8(v);
    };
    /* MAIN header: type 1, flags 0, hdr_size 4 (type+flags vints) */
    put32v(img, p, 0); /* crc placeholder */
    p += 4;
    const size_t hstart = p;
    vint(1); /* hdr_type MAIN */
    vint(0); /* flags */
    /* patch hdr_size into its own vint slot - it precedes the type; for
     * the test we re-emit: size vint 1 byte (value 2) covers type+flags */
    /* simpler: rewrite the block as [crc][size=2][type=1][flags=0] */
    p = hstart;
    vint(2); /* hdr_size = 2 bytes of header data follow */
    vint(1); /* type MAIN */
    vint(0); /* flags */
    /* FILE header */
    put32v(img, p, 0);
    p += 4;
    const size_t fh = p;
    vint(2); /* placeholder hdr_size patched below */
    const size_t fh_body = p;
    vint(2);                   /* type FILE */
    vint(2);                   /* flags: DATA_AREA */
    vint(10);                  /* data_size 10 */
    img[fh] = u8(p - fh_body); /* hdr_size vint (single byte value) */
    p += 10;                   /* data area */
    /* ENDARC */
    put32v(img, p, 0);
    p += 4;
    const size_t eh = p;
    vint(1); /* placeholder size */
    const size_t eh_body = p;
    vint(5); /* type ENDARC */
    vint(0); /* flags */
    img[eh] = u8(p - eh_body);
    const u64 rar_end = p;

    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    flags.hash_small_files = false;
    CarveStats stats;
    u32 rc =
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats);
    ASSERT_EQ(rc, OK) << err_text(rc);
    EXPECT_GE(stats.validated, 2u);

    int pst = 0, rar5 = 0;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling()) {
        if (!c->props())
            continue;
        const std::string nm = c->name_utf8();
        if (nm.find(".pst") != std::string::npos) {
            ++pst;
            EXPECT_EQ(4096u, c->props()->extents[0].offset);
            EXPECT_EQ(u64(128 * 1024), c->props()->extents[0].length);
        }
        if (nm.find(".rar5") != std::string::npos) {
            ++rar5;
            EXPECT_EQ(rar_at, c->props()->extents[0].offset);
            EXPECT_EQ(rar_end - rar_at, c->props()->extents[0].length);
        }
    }
    EXPECT_EQ(1, pst);
    EXPECT_EQ(1, rar5);

    /* RAR5 must NOT match RAR4 (7-byte magic differs at byte 6) - the
     * trial order tries RAR5 first; the RAR4 entry cannot also fire */
    int rar4 = 0;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
        if (c->props() && c->name_utf8().find(".rar") == 0)
            ++rar4;
    EXPECT_EQ(0, rar4);
}

TEST(CarveEngine, ExtendedFormatsGrammar) {
    auto img = std::vector<u8>(8ull << 20, 0);
    size_t cur = 4096;

    auto put = [&](const std::vector<u8>& v) {
        memcpy(img.data() + cur, v.data(), v.size());
        const size_t at = cur;
        cur += v.size() + 512; /* spacing */
        return at;
    };
    auto mk = [](std::initializer_list<u8> l) { return std::vector<u8>(l); };

    /* PSD: 8BPS + v1(BE) + 6 reserved + channels@12 + h@14 + w@18 +
     * depth@22 + mode@24 (all BE) */
    {
        std::vector<u8> v = mk({'8', 'B', 'P', 'S', 0, 1});
        v.insert(v.end(), 6, 0); /* reserved u32+u16 */
        v.push_back(0);
        v.push_back(1); /* channels BE @12 */
        for (u8 b : {u8(0), u8(0), u8(0), u8(64)})
            v.push_back(b); /* height BE @14 */
        for (u8 b : {u8(0), u8(0), u8(0), u8(48)})
            v.push_back(b); /* width BE @18 */
        v.push_back(0);
        v.push_back(8); /* depth BE @22 */
        v.push_back(0);
        v.push_back(1);           /* color mode @24 */
        v.insert(v.end(), 12, 0); /* 3 zero-size sections + compression */
        put(v);
    }
    /* MP3: ID3v2.3 + syncsafe size 100 + FOUR real MPEG1 Layer III frames.
     * Header FF FB 22 22: ver=MPEG1 layer=III, bitrate idx 2 (40 kbps),
     * samplerate idx 0 (44100), pad 1 -> 144*40*1000/44100+1 = 131 B/frame.
     * The strict walk stops at the first invalid header (AUDIT Wave-3);
     * 4 frames -> 10+100+524 = 634 B, extent == last frame end */
    {
        std::vector<u8> v = mk({'I', 'D', '3', 3, 0, 0});
        v.push_back(0);
        v.push_back(0); /* syncsafe size hi */
        v.push_back(0);
        v.push_back(100);             /* size 100 */
        v.insert(v.end(), 100, 0x11); /* tag payload */
        for (int fr = 0; fr < 4; ++fr) {
            v.push_back(0xFF);
            v.push_back(0xFB);            /* 11-bit sync + MPEG1 + Layer III */
            v.push_back(0x22);            /* bitrate 2, samplerate 0, padding 1 */
            v.push_back(0x22);            /* channel mode (not used by the size math) */
            v.insert(v.end(), 127, 0x33); /* frame body: 4+127 = 131 B */
        }
        put(v);
    }
    /* WAV: RIFF + len + WAVE + fmt */
    {
        std::vector<u8> v = mk({'R', 'I', 'F', 'F'});
        u32 len = 1000;
        v.push_back(u8(len));
        v.push_back(u8(len >> 8));
        v.push_back(u8(len >> 16));
        v.push_back(u8(len >> 24));
        const char* w = "WAVEfmt ";
        for (const char* p = w; *p; ++p)
            v.push_back(u8(*p));
        v.insert(v.end(), 64, 0x33);
        put(v);
    }
    /* ASF: GUID + object size + count */
    {
        std::vector<u8> v = {0x30, 0x26, 0xB2, 0x75, 0x8E, 0x66, 0xCF, 0x11,
                             0xA6, 0xD9, 0x00, 0xAA, 0x00, 0x62, 0xCE, 0x6C};
        const u64 sz = 2048;
        for (int i = 0; i < 8; ++i)
            v.push_back(u8(sz >> (8 * i)));
        const u32 cnt = 5;
        for (int i = 0; i < 4; ++i)
            v.push_back(u8(cnt >> (8 * i)));
        v.insert(v.end(), 512, 0x44);
        put(v);
    }
    /* FLV: FLV + v1 + flags 5 + offset 9 + one tag */
    {
        std::vector<u8> v = mk({'F', 'L', 'V', 1, 5, 0, 0, 0, 9});
        v.insert(v.end(), 4, 0); /* PreviousTagSize0 */
        v.push_back(8);          /* tag type audio */
        v.push_back(0);
        v.push_back(0);
        v.push_back(16); /* size 16 */
        v.insert(v.end(), 7, 0);
        v.insert(v.end(), 16, 0x55); /* payload */
        v.insert(v.end(), 4, 0);     /* prev tag size */
        put(v);
    }
    /* CAB: CFHEADER - MSCF@0, reserved@4, cbCabinet@8, reserved@12,
     * coffFiles@16, reserved@20, verMinor@24 verMajor@25, cFolders@26,
     * cFiles@28, flags@30, setID@32 */
    {
        std::vector<u8> v = mk({'M', 'S', 'C', 'F'});
        v.insert(v.end(), 4, 0); /* reserved1 @4 */
        u32 cb = 4096;
        v.push_back(u8(cb));
        v.push_back(u8(cb >> 8));
        v.push_back(u8(cb >> 16));
        v.push_back(u8(cb >> 24)); /* cbCabinet @8 */
        v.insert(v.end(), 4, 0);   /* reserved2 @12 */
        v.push_back(0x24);
        v.push_back(0);
        v.push_back(0);
        v.push_back(0);          /* coffFiles @16 */
        v.insert(v.end(), 4, 0); /* reserved3 @20 */
        v.push_back(3);          /* versionMinor @24 */
        v.push_back(1);          /* versionMajor @25 */
        v.push_back(2);
        v.push_back(0); /* cFolders @26 */
        v.push_back(10);
        v.push_back(0); /* cFiles @28 */
        v.push_back(0);
        v.push_back(0); /* flags @30 */
        v.push_back(1);
        v.push_back(0); /* setID @32 */
        v.push_back(0);
        v.push_back(0); /* iCabinet @34 */
        v.insert(v.end(), 512, 0x66);
        put(v);
    }
    /* ORF: IIRO + IFD */
    {
        std::vector<u8> v = mk({'I', 'I', 'R', 'O', 8, 0, 0, 0});
        v.push_back(1);
        v.push_back(0); /* 1 IFD entry */
        /* entry: tag 0x0100, type 4 (LONG), count 100 (non-inline), value
         * = data offset 1000 -> extent math drives end past min_size */
        v.push_back(0x00);
        v.push_back(0x01);
        v.push_back(4);
        v.push_back(0);
        v.insert(v.end(), {100, 0, 0, 0});
        v.insert(v.end(), {0xE8, 3, 0, 0}); /* value = 1000 LE */
        v.push_back(0);
        v.push_back(0);
        v.push_back(0);
        v.push_back(0); /* next */
        v.insert(v.end(), 2048, 0x77);
        put(v);
    }
    /* ISO: PVD at a 32K-aligned position; the carved file starts 32K
     * earlier (header_offset 0x8001); blocks=64 -> 128 KiB volume */
    {
        cur = (cur + 0x7FFF) & ~size_t(0x7FFF); /* align to 32K */
        const size_t pvd = cur;
        img[pvd] = 1; /* type primary */
        memcpy(img.data() + pvd + 1, "CD001", 5);
        img[pvd + 6] = 1; /* version */
        /* volume space size LE u32 @+80: 4 blocks */
        u32 blocks = 64; /* 128 KiB volume */
        img[pvd + 80] = u8(blocks);
        img[pvd + 81] = u8(blocks >> 8);
        img[pvd + 82] = u8(blocks >> 16);
        img[pvd + 83] = u8(blocks >> 24);
        memset(img.data() + pvd + 0x54, 0x20, 32); /* system id padding */
        cur = pvd + 512;
        (void)put(mk({0}));
        /* remember pvd for assertions: place at fixed math below */
    }

    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"L");
    CarveFlags flags;
    flags.hash_small_files = false;
    CarveStats stats;
    u32 rc =
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats);
    ASSERT_EQ(rc, OK) << err_text(rc);

    std::set<std::string> found;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
        if (c->props()) {
            found.insert(c->name_utf8());
        }
    /* each extended format must validate */
    EXPECT_TRUE(found.count("File000000.psd"));
    EXPECT_TRUE(found.count("File000001.mp3"));
    EXPECT_TRUE(found.count("File000002.wav"));
    EXPECT_TRUE(found.count("File000003.wmv"));
    EXPECT_TRUE(found.count("File000004.flv"));
    EXPECT_TRUE(found.count("File000005.cab"));
    EXPECT_TRUE(found.count("File000006.orf"));
    /* ISO detected via its late magic */
    bool iso_found = false;
    for (const auto& f : found)
        if (f.find(".iso") != std::string::npos)
            iso_found = true;
    EXPECT_TRUE(iso_found);
    /* negative: the RIFF blob must be WAV, not WEBP/AVI (form-type checked) */
    for (const auto& f : found) {
        EXPECT_EQ(std::string::npos, f.find(".webp"));
        EXPECT_EQ(std::string::npos, f.find(".avi"));
    }
    /* MP3 exact extent: the strict frame walk ended at the last valid
     * frame - 10 (ID3 header) + 100 (tag) + 4*131 (frames) = 634 B */
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling()) {
        if (c->props() && c->name_utf8().find(".mp3") != std::string::npos) {
            EXPECT_EQ(u64(634), c->props()->extents[0].length);
        }
    }
}

/* P1-12 + AUDIT Wave-3 semantics: PDF %%EOF last-wins, single-frame MP3
 * rejected, and the amplified forward rescans replaced by 1 MiB blocks. */
TEST(CarveEngine, PdfEofLastWinsAndFrameFloors) {
    auto img = std::vector<u8>(2ull << 20, 0);

    /* PDF with TWO %%EOF markers (incremental-update shape): the LAST
     * one wins - extent = last marker + 5 */
    const size_t pdf1 = 4096;
    memcpy(img.data() + pdf1, "%PDF-1.4\n", 9);
    memcpy(img.data() + pdf1 + 100, "%%EOF", 5); /* superseded revision */
    memcpy(img.data() + pdf1 + 200, "%%EOF", 5); /* final revision end */
    /* 205 >= registry min 128 */

    /* %PDF with NO %%EOF anywhere after it: must be rejected */
    const size_t pdf2 = 8192;
    memcpy(img.data() + pdf2, "%PDF-1.5\n", 9);

    /* MP3 with a SINGLE valid frame: below the 2-frame floor (AUDIT
     * Wave-3) - must be rejected */
    const size_t mp3_one = 12288;
    {
        u8* p = img.data() + mp3_one;
        memcpy(p, "ID3", 3);
        p[3] = 3; /* v2.3 */
        p[4] = 0;
        p[5] = 0; /* flags */
        p[6] = 0;
        p[7] = 0;
        p[8] = 0;
        p[9] = 100; /* syncsafe size 100 */
        memset(p + 10, 0x11, 100);
        /* one real frame: FF FB 22 22 (40 kbps 44100 Hz pad 1 -> 131 B) */
        p[110] = 0xFF;
        p[111] = 0xFB;
        p[112] = 0x22;
        p[113] = 0x22;
        memset(p + 114, 0x33, 127);
        /* the next 4 bytes are zeros: walk stops, nframes = 1 < 2 */
    }

    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"L");
    CarveFlags flags;
    flags.hash_small_files = false;
    CarveStats stats;
    const u32 rc =
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats);
    ASSERT_EQ(rc, OK) << err_text(rc);

    int pdfs = 0;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling()) {
        if (!c->props())
            continue;
        if (c->name_utf8().find(".pdf") == std::string::npos)
            continue;
        ++pdfs;
        ASSERT_EQ(u64(pdf1), c->props()->extents[0].offset);
        EXPECT_EQ(u64(205), c->props()->extents[0].length);
    }
    EXPECT_EQ(1, pdfs); /* pdf2 (no %%EOF) and the 1-frame MP3 rejected */
}

TEST(CarveEngine, RawSignLayerHeaderAndTail) {
    /* header-only format: the corpus "sample" sign (#!/bin/sh) */
    std::vector<u8> img(512 * 1024, 0);
    std::memcpy(img.data() + 4096, "#!/bin/sh\n", 10);
    /* header+tail family: the 46 4c 68 64 magic is SHARED by flp/fst in
     * the og's table - the best-confidence (tail-paired) sign wins, which
     * for this plant is fst (its tail sign matches the planted region) */
    std::memcpy(img.data() + 8192, "\x46\x4c\x68\x64\x06\x00\x00\x00\x00\x00", 10);
    std::memcpy(img.data() + 8192 + 10240 + 512 + 17, "\x5c\x44\x00\x00\x00\x00", 6);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats s;
    ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, s),
              OK);
    EXPECT_GE(s.sign_hits, 2u);
    tree::FileNode* sample_n = nullptr;
    tree::FileNode* tail_n = nullptr;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling()) {
        const std::string nm = c->name_utf8();
        if (nm.size() > 7 && nm.compare(nm.size() - 7, 7, ".sample") == 0)
            sample_n = c;
        if (nm.size() > 4 && nm.compare(nm.size() - 4, 4, ".fst") == 0)
            tail_n = c;
    }
    ASSERT_NE(sample_n, nullptr) << "header-only sign hit missing";
    ASSERT_NE(tail_n, nullptr) << "header+tail sign hit missing";
    /* header-only: extent = the format max (51200) */
    EXPECT_EQ(sample_n->props()->extents.front().offset, u64(4096));
    EXPECT_EQ(sample_n->props()->logical_size, u64(51200));
    EXPECT_GE(sample_n->props()->carve_confidence, 500);
    /* tail-paired: the tail sector closes the file */
    EXPECT_EQ(tail_n->props()->extents.front().offset, u64(8192));
    EXPECT_EQ(tail_n->props()->logical_size, u64(10240 + 512 + 512));
    EXPECT_EQ(tail_n->confidence(), tree::Confidence::PartialRisk);
    EXPECT_GE(tail_n->props()->carve_confidence, 700); /* paired bonus */
}
