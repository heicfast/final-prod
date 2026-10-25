/* Carving engine tests: grammar validation, specificity, dedup, hashing. */
#include <gtest/gtest.h>
#include "cr/carve/cr_carve.h"
#include "cr/query/cr_query.h"
#include "cr/carve/cr_rawsigns.h"
#include "cr/storage/cr_device.h"
#include "cr/storage/cr_slice.h"
#include "cr/session/cr_records.h"
#include <cstring>
#include <ctime>
#include <functional>
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
    /* real central-directory field layout (the CD-anchor validator reads
     * nameLen@+28/extraLen@+30/commentLen@+32/localOffset@+42 - the old
     * plant put nameLen at +26 and left +30..+34 to the zero background,
     * which the 0xAA noise fill now exposes) */
    put16(img, cd_at + 26, 0);             /* (was name_len - wrong field) */
    put16(img, cd_at + 28, u16(name_len)); /* nameLen */
    put16(img, cd_at + 30, 0);             /* extraLen */
    put16(img, cd_at + 32, 0);             /* commentLen */
    put16(img, cd_at + 34, 0);             /* disk start */
    put16(img, cd_at + 36, 0);             /* internal attrs */
    put32v(img, cd_at + 38, 0);            /* external attrs */
    put32v(img, cd_at + 42, 0);            /* local header offset */
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
    /* CARVE-2: the background is NON-ZERO noise - the og's zero-padding
     * end extension (trailing all-zero 16-byte blocks absorbed, 5.24 MB
     * cap) must not eat the gaps between plants on this fixture. */
    /* S-015: 256 KiB - the jfif raw-sign noise plant needs >= 100 KiB of
     * region to clear its KiB-scaled min floor (pre-fix bytes-units made
     * 32 KiB sufficient; the og's true floor is 100 KiB). */
    std::vector<u8> img(256 * 1024, 0xAA);
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
        /* S-032: zero-data inner entries are legitimately extent-less
         * (metadata-only inventory children) - skip them in the offset
         * classification below */
        if (n->props()->extents.empty())
            continue;
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
            /* S-015: jfif max is 51200 KiB (50 MiB), device-clamped to the
             * 256 KiB fixture - NOT the pre-fix 51200-byte stub */
            EXPECT_EQ(n->props()->logical_size,
                      size_t(256 * 1024 - 12288)); /* jfif max, device-clamped */
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

TEST(CarveEngine, CarvesMidSectorStart) {
    /* I-009 pin (PARITY-DELTAS D-59c): byte-granular probing is a
     * deliberate detection superset over the og's sector-aligned dispatch
     * (RAWSearchDLL :9496 advances in whole sectors; a header starting
     * mid-sector is invisible to the og). A valid PNG planted at
     * sector+137 must surface here so the M-025 read-model rework can
     * never silently regress the superset by adopting sector stepping. */
    std::vector<u8> img(64 * 1024, 0xAA);
    const size_t at = 512 + 137; /* sector 1, byte 137 */
    plant_png(img, at);
    MemDevice dev(std::move(img));

    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats stats;
    u32 rc =
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats);
    ASSERT_EQ(rc, OK) << err_text(rc);

    std::vector<tree::FileNode*> all;
    std::function<void(tree::FileNode*)> collect = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = n->next_sibling()) {
            all.push_back(c);
            collect(c);
        }
    };
    collect(root);
    ASSERT_EQ(all.size(), 1u) << "exactly one carve on this fixture";
    EXPECT_EQ(all[0]->props()->extents.front().offset, at)
        << "mid-sector start (sector+137) must be carved - the byte-granularity "
           "superset (I-009 / D-59c) regressed";
    EXPECT_EQ(all[0]->props()->carve_format, FMT_PNG);
    EXPECT_EQ(all[0]->props()->logical_size, size_t(65));
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

namespace {
/* S-015: find a compiled raw-sign format row by extension. */
const rawsigns::RawSignFormat* find_format(const char* ext) {
    for (u32 i = 0; i < rawsigns::kFormatCount; ++i)
        if (std::strcmp(rawsigns::kFormats[i].ext, ext) == 0)
            return &rawsigns::kFormats[i];
    return nullptr;
}
} // namespace

TEST(RawSigns, UnitsAreKib) {
    /* S-015: the corpus raw_signs.json min/max fields are KiB counts in
     * the og (FUN_1800205b0: gate max*0x400, size max<<10, tail floor
     * min<<10, default 0x7d000 KiB); the compiled table carries BYTES.
     * Before the fix the table carried the raw KiB integers as bytes -
     * every format window was 1024x too small and the floor 1024x too
     * low. */
    const auto* vob = find_format("vob");
    const auto* flac = find_format("flac");
    const auto* sample = find_format("sample");
    ASSERT_NE(vob, nullptr);
    ASSERT_NE(flac, nullptr);
    ASSERT_NE(sample, nullptr);
    EXPECT_EQ(vob->min_size, u64(10240) * 1024);  /* 10 MiB */
    EXPECT_EQ(vob->max_size, u64(204800) * 1024); /* 200 MiB */
    EXPECT_EQ(flac->min_size, u64(1024) * 1024);  /* 1 MiB */
    EXPECT_EQ(flac->max_size, u64(51200) * 1024); /* 50 MiB */
    EXPECT_EQ(sample->min_size, u64(1024));       /* 1 KiB */
    EXPECT_EQ(sample->max_size, u64(51200) * 1024);
}

TEST(CarveEngine, RawSignLayerHeaderOnlyAndSharedMagic) {
    /* S-015 re-based to the decoded KiB units + per-format tail scoping.
     * (a) header-only: corpus "sample" (#!/bin/sh), min 1 KiB / max
     * 50 MiB -> in a 4 MiB image the extent clamps to the device end.
     * (b) the 46 4c 68 64 magic is SHARED by flp/fst in the og's table;
     * the best-confidence header sign is fst's 4,06000000 @ 0.63, and fst
     * carries NO tail signs - the og pairs tails within the candidate's
     * own format record, so fst emits header-only even with flp's tail
     * planted in range. The pre-S-015 cross-format pairing (a .fst node
     * closed by flp's tail, and a .sample node closed the same way) is the
     * divergence this pins away. */
    std::vector<u8> img(4 * 1024 * 1024, 0);
    std::memcpy(img.data() + 4096, "#!/bin/sh\n", 10);
    const u64 start = 8192;
    /* flp's own header (10 bytes) so flp's tail really is in-format range;
     * fst's shorter signs still win the best-confidence pick */
    std::memcpy(img.data() + start, "\x46\x4c\x68\x64\x06\x00\x00\x00\x00\x00", 10);
    std::memcpy(img.data() + start + 1024 * 1024 + 2 * 1024 * 1024 + 17, "\x5c\x44\x00\x00\x00\x00",
                6); /* flp tail sign, off 17 (decimal) */
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats s;
    ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, s),
              OK);
    /* F-A3-6 regression: exact count (was doubled by the copy-paste
     * duplicate accumulation lines) */
    EXPECT_EQ(s.sign_hits, 2u);
    /* NO cross-format pairing: the sample/fst candidates own no tail
     * signs, so nothing pairs with flp's tail */
    EXPECT_EQ(s.sign_validated, 0u);
    tree::FileNode* sample_n = nullptr;
    tree::FileNode* fst_n = nullptr;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling()) {
        const std::string nm = c->name_utf8();
        if (nm.size() > 7 && nm.compare(nm.size() - 7, 7, ".sample") == 0)
            sample_n = c;
        if (nm.size() > 4 && nm.compare(nm.size() - 4, 4, ".fst") == 0)
            fst_n = c;
    }
    ASSERT_NE(sample_n, nullptr) << "header-only sign hit missing";
    ASSERT_NE(fst_n, nullptr) << "shared-magic header hit missing";
    /* header-only: extent = the format max (50 MiB) clamped to device end */
    EXPECT_EQ(sample_n->props()->extents.front().offset, u64(4096));
    EXPECT_EQ(sample_n->props()->logical_size, u64(4 * 1024 * 1024 - 4096));
    EXPECT_GE(sample_n->props()->carve_confidence, 500);
    /* fst: header-only at its own max (50 MiB), NOT closed by flp's tail */
    EXPECT_EQ(fst_n->props()->extents.front().offset, start);
    EXPECT_EQ(fst_n->props()->logical_size, u64(4 * 1024 * 1024 - start));
    EXPECT_EQ(fst_n->confidence(), tree::Confidence::MetadataOnly); /* header-only raw-sign grade */
    EXPECT_EQ(fst_n->props()->carve_confidence, 630);
}

