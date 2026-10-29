/* Carving engine tests: grammar validation, specificity, dedup, hashing. */
#include <gtest/gtest.h>
#include "cr/carve/cr_carve.h"
#include "cr/query/cr_query.h"
#include "cr/carve/cr_rawsigns.h"
#include "cr/carve/cr_usersigns.h"
#include "cr/storage/cr_device.h"
#include "cr/storage/cr_slice.h"
#include "cr/session/cr_records.h"
#include "cr/table/cr_table_wire.h"
#include "cr/cr_text.h"
#include <array>
#include <cstring>
#include <ctime>
#include <fstream>
#include <functional>
#include <iterator>
#include <map>
#include <vector>

using namespace cr;
using namespace cr::carve;

/* N-212: the ASF validator's direct-contract pin needs the file-local
 * declaration (the analyser set is registry-internal, not header-public) */
namespace cr {
namespace carve {
CarveResult validate_asf(IStorageDevice* s, u64 hit, u64 cap);
} // namespace carve
} // namespace cr

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

/* A-011/N-089: a byte-exact SQLite header + content (every multi-byte
 * field BIG-ENDIAN per the format spec; the og composes BE first-hand,
 * FUN_18002e2b0). enc: 1 = UTF-8, 2 = UTF-16LE, 3 = UTF-16BE. */
size_t plant_sqlite(std::vector<u8>& img, size_t at, u32 page_size, u32 pages, u32 enc) {
    std::memcpy(img.data() + at, "SQLite format 3", 15);
    img[at + 15] = 0;
    putbe16(img, at + 16, page_size == 65536 ? u16(1) : u16(page_size)); /* BE; 1 = 64KiB */
    img[at + 18] = 1;                                                    /* write ver */
    img[at + 19] = 1;                                                    /* read ver */
    putbe32(img, at + 28, pages);                                        /* page count (BE u32) */
    putbe32(img, at + 56, enc); /* text encoding (BE u32) */
    return size_t(pages) * page_size;
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

    /* validated: jpg, png, zip, docx + the jfif raw-sign emission at
     * 12288 (N-188: the sign-table layer's records count too); grammar
     * noise still rejected; no duplicates yet */
    EXPECT_EQ(stats.validated, 5u);
    EXPECT_GE(stats.rejected, 1u);
    /* N-153: the two archives' inner central-directory positions
     * (PK\x01\x02 inside the emitted extents) now count as duplicates -
     * the same archive, suppressed (pre-fix they re-carved as phantom
     * zips whenever the tail cleared the 128-byte floor) */
    EXPECT_EQ(stats.duplicates, 2u);
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
            EXPECT_EQ(n->name_utf8().substr(n->name_utf8().size() - 4), ".JPG");
            break;
        case FMT_PNG:
            EXPECT_EQ(n->name_utf8().substr(n->name_utf8().size() - 4), ".PNG");
            break;
        case FMT_DOCX:
            EXPECT_EQ(n->name_utf8().substr(n->name_utf8().size() - 5), ".DOCX");
            break;
        case FMT_ZIP:
            EXPECT_EQ(n->name_utf8().substr(n->name_utf8().size() - 4), ".ZIP");
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

TEST(Carve, StatsPerFormatTally) {
    /* N-140: the per-format carve tally - the og's GetRawUserInfoMap
     * per-scan map<extension,count> (RAWSearchDLL's user-info singleton
     * +0x90, FUN_18000ed00 insert-or-increment fed by every analyser
     * detection, cleared per scan; DRWUI renders the "found N .jpg /
     * M .png" post-scan breakdown, :227538-227625). The engine surface
     * must answer the breakdown WITHOUT a tree walk: the fixed tally
     * over the unified format-id space (grammar rows, then raw-sign
     * rows, then the user-sign aggregate) counts every emitted record at
     * the same sites `validated` ticks. RED pre-fix: the fields did not
     * exist (compile-error on the reverted tree). */
    auto img = build_carve_image();
    MemDevice dev(std::move(img));

    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats stats;
    u32 rc =
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats);
    ASSERT_EQ(rc, OK) << err_text(rc);
    ASSERT_GE(stats.validated, 5u); /* jpg, png, zip, docx + the jfif raw-sign row */

    /* the live bound: grammar rows + raw-sign rows + the user-sign
     * aggregate slot - the same counting space cr_ui_carve_format_count
     * enumerates, plus one */
    EXPECT_EQ(stats.per_format_slots, u32(registry_size()) + rawsigns::kFormatCount + 1u)
        << "the unified counting space + the user-sign aggregate";
    EXPECT_EQ(stats.per_format_overflow, 0u);

    /* the grammar rows tick once per emitted record */
    EXPECT_EQ(stats.per_format[FMT_JPG], 1u);
    EXPECT_EQ(stats.per_format[FMT_PNG], 1u);
    EXPECT_EQ(stats.per_format[FMT_ZIP], 1u);
    EXPECT_EQ(stats.per_format[FMT_DOCX], 1u);
    /* the jfif raw-sign row (the layer-1 emission at 12288): the row
     * index is registry_size() + the raw-sign format id; the tree's
     * carve_format tag 0xFFFF0000+fmt maps to the same slot */
    bool saw_raw_row = false;
    for (u32 i = u32(registry_size()); i < registry_size() + rawsigns::kFormatCount; ++i)
        if (stats.per_format[i] != 0)
            saw_raw_row = true;
    EXPECT_TRUE(saw_raw_row) << "the raw-sign layer's jfif emission must tick its row";

    /* the tally's total == validated: every emitted record counted
     * exactly once, no layer double-counting (the F-A3-6 class guard) */
    u64 sum = 0;
    for (u32 i = 0; i < stats.per_format_slots && i < CarveStats::kFormatTallyCapacity; ++i)
        sum += stats.per_format[i];
    EXPECT_EQ(sum, stats.validated) << "the per-format tally must partition `validated`";

    /* cross-check against a manual tree walk: every carved node's
     * carve_format maps to exactly one tally row. The walk counts
     * DETECTIONS (parent records): archive inventory children (S-032)
     * carry the parent's format tag but are inner entries of ONE
     * analyser detection - the og's map ticks per analyser accept, so
     * nodes whose parent is a FileNode (inner entries) are skipped. */
    std::map<u32, u64> walked;
    std::function<void(tree::FileNode*)> collect = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            if (c->parent() != nullptr && !c->parent()->is_full()) {
                collect(c); /* an inner entry - not a detection */
                continue;
            }
            const u32 fmt = c->props()->carve_format;
            if (fmt < 0xFFFF0000u)
                walked[fmt]++;
            else if (fmt >= 0xFFFF0000u + rawsigns::kFormatCount)
                walked[0xFFFE0000u]++; /* the user-sign aggregate class */
            else
                walked[u32(registry_size()) + (fmt - 0xFFFF0000u)]++;
            collect(c);
        }
    };
    collect(root);
    u64 walked_sum = 0;
    for (const auto& kv : walked) {
        walked_sum += kv.second;
        ASSERT_LT(kv.first, stats.per_format_slots) << "a tree row outside the counting space";
        EXPECT_EQ(stats.per_format[kv.first], kv.second)
            << "row " << kv.first << ": the tally must match the tree walk";
    }
    EXPECT_EQ(walked_sum, stats.validated);
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

/* ---- M-021: the post-carve processing hook (the og's EUVideoRepair
 * binding at controller+0x158; the deferred-poll feed) ---- */

/* a carvable QuickTime MOV: ftyp box at +0 with brand "qt  " (the box
 * SIZE at candidate+0, "ftyp" TYPE at +4 - the S-017 registry shape),
 * then a well-formed moov + mdat box walk */
static void plant_mov(std::vector<u8>& img, size_t at) {
    const auto be32 = [&img](size_t p, u32 v) {
        img[p] = u8(v >> 24);
        img[p + 1] = u8(v >> 16);
        img[p + 2] = u8(v >> 8);
        img[p + 3] = u8(v);
    };
    be32(at, 20); /* ftyp box size */
    std::memcpy(img.data() + at + 4, "ftyp", 4);
    std::memcpy(img.data() + at + 8, "qt  ", 4);  /* brand */
    be32(at + 12, 0x200);                         /* minor version */
    std::memcpy(img.data() + at + 16, "qt  ", 4); /* compatible brand */
    be32(at + 20, 8 + 40);                        /* moov box size (48) */
    std::memcpy(img.data() + at + 24, "moov", 4);
    for (int i = 0; i < 40; ++i)
        img[at + 28 + i] = 0; /* moov payload */
    be32(at + 68, 8 + 56);    /* mdat box size (64) */
    std::memcpy(img.data() + at + 72, "mdat", 4);
    for (int i = 0; i < 56; ++i)
        img[at + 76 + i] = 0x5A;
    /* terminator: an over-cap box size right after the stream so the
     * bmff walk ends at the mdat (the 0xAA filler alone would be walked
     * as a 2.8 GiB "box" still inside the 4 GiB cap) */
    be32(at + 132, 0xFFFFFFFF);
}

namespace {
class RecordingProcessor final : public CarvePostProcessor {
public:
    struct Hit {
        u32 format_tag;
        std::string ext;
        u64 offset, length;
    };
    std::vector<Hit> hits;
    u32 version_seen = 0;
    void on_candidate(IStorageDevice* slice, const CarveCandidateInfo& info) override {
        (void)slice;
        version_seen = api_version();
        hits.push_back({info.format_tag, info.format_ext, info.offset, info.length});
    }
};
} // namespace

TEST(CarveEngine, VideoPostProcessorHookInvoked) {
    /* the og feeds every finalized carve candidate to the EUVideoRepair
     * binding when present (GetVideoInfo filters videos; FUN_18000d670
     * :10130-10136) - a registered processor sees the MOV candidate with
     * its stream location (the GetVideoInfo analog: the processor can
     * read the bytes through the slice at the fed offset) */
    /* 0xAA filler: a zero tail would read as a size-0 "to-EOF" box and
     * stretch the extent to the format cap; garbage ends the walk */
    std::vector<u8> img(64 * 1024, 0xAA);
    plant_mov(img, 512);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    RecordingProcessor proc;
    flags.post_processor = &proc;
    CarveStats stats;
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
        OK);
    /* the fixture's well-formed moov box ALSO anchors the S-017
     * naked-moov salvage row - two finalized candidates, and the hook
     * sees BOTH (the og's poll feeds every finalized object) */
    ASSERT_EQ(stats.validated, 2u);
    ASSERT_EQ(proc.hits.size(), 2u) << "every finalized candidate reached the hook";
    /* hits arrive in scan order: the ftyp-anchored MOV first, then the
     * moov-anchored naked salvage */
    EXPECT_EQ(proc.hits[0].format_tag, u32(FMT_MOV));
    EXPECT_EQ(proc.hits[0].ext, "mov");
    EXPECT_EQ(proc.hits[0].offset, 512u);
    EXPECT_EQ(proc.hits[0].length, 132u) << "ftyp(20) + moov(48) + mdat(64)";
    EXPECT_EQ(proc.hits[1].format_tag, u32(FMT_MOV_NAKED));
    EXPECT_EQ(proc.hits[1].offset, 532u) << "the moov box start";
    EXPECT_EQ(proc.hits[1].length, 112u) << "moov(48) + mdat(64)";
    EXPECT_EQ(proc.version_seen, 1u);
    /* the hook is a FEED, not a gate: the nodes are emitted regardless */
    std::vector<tree::FileNode*> all;
    std::function<void(tree::FileNode*)> collect = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            all.push_back(c);
            collect(c);
        }
    };
    collect(root);
    ASSERT_EQ(all.size(), 2u);
    EXPECT_EQ(all[0]->props()->carve_format, u32(FMT_MOV));
}

