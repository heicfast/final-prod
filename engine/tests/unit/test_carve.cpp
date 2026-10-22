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
    plant_zip(img, 4096, "a.txt", 5);                /* plain ZIP */
    plant_zip(img, 8192, "[Content_Types].xml", 19); /* OOXML shape */
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
    tree::FileNode *jpg_node = nullptr, *docx_node = nullptr;
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

    /* sizes from grammar ends */
    ASSERT_NE(jpg_node, nullptr);
    EXPECT_EQ(jpg_node->props()->logical_size, size_t(521));
    ASSERT_NE(docx_node, nullptr);
    /* zip_common end: EOCD at ... */
    EXPECT_GT(docx_node->props()->logical_size, 30u + 19u + 46u + 19u + 22u - 2);
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
    ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, s2),
              OK);
    EXPECT_EQ(s2.validated, 0u);
    EXPECT_EQ(s2.duplicates, 4u); /* every re-found file deduped (F9) */
}

TEST(CarveEngine, RegistryF8Gate) {
    std::vector<const Signature*> bad;
    EXPECT_EQ(validate_registry(&bad), 0u); /* the shipped table is clean */
    EXPECT_EQ(registry_size(), FMT_COUNT);
}