TEST(CarveEngine, RawSignTailPairWithinFormat) {
    /* S-015: sqlite (min 1 KiB, max 50 MiB) carries its own tail sign
     * (offset 31 DECIMAL, conf 1.00) - the far tail pairs within the
     * format and closes the file at the tail sector. Tail sits ~2 MiB past
     * the window start: far beyond the pre-fix 50 KiB window, inside the
     * chunked 8 MiB walk. */
    std::vector<u8> img(4 * 1024 * 1024, 0);
    const u64 start = 4096;
    const u64 lo = 1024;                                /* sqlite min, KiB-scaled */
    const u64 tail_stride = 2 * 1024 * 1024;            /* 512-aligned stride past lo */
    std::memcpy(img.data() + start, "SQLite form", 11); /* sqlite header sign */
    std::memcpy(img.data() + start + lo + tail_stride + 31,
                "\x11\x1d\x05\x15\x19\x0f\x0f\x77\x6f\x72\x64", 11); /* sqlite tail */
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats s;
    ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, s),
              OK);
    EXPECT_GE(s.sign_validated, 1u);
    tree::FileNode* sql_n = nullptr;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling()) {
        const std::string nm = c->name_utf8();
        if (nm.size() > 7 && nm.compare(nm.size() - 7, 7, ".sqlite") == 0)
            sql_n = c;
    }
    ASSERT_NE(sql_n, nullptr) << "sqlite raw-sign hit missing";
    EXPECT_EQ(sql_n->props()->logical_size, lo + tail_stride + 512);
    EXPECT_GE(sql_n->props()->carve_confidence, 700); /* (920+1000)/2+100 capped 1000 */
}

TEST(RawSigns, CarvesFiveMiBVob) {
    /* S-015 recall proof: a vob header sign in unallocated space gets a
     * 200 MiB-class extent (clamped to the region), not the pre-fix
     * 200 KiB stub extent. The region must also exceed vob's 10 MiB
     * floor (min<<10 acceptance) - a smaller region is rejected on BOTH
     * sides (pinned separately by the og's tail-acceptance gate). */
    std::vector<u8> img(12 * 1024 * 1024, 0);
    const u64 start = 6144;                                      /* 512-aligned stride */
    std::memcpy(img.data() + start + 10, "\x01\x89\xc3\xf8", 4); /* vob hdr sign */
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats s;
    ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, s),
              OK);
    tree::FileNode* vob_n = nullptr;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling()) {
        const std::string nm = c->name_utf8();
        if (nm.size() > 4 && nm.compare(nm.size() - 4, 4, ".vob") == 0)
            vob_n = c;
    }
    ASSERT_NE(vob_n, nullptr) << "vob raw-sign hit missing";
    EXPECT_EQ(vob_n->props()->extents.front().offset, start);
    /* extent = max_size (200 MiB) clamped to device end: the whole region,
     * not the pre-fix 204800-byte stub */
    EXPECT_EQ(vob_n->props()->logical_size, u64(12 * 1024 * 1024) - start);
}

TEST(RawSigns, MinFloorOneKib) {
    /* S-015 false-positive floor: formats with min=1 KiB reject
     * sub-KiB remnants (device end closer than 1 KiB from the hit);
     * the pre-fix byte-unit floor admitted them. */
    std::vector<u8> img(64 * 1024, 0);
    /* 512-aligned hit with only 384 bytes left: below the 1 KiB floor */
    std::memcpy(img.data() + 127 * 512, "#!/bin/sh\n", 10); /* sample: min 1 KiB */
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats s;
    ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, s),
              OK);
    EXPECT_EQ(s.sign_hits, 0u); /* 400 bytes < the 1 KiB floor: rejected */
    bool any = false;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
        any = true;
    EXPECT_FALSE(any);
}

TEST(RawSigns, TailPairWindowScaled) {
    /* S-015 tail window: apk (min 1 KiB, max 50 MiB) with its EOCD tail
     * sign ~2 MiB past the header - far beyond the pre-fix 50 KiB window,
     * inside the chunked 8 MiB walk. Pairing confirms the close. The tail
     * sign is planted lo-relative (the walk starts at start+min), with the
     * pattern at a 512-aligned stride + the sign's own 0x28 offset. */
    std::vector<u8> img(4 * 1024 * 1024, 0);
    const u64 start = 4096;
    const u64 lo = 1024;                              /* apk min, KiB-scaled */
    const u64 tail_stride = 2 * 1024 * 1024;          /* 512-aligned */
    std::memcpy(img.data() + start, "PK\x03\x04", 4); /* apk header */
    /* apk tail sign: offset 28 DECIMAL (corpus offsets are decimal),
     * bytes 50 4b 05 06 00 00 00 00 */
    std::memcpy(img.data() + start + lo + tail_stride + 28, "\x50\x4b\x05\x06\x00\x00\x00\x00", 8);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats s;
    ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, s),
              OK);
    EXPECT_GE(s.sign_validated, 1u);
    tree::FileNode* apk_n = nullptr;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling()) {
        const std::string nm = c->name_utf8();
        if (nm.size() > 4 && nm.compare(nm.size() - 4, 4, ".apk") == 0)
            apk_n = c;
    }
    ASSERT_NE(apk_n, nullptr) << "apk raw-sign hit missing";
    EXPECT_EQ(apk_n->props()->logical_size, lo + tail_stride + 512);
    EXPECT_GE(apk_n->props()->carve_confidence, 700); /* paired bonus */
}

/* FB-C01 (AUDIT-FALLBACKS): an unreadable window is SKIPPED and its bytes
 * must NOT be counted as scanned - bytes_scanned is the honest coverage
 * contract (G-05); the old code claimed coverage of bytes it never read. */
namespace {
class PartialFailDevice final : public IStorageDevice {
public:
    PartialFailDevice(std::vector<u8> b, u64 bad_start, u64 bad_len)
        : data_(std::move(b)), bad_start_(bad_start), bad_len_(bad_len) {
        geom_.logical_sector = 512;
        geom_.physical_sector = 512;
        geom_.sector_count = data_.size() / 512;
        geom_.total_bytes = geom_.sector_count * 512;
    }
    u32 read_at(u64 off, void* buf, u32 len) override {
        if (off > data_.size() || len > data_.size() - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        /* any read touching the bad range fails (post-retry state) */
        if (off < bad_start_ + bad_len_ && off + len > bad_start_)
            return ERR_READ_FAILED;
        std::memcpy(buf, data_.data() + off, len);
        return OK;
    }
    const Geometry& geometry() const override { return geom_; }

private:
    std::vector<u8> data_;
    u64 bad_start_, bad_len_;
    Geometry geom_;
};
} // namespace

TEST(CarveEngine, UnreadableWindowNotCountedAsScanned) {
    /* 3 MiB device; the middle window unreadable EXCLUDING the 4 KiB
     * overlap tail window 0 reads (so window 0 itself stays readable) */
    std::vector<u8> img(3 * 1024 * 1024, 0);
    plant_jpeg(img, 512); /* live content in window 0 */
    const u64 bad_start = (1ull << 20) + 4096;
    PartialFailDevice dev(std::move(img), bad_start, (1ull << 20) - 4096);

    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats s;
    ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, s),
              OK);
    /* window 1 skipped; windows 0 and 2 fully counted (1 MiB each - the
     * OVERLAP re-read is excluded by the own-range attribution) */
    EXPECT_EQ(s.windows_skipped, 1u);
    EXPECT_EQ(s.bytes_scanned, 2u * (1ull << 20));
    /* the readable window's content still carved */
    bool saw_jpg = false;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling()) {
        if (c->props()->extents.front().offset == 512)
            saw_jpg = true;
    }
    EXPECT_TRUE(saw_jpg);
}

/* ---- AUDIT-PIPELINE carve batch regressions (CARVE-2/3/4/6) ---- */