TEST(CarveEngine, HookDegradesWhenModuleAbsent) {
    /* the og's DLL-absent degrade: no binding -> null at controller+0x158
     * -> the pipeline runs identically (raw emission, no crash, no
     * behavioral delta) */
    std::vector<u8> img(64 * 1024, 0xAA); /* to-EOF-box guard, as above */
    plant_mov(img, 512);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags; /* post_processor null: the unbound default */
    CarveStats stats;
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
        OK);
    EXPECT_EQ(stats.validated, 2u) << "unbound hook changes nothing";
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
        if (nm.find(".PST") != std::string::npos) {
            ++pst;
            EXPECT_EQ(4096u, c->props()->extents[0].offset);
            EXPECT_EQ(u64(128 * 1024), c->props()->extents[0].length);
        }
        if (nm.find(".RAR5") != std::string::npos) {
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
    /* S-049: the og contract - each format's first carve is
     * <UPPEREXT>_000001.<UPPEREXT> with its own per-format counter */
    EXPECT_TRUE(found.count("PSD_000001.PSD"));
    EXPECT_TRUE(found.count("MP3_000001.MP3"));
    EXPECT_TRUE(found.count("WAV_000001.WAV"));
    EXPECT_TRUE(found.count("WMV_000001.WMV"));
    EXPECT_TRUE(found.count("FLV_000001.FLV"));
    EXPECT_TRUE(found.count("CAB_000001.CAB"));
    EXPECT_TRUE(found.count("ORF_000001.ORF"));
    /* ISO detected via its late magic */
    bool iso_found = false;
    for (const auto& f : found)
        if (f.find(".ISO") != std::string::npos)
            iso_found = true;
    EXPECT_TRUE(iso_found);
    /* negative: the RIFF blob must be WAV, not WEBP/AVI (form-type checked) */
    for (const auto& f : found) {
        EXPECT_EQ(std::string::npos, f.find(".WEBP"));
        EXPECT_EQ(std::string::npos, f.find(".AVI"));
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
        if (c->name_utf8().find(".PDF") == std::string::npos)
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
        if (nm.size() > 7 && nm.compare(nm.size() - 7, 7, ".SAMPLE") == 0)
            sample_n = c;
        if (nm.size() > 4 && nm.compare(nm.size() - 4, 4, ".FST") == 0)
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
        if (nm.size() > 7 && nm.compare(nm.size() - 7, 7, ".SQLITE") == 0)
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
        if (nm.size() > 4 && nm.compare(nm.size() - 4, 4, ".VOB") == 0)
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
        if (nm.size() > 4 && nm.compare(nm.size() - 4, 4, ".APK") == 0)
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
    /* chain end = 30 + 21 name + 256 data = 307 bytes. N-008: the og's
     * salvage extension then absorbs data-bearing trailing sectors
     * (decompile :18492-500) - this multi-format image carries the next
     * fixture's bytes right after, so the span extends sector-wise
     * through them until a zero-tailed sector (or the cap); the exact
     * end is layout-dependent here, the dedicated
     * ZipSalvageAbsorbsDataSectors test pins the precise semantics. */
    EXPECT_GE(zip->props()->logical_size, size_t(30 + 21 + 256));
    EXPECT_LE(zip->props()->carve_confidence, 700u); /* low confidence */
}

TEST(CarveEngine, ZipSalvageAbsorbsDataSectors) {
    /* N-008 (decompile :18492-18508): after a BROKEN chain the og
     * absorbs data-bearing trailing sectors - entry requires the window
     * at [chain_end, +512) to carry a non-zero tail dword; a window
     * whose trailing 16 bytes are all zero ends the absorption and is
     * never absorbed; cap 0x500001. */
    auto img = std::vector<u8>(1 * 1024 * 1024, 0);
    const size_t at = 4096;
    std::memcpy(img.data() + at, "PK\x03\x04", 4);
    put16(img, at + 26, 5);
    put16(img, at + 28, 0);
    put32v(img, at + 18, 64);
    put32v(img, at + 22, 64);
    std::memcpy(img.data() + at + 30, "a.bin", 5);
    std::memset(img.data() + at + 35, 0x5A, 64);
    const size_t chain_end = at + 99;
    /* the window [chain_end, chain_end+512) is data-bearing */
    std::memset(img.data() + chain_end, 0x33, 512);
    /* the NEXT window is zero-tailed (the image is zero beyond) */
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats s;
    ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, s),
              OK);
    tree::FileNode* zip = nullptr;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
        if (c->props() && !c->props()->extents.empty() &&
            c->props()->extents.front().offset == at && c->props()->carve_format == FMT_ZIP)
            zip = c;
    ASSERT_NE(zip, nullptr) << "the torn indexless zip must carve";
    EXPECT_EQ(zip->props()->extents.front().length, size_t(99 + 512))
        << "the chain (99 B) + ONE data-bearing 512 B window absorbed; the zero-tailed window is "
           "not";
}

TEST(CarveEngine, Zip64ExtraFieldContinuesChain) {
    /* N-008: the indexless chain walk continues through ZIP64 entries -
     * csize 0xFFFFFFFF in the LFH resolves through the extra field's
     * 0x0001 record (usize then csize, 8 bytes each); the old walk broke
     * on the sentinel and truncated the archive. */
    auto img = std::vector<u8>(1 * 1024 * 1024, 0);
    const size_t at = 4096;
    std::memcpy(img.data() + at, "PK\x03\x04", 4);
    put16(img, at + 26, 7);
    put16(img, at + 28, 24);
    put32v(img, at + 18, 0xFFFFFFFF);
    put32v(img, at + 22, 0xFFFFFFFF);
    std::memcpy(img.data() + at + 30, "z64.bin", 7);
    {
        const size_t ex = at + 30 + 7;
        put16(img, ex, 0x0001);
        put16(img, ex + 2, 16);
        put32v(img, ex + 4, 64);
        put32v(img, ex + 8, 0);
        put32v(img, ex + 12, 64);
        put32v(img, ex + 16, 0);
    }
    std::memset(img.data() + at + 61, 0x42, 64);
    const size_t at2 = at + 30 + 7 + 24 + 64;
    std::memcpy(img.data() + at2, "PK\x03\x04", 4);
    put16(img, at2 + 26, 7);
    put16(img, at2 + 28, 0);
    put32v(img, at2 + 18, 32);
    put32v(img, at2 + 22, 32);
    std::memcpy(img.data() + at2 + 30, "z65.bin", 7);
    std::memset(img.data() + at2 + 37, 0x43, 32);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats s;
    ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, s),
              OK);
    tree::FileNode* zip = nullptr;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
        if (c->props() && !c->props()->extents.empty() &&
            c->props()->extents.front().offset == at && c->props()->carve_format == FMT_ZIP)
            zip = c;
    ASSERT_NE(zip, nullptr) << "the ZIP64 indexless archive must carve";
    bool has_first = false, has_second = false;
    for (tree::FileNode* e = zip->first_child(); e; e = e->next_sibling()) {
        if (e->props()->name == u"z64.bin")
            has_first = true;
        if (e->props()->name == u"z65.bin")
            has_second = true;
    }
    EXPECT_TRUE(has_first) << "entry 1 in the inventory";
    EXPECT_TRUE(has_second) << "the chain continued past the ZIP64 sentinel into entry 2";
}

TEST(CarveEngine, ZipCleanArchiveNotInflatedByZeroSlack) {
    /* N-008/N-027d: the og NEVER extends a CLEAN archive past its EOCD -
     * the old 16-byte-granular zero-absorption (the pre-N-008 core's
     * invented behavior) inflated intact ZIPs with trailing zero slack.
     * A clean archive with 128 bytes of zeros after the EOCD carves to
     * exactly its EOCD end. */
    auto img = build_carve_image();
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
    EXPECT_EQ(zip->props()->logical_size, size_t(140)) << "zero slack must NOT be absorbed";
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

TEST(CarveEngine, OverlapScorePrefersHigherConfidence) {
    /* N-070: at a position where several magics coincide the og emits the
     * highest accumulated header+tail-confidence candidate (the engine
     * "accumulates header + tail confidences to rank candidate carves
     * (multiple overlapping candidates resolved by score)" -
     * searchdll-family--architecture.md:73), not whichever registry row
     * sits first in trial order. Discriminating fixture: II*\0 with BOTH
     * the "CR" sub-magic at +8 (CR2 accepts: base 850 + validator 850 =
     * 1700) and a DNGVersion tag 0xC612 in the IFD (DNG accepts: base
     * 880 + validator 880 = 1760). Pre-fix the trial loop broke at the
     * first accept (CR2 - kTrialOrder puts it before DNG); post-fix the
     * accumulated score picks DNG. */
    std::vector<u8> img(2 * 1024 * 1024, 0);
    const size_t at = 1024 * 1024;
    u8* t = img.data() + at;
    auto w16 = [&](size_t o, u16 v) {
        t[o] = u8(v);
        t[o + 1] = u8(v >> 8);
    };
    auto w32 = [&](size_t o, u32 v) {
        for (int i = 0; i < 4; ++i)
            t[o + i] = u8(v >> (8 * i));
    };
    std::memcpy(t, "II\x2A\x00", 4);
    w32(4, 16); /* IFD offset */
    t[8] = 'C';
    t[9] = 'R';      /* CR2 sub-magic (validate_cr2's +8 probe) */
    w16(16, 1);      /* one IFD entry */
    w16(18, 0xC612); /* DNGVersion (validate_dng's accept tag) */
    w16(20, 1);      /* type BYTE */
    w32(22, 8);      /* count > 4: data at the value offset */
    w32(26, 1200);   /* value = data offset */
    w32(30, 0);      /* next IFD = 0 */
    for (int i = 0; i < 8; ++i)
        t[1200 + i] = 0x22;
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats s;
    ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, s),
              OK);
    int nodes_at = 0;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling()) {
        if (c->props() && !c->props()->extents.empty() &&
            c->props()->extents.front().offset == at) {
            ++nodes_at;
            EXPECT_EQ(c->props()->carve_format, u32(FMT_DNG))
                << "the higher accumulated score (DNG 1760 vs CR2 1700) must win the position";
            EXPECT_EQ(c->props()->logical_size, size_t(1208));
        }
    }
    EXPECT_EQ(nodes_at, 1) << "exactly one winner per overlap position";
    EXPECT_EQ(s.validated, 1u);
}