TEST(CarveEngine, ZipCdAnchorCarvesDestroyedHead) {
    /* CARVE-2: a planted archive whose LOCAL header is destroyed but whose
     * central directory + EOCD survive must carve from the CD anchor (the
     * og's preferred anchor; before, only the LFH anchor existed). */
    auto img = build_carve_image();
    /* destroy the plain ZIP's head: wipe its local header + name (51 B) */
    std::memset(img.data() + 4096, 0xAA, 51);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats s;
    ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, s),
              OK);
    bool cd_carved = false;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling()) {
        if (c->props() && c->props()->carve_format == FMT_ZIP_CD &&
            c->props()->extents.front().offset > 4090 && c->props()->extents.front().offset < 4160)
            cd_carved = true;
    }
    EXPECT_TRUE(cd_carved) << "destroyed-head archive must carve from its CD";
}

TEST(CarveEngine, ZipIndexlessLfhChainSalvage) {
    /* CARVE-2: CD + EOCD destroyed - the LFH chain still carves (og
     * analyser status-0xd salvage), low confidence, fragmented flag. */
    auto img = build_carve_image();
    /* wipe the plain ZIP's central directory + EOCD (from 4096+51 to end) */
    std::memset(img.data() + 4096 + 51, 0xAA, 140 - 51);
    /* give the entry a real compressed payload so the chain is real AND
     * clears the registry's 128-byte min_size floor: csize=256 */
    put32v(img, 4096 + 18, 256); /* csize */
    for (int i = 0; i < 256; ++i)
        img[4096 + 51 + i] = u8('z');
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats s;
    ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, s),
              OK);
    tree::FileNode* zip = nullptr;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling()) {
        if (c->props() && c->props()->extents.front().offset == 4096)
            zip = c;
    }
    ASSERT_NE(zip, nullptr) << "indexless archive must still carve";
    /* chain end = 30 + 21 name + 256 data = 307 bytes */
    EXPECT_EQ(zip->props()->logical_size, size_t(30 + 21 + 256));
    EXPECT_LE(zip->props()->carve_confidence, 700u); /* low confidence */
}

TEST(CarveEngine, ZipZeroPaddingAbsorbed) {
    /* CARVE-2: the og's last-16-bytes-zero test extends the archive over
     * trailing zero padding (5.24 MB cap) */
    auto img = build_carve_image();
    /* 128 bytes of zero padding right after the plain ZIP's EOCD */
    std::memset(img.data() + 4096 + 140, 0, 128);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats s;
    ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, s),
              OK);
    tree::FileNode* zip = nullptr;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling()) {
        if (c->props() && c->props()->extents.front().offset == 4096 &&
            c->props()->carve_format == FMT_ZIP)
            zip = c;
    }
    ASSERT_NE(zip, nullptr);
    EXPECT_EQ(zip->props()->logical_size, size_t(140 + 128)); /* padding absorbed */
}

TEST(CarveEngine, BeTiffAndOrfVariantsCarve) {
    /* CARVE-3: MM\0\* (BE TIFF), IIRS and MMOR (ORF) anchors are in the
     * registry now (og seeds 2a004d4d / 53524949 / 524f4d4d) - the
     * tiff_common BE walk was dead code before. */
    auto img = build_carve_image();
    auto w16 = [](u8* p, u16 v, bool be) {
        if (be) {
            p[0] = u8(v >> 8);
            p[1] = u8(v);
        } else {
            p[0] = u8(v);
            p[1] = u8(v >> 8);
        }
    };
    auto w32 = [](u8* p, u32 v, bool be) {
        for (int i = 0; i < 4; ++i)
            p[be ? 3 - i : i] = u8(v >> (8 * i));
    };
    /* one-entry IFD whose value data sits at +1200 (clears the ORF rows'
     * 1024-byte min_size floor): entry count 1, tag 0x0100, type 3
     * (SHORT), count 5 -> 10 bytes of data at offset 1200, next-IFD 0 */
    auto plant_ifd = [&](size_t at, const char magic[4], bool be) {
        u8* t = img.data() + at;
        std::memcpy(t, magic, 4);
        w16(t + 4, 8, be); /* IFD offset */
        w16(t + 8, 1, be); /* 1 entry */
        w16(t + 10, 0x0100, be);
        w16(t + 12, 3, be);
        w32(t + 14, 5, be);    /* count > 4: data at the value offset */
        w32(t + 18, 1200, be); /* value = data offset */
        w32(t + 22, 0, be);    /* next IFD */
        for (int i = 0; i < 10; ++i)
            t[1200 + i] = 0x11; /* the referenced data */
    };
    plant_ifd(16384, "IIRS", false); /* fields only; magic set below (BE) */
    img[16384 + 2] = 0x00;
    img[16384 + 3] = 0x2A; /* MM\0\x2A - the BE TIFF magic */
    img[16384 + 0] = 'M';
    img[16384 + 1] = 'M';
    /* rewrite the fields BE (plant_ifd wrote them LE above) */
    w16(img.data() + 16384 + 4, 8, true);
    w16(img.data() + 16384 + 8, 1, true);
    w16(img.data() + 16384 + 10, 0x0100, true);
    w16(img.data() + 16384 + 12, 3, true);
    w32(img.data() + 16384 + 14, 4, true);
    w32(img.data() + 16384 + 18, 0, true);
    w32(img.data() + 16384 + 22, 0, true);
    plant_ifd(20480, "IIRS", false);
    plant_ifd(24576, "MMOR", true);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats s;
    ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, s),
              OK);
    bool be_tiff = false, iirs = false, mmor = false;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling()) {
        const u32 f = c->props()->carve_format;
        if (f == FMT_TIFF_BE && c->props()->extents.front().offset == 16384)
            be_tiff = true;
        if (f == FMT_ORF_IIRS && c->props()->extents.front().offset == 20480)
            iirs = true;
        if (f == FMT_ORF_MMOR && c->props()->extents.front().offset == 24576)
            mmor = true;
    }
    EXPECT_TRUE(be_tiff) << "MM\\0\\2A anchor must carve";
    EXPECT_TRUE(iirs) << "IIRS anchor must carve";
    EXPECT_TRUE(mmor) << "MMOR anchor must carve";
}

TEST(CarveEngine, PstUnknownVersionAcceptedAtFloor) {
    /* CARVE-4: og FUN_180041eb0 accepts unknown PST generations with the
     * 1 MiB floor - the old reject had the polarity inverted. Standalone
     * 2 MiB image: the PST registry row's 64 KiB min_size exceeds the
     * shared 32 KiB fixture. */
    std::vector<u8> img(2 * 1024 * 1024, 0xAA);
    const size_t at = 1024 * 1024;
    std::memcpy(img.data() + at, "!BDN", 4);
    img[at + 4] = 'M';
    img[at + 5] = 'S';
    img[at + 0x0A] = 0x15;   /* unknown generation */
    img[at + 0x1000] = 0x7F; /* AMap probe: non-zero */
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats s;
    ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, s),
              OK);
    bool pst = false;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling()) {
        if (c->props() && c->props()->carve_format == FMT_PST &&
            c->props()->extents.front().offset == at) {
            pst = true;
            EXPECT_EQ(c->props()->logical_size, size_t(1) << 20); /* 1 MiB floor */
        }
    }
    EXPECT_TRUE(pst) << "unknown-version PST must carve at the floor";
}

TEST(CarveEngine, JpegSof0PrecisionGate) {
    /* CARVE-6: og gates SOF0 on precision == 8 (corpus 39653) - a SOF0
     * with a nonsense precision byte must not validate. */
    auto img = build_carve_image();
    {
        /* find the planted JPEG's SOF0 and corrupt its precision byte.
         * plant_jpeg layout: SOI, APP0(JFIF), then SOF0 at a fixed spot -
         * search for FF C0 and patch byte +4 of the segment. */
        for (size_t i = 512; i < 1600; ++i) {
            if (img[i] == 0xFF && img[i + 1] == 0xC0) {
                img[i + 4] = 0x0C; /* precision 12: og rejects */
                break;
            }
        }
    }
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats s;
    ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, s),
              OK);
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling()) {
        if (c->props() && c->props()->carve_format == FMT_JPG &&
            c->props()->extents.front().offset == 512)
            FAIL() << "SOF0 precision!=8 must not validate as grammar JPEG";
    }
}

/* ---- GAP-S6-8: the carve basis must be disk-absolute on sliced devices ----
 *
 * Bench ours-behavior probe (runs 40073093997 + 40077233920): the resumed
 * scan's tree carried EXTRA carved entries the uninterrupted scan's tree
 * did not - the resumed carve re-emitted extents the first pass had
 * already emitted, because the dedup queries ran on the SLICE-relative
 * basis while the checkpoint-loaded tree keys were DISK-absolute (F-1).
 * Carve now emits + queries disk-absolute (reads and bounds stay
 * slice-relative); the resumed re-carve dedups exactly like
 * SecondPassFullyDeduped does on a flat device. */
TEST(CarveEngine, SlicedDeviceSecondPassFullyDedupedAndAbsolute) {
    /* parent disk with the carve image living INSIDE a slice at a nonzero
     * base - the volume-scan shape */
    const u64 kSliceStart = 512ULL * 2048; /* 1 MiB */
    std::vector<u8> disk(kSliceStart + 2 * 1024 * 1024, 0);
    auto img = build_carve_image();
    std::memcpy(disk.data() + kSliceStart, img.data(), img.size());

    MemDevice parent(std::move(disk));
    u32 err = 0;
    auto slice = PartitionSlice::create(&parent, kSliceStart, 2 * 1024 * 1024, &err);
    ASSERT_NE(slice, nullptr) << err_text(err);

    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats s1, s2;
    ASSERT_EQ(carve_slice(slice.get(), flags, nullptr, nullptr, nullptr, tree, root, nullptr,
                          nullptr, s1),
              OK);
    ASSERT_GT(s1.validated, 0u) << "pass 1 must carve the fixture";
    /* every carved extent is DISK-absolute (slice base + relative) */
    for (auto* n = root->first_child(); n; n = n->next_sibling()) {
        if (n->props() && !n->props()->extents.empty()) {
            EXPECT_GE(n->props()->extents.front().offset, kSliceStart)
                << "carved extents must be disk-absolute on sliced devices";
        }
    }
    const size_t nodes_after_p1 = tree.node_count();

    /* the resume shape: a SECOND pass over the same slice against the SAME
     * tree (the checkpoint-loaded nodes are already in it) must add ZERO */
    ASSERT_EQ(carve_slice(slice.get(), flags, nullptr, nullptr, nullptr, tree, root, nullptr,
                          nullptr, s2),
              OK);
    EXPECT_EQ(s2.validated, 0u);
    EXPECT_EQ(tree.node_count(), nodes_after_p1)
        << "the resumed carve must not re-emit pass-1 entries (the exact "
           "ours-behavior resume-equivalence failure)";
    EXPECT_GE(s2.duplicates, 4u);
}

/* ---- S-017: the ftyp family must be REACHABLE and correctly shifted ----- */

namespace {

/* minimal real-world BMFF head: a coherent box chain - ftyp box
 * (size 0x14 = 4 size + 4 type + 4 major + 4 minor + 4 ONE compat brand)
 * followed by one concrete mdat box so the box walk ends sanely. Box
 * sizes must cover exactly the bytes written - a mis-sized box swallows
 * the next box's header (the mdat SIZE becomes the mdat type) and the
 * walk mis-parses. */
std::vector<u8> bmff_head(const char* brand) {
    std::vector<u8> v;
    auto be32 = [&](u32 x) {
        v.push_back(u8(x >> 24));
        v.push_back(u8(x >> 16));
        v.push_back(u8(x >> 8));
        v.push_back(u8(x));
    };
    const u32 mdat_payload = 512;
    be32(0x14); /* ftyp: covers exactly size+type+major+minor+1 compat */
    for (char ch : std::string("ftyp"))
        v.push_back(u8(ch));
    for (int i = 0; i < 4; ++i)
        v.push_back(u8(brand[i])); /* major brand */
    be32(0x00000200);              /* minor version */
    for (int i = 0; i < 4; ++i)
        v.push_back(u8(brand[i])); /* compatible brands (1) */
    be32(8 + mdat_payload);        /* mdat box */
    for (char ch : std::string("mdat"))
        v.push_back(u8(ch));
    v.resize(v.size() + mdat_payload, 0x5A);
    return v;
}

tree::FileNode* find_carved_by_ext(tree::FileNode* root, const char* ext) {
    const std::string dot_ext = std::string(".") + ext;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling()) {
        const std::string nm = c->name_utf8();
        if (nm.size() > dot_ext.size() &&
            nm.compare(nm.size() - dot_ext.size(), dot_ext.size(), dot_ext) == 0)
            return c;
    }
    return nullptr;
}

} // namespace

TEST(Carve, Mp4RealFtypIsom) {
    /* real MP4: ftyp isom size 0x18 + mdat. Pre-S-017 the magic carried
     * four literal zero size-bytes so NO real MP4 could ever match. */
    std::vector<u8> img(256 * 1024, 0);
    auto mp4 = bmff_head("isom");
    std::memcpy(img.data() + 6144, mp4.data(), mp4.size());
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats s;
    ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, s),
              OK);
    auto* n = find_carved_by_ext(root, "mp4");
    ASSERT_NE(n, nullptr) << "real MP4 (ftyp isom, size 0x18) not carved";
    EXPECT_EQ(n->props()->extents.front().offset, u64(6144));
    /* candidate = magic-4 (the SIZE field), so the extent starts at the
     * ftyp box, not at the "ftyp" type bytes */
    EXPECT_GE(n->props()->logical_size, u64(mp4.size()));
}

TEST(Carve, MovRealFtypQt) {
    std::vector<u8> img(256 * 1024, 0);
    auto mov = bmff_head("qt  ");
    std::memcpy(img.data() + 10240, mov.data(), mov.size());
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats s;
    ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, s),
              OK);
    auto* n = find_carved_by_ext(root, "mov");
    ASSERT_NE(n, nullptr) << "real MOV (ftyp qt) not carved";
    EXPECT_EQ(n->props()->extents.front().offset, u64(10240));
}

TEST(Carve, HeicRealFtypHeic) {
    std::vector<u8> img(256 * 1024, 0);
    auto heic = bmff_head("heic");
    std::memcpy(img.data() + 4096, heic.data(), heic.size());
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats s;
    ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, s),
              OK);
    auto* n = find_carved_by_ext(root, "heic");
    ASSERT_NE(n, nullptr) << "real HEIC (ftyp heic) not carved";
    EXPECT_EQ(n->props()->extents.front().offset, u64(4096));
}

TEST(Carve, M4aRealFtyp) {
    std::vector<u8> img(256 * 1024, 0);
    auto m4a = bmff_head("M4A ");
    std::memcpy(img.data() + 20480, m4a.data(), m4a.size());
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats s;
    ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, s),
              OK);
    auto* n = find_carved_by_ext(root, "m4a");
    ASSERT_NE(n, nullptr) << "real M4A (ftyp M4A ) not carved";
    EXPECT_EQ(n->props()->extents.front().offset, u64(20480));
}

TEST(Carve, MovMoovWithoutFtyp) {
    /* og MovAnalyzer parity (S-017 fix plan step 3): a file whose ftyp
     * box was destroyed but whose top-level moov+mdat survive still
     * carves via the naked-moov anchor. */
    std::vector<u8> img(256 * 1024, 0);
    /* coherent top-level chain WITHOUT any ftyp: a real 0x14 moov box
     * (type swapped into the bmff_head layout) followed by mdat */
    auto moov = bmff_head("xxxx");
    moov[4] = 'm';
    moov[5] = 'o';
    moov[6] = 'o';
    moov[7] = 'v';
    std::memcpy(img.data() + 8192, moov.data(), moov.size());
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats s;
    ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, s),
              OK);
    auto* n = find_carved_by_ext(root, "mov");
    ASSERT_NE(n, nullptr) << "naked-moov MOV not salvaged";
    EXPECT_EQ(n->props()->extents.front().offset, u64(8192));
}

TEST(Carve, RegistrySelfProbeReachesEveryFormat) {
    /* S-017 F8 self-probe: every signature's magic must be reachable
     * through the scan's own match primitive with the correct candidate
     * shift. The pre-S-017 BMFF entries (literal zero size-bytes in the
     * magic) pass validate_registry but fail this probe - load-time
     * deadness is now a gate. */
    std::vector<const Signature*> bad;
    EXPECT_EQ(self_probe_registry(&bad), 0u);
    for (const Signature* s : bad)
        ADD_FAILURE() << "unreachable signature: " << (s ? s->name : "?");
}

/* ---- S-007: the og's hardcoded common formats --------------------------- */