TEST(CarveEngine, TiffVsRawTiebreak) {
    /* N-070 companion pin: the census's own collision family (TIFF `II*.`
     * vs camera RAW, rawsearchdll--analyser-registry-census.md:53) has a
     * deterministic winner for the PLAIN case - no CR sub-magic and no
     * DNGVersion: CR2 and DNG both reject, TIFF accepts (800 + 800).
     * This outcome is identical pre- and post-fix (single acceptor); the
     * test pins that the scoring change did not disturb it. */
    std::vector<u8> img(2 * 1024 * 1024, 0);
    const size_t at = 1024 * 1024;
    u8* t = img.data() + at;
    auto w16 = [&](size_t o, u16 v) {
        t[o] = u8(v);
        t[o + 1] = u8(v >> 8);
    };
    auto w32 = [&](size_t o, u32 v) {
        for (int i = 0; i < 4; ++i)
            t[o + i] = u8(v >> (8 * i));
    };
    std::memcpy(t, "II\x2A\x00", 4);
    w32(4, 8);       /* IFD offset */
    w16(8, 1);       /* one entry */
    w16(10, 0x0100); /* ImageWidth */
    w16(12, 3);      /* type SHORT */
    w32(14, 5);      /* count > 4: data at the value offset */
    w32(18, 1200);   /* value = data offset */
    w32(22, 0);      /* next IFD = 0 */
    for (int i = 0; i < 10; ++i)
        t[1200 + i] = 0x33;
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats s;
    ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, s),
              OK);
    bool tiff = false;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling()) {
        if (c->props() && !c->props()->extents.empty() &&
            c->props()->extents.front().offset == at) {
            EXPECT_EQ(c->props()->carve_format, u32(FMT_TIFF));
            /* M-056: the out-of-line entry (count 5 SHORTs at +1200) ends
             * the file at 1210 RELATIVE - the pre-fix absolute-basis end
             * reported hit + 1210 (the candidate offset added to its own
             * size; on real disks the 256 MiB cap then absorbed the
             * neighborhood). */
            EXPECT_EQ(c->props()->logical_size, size_t(1210));
            tiff = true;
        }
    }
    EXPECT_TRUE(tiff) << "plain II*\\0 file: TIFF is the single acceptor (deterministic winner)";
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
    /* S-049: carved names carry the og's UPPERCASE extension
     * (MP4_000001.MP4) - match case-insensitively so the helper serves
     * both the registry's lowercase exts and the carved names */
    std::string dot_ext = std::string(".") + ext;
    for (char& c : dot_ext)
        if (c >= 'a' && c <= 'z')
            c = char(c - 32);
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling()) {
        std::string nm = c->name_utf8();
        for (char& ch : nm)
            if (ch >= 'a' && ch <= 'z')
                ch = char(ch - 32);
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

/* A-010: a grammar-valid ASF (the BruteForce-family walker's fixture).
 * Header Object (size hsz, one declared sub-object) + `n` Data Objects,
 * then one of three tails: a valid unknown-GUID object (grammar break
 * with sane bytes), garbage, or a torn object whose claimed size runs
 * past the device end. Returns the expected carve extent (one past the
 * last COMPLETE valid object). */
size_t plant_asf(std::vector<u8>& img, size_t at, int data_objects, const char* tail = "unknown",
                 u64 torn_size = 0, u32 header_count = 1, u64 declared_size = 0,
                 bool zero_header_size = false) {
    static const u8 asf_guid[16] = {0x30, 0x26, 0xB2, 0x75, 0x8E, 0x66, 0xCF, 0x11,
                                    0xA6, 0xD9, 0x00, 0xAA, 0x00, 0x62, 0xCE, 0x6C};
    static const u8 data_guid[16] = {0x36, 0x26, 0xB2, 0x75, 0x8E, 0x66, 0xCF, 0x11,
                                     0xA6, 0xD9, 0x00, 0xAA, 0x00, 0x62, 0xCE, 0x6C};
    size_t p = at;
    /* Header Object: GUID + size (100) + count + 2 reserved + payload */
    std::memcpy(img.data() + p, asf_guid, 16);
    const u64 hsz = zero_header_size ? 0 : 100;
    for (int i = 0; i < 8; ++i)
        img[p + 16 + size_t(i)] = u8(hsz >> (8 * i));
    for (int i = 0; i < 4; ++i)
        img[p + 24 + size_t(i)] = u8(header_count >> (8 * i));
    img[p + 28] = 1; /* broadcast flag (arbitrary non-reserved byte) */
    for (u32 i = 30; i < hsz && i < img.size() - p; ++i)
        img[p + size_t(i)] = 0x44;
    /* N-212: an optional File Properties child at +30 (the og's +0x174
     * GUID) carrying the declared total file size at child+0x28 (the ASF
     * spec's file_size; the og's AVX extraction site) */
    if (declared_size != 0) {
        static const u8 fprops_guid[16] = {0xA1, 0xDC, 0xAB, 0x8C, 0x47, 0xA9, 0xCF, 0x11,
                                           0x8E, 0xE4, 0x00, 0xC0, 0x0C, 0x20, 0x53, 0x65};
        const size_t fp = p + 30;
        std::memcpy(img.data() + fp, fprops_guid, 16);
        const u64 fsz = 0x30;
        for (int i = 0; i < 8; ++i)
            img[fp + 16 + size_t(i)] = u8(fsz >> (8 * i));
        for (int i = 0; i < 8; ++i)
            img[fp + 0x28 + size_t(i)] = u8(declared_size >> (8 * i));
    }
    p += size_t(hsz);
    /* Data Objects: GUID + size + payload (sizes 200/150/300/...) */
    static const u64 sizes[] = {200, 150, 300, 240};
    for (int d = 0; d < data_objects; ++d) {
        std::memcpy(img.data() + p, data_guid, 16);
        const u64 sz = sizes[d % 4];
        for (int i = 0; i < 8; ++i)
            img[p + 16 + size_t(i)] = u8(sz >> (8 * i));
        for (u64 i = 24; i < sz; ++i)
            img[p + size_t(i)] = u8(0x55 + d);
        p += size_t(sz);
    }
    if (tail == nullptr || std::strcmp(tail, "none") == 0)
        return p - at;
    if (std::strcmp(tail, "garbage") == 0) {
        for (int i = 0; i < 64; ++i)
            img[p + size_t(i)] = 0xAA;
        return p - at;
    }
    if (std::strcmp(tail, "torn") == 0) {
        /* a Data Object whose claimed size runs past the device end */
        std::memcpy(img.data() + p, data_guid, 16);
        const u64 sz = torn_size ? torn_size : (1ull << 40);
        for (int i = 0; i < 8; ++i)
            img[p + 16 + size_t(i)] = u8(sz >> (8 * i));
        return p - at; /* the torn object itself is NOT salvage */
    }
    /* "unknown": a well-formed object with an unrecognized GUID */
    for (int i = 0; i < 16; ++i)
        img[p + size_t(i)] = u8(0xE0 + i);
    const u64 usz = 128;
    for (int i = 0; i < 8; ++i)
        img[p + 16 + size_t(i)] = u8(usz >> (8 * i));
    for (u64 i = 24; i < usz; ++i)
        img[p + size_t(i)] = 0x66;
    return p - at;
}

/* S-037: a FreeHand 11 header - the og's FUN_180029f30 constant set:
 * registry key 1C 01 00 00, u64 @+4 = 0x020014011C040002,
 * u32 @+0xC = 0x011C1400, u16 @+0x22 = "11". */
size_t plant_fh11(std::vector<u8>& img, size_t at, size_t payload = 4096) {
    std::memcpy(img.data() + at, "\x1C\x01\x00\x00", 4);
    const u64 v8 = 0x020014011C040002ULL;
    for (int i = 0; i < 8; ++i)
        img[at + 4 + size_t(i)] = u8(v8 >> (8 * i));
    put32v(img, at + 0xC, 0x011C1400);
    img[at + 0x22] = '1';
    img[at + 0x23] = '1';
    for (size_t i = 0x24; i < 0x24 + payload; ++i)
        img[at + i] = u8(i);
    return 0x24 + payload;
}

/* S-037: a Corel PhotoPaint container - the og's FUN_180043630 Analyse
 * gate ("FILE" @ +4, 0x13C @ +0x34) + the FUN_1800436e0 pointer-chase:
 * n @ +0x28 entries of 8 bytes at table 0x13C; the LAST entry points at
 * the final block; that block's +0x20/+0x3c pair (nonzero, equal) adds
 * the 0x3c+val rebasing; the data-section pointer pair at base yields
 * size = (len at t2-8) + (extras at t2-4). */
size_t plant_cpt(std::vector<u8>& img, size_t at, u32 gen /* '7'/'8'/'9' */) {
    size_t p = at;
    std::memcpy(img.data() + p, "CPT", 3);
    img[p + 3] = u8(gen);
    std::memcpy(img.data() + p + 4, "FILE", 4); /* +4 the Analyse magic */
    /* +0x28: the pointer-table count (2 entries) */
    put32v(img, p + 0x28, 2);
    /* +0x34: the version gate == the table offset (0x13C) */
    put32v(img, p + 0x34, 0x13C);
    /* the pointer table at 0x13C: 2 entries x 8 bytes; entry 0 = 0x40,
     * entry 1 = block_off (the walk reads only the LAST) */
    const u64 block_off = 0x200; /* the final block lives here */
    put32v(img, p + 0x13C, 0x40);
    put32v(img, p + 0x13C + 8, u32(block_off));
    /* the final block: +0x20 tail check, +0x3c the same value (the
     * rebasing path), then base = block + 0x3c + val */
    const u32 tail_val = 0x10; /* rebasing adds 0x10 */
    put32v(img, p + block_off + 0x20, tail_val);
    put32v(img, p + block_off + 0x3C, tail_val);
    const u64 base = block_off + 0x3C + tail_val; /* 0x24C */
    /* the data-section descriptor at base: t2 (a pointer), t3 (nonzero);
     * size = u32 @ (t2-8) + u32 @ (t2-4) */
    const u32 t2 = 0x400; /* the data descriptor */
    put32v(img, p + base, t2);
    put32v(img, p + base + 4, 1);
    const u32 len = 0x3000;  /* the data length */
    const u32 extras = 0x40; /* trailing extras */
    put32v(img, p + t2 - 8, len);
    put32v(img, p + t2 - 4, extras);
    const size_t total = size_t(t2) + len + extras;
    /* some non-zero texture strictly between the predicate block and the
     * pointer table (0x38..0x13B) - never touching any structure; keeps
     * the fixture hostile to zero-skip fast paths without clobbering
     * the LE fields whose high/low bytes may be zero */
    for (size_t i = p + 0x40; i < p + 0x13B; ++i)
        if (img[i] == 0)
            img[i] = u8(i & 0xFF);
    return total;
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
    img[p + 3] = 0xBB;      /* system header */
    putbe16(img, p + 4, 6); /* length - N-156: the BE u16 wire form */
    for (int i = 0; i < 6; ++i)
        img[p + 6 + size_t(i)] = 0;
    p += 6 + 6;
    img[p + 0] = 0;
    img[p + 1] = 0;
    img[p + 2] = 1;
    img[p + 3] = 0xE0;      /* video packet */
    putbe16(img, p + 4, 8); /* PES length - N-156: the BE u16 wire form */
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
    /* M-107 member 3: the plant now carries a REAL section-locator map
     * (the CRW-landing precedent - the header-only plant exercised the
     * stand-in era's blind acceptance; the walk declines noise). The
     * map: count=1 @ 0x15, record {0, seeker 0x800, size 0x800} from
     * 0x19 -> the og walk's extent = 0x1000 (the batch's old length). */
    std::memcpy(img.data() + at, "AC1015", 6);
    std::memcpy(img.data() + at + 6, "\x0D\x0A\x0D\x0A\x00", 5);
    put32v(img, at + 0x15, 1);
    img[at + 0x19] = 0; /* record 0's number byte */
    put32v(img, at + 0x1A, 0x800);
    put32v(img, at + 0x1E, 0x800);
    for (size_t i = 0x30; i < 4096; ++i)
        img[at + i] = u8((i * 31) & 0xFF);
    return 0x800 + 0x800; /* the og walk's extent: max(seeker+size) */
}

/* ---- S-007 niche batch 1 plants (SCAN-2f) --------------------------- */

/* SMF: MThd + one complete MTrk. */
size_t plant_mid(std::vector<u8>& img, size_t at, u32 ntrks_declared = 1, int complete = 1) {
    std::memcpy(img.data() + at, "MThd", 4);
    putbe32(img, at + 4, 6);
    putbe16(img, at + 8, 0); /* format 0 */
    putbe16(img, at + 10, u16(ntrks_declared));
    putbe16(img, at + 12, 480); /* division */
    size_t p = at + 14;
    for (int t = 0; t < complete; ++t) {
        std::memcpy(img.data() + p, "MTrk", 4);
        putbe32(img, p + 4, 16);
        for (int i = 0; i < 16; ++i)
            img[p + 8 + size_t(i)] = u8(0x40 + i);
        p += 8 + 16;
    }
    return p - at;
}

/* Ogg: a BOS page + (optionally) an EOS page. */
size_t plant_ogg(std::vector<u8>& img, size_t at, bool with_eos = true) {
    auto page = [&](size_t p, u8 htype, u8 seglen) {
        std::memcpy(img.data() + p, "OggS", 4);
        img[p + 4] = 0; /* version */
        img[p + 5] = htype;
        for (int i = 0; i < 16; ++i)
            img[p + 6 + size_t(i)] = u8(i); /* granule..CRC noise */
        img[p + 26] = 1;                    /* one segment */
        img[p + 27] = seglen;
        for (int i = 0; i < seglen; ++i)
            img[p + 28 + size_t(i)] = u8(0x30 + i);
        return 28 + size_t(seglen);
    };
    size_t p = at;
    p += page(p, 0x02, 30);
    if (with_eos)
        p += page(p, 0x04, 8);
    return p - at;
}

/* RealMedia: .RMF lead-in + one PROP chunk. */
size_t plant_rm(std::vector<u8>& img, size_t at) {
    std::memcpy(img.data() + at, ".RMF", 4);
    putbe32(img, at + 4, 1); /* version 1 */
    putbe32(img, at + 8, 1); /* n-headers sanity */
    std::memcpy(img.data() + at + 0x10, "PROP", 4);
    putbe32(img, at + 0x14, 0x28); /* chunk total size */
    for (size_t i = 0x18; i < 0x38; ++i)
        img[at + i] = u8(i & 0xFF);
    return 0x38;
}

/* MXF: two minimal KLV items. */
size_t plant_mxf(std::vector<u8>& img, size_t at) {
    for (int item = 0; item < 2; ++item) {
        const size_t p = at + size_t(item) * 25;
        std::memcpy(img.data() + p,
                    "\x06\x0E\x2B\x34\x02\x05\x01\x01\x0D\x01\x02\x01\x01\x00\x00\x00", 16);
        img[p + 16] = 8; /* short-form BER length */
        for (int i = 0; i < 8; ++i)
            img[p + 17 + size_t(i)] = u8(0x10 + i);
    }
    return 50;
}

/* A level-1 bzip2 stream: BZh1 + block pi magic (+ optional footer). */
size_t plant_dmg(std::vector<u8>& img, size_t at, bool with_footer = true) {
    std::memcpy(img.data() + at, "BZh1", 4);
    std::memcpy(img.data() + at + 4, "\x31\x41\x59\x26\x53\x59", 6);
    for (size_t i = 10; i < 128; ++i)
        img[at + i] = u8(i & 0xFF);
    if (with_footer) {
        std::memcpy(img.data() + at + 128, "\x17\x72\x45\x38\x50\x90", 6);
        put32v(img, at + 134, 0x12345678);
        return 138;
    }
    return 128;
}

/* M-090: a real UDIF shape - BZh1 stream, zero padding, then the 512-byte
 * "koly" footer block at a 512-aligned offset (or unaligned for the
 * salvage form). The og sizes the carve koly + 0x200
 * (FUN_180053de0 :63782-63868). */
size_t plant_dmg_koly(std::vector<u8>& img, size_t at, size_t koly_rel, bool aligned = true,
                      bool zero_tail = true) {
    std::memcpy(img.data() + at, "BZh1", 4);
    std::memcpy(img.data() + at + 4, "\x31\x41\x59\x26\x53\x59", 6);
    for (size_t i = 10; i < 128; ++i)
        img[at + i] = u8(i & 0xFF);
    const size_t k = at + koly_rel;
    std::memcpy(img.data() + k, "koly", 4);
    put32v(img, k + 4, 4);   /* version 4 */
    put32v(img, k + 8, 512); /* header size */
    for (size_t i = 12; i < 504; ++i)
        img[k + i] = u8(i & 0x7F); /* footer fields (non-zero body) */
    if (!zero_tail)
        for (size_t i = 504; i < 512; ++i)
            img[k + i] = 0xAB; /* a non-zero tail breaks the og's gate */
    (void)aligned;
    return koly_rel + 0x200;
}

/* M-107: the X3F plant carries the section directory (the og walk's
 * target - FileX3FAnalyser FUN_1800535b0, the M-090-precedent authority):
 * header(16) + payload + the directory at a variable position after the
 * data: "SECd" + size + count + pad, then count 12-byte entries
 * {offset, length, name}. Sigma writes the directory after the section
 * data - the og SEARCHES for it (sector-stepped with 2-sector overlap),
 * so the plant places it at the true end. */
size_t plant_x3f(std::vector<u8>& img, size_t at, size_t payload_len = 0x800, u32 dir_entries = 3,
                 u64 dir_offset_override = 0, bool binary_version = false) {
    std::memcpy(img.data() + at, "FOVb", 4);
    if (binary_version) {
        /* the BINARY u32 version (0x00020001 = v2.1) - real Sigma X3F
         * files carry binary versions; the modern generation's tier-1
         * gate (FUN_180040080: bytes +5/+7 == 0) accepts exactly this
         * shape (M-156's chain router; the ASCII form exercises only the
         * legacy generation) */
        img[at + 4] = 0x01;
        img[at + 5] = 0x00;
        img[at + 6] = 0x02;
        img[at + 7] = 0x00;
    } else {
        std::memcpy(img.data() + at + 4, "2.1\x00", 4);
    }
    for (size_t i = 8; i < 16; ++i)
        img[at + i] = u8(i & 0xFF);
    /* benign payload fill: a constant that forms no 4-byte registry
     * signature (the stride patterns cycle all 256 values and collide
     * with other rows' keys inside the shared batch image) */
    for (size_t i = 0; i < payload_len; ++i)
        img[at + 16 + i] = 0x44;
    const size_t d = dir_offset_override ? size_t(dir_offset_override) : at + 16 + payload_len;
    std::memcpy(img.data() + d, "SECd", 4);
    const u32 table_bytes = 16 + dir_entries * 12;
    for (int i = 0; i < 4; ++i) {
        img[d + 4 + size_t(i)] = u8(table_bytes >> (8 * i)); /* size */
        img[d + 8 + size_t(i)] = u8(dir_entries >> (8 * i)); /* count */
        img[d + 12 + size_t(i)] = 0;                         /* pad */
    }
    for (u32 e = 0; e < dir_entries; ++e) {
        const size_t ep = d + 16 + size_t(e) * 12;
        const u32 sec_off = u32(16 + e * 0x100);
        const u32 sec_len = 0x100;
        for (int i = 0; i < 4; ++i) {
            img[ep + size_t(i)] = u8(sec_off >> (8 * i));     /* entry offset */
            img[ep + 4 + size_t(i)] = u8(sec_len >> (8 * i)); /* entry length */
        }
        std::memcpy(img.data() + ep + 8, "SECi", 4); /* entry name */
    }
    return d + 16 + size_t(dir_entries) * 12 - at; /* the directory's end */
}

/* ---- M-107: the X3F section-directory size walk (the FileX3FAnalyser
 * authority - FUN_1800535b0, SECd + 0x4C in all three found-branches;
 * the S71/M-155 corpus resolution of the two racing landings) ---- */

TEST(Carve, X3fExtentIsSecdPlusFixedTail) {
    /* the og's walk: the extent is SECd_position + 0x4C - the
     * directory's fixed 76-byte tail (FUN_1800535b0's `uVar7 + 0x4c`
     * and both search-hit `piVar5 + ... + 0x4c` forms) - NOT a
     * function of the entry count (the count is never read). */
    std::vector<u8> img(256 * 1024, 0);
    const size_t at = 512;
    plant_x3f(img, at, 0x800, 3);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"L");
    CarveFlags flags;
    CarveStats stats;
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
        OK);
    tree::FileNode* x3f = nullptr;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
        if (c->props() && c->props()->extents.front().offset == at)
            x3f = c;
    ASSERT_NE(x3f, nullptr) << "the X3F must carve through the directory walk";
    EXPECT_EQ(x3f->props()->logical_size, 16 + 0x800 + 0x4C)
        << "extent = SECd + the fixed 0x4C tail, count-independent (og-exact)";
}