namespace {

/* A valid single-image GIF: header + LSD (+GCT) + image descriptor + one
 * LZW sub-block + trailer. */
size_t plant_gif(std::vector<u8>& img, size_t at) {
    std::memcpy(img.data() + at, "GIF89a", 6);
    put16(img, at + 6, 16);  /* width */
    put16(img, at + 8, 16);  /* height */
    img[at + 10] = 0x80 | 2; /* GCT flag, 8-entry table (3<<3 = 24 bytes) */
    img[at + 11] = 0;
    img[at + 12] = 0;
    size_t p = at + 13 + 24; /* header + LSD + GCT */
    img[p] = 0x2C;           /* image descriptor */
    put16(img, p + 1, 0);
    put16(img, p + 3, 0);
    put16(img, p + 5, 16);
    put16(img, p + 7, 16);
    img[p + 9] = 0; /* no local color table */
    p += 10;
    img[p] = 8; /* LZW min code size */
    ++p;
    img[p] = 32; /* one sub-block of 32 bytes */
    for (int i = 0; i < 32; ++i)
        img[p + 1 + size_t(i)] = u8(i);
    p += 33;
    img[p] = 0; /* sub-block terminator */
    ++p;
    img[p] = 0x3B; /* trailer */
    return (p + 1) - at;
}

/* A valid uncompressed 4x4 24-bpp BMP. */
size_t plant_bmp(std::vector<u8>& img, size_t at) {
    img[at] = 'B';
    img[at + 1] = 'M';
    const u32 off_bits = 14 + 40;
    const u32 image_bytes = 4 * 4 * 3;           /* rows already 4-aligned */
    put32v(img, at + 2, off_bits + image_bytes); /* bfSize */
    put32v(img, at + 10, off_bits);
    put32v(img, at + 14, 40); /* BITMAPINFOHEADER */
    put32v(img, at + 18, 4);  /* width */
    put32v(img, at + 22, 4);  /* height */
    put16(img, at + 26, 1);   /* planes */
    put16(img, at + 28, 24);  /* bpp */
    put32v(img, at + 30, 0);  /* BI_RGB */
    put32v(img, at + 34, image_bytes);
    for (u32 i = 0; i < image_bytes; ++i)
        img[at + off_bits + i] = u8(0x40 + (i % 64));
    return off_bits + image_bytes;
}

/* N headerless MPEG-1 Layer III frames at a fixed, simple bitrate. */
size_t plant_mp3_raw(std::vector<u8>& img, size_t at, int frames) {
    /* FF FB: MPEG1 Layer III no-CRC; 0x9000 = 128kbps idx, 0x00 = 44100 */
    size_t p = at;
    for (int f = 0; f < frames; ++f) {
        img[p] = 0xFF;
        img[p + 1] = 0xFB;
        img[p + 2] = 0x90;
        img[p + 3] = 0x00;
        /* frame size = 144*128000/44100 = 417 (+pad 0) */
        for (int i = 4; i < 417; ++i)
            img[p + size_t(i)] = u8((i * 7) & 0x7F); /* non-sync payload */
        p += 417;
    }
    return p - at;
}

/* N AAC ADTS frames: AAC-LC 44.1kHz stereo, 13-bit lengths. */
size_t plant_aac_adts(std::vector<u8>& img, size_t at, int frames) {
    size_t p = at;
    const u32 flen = 200; /* fixed frame length for the fixture */
    for (int f = 0; f < frames; ++f) {
        img[p] = 0xFF;
        img[p + 1] = 0xF1;                        /* sync, MPEG-4, layer 00, no CRC */
        img[p + 2] = 0x50;                        /* profile 1 (LC), sr idx 4 (44100), private 0 */
        img[p + 3] = 0x80 | u8((flen >> 11) & 3); /* chan_lo=2 + len hi bits */
        img[p + 4] = u8((flen >> 3) & 0xFF);
        img[p + 5] = u8((flen & 7) << 5) | 0x1F; /* len lo + buffer fullness */
        img[p + 6] = 0xFC;                       /* buffer fullness tail + blocks-in-frame 0 */
        for (u32 i = 7; i < flen; ++i)
            img[p + i] = u8((i * 13) & 0x7F);
        p += flen;
    }
    return p - at;
}

/* A brace-balanced RTF. */
size_t plant_rtf(std::vector<u8>& img, size_t at) {
    const char* body =
        "{\\rtf1\\ansi{\\fonttbl{\\f0 Times New Roman;}}\\pard Hello, \\{world\\}!\\par}";
    const size_t n = std::strlen(body);
    std::memcpy(img.data() + at, body, n);
    return n;
}

/* A minimal Compound File: header + DIFAT head pointing at FAT sector 1,
 * directory chain sector 0..1. */
size_t plant_ole(std::vector<u8>& img, size_t at) {
    const u64 ssz = 512; /* sector shift 9 */
    std::memcpy(img.data() + at, "\xD0\xCF\x11\xE0\xA1\xB1\x1A\xE1", 8);
    put16(img, at + 24, 0x003E); /* minor */
    put16(img, at + 26, 3);      /* major */
    put16(img, at + 28, 0xFFFE); /* byte order */
    put16(img, at + 30, 9);      /* sector shift */
    put16(img, at + 32, 6);      /* mini shift */
    put32v(img, at + 44, 1);     /* one FAT sector */
    put32v(img, at + 48, 0);     /* first dir sector */
    put32v(img, at + 56, 4096);  /* mini cutoff */
    put32v(img, at + 72, 0);     /* no DIFAT chain */
    put32v(img, at + 76, 1);     /* DIFAT[0] = FAT at sector 1 */
    for (u32 i = 1; i < 109; ++i)
        put32v(img, at + 76 + i * 4, 0xFFFFFFFF); /* FREESECT */
    /* CFB layout: the 512-byte header occupies the first sector slot;
     * sector N's data lives at (N+1)*ssz. FAT = sector 1 (offset 2*ssz);
     * directory = sector 0 (offset 1*ssz). */
    put32v(img, at + 2 * ssz + 0, 0xFFFFFFFE); /* FAT[0]: dir chain ends */
    put32v(img, at + 2 * ssz + 4, 0xFFFFFFFD); /* FAT[1]: FATSECT itself */
    std::memcpy(img.data() + at + 1 * ssz + 0, "Root Entry", 10);
    return 3 * ssz;
}

/* An MPEG-PS: one MPEG-2 pack + one system header + one video packet +
 * end code. */
size_t plant_mpegps(std::vector<u8>& img, size_t at) {
    size_t p = at;
    img[p] = 0;
    img[p + 1] = 0;
    img[p + 2] = 1;
    img[p + 3] = 0xBA; /* pack header, MPEG-2 form */
    img[p + 4] = 0x44; /* SCR markers => MPEG-2 14-byte pack */
    for (int i = 5; i < 14; ++i)
        img[p + size_t(i)] = u8(0x10 + i);
    p += 14;
    img[p + 0] = 0;
    img[p + 1] = 0;
    img[p + 2] = 1;
    img[p + 3] = 0xBB;    /* system header */
    put16(img, p + 4, 6); /* length */
    for (int i = 0; i < 6; ++i)
        img[p + 6 + size_t(i)] = 0;
    p += 6 + 6;
    img[p + 0] = 0;
    img[p + 1] = 0;
    img[p + 2] = 1;
    img[p + 3] = 0xE0;    /* video packet */
    put16(img, p + 4, 8); /* PES length */
    for (int i = 0; i < 8; ++i)
        img[p + 6 + size_t(i)] = u8(0x20 + i);
    p += 6 + 8;
    img[p + 0] = 0;
    img[p + 1] = 0;
    img[p + 2] = 1;
    img[p + 3] = 0xB9; /* end code */
    return (p + 4) - at;
}

/* An R2000 DWG (AC1015) with the r13+ sentinel. */
size_t plant_dwg(std::vector<u8>& img, size_t at) {
    std::memcpy(img.data() + at, "AC1015", 6);
    std::memcpy(img.data() + at + 6, "\x0D\x0A\x0D\x0A\x00", 5);
    for (size_t i = 11; i < 4096; ++i)
        img[at + i] = u8((i * 31) & 0xFF);
    return 4096;
}

} // namespace

TEST(Carve, S007CommonFormatsCarve) {
    /* S-007's falsifiable claim: gif, bmp, headerless mp3, rtf, OLE doc,
     * mpg and dwg plants yield carve results - the pre-fix registry had
     * none of these families. */
    std::vector<u8> img(256 * 1024, 0);
    const size_t gif_at = 512, bmp_at = 4096, mp3_at = 8192, rtf_at = 20480, ole_at = 24576,
                 mpg_at = 32768, dwg_at = 65536, aac_at = 131072;
    const size_t gif_len = plant_gif(img, gif_at);
    const size_t bmp_len = plant_bmp(img, bmp_at);
    plant_mp3_raw(img, mp3_at, 12);
    const size_t rtf_len = plant_rtf(img, rtf_at);
    const size_t ole_len = plant_ole(img, ole_at);
    const size_t mpg_len = plant_mpegps(img, mpg_at);
    const size_t dwg_len = plant_dwg(img, dwg_at);
    plant_aac_adts(img, aac_at, 12);

    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats stats;
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
        OK)
        << err_text(OK);

    std::vector<tree::FileNode*> all;
    std::function<void(tree::FileNode*)> collect = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            all.push_back(c);
            collect(c);
        }
    };
    collect(root);

    auto find_at = [&](size_t off) -> tree::FileNode* {
        for (tree::FileNode* n : all)
            if (!n->props()->extents.empty() && n->props()->extents.front().offset == off)
                return n;
        return nullptr;
    };
    auto expect_carved = [&](size_t off, u32 fmt, size_t min_len, const char* what) {
        tree::FileNode* n = find_at(off);
        ASSERT_NE(n, nullptr) << what << " must carve (S-007)";
        EXPECT_EQ(n->props()->carve_format, fmt) << what;
        EXPECT_GE(n->props()->logical_size, min_len) << what;
    };
    expect_carved(gif_at, FMT_GIF, gif_len, "GIF");
    expect_carved(bmp_at, FMT_BMP, bmp_len, "BMP");
    expect_carved(mp3_at, FMT_MPGA_FB, 12u * 417u, "headerless MP3");
    expect_carved(rtf_at, FMT_RTF, rtf_len, "RTF");
    expect_carved(ole_at, FMT_OLE, ole_len, "OLE compound");
    expect_carved(mpg_at, FMT_MPEGPS, mpg_len, "MPEG-PS");
    expect_carved(dwg_at, FMT_DWG, dwg_len, "DWG");
    expect_carved(aac_at, FMT_AAC, 12u * 200u, "AAC ADTS");
}

TEST(Carve, S007ValidatorsRejectNoise) {
    /* The 2-byte keys (BM, FF xx) match noise constantly - the grammar
     * gates are the discriminator: <10-frame audio, bad DIB, bad GIF
     * version, wrong OLE shift, unknown DWG version all reject. */
    std::vector<u8> img(128 * 1024, 0);
    /* "BM" + garbage DIB size at 512 */
    img[512] = 'B';
    img[513] = 'M';
    put32v(img, 514 + 0, 0); /* bfSize */
    put32v(img, 514 + 8, 64);
    put32v(img, 514 + 12, 33); /* bogus DIB size */
    /* GIF8 + bad version at 1024 */
    std::memcpy(img.data() + 1024, "GIF88a", 6);
    /* 3-frame headerless MP3 at 2048 */
    plant_mp3_raw(img, 2048, 3);
    /* 4-frame ADTS at 4096 */
    plant_aac_adts(img, 4096, 4);
    /* OLE magic + wrong sector shift at 8192 */
    std::memcpy(img.data() + 8192, "\xD0\xCF\x11\xE0\xA1\xB1\x1A\xE1", 8);
    put16(img, 8192 + 26, 3);
    put16(img, 8192 + 28, 0xFFFE);
    put16(img, 8192 + 30, 11); /* invalid shift */
    put16(img, 8192 + 32, 6);
    /* unknown DWG version at 16384 */
    std::memcpy(img.data() + 16384, "AC1099", 6);
    /* lone MPEG pack header at 32768 (no following elements) */
    std::memcpy(img.data() + 32768, "\x00\x00\x01\xBA\x44\x10\x10\x10\x10\x10\x10\x10\x10\x10", 14);

    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats stats;
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
        OK);
    std::vector<tree::FileNode*> all;
    std::function<void(tree::FileNode*)> collect = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            all.push_back(c);
            collect(c);
        }
    };
    collect(root);
    for (tree::FileNode* n : all) {
        const u64 off = n->props()->extents.front().offset;
        /* none of the noise plants may carve (the JPEG-tail raw-sign
         * layer is NOT in this fixture - every node here is grammar) */
        const u64 fmt = n->props()->carve_format;
        EXPECT_FALSE(off == 512 && fmt == FMT_BMP) << "bad-DIB BMP rejected";
        EXPECT_FALSE(off == 1024 && fmt == FMT_GIF) << "bad-version GIF rejected";
        EXPECT_FALSE(off == 2048 && (fmt == FMT_MPGA_FB || fmt >= FMT_MPGA_E2))
            << "3-frame MP3 rejected (10-frame gate)";
        EXPECT_FALSE(off == 4096 && (fmt == FMT_AAC || fmt == FMT_AAC2))
            << "4-frame ADTS rejected (10-frame gate)";
        EXPECT_FALSE(off == 8192 && fmt == FMT_OLE) << "bad-shift OLE rejected";
        EXPECT_FALSE(off == 16384 && fmt == FMT_DWG) << "unknown DWG version rejected";
        EXPECT_FALSE(off == 32768 && fmt == FMT_MPEGPS) << "lone pack header rejected";
    }
    EXPECT_TRUE(stats.rejected >= 7) << "each noise plant must reach a validator rejection";
}

TEST(Carve, S007TruncatedSalvageSemantics) {
    /* GIF truncated after images: salvage at degraded confidence; RTF
     * with unclosed groups: salvage to the slab end. */
    std::vector<u8> img(64 * 1024, 0);
    const size_t gif_at = 512;
    const size_t gif_len = plant_gif(img, gif_at);
    img[gif_at + gif_len - 1] = 0x21; /* destroy the trailer: truncated */
    const size_t rtf_at = 4096;
    const char kOpen[] = "{\\rtf1\\ansi{\\b unclosed";
    std::memcpy(img.data() + rtf_at, kOpen, sizeof(kOpen) - 1);

    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats stats;
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
        OK);
    std::vector<tree::FileNode*> all;
    std::function<void(tree::FileNode*)> collect = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            all.push_back(c);
            collect(c);
        }
    };
    collect(root);
    tree::FileNode* gif = nullptr;
    tree::FileNode* rtf = nullptr;
    for (tree::FileNode* n : all) {
        if (n->props()->extents.front().offset == gif_at)
            gif = n;
        else if (n->props()->extents.front().offset == rtf_at)
            rtf = n;
    }
    ASSERT_NE(gif, nullptr) << "truncated GIF salvages after its images";
    EXPECT_LT(gif->props()->logical_size, gif_len) << "extent stops before the broken trailer";
    ASSERT_NE(rtf, nullptr) << "unclosed-group RTF salvages to the slab end";
    EXPECT_EQ(rtf->props()->logical_size, size_t(64 * 1024 - rtf_at));
}

/* ===================================================================== */
/* S-031: every carved record is deleted-family state (og CRAWFileProperties */
/* FLAGS = constant 0x0020, 12,147/12,147 records — ifileproperties-part3 :82). */
/* Both emit sites (grammar + raw-sign) must flag Deleted so the deleted_only */
/* view and deleted_count include the carve population.                       */
/* ===================================================================== */

TEST(CarveEngine, CarvedNodesAreDeleted) {
    auto img = build_carve_image();
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats stats;
    u32 rc =
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats);
    ASSERT_EQ(rc, OK) << err_text(rc);
    ASSERT_GE(stats.validated, 1u);

    int carved = 0;
    std::function<void(tree::FileNode*)> collect = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            if (c->props() && c->origin() == tree::Origin::Carved) {
                ++carved;
                EXPECT_EQ(tree::DeletedState::Deleted, c->deleted_state())
                    << "carved node not flagged deleted";
            }
            collect(c);
        }
    };
    collect(root);
    EXPECT_GE(carved, 4) << "grammar layer emit not found";

    /* stats: the deleted population includes the carved ARCHIVE records
     * (direct children - the og's census counts one CRAWFileProperties
     * record per archive; inner entries are presentation children, not
     * records - S-032) */
    tree.refresh_stats();
    size_t carved_direct = 0;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
        if (c->props() && c->origin() == tree::Origin::Carved)
            ++carved_direct;
    EXPECT_EQ(root->stats().deleted_count, carved_direct);
}