TEST(Carve, X3fNoSectionDirectorySalvagesWalkedPrefix) {
    /* the og's loop-exit shape: no directory in range stamps the
     * walked prefix as the size with the format id regardless
     * (FUN_1800535b0's post-loop `*(param_2 + 0x10) = uVar7` +
     * `*(param_2 + 0x18) = 0x6c`) - never a refusal. On a small device
     * the walk's bound is the device end. */
    std::vector<u8> img(256 * 1024, 0);
    const size_t at = 512;
    std::memcpy(img.data() + at, "FOVb", 4);
    std::memcpy(img.data() + at + 4, "2.1\x00", 4);
    for (size_t i = 8; i < 4096; ++i)
        img[at + i] = u8((i * 3) & 0xFF); /* payload, NO "SECd" anywhere */
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"L");
    CarveFlags flags;
    CarveStats stats;
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
        OK);
    tree::FileNode* x3f = nullptr;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
        if (c->props() && c->props()->extents.front().offset == at)
            x3f = c;
    ASSERT_NE(x3f, nullptr) << "the torn X3F must still carve (the loop-exit salvage)";
    EXPECT_EQ(x3f->props()->logical_size, size_t(256 * 1024 - at))
        << "the walked prefix at the walk's bound - the og's loop-exit salvage";
}

TEST(Carve, X3fGarbageCountIsIrrelevantToExtent) {
    /* the walk never reads the directory's count field (the fixed
     * 0x4C tail is the extent in every found branch): a garbage-huge
     * count changes nothing - the S78 count-walk decode is refuted. */
    std::vector<u8> img(1 * 1024 * 1024, 0);
    const size_t at = 512;
    plant_x3f(img, at, 0x800, 3);
    for (int i = 0; i < 4; ++i)
        img[at + 16 + 0x800 + 8 + size_t(i)] = u8(0xFF); /* count = 0xFFFFFF */
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"L");
    CarveFlags flags;
    CarveStats stats;
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
        OK);
    tree::FileNode* x3f = nullptr;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
        if (c->props() && c->props()->extents.front().offset == at)
            x3f = c;
    ASSERT_NE(x3f, nullptr);
    EXPECT_EQ(x3f->props()->logical_size, 16 + 0x800 + 0x4C)
        << "the garbage count does not move the fixed-tail extent";
}

/* ---- M-156: the chain's MODERN path (the binary-version router) ---- */

TEST(Carve, X3fModernCountWalkSetsExtent) {
    /* the MODERN generation (Brance::X3FAnalyser walker FUN_1800404a0,
     * binary version - the magic-key dispatch's wired path per the
     * raw-asm decode): the extent is the section directory's END (magic
     * + 16 + count*12), not the legacy fixed tail - verified at
     * 0x180040541-0x180040571 (evidence/s83-lane-sheets/
     * s83-l1-x3f-chain-raw-asm-decode.md). */
    std::vector<u8> img(256 * 1024, 0);
    const size_t at = 512;
    const size_t expect = plant_x3f(img, at, 0x800, 3, 0, true);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"L");
    CarveFlags flags;
    CarveStats stats;
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
        OK);
    tree::FileNode* x3f = nullptr;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
        if (c->props() && c->props()->extents.front().offset == at)
            x3f = c;
    ASSERT_NE(x3f, nullptr) << "the binary-version X3F must carve";
    EXPECT_EQ(x3f->props()->logical_size, expect)
        << "extent = the directory end (16 + payload + 16 + count*12) - the "
           "modern count-walk, og-exact";
}

TEST(Carve, X3fModernGarbageHugeCountClampsToBound) {
    /* the MODERN walk's clamp: a torn directory whose count field is
     * garbage-huge clamps the extent to the walk bound (the raw-asm
     * cmova clamp at 0x180040602-0x180040609; the core's stand-ins are
     * the row bound + the device end). The garbage-count IRRELEVANCE pin
     * above is the LEGACY path's pin - this is the modern path's. */
    std::vector<u8> img(1 * 1024 * 1024, 0);
    const size_t at = 512;
    (void)plant_x3f(img, at, 0x800, 3, 0, true);
    /* tear the count: 0xFFFFFF entries -> extent clamps to the device end */
    for (int i = 0; i < 4; ++i)
        img[at + 16 + 0x800 + 8 + size_t(i)] = u8(0xFF);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"L");
    CarveFlags flags;
    CarveStats stats;
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
        OK);
    tree::FileNode* x3f = nullptr;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
        if (c->props() && c->props()->extents.front().offset == at)
            x3f = c;
    ASSERT_NE(x3f, nullptr);
    EXPECT_EQ(x3f->props()->logical_size, size_t(1 * 1024 * 1024 - at))
        << "the garbage count clamps the modern walk to the window end";
}

TEST(Carve, X3fModernTornFallsBackToLegacySalvage) {
    /* the CHAIN's binary-torn shape: a binary-version file with no
     * directory - the modern generation declines (its extent field is
     * never written - the ctor zeroes [obj+0x110] and the cap-exit never
     * touches it), the chain falls to the legacy generation, whose
     * loop-exit SALVAGES the walked prefix (FUN_1800535b0's exit
     * carries the last walked position - never a refusal). */
    std::vector<u8> img(256 * 1024, 0);
    const size_t at = 512;
    std::memcpy(img.data() + at, "FOVb", 4);
    img[at + 4] = 0x01; /* binary 0x00020001 - the modern path's gate */
    img[at + 6] = 0x02;
    for (size_t i = 8; i < 4096; ++i)
        img[at + i] = u8((i * 3) & 0xFF); /* payload, NO "SECd" anywhere */
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"L");
    CarveFlags flags;
    CarveStats stats;
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
        OK);
    tree::FileNode* x3f = nullptr;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
        if (c->props() && c->props()->extents.front().offset == at)
            x3f = c;
    ASSERT_NE(x3f, nullptr) << "the binary torn X3F still carves (the legacy salvage)";
    EXPECT_EQ(x3f->props()->logical_size, size_t(256 * 1024 - at))
        << "the walked prefix at the device end - the chain's torn salvage";
}

TEST(Carve, X3fChainRoutesByVersionShape) {
    /* M-156 (the chain router): the SAME directory position carves at
     * TWO different extents depending on the version field's shape - the
     * binary u32 version (0x00020001) routes to the MODERN generation's
     * count-table walk (SECd + 16 + count*12); the ASCII "2.1" version
     * fails the modern's tier-1 (bytes +5/+7 == 0, FUN_180040080) and
     * falls to the LEGACY generation's fixed tail (SECd + 0x4C,
     * FUN_1800535b0). Both generations are real og code - the raw-asm
     * decode pins the routing (evidence/s83-lane-sheets). */
    const u64 payload = 0x800; /* 2 KiB of sections before the directory */
    const u32 entries = 3;
    const u64 expect_modern = 16 + payload + 16 + u64(entries) * 12;
    const u64 expect_legacy = 16 + payload + 0x4C;
    for (int gen = 0; gen < 2; ++gen) {
        std::vector<u8> img(64 * 1024, 0);
        const u64 at = 512;
        std::memcpy(img.data() + at, "FOVb", 4);
        if (gen == 0) {
            img[at + 4] = 0x01; /* binary 0x00020001 - the modern path */
            img[at + 6] = 0x02;
        } else {
            img[at + 4] = '2'; /* ASCII "2.1" - the legacy path */
            img[at + 5] = '.';
            img[at + 6] = '1';
        }
        const u64 sec_at = at + 16 + payload;
        std::memcpy(img.data() + sec_at, "SECd", 4);
        for (int i = 0; i < 4; ++i) {
            img[sec_at + 8 + size_t(i)] = u8(entries >> (8 * i)); /* count */
        }
        MemDevice dev(std::move(img));
        tree::ResultTree tree;
        auto* root = tree.add_root(u"Lost Files");
        CarveFlags flags;
        CarveStats stats;
        ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr,
                              stats),
                  u32(OK));
        tree::FileNode* nd = nullptr;
        for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
            if (c->props() && c->props()->extents.front().offset == at)
                nd = c;
        ASSERT_NE(nd, nullptr) << "both generations carve the directory walk";
        EXPECT_EQ(nd->props()->carve_format, u64(FMT_X3F));
        EXPECT_EQ(nd->props()->logical_size, size_t(gen == 0 ? expect_modern : expect_legacy))
            << (gen == 0 ? "binary version -> the modern count-table walk "
                           "(SECd + 16 + count*12)"
                         : "ASCII version -> the legacy fixed tail (SECd + 0x4C)");
    }
}

TEST(Carve, X3fRealSigmaBinaryVersionCarves) {
    /* M-156's headline pin: a REAL Sigma X3F file (binary u32 version
     * 0x00020001 - the shape every real camera writes) MUST carve. The
     * landed ASCII gate (S71/M-155/S81/S82) rejected this shape outright
     * - the live bug this correction fixes. RED at the pre-correction
     * tree (the ASCII gate refuses h[4]=0x01), GREEN here. */
    std::vector<u8> img(256 * 1024, 0);
    const size_t at = 512;
    const size_t expect = plant_x3f(img, at, 0x800, 3, 0, true);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"L");
    CarveFlags flags;
    CarveStats stats;
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
        OK);
    tree::FileNode* x3f = nullptr;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
        if (c->props() && c->props()->extents.front().offset == at)
            x3f = c;
    ASSERT_NE(x3f, nullptr) << "a real binary-version Sigma X3F must carve";
    EXPECT_EQ(x3f->props()->carve_format, u64(FMT_X3F));
    EXPECT_EQ(x3f->props()->logical_size, expect);
}

/* M-081/SCAN-4: Canon CRW (CIFF) - the II/MM 1A 00 keys with the og
 * grammar (00 00 HEAP + {CCDR,TIFP,ARCH} + version 2,
 * FUN_180052000 :62333-62353, first-hand). */
size_t plant_crw(std::vector<u8>& img, size_t at, const char* type4 = "CCDR", bool be = false) {
    std::memcpy(img.data() + at, be ? "MM\x1A\x00" : "II\x1A\x00", 4);
    std::memcpy(img.data() + at + 4, "\x00\x00HEAP\x00\x00", 8);
    std::memcpy(img.data() + at + 0xA, type4, 4);
    img[at + 0xE] = 2; /* the version byte the og gate demands */
    for (size_t i = 0xF; i < 0x40; ++i)
        img[at + i] = u8(i & 0xFF);
    return 0x40;
}

/* M-107/S83 (the CRW member): plant a CIFF directory entry of the og's
 * known shape {u16 type 0x2807, u32 len} at `at` - the walk's target. */
void plant_crw_dir_entry(std::vector<u8>& img, size_t at, u32 len) {
    img[at] = u8(0x07);
    img[at + 1] = u8(0x28); /* the u16 type 0x2807 the signature filter matches */
    img[at + 2] = u8(len & 0xFF);
    img[at + 3] = u8((len >> 8) & 0xFF);
    img[at + 4] = 0;
    img[at + 5] = 0; /* the u32 length: 0xa8 / 0xb2 / 0xe4 */
}

/* ---- M-107/S83: the CRW CIFF heap size walk (member 2 of 11) ---- */

TEST(Carve, CrwDirectoryEntryWalkSetsExtent) {
    /* the og's walk (FUN_180052160's walk section, raw-asm
     * 0x1800524e4-0x1800526d7; evidence/s83-lane-sheets/
     * s83-l2-crw-ciff-walk-decode.md): the extent is the CIFF directory
     * entry's position + the length's tail constant from the walker's
     * own table ({0xa8 -> 0x46, 0xb2 -> 0x3c, 0xe4 -> 0x32}) - not the
     * blind 64 MiB stand-in. */
    for (int k = 0; k < 3; ++k) {
        const u32 lens[3] = {0xa8, 0xb2, 0xe4};
        const u64 tails[3] = {0x46, 0x3c, 0x32};
        std::vector<u8> img(256 * 1024, 0);
        const size_t at = 512;
        plant_crw(img, at);
        const size_t entry_at = at + 0x1000; /* past the heap header */
        plant_crw_dir_entry(img, entry_at, lens[k]);
        MemDevice dev(std::move(img));
        tree::ResultTree tree;
        auto* root = tree.add_root(u"L");
        CarveFlags flags;
        CarveStats stats;
        ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr,
                              stats),
                  OK);
        tree::FileNode* crw = nullptr;
        for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
            if (c->props() && c->props()->extents.front().offset == at)
                crw = c;
        ASSERT_NE(crw, nullptr) << "the CRW must carve through the walk";
        EXPECT_EQ(crw->props()->logical_size, size_t(entry_at + tails[k]) - at)
            << "extent = the directory entry + the tail constant (len " << lens[k] << " -> +"
            << tails[k] << ")";
    }
}

TEST(Carve, CrwTornSalvagesTheWalkedBound) {
    /* the og's loop-exit (raw-asm 0x180052641's r15 reset + the exit at
     * 0x1800526d7): no directory entry anywhere in range -> the walked
     * bound (the walker's own 50 MiB cap, clamped by the device end) -
     * never a refusal. RED at the pre-fix tree on a >64 MiB device (the
     * old stand-in carved exactly 64 MiB); here the device end clamps
     * the small image. */
    std::vector<u8> img(256 * 1024, 0);
    const size_t at = 512;
    plant_crw(img, at);
    for (size_t i = 0x40; i < 0x2000; ++i)
        img[at + i] = u8((i * 5) & 0xFF); /* payload, NO dir entry */
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"L");
    CarveFlags flags;
    CarveStats stats;
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
        OK);
    tree::FileNode* crw = nullptr;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
        if (c->props() && c->props()->extents.front().offset == at)
            crw = c;
    ASSERT_NE(crw, nullptr) << "the torn CRW still carves (the salvage)";
    EXPECT_EQ(crw->props()->logical_size, size_t(256 * 1024 - at))
        << "the walked prefix at the device end - the og's loop-exit salvage";
}

TEST(Carve, CrwWalkCapIsTheOgsOwnConstant) {
    /* the og walker's own cap: 0x3200000 = 50 MiB (raw-asm 0x18005219d
     * `mov r15d,0x3200000`). On a 100 MiB device with no directory
     * entry, the torn salvage stops at 50 MiB - NOT the old stand-in's
     * 64 MiB and NOT the device end. */
    std::vector<u8> img(100ull << 20, 0);
    const u64 at = 512;
    plant_crw(img, size_t(at));
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"L");
    CarveFlags flags;
    CarveStats stats;
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
        u32(OK));
    tree::FileNode* crw = nullptr;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
        if (c->props() && c->props()->extents.front().offset == at)
            crw = c;
    ASSERT_NE(crw, nullptr);
    EXPECT_EQ(crw->props()->logical_size, size_t(0x3200000))
        << "the og walker's own 50 MiB cap constant - the walked bound";
}

/* M-107 member 3 (DWG): the og's three-generation walk. The plant helpers
 * build the exact structures the og's CDWGRawInfo child reads (the ctor's
 * cursor 0x15, the 9-byte section-locator records from 0x19, the 0x6c
 * rand-XOR struct at 0x20 - evidence/s85-lane-sheets/
 * s85-l1-dwg-section-walk-raw-asm-decode.md). */
void dwg_put32(u8* p, u32 v) {
    for (int i = 0; i < 4; ++i)
        p[i] = u8(v >> (8 * i));
}
void dwg_put64(u8* p, u64 v) {
    for (int i = 0; i < 8; ++i)
        p[i] = u8(v >> (8 * i));
}
void plant_dwg_header(std::vector<u8>& img, size_t at, const char ver[2]) {
    std::memcpy(img.data() + at, "AC10", 4);
    img[at + 4] = u8(ver[0]);
    img[at + 5] = u8(ver[1]);
    /* the og's tier-1 is the version table ONLY - no sentinel check
     * exists anywhere in its chain */
}
void plant_dwg_locator(std::vector<u8>& img, size_t at, u32 count,
                       const std::vector<std::array<u32, 2>>& seeker_size) {
    /* the count u32 @ 0x15 (the ctor's cursor), then the 9-byte records
     * from 0x19: {number:u8 == index, seeker:u32 @+1, size:u32 @+5} */
    dwg_put32(img.data() + at + 0x15, count);
    for (u32 i = 0; i < seeker_size.size(); ++i) {
        const size_t rec = at + 0x19 + size_t(i) * 9;
        img[rec] = u8(i); /* the number byte MUST equal the record index */
        dwg_put32(img.data() + rec + 1, seeker_size[i][0]);
        dwg_put32(img.data() + rec + 5, seeker_size[i][1]);
    }
}
/* the og's gen-2 obfuscation stream: the MSVC rand() LCG, seed 1
 * (FUN_18002f630: state = state*0x343fd + 0x269ec3, byte = state>>16).
 * The well-known seed-1 rand() sequence (41, 18467, 6334, ...) is the
 * independent oracle - the first test below pins its first four bytes. */
u8 dwg_rand_stream(u32 idx) {
    u32 st = 1;
    for (u32 i = 0; i <= idx; ++i)
        st = st * 0x343fd + 0x269ec3;
    return u8(st >> 16);
}
void plant_dwg_r2004(std::vector<u8>& img, size_t at, u64 extent_base) {
    plant_dwg_header(img, at, "18");
    u8 plain[0x6c];
    std::memset(plain, 0, sizeof(plain));
    dwg_put32(plain + 0x10, 0x6c);
    dwg_put32(plain + 0x14, 4);
    dwg_put64(plain + 0x34, extent_base);
    dwg_put32(plain + 0x44, 0x20);
    dwg_put32(plain + 0x48, 0x80);
    dwg_put32(plain + 0x4c, 0x40);
    /* the encrypted struct sits at +0x80 BYTES (raw-asm 0x18002f5a0:
     * `lea rdx,[rdi+0x80]` with rdi = the chunk base, set once at
     * 0x18002f342 and never reloaded; the Ghidra decompile's `param_2 +
     * 0x20` is INT* arithmetic = 0x80 bytes - the unscaled port repaired
     * against the asm by the convergent lane's S84 evidence sheet) */
    for (size_t i = 0; i < sizeof(plain); ++i)
        img[at + 0x80 + i] = u8(plain[i] ^ dwg_rand_stream(u32(i)));
}

TEST(Carve, DwgSectionLocatorWalkSetsExtent) {
    /* the og's gen-1 walk (FUN_18002f320 :36356-36403, first-hand this
     * session): extent = max(seeker + size) over the section-locator
     * records - NOT the blind 64 MiB stand-in. RED at the stand-in tree
     * (the extent was the device-end-clamped 64 MiB). */
    std::vector<u8> img(1 * 1024 * 1024, 0);
    const size_t at = 512;
    plant_dwg_header(img, at, "15");
    plant_dwg_locator(img, at, 3, {{0x1000, 0x2000}, {0x4000, 0x1000}, {0x2000, 0x0800}});
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"L");
    CarveFlags flags;
    CarveStats stats;
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
        OK);
    tree::FileNode* dwg = nullptr;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
        if (c->props() && c->props()->extents.front().offset == at)
            dwg = c;
    ASSERT_NE(dwg, nullptr) << "the R2000 DWG must carve through the locator walk";
    EXPECT_EQ(dwg->props()->logical_size, size_t(0x5000))
        << "extent = max(seeker+size) = max(0x3000, 0x5000, 0x2800)";
}