TEST(CarveEngine, RawSignCarvedNodesAreDeleted) {
    /* the raw-sign layer (the second emit site) flags Deleted too: a lone
     * "sample" header sign (#!/bin/sh) carves header-only (S-015's shape) */
    std::vector<u8> img(2 * 1024 * 1024, 0);
    std::memcpy(img.data() + 4096, "#!/bin/sh\n", 10);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats stats;
    u32 rc =
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats);
    ASSERT_EQ(rc, OK) << err_text(rc);

    int carved = 0;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling()) {
        if (c->props() && c->origin() == tree::Origin::Carved) {
            ++carved;
            EXPECT_EQ(tree::DeletedState::Deleted, c->deleted_state())
                << "raw-sign carved node not flagged deleted";
        }
    }
    EXPECT_GE(carved, 1) << "raw-sign layer emit not found";
}

TEST(CarveEngine, DeletedOnlyQueryIncludesCarvedFiles) {
    /* THE product-level discriminator: the deleted_only filter is the
     * recovery product's primary "show me deleted files" view - carved
     * files (the only recoverable content after metadata destruction)
     * must appear there. Pre-fix: zero carved rows returned. */
    auto img = build_carve_image();
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats stats;
    u32 rc =
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats);
    ASSERT_EQ(rc, OK) << err_text(rc);
    tree.refresh_stats();

    query::Query q;
    q.filter.deleted_only = true;
    auto res = query::run(tree, q);
    EXPECT_GE(res.rows.size(), 4u) << "deleted_only query dropped the carve set";
    EXPECT_GE(res.total_matched, 4u);
    bool saw_carved = false;
    for (const auto& r : res.rows)
        if (r.node && r.node->origin() == tree::Origin::Carved)
            saw_carved = true;
    EXPECT_TRUE(saw_carved) << "no carved row in the deleted_only result";
}

/* ===================================================================== */
<<<<<<< HEAD
/* S-031 timestamp residual: every carved node carries the scan-time     */
/* stamp (the og's CRAWFileProperties per-record timestamp string,      */
/* FUN_140c41aa0 "%d-%d-%d %d-%d-%d" — ifileproperties-part3 :80).       */
/* ===================================================================== */

TEST(CarveEngine, CarvedNodesCarryScanTimestamp) {
    /* both emit sites stamp times.modified with the run's wall clock:
     * captured once per carve_slice so one scan's population agrees */
    auto img = build_carve_image();
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats stats;
    const u64 before = u64(std::time(nullptr));
    u32 rc =
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats);
    const u64 after = u64(std::time(nullptr));
    ASSERT_EQ(rc, OK) << err_text(rc);
    ASSERT_GE(stats.validated, 1u);

    int carved = 0;
    std::function<void(tree::FileNode*)> collect = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            if (c->props() && c->origin() == tree::Origin::Carved) {
                ++carved;
                const u64 t = c->props()->times.modified;
                EXPECT_NE(t, 0u) << "grammar-layer carved node has no scan timestamp";
                if (t != 0) {
                    /* 100ns since 1601 -> unix seconds */
                    const u64 unix_t = (t / 10000000ull) - 11644473600ull;
                    EXPECT_GE(unix_t, before) << "scan timestamp before run start";
                    EXPECT_LE(unix_t, after) << "scan timestamp after run end";
                }
            }
            collect(c);
        }
    };
    collect(root);
    EXPECT_GE(carved, 4) << "grammar layer emit not found";
}

TEST(CarveEngine, RawSignCarvedNodesCarryScanTimestamp) {
    /* the second emit site (raw-sign layer) stamps the same field */
    std::vector<u8> img(2 * 1024 * 1024, 0);
    std::memcpy(img.data() + 4096, "#!/bin/sh\n", 10);
=======
/* S-032: archive inner-entry inventory (og: complete inventory returned  */
/* by the analysers — rawsearchdll--zip-analyser-carving.md :29-38;       */
/* analyser-granular-batch1 :36-41 for the RAR4 272-byte name buffer).    */
/* ===================================================================== */

namespace {

/* a minimal REAL-layout ZIP: one LFH (named entry + real data) + CD + EOCD */
size_t plant_zip_with_data(std::vector<u8>& img, size_t at, const char* name,
                            const std::vector<u8>& data) {
    const size_t nlen = strlen(name);
    std::memcpy(img.data() + at, "PK\x03\x04", 4);
    put16(img, at + 4, 20);
    put16(img, at + 6, 0);
    put16(img, at + 8, 0); /* stored */
    put16(img, at + 10, 0);
    put16(img, at + 12, 0x5A46);
    put32v(img, at + 14, 0);
    put32v(img, at + 18, u32(data.size())); /* csize */
    put32v(img, at + 22, u32(data.size())); /* usize */
    put16(img, at + 26, u16(nlen));
    put16(img, at + 28, 0);
    std::memcpy(img.data() + at + 30, name, nlen);
    std::memcpy(img.data() + at + 30 + nlen, data.data(), data.size());
    const size_t data_at = at + 30 + nlen;
    const size_t local_len = 30 + nlen + data.size();
    const size_t cd_at = at + local_len;
    std::memcpy(img.data() + cd_at, "PK\x01\x02", 4);
    put16(img, cd_at + 4, 20);
    put16(img, cd_at + 12, 0x5A46);
    put32v(img, cd_at + 20, u32(data.size())); /* csize */
    put32v(img, cd_at + 24, u32(data.size())); /* usize */
    put16(img, cd_at + 28, u16(nlen));
    put16(img, cd_at + 30, 0);
    put16(img, cd_at + 32, 0);
    put32v(img, cd_at + 42, 0); /* LFH offset = 0 (file-relative) */
    std::memcpy(img.data() + cd_at + 46, name, nlen);
    const size_t cd_len = 46 + nlen;
    const size_t eocd_at = cd_at + cd_len;
    std::memcpy(img.data() + eocd_at, "PK\x05\x06", 4);
    put16(img, eocd_at + 4, 0);  /* this disk */
    put16(img, eocd_at + 6, 0);  /* disk with CD start */
    put16(img, eocd_at + 8, 1);  /* entries this disk */
    put16(img, eocd_at + 10, 1); /* total entries */
    put32v(img, eocd_at + 12, u32(cd_len));       /* CD size */
    put32v(img, eocd_at + 16, u32(local_len));    /* CD offset (LFH-relative) */
    put16(img, eocd_at + 20, 0); /* comment len */
    return data_at;
}

} // namespace

TEST(CarveEngine, ZipInnerEntryInventoryEmitted) {
    /* the og's ZIP analyser returns the complete inner inventory (names,
     * sizes, offsets) which the results tree presents as the archive's
     * contents; the pre-fix core emitted one opaque node. The inventory
     * child carries a PRECISE data extent inside the parent (superset:
     * per-entry recovery) */
    auto img = std::vector<u8>(2 * 1024 * 1024, 0);
    const std::vector<u8> payload(40, 0xAB);
    const size_t data_at = plant_zip_with_data(img, 4096, "letter.txt", payload);
>>>>>>> ee78d15 (fix: S-032 - archive carve inner-entry inventories (ZIP CD + indexless LFH + RAR4 FILE blocks) + RAR4 HEAD_SIZE off-by-one corrected)
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats stats;
<<<<<<< HEAD
    const u64 before = u64(std::time(nullptr));
    u32 rc =
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats);
    const u64 after = u64(std::time(nullptr));
    ASSERT_EQ(rc, OK) << err_text(rc);

    int carved = 0;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling()) {
        if (c->props() && c->origin() == tree::Origin::Carved) {
            ++carved;
            const u64 t = c->props()->times.modified;
            EXPECT_NE(t, 0u) << "raw-sign carved node has no scan timestamp";
            if (t != 0) {
                const u64 unix_t = (t / 10000000ull) - 11644473600ull;
                EXPECT_GE(unix_t, before);
                EXPECT_LE(unix_t, after);
            }
        }
    }
    EXPECT_GE(carved, 1) << "raw-sign layer emit not found";
}