TEST(Carve, DwgSectionNumberMismatchDeclines) {
    /* the og's only structural gate (:36379-36383): a record whose number
     * byte != its index zeroes the extent - the decline. The stand-in
     * carved these blind. */
    std::vector<u8> img(256 * 1024, 0);
    const size_t at = 512;
    plant_dwg_header(img, at, "15");
    plant_dwg_locator(img, at, 2, {{0x1000, 0x2000}, {0x4000, 0x1000}});
    img[at + 0x19 + 9] = 7; /* record 1's number byte: WRONG */
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"L");
    CarveFlags flags;
    CarveStats stats;
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
        OK);
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
        EXPECT_FALSE(c->props() && c->props()->extents.front().offset == at)
            << "the mismatched section map must decline (extent 0)";
}

TEST(Carve, DwgTornSalvagesTheWalkedPrefix) {
    /* the og's chunk model: the feed stops at the device end with the
     * running max intact - the walked-prefix salvage (the CRW/X3F
     * precedent class), at reduced confidence. 200 records declared,
     * the last one straddles the device end; the salvage is the max
     * seeker+size over the FULLY-WALKED records. */
    std::vector<u8> img(2048, 0);
    const size_t at = 512;
    plant_dwg_header(img, at, "14");
    const u32 count = 200;
    dwg_put32(img.data() + at + 0x15, count);
    /* records 0..165 fit (547 + 9*166 + 9 <= 2048: record 167 stays
     * starts at 2040, ends at 2049 - past the end); the max lives at
     * record 100 (seeker 0x4000 + size 0x1000) */
    for (u32 i = 0; i <= 166; ++i) {
        const size_t rec = at + 0x19 + size_t(i) * 9;
        if (rec + 9 > img.size())
            break; /* the straddling record stays zeros = unreadable */
        img[rec] = u8(i);
        dwg_put32(img.data() + rec + 1, i == 100 ? 0x400u : 0x40u);
        dwg_put32(img.data() + rec + 5, i == 100 ? 0x100u : 0x10u);
    }
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"L");
    CarveFlags flags;
    CarveStats stats;
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
        OK);
    tree::FileNode* dwg = nullptr;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
        if (c->props() && c->props()->extents.front().offset == at)
            dwg = c;
    ASSERT_NE(dwg, nullptr) << "the torn map still carves the walked prefix";
    EXPECT_EQ(dwg->props()->logical_size, size_t(0x500))
        << "the salvage is record 100's seeker+size (0x400+0x100)";
}

TEST(Carve, DxfEofTrailerWalkSetsExtent) {
    /* the og's trailer scan (FUN_1800379c0 :42395-42432 + the backward
     * search FUN_180042ab0): the line ending from byte[3]; the extent
     * ends past the FIRST "  0\nEOF\n" / "  0\r\nEOF\r\n" marker -
     * NOT the blind 64 MiB stand-in. */
    std::vector<u8> img(256 * 1024, 0);
    const size_t at = 512;
    /* the CRLF canonical header (N-208's gate) + a body + the trailer */
    std::memcpy(img.data() + at, "  0\r\nSECTION\r\n  ", 16);
    for (size_t i = 16; i < 0x2000; ++i)
        img[at + i] = u8(0x20 + (i % 90)); /* body text, no NUL */
    std::memcpy(img.data() + at + 0x2000, "  0\r\nEOF\r\n", 10);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"L");
    CarveFlags flags;
    CarveStats stats;
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
        OK);
    tree::FileNode* dxf = nullptr;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
        if (c->props() && c->props()->extents.front().offset == at)
            dxf = c;
    ASSERT_NE(dxf, nullptr) << "the DXF must carve through the trailer walk";
    EXPECT_EQ(dxf->props()->logical_size, size_t(0x2000 + 10))
        << "extent = past the CRLF EOF trailer (0x2000 + 10)";
}

TEST(Carve, DxfLfFormUsesTheLfTrailer) {
    /* the og's byte[3] line-ending detection: an LF-form file (the
     * 999-comment entry) ends at its LF trailer "  0\nEOF\n" (8B). */
    std::vector<u8> img(256 * 1024, 0);
    const size_t at = 512;
    std::memcpy(img.data() + at, "999\n", 4);
    std::memcpy(img.data() + at + 4, "  0\nSECTION\n  ", 14);
    for (size_t i = 18; i < 0x1000; ++i)
        img[at + i] = u8(0x20 + (i % 90));
    std::memcpy(img.data() + at + 0x1000, "  0\nEOF\n", 8);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"L");
    CarveFlags flags;
    CarveStats stats;
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
        OK);
    tree::FileNode* dxf = nullptr;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
        if (c->props() && c->props()->extents.front().offset == at)
            dxf = c;
    ASSERT_NE(dxf, nullptr);
    EXPECT_EQ(dxf->props()->logical_size, size_t(0x1000 + 8))
        << "the LF form's extent = past the LF trailer";
}

TEST(Carve, DxfTornNoTrailerSalvagesTheWalkedBound) {
    /* no trailer anywhere: the og's extent accumulation ends at the
     * walked bound (the cap or the device end) - never a refusal. */
    std::vector<u8> img(128 * 1024, 0);
    const size_t at = 512;
    std::memcpy(img.data() + at, "  0\r\nSECTION\r\n  ", 16);
    for (size_t i = 16; i < 128 * 1024 - at; ++i)
        img[at + i] = u8(0x20 + (i % 90)); /* body, no trailer */
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"L");
    CarveFlags flags;
    CarveStats stats;
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
        OK);
    tree::FileNode* dxf = nullptr;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
        if (c->props() && c->props()->extents.front().offset == at)
            dxf = c;
    ASSERT_NE(dxf, nullptr) << "the torn DXF still carves (the salvage)";
    EXPECT_EQ(dxf->props()->logical_size, size_t(128 * 1024 - at))
        << "the walked bound at the device end";
}

TEST(Carve, EpsAdobeCovenantRequired) {
    /* the og's tier-1 (FUN_180032b50 :38746-62) accepts ONLY
     * "%!PS-Adobe-" headers - a bare "%!PS" file RETURNS 0 (the
     * decline). The pre-fix core accepted it - the discovered
     * divergence, pinned in both directions. */
    for (int k = 0; k < 2; ++k) {
        std::vector<u8> img(128 * 1024, 0);
        const size_t at = 512;
        if (k == 0)
            std::memcpy(img.data() + at, "%!PS-Adobe-3.0\n", 14);
        else
            std::memcpy(img.data() + at, "%!PS plain header\n", 18);
        std::memcpy(img.data() + at + 0x800, "%%EOF", 5);
        MemDevice dev(std::move(img));
        tree::ResultTree tree;
        auto* root = tree.add_root(u"L");
        CarveFlags flags;
        CarveStats stats;
        ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr,
                              stats),
                  OK);
        tree::FileNode* eps = nullptr;
        for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
            if (c->props() && c->props()->extents.front().offset == at)
                eps = c;
        if (k == 0)
            EXPECT_NE(eps, nullptr) << "the Adobe covenant accepts";
        else
            EXPECT_EQ(eps, nullptr) << "a bare %!PS header is the og's decline";
    }
}

TEST(Carve, EpsEofTrailerWalkSetsExtent) {
    /* the og's child walk (the %%EOF backward search, the general-ctx
     * arm :41446-49): the extent ends past "%%EOF" + 2 (the DSC
     * trailer's EOL) - NOT the blind 64 MiB stand-in. */
    std::vector<u8> img(256 * 1024, 0);
    const size_t at = 512;
    std::memcpy(img.data() + at, "%!PS-Adobe-3.0\n", 14);
    for (size_t i = 14; i < 0x3000; ++i)
        img[at + i] = u8(0x20 + (i % 90)); /* body text */
    std::memcpy(img.data() + at + 0x3000, "%%EOF\r\n", 7);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"L");
    CarveFlags flags;
    CarveStats stats;
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
        OK);
    tree::FileNode* eps = nullptr;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
        if (c->props() && c->props()->extents.front().offset == at)
            eps = c;
    ASSERT_NE(eps, nullptr) << "the EPS must carve through the trailer walk";
    EXPECT_EQ(eps->props()->logical_size, size_t(0x3000 + 7))
        << "extent = past %%EOF + the 2-byte EOL";
}

TEST(Carve, AiHeaderFormulaWalkSetsExtent) {
    /* the og's AI arm (FUN_180032b50 :38765-38790): extent = max of
     * the three (start + length) pairs - (dw@4+dw@8), (dw@0xC+dw@0x10),
     * (dw@0x14+dw@0x18) - the tier-1's formula IS the size (the
     * factory's non-Adobe path returns the plain base child, no walk).
     * The pre-fix "version 1..64" gate was a stand-in invention. */
    std::vector<u8> img(256 * 1024, 0);
    const size_t at = 512;
    std::memcpy(img.data() + at, "\xC5\xD0\xD3\xC6", 4);
    put32v(img, at + 4, 0x100);    /* block A start */
    put32v(img, at + 8, 0x2000);   /* block A length */
    put32v(img, at + 0xC, 0x2800); /* block B start */
    put32v(img, at + 0x10, 0x800); /* block B length */
    put32v(img, at + 0x14, 0x40);  /* block C start */
    put32v(img, at + 0x18, 0x100); /* block C length */
    /* max(0x2100, 0x3000, 0x140) = 0x3000 */
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"L");
    CarveFlags flags;
    CarveStats stats;
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
        OK);
    tree::FileNode* ai = nullptr;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
        if (c->props() && c->props()->extents.front().offset == at)
            ai = c;
    ASSERT_NE(ai, nullptr) << "the AI must carve through the header formula";
    EXPECT_EQ(ai->props()->logical_size, size_t(0x3000))
        << "extent = max(dw4+dw8, dwC+dw10, dw14+dw18) = 0x3000";
}

TEST(Carve, AiDegenerateZeroHeaderDeclines) {
    /* the all-zero length fields: extent 0 - the decline (nothing to
     * carve; the og's formula yields 0 and the emission carries it). */
    std::vector<u8> img(128 * 1024, 0);
    const size_t at = 512;
    std::memcpy(img.data() + at, "\xC5\xD0\xD3\xC6", 4);
    /* all length/start fields stay zero */
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"L");
    CarveFlags flags;
    CarveStats stats;
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
        OK);
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
        EXPECT_FALSE(c->props() && c->props()->extents.front().offset == at)
            << "the degenerate zero header declines";
}

TEST(Carve, DwgR2004RandStructWalkSetsExtent) {
    /* the og's gen-2 (FUN_18002f630's MSVC-rand XOR stream over the
     * 0x6c struct at +0x20; the six gates; extent = u64@0x34 + 0x6c).
     * The independent oracle: the well-known seed-1 rand() sequence's
     * first four low bytes (41, 35, 190, 132). */
    EXPECT_EQ(dwg_rand_stream(0), u8(41));
    EXPECT_EQ(dwg_rand_stream(1), u8(35));
    EXPECT_EQ(dwg_rand_stream(2), u8(190));
    EXPECT_EQ(dwg_rand_stream(3), u8(132));
    std::vector<u8> img(1 * 1024 * 1024, 0);
    const size_t at = 512;
    plant_dwg_r2004(img, at, 0x30000);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"L");
    CarveFlags flags;
    CarveStats stats;
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
        OK);
    tree::FileNode* dwg = nullptr;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
        if (c->props() && c->props()->extents.front().offset == at)
            dwg = c;
    ASSERT_NE(dwg, nullptr) << "the R2004 DWG must carve through the rand-XOR walk";
    EXPECT_EQ(dwg->props()->logical_size, size_t(0x30000 + 0x6c))
        << "extent = the decoded struct's u64@0x34 + 0x6c";
}

TEST(Carve, DwgR2004GateFailDeclines) {
    /* the og's gates (:36424-36427): any failed gate leaves the extent
     * at the ctor's 0 - the decline. */
    std::vector<u8> img(256 * 1024, 0);
    const size_t at = 512;
    plant_dwg_r2004(img, at, 0x30000);
    /* tear one gate byte in the PLAIN domain: byte 0x44 must decode to
     * 0x20; corrupt its ciphertext (at the +0x80 struct offset) */
    img[at + 0x80 + 0x44] = u8(img[at + 0x80 + 0x44] ^ 0xFF);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"L");
    CarveFlags flags;
    CarveStats stats;
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
        OK);
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
        EXPECT_FALSE(c->props() && c->props()->extents.front().offset == at)
            << "the failed struct gate must decline";
}

TEST(Carve, DwgVersionGateIsTheOgsNine) {
    /* the og's tier-1 table (FUN_18002fe30 :36766-36794): exactly
     * {12,13,14,15,18,21,24,27,32}. The pre-fix core carried "30"
     * (AC1030 - no such DWG release) and LACKED "13" (AC1013 = R13) -
     * both directions pinned here. */
    for (int k = 0; k < 2; ++k) {
        std::vector<u8> img(128 * 1024, 0);
        const size_t at = 512;
        if (k == 0)
            plant_dwg_header(img, at, "13"); /* R13 - MUST carve */
        else
            plant_dwg_header(img, at, "30"); /* AC1030 - MUST NOT carve */
        plant_dwg_locator(img, at, 1, {{0x100, 0x2000}});
        MemDevice dev(std::move(img));
        tree::ResultTree tree;
        auto* root = tree.add_root(u"L");
        CarveFlags flags;
        CarveStats stats;
        ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr,
                              stats),
                  OK);
        tree::FileNode* dwg = nullptr;
        for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
            if (c->props() && c->props()->extents.front().offset == at)
                dwg = c;
        if (k == 0)
            EXPECT_NE(dwg, nullptr) << "AC1013 (R13) is in the og's table - it must carve";
        else
            EXPECT_EQ(dwg, nullptr) << "AC1030 is NOT a DWG release - the og's table refuses";
    }
}

/* ---- the S86 convergence riders over the racing DWG landing (this lane renumbered from the
 * born-collided S86 seat) ---- */

TEST(Carve, DwgRowCarriesTheOgsRegisteredFloor) {
    /* the S86 rider A: the og's registered row floor
     * is 0x15 = 21 (the CDWGRawInfo factory FUN_18002ff20's qword write at +0x110 - puVar1[0x22] =
     * 0x15, the PNG/ONE ctor pattern [0x22]=min). A one-record walk ending at exactly 21 bytes
     * emits at 21: RED at the first landing's tree (the residual 512-byte floor refused it AND the
     * min-64 row rejected it at the emit gate); GREEN with the rider (the 512 floor gone from
     * gen-1, the row min re-pinned). */
    std::vector<u8> img(256 * 1024, 0);
    const size_t at = 512;
    plant_dwg_header(img, at, "12");          /* R13 - a gen-1 member */
    plant_dwg_locator(img, at, 1, {{5, 16}}); /* off 5 + size 16 = 21 */
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"L");
    CarveFlags flags;
    CarveStats stats;
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
        OK);
    tree::FileNode* dwg = nullptr;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
        if (c->props() && c->props()->extents.front().offset == at)
            dwg = c;
    ASSERT_NE(dwg, nullptr) << "the 21-byte walked extent must clear the og's own floor";
    EXPECT_EQ(dwg->props()->logical_size, size_t(21)) << "off 5 + size 16";
}

TEST(Carve, DwgWalkSelfLimitsAt256Records) {
    /* the S86 rider B: the og's id-sequence gate
     * compares the id byte against the FULL 32-bit running counter (raw-asm 0x18002f40a: movzbl the
     * id, `cmp %ecx,%eax` with ecx the counter) - no byte can match a counter >= 256, so the walk
     * SELF-LIMITS at 256 records. A count>256 fixture with wrapped ids (record 256 carrying id 0)
     * is accepted by the u8(i) truncation and refused by the og. RED at the first landing's tree
     * (the fixture carved); GREEN with the rider. */
    std::vector<u8> img(64 * 1024, 0);
    const size_t at = 512;
    plant_dwg_header(img, at, "15");
    /* 300 records: the plant stamps id=u8(i) per record - records 0..255
     * carry their true ids, records 256..299 carry the wrapped ids
     * (u8(256)=0 ... u8(299)=43) - exactly the shape the u8(i)
     * truncation accepts and the og's full-counter gate refuses */
    const u32 count = 300;
    std::vector<std::array<u32, 2>> seeker_size;
    /* each record's end = 0x100 + 0x800 = 2304 >= 512 so the residual
     * 512 floor never refuses the fixture - the counter gate is the
     * ONLY thing under test (rider A's floor is proven separately) */
    for (u32 i = 0; i < count; ++i)
        seeker_size.push_back({0x100, 0x800});
    plant_dwg_locator(img, at, count, seeker_size);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"L");
    CarveFlags flags;
    CarveStats stats;
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
        OK);
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
        ASSERT_TRUE(!(c->props() && c->props()->extents.front().offset == at))
            << "a count>256 fixture with wrapped ids must refuse (the og's "
               "full-counter gate self-limits the walk)";
}

size_t plant_psp(std::vector<u8>& img, size_t at);

TEST(Carve, PspBkBlockChainWalkSetsExtent) {
    /* the og's walk (FUN_180016510 :17410-17482): the ~BK chain from
     * 0x24, each block {magic "~BK\0", length u32 @ +6}, advance
     * 10+length; the extent = the walked end (0x24 + 0x10A + 0x8A) -
     * NOT the blind 64 MiB stand-in. */
    std::vector<u8> img(256 * 1024, 0);
    const size_t at = 512;
    const size_t expect = plant_psp(img, at);
    EXPECT_EQ(expect, size_t(0x1B8));
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"L");
    CarveFlags flags;
    CarveStats stats;
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
        OK);
    tree::FileNode* psp = nullptr;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
        if (c->props() && c->props()->extents.front().offset == at)
            psp = c;
    ASSERT_NE(psp, nullptr) << "the PSP must carve through the ~BK walk";
    EXPECT_EQ(psp->props()->logical_size, size_t(0x1B8))
        << "extent = 0x24 + (10+0x100) + (10+0x80) - the block-chain end";
}

TEST(Carve, PspExactSignatureRequired) {
    /* the og's tier-1 is the FULL 32-byte compare incl. the 0x0a 0x1a
     * tail (FUN_180016390) - the old core accepted a 16-byte prefix;
     * a file with the 26-byte old-core shape (no 0x0a/0x1a) REFUSES. */
    std::vector<u8> img(128 * 1024, 0);
    const size_t at = 512;
    std::memcpy(img.data() + at, "Paint Shop Pro Image File", 25);
    img[at + 25] = 0;
    img[at + 26] = 0; /* the OLD plant's shape: NUL NUL, not 0x0a 0x1a */
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"L");
    CarveFlags flags;
    CarveStats stats;
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
        OK);
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
        EXPECT_FALSE(c->props() && c->props()->extents.front().offset == at)
            << "the 26-byte short signature must refuse (the og compares 32)";
}

TEST(Carve, PspTornChainSalvagesTheWalkedEnd) {
    /* the chain runs past the device end: the walked end salvages (the
     * og's chunk-feed exhaustion keeps +0x110's walked position). */
    std::vector<u8> img(1024, 0);
    const size_t at = 512;
    static const char kSig[] = "Paint Shop Pro Image File";
    std::memcpy(img.data() + at, kSig, 25);
    img[at + 25] = 0x0a;
    img[at + 26] = 0x1a;
    /* one block: len 0x200 -> the next header at 0x24+0x20A = 0x22E =
     * 558 past the device end (1024): the salvage is the walked end */
    put32v(img, at + 0x24, 0x004b427eu);
    put32v(img, at + 0x24 + 6, 0x200);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"L");
    CarveFlags flags;
    CarveStats stats;
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
        OK);
    tree::FileNode* psp = nullptr;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
        if (c->props() && c->props()->extents.front().offset == at)
            psp = c;
    ASSERT_NE(psp, nullptr) << "the torn chain still carves the walked end";
    EXPECT_EQ(psp->props()->logical_size, size_t(1024 - 512))
        << "the walked end clamps to the device end - never a refusal";
}

TEST(Carve, WpgZeroTailWalkSetsExtent) {
    /* the og's zero-tail walk (FUN_180042b30 :50703-50750 + the
     * processor FUN_180014160: extent = the trailing-zero-run position
     * of the first 512-byte window) - NOT the blind 64 MiB stand-in. */
    std::vector<u8> img(256 * 1024, 0);
    const size_t at = 512;
    std::memcpy(img.data() + at, "wpg\x00", 4);
    for (size_t i = 4; i < 300; ++i)
        img[at + i] = u8((i * 7) & 0xFF); /* 300 bytes of content */
    /* [at+300, at+512) stays zeros: the zero run -> extent 300 */
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"L");
    CarveFlags flags;
    CarveStats stats;
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
        OK);
    tree::FileNode* wpg = nullptr;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
        if (c->props() && c->props()->extents.front().offset == at)
            wpg = c;
    ASSERT_NE(wpg, nullptr) << "the WPG must carve through the zero-tail walk";
    EXPECT_EQ(wpg->props()->logical_size, size_t(300))
        << "extent = the first all-zero-tail position (300), not 512 and "
           "not the 64 MiB stand-in";
}

TEST(Carve, WpgZeroTailFullWindowIsTheBoundary) {
    /* FUN_180042b30's no-zeros arm: the whole window non-zero -> the
     * extent is the WINDOW length (512) - the og's one-chunk walk. */
    std::vector<u8> img(256 * 1024, 0);
    const size_t at = 512;
    std::memcpy(img.data() + at, "wpg\x00", 4);
    for (size_t i = 4; i < 512; ++i)
        img[at + i] = u8(1 + (i % 251)); /* no zeros anywhere in-window */
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"L");
    CarveFlags flags;
    CarveStats stats;
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
        OK);
    tree::FileNode* wpg = nullptr;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
        if (c->props() && c->props()->extents.front().offset == at)
            wpg = c;
    ASSERT_NE(wpg, nullptr);
    EXPECT_EQ(wpg->props()->logical_size, size_t(512))
        << "no zero tail in the window: the extent is the window length";
}

TEST(Carve, WpgAllZeroWindowDeclines) {
    /* FUN_180042b30's whole-buffer-zero arm returns 0 - the og's
     * extent-0 decline class; the core refuses the carve. */
    std::vector<u8> img(64 * 1024, 0);
    const size_t at = 512;
    std::memcpy(img.data() + at, "wpg\x00", 4);
    /* everything past the magic stays zero */
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"L");
    CarveFlags flags;
    CarveStats stats;
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
        OK);
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
        EXPECT_FALSE(c->props() && c->props()->extents.front().offset == at)
            << "the all-zero window is the og's extent-0 decline";
}

/* ---- M-160: the APE version gate (the u16 LE [1000, 9999] window) -- */

size_t plant_ape(std::vector<u8>& img, size_t at);

TEST(Carve, ApeRealFormVersionGateCarves) {
    /* The og's gate (FUN_180033180): the u16 LE at +4 in [1000, 9999].
     * The old core's u32-BE [300, 5000] read rejected EVERY real APE
     * file (the version field is the u16 LE; the BE read of a real
     * header is ~0x820F0000) - the whole-family absence this finding
     * filed. RED at the old gate: this exact plant was rejected. */
    std::vector<u8> img(128 * 1024, 0);
    const size_t at = 512;
    plant_ape(img, at);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"L");
    CarveFlags flags;
    CarveStats stats;
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
        OK);
    tree::FileNode* ape = nullptr;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
        if (c->props() && c->props()->extents.front().offset == at)
            ape = c;
    ASSERT_NE(ape, nullptr) << "a real-form APE (u16 LE version 3980) must carve";
    EXPECT_EQ(ape->props()->carve_format, FMT_APE);
}

TEST(Carve, ApeVersionWindowIsTheOgsThousandWindow) {
    /* The window's both edges and both outsides: 1000 and 9999 carve;
     * 999 and 10000 decline - the og's exact bounds. */
    for (int k = 0; k < 4; ++k) {
        std::vector<u8> img(64 * 1024, 0);
        const size_t at = 512;
        std::memcpy(img.data() + at, "MAC ", 4);
        const u16 ver = (k == 0) ? 1000 : (k == 1) ? 9999 : (k == 2) ? 999 : 10000;
        put16(img, at + 4, ver);
        MemDevice dev(std::move(img));
        tree::ResultTree tree;
        auto* root = tree.add_root(u"L");
        CarveFlags flags;
        CarveStats stats;
        ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr,
                              stats),
                  OK);
        tree::FileNode* ape = nullptr;
        for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
            if (c->props() && c->props()->extents.front().offset == at)
                ape = c;
        if (k < 2)
            EXPECT_NE(ape, nullptr) << "version " << ver << " is inside [1000, 9999] - must carve";
        else
            EXPECT_EQ(ape, nullptr)
                << "version " << ver << " is outside [1000, 9999] - the og declines";
    }
}

/* ---- M-107 member 6 (WPD): the version-table walk
 * (FUN_180041f50) ------------------------------------------ */

size_t plant_wpd(std::vector<u8>& img, size_t at, u32 size);