TEST(CarveEngine, ScanTimestampSurvivesSessionRoundTrip) {
    /* the og PERSISTS the per-record timestamp (the CRAW string field);
     * our equivalent carrier is record 4's times.modified — the stamped
     * value must round-trip through serialize/deserialize intact */
    auto img = build_carve_image();
=======
    ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr,
                          stats),
              OK);

    tree::FileNode* zip_node = nullptr;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
        if (c->props() && c->props()->carve_format == FMT_ZIP &&
            !c->props()->extents.empty() && c->props()->extents.front().offset == 4096)
            zip_node = c;
    ASSERT_NE(zip_node, nullptr) << "archive node not carved";
    ASSERT_EQ(tree::Origin::Carved, zip_node->origin());

    tree::FileNode* entry = zip_node->first_child();
    ASSERT_NE(entry, nullptr) << "no inner-entry inventory emitted";
    ASSERT_EQ(entry->props()->name, u"letter.txt");
    EXPECT_EQ(40u, entry->props()->logical_size);
    ASSERT_FALSE(entry->props()->extents.empty());
    EXPECT_EQ(u64(data_at), entry->props()->extents.front().offset);
    EXPECT_EQ(40u, entry->props()->extents.front().length);
    EXPECT_EQ(tree::Origin::Carved, entry->origin());
    EXPECT_EQ(tree::DeletedState::Deleted, entry->deleted_state());
}

TEST(CarveEngine, ZipIndexlessEntryInventory) {
    /* CD destroyed: the LFH-chain walk (CARVE-2) still returns the
     * inventory - names + data spans from each local header */
    auto img = std::vector<u8>(2 * 1024 * 1024, 0);
    size_t at = 4096;
    for (const char* nm : {"first.bin", "second.bin"}) {
        const size_t nlen = strlen(nm);
        const size_t dlen = 100; /* indexless total must clear FMT_ZIP's
                                     128-byte min_size floor */
        std::memcpy(img.data() + at, "PK\x03\x04", 4);
        put16(img, at + 4, 20);
        put32v(img, at + 18, u32(dlen));
        put32v(img, at + 22, u32(dlen));
        put16(img, at + 26, u16(nlen));
        put16(img, at + 28, 0);
        std::memcpy(img.data() + at + 30, nm, nlen);
        memset(img.data() + at + 30 + nlen, 0xCD, dlen);
        at += 30 + nlen + dlen;
    }
>>>>>>> ee78d15 (fix: S-032 - archive carve inner-entry inventories (ZIP CD + indexless LFH + RAR4 FILE blocks) + RAR4 HEAD_SIZE off-by-one corrected)
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats stats;
<<<<<<< HEAD
    u32 rc =
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats);
    ASSERT_EQ(rc, OK) << err_text(rc);

    u64 stamped = 0;
    int carved = 0;
    std::function<void(tree::FileNode*)> collect = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            if (c->props() && c->origin() == tree::Origin::Carved) {
                ++carved;
                stamped = c->props()->times.modified;
            }
            collect(c);
        }
    };
    collect(root);
    ASSERT_GE(carved, 1);
    ASSERT_NE(stamped, 0u) << "nothing stamped to round-trip";

    std::vector<u8> blob;
    ASSERT_EQ(session::serialize_tree(tree, blob), OK);
    tree::ResultTree loaded;
    ASSERT_EQ(session::deserialize_tree(blob.data(), blob.size(), loaded), OK);
    bool found = false;
    std::function<void(tree::FileNode*)> scan2 = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            if (c->props() && c->origin() == tree::Origin::Carved) {
                found = true;
                EXPECT_EQ(c->props()->times.modified, stamped)
                    << "scan timestamp lost in session round-trip";
            }
            scan2(c);
        }
    };
    scan2(loaded.roots().empty() ? nullptr : loaded.roots()[0]);
    EXPECT_TRUE(found) << "carved node not found after reload";
=======
    ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr,
                          stats),
              OK);

    tree::FileNode* zip_node = nullptr;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
        if (c->props() && c->props()->carve_format == FMT_ZIP &&
            !c->props()->extents.empty() && c->props()->extents.front().offset == 4096) {
            zip_node = c;
            break; /* the FIRST archive: the second LFH re-carves as its own
                    * nested candidate (dedup keyed exact-start, the
                    * indexless chain has no CD to claim it) */
        }
    ASSERT_NE(zip_node, nullptr) << "indexless archive not carved";
    std::set<std::string> names;
    for (tree::FileNode* e = zip_node->first_child(); e; e = e->next_sibling())
        if (e->props())
            names.insert(e->name_utf8());
    EXPECT_EQ(2u, names.size());
    EXPECT_TRUE(names.count("first.bin"));
    EXPECT_TRUE(names.count("second.bin"));
}

TEST(CarveEngine, Rar4InnerEntryInventory) {
    /* a REAL-layout RAR4 (MARK 7B + MAIN_HEAD size@+12 + FILE_HEAD with a
     * name + END): the inventory child carries the entry name + size +
     * data extent. Doubles as the positive real-layout fixture for the
     * S-032 HEAD_SIZE off-by-one fix (the old validator read the size
     * field at +11 and could not parse real archives' block chain). */
    auto img = std::vector<u8>(2 * 1024 * 1024, 0);
    size_t at = 4096;
    const char* nm = "photo.bin";
    const size_t nlen = strlen(nm);
    const size_t dlen = 100;
    /* marker (7) */
    std::memcpy(img.data() + at, "Rar!\x1a\x07\x00", 7);
    /* MAIN_HEAD: crc, type 0x73, flags 0, size 13 (+6 pad) */
    put16(img, at + 7, 0);
    img[at + 9] = 0x73;
    put16(img, at + 10, 0);
    put16(img, at + 12, 13); /* HEAD_SIZE - the corrected field position */
    /* FILE_HEAD at at+20 */
    const size_t fh = at + 20;
    const size_t head_size = 32 + nlen; /* base7+pack4+unp4+host1+crc4+ftime4+ver1+method1+nsize2+attr4+name */
    put16(img, fh + 0, 0);
    img[fh + 2] = 0x74; /* FILE */
    put16(img, fh + 3, 0x8000); /* LONG_BLOCK: PACK_SIZE follows */
    put16(img, fh + 5, u16(head_size));
    put32v(img, fh + 7, u32(dlen));    /* PACK_SIZE */
    put32v(img, fh + 11, u32(dlen));   /* UNP_SIZE */
    img[fh + 15] = 0;                  /* HOST_OS */
    put32v(img, fh + 16, 0);           /* FILE_CRC */
    put32v(img, fh + 20, 0x60000000);  /* FTIME */
    img[fh + 24] = 29;                 /* UNP_VER */
    img[fh + 25] = 0x30;               /* METHOD stored */
    put16(img, fh + 26, u16(nlen));    /* NAME_SIZE */
    put32v(img, fh + 28, 0x20);        /* ATTR archive */
    std::memcpy(img.data() + fh + 32, nm, nlen);
    memset(img.data() + fh + head_size, 0x5E, dlen);
    /* END block */
    const size_t end_at = fh + head_size + dlen;
    put16(img, end_at + 0, 0);
    img[end_at + 2] = 0x7B;
    put16(img, end_at + 3, 0);
    put16(img, end_at + 5, 7);

    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats stats;
    ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr,
                          stats),
              OK);

    tree::FileNode* rar_node = nullptr;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
        if (c->props() && c->props()->carve_format == FMT_RAR4)
            rar_node = c;
    ASSERT_NE(rar_node, nullptr) << "real-layout RAR4 not carved (the off-by-one fix)";
    EXPECT_EQ(u64(end_at + 7), rar_node->props()->extents.front().offset +
                                   rar_node->props()->extents.front().length);

    tree::FileNode* entry = rar_node->first_child();
    ASSERT_NE(entry, nullptr) << "no RAR4 inner-entry inventory";
    EXPECT_EQ(entry->props()->name, u"photo.bin");
    EXPECT_EQ(100u, entry->props()->logical_size);
    ASSERT_FALSE(entry->props()->extents.empty());
    EXPECT_EQ(u64(fh + head_size), entry->props()->extents.front().offset);
    EXPECT_EQ(100u, entry->props()->extents.front().length);
>>>>>>> ee78d15 (fix: S-032 - archive carve inner-entry inventories (ZIP CD + indexless LFH + RAR4 FILE blocks) + RAR4 HEAD_SIZE off-by-one corrected)
}