TEST(Carve, WpdVersionTableWalkSetsExtent) {
    /* The og's walk stamps extent = u32@+0x14 (the WPC common header's
     * own file-size field) verbatim on a version-record match - NOT the
     * blind 64 MiB stand-in. The plant declares 0x8000 (32 KiB) on a
     * 1 MiB device: the stand-in carved the whole device reach; the
     * walk carves exactly the declared size. */
    std::vector<u8> img(1024 * 1024, 0);
    const size_t at = 512;
    const size_t expect = plant_wpd(img, at, 0x8000);
    EXPECT_EQ(expect, size_t(0x8000));
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"L");
    CarveFlags flags;
    CarveStats stats;
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
        OK);
    tree::FileNode* wpd = nullptr;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
        if (c->props() && c->props()->extents.front().offset == at)
            wpd = c;
    ASSERT_NE(wpd, nullptr) << "the WPD must carve through the version-table walk";
    EXPECT_EQ(wpd->props()->logical_size, size_t(0x8000))
        << "extent = the header's own size field u32@+0x14";
}

TEST(Carve, WpdOverCapSizeCarvesFull) {
    /* The truncation class the finding filed: a WPD whose declared size
     * exceeds the old 64 MiB stand-in cap carves og-exact (the row max
     * now carries the u32 field's own ceiling; the device end clamps). */
    std::vector<u8> img(80ull << 20, 0); /* 80 MiB device */
    const size_t at = 512;
    plant_wpd(img, at, u32(72ull << 20)); /* 72 MiB > the old 64 MiB cap */
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"L");
    CarveFlags flags;
    CarveStats stats;
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
        OK);
    tree::FileNode* wpd = nullptr;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
        if (c->props() && c->props()->extents.front().offset == at)
            wpd = c;
    ASSERT_NE(wpd, nullptr) << "the over-cap WPD must carve";
    EXPECT_EQ(wpd->props()->logical_size, size_t(72ull << 20))
        << "the declared 72 MiB size carves full - no 64 MiB truncation";
}

TEST(Carve, WpdVersionTableGateDeclinesUnknownVersion) {
    /* The og's gate is the three-record table, NOT the bare key: a
     * \xFFWPC hit with a version pair in NO record declines (the
     * return-0 walk). The stand-in accepted any key + product word. */
    std::vector<u8> img(256 * 1024, 0);
    const size_t at = 512;
    std::memcpy(img.data() + at,
                "\xFF"
                "WPC",
                4);
    put16(img, at + 4, 0x0200);        /* nonzero product word (the stand-in's gate) */
    put32v(img, at + 8, 0x020a0a0a);   /* verA: NOT in any record */
    put32v(img, at + 0xe, 0x00050200); /* verB: record 1's - the PAIR is what gates */
    put32v(img, at + 0x14, 0x1000);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"L");
    CarveFlags flags;
    CarveStats stats;
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
        OK);
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
        EXPECT_FALSE(c->props() && c->props()->extents.front().offset == at)
            << "an unmatched version pair must decline (the og's table gate)";
}

TEST(Carve, WpdSecondAndThirdRecordsCarve) {
    /* The table's other two records - the WPG-family WPC variant
     * (0x00021601, 0x00030000) and the third variant (0x00010f0f,
     * 0x00030000) - carve through the same walk at their own declared
     * sizes, and carry their OWN sub-format ids: the og's engine (not
     * the report layer) assigns the name - the CRawFileInfo child's
     * +0x28 processor (0x18000b7a0) indexes the 9-byte-stride name
     * table at 0x18008f070 by the tier-1's stamped id (the S91
     * sub-format rider; the racing S87 'report-layer routing' framing
     * corrected by the processor decode). */
    std::vector<u8> img(256 * 1024, 0);
    const size_t at2 = 512, at3 = 16384; /* at3 beyond at2's extent (no
        mid-stream suppression: at2's carve ends at 512+0x3000) */
    std::memcpy(img.data() + at2,
                "\xFF"
                "WPC",
                4);
    put32v(img, at2 + 8, 0x00021601);
    put32v(img, at2 + 0xe, 0x00030000);
    put32v(img, at2 + 0x14, 0x3000);
    std::memcpy(img.data() + at3,
                "\xFF"
                "WPC",
                4);
    put32v(img, at3 + 8, 0x00010f0f);
    put32v(img, at3 + 0xe, 0x00030000);
    put32v(img, at3 + 0x14, 0x5000);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"L");
    CarveFlags flags;
    CarveStats stats;
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
        OK);
    tree::FileNode* v2 = nullptr;
    tree::FileNode* v3 = nullptr;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling()) {
        if (!c->props() || c->props()->extents.empty())
            continue;
        if (c->props()->extents.front().offset == at2)
            v2 = c;
        if (c->props()->extents.front().offset == at3)
            v3 = c;
    }
    ASSERT_NE(v2, nullptr) << "record 2 (the WPG-family WPC variant) must carve";
    EXPECT_EQ(v2->props()->logical_size, size_t(0x3000));
    EXPECT_EQ(v2->props()->carve_format, u32(FMT_WPG))
        << "the engine's name table stamps id 0x45 (WPG) for record 2";
    ASSERT_NE(v3, nullptr) << "record 3 (the third WPC variant) must carve";
    EXPECT_EQ(v3->props()->logical_size, size_t(0x5000));
    EXPECT_EQ(v3->props()->carve_format, u32(FMT_SHW))
        << "the engine's name table stamps id 0x3d (SHW) for record 3";
}

/* M-081/SCAN-4: the ONLY true Minolta MRW form - \0MRM + the og size
 * formula fields (FUN_180012100 :14233-14258, first-hand):
 * BE32@+4 + 8 + ceil(u8@0x20 * BE16@+0x1a * BE16@+0x18 / 8). */
size_t plant_mrm(std::vector<u8>& img, size_t at) {
    std::memcpy(img.data() + at, "\x00MRM", 4);
    putbe32(img, at + 4, 0x400); /* BE base */
    putbe16(img, at + 0x18, 32); /* BE width */
    putbe16(img, at + 0x1A, 32); /* BE height */
    img[at + 0x20] = 8;          /* bit depth */
    for (size_t i = 0x21; i < 0x400 + 8 + 32 * 32; ++i)
        img[at + i] = u8((i * 7) & 0xFF);
    return 0x400 + 8 + 32 * 32; /* the og formula's exact value */
}

/* SCAN-4: FileLowDocAnalyser (id 0x17, the old-Word .doc family) - the
 * DB A5 2D 00 / 94 A6 2E 00 keys with the og formula fields
 * (FUN_18001b650 :21167-21188, first-hand): size = (u32@0x124 +
 * u16@0x128 + 0x1FF) & ~0x1FF. */
size_t plant_lowdoc(std::vector<u8>& img, size_t at, bool second_key = false) {
    std::memcpy(img.data() + at, second_key ? "\x94\xA6\x2E\x00" : "\xDB\xA5\x2D\x00", 4);
    for (size_t i = 4; i < 0x124; ++i)
        img[at + i] = u8((i * 3) & 0xFF);
    put32v(img, at + 0x124, 0x400);
    put16(img, at + 0x128, 0x200);
    return 0x600; /* (0x400 + 0x200 + 0x1FF) & ~0x1FF - the formula's exact value */
}

size_t plant_wpg(std::vector<u8>& img, size_t at) {
    std::memcpy(img.data() + at, "wpg\x00", 4);
    for (size_t i = 4; i < 32; ++i)
        img[at + i] = u8(i & 0xFF);
    return 32;
}

size_t plant_wpd(std::vector<u8>& img, size_t at, u32 declared = 0x3000) {
    /* M-107 member 6: the og's tier-1 pair table (FUN_180041f50's
     * compare groups at 0x180041f8a-0x180041fd0): (u32@+8, u32@+0xE)
     * = the known pair (0x01020a01, 0x00050200); the extent =
     * u32@+0x14 (0x18004200c) - the declared document size. The
     * S87-continuation lane's convergent decode: the FULL three-record
     * table {(0x01020a01,0x00050200)->0x44, (0x00021601,0x00030000)
     * ->0x45, (0x00010f0f,0x00030000)->0x3d} - corroborated by the
     * .rdata dump at 0x18006ef40. */
    std::memcpy(img.data() + at,
                "\xFF"
                "WPC",
                4);
    put32v(img, at + 8, 0x01020a01);   /* the pair's first word */
    put32v(img, at + 0xE, 0x00050200); /* the pair's second word */
    put32v(img, at + 0x14, declared);  /* the og's size source */
    return declared;
}

TEST(Carve, WpdPairTableDeclaredSizeSetsExtent) {
    /* M-107 member 6 (WPD): the og's FileWPDAnalyser walk (the +0x10
     * slot FUN_180041f50; the registry binding FF 'W' 'P' 'C' -> vtable
     * 0x18006cff0 idx 0x44): the tier-1 pair table then extent =
     * u32@+0x14 (0x18004200c) - the declared document size, NOT the
     * blind 64 MiB stand-in. RED at the stand-in tree: the extent was
     * the device-end-clamped 64 MiB. */
    std::vector<u8> img(256 * 1024, 0);
    const size_t at = 512;
    const u32 declared = 0x5000;
    plant_wpd(img, at, declared);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"L");
    CarveFlags flags;
    CarveStats stats;
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
        u32(OK));
    tree::FileNode* wpd = nullptr;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
        if (c->props() && !c->props()->extents.empty() && c->props()->extents.front().offset == at)
            wpd = c;
    ASSERT_NE(wpd, nullptr) << "the WPD must carve through the pair table";
    EXPECT_EQ(wpd->props()->logical_size, size_t(declared))
        << "the extent is the declared size at +0x14, not 64 MiB";
}

TEST(Carve, WpdUnknownPairDeclines) {
    /* the og's tier-1 (0x180041ff0-0x180041ff5): an unknown
     * (u32@+8, u32@+0xE) pair returns 0 - the DECLINE, no bounded-extent
     * fallback. The old 'nonzero product word' gate is gone with it. */
    std::vector<u8> img(128 * 1024, 0);
    const size_t at = 512;
    std::memcpy(img.data() + at,
                "\xFF"
                "WPC",
                4);
    put32v(img, at + 8, 0xDEADBEEF); /* an unknown pair */
    put32v(img, at + 0xE, 0xFEEDFACE);
    put32v(img, at + 0x14, 0x4000);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"L");
    CarveFlags flags;
    CarveStats stats;
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
        u32(OK));
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
        if (c->props() && !c->props()->extents.empty() && c->props()->extents.front().offset == at)
            FAIL() << "an unknown pair must decline the carve";
    SUCCEED();
}

size_t plant_sit(std::vector<u8>& img, size_t at, size_t bytes, u32 flavor) {
    /* N-093: the og's SIT floor is 20,480 bytes (SITRawFileInfo :16886-92).
     * M-107 (the SIT member, the S91 arbitration): the '!' form's size walk
     * is the entry-marker search (extent = marker + 8 + stride, the stride
     * table {0x19: 4, 0x3B: 5, else: 6}). The plant carries the '!' form
     * with the flavor's marker at bytes - 8 - stride so the walked extent
     * lands exactly at the planted length for EVERY flavor. [The img[m]
     * indexing repaired at the racing 98bd30d6 sweep - the 2214af90 push
     * committed the marker writes without the subscript, a red tree on
     * main; the unused 'flavor' parameter (the second red, N-231's
     * record) repaired at this lane's arbitration.] */
    const u32 stride = (flavor == 0x19) ? 4 : (flavor == 0x3b ? 5 : 6);
    if (bytes < 24 + stride)
        bytes = 24 + stride; /* the magic (8) + the marker (8) + the stride + slack */
    img[at] = 'S';
    img[at + 1] = 't';
    img[at + 2] = 'u';
    img[at + 3] = 'f';
    img[at + 4] = 'f';
    img[at + 5] = 'I';
    img[at + 6] = 't';
    img[at + 7] = '!'; /* the FileSITAnalyser's own gate (raw-asm 0x180044a93) */
    const size_t m = at + bytes - 8 - stride;
    for (size_t i = 8; i < bytes - 8 - stride; ++i)
        img[at + i] = u8((i * 7) & 0xFF); /* archive-ish body, marker-free */
    img[m] = 0x02;
    img[m + 1] = 0xD7;
    img[m + 2] = 0x0A;
    img[m + 3] = u8(flavor); /* the stride selector IS the magic's variant byte */
    img[m + 4] = 0x98;
    img[m + 5] = 0xF6;
    img[m + 6] = 0x42;
    img[m + 7] = 0x88;
    for (size_t i = bytes - 4; i < bytes; ++i)
        img[at + i] = u8(i & 0xFF);
    return bytes;
}
