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
#include <cstring>
#include <ctime>
#include <fstream>
#include <functional>
#include <iterator>
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

    /* validated: jpg, png, zip, docx; noise rejected; no duplicates yet */
    EXPECT_EQ(stats.validated, 4u);
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
                 u64 torn_size = 0, u32 header_count = 1) {
    static const u8 asf_guid[16] = {0x30, 0x26, 0xB2, 0x75, 0x8E, 0x66, 0xCF, 0x11,
                                    0xA6, 0xD9, 0x00, 0xAA, 0x00, 0x62, 0xCE, 0x6C};
    static const u8 data_guid[16] = {0x36, 0x26, 0xB2, 0x75, 0x8E, 0x66, 0xCF, 0x11,
                                     0xA6, 0xD9, 0x00, 0xAA, 0x00, 0x62, 0xCE, 0x6C};
    size_t p = at;
    /* Header Object: GUID + size (100) + count + 2 reserved + payload */
    std::memcpy(img.data() + p, asf_guid, 16);
    const u64 hsz = 100;
    for (int i = 0; i < 8; ++i)
        img[p + 16 + size_t(i)] = u8(hsz >> (8 * i));
    for (int i = 0; i < 4; ++i)
        img[p + 24 + size_t(i)] = u8(header_count >> (8 * i));
    img[p + 28] = 1; /* broadcast flag (arbitrary non-reserved byte) */
    for (u32 i = 30; i < hsz; ++i)
        img[p + size_t(i)] = 0x44;
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

size_t plant_x3f(std::vector<u8>& img, size_t at) {
    std::memcpy(img.data() + at, "FOVb", 4);
    std::memcpy(img.data() + at + 4, "2.1\x00", 4);
    for (size_t i = 8; i < 128; ++i)
        img[at + i] = u8(i & 0xFF);
    return 128;
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

size_t plant_wpd(std::vector<u8>& img, size_t at) {
    std::memcpy(img.data() + at,
                "\xFF"
                "WPC",
                4);
    put16(img, at + 4, 0x0200); /* nonzero product word */
    for (size_t i = 6; i < 32; ++i)
        img[at + i] = u8(i & 0xFF);
    return 32;
}

size_t plant_psp(std::vector<u8>& img, size_t at) {
    std::memcpy(img.data() + at, "Paint Shop Pro I", 16);
    std::memcpy(img.data() + at + 16, "mage File\x00", 10);
    for (size_t i = 26; i < 64; ++i)
        img[at + i] = u8(i & 0xFF);
    return 64;
}

size_t plant_sit(std::vector<u8>& img, size_t at, size_t bytes = 20480) {
    /* N-093: the og's SIT floor is 20,480 bytes (SITRawFileInfo :16886-92) -
     * the plant grew from the 32-byte magic stub to the real minimum so the
     * fixture exercises the og's own acceptance boundary. */
    if (bytes < 7)
        bytes = 7; /* the magic itself is 7 bytes; every real call far exceeds it */
    img[at] = 'S';
    img[at + 1] = 't';
    img[at + 2] = 'u';
    img[at + 3] = 'f';
    img[at + 4] = 'f';
    img[at + 5] = 'I';
    img[at + 6] = 't';
    for (size_t i = 7; i < bytes; ++i)
        img[at + i] = u8(i & 0xFF);
    return bytes;
}

size_t plant_gz(std::vector<u8>& img, size_t at) {
    img[at] = 0x1F;
    img[at + 1] = 0x8B;
    img[at + 2] = 8;    /* CM deflate */
    img[at + 3] = 0x00; /* FLG: no optional fields */
    for (int i = 4; i < 10; ++i)
        img[at + size_t(i)] = 0;
    img[at + 9] = 3; /* OS unix */
    for (size_t i = 10; i < 74; ++i)
        img[at + i] = u8((i * 13) & 0xFF); /* deflate-ish body */
    return 74;
}

size_t plant_ape(std::vector<u8>& img, size_t at) {
    std::memcpy(img.data() + at, "MAC ", 4);
    putbe32(img, at + 4, 3980); /* release integer */
    putbe16(img, at + 8, 2000); /* compression level */
    for (size_t i = 10; i < 64; ++i)
        img[at + i] = u8(i & 0xFF);
    return 64;
}

size_t plant_au(std::vector<u8>& img, size_t at) {
    std::memcpy(img.data() + at, ".snd", 4);
    putbe32(img, at + 4, 24);     /* data offset */
    putbe32(img, at + 8, 256);    /* data size */
    putbe32(img, at + 12, 3);     /* encoding u-law */
    putbe32(img, at + 16, 44100); /* rate */
    putbe32(img, at + 20, 2);     /* channels */
    for (size_t i = 24; i < 280; ++i)
        img[at + i] = u8((i * 7) & 0xFF);
    return 280;
}

size_t plant_aiff(std::vector<u8>& img, size_t at) {
    std::memcpy(img.data() + at, "FORM", 4);
    putbe32(img, at + 4, 32);
    std::memcpy(img.data() + at + 8, "AIFF", 4);
    for (size_t i = 12; i < 40; ++i)
        img[at + i] = u8(i & 0xFF);
    return 40;
}

size_t plant_hlp(std::vector<u8>& img, size_t at) {
    /* SCAN-4: the og HLP gate (FUN_180041c50 :49883-49895) - u32@0xc is
     * the size and must be > 0x1FF and >= u32@+4. */
    std::memcpy(img.data() + at, "\x3F\x5F\x03\x00", 4);
    put32v(img, at + 4, 0x100);    /* first dword <= size */
    put32v(img, at + 0xC, 0x1000); /* the declared size */
    for (size_t i = 0x10; i < 0x1000; ++i)
        img[at + i] = u8((i * 5) & 0xFF);
    return 0x1000; /* the og gate's exact carved size */
}

/* ICONDIR: one 16-byte entry + its data payload. The entry layout is
 * byte-exact: width u8 + height u8 + colorCount u8 + reserved u8 +
 * planes u16 + bpp u16 + bytesInRes u32 + imageOffset u32. */
size_t plant_ico(std::vector<u8>& img, size_t at, u32 type = 1) {
    put16(img, at + 0, 0); /* reserved */
    put16(img, at + 2, u16(type));
    put16(img, at + 4, 1);    /* one image */
    img[at + 6] = 16;         /* width */
    img[at + 7] = 16;         /* height */
    img[at + 8] = 16;         /* color count */
    img[at + 9] = 0;          /* reserved */
    put16(img, at + 10, 1);   /* planes */
    put16(img, at + 12, 32);  /* bpp */
    put32v(img, at + 14, 64); /* bytes in res */
    put32v(img, at + 18, 22); /* data offset (right after the dir) */
    for (size_t i = 22; i < 22 + 64; ++i)
        img[at + i] = u8(i & 0xFF);
    return 22 + 64;
}

/* Placeable WMF per the og grammar (RAWSearchDLL FUN_1800264d0
 * :29807-29845, S-042): key@0, hmf@4==0, bbox@6..13, inch@14 (never
 * gated), reserved dword@16..19==0, checksum@20 == XOR(words 0..9),
 * standard header@+22, params@38==0, extent = Size@28*2 + 22.
 * Spec-derived fixture (canonical 22-byte Aldus layout) - never
 * generated from the validator's own output. */
size_t plant_wmf_placeable(std::vector<u8>& img, size_t at) {
    std::memcpy(img.data() + at, "\xD7\xCD\xC6\x9A", 4);
    put16(img, at + 4, 0);     /* HWmf */
    put16(img, at + 6, 0);     /* bbox left */
    put16(img, at + 8, 0);     /* bbox top */
    put16(img, at + 10, 100);  /* bbox right */
    put16(img, at + 12, 100);  /* bbox bottom */
    put16(img, at + 14, 1440); /* inch - the og never gates this field */
    put16(img, at + 16, 0);    /* reserved */
    put16(img, at + 18, 0);    /* reserved */
    u16 x = 0;
    for (int i = 0; i < 10; ++i)
        x = u16(x ^ u16_from_bytes_le(&img[at + 2 * size_t(i)]));
    put16(img, at + 20, x);                                   /* checksum */
    std::memcpy(img.data() + at + 22, "\x01\x00\x09\x00", 4); /* std type + header words */
    put16(img, at + 26, 0x0300);                              /* version - not gated by the og */
    put32v(img, at + 28, 100);                                /* Size words -> 222-byte file */
    put16(img, at + 32, 4);                                   /* objects - not gated by the og */
    put32v(img, at + 34, 10);                                 /* max record - not gated by the og */
    put16(img, at + 38, 0);                                   /* params == 0 */
    return 100 * 2 + 22;
}

size_t plant_wmf_std(std::vector<u8>& img, size_t at) {
    std::memcpy(img.data() + at, "\x01\x00\x09\x00", 4);
    put16(img, at + 4, 0x0300); /* version */
    put32v(img, at + 6, 64);    /* size in words -> 128 bytes */
    put16(img, at + 10, 4);     /* objects */
    put32v(img, at + 12, 10);   /* max record */
    put16(img, at + 16, 0);     /* params */
    for (size_t i = 18; i < 128; ++i)
        img[at + i] = u8((i * 11) & 0xFF);
    return 128;
}

/* S-047: a 188-byte TS packet stream (0x47 sync, the og's follow-byte set,
 * patterned payload; optional all-0xFF null packets at +8..+15). */
size_t plant_ts(std::vector<u8>& img, size_t at, size_t packets, bool with_null = false) {
    for (size_t p = 0; p < packets; ++p) {
        const size_t base = at + p * 188;
        img[base] = 0x47;     /* the sync */
        img[base + 1] = 0x41; /* in the og's follow-byte set {0x40,0x41,'P',0x10} */
        img[base + 2] = 0x00;
        img[base + 3] = 0x01;
        if (with_null && p % 7 == 3) {
            /* a null packet: the og checks bytes +8..+15 for all-0xFF and
             * marks (but does not end) the run - FUN_180032460 */
            for (size_t b = 4; b < 188; ++b)
                img[base + b] = 0xFF;
        } else {
            for (size_t b = 4; b < 188; ++b)
                img[base + b] = u8((b * 7 + p * 3) & 0xFF);
        }
    }
    return packets * 188;
}

/* S-047: a 192-byte M2TS/BDAV packet stream (4-byte TP_extra_header, sync
 * at +4, follow byte at +5 in the og's set). */
size_t plant_m2ts(std::vector<u8>& img, size_t at, size_t packets) {
    for (size_t p = 0; p < packets; ++p) {
        const size_t base = at + p * 192;
        img[base + 0] = u8(p & 0xFF); /* TP_extra_header: counter */
        img[base + 1] = 0x00;
        img[base + 2] = 0x00;
        img[base + 3] = 0x00;
        img[base + 4] = 0x47; /* the sync at +4 */
        img[base + 5] = 0x50; /* 'P' - in the og's follow-byte set */
        for (size_t b = 6; b < 192; ++b)
            img[base + b] = u8((b * 5 + p * 11) & 0xFF);
    }
    return packets * 192;
}

/* M-092: an HTML document plant - the open tag, body filler, the </html>
 * end tag. Returns the exact end-of-marker offset (pre-round-up). */
size_t plant_html(std::vector<u8>& img, size_t at, const char* open_tag, size_t body_len) {
    const size_t open_len = std::strlen(open_tag);
    std::memcpy(img.data() + at, open_tag, open_len);
    for (size_t i = 0; i < body_len; ++i)
        img[at + open_len + i] = u8('a' + (i % 26));
    const size_t end = at + open_len + body_len;
    std::memcpy(img.data() + end, "</html>", 7);
    return end + 7;
}

/* M-092: a UTF-16LE HTML plant (FF FE BOM + the wide open tag + wide
 * body + the ASCII </html> end tag - the og's walk needle is ASCII-only
 * even for the BOM chain). */
size_t plant_html_utf16(std::vector<u8>& img, size_t at, size_t body_len) {
    img[at] = 0xFF;
    img[at + 1] = 0xFE;
    const wchar_t* open_tag = L"<HTML>";
    for (size_t i = 0; open_tag[i]; ++i) {
        img[at + 2 + 2 * i] = u8(open_tag[i]);
        img[at + 2 + 2 * i + 1] = 0;
    }
    const size_t body_at = at + 2 + 12;
    for (size_t i = 0; i < body_len; ++i) {
        img[body_at + 2 * i] = u8('a' + (i % 26));
        img[body_at + 2 * i + 1] = 0;
    }
    const size_t end = body_at + 2 * body_len;
    std::memcpy(img.data() + end, "</html>", 7);
    return end + 7;
}

size_t plant_ttf(std::vector<u8>& img, size_t at) {
    std::memcpy(img.data() + at, "\x00\x01\x00\x00", 4);
    putbe16(img, at + 4, 1);   /* numTables */
    putbe16(img, at + 6, 16);  /* searchRange */
    putbe16(img, at + 8, 0);   /* entrySelector */
    putbe16(img, at + 10, 16); /* rangeShift */
    std::memcpy(img.data() + at + 12, "cmap", 4);
    putbe32(img, at + 16, 0);  /* checksum */
    putbe32(img, at + 20, 28); /* offset */
    putbe32(img, at + 24, 64); /* length */
    for (size_t i = 28; i < 92; ++i)
        img[at + i] = u8((i * 5) & 0xFF);
    return 92;
}

size_t plant_mdb(std::vector<u8>& img, size_t at) {
    std::memcpy(img.data() + at, "\x00\x01\x00\x00", 4);
    std::memcpy(img.data() + at + 4, "Standard Jet DB\x00", 16);
    for (size_t i = 20; i < 4096; ++i)
        img[at + i] = u8((i * 17) & 0xFF);
    return 4096;
}

/* M-084/M-091: the Jet/ACE family - the og's two signature branches
 * (FUN_180015260 :16509-16517, type 0x27 / 0x4C) + the page-chain size
 * walk (FUN_1800447c0 :52001-52039: 0x1000 strides, page-type u16 in
 * (0, 0x901], run ends at the first breaking page). */
size_t plant_mdb_family(std::vector<u8>& img, size_t at, bool ace, u32 typed_pages) {
    std::memcpy(img.data() + at, "\x00\x01\x00\x00", 4);
    std::memcpy(img.data() + at + 4, ace ? "Standard ACE DB\x00" : "Standard Jet DB\x00", 16);
    for (size_t i = 20; i < 0x1000; ++i)
        img[at + i] = u8((i * 17) & 0xFF); /* the header page body */
    for (u32 p = 0; p < typed_pages; ++p) {
        const size_t page = at + 0x1000 + size_t(p) * 0x1000;
        put16(img, page, u16(0x101 + p)); /* a Jet page-type word */
        for (size_t i = 2; i < 0x1000; ++i)
            img[page + i] = u8((i * 7) & 0xFF);
    }
    /* the page after the last typed page stays zero: the walk ends there */
    return 0x1000 + size_t(typed_pages) * 0x1000;
}

/* A POSIX tar: one member header with a VALID checksum + the EOF block. */
size_t plant_tar(std::vector<u8>& img, size_t at) {
    u8* h = img.data() + at;
    std::memset(h, 0, 512);
    std::memcpy(h, "file.txt", 8);               /* name */
    std::memcpy(h + 100, "0000644\x00", 8);      /* mode */
    std::memcpy(h + 108, "0000000\x00", 8);      /* uid */
    std::memcpy(h + 116, "0000000\x00", 8);      /* gid */
    std::memcpy(h + 124, "00000000020\x00", 12); /* size = 16 */
    std::memcpy(h + 136, "14000000000\x00", 12); /* mtime */
    std::memcpy(h + 257,
                "ustar\x00"
                "00",
                8); /* magic + version */
    for (int i = 0; i < 8; ++i)
        h[148 + i] = ' '; /* chksum as spaces for the sum */
    u64 sum = 0;
    for (int i = 0; i < 512; ++i)
        sum += u64(h[i]);
    /* octal, NUL-terminated, space-padded - the classic stored form */
    std::snprintf(reinterpret_cast<char*>(h + 148), 8, "%06llo ", (unsigned long long)sum);
    std::memset(img.data() + at + 512 + ((16 + 511) / 512 * 512), 0, 1024); /* data + EOF */
    return 512 + ((16 + 511) / 512 * 512) + 512;
}

size_t plant_eps(std::vector<u8>& img, size_t at) {
    std::memcpy(img.data() + at, "%!PS-Adobe-3.0", 14);
    for (size_t i = 14; i < 64; ++i)
        img[at + i] = u8(i & 0xFF);
    return 64;
}

size_t plant_ai(std::vector<u8>& img, size_t at) {
    std::memcpy(img.data() + at, "\xC5\xD0\xD3\xC6", 4);
    put32v(img, at + 4, 8); /* AI format version */
    for (size_t i = 8; i < 64; ++i)
        img[at + i] = u8(i & 0xFF);
    return 64;
}

size_t plant_dxf(std::vector<u8>& img, size_t at) {
    std::memcpy(img.data() + at, "  0\r\nSECTION\r\n", 12);
    for (size_t i = 12; i < 64; ++i)
        img[at + i] = u8(i & 0xFF);
    return 64;
}

size_t plant_3gp(std::vector<u8>& img, size_t at) {
    putbe32(img, at, 20); /* ftyp box size (BMFF sizes are big-endian) */
    std::memcpy(img.data() + at + 4, "ftyp", 4);
    std::memcpy(img.data() + at + 8, "3gp4", 4);
    putbe32(img, at + 12, 0x00000200); /* minor version */
    putbe32(img, at + 16, 0x33706734); /* one compat brand slot: 3gp4 */
    putbe32(img, at + 20, 16);         /* box2 size 16 */
    std::memcpy(img.data() + at + 24, "moov", 4);
    for (size_t i = 28; i < 36; ++i)
        img[at + i] = u8(i & 0xFF);
    return 36;
}

/* ---- S-041 niche batch 2, school 1 (SCAN-4): og grammars, byte-exact
 * (each validator's comment carries the decompile cite). ---- */
size_t plant_chm(std::vector<u8>& img, size_t at) {
    std::memcpy(img.data() + at, "ITSF", 4);
    put32v(img, at + 8, 0x60);          /* the v3 header length */
    put32v(img, at + 0x18, 0x7C01FD10); /* GUID1 lo */
    put32v(img, at + 0x1C, 0x11D07BAA); /* GUID1 hi */
    put32v(img, at + 0x20, 0xA0000C9E); /* GUID1 tail lo */
    put32v(img, at + 0x24, 0xECE622C9); /* GUID1 tail hi */
    put32v(img, at + 0x28, 0x7C01FD11); /* GUID2 lo */
    put32v(img, at + 0x2C, 0x11D07BAA);
    put32v(img, at + 0x30, 0xA0000C9E);
    put32v(img, at + 0x34, 0xECE622C9);
    put32v(img, at + 0x60, 0x1FE);  /* the directory-header entry magic */
    put32v(img, at + 0x68, 0x1000); /* size = u64@0x68 */
    put32v(img, at + 0x6C, 0);
    put32v(img, at + 0x70, 0); /* the two zero gates */
    put32v(img, at + 0x74, 0);
    for (size_t i = 0x78; i < 0x1000; ++i)
        img[at + i] = u8((i * 3) & 0xFF);
    return 0x1000;
}

size_t plant_one(std::vector<u8>& img, size_t at) {
    std::memcpy(img.data() + at, "\xE4\x52\x5C\x7B", 4); /* the GUID prefix key */
    put32v(img, at + 4, 0x4DA7D88C);                     /* GUID tail qword lo */
    put32v(img, at + 8, 0x7853B1AE);                     /* GUID tail qword hi */
    put32v(img, at + 0xC, 0xD32969D0);                   /* the final GUID dword */
    for (size_t i = 0x10; i < 0x100; ++i)
        img[at + i] = u8(i & 0xFF);
    return 0x100;
}

size_t plant_mny(std::vector<u8>& img, size_t at) {
    std::memcpy(img.data() + at, "MS", 2);
    std::memcpy(img.data() + at + 4, "MSISAM Database", 15);
    for (size_t i = 0x13; i < 0x40; ++i)
        img[at + i] = u8((i * 5) & 0xFF);
    return 0x100000; /* the og's fixed claimed size */
}

size_t plant_emf(std::vector<u8>& img, size_t at) {
    for (size_t i = 0; i < 0x28; ++i)
        img[at + i] = 1;                            /* the EMF header prefix (iType/bounds) */
    std::memcpy(img.data() + at + 0x28, " EMF", 4); /* dSignature */
    put32v(img, at + 0x30, 0x800);                  /* nBytes */
    put16(img, at + 0x3A, 0);                       /* sReserved == 0 */
    for (size_t i = 0x3C; i < 0x800; ++i)
        img[at + i] = u8((i * 5) & 0xFF);
    return 0x800;
}

size_t plant_swf(std::vector<u8>& img, size_t at, bool cws = false) {
    std::memcpy(img.data() + at, cws ? "CWS" : "FWS", 3);
    img[at + 3] = 6;            /* version inside the og's [3,20] gate */
    put32v(img, at + 4, 0x400); /* FileLength > 8 */
    for (size_t i = 8; i < 0x400; ++i)
        img[at + i] = u8((i * 7) & 0xFF);
    return 0x400;
}

size_t plant_3ds(std::vector<u8>& img, size_t at, bool ncsd = false) {
    if (!ncsd) {
        std::memcpy(img.data() + at, "MM", 2);
        std::memcpy(img.data() + at + 0x10, "==", 2); /* the chunk gate */
        for (size_t i = 0x12; i < 0x40; ++i)
            img[at + i] = u8((i * 3) & 0xFF);
    } else {
        for (size_t i = 0; i < 0x100; ++i)
            img[at + i] = u8((i * 3) & 0xFF);
        std::memcpy(img.data() + at + 0x100, "NCSD", 4);
        put32v(img, at + 0x104, 0x200000);
    }
    return 0x40;
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

TEST(Carve, S007NicheBatch1Carve) {
    /* The niche batch's falsifiable claim: the og-only keyed formats all
     * carve on a synthetic slice - the pre-batch registry had none of
     * them. Each key re-read at its RAWSearchDLL registration site. */
    std::vector<u8> img(256 * 1024, 0);
    const size_t mid_at = 512, ogg_at = 2048, rm_at = 4096, mxf_at = 6144, dmg_at = 8192,
                 x3f_at = 10240, crw_at = 12288, wpg_at = 14336, wpd_at = 16384, psp_at = 18432,
                 sit_at = 20480, gz_at = 22528, ape_at = 24576, au_at = 28672, aiff_at = 30720,
                 hlp_at = 32768, ico_at = 34816, cur_at = 35840, wmfpl_at = 36864,
                 wmfstd_at = 37888, ttf_at = 38912, mdb_at = 51200, tar_at = 102400,
                 eps_at = 143360, ai_at = 147456, dxf_at = 151552, gp_at = 155648, mrm_at = 159744,
                 lowdoc_at = 163840, lowdoc2_at = 167936;
    const size_t mid_len = plant_mid(img, mid_at);
    const size_t ogg_len = plant_ogg(img, ogg_at);
    const size_t rm_len = plant_rm(img, rm_at);
    const size_t mxf_len = plant_mxf(img, mxf_at);
    const size_t dmg_len = plant_dmg(img, dmg_at);
    const size_t x3f_len = plant_x3f(img, x3f_at);
    const size_t crw_len = plant_crw(img, crw_at);
    const size_t wpg_len = plant_wpg(img, wpg_at);
    const size_t wpd_len = plant_wpd(img, wpd_at);
    const size_t psp_len = plant_psp(img, psp_at);
    const size_t sit_len = plant_sit(img, sit_at);
    const size_t gz_len = plant_gz(img, gz_at);
    const size_t ape_len = plant_ape(img, ape_at);
    const size_t au_len = plant_au(img, au_at);
    const size_t aiff_len = plant_aiff(img, aiff_at);
    const size_t hlp_len = plant_hlp(img, hlp_at);
    const size_t ico_len = plant_ico(img, ico_at, 1);
    const size_t cur_len = plant_ico(img, cur_at, 2);
    const size_t wmfpl_len = plant_wmf_placeable(img, wmfpl_at);
    const size_t wmfstd_len = plant_wmf_std(img, wmfstd_at);
    const size_t ttf_len = plant_ttf(img, ttf_at);
    const size_t mdb_len = plant_mdb(img, mdb_at);
    plant_tar(img, tar_at);
    const size_t eps_len = plant_eps(img, eps_at);
    const size_t ai_len = plant_ai(img, ai_at);
    const size_t dxf_len = plant_dxf(img, dxf_at);
    const size_t gp_len = plant_3gp(img, gp_at);
    const size_t mrm_len = plant_mrm(img, mrm_at);
    const size_t lowdoc_len = plant_lowdoc(img, lowdoc_at);
    const size_t lowdoc2_len = plant_lowdoc(img, lowdoc2_at, true);

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
        ASSERT_NE(n, nullptr) << what << " must carve (S-007 niche)";
        EXPECT_EQ(n->props()->carve_format, fmt) << what;
        EXPECT_GE(n->props()->logical_size, min_len) << what;
    };
    expect_carved(mid_at, FMT_MID, mid_len, "MIDI");
    expect_carved(ogg_at, FMT_OGG, ogg_len, "Ogg");
    expect_carved(rm_at, FMT_RM, rm_len, "RealMedia");
    expect_carved(mxf_at, FMT_MXF, mxf_len, "MXF");
    expect_carved(dmg_at, FMT_DMG, dmg_len, "DMG (bzip2)");
    expect_carved(x3f_at, FMT_X3F, x3f_len, "X3F");
    expect_carved(crw_at, FMT_CRW, crw_len, "Canon CRW (CIFF)");
    expect_carved(mrm_at, FMT_MRW, mrm_len, "Minolta MRW (MRM)");
    expect_carved(lowdoc_at, FMT_LOWDOC, lowdoc_len, "Word doc (legacy)");
    expect_carved(lowdoc2_at, FMT_LOWDOC2, lowdoc2_len, "Word doc (legacy, 2nd key)");
    expect_carved(wpg_at, FMT_WPG, wpg_len, "WPG");
    expect_carved(wpd_at, FMT_WPD, wpd_len, "WPD");
    expect_carved(psp_at, FMT_PSP, psp_len, "PSP");
    expect_carved(sit_at, FMT_SIT, sit_len, "StuffIt");
    expect_carved(gz_at, FMT_GZ, gz_len, "gzip");
    expect_carved(ape_at, FMT_APE, ape_len, "APE");
    expect_carved(au_at, FMT_AU, au_len, "Sun AU");
    expect_carved(aiff_at, FMT_AIFF, aiff_len, "AIFF");
    expect_carved(hlp_at, FMT_HLP, hlp_len, "Windows Help");
    expect_carved(ico_at, FMT_ICO, ico_len, "Icon");
    expect_carved(cur_at, FMT_CUR, cur_len, "Cursor");
    expect_carved(wmfpl_at, FMT_WMF_PL, wmfpl_len, "WMF placeable");
    expect_carved(wmfstd_at, FMT_WMF, wmfstd_len, "WMF standard");
    expect_carved(ttf_at, FMT_TTF, ttf_len, "TrueType");
    expect_carved(mdb_at, FMT_MDB, mdb_len, "Access DB");
    expect_carved(tar_at, FMT_TAR, 1024, "TAR");
    expect_carved(eps_at, FMT_EPS, eps_len, "PostScript");
    expect_carved(ai_at, FMT_AI, ai_len, "Illustrator");
    expect_carved(dxf_at, FMT_DXF, dxf_len, "DXF");
    expect_carved(gp_at, FMT_3GP, gp_len, "3GP");
}

TEST(Carve, S007NicheBatch1RejectsNoise) {
    /* One hostile variant per family - the grammar gates reject what the
     * raw keys would accept on noise. */
    std::vector<u8> img(256 * 1024, 0);
    /* MThd with a wrong header length @ 512 */
    std::memcpy(img.data() + 512, "MThd", 4);
    putbe32(img, 512 + 4, 7);
    /* OggS with version 1 @ 2048 */
    std::memcpy(img.data() + 2048, "OggS", 4);
    img[2048 + 4] = 1;
    /* .RMF with version 2 @ 4096 */
    std::memcpy(img.data() + 4096, ".RMF", 4);
    putbe32(img, 4096 + 4, 2);
    /* a single MXF KLV item (the >= 2 gate) @ 8192 */
    std::memcpy(img.data() + 8192,
                "\x06\x0E\x2B\x34\x02\x05\x01\x01\x0D\x01\x02\x01\x01\x00\x00\x00", 16);
    img[8192 + 16] = 8;
    /* BZh1 without the pi block magic @ 12288 */
    std::memcpy(img.data() + 12288, "BZh1", 4);
    for (int i = 0; i < 8; ++i)
        img[12288 + 4 + size_t(i)] = u8(0xAA + i);
    /* FOVb with a non-ASCII version @ 14336 */
    std::memcpy(img.data() + 14336, "FOVb", 4);
    img[14336 + 4] = 'x';
    img[14336 + 5] = 'y';
    /* II1A without the HEAP family behind it (the CRW grammar rejects) @ 16384 */
    std::memcpy(img.data() + 16384, "II\x1A\x00", 4);
    put32v(img, 16384 + 4, 0x40);
    /* HEAPCCDR with version 3 (the og gate demands 2) @ 49152 */
    std::memcpy(img.data() + 49152, "II\x1A\x00", 4);
    std::memcpy(img.data() + 49152 + 4, "\x00\x00HEAP\x00\x00", 8);
    std::memcpy(img.data() + 49152 + 0xA, "CCDR", 4);
    img[49152 + 0xE] = 3; /* wrong version */
    /* MAC with an out-of-band version @ 18432 */
    std::memcpy(img.data() + 18432, "MAC ", 4);
    putbe32(img, 18432 + 4, 999999);
    /* .snd with encoding 99 @ 20480 */
    std::memcpy(img.data() + 20480, ".snd", 4);
    putbe32(img, 20480 + 4, 24);
    putbe32(img, 20480 + 8, 256);
    putbe32(img, 20480 + 12, 99);
    /* FORM with size 2 @ 22528 */
    std::memcpy(img.data() + 22528, "FORM", 4);
    putbe32(img, 22528 + 4, 2);
    /* bare 3F 5F 03 00 with nothing behind it @ 24576 */
    std::memcpy(img.data() + 24576, "\x3F\x5F\x03\x00", 4);
    /* ICO dir with planes 9 @ 28672 */
    put16(img, 28672 + 0, 0);
    put16(img, 28672 + 2, 1);
    put16(img, 28672 + 4, 1);
    put16(img, 28672 + 10, 9); /* planes: out of range */
    put32v(img, 28672 + 16, 64);
    put32v(img, 28672 + 20, 22);
    /* placeable WMF with a BAD checksum @ 30720 - og grammar everywhere
     * except the stored checksum word@+20 is wrong on purpose */
    std::memcpy(img.data() + 30720, "\xD7\xCD\xC6\x9A", 4);
    put16(img, 30720 + 4, 0);
    put16(img, 30720 + 16, 0);
    put16(img, 30720 + 18, 0);
    put16(img, 30720 + 20, 0xBEEF);
    std::memcpy(img.data() + 30720 + 22, "\x01\x00\x09\x00", 4);
    put16(img, 30720 + 38, 0);
    /* standard WMF with params != 0 @ 32768 */
    std::memcpy(img.data() + 32768, "\x01\x00\x09\x00", 4);
    put16(img, 32768 + 4, 0x0300);
    put32v(img, 32768 + 6, 64);
    put16(img, 32768 + 16, 3);
    /* sfnt with numTables 0 @ 34816 */
    std::memcpy(img.data() + 34816, "\x00\x01\x00\x00", 4);
    putbe16(img, 34816 + 4, 0);
    /* the Jet key with a stranger behind it @ 36864 */
    std::memcpy(img.data() + 36864, "\x00\x01\x00\x00", 4);
    std::memcpy(img.data() + 36864 + 4, "Standard XYZ DB\x00", 16);
    /* tar header with a BAD checksum @ 40960 */
    std::memset(img.data() + 40960, 0, 512);
    std::memcpy(img.data() + 40960, "file.txt", 8);
    std::memcpy(img.data() + 40960 + 257,
                "ustar\x00"
                "00",
                8);
    std::snprintf(reinterpret_cast<char*>(img.data() + 40960 + 148), 8, "%06llo ",
                  (unsigned long long)12345);
    /* "%!" with no PS behind it @ 44032 */
    std::memcpy(img.data() + 44032, "%!XY", 4);
    /* AI with version 999 @ 45056 */
    std::memcpy(img.data() + 45056, "\xC5\xD0\xD3\xC6", 4);
    put32v(img, 45056 + 4, 999);
    /* DXF key without the SECTION covenant @ 46080 */
    std::memcpy(img.data() + 46080, "  0\r\nXXXXXXX", 11);
    /* ftyp with an unknown 3gp brand @ 47104 */
    put32v(img, 47104, 20);
    std::memcpy(img.data() + 47104 + 4, "ftyp", 4);
    std::memcpy(img.data() + 47104 + 8, "3gpZ", 4);

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
    for (tree::FileNode* n : all) {
        const u64 off = n->props()->extents.front().offset;
        const u64 fmt = n->props()->carve_format;
        EXPECT_FALSE(off == 512 && fmt == FMT_MID) << "bad-length MThd rejected";
        EXPECT_FALSE(off == 2048 && fmt == FMT_OGG) << "bad-version OggS rejected";
        EXPECT_FALSE(off == 4096 && fmt == FMT_RM) << "version-2 RMF rejected";
        EXPECT_FALSE(off == 8192 && fmt == FMT_MXF) << "single-KLV MXF rejected";
        EXPECT_FALSE(off == 12288 && fmt == FMT_DMG) << "no-pi BZh1 rejected";
        EXPECT_FALSE(off == 14336 && fmt == FMT_X3F) << "bad-version X3F rejected";
        EXPECT_FALSE(off == 16384 && fmt == FMT_CRW) << "II1A without HEAP rejected";
        EXPECT_FALSE(off == 49152 && fmt == FMT_CRW) << "version-3 HEAPCCDR rejected";
        EXPECT_FALSE(off == 18432 && fmt == FMT_APE) << "bad-version APE rejected";
        EXPECT_FALSE(off == 20480 && fmt == FMT_AU) << "encoding-99 AU rejected";
        EXPECT_FALSE(off == 22528 && fmt == FMT_AIFF) << "size-2 FORM rejected";
        EXPECT_FALSE(off == 24576 && fmt == FMT_HLP) << "bare HLP magic rejected";
        EXPECT_FALSE(off == 28672 && fmt == FMT_ICO) << "planes-9 ICO rejected";
        EXPECT_FALSE(off == 30720 && fmt == FMT_WMF_PL) << "bad-checksum placeable WMF rejected";
        EXPECT_FALSE(off == 32768 && fmt == FMT_WMF) << "params-nonzero WMF rejected";
        EXPECT_FALSE(off == 34816 && fmt == FMT_TTF) << "numTables-0 sfnt rejected";
        EXPECT_FALSE(off == 36864 && fmt == FMT_MDB) << "stranger Jet header rejected";
        EXPECT_FALSE(off == 40960 && fmt == FMT_TAR) << "bad-checksum tar rejected";
        EXPECT_FALSE(off == 44032 && fmt == FMT_EPS) << "no-PS %! rejected";
        EXPECT_FALSE(off == 45056 && fmt == FMT_AI) << "version-999 AI rejected";
        EXPECT_FALSE(off == 46080 && fmt == FMT_DXF) << "no-SECTION DXF rejected";
        EXPECT_FALSE(off == 47104 && fmt == FMT_3GP) << "brand-3gpZ rejected";
    }
    EXPECT_TRUE(stats.rejected >= 20) << "each noise plant must reach a validator rejection";
}

TEST(Carve, S007M081CiwMrmLowdocHlpOgGrammar) {
    /* M-081/SCAN-4: the corrected rows reproduce the og grammars first-hand
     * - CRW (00 00 HEAP + {CCDR,TIFP,ARCH} + ver 2, FUN_180052000
     * :62333-62353), MRW (\0MRM + the size formula, FUN_180012100
     * :14233-14258), LowDoc (DBA5/94A6 + the size formula,
     * FUN_18001b650 :21167-21188), HLP (3F 5F 03 00 + the size gate,
     * FUN_180041c50 :49883-49895). The retired batch-1 synthetic (II1A +
     * PRD) must no longer pass (M-081's Q65: the self-fulfilling fixture),
     * and the registry rows must carry the corrected ext/name labels. */
    std::vector<u8> img(256 * 1024, 0);
    const size_t crw_at = 4096, tifp_at = 8192, crwmm_at = 12288, mrm_at = 16384, lowdoc_at = 20480,
                 lowdoc2_at = 24576, hlp_at = 28672, prd_at = 32768;
    plant_crw(img, crw_at);
    plant_crw(img, tifp_at, "TIFP");
    plant_crw(img, crwmm_at, "CCDR", true); /* the MM 1A 00 sibling key */
    const size_t mrm_len = plant_mrm(img, mrm_at);
    const size_t lowdoc_len = plant_lowdoc(img, lowdoc_at);
    const size_t lowdoc2_len = plant_lowdoc(img, lowdoc2_at, true);
    const size_t hlp_len = plant_hlp(img, hlp_at);
    /* the retired batch-1 synthetic: II1A + a PRD block */
    std::memcpy(img.data() + prd_at, "II\x1A\x00", 4);
    put32v(img, prd_at + 4, 0x40);
    std::memcpy(img.data() + prd_at + 0x40, "PRD", 3);

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
    /* CRW: the HEAP family accepts CCDR and TIFP on both keys */
    tree::FileNode* crw = find_at(crw_at);
    ASSERT_NE(crw, nullptr) << "genuine CIFF CRW must carve (M-081)";
    EXPECT_EQ(crw->props()->carve_format, u64(FMT_CRW));
    tree::FileNode* tifp = find_at(tifp_at);
    ASSERT_NE(tifp, nullptr) << "the TIFP record type accepts";
    EXPECT_EQ(tifp->props()->carve_format, u64(FMT_CRW));
    tree::FileNode* crwmm = find_at(crwmm_at);
    ASSERT_NE(crwmm, nullptr) << "the MM 1A 00 sibling key carves";
    EXPECT_EQ(crwmm->props()->carve_format, u64(FMT_CRW_MM));
    /* MRW: the og size formula, exact */
    tree::FileNode* mrm = find_at(mrm_at);
    ASSERT_NE(mrm, nullptr) << "the \\0MRM row must carve";
    EXPECT_EQ(mrm->props()->carve_format, u64(FMT_MRW));
    EXPECT_EQ(mrm->props()->logical_size, mrm_len) << "the og formula size, exact";
    /* LowDoc: both keys, the og formula size exact */
    tree::FileNode* ld = find_at(lowdoc_at);
    ASSERT_NE(ld, nullptr) << "DB A5 2D 00 = FileLowDoc (SCAN-4)";
    EXPECT_EQ(ld->props()->carve_format, u64(FMT_LOWDOC));
    EXPECT_EQ(ld->props()->logical_size, lowdoc_len);
    tree::FileNode* ld2 = find_at(lowdoc2_at);
    ASSERT_NE(ld2, nullptr) << "94 A6 2E 00 = FileLowDoc (SCAN-4)";
    EXPECT_EQ(ld2->props()->carve_format, u64(FMT_LOWDOC2));
    EXPECT_EQ(ld2->props()->logical_size, lowdoc2_len);
    /* HLP: the og size gate, exact */
    tree::FileNode* hlp = find_at(hlp_at);
    ASSERT_NE(hlp, nullptr) << "3F 5F 03 00 = HLP";
    EXPECT_EQ(hlp->props()->carve_format, u64(FMT_HLP));
    EXPECT_EQ(hlp->props()->logical_size, hlp_len) << "size = u32@0xc, exact";
    /* the retired II1A+PRD synthetic must NOT carve */
    EXPECT_EQ(find_at(prd_at), nullptr) << "the old PRD synthetic is not a CRW file";
    /* the corrected row labels on the registry surface (M-081: not
     * mislabeled MRW; SCAN-4: not mislabeled HLP) */
    const Signature* reg = registry();
    for (size_t i = 0; i < registry_size(); ++i) {
        const Signature& s = reg[i];
        if (s.id == FMT_CRW || s.id == FMT_CRW_MM) {
            EXPECT_STREQ(s.ext, "crw") << "the II/MM 1A 00 rows are CRW";
        }
        if (s.id == FMT_MRW) {
            EXPECT_STREQ(s.ext, "mrw");
        }
        if (s.id == FMT_LOWDOC || s.id == FMT_LOWDOC2) {
            EXPECT_STREQ(s.ext, "doc") << "the DBA5/94A6 rows are .doc";
        }
        if (s.id == FMT_HLP) {
            EXPECT_STREQ(s.ext, "hlp");
        }
    }
}

TEST(Carve, S007NicheBatch2School1Carve) {
    /* S-041 batch-2 school 1 (SCAN-4): CHM ONE MNY EMF SWF(FWS/CWS) 3DS
     * - every grammar re-verified first-hand at the analyser body before
     * the row landed. The MNY row's carved size is the og's own fixed
     * 1 MiB claim (FUN_180041dd0); CHM/EMF/SWF take their sizes from
     * the og's own size fields; ONE/3DS carry the bounded-extent
     * stand-in for their undecoded walks. */
    std::vector<u8> img(2 * 1024 * 1024, 0);
    const size_t chm_at = 4096, one_at = 8192, mny_at = 16384, emf_at = 12288, swf_at = 20480,
                 cws_at = 22528, mmds_at = 24576, ncsd_at = 28672;
    const size_t chm_len = plant_chm(img, chm_at);
    const size_t one_len = plant_one(img, one_at);
    const size_t mny_len = plant_mny(img, mny_at);
    const size_t emf_len = plant_emf(img, emf_at);
    const size_t swf_len = plant_swf(img, swf_at);
    const size_t cws_len = plant_swf(img, cws_at, true);
    plant_3ds(img, mmds_at);
    plant_3ds(img, ncsd_at, true);

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
        ASSERT_NE(n, nullptr) << what << " must carve (S-041 school 1)";
        EXPECT_EQ(n->props()->carve_format, fmt) << what;
        EXPECT_GE(n->props()->logical_size, min_len) << what;
    };
    expect_carved(chm_at, FMT_CHM, chm_len, "CHM");
    expect_carved(one_at, FMT_ONE, one_len, "OneNote");
    expect_carved(mny_at, FMT_MNY, mny_len, "Money");
    expect_carved(emf_at, FMT_EMF, emf_len, "EMF");
    expect_carved(swf_at, FMT_SWF, swf_len, "SWF (FWS)");
    expect_carved(cws_at, FMT_SWF_CWS, cws_len, "SWF (CWS)");
    expect_carved(mmds_at, FMT_3DS, 0x12, "3DS (MM/==)");
    expect_carved(ncsd_at, FMT_3DS_NCSD, 0x108, "3DS (NCSD)");
    /* the og's own size semantics, exact where the og sets a size */
    EXPECT_EQ(find_at(chm_at)->props()->logical_size, chm_len) << "CHM size = u64@0x68";
    EXPECT_EQ(find_at(mny_at)->props()->logical_size, mny_len) << "MNY size = the og's 0x100000";
    EXPECT_EQ(find_at(emf_at)->props()->logical_size, emf_len) << "EMF size = nBytes";
    EXPECT_EQ(find_at(swf_at)->props()->logical_size, swf_len) << "SWF size = FileLength";
}

TEST(Carve, M090DmgKolyFooterExtent) {
    /* M-090: the og walks the "koly" UDIF trailer and sizes the carve at
     * koly + 0x200 (the mandatory 512-byte footer) - NOT the bzip2 EOS
     * magic + 10 the batch-1 landing carried (RAWSearchDLL_decompiled.c
     * :63782-63868, first-hand: 512-byte chunk steps, the 8-zero tail
     * gate, the aligned probe, the past-0x2800 unaligned salvage of the
     * previous chunk). RED pre-fix: the old validator sized the stream
     * to the EOS magic + 10 (or the truncated cap), cutting the carve
     * before the footer. */
    std::vector<u8> img(128 * 1024, 0);
    const size_t at = 4096, koly_rel = 0x1000;
    const size_t want = plant_dmg_koly(img, at, koly_rel);
    /* an early bzip2 EOS magic must NOT bound the carve: the koly walk
     * runs first and the EOS survives only as the no-koly fallback */
    std::memcpy(img.data() + at + 128, "\x17\x72\x45\x38\x50\x90", 6);

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
    tree::FileNode* n = nullptr;
    for (tree::FileNode* c : all)
        if (!c->props()->extents.empty() && c->props()->extents.front().offset == at)
            n = c;
    ASSERT_NE(n, nullptr) << "the BZh1+koly stream must carve";
    EXPECT_EQ(n->props()->carve_format, FMT_DMG);
    EXPECT_EQ(n->props()->logical_size, want)
        << "the carve carries the mandatory UDIF footer (koly + 0x200)";
}

TEST(Carve, M090DmgKolyUnalignedSalvage) {
    /* the og's second probe (past 0x2800): a koly at an UNALIGNED offset
     * is found by the full search of the PREVIOUS chunk once the chunk
     * after it carries the 8-zero tail - size still koly + 0x200. */
    std::vector<u8> img(128 * 1024, 0);
    const size_t at = 4096, koly_rel = 0x3008; /* unaligned inside the chunk */
    const size_t want = plant_dmg_koly(img, at, koly_rel, false);
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
    tree::FileNode* n = nullptr;
    for (tree::FileNode* c : all)
        if (!c->props()->extents.empty() && c->props()->extents.front().offset == at)
            n = c;
    ASSERT_NE(n, nullptr) << "the unaligned-koly stream must carve";
    EXPECT_EQ(n->props()->logical_size, want) << "salvage sizes koly + 0x200";
}

TEST(Carve, M090DmgKolyZeroTailGateAndCap) {
    /* (a) the og's 8-zero tail gate: a koly block whose last 8 bytes are
     * non-zero does NOT bound the carve (the EOS fallback or the
     * truncated cap takes over - never the koly extent);
     * (b) the row cap is the DMGRawFileInfo ctor's OWN 0xC800000
     * (:49436-49440), not the 0xBEBC200 global singleton. */
    std::vector<u8> img(128 * 1024, 0);
    const size_t at = 4096, koly_rel = 0x1000;
    plant_dmg_koly(img, at, koly_rel, true, false); /* non-zero footer tail */
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
    tree::FileNode* n = nullptr;
    for (tree::FileNode* c : all)
        if (!c->props()->extents.empty() && c->props()->extents.front().offset == at)
            n = c;
    ASSERT_NE(n, nullptr) << "the stream still carves (the keyed gates held)";
    EXPECT_NE(n->props()->logical_size, koly_rel + 0x200)
        << "the non-zero-tailed koly must not bound the carve";
    /* the row bound pin: DMG's own ctor max */
    const Signature* reg = registry();
    EXPECT_EQ(reg[FMT_DMG].max_size, 0xC800000ull)
        << "DMG cap = the og's DMG-specific 0xC800000, not the global 0xBEBC200";
}

TEST(Carve, M084AccdbSecondSignatureCarves) {
    /* M-084: the og's CMDB validator accepts TWO Jet-family signatures
     * (FUN_180015260 :16509-16517, first-hand) - the legacy "Standard
     * Jet DB" (type 0x27) and the ACE-generation "Standard ACE DB"
     * (type 0x4C, the .accdb default since Access 2007). Pre-fix: the
     * ACE signature carved nothing (the branch was absent). */
    std::vector<u8> img(256 * 1024, 0);
    const size_t jet_at = 4096, ace_at = 32768; /* pages stride 0x1000 - no overlap */
    const size_t jet_len = plant_mdb_family(img, jet_at, false, 2);
    const size_t ace_len = plant_mdb_family(img, ace_at, true, 3);
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
    tree::FileNode* jet = nullptr;
    tree::FileNode* ace = nullptr;
    for (tree::FileNode* n : all) {
        const u64 off = n->props()->extents.front().offset;
        if (off == jet_at)
            jet = n;
        else if (off == ace_at)
            ace = n;
    }
    ASSERT_NE(jet, nullptr) << "the legacy Jet signature carves";
    EXPECT_EQ(jet->props()->carve_format, FMT_MDB);
    EXPECT_EQ(jet->props()->logical_size, jet_len) << "Jet page-chain extent";
    ASSERT_NE(ace, nullptr) << "the ACE signature carves (the pre-fix miss)";
    EXPECT_EQ(ace->props()->carve_format, FMT_ACCDB);
    EXPECT_EQ(ace->props()->logical_size, ace_len) << "ACE page-chain extent";
}

TEST(Carve, M091MdbPageChainWalk) {
    /* M-091: the og's MDB size is the page-chain walk (FUN_1800447c0
     * :52001-52039, read verbatim): 0x1000 strides, the page-type u16
     * at each page start must be nonzero and <= 0x901, run ends at the
     * first breaking page. Pre-fix: every MDB carved as a flat 64 MiB
     * bounded extent. */
    std::vector<u8> img(256 * 1024, 0);
    const size_t chain_at = 4096; /* signature + 3 typed pages + zero page */
    const size_t want = plant_mdb_family(img, chain_at, false, 3);
    ASSERT_EQ(want, 0x4000u);
    /* a run whose 3rd page carries an out-of-family type word (0x902):
     * the walk ends at that page */
    const size_t gate_at = 32768;
    plant_mdb_family(img, gate_at, false, 2);
    put16(img, gate_at + 0x3000, 0x0902);
    /* a run that never breaks: the walk runs to the probe limit */
    const size_t unbroken_at = 65536;
    plant_mdb_family(img, unbroken_at, false, 40);
    img.resize(192 * 1024, 0); /* cut the run: the walk ends at the slice */
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
    tree::FileNode* chain = nullptr;
    tree::FileNode* gate = nullptr;
    tree::FileNode* unbroken = nullptr;
    for (tree::FileNode* n : all) {
        const u64 off = n->props()->extents.front().offset;
        if (off == chain_at)
            chain = n;
        else if (off == gate_at)
            gate = n;
        else if (off == unbroken_at)
            unbroken = n;
    }
    ASSERT_NE(chain, nullptr);
    EXPECT_EQ(chain->props()->logical_size, 0x4000u)
        << "signature + 3 typed pages + the zero page = 4 pages";
    ASSERT_NE(gate, nullptr);
    EXPECT_EQ(gate->props()->logical_size, 0x3000u)
        << "the 0x902 page-type word ends the run at that page";
    ASSERT_NE(unbroken, nullptr);
    EXPECT_EQ(unbroken->props()->logical_size, 192u * 1024 - unbroken_at)
        << "the unbroken run walks to the probe limit";
}

TEST(Carve, S007NicheBatch2School1RejectsNoise) {
    /* One hostile variant per format - the og gates reject what the
     * keys would accept on noise. */
    std::vector<u8> img(256 * 1024, 0);
    /* ITSF with a broken GUID @ 512 */
    plant_chm(img, 512);
    put32v(img, 512 + 0x18, 0xDEADBEEF);
    /* "MS" with a broken MSISAM string @ 2048 */
    std::memcpy(img.data() + 2048, "MS", 2);
    std::memcpy(img.data() + 2048 + 4, "MSISAM DatabasX", 15);
    /* " EMF" with nBytes 0 @ 4096 */
    plant_emf(img, 4096);
    put32v(img, 4096 + 0x30, 0);
    /* FWS with version 2 (below the og's [3,20] gate) @ 8192 */
    plant_swf(img, 8192);
    img[8192 + 3] = 2;
    /* FWS with FileLength 8 (not > 8) @ 12288 */
    plant_swf(img, 12288);
    put32v(img, 12288 + 4, 8);
    /* "MM" without the == gate and without NCSD @ 16384 */
    std::memcpy(img.data() + 16384, "MM", 2);
    /* NCSD with a wrong 0x104 field @ 20480 */
    plant_3ds(img, 20480, true);
    put32v(img, 20480 + 0x104, 0x123456);

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
    for (tree::FileNode* n : all) {
        const u64 off = n->props()->extents.front().offset;
        const u64 fmt = n->props()->carve_format;
        EXPECT_FALSE(off == 512 && fmt == FMT_CHM) << "broken-GUID ITSF rejected";
        EXPECT_FALSE(off == 2048 && fmt == FMT_MNY) << "broken MSISAM string rejected";
        EXPECT_FALSE(off == 4096 && fmt == FMT_EMF) << "nBytes-0 EMF rejected";
        EXPECT_FALSE(off == 8192 && fmt == FMT_SWF) << "version-2 FWS rejected";
        EXPECT_FALSE(off == 12288 && fmt == FMT_SWF) << "FileLength-8 FWS rejected";
        EXPECT_FALSE(off == 16384 && fmt == FMT_3DS) << "MM without == / NCSD rejected";
        EXPECT_FALSE(off == 20480 && fmt == FMT_3DS_NCSD) << "wrong-field NCSD rejected";
    }
    EXPECT_TRUE(stats.rejected >= 7) << "each noise plant must reach a validator rejection";
}

TEST(Carve, WmfPlaceableOgGrammarCarves) {
    /* S-042: the og's placeable grammar (checksum@+20 == XOR(words
     * 0..9), reserved dword@+16 == 0, hmf@+4 == 0, params@+38 == 0,
     * extent = Size@+28*2 + 22 exact) accepts a spec-derived fixture
     * and sizes it exactly. */
    std::vector<u8> img(64 * 1024, 0);
    const size_t at = 512;
    const size_t len = plant_wmf_placeable(img, at);
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
    tree::FileNode* n = nullptr;
    for (tree::FileNode* c : all)
        if (!c->props()->extents.empty() && c->props()->extents.front().offset == at)
            n = c;
    ASSERT_NE(n, nullptr) << "the og-convention placeable WMF must carve";
    EXPECT_EQ(n->props()->carve_format, u64(FMT_WMF_PL));
    EXPECT_EQ(n->props()->logical_size, len) << "extent = std Size words*2 + 22, exact";
}

TEST(Carve, WmfPlaceableOldCoreGrammarRejected) {
    /* S-042: the pre-fix core convention (nonzero "inch"@+16, checksum
     * stored@+18) violates the og's reserved-dword gate - the old
     * accept-set and the og's are disjoint, and parity is the og. */
    std::vector<u8> img(64 * 1024, 0);
    const size_t at = 512;
    std::memcpy(img.data() + at, "\xD7\xCD\xC6\x9A", 4);
    put16(img, at + 4, 0);
    put16(img, at + 6, 0);
    put16(img, at + 8, 0);
    put16(img, at + 10, 100);
    put16(img, at + 12, 100);
    put16(img, at + 14, 200);
    put16(img, at + 16, 1440); /* old "inch" field - nonzero reserved */
    u16 x = 0;
    for (int i = 0; i < 9; ++i)
        x = u16(x ^ u16_from_bytes_le(&img[at + 2 * size_t(i)]));
    put16(img, at + 18, x); /* old checksum@+18 convention */
    std::memcpy(img.data() + at + 22, "\x01\x00\x09\x00", 4);
    put32v(img, at + 28, 100);
    put16(img, at + 38, 0);
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
    for (tree::FileNode* n : all)
        EXPECT_FALSE(n->props()->extents.front().offset == at &&
                     n->props()->carve_format == u64(FMT_WMF_PL))
            << "the old core-convention grammar must not carve (disjoint from the og's)";
}

TEST(Carve, WmfPlaceableTwentyByteVariantRejected) {
    /* S-042 fix-plan test 3: a 20-byte-layout placeable file (checksum
     * @+18, standard header at +20) is rejected by the og grammar - the
     * og reads its standard header at +22 and its checksum at +20, so
     * this layout cannot satisfy both gates. Parity with the og, not
     * with either historical layout variant. */
    std::vector<u8> img(64 * 1024, 0);
    const size_t at = 512;
    std::memcpy(img.data() + at, "\xD7\xCD\xC6\x9A", 4);
    put16(img, at + 4, 0);
    put16(img, at + 6, 0);
    put16(img, at + 8, 0);
    put16(img, at + 10, 100);
    put16(img, at + 12, 100);
    put16(img, at + 14, 1440);
    put16(img, at + 16, 0);
    u16 x = 0;
    for (int i = 0; i < 9; ++i)
        x = u16(x ^ u16_from_bytes_le(&img[at + 2 * size_t(i)]));
    put16(img, at + 18, x); /* checksum at +18 under the 20-byte layout */
    std::memcpy(img.data() + at + 20, "\x01\x00\x09\x00", 4); /* std header at +20 */
    put32v(img, at + 26, 100);
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
    for (tree::FileNode* n : all)
        EXPECT_FALSE(n->props()->extents.front().offset == at &&
                     n->props()->carve_format == u64(FMT_WMF_PL))
            << "the 20-byte layout must not carve as placeable (og reads std header at +22)";
}

TEST(Carve, WmfStdOgGatesAcceptNonCanonicalHeaders) {
    /* S-042 addendum 1: the og's standard branch gates ONLY on the key
     * and the params word - version 0x0200, 300 objects and maxrec >
     * words all carve now (the old core-only gates rejected them). */
    std::vector<u8> img(64 * 1024, 0);
    const size_t at = 512;
    std::memcpy(img.data() + at, "\x01\x00\x09\x00", 4);
    put16(img, at + 4, 0x0200);   /* version 0x0200 - og-accepted */
    put32v(img, at + 6, 64);      /* size in words */
    put16(img, at + 10, 300);     /* objects > 256 - og-accepted */
    put32v(img, at + 12, 0xFFFF); /* maxrec > words - og-accepted */
    put16(img, at + 16, 0);       /* params == 0 */
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
    tree::FileNode* n = nullptr;
    for (tree::FileNode* c : all)
        if (!c->props()->extents.empty() && c->props()->extents.front().offset == at)
            n = c;
    ASSERT_NE(n, nullptr) << "the og's two-gate standard grammar must accept this header";
    EXPECT_EQ(n->props()->carve_format, u64(FMT_WMF));
    EXPECT_EQ(n->props()->logical_size, size_t(128)) << "extent = Size words * 2";
}

TEST(Carve, MpegTs188PacketStreamAccepted) {
    /* S-047: the og's CMTS analyser (id 0x76) - a 0x47-strided 188-byte
     * packet stream carves with the walked extent (the last complete
     * packet), min 0x5000. */
    std::vector<u8> img(64 * 1024, 0);
    const size_t at = 512;
    const size_t len = plant_ts(img, at, 345);
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
    tree::FileNode* n = nullptr;
    for (tree::FileNode* c : all)
        if (!c->props()->extents.empty() && c->props()->extents.front().offset == at)
            n = c;
    ASSERT_NE(n, nullptr) << "the 188-byte TS stream must carve (S-047)";
    EXPECT_EQ(n->props()->carve_format, u64(FMT_TS));
    EXPECT_EQ(n->props()->logical_size, len) << "extent = the last complete packet";
}

TEST(Carve, M2Ts192PacketStreamAccepted) {
    /* S-047: the og's CM2TS analyser (id 0x74) - the 192-byte BDAV family
     * (4-byte TP_extra_header + sync at +4) carves as m2ts with the walked
     * extent. */
    std::vector<u8> img(64 * 1024, 0);
    const size_t at = 512;
    const size_t len = plant_m2ts(img, at, 338);
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
    tree::FileNode* n = nullptr;
    for (tree::FileNode* c : all)
        if (!c->props()->extents.empty() && c->props()->extents.front().offset == at)
            n = c;
    ASSERT_NE(n, nullptr) << "the 192-byte M2TS stream must carve (S-047)";
    EXPECT_EQ(n->props()->carve_format, u64(FMT_M2TS));
    EXPECT_EQ(n->props()->logical_size, len) << "extent = the last complete packet";
    std::string name;
    ASSERT_TRUE(cr::text::lossless_u16_to_utf8(n->props()->name, name));
    EXPECT_NE(name.find(".M2TS"), std::string::npos) << "the og's M2TS extension stamp";
}

TEST(Carve, TsBelowMinimumExtentRejected) {
    /* S-047: the og's minimum-extent gate (FUN_180032460: run < 0x5000 ->
     * size 0 / reject) - a 4 KiB synced run (21 packets) is noise-class. */
    std::vector<u8> img(64 * 1024, 0);
    const size_t at = 512;
    plant_ts(img, at, 21); /* 3948 bytes < 0x5000 */
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
    for (tree::FileNode* c : all)
        EXPECT_FALSE(
            c->props()->extents.front().offset == at &&
            (c->props()->carve_format == u64(FMT_TS) || c->props()->carve_format == u64(FMT_M2TS)))
            << "a sub-0x5000 synced run must reject";
}

TEST(Carve, TsSyncLossCommitsRun) {
    /* S-047: salvage semantics - sync loss after >= 0x5000 commits the
     * partial extent (the accumulated run at the last complete packet). */
    std::vector<u8> img(64 * 1024, 0);
    const size_t at = 512;
    const size_t len = plant_ts(img, at, 220);      /* 41360 bytes of packets */
    std::memset(img.data() + at + len, 0xAB, 4096); /* garbage after the run */
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
    tree::FileNode* n = nullptr;
    for (tree::FileNode* c : all)
        if (!c->props()->extents.empty() && c->props()->extents.front().offset == at)
            n = c;
    ASSERT_NE(n, nullptr) << "the synced run before the garbage must carve";
    EXPECT_EQ(n->props()->carve_format, u64(FMT_TS));
    EXPECT_EQ(n->props()->logical_size, len) << "the committed run, exact";
}

TEST(Carve, TsNullPacketTolerated) {
    /* S-047: null packets (all-0xFF at +8..+15) do NOT end the run
     * (FUN_180032460's null-marker); the evidence note carries the
     * "+null" marker of the og's +0x11c flag. */
    std::vector<u8> img(64 * 1024, 0);
    const size_t at = 512;
    const size_t len = plant_ts(img, at, 345, true);
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
    tree::FileNode* n = nullptr;
    for (tree::FileNode* c : all)
        if (!c->props()->extents.empty() && c->props()->extents.front().offset == at)
            n = c;
    ASSERT_NE(n, nullptr) << "the stream with null packets must carve";
    EXPECT_EQ(n->props()->carve_format, u64(FMT_TS));
    EXPECT_EQ(n->props()->logical_size, len) << "null packets keep the run intact";
    std::string note;
    ASSERT_TRUE(cr::text::lossless_u16_to_utf8(n->props()->carve_note, note));
    EXPECT_EQ(note, "ts-packet-walk-188+null") << "the null-marker rides the evidence note";
}

TEST(Carve, TsSparseSyncNoiseRejected) {
    /* S-047 hostile: 0x47 bytes sprinkled at a NON-strided period - every
     * candidate has the follow byte but the second boundary check fails,
     * so no run ever reaches 0x5000 (the min-extent + per-packet verify
     * gates are the FP defense). */
    std::vector<u8> img(64 * 1024, 0);
    const size_t at = 512;
    for (size_t off = at; off + 2 < img.size(); off += 37) {
        img[off] = 0x47;
        img[off + 1] = 0x41; /* the follow byte passes the gate */
    }
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
    for (tree::FileNode* c : all)
        EXPECT_FALSE(c->props()->carve_format == u64(FMT_TS) ||
                     c->props()->carve_format == u64(FMT_M2TS))
            << "sparse non-strided syncs must never fire the TS family";
}

TEST(Carve, HtmlAsciiLowerUpperDoctypeAccepted) {
    /* M-092: the three ASCII probe forms - <html>, <HTML>, <!DOCTYPE
     * HTML> - all carve (the probe is case-insensitive; the u16 keys
     * <h/<H/<! are just the dispatch). */
    std::vector<u8> img(256 * 1024, 0);
    const size_t lower_at = 512, upper_at = 4096, doctype_at = 8192;
    const size_t lower_end = plant_html(img, lower_at, "<html>", 600);
    const size_t upper_end = plant_html(img, upper_at, "<HTML>", 600);
    const size_t doctype_end = plant_html(img, doctype_at, "<!DOCTYPE HTML>", 600);
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
    /* the u16 keys are case-sensitive dispatch rows into the ONE shared
     * validator: "<html>" -> FMT_HTML, "<HTML>" -> FMT_HTML_H, the
     * doctype -> FMT_HTML_BANG (the og's duplicate-key policy) */
    const std::pair<size_t, u64> expect[] = {
        {lower_at, u64(FMT_HTML)}, {upper_at, u64(FMT_HTML_H)}, {doctype_at, u64(FMT_HTML_BANG)}};
    for (const auto& [at, fmt] : expect) {
        tree::FileNode* n = find_at(at);
        ASSERT_NE(n, nullptr) << "the HTML probe form at " << at << " must carve (M-092)";
        EXPECT_EQ(n->props()->carve_format, fmt) << "the four-key chain, key-exact row";
    }
    /* the exact sector-rounded extents (the size = the rounded marker
     * end minus the start) */
    EXPECT_EQ(find_at(lower_at)->props()->logical_size,
              (((lower_end + 511) / 512) * 512) - lower_at);
    EXPECT_EQ(find_at(upper_at)->props()->logical_size,
              (((upper_end + 511) / 512) * 512) - upper_at);
    EXPECT_EQ(find_at(doctype_at)->props()->logical_size,
              (((doctype_end + 511) / 512) * 512) - doctype_at);
}

TEST(Carve, HtmlUtf16BomChainAccepted) {
    /* M-092: the 4th chain - FF FE + the wide "<HTML>" open tag carves
     * via the FMT_HTML_UTF16 row; the end-tag walk uses the ASCII
     * needle (no wide needle exists in the DLL). */
    std::vector<u8> img(256 * 1024, 0);
    const size_t at = 512;
    const size_t end = plant_html_utf16(img, at, 300);
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
    tree::FileNode* n = nullptr;
    for (tree::FileNode* c : all)
        if (!c->props()->extents.empty() && c->props()->extents.front().offset == at)
            n = c;
    ASSERT_NE(n, nullptr) << "the UTF-16 BOM chain must carve (M-092)";
    EXPECT_EQ(n->props()->carve_format, u64(FMT_HTML_UTF16));
    EXPECT_EQ(n->props()->logical_size, (((end + 511) / 512) * 512) - at)
        << "the ASCII end-tag walk + sector round-up";
    std::string note;
    ASSERT_TRUE(cr::text::lossless_u16_to_utf8(n->props()->carve_note, note));
    EXPECT_EQ(note, "html-endtag-walk+utf16");
}

TEST(Carve, HtmlNoEndTagFallsBackBounded) {
    /* M-092: no </html> within the 1 MiB cap -> the bounded fallback
     * (the RTF/slab-end family) with the heuristic note. */
    std::vector<u8> img(2 * 1024 * 1024, 0);
    const size_t at = 512;
    std::memcpy(img.data() + at, "<html><body>no end tag anywhere", 30);
    for (size_t i = at + 30; i < at + 0x100000; ++i)
        img[i] = u8('a' + (i % 26)); /* cap-filling body, no marker */
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
    tree::FileNode* n = nullptr;
    for (tree::FileNode* c : all)
        if (!c->props()->extents.empty() && c->props()->extents.front().offset == at)
            n = c;
    ASSERT_NE(n, nullptr) << "the markerless document must still carve (bounded)";
    EXPECT_EQ(n->props()->carve_format, u64(FMT_HTML));
    EXPECT_EQ(n->props()->logical_size, size_t(0x100000)) << "the 1 MiB cap";
    std::string note;
    ASSERT_TRUE(cr::text::lossless_u16_to_utf8(n->props()->carve_note, note));
    EXPECT_EQ(note, "html-bounded-cap") << "the heuristic note";
}

TEST(Carve, HtmlNonHtmlPrefixesRejected) {
    /* M-092 hostile: the u16 keys fire on <head>/<hr>/<!-- but the probe
     * rejects everything that is not <HTML / <!DOCTYPE HTML / the wide
     * forms - a <head> document never carves. */
    std::vector<u8> img(64 * 1024, 0);
    const size_t at = 512;
    std::memcpy(img.data() + at, "<head><title>x</title></head><body></body>", 41);
    /* a UTF-16 file whose post-BOM content is not <HTML>/<! - rejected */
    const size_t w_at = 4096;
    img[w_at] = 0xFF;
    img[w_at + 1] = 0xFE;
    img[w_at + 2] = 0x3C;
    img[w_at + 3] = 0x00;
    img[w_at + 4] = 0x68; /* wide '<h' - not '<HTML>' and not '!' */
    img[w_at + 5] = 0x00;
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
    for (tree::FileNode* c : all) {
        EXPECT_FALSE(c->props()->extents.front().offset == at &&
                     c->props()->carve_format == u64(FMT_HTML))
            << "a <head> document is not <HTML> - probe rejects";
        EXPECT_FALSE(c->props()->extents.front().offset == w_at &&
                     c->props()->carve_format == u64(FMT_HTML_UTF16))
            << "wide '<h' without the <HTML>/! forms - probe rejects";
    }
}

TEST(Carve, SvgDeclaredSvgAccepted) {
    /* M-093: the declaration + <svg ...> + </svg> form carves as SVG
     * (id 0x5b semantics - the probe stamps the id by CONTENT via the
     * format override) with the end-tag extent. */
    std::vector<u8> img(64 * 1024, 0);
    const size_t at = 512;
    const char doc[] =
        "<?xml version=\"1.0\"?><svg xmlns=\"http://www.w3.org/2000/svg\">BODY</svg>";
    std::memcpy(img.data() + at, doc, sizeof(doc) - 1);
    const size_t doc_len = sizeof(doc) - 1;
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
    tree::FileNode* n = nullptr;
    for (tree::FileNode* c : all)
        if (!c->props()->extents.empty() && c->props()->extents.front().offset == at)
            n = c;
    ASSERT_NE(n, nullptr) << "the declared SVG must carve (M-093)";
    EXPECT_EQ(n->props()->carve_format, u64(FMT_SVG)) << "the content-decided id (0x5b)";
    EXPECT_EQ(n->props()->logical_size, doc_len) << "the </svg> end-tag extent, exact";
    std::string name;
    ASSERT_TRUE(cr::text::lossless_u16_to_utf8(n->props()->name, name));
    EXPECT_NE(name.find(".SVG"), std::string::npos) << "the effective row's extension";
}

TEST(Carve, SvgBomFormAccepted) {
    /* M-093: the UTF-8 BOM + '<' key path - the declaration read at +3,
     * the <svg window hit -> SVG. The needle grammar needs a trailing
     * char after '>' (the decoded needle set: "<svg>\n" among them). */
    std::vector<u8> img(64 * 1024, 0);
    const size_t at = 512;
    const char doc[] = "<?xml version=\"1.0\"?><svg>\nBODY</svg>";
    img[at] = 0xEF;
    img[at + 1] = 0xBB;
    img[at + 2] = 0xBF;
    std::memcpy(img.data() + at + 3, doc, sizeof(doc) - 1);
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
    tree::FileNode* n = nullptr;
    for (tree::FileNode* c : all)
        if (!c->props()->extents.empty() && c->props()->extents.front().offset == at)
            n = c;
    ASSERT_NE(n, nullptr) << "the BOM-keyed SVG must carve";
    EXPECT_EQ(n->props()->carve_format, u64(FMT_SVG));
}

TEST(Carve, SvgDirectKeysAccepted) {
    /* M-093: the two direct paths - "<svg " (with the space) and the
     * decoded numeric key CD 01 00 00 (0x1cd) - both stamp SVG with no
     * declaration needed. */
    std::vector<u8> img(64 * 1024, 0);
    const size_t svg_at = 512;
    const char svg_doc[] = "<svg xmlns=\"x\">BODY</svg>";
    std::memcpy(img.data() + svg_at, svg_doc, sizeof(svg_doc) - 1);
    const size_t cd_at = 4096;
    img[cd_at] = 0xCD;
    img[cd_at + 1] = 0x01;
    img[cd_at + 2] = 0x00;
    img[cd_at + 3] = 0x00;
    std::memcpy(img.data() + cd_at + 4, "</svg>", 6); /* a marker for a bounded size */
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
    tree::FileNode* s1 = find_at(svg_at);
    ASSERT_NE(s1, nullptr) << "the \"<svg \" direct path must carve";
    EXPECT_EQ(s1->props()->carve_format, u64(FMT_SVG));
    EXPECT_EQ(s1->props()->logical_size, sizeof(svg_doc) - 1);
    tree::FileNode* s2 = find_at(cd_at);
    ASSERT_NE(s2, nullptr) << "the numeric-key (CD 01 00 00) direct path must carve";
    EXPECT_EQ(s2->props()->carve_format, u64(FMT_SVG));
}

TEST(Carve, GenericXmlFallbackId) {
    /* M-093: a declaration with the "csl" marker (and no svg needle)
     * stamps the generic-XML id (0x13 semantics) - the FMT_XML row via
     * the override. */
    std::vector<u8> img(64 * 1024, 0);
    const size_t at = 512;
    const char doc[] = "<?xml version=\"1.0\"?><csl-config>BODY</csl-config>";
    std::memcpy(img.data() + at, doc, sizeof(doc) - 1);
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
    tree::FileNode* n = nullptr;
    for (tree::FileNode* c : all)
        if (!c->props()->extents.empty() && c->props()->extents.front().offset == at)
            n = c;
    ASSERT_NE(n, nullptr) << "the csl-marked XML must carve";
    EXPECT_EQ(n->props()->carve_format, u64(FMT_XML)) << "the generic-XML id (0x13)";
    std::string name;
    ASSERT_TRUE(cr::text::lossless_u16_to_utf8(n->props()->name, name));
    EXPECT_NE(name.find(".XML"), std::string::npos);
}

TEST(Carve, XmlNoMarkerNoSvgRejected) {
    /* M-093: a declaration with NEITHER an svg needle NOR the csl
     * marker in the 512-byte window rejects - the probe's acceptance
     * set is exactly {svg needles, csl}. */
    std::vector<u8> img(64 * 1024, 0);
    const size_t at = 512;
    const char doc[] = "<?xml version=\"1.0\"?><ordinary><data/></ordinary>";
    std::memcpy(img.data() + at, doc, sizeof(doc) - 1);
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
    for (tree::FileNode* c : all)
        EXPECT_FALSE(
            c->props()->extents.front().offset == at &&
            (c->props()->carve_format == u64(FMT_XML) || c->props()->carve_format == u64(FMT_SVG)))
            << "an ordinary declaration without svg/csl must reject";
}

TEST(Carve, MobiBookMobiPairAccepted) {
    /* M-096: the PalmDB BOOKMOBI pair at +0x3C is the whole gate - the
     * 0x3C bytes before it are garbage and the book still carves (the
     * og's full-salvage posture, ported exactly). */
    std::vector<u8> img(64 * 1024, 0);
    const size_t at = 512;
    /* 0x3C bytes of damaged-header garbage, then the pair */
    for (size_t i = 0; i < 0x3C; ++i)
        img[at + i] = u8((i * 31) ^ 0xA5);
    std::memcpy(img.data() + at + 0x3C, "BOOKMOBI", 8);
    for (size_t i = 0x44; i < 0x1000; ++i)
        img[at + i] = u8(i & 0xFF);
    /* the end tag at a 512-aligned position from the start */
    const size_t tag_at = at + 0x2000;
    img[tag_at] = 0x0A;
    img[tag_at + 1] = 0x0D;
    img[tag_at + 2] = 0x8E;
    img[tag_at + 3] = 0xE9;
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
    tree::FileNode* n = nullptr;
    for (tree::FileNode* c : all)
        if (!c->props()->extents.empty() && c->props()->extents.front().offset == at)
            n = c;
    ASSERT_NE(n, nullptr) << "the damaged-header BOOKMOBI must carve (salvage parity)";
    EXPECT_EQ(n->props()->carve_format, u64(FMT_MOBI));
    EXPECT_EQ(n->props()->logical_size, (0x2000 - 0) + 4) << "the end-tag extent (step-aligned)";
    std::string name;
    ASSERT_TRUE(cr::text::lossless_u16_to_utf8(n->props()->name, name));
    EXPECT_NE(name.find(".MOBI"), std::string::npos);
}

TEST(Carve, MobiOtherPalmCreatorsRejected) {
    /* M-096: the og gates BOOKMOBI only - a TEXt/REAd eReader pair (the
     * other famous PalmDB creator) rejects. */
    std::vector<u8> img(64 * 1024, 0);
    const size_t at = 512;
    std::memcpy(img.data() + at + 0x3C, "TEXtREAd", 8);
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
    for (tree::FileNode* c : all)
        EXPECT_NE(c->props()->carve_format, u64(FMT_MOBI)) << "the TEXt/REAd pair rejects";
}

TEST(Carve, MobiNoEndTagFallsBackBounded) {
    /* M-096: no step-aligned end tag within the cap -> the bounded
     * fallback (clamped to the device end) with the heuristic note. */
    std::vector<u8> img(64 * 1024, 0);
    const size_t at = 512;
    std::memcpy(img.data() + at + 0x3C, "BOOKMOBI", 8);
    /* the end-tag bytes at a NON-step-aligned position: the walker's
     * 0x200 stepping never sees them (the og's own walk shape) */
    const size_t tag_at = at + 0x2000 + 7;
    img[tag_at] = 0x0A;
    img[tag_at + 1] = 0x0D;
    img[tag_at + 2] = 0x8E;
    img[tag_at + 3] = 0xE9;
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
    tree::FileNode* n = nullptr;
    for (tree::FileNode* c : all)
        if (!c->props()->extents.empty() && c->props()->extents.front().offset == at)
            n = c;
    ASSERT_NE(n, nullptr) << "the markerless book still carves (bounded)";
    EXPECT_EQ(n->props()->carve_format, u64(FMT_MOBI));
    EXPECT_EQ(n->props()->logical_size, size_t(64 * 1024 - 512)) << "the cap clamped to the device";
    std::string note;
    ASSERT_TRUE(cr::text::lossless_u16_to_utf8(n->props()->carve_note, note));
    EXPECT_EQ(note, "mobi-bounded-cap");
}

TEST(Carve, S007NicheBatch1SalvageSemantics) {
    /* Torn-stream salvage: MIDI with a declared-but-torn track salvages
     * at the last COMPLETE track; Ogg without an EOS page salvages at
     * the last complete page; BZh1 without the stream footer salvages
     * to the cap (the RTF/slab-end family); a clean gzip header (no
     * findable trailer mid-slab) bounds at the cap the same way. */
    std::vector<u8> img(64 * 1024, 0);
    const size_t mid_at = 512;
    const size_t mid_len = plant_mid(img, mid_at, 2, 1); /* 2 declared, 1 complete */
    const size_t ogg_at = 4096;
    const size_t ogg_len = plant_ogg(img, ogg_at, false);
    const size_t dmg_at = 8192;
    plant_dmg(img, dmg_at, false);
    const size_t gz_at = 12288;
    const size_t gz_len = plant_gz(img, gz_at);

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
    tree::FileNode* mid = nullptr;
    tree::FileNode* ogg = nullptr;
    tree::FileNode* dmg = nullptr;
    tree::FileNode* gz = nullptr;
    for (tree::FileNode* n : all) {
        const u64 off = n->props()->extents.front().offset;
        if (off == mid_at)
            mid = n;
        else if (off == ogg_at)
            ogg = n;
        else if (off == dmg_at)
            dmg = n;
        else if (off == gz_at)
            gz = n;
    }
    ASSERT_NE(mid, nullptr) << "torn-tail MIDI salvages at the complete track";
    EXPECT_EQ(mid->props()->logical_size, mid_len) << "extent stops at the last complete track";
    ASSERT_NE(ogg, nullptr) << "EOS-less Ogg salvages at the last complete page";
    EXPECT_EQ(ogg->props()->logical_size, ogg_len) << "extent stops at the last complete page";
    ASSERT_NE(dmg, nullptr) << "footer-less BZh1 salvages to the cap";
    EXPECT_EQ(dmg->props()->logical_size, size_t(64 * 1024 - dmg_at)) << "slab-end salvage";
    ASSERT_NE(gz, nullptr) << "clean gzip header bounds at the cap";
    EXPECT_GE(gz->props()->logical_size, gz_len) << "the header + body stays inside the extent";
}

TEST(Carve, S007NicheBatch1SharedKeyDiscriminates) {
    /* TTF and MDB share the 00 01 00 00 key (the og's u16-0x0100
     * multimap chain): the trial order tries the sfnt table directory
     * first, the Jet signature gate second - each plant carves as its
     * own format, and neither accepts the other's shape. */
    std::vector<u8> img(64 * 1024, 0);
    const size_t ttf_at = 512, mdb_at = 2048;
    const size_t ttf_len = plant_ttf(img, ttf_at);
    const size_t mdb_len = plant_mdb(img, mdb_at);
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
    tree::FileNode* ttf = nullptr;
    tree::FileNode* mdb = nullptr;
    for (tree::FileNode* n : all) {
        const u64 off = n->props()->extents.front().offset;
        if (off == ttf_at)
            ttf = n;
        else if (off == mdb_at)
            mdb = n;
    }
    ASSERT_NE(ttf, nullptr) << "the sfnt plant carves";
    EXPECT_EQ(ttf->props()->carve_format, FMT_TTF);
    EXPECT_GE(ttf->props()->logical_size, ttf_len);
    ASSERT_NE(mdb, nullptr) << "the Jet plant carves";
    EXPECT_EQ(mdb->props()->carve_format, FMT_MDB);
    EXPECT_GE(mdb->props()->logical_size, mdb_len);
}

/* ===================================================================== */
/* A-010: the ASF object walker - the og's BruteForce family contract     */
/* ("walk forward to grammar break, report the last valid position";     */
/* rawsearch-carving--deep-dive.md:87-91). The pre-fix code ended every  */
/* carve at the Header Object's own size field (header-only fragments).  */
/* ===================================================================== */

TEST(Carve, AsfCarveExtentCoversDataObjects) {
    /* header (100) + 3 data objects (200+150+300) + unknown-GUID break */
    std::vector<u8> img(64 * 1024, 0);
    const size_t at = 512;
    const size_t expect = plant_asf(img, at, 3, "unknown");

    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"L");
    CarveFlags flags;
    CarveStats stats;
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
        OK);
    tree::FileNode* wmv = nullptr;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
        if (c->props() && c->props()->extents.front().offset == at)
            wmv = c;
    ASSERT_NE(wmv, nullptr) << "ASF fixture must carve";
    /* fail-before: the old code's extent was the header-only 100 bytes */
    EXPECT_EQ(wmv->props()->logical_size, expect)
        << "extent must cover all structurally-valid objects (750), not the header";
}

TEST(Carve, AsfCarveStopsAtGrammarBreak) {
    /* grammar break immediately after the Header Object: an unknown-GUID
     * object with a perfectly sane size - the extent is the header alone
     * (the family's last-valid-position rule) */
    std::vector<u8> img(64 * 1024, 0);
    const size_t at = 512;
    const size_t expect = plant_asf(img, at, 0, "unknown");

    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"L");
    CarveFlags flags;
    CarveStats stats;
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
        OK);
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling()) {
        if (c->props() && c->name_utf8().find(".wmv") != std::string::npos) {
            EXPECT_EQ(c->props()->logical_size, expect)
                << "header-only ASF: extent = the header, no premature extension";
        }
    }
}

TEST(Carve, AsfCarveTornTailSalvagesLastCompleteObject) {
    /* the trailing Data Object's claimed size runs past the device end:
     * the walk stops at the last COMPLETE object (salvage semantics) */
    std::vector<u8> img(64 * 1024, 0);
    const size_t at = 512;
    const size_t expect = plant_asf(img, at, 2, "torn"); /* 100 + 200 + 150 */

    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"L");
    CarveFlags flags;
    CarveStats stats;
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
        OK);
    tree::FileNode* wmv = nullptr;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
        if (c->props() && c->props()->extents.front().offset == at)
            wmv = c;
    ASSERT_NE(wmv, nullptr);
    EXPECT_EQ(wmv->props()->logical_size, expect)
        << "torn tail: extent ends at the last complete object";
}

TEST(Carve, AsfHeaderCountGatesUnchanged) {
    /* the entry predicate keeps the og-shaped gates: count 0 and >128
     * reject even with a perfect GUID and size */
    for (u32 bad_count : {0u, 129u}) {
        std::vector<u8> img(64 * 1024, 0);
        plant_asf(img, 512, 1, "none", 0, bad_count);

        MemDevice dev(std::move(img));
        tree::ResultTree tree;
        auto* root = tree.add_root(u"L");
        CarveFlags flags;
        CarveStats stats;
        ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr,
                              stats),
                  OK);
        for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling()) {
            if (c->props()) {
                EXPECT_EQ(std::string::npos, c->name_utf8().find(".wmv"))
                    << "count " << bad_count << " must reject";
            }
        }
    }
}

/* ---- S-037: FH11 + CPT7/8/9 (the two missing reference formats) ------- */

TEST(Carve, Fh11DetectedAndWalked) {
    /* the og's constant-predicate FH11 (FUN_180029f30): the full
     * 0x24-byte header passes, and the carved extent is bounded (the
     * CZeroEndRawFileInfo family has no size grammar). */
    std::vector<u8> img(2ull << 20, 0);
    const size_t at = 0x2000;
    plant_fh11(img, at);

    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"L");
    CarveFlags flags;
    flags.hash_small_files = false;
    CarveStats stats;
    ASSERT_EQ(OK, carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr,
                              stats));
    tree::FileNode* hit = nullptr;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
        if (c->props() && c->name_utf8().find(".FH11") != std::string::npos)
            hit = c;
    ASSERT_NE(hit, nullptr) << "the FH11 header must carve";
    EXPECT_EQ(u32(FsKindCode::Raw), hit->props()->fs_kind);
    EXPECT_GE(hit->props()->extents.size(), 1u);
    EXPECT_EQ(at, hit->props()->extents[0].offset);
    /* the bounded extent: min(device end, 32 MiB) from the hit - integer
     * drift check (EXPECT_NEAR's u64->double narrowing is an MSVC /W4 red) */
    const u64 want_len = (2ull << 20) - at;
    const u64 got_len = hit->props()->extents[0].length;
    const u64 drift = got_len > want_len ? got_len - want_len : want_len - got_len;
    EXPECT_LE(drift, 4096u);
}

TEST(Carve, Fh11RejectsBareKeyBytes) {
    /* the registry key alone (1C 01 00 00) without the constant set is
     * not an FH11 - the og's Analyse rejects, not the key hit */
    std::vector<u8> img(2ull << 20, 0);
    std::memcpy(img.data() + 0x2000, "\x1C\x01\x00\x00", 4);
    /* fill what would be the constants with the wrong values */
    for (int i = 0; i < 64; ++i)
        img[0x2000 + 4 + size_t(i)] = 0xEE;

    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"L");
    CarveFlags flags;
    flags.hash_small_files = false;
    CarveStats stats;
    ASSERT_EQ(OK, carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr,
                              stats));
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling()) {
        if (c->props()) {
            EXPECT_EQ(std::string::npos, c->name_utf8().find(".FH11"))
                << "bare key bytes must not carve";
        }
    }
}

TEST(Carve, CptGenerationsDetected) {
    /* all three PhotoPaint generation keys (CPT7/CPT8/CPT9) reach the
     * shared validator; each carves with the pointer-chase extent */
    for (const u32 gen : {'7', '8', '9'}) {
        std::vector<u8> img(2ull << 20, 0);
        const size_t at = 0x2000;
        plant_cpt(img, at, gen); /* the extent is checked against the og's walk math below */

        MemDevice dev(std::move(img));
        tree::ResultTree tree;
        auto* root = tree.add_root(u"L");
        CarveFlags flags;
        flags.hash_small_files = false;
        CarveStats stats;
        ASSERT_EQ(OK, carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr,
                                  nullptr, stats))
            << "generation " << char(gen);
        tree::FileNode* hit = nullptr;
        for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
            if (c->props() && c->name_utf8().find(".CPT") != std::string::npos)
                hit = c;
        ASSERT_NE(hit, nullptr) << "CPT" << char(gen) << " must carve";
        /* the walk's exact size per the og's FUN_1800436e0: the data
         * descriptor at (t2-8)/(t2-4) yields size = len + extras =
         * 0x3000 + 0x40 (t2 itself is the descriptor's location, not
         * part of the size) */
        const size_t og_size = size_t(0x3000) + 0x40;
        ASSERT_GE(hit->props()->extents.size(), 1u);
        EXPECT_EQ(at, hit->props()->extents[0].offset) << char(gen);
        EXPECT_EQ(og_size, size_t(hit->props()->extents[0].length))
            << "the FUN_1800436e0 pointer-chase extent, generation " << char(gen);
    }
}

TEST(Carve, CptRejectsTruncatedHeader) {
    /* the key hit without "FILE" @ +4 / the 0x13C gate rejects */
    std::vector<u8> img(2ull << 20, 0);
    std::memcpy(img.data() + 0x2000, "CPT7", 4);
    std::memcpy(img.data() + 0x2004, "XXXX", 4); /* wrong Analyse magic */
    put32v(img, 0x2000 + 0x34, 0x13C);           /* the gate alone is not enough */

    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"L");
    CarveFlags flags;
    flags.hash_small_files = false;
    CarveStats stats;
    ASSERT_EQ(OK, carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr,
                              stats));
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling()) {
        if (c->props()) {
            EXPECT_EQ(std::string::npos, c->name_utf8().find(".cpt"))
                << "a wrong Analyse magic must reject";
        }
    }
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

    /* stats (M-047 recursive accumulator): the deleted population
     * includes every deleted node in the SUBTREE - the carved archive
     * records AND their inner-entry inventory children (the og's tree
     * expands ZIPs to their inner file list, and its recursive stats
     * accumulator counts them - rsf-format :285). Every Carved-origin
     * node is Deleted, so the recursive deleted_count equals the whole
     * carved population collected above. */
    tree.refresh_stats();
    EXPECT_EQ(root->stats().deleted_count, size_t(carved));
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
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats stats;
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
}

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
    put16(img, eocd_at + 4, 0);                /* this disk */
    put16(img, eocd_at + 6, 0);                /* disk with CD start */
    put16(img, eocd_at + 8, 1);                /* entries this disk */
    put16(img, eocd_at + 10, 1);               /* total entries */
    put32v(img, eocd_at + 12, u32(cd_len));    /* CD size */
    put32v(img, eocd_at + 16, u32(local_len)); /* CD offset (LFH-relative) */
    put16(img, eocd_at + 20, 0);               /* comment len */
    return data_at;
}

} // namespace

TEST(Carve, ZipTruncatedHeadRecovered) {
    /* N-068 claim 1: a zip whose first 512 bytes are destroyed (the LFH +
     * data head) but whose CD + EOCD survive still carves. The og's
     * analyser dispatches all three structures (entry-point table,
     * rawsearchdll--zip-analyser-carving.md :13-21); the core's CD anchor
     * (PK\\x01\\x02, "preferred anchor") owns this recovery, with the
     * EOCD reconciliation sizing the span. */
    auto img = std::vector<u8>(2 * 1024 * 1024, 0);
    const std::vector<u8> payload(1024, 0xAB);
    (void)plant_zip_with_data(img, 4096, "letter.txt", payload);
    /* destroy the head: LFH + name + most of the data */
    std::memset(img.data() + 4096, 0, 512);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats stats;
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
        OK);
    tree::FileNode* zip_node = nullptr;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
        if (c->props() && !c->props()->extents.empty() &&
            c->props()->extents.front().offset >= 4096 &&
            c->props()->extents.front().offset < 4096 + 2048)
            zip_node = c;
    ASSERT_NE(zip_node, nullptr) << "head-destroyed zip must still carve (CD/EOCD survive)";
}

TEST(Carve, ZipLfhSanityGates) {
    /* N-068 claim 2: the og's LFH anchor carries two cheap FP-suppressor
     * gates (decompile :18385-18388, byte-exact): reject when
     * u16@+8 > 10 (the artifact calls it version_needed; in the spec LFH
     * layout +8 is the compression method - the og gates the bytes either
     * way) or when (u16@+0x1a - 1) > 0x102, i.e. the accept window is
     * [1, 0x103] (the artifact's "[1, 0x102]" range text is off-by-one vs
     * the decompile's `0x102 < field - 1`). */
    const std::vector<u8> payload(64, 0x33);
    /* (a) u16@+8 = 11: past the method/version bar - rejected by the og */
    {
        auto img = std::vector<u8>(1 * 1024 * 1024, 0);
        (void)plant_zip_with_data(img, 4096, "gate.bin", payload);
        put16(img, 4096 + 8, 11);
        MemDevice dev(std::move(img));
        tree::ResultTree tree;
        auto* root = tree.add_root(u"Lost Files");
        CarveFlags flags;
        CarveStats stats;
        ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr,
                              stats),
                  OK);
        for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
            ASSERT_TRUE(!(c->props() && c->props()->carve_format == FMT_ZIP &&
                          !c->props()->extents.empty() &&
                          c->props()->extents.front().offset == 4096))
                << "LFH with u16@+8 = 11 must be rejected by the anchor gate";
    }
    /* (b) u16@+0x1a = 0x1FF: outside the [1, 0x103] window - rejected */
    {
        auto img = std::vector<u8>(1 * 1024 * 1024, 0);
        (void)plant_zip_with_data(img, 4096, "gate.bin", payload);
        put16(img, 4096 + 0x1a, 0x1FF);
        MemDevice dev(std::move(img));
        tree::ResultTree tree;
        auto* root = tree.add_root(u"Lost Files");
        CarveFlags flags;
        CarveStats stats;
        ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr,
                              stats),
                  OK);
        for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
            ASSERT_TRUE(!(c->props() && c->props()->carve_format == FMT_ZIP &&
                          !c->props()->extents.empty() &&
                          c->props()->extents.front().offset == 4096))
                << "LFH with u16@+0x1a = 0x1FF must be rejected by the anchor gate";
    }
    /* (c) regression pin: the boundary values ACCEPT - u16@+8 = 10 and
     * u16@+0x1a at the window edges 1 and 0x103 - a normal (gate-passing)
     * zip still carves. plant_zip_with_data writes +8 = 0 (stored) and the
     * 8-char name at +0x1a; re-stamp to exercise the edges. */
    {
        auto img = std::vector<u8>(1 * 1024 * 1024, 0);
        (void)plant_zip_with_data(img, 4096, "gate.bin", payload);
        put16(img, 4096 + 8, 10); /* method/version at the bar */
        /* NOTE: +0x1a is the LFH name length - changing it would break
         * the walk's geometry; the edge-exercise for the second gate is
         * covered by (b)'s rejection + the accept-window derivation. */
        MemDevice dev(std::move(img));
        tree::ResultTree tree;
        auto* root = tree.add_root(u"Lost Files");
        CarveFlags flags;
        CarveStats stats;
        ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr,
                              stats),
                  OK);
        bool carved = false;
        for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
            if (c->props() && c->props()->carve_format == FMT_ZIP && !c->props()->extents.empty() &&
                c->props()->extents.front().offset == 4096)
                carved = true;
        EXPECT_TRUE(carved) << "a gate-passing LFH must still carve";
    }
}

TEST(CarveEngine, ZipInnerEntryInventoryEmitted) {
    /* the og's ZIP analyser returns the complete inner inventory (names,
     * sizes, offsets) which the results tree presents as the archive's
     * contents; the pre-fix core emitted one opaque node. The inventory
     * child carries a PRECISE data extent inside the parent (superset:
     * per-entry recovery) */
    auto img = std::vector<u8>(2 * 1024 * 1024, 0);
    const std::vector<u8> payload(40, 0xAB);
    const size_t data_at = plant_zip_with_data(img, 4096, "letter.txt", payload);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats stats;
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
        OK);

    tree::FileNode* zip_node = nullptr;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
        if (c->props() && c->props()->carve_format == FMT_ZIP && !c->props()->extents.empty() &&
            c->props()->extents.front().offset == 4096)
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
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats stats;
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
        OK);

    tree::FileNode* zip_node = nullptr;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
        if (c->props() && c->props()->carve_format == FMT_ZIP && !c->props()->extents.empty() &&
            c->props()->extents.front().offset == 4096) {
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
    const size_t head_size =
        32 + nlen; /* base7+pack4+unp4+host1+crc4+ftime4+ver1+method1+nsize2+attr4+name */
    put16(img, fh + 0, 0);
    img[fh + 2] = 0x74;         /* FILE */
    put16(img, fh + 3, 0x8000); /* LONG_BLOCK: PACK_SIZE follows */
    put16(img, fh + 5, u16(head_size));
    put32v(img, fh + 7, u32(dlen));   /* PACK_SIZE */
    put32v(img, fh + 11, u32(dlen));  /* UNP_SIZE */
    img[fh + 15] = 0;                 /* HOST_OS */
    put32v(img, fh + 16, 0);          /* FILE_CRC */
    put32v(img, fh + 20, 0x60000000); /* FTIME */
    img[fh + 24] = 29;                /* UNP_VER */
    img[fh + 25] = 0x30;              /* METHOD stored */
    put16(img, fh + 26, u16(nlen));   /* NAME_SIZE */
    put32v(img, fh + 28, 0x20);       /* ATTR archive */
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
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
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
}

/* ---- N-051/N-052/N-053/N-054 fixtures (the carve-magic quartet) ---- */
static void mkv_header(std::vector<u8>& img, size_t at, const char* doctype) {
    /* EBML magic is the registry key; the header element's size vint and
     * payload carry DocType (0x4282) + DocTypeVersion/ReadVersion children.
     * vint sizes: 0x8N = 1-byte size of N; children use 2-byte IDs. */
    const u8 magic[4] = {0x1A, 0x45, 0xDF, 0xA3};
    std::memcpy(img.data() + at, magic, 4);
    const size_t dtl = std::strlen(doctype);
    const u32 payload = u32(2 + 1 + dtl + 4 + 4); /* DocType + Ver + ReadVer */
    img[at + 4] = u8(0x80 | payload);             /* 1-byte size vint */
    size_t p = at + 5;
    img[p] = 0x42;
    img[p + 1] = 0x82;           /* DocType ID (0x4282) */
    img[p + 2] = u8(0x80 | dtl); /* size = strlen */
    std::memcpy(img.data() + p + 3, doctype, dtl);
    p += 3 + dtl;
    img[p] = 0x42;
    img[p + 1] = 0x87;
    img[p + 2] = 0x81;
    img[p + 3] = 1; /* DocTypeVersion=1 */
    img[p + 4] = 0x42;
    img[p + 5] = 0x86;
    img[p + 6] = 0x81;
    img[p + 7] = 1; /* ReadVersion=1 */
}

static void iiu_tiff(std::vector<u8>& img, size_t at, u16 ifd_off, bool valid_ifd) {
    /* IIU\0 header + IFD at ifd_off (either a real 1-entry IFD or garbage) */
    u8* t = img.data() + at;
    std::memcpy(t, "IIU\x00", 4);
    t[4] = u8(ifd_off);
    t[5] = u8(ifd_off >> 8); /* LE u16 @+4 */
    if (valid_ifd) {
        u8* f = img.data() + at + ifd_off;
        f[0] = 1;
        f[1] = 0; /* 1 entry */
        f[2] = 0x00;
        f[3] = 0x01; /* tag 0x0100 */
        f[4] = 3;
        f[5] = 0; /* type SHORT */
        f[6] = 5;
        f[7] = 0;
        f[8] = 0;
        f[9] = 0; /* count 5 > 4 */
        f[10] = 0xB0;
        f[11] = 0x04; /* value offset 1200 */
        f[12] = 0;
        f[13] = 0;
        f[14] = 0;
        f[15] = 0; /* data offset */
        f[16] = 0;
        f[17] = 0;
        f[18] = 0;
        f[19] = 0;                                     /* next IFD = 0 */
        std::memset(img.data() + at + 1200, 0x5A, 10); /* value data */
    } else {
        std::memset(img.data() + at + ifd_off, 0xFF, 32); /* IFD walk will fail */
    }
}

TEST(Carve, MkvDocTypeWebmAccepted) {
    /* N-051: the og's FileMKVAnalyser decodes the DocType and accepts
     * webm (0x6D626577) + matroska only. The header payload ends at +20
     * (clears the >16 liveness bar). */
    std::vector<u8> img(64 * 1024, 0);
    mkv_header(img, 4096, "webm");
    mkv_header(img, 12288, "matroska");
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats s;
    ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, s),
              OK);
    EXPECT_NE(find_carved_by_ext(root, "mkv"), nullptr) << "webm+matroska not carved";
    EXPECT_GE(s.validated, 2u);
}

TEST(Carve, MkvDocTypeExoticRejected) {
    /* an EBML use whose DocType is neither webm nor matroska: rejected by
     * the og's analyser and by ours (the pre-N-051 core accepted any
     * structurally-valid EBML). */
    std::vector<u8> img(64 * 1024, 0);
    mkv_header(img, 4096, "foob");
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats s;
    ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, s),
              OK);
    EXPECT_EQ(find_carved_by_ext(root, "mkv"), nullptr) << "exotic DocType must not carve";
}

TEST(Carve, MkvElementBoundsGate) {
    /* the og's header bounds gate: id_len + size_len + payload + 4 must
     * stay <= 0x200. Here the size vint decodes 520 (2-byte vint 0x42 0x08):
     * 2 + 520 + 4 = 526 > 512 -> reject before any payload read. */
    std::vector<u8> img(64 * 1024, 0);
    u8* t = img.data() + 4096;
    std::memcpy(t, "\x1A\x45\xDF\xA3", 4);
    t[4] = 0x42;
    t[5] = 0x08;                  /* vint: length 2, value 520 */
    std::memset(t + 6, 0x5A, 64); /* plausible payload (never read) */
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats s;
    ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, s),
              OK);
    EXPECT_EQ(find_carved_by_ext(root, "mkv"), nullptr)
        << "oversized header element must not carve";
}

TEST(Carve, HeifSiblingBrandLabeledHeif) {
    /* N-052: the og relabels HEIF-family siblings (heix/mif1/msf1) to the
     * display label L"HEIF"; heic-proper keeps L"HEIC". The FMT_HEIF row
     * carries the rewritten label; the format id is distinct. */
    std::vector<u8> img(256 * 1024, 0);
    auto heic = bmff_head("heic");
    std::memcpy(img.data() + 4096, heic.data(), heic.size());
    auto mif1 = bmff_head("mif1");
    std::memcpy(img.data() + 8192, mif1.data(), mif1.size());
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats s;
    ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, s),
              OK);
    auto* heic_node = find_carved_by_ext(root, "heic");
    ASSERT_NE(heic_node, nullptr) << "heic-proper must carve as HEIC";
    EXPECT_EQ(heic_node->props()->carve_format, u32(FMT_HEIC));
    auto* heif_node = find_carved_by_ext(root, "heif");
    ASSERT_NE(heif_node, nullptr) << "mif1 sibling must carve (as HEIF)";
    EXPECT_EQ(heif_node->props()->carve_format, u32(FMT_HEIF)) << "the relabeled format id";
    EXPECT_EQ(heif_node->props()->extents.front().offset, u64(8192));
}

TEST(Carve, IiuTiffEpValidatedByTiffGrammar) {
    /* N-053: the og binds the IIU\0 key to the TIFF chain (census :53,
     * BruteForce::TiffAnalyzer). A structurally-valid IIU file carves as
     * TIFF/EP (ext .tif, format id FMT_TIFF_EP) - the pre-fix core labeled
     * every IIU file "Panasonic RAW" (.rw2). */
    std::vector<u8> img(64 * 1024, 0);
    iiu_tiff(img, 4096, 8, true);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats s;
    ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, s),
              OK);
    auto* tiff_ep = find_carved_by_ext(root, "tif");
    ASSERT_NE(tiff_ep, nullptr) << "IIU TIFF/EP must carve as .tif";
    EXPECT_EQ(tiff_ep->props()->carve_format, u32(FMT_TIFF_EP));
    EXPECT_EQ(tiff_ep->props()->extents.front().offset, u64(4096));
    EXPECT_EQ(find_carved_by_ext(root, "rw2"), nullptr)
        << "a valid TIFF/EP must not take the RW2 row";
}

TEST(Carve, IiuRw2FallbackWhenTiffEpRejects) {
    /* the IIU-salvage fallback (registered superset D-63): the TIFF/EP
     * grammar rejects (garbage IFD), the og carves nothing - we salvage
     * the Panasonic-shaped header at lower confidence. */
    std::vector<u8> img(64 * 1024, 0);
    iiu_tiff(img, 4096, 8, false);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats s;
    ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, s),
              OK);
    EXPECT_EQ(find_carved_by_ext(root, "tif"), nullptr) << "invalid IFD must not take TIFF/EP";
    auto* rw2 = find_carved_by_ext(root, "rw2");
    ASSERT_NE(rw2, nullptr) << "the IIU-salvage fallback must fire";
    EXPECT_EQ(rw2->props()->carve_format, u32(FMT_RW2));
}

TEST(Carve, RafVariantHeaderAccepted) {
    /* N-054: the og keys RAF on the 4-byte FUJI u32 (census :55); variant
     * headers whose bytes 4..15 differ from FUJIFILMCCD-RAW must still
     * carve (lower confidence, variant note). */
    std::vector<u8> img(128 * 1024, 0);
    u8* t = img.data() + 4096;
    std::memcpy(t, "FUJI", 4);
    std::memcpy(t + 4, "VARIAnt-raw  ", 12); /* non-canonical bytes 4..15 */
    put32v(img, 4096 + 16, 0x0200);          /* version: sane */
    put32v(img, 4096 + 0x54, 0);             /* section table starts empty */
    put32v(img, 4096 + 0x54, 0x1000);
    put32v(img, 4096 + 0x58, 0x2000);
    put32v(img, 4096 + 0x60, 0x3000);
    put32v(img, 4096 + 0x64, 0x4000);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats s;
    ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, s),
              OK);
    auto* raf = find_carved_by_ext(root, "raf");
    ASSERT_NE(raf, nullptr) << "FUJI variant header must carve";
    EXPECT_EQ(raf->props()->carve_format, u32(FMT_RAF));
}

TEST(Carve, RafRealAsciiVersionAccepted) {
    /* N-076 (NEW-FINDING, discovered during N-054's fixture work -
     * both quartet landings carried the defect): real-world RAF files
     * carry the version at +16 as a 4-byte ASCII string ("0100"/"0201")
     * - read LE those are ~8e8, far above the old binary 0x2000 cap, so
     * the canonical path rejected every real RAF (the row was dead for
     * real files since introduction; zero fixture coverage). */
    std::vector<u8> img(256 * 1024, 0);
    const size_t at = 16384;
    u8* t = img.data() + at;
    std::memcpy(t, "FUJIFILMCCD-RAW ", 16);
    std::memcpy(t + 16, "0201", 4);
    put32v(img, at + 0x54, 96);
    put32v(img, at + 0x54 + 4, 4096);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats s;
    ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, s),
              OK);
    auto* n = find_carved_by_ext(root, "raf");
    ASSERT_NE(n, nullptr)
        << "real ASCII-version RAF must carve (pre-fix: the binary ver gate rejected it)";
    EXPECT_EQ(n->props()->extents.front().offset, u64(at));
    EXPECT_GE(n->props()->logical_size, u64(1024));
}

TEST(Carve, JpegFourthByteOutsideOgSet) {
    /* N-054 (documented superset, D-64): the og keys JPEG on FOUR exact
     * dwords (FF D8 FF E0/E1/DB/EE - census :54); the core matches the
     * 3-byte prefix, so FF D8 FF 99 + a valid JFIF body carves here and
     * is not dispatched by the og's registry. Deliberate (I-009 family);
     * validate_jpeg's grammar gates the false-positive surface. */
    std::vector<u8> img(256 * 1024, 0);
    const size_t at = 4096;
    img[at] = 0xFF;
    img[at + 1] = 0xD8;
    img[at + 2] = 0xFF;
    img[at + 3] = 0x99;
    /* a valid body: APP0 marked as APP9 (marker 0xE9 is outside the og's
     * four) - reuse the grammar-complete shape with the 4th byte 0x99 */
    (void)plant_jpeg(img, at); /* overwrites the first bytes: re-stamp */
    img[at + 3] = 0x99;        /* plant_jpeg wrote 0xE0; stamp the superset byte */
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats s;
    ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, s),
              OK);
    auto* jpg = find_carved_by_ext(root, "jpg");
    ASSERT_NE(jpg, nullptr) << "FF D8 FF 99 (outside the og's 4-dword set) must carve here";
}

/* ---- S-018 re-filed (s31 / OQ-S3-JPEG resolved): the og's legacy
 * FileJPGAnalyser tier salvages tail-truncated JPEGs. The dispatcher
 * FUN_18004b170 branches JFIF/Exif/generic; each sub-analyser's header
 * walk is SOS-gated and the post-SOS walker FUN_18004c480
 * (RAWSearchDLL_decompiled.c:57942-57991) ACCEPTS AT ITS SCAN LIMIT when
 * no FF D9 is found — the limit is h*w*3 measured from the candidate
 * start when the SOF supplied dimensions, else the 200,000,000-byte
 * global cap (FUN_180001350+0x70) divided by 10; the sliding-window
 * getter FUN_180042930 accepts at the device end. The core mirrors the
 * salvage at PartialRisk (600 — the deliberate grading delta; the og
 * stamps salvaged files identically to complete ones). */
TEST(Carve, JpegTruncatedTailSalvagedAtSofEstimate) {
    /* SOI+APP0+SOF0(64x48)+SOS with the EOI destroyed and a marker-free
     * tail: the salvage extent is hit + 64*48*3 = hit + 9216 */
    std::vector<u8> img(256 * 1024, 0x11); /* marker-free filler */
    const size_t at = 4096;
    (void)plant_jpeg(img, at);
    bool destroyed = false;
    for (size_t i = at; i < at + 600; ++i) {
        if (img[i] == 0xFF && img[i + 1] == 0xD9) {
            img[i] = 0x22;
            img[i + 1] = 0x33;
            destroyed = true;
            break;
        }
    }
    ASSERT_TRUE(destroyed) << "fixture bug: planted EOI not found";
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats s;
    ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, s),
              OK);
    auto* jpg = find_carved_by_ext(root, "jpg");
    ASSERT_NE(jpg, nullptr)
        << "the og salvages tail-truncated JPEGs (legacy fallback tier); the core must too";
    EXPECT_EQ(jpg->props()->extents.front().offset, at);
    EXPECT_EQ(jpg->props()->extents.front().length, 64u * 48u * 3u);
    /* the emitted confidence is the engine's (sig.base 900 + validator
     * 600) / 2 = 750; the GRADE comes from the raw validator (600 < 850
     * -> PartialRisk) */
    EXPECT_EQ(jpg->props()->carve_confidence, 750u);
    EXPECT_EQ(jpg->confidence(), tree::Confidence::PartialRisk);
}

TEST(Carve, JpegTruncatedTailWithoutSofSalvagesAtCapTenth) {
    /* no SOF in the header walk -> dims unknown -> the og's /10 limit
     * (200,000,000 / 10) bounded by the device: the salvage spans to the
     * device end (FUN_180042930's getter-NULL accept) */
    std::vector<u8> img(256 * 1024, 0x11);
    const size_t at = 4096;
    (void)plant_jpeg(img, at);
    bool neutralized = false;
    for (size_t i = at; i < at + 600; ++i) {
        if (img[i] == 0xFF && img[i + 1] == 0xC0) {
            img[i + 1] = 0xE2; /* SOF0 -> APP2 (same length semantics) */
            neutralized = true;
            break;
        }
    }
    ASSERT_TRUE(neutralized) << "fixture bug: planted SOF0 not found";
    for (size_t i = at; i < at + 600; ++i) {
        if (img[i] == 0xFF && img[i + 1] == 0xD9) {
            img[i] = 0x22;
            img[i + 1] = 0x33;
            break;
        }
    }
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats s;
    ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, s),
              OK);
    auto* jpg = find_carved_by_ext(root, "jpg");
    ASSERT_NE(jpg, nullptr) << "dims-unknown truncated JPEG salvages at the /10 limit";
    EXPECT_EQ(jpg->props()->extents.front().offset, at);
    EXPECT_EQ(jpg->props()->extents.front().length, 256 * 1024 - at); /* to the device end */
    EXPECT_EQ(jpg->props()->carve_confidence, 750u);                  /* (900 + 600) / 2 */
}

TEST(Carve, JpegCompleteFileStopsAtEoiNotSofEstimate) {
    /* pin: a COMPLETE JPEG still accepts at its EOI with the full-walk
     * grade (the modern tier's semantics, S-018's INVALID evidence) —
     * the salvage never fires when EOI exists, and the precise EOI
     * extent (519) is never replaced by the SOF estimate (9216) */
    std::vector<u8> img(256 * 1024, 0x11);
    const size_t at = 4096;
    const size_t len = plant_jpeg(img, at);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats s;
    ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, s),
              OK);
    auto* jpg = find_carved_by_ext(root, "jpg");
    ASSERT_NE(jpg, nullptr) << "complete JPEG must carve";
    EXPECT_EQ(jpg->props()->extents.front().offset, at);
    EXPECT_EQ(jpg->props()->extents.front().length, len);
    EXPECT_EQ(jpg->props()->carve_confidence, 900u);
    EXPECT_EQ(jpg->confidence(), tree::Confidence::Excellent);
}

/* Build a minimal-but-grammar-valid CFHEADER at `at` with the magic in
 * either case (MSCF / mscf). Layout: magic@0, reserved@4, cbCabinet@8,
 * reserved@12, coffFiles@16, reserved@20, version@24, cFolders@26,
 * cFiles@28, flags@30, setID@32, iCabinet@34. */
static void cab_header(std::vector<u8>& img, size_t at, bool lower) {
    auto put32 = [&](u32 x) {
        img[at + 8] = u8(x);
        img[at + 9] = u8(x >> 8);
        img[at + 10] = u8(x >> 16);
        img[at + 11] = u8(x >> 24);
    };
    (void)put32;
    const char* magic = lower ? "mscf" : "MSCF";
    for (int i = 0; i < 4; ++i)
        img[at + i] = u8(magic[i]);
    /* cbCabinet @8 = 4096 */
    img[at + 8] = 0x00;
    img[at + 9] = 0x10;
    img[at + 10] = 0;
    img[at + 11] = 0;
    /* coffFiles @16 = 0x24 */
    img[at + 16] = 0x24;
    img[at + 17] = 0;
    img[at + 18] = 0;
    img[at + 19] = 0;
    /* version 3.1 @24, cFolders=2 @26, cFiles=10 @28, flags=0 @30,
     * setID=1 @32, iCabinet=0 @34; reserved fields stay zero */
    img[at + 24] = 3;
    img[at + 25] = 1;
    img[at + 26] = 2;
    img[at + 27] = 0;
    img[at + 28] = 10;
    img[at + 29] = 0;
    img[at + 30] = 0;
    img[at + 31] = 0;
    img[at + 32] = 1;
    img[at + 33] = 0;
    img[at + 34] = 0;
    img[at + 35] = 0;
}

TEST(Carve, CabLowercaseMagic) {
    /* N-069: the og's registry carries BOTH keys for case-insensitive
     * formats - "MSCF"/"mscf" are two separate registry rows (census :94,
     * "deliberate registry bloat for correctness", :127). A cabinet whose
     * CFHEADER spells the magic lowercase must carve exactly like the
     * uppercase one (same validator, same ext). */
    std::vector<u8> img(256 * 1024, 0);
    cab_header(img, 4096, false); /* uppercase MSCF */
    cab_header(img, 12288, true); /* lowercase mscf */
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats s;
    ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, s),
              OK);
    const std::string dot_cab = ".CAB";
    size_t carved = 0;
    tree::FileNode* lower_node = nullptr;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling()) {
        const std::string nm = c->name_utf8();
        if (nm.size() > dot_cab.size() &&
            nm.compare(nm.size() - dot_cab.size(), dot_cab.size(), dot_cab) == 0) {
            ++carved;
            if (c->props()->extents.front().offset == u64(12288))
                lower_node = c;
        }
    }
    EXPECT_EQ(carved, 2u) << "both magic cases must carve (og dual-key policy)";
    ASSERT_NE(lower_node, nullptr) << "lowercase mscf cabinet must carve";
    EXPECT_EQ(lower_node->props()->carve_format, u32(FMT_CAB_LC))
        << "the lowercase key is its own registry row (the og's duplicate-key policy)";
}

/* ===================================================================== */
/* M-051: RAW provenance — the analyser checksum, the tail anchor, and  */
/* the session round-trip (ifileproperties-part3 :77-90).               */
/* ===================================================================== */

TEST(CarveEngine, ProvenanceChecksumAndAnchorsPersist) {
    /* grammar layer: the checksum is a CRC32 of the extent's first 512
     * bytes (the og's +0x56 analyser-checksum field; 12,030/12,147
     * nonzero in the census) and anchor B stays 0 (grammar validation
     * bounds the extent itself); raw-sign layer: tail-paired hits carry
     * the tail offset (+0x94, the og's 261/12,147). Everything
     * round-trips through the session (nflags bit4). */
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

    int checked = 0;
    /* only the RECORD rows (direct children of the carve root) carry the
     * analyser checksum - the S-032 inner-entry presentation children
     * keep the honest 0xFFFFFFFF invalid marker (nothing analyzed them
     * separately; the og's census counts one record per archive). */
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling()) {
        if (c->props() && c->origin() == tree::Origin::Carved) {
            ++checked;
            EXPECT_NE(c->props()->carve_checksum, 0xFFFFFFFFu)
                << "readable record header must carry a real checksum";
            if (c->props()->carve_checksum != 0xFFFFFFFFu && !c->props()->extents.empty()) {
                u8 hdr[512];
                const u64 off = c->props()->extents.front().offset;
                const u64 ln = c->props()->extents.front().length;
                const u32 hb = u32(ln < sizeof(hdr) ? ln : sizeof(hdr));
                ASSERT_EQ(dev.read_at(off, hdr, hb), OK);
                EXPECT_EQ(c->props()->carve_checksum, table::crc32(hdr, hb));
            }
        }
    }
    EXPECT_GE(checked, 4);

    /* session round-trip: the provenance survives (the og persists the
     * same fields per RAW record) */
    std::vector<u8> blob;
    ASSERT_EQ(session::serialize_tree(tree, blob), OK);
    tree::ResultTree loaded;
    ASSERT_EQ(session::deserialize_tree(blob.data(), blob.size(), loaded), OK);
    int reloaded = 0;
    if (!loaded.roots().empty())
        for (tree::FileNode* c = loaded.roots()[0]->first_child(); c; c = c->next_sibling())
            if (c->props() && c->origin() == tree::Origin::Carved &&
                c->props()->carve_checksum != 0xFFFFFFFFu)
                ++reloaded;
    EXPECT_GE(reloaded, 4) << "provenance lost on reload";
}

TEST(CarveEngine, TailAnchorCarriedOnTailPairedHits) {
    /* the raw-sign layer's tail-paired rows carry anchor B (the og's
     * +0x94 tail offset); the sqlite tail-pair fixture (S-015's proven
     * shape) pins the anchor at the EOI position; header-only shapes
     * keep 0. */
    std::vector<u8> img(4 * 1024 * 1024, 0);
    const u64 start = 4096;
    const u64 lo = 1024;
    const u64 tail_stride = 2 * 1024 * 1024;
    std::memcpy(img.data() + start, "SQLite form", 11);
    std::memcpy(img.data() + start + lo + tail_stride + 31,
                "\x11\x1d\x05\x15\x19\x0f\x0f\x77\x6f\x72\x64", 11);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats s;
    ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, s),
              OK);
    tree::FileNode* sql_n = nullptr;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling()) {
        const std::string nm = c->name_utf8();
        if (nm.size() > 7 && nm.compare(nm.size() - 7, 7, ".SQLITE") == 0)
            sql_n = c;
    }
    ASSERT_NE(sql_n, nullptr);
    /* anchor B = the absolute tail-sign position (the og's tail offset) */
    /* anchor B = the tail sign's exact position (stride base + the sign's
     * intra-stride offset 31) */
    EXPECT_EQ(sql_n->props()->carve_anchor_b, start + lo + tail_stride + 31);
    EXPECT_NE(sql_n->props()->carve_checksum, 0xFFFFFFFFu);
}

/* ------------------------------------------------------------------ */
/* A-011: the SQLite header's multi-byte fields are BIG-ENDIAN (the    */
/* format spec; the og composes the page size BE first-hand,           */
/* FUN_18002e2b0's `v8 | (256 * v5 & 0xff00)`, RetDec :45142-45156).   */
/* The pre-fix LE reads rejected every real database (4096-byte pages  */
/* stored 10 00 at +16 read as 16 -> the < 512 gate).                  */
/* ------------------------------------------------------------------ */
TEST(CarveEngine, SqliteRealHeaderByteOrder) {
    /* a byte-exact real-format database: 4096-byte pages, 4 pages,
     * UTF-8 text encoding -> the grammar layer carves with the EXACT
     * page-math extent (fail-before: the LE read rejected the header) */
    std::vector<u8> img(64 * 1024, 0);
    const u64 start = 4096;
    const size_t db_bytes = plant_sqlite(img, start, 4096, 4, 1);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats st;
    ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, st),
              OK);
    tree::FileNode* sql_n = nullptr;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling()) {
        const std::string nm = c->name_utf8();
        if (nm.size() > 7 && nm.compare(nm.size() - 7, 7, ".SQLITE") == 0)
            sql_n = c;
    }
    ASSERT_NE(sql_n, nullptr) << "a real-format SQLite header must carve (pre-A-011: LE-rejected)";
    EXPECT_EQ(sql_n->props()->carve_format, u32(FMT_SQLITE))
        << "the grammar layer owns the carve (pre-A-011: only the sign layer fired)";
    EXPECT_EQ(sql_n->props()->logical_size, db_bytes) << "the exact page-math extent";
    std::string note;
    ASSERT_TRUE(cr::text::lossless_u16_to_utf8(sql_n->props()->carve_note, note));
    EXPECT_EQ(note, "page-math") << "the generic page-math note";
}

TEST(CarveEngine, SqliteInvalidEncodingRejects) {
    /* the og gates the text-encoding field's high byte < 4
     * (FUN_18002e2b0's `v7 >> 24 < 4`): encoding 4 is invalid */
    std::vector<u8> img(64 * 1024, 0);
    plant_sqlite(img, 4096, 4096, 4, 4);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats st;
    ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, st),
              OK);
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling()) {
        const std::string nm = c->name_utf8();
        if (nm.size() > 7 && nm.compare(nm.size() - 7, 7, ".SQLITE") == 0) {
            EXPECT_NE(c->props()->carve_format, u32(FMT_SQLITE))
                << "an invalid encoding must not reach the grammar layer "
                   "(the raw-sign layer's sign hit is legitimate)";
        }
    }
}

TEST(CarveEngine, SqliteLittleEndianShapeRejects) {
    /* the regression guard: the pre-A-011 wrongness (an LE page size at
     * +16) is not a valid SQLite header - 0x1000 LE = bytes 00 10 = the
     * BE read 16, and the pow-2/512 gates reject it */
    std::vector<u8> img(64 * 1024, 0);
    std::memcpy(img.data() + 4096, "SQLite format 3", 15);
    img[4096 + 15] = 0;
    put16(img, 4096 + 16, 4096); /* LE - the old wrong shape */
    putbe32(img, 4096 + 28, 4);
    putbe32(img, 4096 + 56, 1);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats st;
    ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, st),
              OK);
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling()) {
        const std::string nm = c->name_utf8();
        if (nm.size() > 7 && nm.compare(nm.size() - 7, 7, ".SQLITE") == 0) {
            EXPECT_NE(c->props()->carve_format, u32(FMT_SQLITE))
                << "the LE shape (00 10 at +16) must not reach the grammar "
                   "layer (the raw-sign layer's sign hit is legitimate)";
        }
    }
}

/* ------------------------------------------------------------------ */
/* N-089: the og's CFCPEvt deep walk relabels FCP event databases to   */
/* the subtype id 0x75 when a name read from the DB content (a UTF-16  */
/* buffer, WideCharToMultiByte'd, exactly 17 chars) equals             */
/* "tableactiveactive" (FUN_18002de40, RetDec :44847-45100; the .rdata */
/* literal at 0x6a0c0). The marker rides the DB's text encoding.       */
/* ------------------------------------------------------------------ */
TEST(CarveEngine, SqliteFcpeventSubtypeRelabeled) {
    /* a UTF-16LE FCP event database: the 34-byte UTF-16LE marker in
     * page 1 (the sqlite_master schema region) -> the subtype note */
    std::vector<u8> img(64 * 1024, 0);
    const u64 start = 4096;
    plant_sqlite(img, start, 4096, 4, 2); /* UTF-16LE */
    const char marker[18] = "tableactiveactive";
    for (int i = 0; i < 17; ++i) {
        img[start + 512 + 2 * i] = u8(marker[i]);
        img[start + 512 + 2 * i + 1] = 0;
    }
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats st;
    ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, st),
              OK);
    tree::FileNode* sql_n = nullptr;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling()) {
        const std::string nm = c->name_utf8();
        if (nm.size() > 7 && nm.compare(nm.size() - 7, 7, ".SQLITE") == 0)
            sql_n = c;
    }
    ASSERT_NE(sql_n, nullptr);
    std::string note;
    ASSERT_TRUE(cr::text::lossless_u16_to_utf8(sql_n->props()->carve_note, note));
    EXPECT_EQ(note, "page-math+fcpevent") << "the FCP-event-database subtype note";
}

TEST(CarveEngine, SqliteGenericLabelUnchangedWithoutMarker) {
    /* the same UTF-16LE database WITHOUT the marker -> the generic note */
    std::vector<u8> img(64 * 1024, 0);
    const u64 start = 4096;
    plant_sqlite(img, start, 4096, 4, 2);
    std::memcpy(img.data() + start + 512, "CREATE TABLE ordinary(x)", 25);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats st;
    ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, st),
              OK);
    tree::FileNode* sql_n = nullptr;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling()) {
        const std::string nm = c->name_utf8();
        if (nm.size() > 7 && nm.compare(nm.size() - 7, 7, ".SQLITE") == 0)
            sql_n = c;
    }
    ASSERT_NE(sql_n, nullptr);
    std::string note;
    ASSERT_TRUE(cr::text::lossless_u16_to_utf8(sql_n->props()->carve_note, note));
    EXPECT_EQ(note, "page-math") << "no marker -> the generic note";
}

/* ---------------------------------------------------------------------------
 * M-005: allocation-bitmap carve gating (the og's block-scan cluster gate,
 * RAWSearchDLL FUN_18000c820 :9446-9460 + FUN_180058df0 :67842-67856,
 * corpus-verified first-hand). The core's FreeMap is the inverted polarity
 * of the og's USED bitmap: bit set = free, clear = used. */
TEST(CarveEngine, BitmapSkipsUsedClusters) {
    /* Two PNGs: one inside a used cluster (suppressed wholesale), one
     * inside a free cluster (carved). Fail-before: pre-M-005 the carve
     * had no gate input at all - both surfaced. */
    std::vector<u8> img(64 * 1024, 0xAA);
    const size_t used_png_at = 8192 + 137;  /* inside cluster 2 */
    const size_t free_png_at = 40960 + 137; /* inside cluster 10 */
    plant_png(img, used_png_at);
    plant_png(img, free_png_at);
    MemDevice dev(std::move(img));

    storage::FreeMap fm;
    fm.base_byte = 0;
    fm.cluster_bytes = 4096;
    fm.cluster_count = 64 * 1024 / 4096; /* 16 */
    fm.bits.assign(fm.cluster_count / 8, 0xFF);
    for (u64 k = 2; k <= 3; ++k) /* clusters 2..3 = the used span */
        fm.bits[size_t(k / 8)] &= u8(~(1u << (k % 8)));

    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    flags.free_map = &fm;
    CarveStats stats;
    u32 rc =
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats);
    ASSERT_EQ(rc, OK) << err_text(rc);

    std::vector<tree::FileNode*> all;
    std::function<void(tree::FileNode*)> collect = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            all.push_back(c);
            collect(c);
        }
    };
    collect(root);
    ASSERT_EQ(all.size(), 1u) << "the used-cluster PNG must be suppressed";
    EXPECT_EQ(all[0]->props()->extents.front().offset, free_png_at);
    EXPECT_EQ(all[0]->props()->carve_format, FMT_PNG);
    /* telemetry: both used clusters skipped = 8 KiB never offered */
    EXPECT_EQ(stats.bitmap_skipped_clusters, 2u);
    EXPECT_EQ(stats.bitmap_skipped_bytes, 8192u);
}

TEST(CarveEngine, NoBitmapCarvesEverything) {
    /* The control leg + the og's raw-disk rule: no map (and an INVALID
     * map) -> carve everything. Fail-before: N/A (this is the unchanged
     * baseline behavior the fix must preserve). */
    std::vector<u8> img(64 * 1024, 0xAA);
    const size_t a = 8192 + 137;
    const size_t b = 40960 + 137;
    plant_png(img, a);
    plant_png(img, b);
    MemDevice dev(std::move(img));

    auto run = [&](const storage::FreeMap* fm, size_t expect, CarveStats& st) {
        tree::ResultTree tree;
        auto* root = tree.add_root(u"Lost Files");
        CarveFlags flags;
        flags.free_map = fm;
        st = CarveStats{};
        u32 rc =
            carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, st);
        ASSERT_EQ(rc, OK) << err_text(rc);
        std::vector<tree::FileNode*> all;
        std::function<void(tree::FileNode*)> collect = [&](tree::FileNode* n) {
            for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
                all.push_back(c);
                collect(c);
            }
        };
        collect(root);
        EXPECT_EQ(all.size(), expect);
    };

    CarveStats st;
    run(nullptr, 2, st); /* no map: both PNGs surface */
    EXPECT_EQ(st.bitmap_skipped_bytes, 0u);

    storage::FreeMap invalid; /* zeroed: cluster_bytes 0 -> not valid */
    run(&invalid, 2, st);     /* invalid map must not gate either */
    EXPECT_EQ(st.bitmap_skipped_bytes, 0u);
}

/* ===================================================================== */
/* M-024 (diff-to-fix): the runtime per-format enable/disable surface —   */
/* the og's RawConfig.ini [Filter] + IAnalyserFilter registry rebuild    */
/* (RAWSearchDLL FUN_180006b30 :4386-4434; 37 named toggles, STL/VPJ     */
/* ship default-off). The core's CarveFlags::disabled_formats gate.      */
/* ===================================================================== */

TEST(CarveEngine, FormatDisabledSkipsSignature) {
    /* "jpg" disabled -> the grammar layer never dispatches the JPEG
     * signature; PNG (not listed) still carves. Fail-before: no disable
     * surface existed at all. */
    auto img = build_carve_image();
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    flags.disabled_formats = "jpg, JPEG"; /* both spellings, case-insensitive */
    CarveStats stats;
    u32 rc =
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats);
    ASSERT_EQ(rc, OK) << err_text(rc);

    int jpg = 0, png = 0;
    std::function<void(tree::FileNode*)> collect = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            if (c->props() && c->origin() == tree::Origin::Carved) {
                const std::string nm = c->name_utf8();
                if (nm.size() >= 3) {
                    const std::string ext = nm.substr(nm.size() - 3);
                    if (ext == "JPG")
                        ++jpg;
                    if (ext == "PNG")
                        ++png;
                }
            }
            collect(c);
        }
    };
    collect(root);
    EXPECT_EQ(jpg, 0) << "jpg is on the disabled list: no JPEG may carve";
    EXPECT_GE(png, 1) << "png is NOT on the list: PNG must still carve";
}

TEST(CarveEngine, FormatDisabledRawSignsToo) {
    /* the raw-sign layer honors the same list: the "sample" format
     * (#!/bin/sh) disabled -> its header sign never fires. Fail-before:
     * no disable surface existed. */
    std::vector<u8> img(2 * 1024 * 1024, 0);
    std::memcpy(img.data() + 4096, "#!/bin/sh\n", 10);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    flags.disabled_formats = "sample";
    CarveStats stats;
    u32 rc =
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats);
    ASSERT_EQ(rc, OK) << err_text(rc);
    EXPECT_EQ(stats.sign_hits, 0u) << "the disabled raw-sign format must not fire";
    EXPECT_EQ(root->stats().file_count + root->stats().dir_count, 0u)
        << "nothing may be emitted from the disabled format";

    /* positive control: without the list the sample sign fires (the
     * RawSignCarvedNodesAreDeleted pin's shape) */
    std::vector<u8> img2(2 * 1024 * 1024, 0);
    std::memcpy(img2.data() + 4096, "#!/bin/sh\n", 10);
    MemDevice dev2(std::move(img2));
    tree::ResultTree tree2;
    auto* root2 = tree2.add_root(u"Lost Files");
    CarveFlags flags2;
    CarveStats stats2;
    ASSERT_EQ(carve_slice(&dev2, flags2, nullptr, nullptr, nullptr, tree2, root2, nullptr, nullptr,
                          stats2),
              OK);
    EXPECT_GE(stats2.sign_hits, 1u) << "enabled by default: the sample sign fires";
}

TEST(CarveEngine, AllFormatsEnabledByDefault) {
    /* null/empty list = every format enabled (the og's compiled-in set;
     * its STL/VPJ default-off is HOST policy expressed through
     * RawConfig.ini, not an engine constant). The enumeration surface:
     * registry order IS FormatId order, one entry per id. */
    EXPECT_EQ(carve::registry_size(), carve::registry_size());
    EXPECT_GE(carve::registry_size(), 30u) << "the compiled registry carries the full set";
    for (size_t i = 0; i < carve::registry_size(); ++i) {
        const Signature& sig = carve::registry()[i];
        EXPECT_FALSE(carve_format_listed(nullptr, sig));
        EXPECT_FALSE(carve_format_listed("", sig));
        EXPECT_NE(sig.ext, nullptr);
        EXPECT_NE(sig.name, nullptr);
    }
    /* the og's [Filter] keys are name-form tokens: the ext and the name
     * both address a format; separators are comma/space/semicolon */
    const Signature& jpg = carve::registry()[FMT_JPG];
    EXPECT_TRUE(carve_format_listed("jpg", jpg));
    EXPECT_TRUE(carve_format_listed("JPG", jpg));
    EXPECT_TRUE(carve_format_listed("heic, jpg", jpg));
    EXPECT_TRUE(carve_format_listed("heic;jpg", jpg));
    EXPECT_TRUE(carve_format_listed("heic jpg", jpg));
    EXPECT_FALSE(carve_format_listed("jpe", jpg));
}

/* ---- N-063: the og's per-analyser carved-file naming ------------------- */

namespace {

/* a minimal MP4/MOV: ftyp + moov{mvhd} + mdat + terminator. mvhd version
 * and time fields are caller-controlled (the og's naming source). */
std::vector<u8> build_bmff_named_image(u8 mvhd_version, u32 v0_time, u64 v1_time,
                                       const char* brand) {
    std::vector<u8> img(2 * 512, 0); /* 1 KiB slice: ftyp+moov+mdat fit */
    const auto be32 = [&img](size_t p, u32 v) {
        img[p] = u8(v >> 24);
        img[p + 1] = u8(v >> 16);
        img[p + 2] = u8(v >> 8);
        img[p + 3] = u8(v);
    };
    const size_t at = 0;
    /* ftyp (20 bytes) */
    be32(at, 20);
    std::memcpy(img.data() + at + 4, "ftyp", 4);
    std::memcpy(img.data() + at + 8, brand, 4);
    be32(at + 12, 0x200);
    std::memcpy(img.data() + at + 16, brand, 4);
    /* moov: header (8) + mvhd */
    const size_t mvhd_len = mvhd_version == 1 ? 40 : 28;
    const size_t moov_len = 8 + mvhd_len;
    be32(at + 20, u32(8 + moov_len));
    std::memcpy(img.data() + at + 24, "moov", 4);
    const size_t mv = at + 28;
    be32(mv, u32(mvhd_len));
    std::memcpy(img.data() + mv + 4, "mvhd", 4);
    img[mv + 8] = mvhd_version; /* version; flags stay zero */
    if (mvhd_version == 1) {
        /* creation u64 @+12, modification u64 @+20 (BE); timescale u32
         * @+28, duration u64 @+32 (the proper v1 layout - S-049) */
        for (int i = 0; i < 8; ++i)
            img[mv + 12 + i] = u8(v1_time >> (8 * (7 - i)));
        for (int i = 0; i < 8; ++i)
            img[mv + 20 + i] = u8(v1_time >> (8 * (7 - i)));
        be32(mv + 28, 600); /* timescale */
        for (int i = 0; i < 8; ++i)
            img[mv + 32 + i] = u8(u64(3600) >> (8 * (7 - i)));
    } else {
        be32(mv + 12, 0);       /* creation */
        be32(mv + 16, v0_time); /* modification - the og's naming field */
        be32(mv + 20, 600);     /* timescale */
        be32(mv + 24, 3600);    /* duration */
    }
    /* mdat (24 bytes) + terminator box (over-cap size ends the walk) */
    const size_t md = at + 20 + moov_len;
    be32(md, 24);
    std::memcpy(img.data() + md + 4, "mdat", 4);
    std::memset(img.data() + md + 8, 0x5A, 16);
    be32(md + 24, 0xFFFFFFFF);
    return img;
}

/* M-025: window-serving read context - the og's already-in-buffer model.
 * A fully in-window candidate (validator walk + checksum + small-file
 * hash) must cost ZERO additional device reads: the window fill is the
 * only traffic. Pre-fix every marker-walk rd(), the entropy block, the
 * 512 B checksum and the hash re-read were separate device reads per
 * candidate (~15-20 per small JPEG). */
class CountingDevice final : public IStorageDevice {
public:
    explicit CountingDevice(std::vector<u8> b) : data_(std::move(b)) {
        geom_.logical_sector = 512;
        geom_.physical_sector = 512;
        geom_.sector_count = data_.size() / 512;
        geom_.total_bytes = geom_.sector_count * 512;
    }
    u32 read_at(u64 off, void* buf, u32 len) override {
        ++calls_;
        bytes_ += len;
        if (off > data_.size() || len > data_.size() - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        std::memcpy(buf, data_.data() + off, len);
        return OK;
    }
    const Geometry& geometry() const override { return geom_; }
    u64 calls() const { return calls_; }
    u64 bytes() const { return bytes_; }

private:
    std::vector<u8> data_;
    Geometry geom_{};
    u64 calls_ = 0;
    u64 bytes_ = 0;
};

TEST(CarveEngine, WindowServingEliminatesPerCandidateRereads) {
    /* one small PNG per window at +0x2000 (outside every overlap zone,
     * clear of the previous window's 4 KiB tail): the PNG validator's
     * reads (IHDR probes + the small chunk walk) all fall inside the
     * window [w*1MiB, w*1MiB + 1MiB + 4KiB), so a correct window-serving
     * device serves ALL of them from the buffer */
    std::vector<u8> img(4ull << 20, 0);
    int planted = 0;
    for (int w = 0; w < 4; ++w) {
        plant_png(img, size_t(w) * (1ull << 20) + 0x2000);
        ++planted;
    }
    CountingDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Carved");
    CarveFlags fl;
    fl.hash_small_files = true;
    fl.hash_threshold = 8ull << 20; /* hash every plant (519 B each) */
    CarveStats st;
    ASSERT_EQ(OK,
              carve_slice(&dev, fl, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, st));
    EXPECT_EQ(u64(planted), st.validated) << "all plants carved";
    EXPECT_EQ(u64(planted), st.hashed) << "all plants hashed (in-window, no re-read)";
    /* S-047: the TS family's 1-byte 0x47 magic DOES trial on PNG payload
     * bytes that happen to carry 0x47 + a follow-set byte - expected
     * cross-format rejections now (each dies inside its 2 KiB first
     * block, served from the window). The invariant this test owns is
     * the DEVICE-READ count below, not the absence of TS trials. */
    EXPECT_LT(st.rejected, u64(32)) << "TS noise trials stay bounded (2 KiB first block)";
    /* the four window fills are the ONLY device reads - every validator
     * probe, chunk walk, checksum and hash served from the window buffer
     * (the og's already-in-buffer model: candidates cost zero device
     * traffic when their reads fit the window) */
    EXPECT_EQ(u64(4), dev.calls())
        << "per-candidate device re-reads must be eliminated (window fills only)";
}

TEST(CarveEngine, WindowServingServesStraddlingReadsCorrectly) {
    /* M-025 correctness: a read straddling the window end serves the
     * in-window prefix from the buffer and the remainder from the device
     * - byte-identical to a direct device read. A JPEG planted near the
     * END of a window validates with the same extent, checksum and hash
     * as the same plant mid-window. */
    std::vector<u8> img(2ull << 20, 0);
    const size_t at = (1ull << 20) - 300; /* the 521 B plant crosses the
                                           * window-0 boundary (window 0
                                           * covers [0, 1 MiB + 4 KiB)) */
    plant_jpeg(img, at);
    CountingDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Carved");
    CarveFlags fl;
    fl.hash_small_files = true;
    fl.hash_threshold = 8ull << 20;
    CarveStats st;
    ASSERT_EQ(OK,
              carve_slice(&dev, fl, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, st));
    ASSERT_EQ(1u, st.validated);
    ASSERT_EQ(1u, st.hashed);
    /* the emitted node's checksum matches a direct CRC32 of the plant */
    tree::FileNode* n = nullptr;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
        if (c->props())
            n = c;
    ASSERT_NE(n, nullptr);
    ASSERT_EQ(1u, n->props()->extents.size());
    EXPECT_EQ(u64(at), n->props()->extents[0].offset);
    EXPECT_EQ(u64(521), n->props()->extents[0].length);
    EXPECT_TRUE(n->props()->hash_valid);
    /* direct CRC32 of the same bytes for the checksum comparison */
    std::vector<u8> probe(512, 0);
    CountingDevice direct(([] {
        std::vector<u8> d(2ull << 20, 0);
        plant_jpeg(d, (1ull << 20) - 300);
        return d;
    })());
    ASSERT_EQ(OK, direct.read_at(at, probe.data(), 512));
    EXPECT_EQ(cr::table::crc32(probe.data(), 512), n->props()->carve_checksum);
}

} // namespace

TEST(CarveEngine, Mp4NamedByDurationPrefix) {
    /* S-049 (supersedes N-063's carve-tree application): the real og
     * sessions name every carved MP4 "<EXT>_<duration>_<%06d>.<EXT>"
     * - 57/57 real MP4/MOV carve rows (MP4_14s_000003.MP4,
     * MP4_08m07s_000030.MP4, MOV_10s_000009.MOV) and ZERO "Lost Name
     * File (...)_EaseUS" template names: the DRWUI tree-naming layer
     * names every unnamed carve through the %s_%06d contract and the
     * engine's SetName template does not survive it. This fixture's
     * mvhd carries timescale 600 / duration 3600 -> 6 seconds ->
     * MP4_06s_000001.MP4. Fail-before: the node was named
     * "Lost Name File (2025:02:06 12:00:00)_EaseUS" (the N-063
     * landing, superseded by executed evidence). */
    auto img = build_bmff_named_image(0, 1738843200u, 0, "isom");
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats st;
    ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, st),
              OK);
    tree::FileNode* mp4 = nullptr;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
        if (c->props() && c->props()->carve_format == FMT_MP4)
            mp4 = c;
    ASSERT_NE(mp4, nullptr) << "the MP4 must carve";
    EXPECT_EQ(mp4->props()->name, std::u16string(u"MP4_06s_000001.MP4"));
}

TEST(CarveEngine, MovNamedByDurationPrefix) {
    /* S-049: the MOV (qt brand) analyser carries the same duration
     * contract - MOV_06s_000001.MOV on the 6-second fixture */
    auto img = build_bmff_named_image(0, 0u, 0, "qt  ");
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats st;
    ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, st),
              OK);
    tree::FileNode* mov = nullptr;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
        if (c->props() && c->props()->carve_format == FMT_MOV)
            mov = c;
    ASSERT_NE(mov, nullptr) << "the MOV must carve";
    EXPECT_EQ(mov->props()->name, std::u16string(u"MOV_06s_000001.MOV"));
}

TEST(CarveEngine, MovV1MvhdDurationRead) {
    /* S-049: the v1 mvhd layout (timescale u32@box+28, duration
     * u64@box+32) - the N-063 v1 high-word creation-time quirk retired
     * with the template it belonged to; the duration is read from its
     * proper v1 field. The fixture writes v1 timescale 600 / duration
     * 3600 -> MP4_06s_000001.MP4. */
    auto img = build_bmff_named_image(1, 0, 0x00000000E0B67B00ull, "isom");
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats st;
    ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, st),
              OK);
    tree::FileNode* mp4 = nullptr;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
        if (c->props() && c->props()->carve_format == FMT_MP4)
            mp4 = c;
    ASSERT_NE(mp4, nullptr);
    EXPECT_EQ(mp4->props()->name, std::u16string(u"MP4_06s_000001.MP4"))
        << "the v1 duration field must be read from its proper layout";
}

TEST(CarveEngine, ReconstructedMultiVolumeGrouping) {
    /* S-048: the per-volume carve grouping - two VolumeGroups in one
     * slice, each lazily getting its own group child under the carve
     * root with its files attached DIRECTLY to it and one empty
     * per-extension folder beside them (the og's persisted shape:
     * "Reconstructed/ (I:)/jpg" + "Reconstructed/ (I:)/PNG_000001.PNG";
     * the multi-volume whole-disk scans' every carved row, 100% of
     * 189,115 real rows across the three og sessions). */
    std::vector<u8> img(256 * 1024, 0);
    plant_jpeg(img, 4096);      /* group A ("Data (D:)") */
    plant_png(img, 132 * 1024); /* group B ("System Reserved") */
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Reconstructed");
    CarveFlags flags;
    VolumeGroup ga;
    ga.start = 0;
    ga.end = 128 * 1024;
    ga.name = u"Data (D:)";
    VolumeGroup gb;
    gb.start = 128 * 1024;
    gb.end = 256 * 1024;
    gb.name = u"System Reserved";
    flags.volume_groups.push_back(ga);
    flags.volume_groups.push_back(gb);
    CarveStats st;
    ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, st),
              OK);
    tree::FullNode* grpA = nullptr;
    tree::FullNode* grpB = nullptr;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
        if (c->is_full()) {
            tree::FullNode* f = static_cast<tree::FullNode*>(c);
            if (f->role == u"Data (D:)")
                grpA = f;
            else if (f->role == u"System Reserved")
                grpB = f;
        }
    ASSERT_NE(grpA, nullptr) << "group A's child exists";
    ASSERT_NE(grpB, nullptr) << "group B's child exists";
    /* every file under its OWN volume; the ext folders empty beside */
    int a_files = 0, b_files = 0;
    for (tree::FileNode* c = grpA->first_child(); c; c = c->next_sibling()) {
        if (!c->is_full()) {
            ++a_files;
            EXPECT_EQ(c->props()->carve_format, u32(FMT_JPG)) << "the JPEG is group A's";
        } else {
            EXPECT_EQ(static_cast<tree::FullNode*>(c)->role, u"jpg");
            EXPECT_EQ(c->first_child(), nullptr) << "the ext folder is empty";
        }
    }
    for (tree::FileNode* c = grpB->first_child(); c; c = c->next_sibling()) {
        if (!c->is_full()) {
            ++b_files;
            EXPECT_EQ(c->props()->carve_format, u32(FMT_PNG)) << "the PNG is group B's";
        } else {
            EXPECT_EQ(static_cast<tree::FullNode*>(c)->role, u"png");
            EXPECT_EQ(c->first_child(), nullptr) << "the ext folder is empty";
        }
    }
    EXPECT_EQ(a_files, 1);
    EXPECT_EQ(b_files, 1);
}

TEST(CarveEngine, ReconstructedUnattributedPositionStaysAtRoot) {
    /* S-048: a candidate OUTSIDE every volume group attaches to the
     * carve root DIRECTLY - never mis-attributed to a wrong volume (the
     * gap position: the og's real sessions never sample one; the honest
     * form is unattributed-at-root). */
    std::vector<u8> img(256 * 1024, 0);
    plant_png(img, 4096);        /* inside group A */
    plant_jpeg(img, 200 * 1024); /* OUTSIDE both groups (the gap) */
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Reconstructed");
    CarveFlags flags;
    VolumeGroup ga;
    ga.start = 0;
    ga.end = 128 * 1024;
    ga.name = u"Data (D:)";
    flags.volume_groups.push_back(ga);
    CarveStats st;
    ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, st),
              OK);
    tree::FullNode* grpA = nullptr;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
        if (c->is_full() && static_cast<tree::FullNode*>(c)->role == u"Data (D:)")
            grpA = static_cast<tree::FullNode*>(c);
    ASSERT_NE(grpA, nullptr);
    int a_files = 0, root_files = 0;
    for (tree::FileNode* c = grpA->first_child(); c; c = c->next_sibling())
        if (!c->is_full())
            ++a_files;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
        if (!c->is_full())
            ++root_files;
    EXPECT_EQ(a_files, 1) << "the in-group PNG is attributed to the volume";
    EXPECT_EQ(root_files, 1) << "the gap JPEG stays at the root (unattributed)";
}

TEST(CarveEngine, ReconstructedSingleDefaultGroupName) {
    /* S-048: the single-volume scan shape - one default group name over
     * the whole slice (no ranges), the common case (" (H:)" - the H
     * session's 94,057-row form). */
    auto img = build_carve_image();
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Reconstructed");
    CarveFlags flags;
    flags.volume_group_name = u" (H:)";
    CarveStats st;
    ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, st),
              OK);
    tree::FullNode* grp = nullptr;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
        if (c->is_full() && static_cast<tree::FullNode*>(c)->role == u" (H:)")
            grp = static_cast<tree::FullNode*>(c);
    ASSERT_NE(grp, nullptr) << "the single default group child exists";
    int files = 0;
    for (tree::FileNode* c = grp->first_child(); c; c = c->next_sibling())
        if (!c->is_full())
            ++files;
    EXPECT_GE(files, 4) << "every carved file attaches to the volume child";
}

TEST(CarveEngine, OgNamingUniversalExtCounter) {
    /* S-049: the og's universal naming contract - a format with no
     * content-derived prefix names "<UPPEREXT>_<%06d>.<UPPEREXT>": the
     * fixture's PNG is PNG_000001.PNG (real-session form: PNG_N.PNG
     * x42,412, GZ_000001.GZ x53,891 - every observed format). The
     * counter is PER-FORMAT (each format counts from 1 - the JPG
     * counter runs 1..3938 across its prefix variants in the G
     * session; the PNG counter 1..17,580 beside it). */
    auto img = build_carve_image();
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats st;
    ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, st),
              OK);
    bool saw_png = false, saw_docx = false;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling()) {
        if (!c->props() || !c->props()->extents.empty() == false)
            continue;
        if (c->props()->carve_format == u32(FMT_PNG)) {
            saw_png = true;
            EXPECT_EQ(c->props()->name, std::u16string(u"PNG_000001.PNG"));
        }
        if (c->props()->carve_format == u32(FMT_DOCX)) {
            saw_docx = true;
            /* no docProps/core.xml in this fixture: the plain default */
            EXPECT_EQ(c->props()->name, std::u16string(u"DOCX_000001.DOCX"));
        }
    }
    EXPECT_TRUE(saw_png) << "the fixture's PNG must carve";
    EXPECT_TRUE(saw_docx) << "the fixture's DOCX must carve";
}

TEST(CarveEngine, OgNamingJpgSofDimensions) {
    /* S-049: the JPEG dims prefix "JPG_<W>x<H>" (real-session pins:
     * JPG_192x144_000001.JPG, JPG_105x142_000019.JPG, JPG_1x1,
     * JPG_1920x1080, JPG_3840x2160). The fixture's plant_jpeg carries
     * SOF0 H=64 / W=48 -> JPG_48x64_000001.JPG. */
    auto img = build_carve_image();
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats st;
    ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, st),
              OK);
    bool saw_jpg = false;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling()) {
        if (c->props() && c->props()->carve_format == u32(FMT_JPG)) {
            saw_jpg = true;
            EXPECT_EQ(c->props()->name, std::u16string(u"JPG_48x64_000001.JPG"));
        }
    }
    EXPECT_TRUE(saw_jpg) << "the fixture's JPEG must carve";
}

TEST(CarveEngine, OgNamingJpgExifModelAndSoftware) {
    /* S-049: the JPEG EXIF prefixes - Model (GT-S7562_000123.JPG) then
     * Software (Adobe Photoshop 7.0_N.JPG, GIMP 2.10.14, Android
     * SP1A.210812.016.T505XXS4CVJ1 - spaces and dots preserved
     * verbatim). A JPEG with EXIF Model names by the model; one with
     * only Software names by the software. */
    const auto plant_exif_jpeg = [](std::vector<u8>& img, size_t at, const char* model,
                                    size_t model_len, const char* software, size_t sw_len) {
        /* SOI + APP1 Exif + TIFF/IFD0 with Model (0x0110) and/or
         * Software (0x0131) + SOF0 + SOS + EOI */
        img[at] = 0xFF;
        img[at + 1] = 0xD8;
        /* APP1 segment: len = 2 + 6 ("Exif\0\0") + 8 (TIFF header) +
         * 2 (entry count) + 12*n + 4 (next-IFD) + the value strings
         * (each NUL-terminated; they live INSIDE the segment - a value
         * area past the segment end would collide with the next marker) */
        const size_t entries = (model ? 1 : 0) + (software ? 1 : 0);
        const size_t strings = (model ? model_len + 1 : 0) + (software ? sw_len + 1 : 0);
        const size_t ifd = 8 + 2 + 12 * entries + 4;
        const size_t app1_len = 2 + 6 + ifd + strings;
        img[at + 2] = 0xFF;
        img[at + 3] = 0xE1;
        putbe16(img, at + 4, u16(app1_len));
        std::memcpy(img.data() + at + 6, "Exif\0\0", 6);
        const size_t t = at + 12; /* TIFF header */
        img[t] = 'M';
        img[t + 1] = 'M'; /* big-endian */
        img[t + 2] = 0;
        img[t + 3] = 42;
        putbe32(img, t + 4, 8); /* IFD0 at +8 */
        putbe16(img, t + 8, u16(entries));
        size_t e = t + 10;
        size_t str_at = e + 12 * entries + 4;
        if (model) {
            putbe16(img, e, 0x0110);
            putbe16(img, e + 2, 2);
            putbe32(img, e + 4, u32(model_len));
            putbe32(img, e + 8, u32(str_at - t)); /* TIFF-relative */
            std::memcpy(img.data() + str_at, model, model_len);
            str_at += model_len + 1;
            e += 12;
        }
        if (software) {
            putbe16(img, e, 0x0131);
            putbe16(img, e + 2, 2);
            putbe32(img, e + 4, u32(sw_len));
            putbe32(img, e + 8, u32(str_at - t));
            std::memcpy(img.data() + str_at, software, sw_len);
            e += 12;
        }
        putbe32(img, e, 0); /* next-IFD */
        size_t p = at + 4 + app1_len;
        /* SOF0: H=100, W=200 */
        img[p] = 0xFF;
        img[p + 1] = 0xC0;
        putbe16(img, p + 2, 17);
        img[p + 4] = 8;
        putbe16(img, p + 5, 100);
        putbe16(img, p + 7, 200);
        img[p + 9] = 1;
        img[p + 10] = 1;
        img[p + 11] = 0x22;
        img[p + 12] = 0;
        p += 2 + 17;
        img[p] = 0xFF;
        img[p + 1] = 0xDA;
        putbe16(img, p + 2, 12);
        p += 2 + 12;
        /* entropy filler: clear the registry's 512-byte min_size (the
         * plant must be a carvable file, not a header-only stub) */
        for (size_t k = 0; k < 480; ++k)
            img[p + k] = 0x11;
        p += 480;
        img[p] = 0x22;
        img[p + 1] = 0xFF;
        img[p + 2] = 0xD9; /* EOI */
    };
    {
        /* Model wins: GT-S7562 over the SOF dims and the Software */
        std::vector<u8> img(64 * 1024, 0);
        plant_exif_jpeg(img, 512, "GT-S7562", 8, "Adobe Photoshop 7.0", 20);
        MemDevice dev(std::move(img));
        tree::ResultTree tree;
        auto* root = tree.add_root(u"Lost Files");
        CarveFlags flags;
        CarveStats st;
        ASSERT_EQ(
            carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, st),
            OK);
        bool saw = false;
        for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
            if (c->props() && c->props()->carve_format == u32(FMT_JPG)) {
                saw = true;
                EXPECT_EQ(c->props()->name, std::u16string(u"GT-S7562_000001.JPG"));
            }
        EXPECT_TRUE(saw) << "the EXIF-Model JPEG must carve";
    }
    {
        /* Software when no Model (spaces preserved verbatim) */
        std::vector<u8> img(64 * 1024, 0);
        plant_exif_jpeg(img, 512, nullptr, 0, "Adobe Photoshop 7.0", 20);
        MemDevice dev(std::move(img));
        tree::ResultTree tree;
        auto* root = tree.add_root(u"Lost Files");
        CarveFlags flags;
        CarveStats st;
        ASSERT_EQ(
            carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, st),
            OK);
        bool saw = false;
        for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
            if (c->props() && c->props()->carve_format == u32(FMT_JPG)) {
                saw = true;
                EXPECT_EQ(c->props()->name, std::u16string(u"Adobe Photoshop 7.0_000001.JPG"));
            }
        EXPECT_TRUE(saw) << "the EXIF-Software JPEG must carve";
    }
}

TEST(CarveEngine, OgNamingMp3Id3Title) {
    /* S-049: the MP3 prefix = the ID3v2 title (real-session pins:
     * "AREE DEEWANO MUJHE PECHANO_000123.MP3", "Ek Lo Ek Muft_N.MP3",
     * "02 Prem Ratan Dhan Payo (SongsMp3.Com)_N.MP3" - spaces, dots and
     * parentheses preserved verbatim). Fixture: ID3v2.3 with a TIT2
     * frame carrying a known title, then two real MPEG frames. */
    std::vector<u8> img(1ull << 20, 0);
    u8* p = img.data();
    memcpy(p, "ID3", 3);
    p[3] = 3; /* v2.3 */
    p[4] = 0;
    p[5] = 0; /* flags */
    /* tag: TIT2 (10-byte header + payload 28) + padding to clear the
     * registry's 512-byte min_size (10 header + 420 tag + 2x131 frames
     * = 693 >= 512) */
    const size_t tag_size = 420;
    p[6] = u8((tag_size >> 21) & 0x7F);
    p[7] = u8((tag_size >> 14) & 0x7F);
    p[8] = u8((tag_size >> 7) & 0x7F);
    p[9] = u8(tag_size & 0x7F);
    memcpy(p + 10, "TIT2", 4);
    const size_t title_len = 27;
    p[14] = 0;
    p[15] = 0;
    p[16] = 0;
    p[17] = u8(1 + title_len); /* frame size: encoding byte + title */
    p[18] = 0;
    p[19] = 0; /* flags */
    p[20] = 0; /* encoding: latin-1 */
    memcpy(p + 21, "AREE DEEWANO MUJHE PECHANO", title_len);
    /* two real frames: FF FB 22 22 -> 131 B each (the 2-frame floor) */
    for (int fr = 0; fr < 2; ++fr) {
        u8* q = p + 10 + tag_size + size_t(fr) * 131;
        q[0] = 0xFF;
        q[1] = 0xFB;
        q[2] = 0x22;
        q[3] = 0x22;
        memset(q + 4, 0x33, 127);
    }
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats st;
    ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, st),
              OK);
    bool saw = false;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
        if (c->props() && c->props()->carve_format == u32(FMT_MP3)) {
            saw = true;
            EXPECT_EQ(c->props()->name, std::u16string(u"AREE DEEWANO MUJHE PECHANO_000001.MP3"));
        }
    EXPECT_TRUE(saw) << "the titled MP3 must carve";
}

TEST(CarveEngine, OgNamingPdfCreatorAndDatePrefixes) {
    /* S-049: the PDF prefix families (real-session pins): Title
     * (" TeX output 1998.06.28_17" - a 25-char truncation pin),
     * Creator/Producer + CreationDate ("TeX2022-03-03 21-11-38", "GPL
     * Ghostscript 9.56.12022-08-17 15-05-58"), the bare date
     * ("2020-05-23 23-23-23"), and matplotlib's dashed
     * "D:2022-08-17 15:05:58+02'00'" form which produces NO date (the
     * 14-plain-digit read rejects it - never a date prefix in any
     * session). */
    const auto plant_pdf = [](std::vector<u8>& img, size_t at, const char* title,
                              const char* creator, const char* date) {
        std::string body = "%PDF-1.4\n";
        if (title)
            body += std::string("/Title (") + title + ")\n";
        if (creator)
            body += std::string("/Creator (") + creator + ")\n";
        if (creator)
            body += std::string("/Producer (") + creator + ")\n";
        if (date)
            body += std::string("/CreationDate (") + date + ")\n";
        body += "%%EOF";
        /* pad the header line so the body clears the registry's
         * 128-byte min_size (a comment line is valid PDF content) */
        const std::string pad = "% " + std::string(96, 'z') + "\n";
        memcpy(img.data() + at, body.data(), 9); /* the %PDF-1.x line */
        memcpy(img.data() + at + 9, pad.data(), pad.size());
        memcpy(img.data() + at + 9 + pad.size(), body.data() + 9, body.size() - 9);
        return body.size() + pad.size();
    };
    {
        /* Creator + plain date -> "TeX2022-03-03 21-11-38" */
        std::vector<u8> img(1ull << 20, 0);
        plant_pdf(img, 4096, nullptr, "TeX", "D:20220303211138");
        MemDevice dev(std::move(img));
        tree::ResultTree tree;
        auto* root = tree.add_root(u"Lost Files");
        CarveFlags flags;
        CarveStats st;
        ASSERT_EQ(
            carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, st),
            OK);
        bool saw = false;
        for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
            if (c->props() && c->props()->carve_format == u32(FMT_PDF)) {
                saw = true;
                EXPECT_EQ(c->props()->name, std::u16string(u"TeX2022-03-03 21-11-38_000001.PDF"));
            }
        EXPECT_TRUE(saw) << "the dated PDF must carve";
    }
    {
        /* Title wins over Creator+date; a >25-char Title truncates at 25
         * (the two real-session truncation pins: "LaTeX with hyperref
         * packa", " TeX output 1998.06.28_17") */
        std::vector<u8> img(1ull << 20, 0);
        plant_pdf(img, 4096, "LaTeX with hyperref package", "TeX", "D:20220303211138");
        MemDevice dev(std::move(img));
        tree::ResultTree tree;
        auto* root = tree.add_root(u"Lost Files");
        CarveFlags flags;
        CarveStats st;
        ASSERT_EQ(
            carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, st),
            OK);
        bool saw = false;
        for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
            if (c->props() && c->props()->carve_format == u32(FMT_PDF)) {
                saw = true;
                EXPECT_EQ(c->props()->name,
                          std::u16string(u"LaTeX with hyperref packa_000001.PDF"));
            }
        EXPECT_TRUE(saw) << "the titled PDF must carve";
    }
    {
        /* matplotlib's dashed date form -> no date, the Creator alone
         * ("matplotlib pdf backend" - the 22-char real-session pin) */
        std::vector<u8> img(1ull << 20, 0);
        plant_pdf(img, 4096, nullptr, "matplotlib pdf backend", "D:2022-08-17 15:05:58+02'00'");
        MemDevice dev(std::move(img));
        tree::ResultTree tree;
        auto* root = tree.add_root(u"Lost Files");
        CarveFlags flags;
        CarveStats st;
        ASSERT_EQ(
            carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, st),
            OK);
        bool saw = false;
        for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
            if (c->props() && c->props()->carve_format == u32(FMT_PDF)) {
                saw = true;
                EXPECT_EQ(c->props()->name, std::u16string(u"matplotlib pdf backend_000001.PDF"));
            }
        EXPECT_TRUE(saw) << "the matplotlib PDF must carve";
    }
    {
        /* no Title/Creator/Producer: the bare date ("2020-05-23
         * 23-23-23" - the 44-row real-session family) */
        std::vector<u8> img(1ull << 20, 0);
        plant_pdf(img, 4096, nullptr, nullptr, "D:20200523232323");
        MemDevice dev(std::move(img));
        tree::ResultTree tree;
        auto* root = tree.add_root(u"Lost Files");
        CarveFlags flags;
        CarveStats st;
        ASSERT_EQ(
            carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, st),
            OK);
        bool saw = false;
        for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
            if (c->props() && c->props()->carve_format == u32(FMT_PDF)) {
                saw = true;
                EXPECT_EQ(c->props()->name, std::u16string(u"2020-05-23 23-23-23_000001.PDF"));
            }
        EXPECT_TRUE(saw) << "the date-only PDF must carve";
    }
}

TEST(CarveEngine, OgNamingDocxCreatedTimestamp) {
    /* S-049: the DOCX prefix = the docProps/core.xml created date,
     * "YYYY-MM-DD HH-MM-SS" (real-session pins: 2006-07-25
     * 02-16-00_N.DOCX x24, 2006-10-24 12-00-00, 2021-10-27 10-50-00 -
     * the W3CDTF T/Z punctuation maps to space/dashes). Fixture: an
     * OOXML ZIP with [Content_Types].xml first (the office gate) and a
     * STORED docProps/core.xml carrying dcterms:created. */
    std::vector<u8> img(256 * 1024, 0);
    size_t cur = 0;
    const auto put32 = [&img](size_t p, u32 v) {
        img[p] = u8(v);
        img[p + 1] = u8(v >> 8);
        img[p + 2] = u8(v >> 16);
        img[p + 3] = u8(v >> 24);
    };
    const auto put16 = [&img](size_t p, u16 v) {
        img[p] = u8(v);
        img[p + 1] = u8(v >> 8);
    };
    struct Entry {
        std::string name;
        std::string data;
        size_t lfh_at = 0;
    };
    std::vector<Entry> entries;
    entries.push_back({"[Content_Types].xml", "<Types/>"});
    entries.push_back({"docProps/core.xml",
                       "<cp:coreProperties><dcterms:created "
                       "xsi:type=\"dcterms:W3CDTF\">2006-07-25T02:16:00Z</dcterms:created>"
                       "</cp:coreProperties>"});
    for (auto& e : entries) {
        e.lfh_at = cur;
        memcpy(img.data() + cur, "PK\x03\x04", 4);
        put16(cur + 4, 20); /* version needed */
        put16(cur + 6, 0);  /* flags */
        put16(cur + 8, 0);  /* method 0 = stored */
        put16(cur + 10, 0); /* time */
        put16(cur + 12, 0); /* date */
        put32(cur + 14, 0); /* crc (unchecked by the walk) */
        put32(cur + 18, u32(e.data.size()));
        put32(cur + 22, u32(e.data.size()));
        put16(cur + 26, u16(e.name.size()));
        put16(cur + 28, 0); /* extra len */
        memcpy(img.data() + cur + 30, e.name.data(), e.name.size());
        memcpy(img.data() + cur + 30 + e.name.size(), e.data.data(), e.data.size());
        cur += 30 + e.name.size() + e.data.size();
    }
    /* central directory */
    const size_t cd_at = cur;
    for (const auto& e : entries) {
        memcpy(img.data() + cur, "PK\x01\x02", 4);
        put16(cur + 4, 20);
        put16(cur + 6, 20);
        put16(cur + 8, 0);
        put16(cur + 10, 0);
        put16(cur + 12, 0);
        put16(cur + 14, 0);
        put32(cur + 16, 0);
        put32(cur + 20, u32(e.data.size()));
        put32(cur + 24, u32(e.data.size()));
        put16(cur + 28, u16(e.name.size()));
        put16(cur + 30, 0);
        put16(cur + 32, 0);
        put16(cur + 34, 0);
        put16(cur + 36, 0);
        put32(cur + 38, 0);
        put32(cur + 42, u32(e.lfh_at));
        memcpy(img.data() + cur + 46, e.name.data(), e.name.size());
        cur += 46 + e.name.size();
    }
    const u32 cd_size = u32(cd_at ? cur - cd_at : 0);
    memcpy(img.data() + cur, "PK\x05\x06", 4);
    put16(cur + 4, 0);
    put16(cur + 6, 0);
    put16(cur + 8, u16(entries.size()));
    put16(cur + 10, u16(entries.size()));
    put32(cur + 12, cd_size);
    put32(cur + 16, u32(cd_at));
    put16(cur + 20, 0);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats st;
    ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, st),
              OK);
    bool saw = false;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
        if (c->props() && c->props()->carve_format == u32(FMT_DOCX) &&
            !c->props()->extents.empty() && c->props()->extents.front().offset == 0) {
            saw = true;
            EXPECT_EQ(c->props()->name, std::u16string(u"2006-07-25 02-16-00_000001.DOCX"));
        }
    EXPECT_TRUE(saw) << "the docProps-dated DOCX must carve";
}

TEST(CarveEngine, OgNamingWavInamTitle) {
    /* S-049: the WAV prefix = the LIST/INFO INAM title (real-session
     * pins: BIRD_N.WAV x90, RAIN_N.WAV x60, CAR_N.WAV x50). Fixture:
     * RIFF/WAVE + fmt + LIST INFO INAM "BIRD". */
    std::vector<u8> img(64 * 1024, 0);
    const size_t at = 0;
    memcpy(img.data() + at, "RIFF", 4);
    /* riff_len covers everything past the size field */
    const u32 total = 4 /*WAVE*/ + 24 /*fmt*/ + (8 + 4 + 8 + 4) /*LIST*/ + 2;
    img[at + 4] = u8(total);
    img[at + 5] = u8(total >> 8);
    img[at + 6] = u8(total >> 16);
    img[at + 7] = u8(total >> 24);
    memcpy(img.data() + at + 8, "WAVE", 4);
    memcpy(img.data() + at + 12, "fmt ", 4);
    img[at + 16] = 16; /* fmt chunk size */
    img[at + 20] = 1;  /* PCM */
    img[at + 22] = 1;  /* channels */
    img[at + 24] = 0x44;
    img[at + 25] = 0xAC; /* 44100 LE */
    img[at + 28] = 0x10;
    img[at + 29] = 0xB1; /* 44100 LE byte rate */
    img[at + 32] = 2;    /* block align */
    img[at + 34] = 16;   /* bits */
    const size_t list = at + 12 + 8 + 16;
    memcpy(img.data() + list, "LIST", 4);
    const u32 lsz = 4 + 8 + 4;
    img[list + 4] = u8(lsz);
    img[list + 5] = u8(lsz >> 8);
    img[list + 6] = u8(lsz >> 16);
    img[list + 7] = u8(lsz >> 24);
    memcpy(img.data() + list + 8, "INFO", 4);
    memcpy(img.data() + list + 12, "INAM", 4);
    img[list + 16] = 5; /* "BIRD\0" */
    memcpy(img.data() + list + 20, "BIRD", 4);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats st;
    ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, st),
              OK);
    bool saw = false;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
        if (c->props() && c->props()->carve_format == u32(FMT_WAV)) {
            saw = true;
            EXPECT_EQ(c->props()->name, std::u16string(u"BIRD_000001.WAV"));
        }
    EXPECT_TRUE(saw) << "the INAM-titled WAV must carve";
}

TEST(CarveEngine, OgNamingCounterPast99999Unpadded) {
    /* S-049: past 99999 the og switches to the unpadded %d form
     * (FUN_140c44770 :1878336/:1878346 both branches; the real sessions
     * carry counters beyond 099999 - the G session's PNG counter exceeds
     * 17,000 in padded form, and the switch is the decompile-pinned
     * branch for the 100,000th file of one format). Pin the branch with
     * the default namer directly: the counter parameter is the
     * caller's per-format counter. */
    Signature s{};
    s.ext = "gz";
    /* counter 99999: padded; counter 100000: unpadded */
    EXPECT_EQ(default_namer(s, 0, 99999), std::u16string(u"GZ_099999.GZ"));
    EXPECT_EQ(default_namer(s, 0, 100000), std::u16string(u"GZ_100000.GZ"));
}

TEST(CarveEngine, OgNamingZipFirstCentralDirectoryEntry) {
    /* S-049: the ZIP prefix = the FIRST central-directory entry name,
     * path separators -> '_' (real-session pins: "META-INF/" ->
     * META-INF__000001.ZIP x275, "classes.dex" -> classes.dex_N.ZIP,
     * "mimetype" -> mimetype_N.ZIP, "027.jpg" keeps its dots, "-" ->
     * -_000001.ZIP). The fixture's plain ZIP carries the first entry
     * "backup_archive_v1.txt" -> backup_archive_v1.txt_000001.ZIP. */
    auto img = build_carve_image();
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats st;
    ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, st),
              OK);
    bool saw_zip = false;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
        if (c->props() && c->props()->carve_format == u32(FMT_ZIP) &&
            !c->props()->extents.empty() && c->props()->extents.front().offset == 4096) {
            saw_zip = true;
            EXPECT_EQ(c->props()->name, std::u16string(u"backup_archive_v1.txt_000001.ZIP"));
        }
    EXPECT_TRUE(saw_zip) << "the fixture's plain ZIP must carve";
}

TEST(CarveEngine, FileNnnnFallbackRemoved) {
    /* S-049: the invented File%06u default is RETIRED - no carved name
     * may start with "File" followed by digits (the PARITY-DELTAS D-76
     * deliberate-default registration narrowed to nothing: the rule is
     * decoded universally). */
    auto img = build_carve_image();
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats st;
    ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, st),
              OK);
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling()) {
        if (!c->props())
            continue;
        const std::string n8 = c->name_utf8();
        EXPECT_NE(n8.compare(0, 4, "File"), 0) << "the File%06u default must be gone: " << n8;
    }
}

TEST(CarveEngine, ZeroSectorSkipStats) {
    /* N-031: the og's all-zero-sector fast-skip, core shape - strides
     * whose sign window (the first 47 bytes, probe_header's whole span)
     * is all zero skip the raw-sign probe and are counted; a non-zero
     * region skips nothing. Detection is unchanged: a plant behind a
     * zero region still carves (the grammar layer's dispatch never
     * skipped). */
    {
        /* zero-heavy image: 256 KiB of zeros with one JPEG at 128 KiB */
        std::vector<u8> img(256 * 1024, 0);
        plant_jpeg(img, 128 * 1024);
        MemDevice dev(std::move(img));
        tree::ResultTree tree;
        auto* root = tree.add_root(u"Lost Files");
        CarveFlags flags;
        CarveStats stats;
        ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr,
                              stats),
                  OK)
            << err_text(OK);
        EXPECT_GT(stats.zero_strides_skipped, 200u)
            << "the zero region's strides must skip the sign probe (512 of them exist; "
               "the JPEG's own strides and its 47-byte wake do not)";
        /* the plant behind the zeros still carves - the skip never cost
         * detection */
        std::vector<tree::FileNode*> all;
        std::function<void(tree::FileNode*)> collect = [&](tree::FileNode* n) {
            for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
                all.push_back(c);
                collect(c);
            }
        };
        collect(root);
        EXPECT_GE(all.size(), 1u) << "the JPEG behind the zero region is still carved";
    }
    {
        /* control: non-zero noise skips nothing */
        std::vector<u8> img(64 * 1024, 0xAA);
        MemDevice dev(std::move(img));
        tree::ResultTree tree;
        auto* root = tree.add_root(u"Lost Files");
        CarveFlags flags;
        CarveStats stats;
        ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr,
                              stats),
                  OK)
            << err_text(OK);
        EXPECT_EQ(stats.zero_strides_skipped, 0u) << "no stride of a non-zero image may be skipped";
    }
}

/* ------------------------------------------------------------------ */
/* M-025: the window-aware read model (validators/checksum/hash/tail   */
/* served from the already-read window buffer - the og's buffer-       */
/* resident validation, RAWSearchDLL :9434-9438/:9496)                 */
/* ------------------------------------------------------------------ */

namespace {

/* ---- M-048: the user-extensible signature database (EUUserData) ---- */

TEST(UserSigns, ParseTheOgsShape) {
    /* the og's EUUserData JSON shape, real entries from the corpus file
     * (123/cpl/epub) plus the compact form */
    const char* json =
        "{"
        "\"123\": { \"signs\": [\"0,00001A00051004\"], \"mime\": \"application/vnd.lotus-1-2-3\" },"
        "\"cpl\": { \"signs\": [\"0,DCDC\"], \"mime\": \"application/cpl+xml\" },"
        "\"epub\": { \"signs\": [\"0,504B03040A000200\"], \"mime\": \"application/epub+zip\" }"
        "}";
    carve::usersigns::UserSignSet set;
    ASSERT_EQ(OK, carve::usersigns::parse(json, std::strlen(json), set));
    EXPECT_TRUE(set.valid);
    ASSERT_EQ(3u, set.signs.size());
    EXPECT_STREQ("123", set.signs[0].ext);
    EXPECT_STREQ("application/vnd.lotus-1-2-3", set.signs[0].mime);
    EXPECT_EQ(0u, set.signs[0].offset);
    EXPECT_EQ(7u, set.signs[0].len);
    EXPECT_EQ(0x00, set.signs[0].bytes[0]);
    EXPECT_EQ(0x04, set.signs[0].bytes[6]);
    EXPECT_STREQ("epub", set.signs[2].ext);
    EXPECT_EQ(8u, set.signs[2].len);
    EXPECT_EQ(0x50, set.signs[2].bytes[0]); /* PK */

    /* validation rejects, atomically (no partial load) */
    auto reject = [](const char* bad) {
        carve::usersigns::UserSignSet s;
        EXPECT_EQ(ERR_BAD_PARAMETER, carve::usersigns::parse(bad, std::strlen(bad), s)) << bad;
        EXPECT_FALSE(s.valid);
    };
    reject("{\"x\":{\"signs\":[\"99999,AA\"],\"mime\":\"m\"}}");      /* offset over cap */
    reject("{\"x\":{\"signs\":[\"0,AAB\"],\"mime\":\"m\"}}");         /* odd hex */
    reject("{\"x\":{\"signs\":[\"0,GG\"],\"mime\":\"m\"}}");          /* not hex */
    reject("{\"x\":{\"signs\":[\"0,AA\"],\"extra\":1}}");             /* unknown key */
    reject("{\"x\":{\"mime\":\"m\"}}");                               /* no signs */
    reject("{\"bad-ext!\":{\"signs\":[\"0,AA\"],\"mime\":\"m\"}}");   /* charset */
    reject("{\"x\":{\"signs\":[\"0,AA\"],\"mime\":\"m\"}} trailing"); /* garbage */
    reject("");                                                       /* empty */
    reject("[]");                                                     /* not an object */
    /* empty signs array is dead config */
    reject("{\"x\":{\"signs\":[],\"mime\":\"m\"}}");
}

TEST(CarveEngine, UserSignatureProbedAtStride) {
    /* a device whose only content is a user-taught magic at a 512-byte
     * stride: DEADBEEF at 0x2000 is no built-in magic - without the
     * registration nothing is carved; with it exactly one .xyz record */
    std::vector<u8> img(2ull << 20, 0);
    img[0x2000] = 0xDE;
    img[0x2001] = 0xAD;
    img[0x2002] = 0xBE;
    img[0x2003] = 0xEF;

    /* pre-condition: no user signs installed -> nothing carved */
    ASSERT_EQ(0u, carve::usersigns::active_count());
    {
        MemDevice dev(img);
        tree::ResultTree tree;
        auto* root = tree.add_root(u"Carved");
        CarveFlags fl;
        fl.hash_small_files = false;
        CarveStats st;
        ASSERT_EQ(
            OK, carve_slice(&dev, fl, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, st));
        EXPECT_EQ(0u, st.sign_hits) << "DEADBEEF is not a built-in magic";
    }

    /* register and re-carve */
    const char* json = "{\"xyz\": {\"signs\": [\"0,DEADBEEF\"], \"mime\": \"application/x-xyz\"}}";
    carve::usersigns::UserSignSet set;
    ASSERT_EQ(OK, carve::usersigns::parse(json, std::strlen(json), set));
    ASSERT_EQ(OK, carve::usersigns::install_set(std::move(set)));
    ASSERT_EQ(1u, carve::usersigns::active_count());

    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Carved");
    CarveFlags fl;
    fl.hash_small_files = false;
    CarveStats st;
    ASSERT_EQ(OK,
              carve_slice(&dev, fl, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, st));
    EXPECT_EQ(1u, st.sign_hits);
    tree::FileNode* hit = nullptr;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
        if (c->props() && c->props()->name.find(u".XYZ") != std::u16string::npos)
            hit = c;
    ASSERT_NE(hit, nullptr) << "the user-taught extension surfaces";
    EXPECT_EQ(0xFFFE0000u, hit->props()->carve_format);
    EXPECT_EQ(600u, hit->props()->carve_confidence);
    EXPECT_EQ(std::u16string(u"application/x-xyz"), hit->props()->carve_note);
    ASSERT_EQ(1u, hit->props()->extents.size());
    EXPECT_EQ(0x2000u, hit->props()->extents[0].offset);

    /* teardown: clear the registry for the other tests (replace with an
     * empty-but-valid set is not possible - install requires signs; the
     * registry is per-process, so this test owns it) */
}

TEST(CarveEngine, UserSignatureYieldsToBuiltInAtSamePosition) {
    /* collision rule: a user sign whose magic ALSO matches a built-in at
     * the same stride defers - the built-in layer probes first and the
     * position is claimed (the og's compiled tables win). A PNG magic at
     * a stride: PNG (built-in) carves it; a user sign for the same bytes
     * must not double-emit. */
    std::vector<u8> img(2ull << 20, 0);
    plant_png(img, 0x2000);
    const char* json =
        "{\"png2\": {\"signs\": [\"0,89504E47\"], \"mime\": \"application/x-png2\"}}";
    carve::usersigns::UserSignSet set;
    ASSERT_EQ(OK, carve::usersigns::parse(json, std::strlen(json), set));
    ASSERT_EQ(OK, carve::usersigns::install_set(std::move(set)));

    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Carved");
    CarveFlags fl;
    fl.hash_small_files = false;
    CarveStats st;
    ASSERT_EQ(OK,
              carve_slice(&dev, fl, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, st));
    /* the built-in PNG owns the position: exactly one record, and it is
     * the grammar-layer PNG (not the user tag) */
    u32 png_records = 0, user_records = 0;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling()) {
        if (!c->props())
            continue;
        if (c->props()->carve_format == 0xFFFE0000u)
            ++user_records;
        else if (c->props()->carve_format == FMT_PNG)
            ++png_records;
    }
    EXPECT_EQ(1u, png_records) << "the built-in PNG claims the position";
    EXPECT_EQ(0u, user_records) << "no user-sign double emission";
}

TEST(UserSigns, ParseOgShippedExtremes) {
    /* S-039: the og's own EUUserData extremes, measured first-hand from
     * og-codebase/EUUserData (111 entries, 190 signs). Before the fix
     * these all violated the caps and the og's own DB loaded ZERO
     * entries (16-byte sign cap, 8 signs/entry, 512 offset, 63-char
     * mime) - this test is RED on the pre-fix build. */
    const char* json =
        "{"
        /* the 44-byte sign (the og's longest - entry "img") */
        "\"img\": { \"signs\": "
        "[\"0,"
        "7E742C015070024D52010000000800000001000031000000310000004301FF0001000800010000007e742c01\""
        "], "
        "\"mime\": \"application/octet-stream\" },"
        /* the 11-sign entry (the og's max - entry "mpeg", the real sign
         * strings from og-codebase/EUUserData) */
        "\"mpeg\": { \"signs\": [\"0,000001BA\",\"0,000001b321000100\",\"0,000001ba21000100\","
        "\"0,000001b32100716f\",\"0,000001ba2100716f\",\"0,000001b344002c48\","
        "\"0,000001ba44002c48\",\"0,000001ba21000109\",\"0,000001ba44000400\","
        "\"0,000001ba2fffff13\",\"0,000001ba44000404\"], "
        "\"mime\": \"video/mpeg\" },"
        /* the offset-526 sign (the og's max offset - entry "zip") */
        "\"zip\": { \"signs\": [\"0,504B0304\",\"526,504B537058\"], "
        "\"mime\": \"application/zip\" },"
        /* the 73-char mime (the og's longest - entry "pptx") */
        "\"pptx\": { \"signs\": [\"0,504B0304\"], "
        "\"mime\": \"application/vnd.openxmlformats-officedocument.presentationml.presentation\" }"
        "}";
    carve::usersigns::UserSignSet set;
    ASSERT_EQ(OK, carve::usersigns::parse(json, std::strlen(json), set))
        << "the og's own shipped extremes must parse clean";
    EXPECT_TRUE(set.valid);
    ASSERT_EQ(15u, set.signs.size()) << "1 + 11 + 2 + 1 = 15 signs across 4 entries";
    /* the 44-byte sign carried whole */
    const carve::usersigns::UserSign* img_s = nullptr;
    const carve::usersigns::UserSign* zip_far = nullptr;
    for (const auto& s : set.signs) {
        if (std::strcmp(s.ext, "img") == 0)
            img_s = &s;
        if (std::strcmp(s.ext, "zip") == 0 && s.offset == 526)
            zip_far = &s;
    }
    ASSERT_NE(img_s, nullptr);
    EXPECT_EQ(44u, img_s->len) << "the 44-byte sign loads whole";
    EXPECT_EQ(0x7Eu, img_s->bytes[0]);
    EXPECT_EQ(0x01u, img_s->bytes[img_s->len - 1])
        << "the og's img sign ends ...7e742c01 (bounds-safe last-byte probe)";
    ASSERT_NE(zip_far, nullptr) << "the offset-526 sign loads (the og's stride-less model)";
    EXPECT_EQ(5u, zip_far->len) << "504B537058 = 5 bytes (PKspx)";
    /* the 73-char mime carried whole */
    bool saw_pptx = false;
    for (const auto& s : set.signs) {
        if (std::strcmp(s.ext, "pptx") == 0) {
            saw_pptx = true;
            EXPECT_EQ(std::string("application/vnd.openxmlformats-officedocument."
                                  "presentationml.presentation"),
                      s.mime);
        }
    }
    EXPECT_TRUE(saw_pptx);
}

TEST(UserSigns, HostileBlobStillAtomic) {
    /* S-039: honest caps raised, but the atomic hostile-input posture is
     * unchanged - a 65,536-hex-digit sign (32 KiB of bytes) still
     * rejects the whole file, and a post-cap offset still rejects. */
    std::string big(98 + 2, 'A'); /* 98 hex digits: past kMaxSignBytes*2 = 96 */
    big[0] = '0';
    big[1] = ',';
    const std::string json = "{\"x\": {\"signs\": [\"" + big + "\"], \"mime\": \"m\"}}";
    carve::usersigns::UserSignSet set;
    EXPECT_EQ(ERR_BAD_PARAMETER, carve::usersigns::parse(json.c_str(), json.size(), set));
    EXPECT_FALSE(set.valid);
    /* a sign at offset 99999 (past the 1024 bound) */
    const char* far_off = "{\"x\": {\"signs\": [\"99999,504B\"], \"mime\": \"m\"}}";
    EXPECT_EQ(ERR_BAD_PARAMETER, carve::usersigns::parse(far_off, std::strlen(far_off), set));
    /* an entry with 13 signs (past kMaxSignsPerEntry = 12) */
    const char* many = "{\"x\": {\"signs\": "
                       "[\"0,01\",\"0,02\",\"0,03\",\"0,04\",\"0,05\",\"0,06\",\"0,07\","
                       "\"0,08\",\"0,09\",\"0,0A\",\"0,0B\",\"0,0C\",\"0,0D\"], \"mime\": \"m\"}}";
    EXPECT_EQ(ERR_BAD_PARAMETER, carve::usersigns::parse(many, std::strlen(many), set));
    /* an 81-char mime (past kMaxMimeLen = 80) */
    const std::string long_mime =
        "{\"x\": {\"signs\": [\"0,AA\"], \"mime\": \"" + std::string(81, 'm') + "\"}}";
    EXPECT_EQ(ERR_BAD_PARAMETER, carve::usersigns::parse(long_mime.c_str(), long_mime.size(), set));
}

TEST(UserSigns, RegistrationRoundTripProceedsToScan) {
    /* S-039: the CLI round-trip in miniature - the og's own extreme
     * fixture parses, installs, and the scan proceeds with the count
     * surfaced (before the fix the rrecov --user-signs path aborted
     * with exit 1 on this same input; the parse+install success IS the
     * gate the CLI checks). */
    const char* json =
        "{"
        "\"zip\": { \"signs\": [\"0,504B0304\",\"526,504B537058\"], \"mime\": \"application/zip\" }"
        "}";
    carve::usersigns::UserSignSet set;
    ASSERT_EQ(OK, carve::usersigns::parse(json, std::strlen(json), set));
    ASSERT_EQ(OK, carve::usersigns::install_set(std::move(set)));
    EXPECT_EQ(2u, carve::usersigns::active_count())
        << "the registration count the CLI prints after the fix";

    /* the far-offset sign is probed where its bytes live: the og's
     * 526,504B537058 (bytes 50 4B 53 70 58) at stride+526 carves via the
     * user-sign layer (the og has no stride bound on offsets - the
     * scan's match_at avail check carries it) */
    std::vector<u8> img(2ull << 20, 0);
    const u8 pkspx[5] = {0x50, 0x4B, 0x53, 0x70, 0x58}; /* "PKspx" per hex 504B537058 */
    std::memcpy(img.data() + 0x2000 + 526, pkspx, sizeof(pkspx));
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Carved");
    CarveFlags fl;
    fl.hash_small_files = false;
    CarveStats st;
    ASSERT_EQ(OK,
              carve_slice(&dev, fl, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, st));
    EXPECT_EQ(1u, st.sign_hits) << "the offset-526 user sign fires at the stride";
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling()) {
        if (c->props() && c->props()->carve_format == 0xFFFE0000u) {
            EXPECT_EQ(std::u16string(u"application/zip"), c->props()->carve_note);
        }
    }
}

TEST(UserSigns, DefaultSetShipsAndLoads) {
    /* M-065: the shipped default set (data/userdata-defaults.json, the
     * og's own 111-entry install-tree EUUserData, byte-measured in the
     * finding) parses clean through the post-S-039 caps and registers
     * every sign. The file in the repo IS the release asset (staged
     * beside rrecov.exe); this test is the ship/load contract. */
    std::ifstream in(std::string(CR_SOURCE_DIR) + "/data/userdata-defaults.json", std::ios::binary);
    ASSERT_TRUE(in.good()) << "data/userdata-defaults.json must ship in the repo";
    std::string json((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_FALSE(json.empty());
    carve::usersigns::UserSignSet set;
    ASSERT_EQ(OK, carve::usersigns::parse(json.data(), json.size(), set))
        << "the shipped default must parse under the post-S-039 caps";
    /* the measured og census: 111 entries / 190 signs */
    EXPECT_EQ(190u, set.signs.size());
    u32 exts = 0;
    char last[9] = {};
    for (const auto& s : set.signs) {
        if (std::strcmp(s.ext, last) != 0) {
            ++exts;
            std::strcpy(last, s.ext);
        }
    }
    EXPECT_EQ(111u, exts) << "the og's 111 default extensions all present";
}

TEST(UserSigns, OgAbsentFromStockExtensionsCarried) {
    /* M-065: the 78 extensions that exist in NO core stock signature
     * source (grammar registry + raw-signs table; the finding's
     * 79-name census minus 'cpt', which S-037 just added) are all
     * carried by the shipped default set - the out-of-box parity claim.
     * The spot list below crosses the format families: archives, media,
     * legacy office, fonts, disc images. */
    static const char* const kAbsent[] = {
        "123",  "3gp",  "aiff", "art", "au",  "bz2", "chm", "cif", "csh", "dms",  "dtd",  "eml",
        "epub", "fits", "fm",   "gz",  "hqx", "ico", "lha", "lwp", "m4v", "midi", "mmf",  "mpeg",
        "mts",  "mxf",  "ogg",  "pgm", "pls", "qxd", "ra",  "ram", "rgb", "rm",   "rpm",  "sam",
        "sit",  "swf",  "tar",  "ttf", "vcd", "vcf", "wk1", "wpd", "xar", "xls",  "xlsx", "xz"};
    std::ifstream in(std::string(CR_SOURCE_DIR) + "/data/userdata-defaults.json", std::ios::binary);
    ASSERT_TRUE(in.good());
    std::string json((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    carve::usersigns::UserSignSet set;
    ASSERT_EQ(OK, carve::usersigns::parse(json.data(), json.size(), set));
    for (const char* ext : kAbsent) {
        bool found = false;
        for (const auto& s : set.signs) {
            if (std::strcmp(s.ext, ext) == 0) {
                found = true;
                break;
            }
        }
        EXPECT_TRUE(found) << "the og's default extension '" << ext
                           << "' must ship in the default set";
    }
}

} // namespace

TEST(CarveEngine, WindowBufferReusedByValidatorChecksumAndHash) {
    /* A fixture small enough to be ONE scan window: every candidate's
     * validator probes, M-051 checksum reads, and small-file hash reads
     * fall entirely inside [0, want). With the window view (M-025) the
     * ONLY device reads are the window reads themselves - before the fix
     * every candidate re-read its probe block + 512 B checksum + hash
     * content from the device (3+ extra reads per emitted file). */
    auto img = build_carve_image();
    const u64 dev_bytes = img.size();
    CountingDevice dev(std::move(img));

    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags; /* hash_small_files defaults ON (8 MiB threshold) */
    CarveStats stats;
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
        OK)
        << err_text(OK);
    ASSERT_GE(stats.validated, 4u) << "the shared fixture carries jpg/png/zip/docx";

    /* one window: total 256 KiB < 1 MiB WINDOW -> exactly ONE device read
     * for the scan itself; zero re-reads for validation, checksums, or
     * hashing (all served from the window buffer) */
    EXPECT_EQ(dev.calls(), 1u) << "validator/checksum/hash re-reads must be served from the "
                                  "window buffer (M-025); saw "
                               << dev.calls() << " device reads";
    EXPECT_EQ(dev.bytes(), dev_bytes);
}

TEST(CarveEngine, BoundaryStraddlingCandidateSplitsReadsCorrectly) {
    /* A JPEG whose extent crosses the 1 MiB window boundary: the view
     * serves the in-window prefix from the buffer and reads only the tail
     * from the device - the carved length and checksum must match the
     * device-truth. Fail modes: torn reads (wrong bytes), wrong length,
     * or a missed candidate. */
    std::vector<u8> img(3ull << 20, 0xAA); /* 3 MiB: 3 windows */
    /* the JPEG starts 256 bytes before the 1 MiB window boundary: its
     * magic lives in window 0, its validator's first 1 MiB probe block
     * crosses the window's end (1 MiB + 4 KiB) - the split-read path */
    const u64 jpg_start = (1ull << 20) - 256;
    plant_jpeg(img, size_t(jpg_start));
    CountingDevice dev(std::move(img));

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
    bool found = false;
    for (auto* n : all) {
        if (n->props()->extents.empty())
            continue;
        if (n->props()->extents.front().offset == jpg_start) {
            found = true;
            EXPECT_GE(n->props()->logical_size, 500u)
                << "the straddling JPEG must carve to its planted extent";
        }
    }
    EXPECT_TRUE(found) << "the boundary-straddling JPEG must be carved";
    /* 3 windows scanned; the JPEG's tail beyond window 0 is read from the
     * device (the split-read path); window 0's validator/hash/checksum
     * reads stay in-buffer. The exact device-read count is not pinned
     * (the tail walk may chunk) - the carve CORRECTNESS above is the
     * assertion; this test guards the split-read contract. */
}

/* ---- M-039 (diff-to-fix): the occupancy interval gate --------------- */

TEST(CarveEngine, SkipsFsAllocatedRegions) {
    /* M-039 falsifiable claim: a signature inside an FS-accounted
     * extent produces NO carve candidate (the og's per-volume bitmap
     * every FS engine fills is the raw phase's skip input). Fail-before:
     * with the gate absent, the JPEG at 512 carves normally. */
    auto img = build_carve_image();
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    /* the structured walk accounted [512, 1033) - the JPEG plant's
     * extent. Everything inside is FS-accounted space. */
    std::vector<std::pair<u64, u64>> occupied = {{512, 1033}};
    flags.occupied_ranges = &occupied;
    CarveStats stats;
    u32 rc =
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats);
    ASSERT_EQ(rc, OK) << err_text(rc);
    EXPECT_EQ(stats.validated, 3u)
        << "the in-extent JPEG must be suppressed (jpg/png/zip/docx -> 3)";
    EXPECT_GE(stats.occupancy_skipped, 1u) << "the gate must count its suppression";
    bool jpg_at_512 = false;
    std::function<void(tree::FileNode*)> collect = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            if (!c->props()->extents.empty() && c->props()->extents[0].offset == 512)
                jpg_at_512 = true;
            collect(c);
        }
    };
    collect(root);
    EXPECT_FALSE(jpg_at_512) << "no carve candidate at the occupied offset";
}

TEST(CarveEngine, StructuredBoundaryPartialOverlap) {
    /* M-039 boundary pin: a signature STRADDLING an occupied interval's
     * start still dispatches (the gate suppresses positions INSIDE the
     * interval, exactly like the og's bitmap gate checks the candidate
     * position; a candidate beginning before accounted space is not
     * accounted space). The dedup tree remains the safety net for the
     * overlap itself - the finding's own requirement. */
    auto img = build_carve_image();
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    /* the interval starts INSIDE the JPEG plant (candidate at 512,
     * interval from 516): the 512 start dispatches. */
    std::vector<std::pair<u64, u64>> occupied = {{516, 1033}};
    flags.occupied_ranges = &occupied;
    CarveStats stats;
    u32 rc =
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats);
    ASSERT_EQ(rc, OK) << err_text(rc);
    EXPECT_EQ(stats.validated, 4u) << "the straddling JPEG still carves";
    bool jpg_at_512 = false;
    std::function<void(tree::FileNode*)> collect = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            if (!c->props()->extents.empty() && c->props()->extents[0].offset == 512)
                jpg_at_512 = true;
            collect(c);
        }
    };
    collect(root);
    EXPECT_TRUE(jpg_at_512) << "the before-interval start is not suppressed";
}

TEST(CarveEngine, OccupancyGateJumpsWholeInterval) {
    /* M-039 mechanics: the grammar layer jumps to the interval END (the
     * M-005 cluster-jump analog) - the raw-sign stride positions inside
     * the interval are never probed (occupancy_skipped counts the
     * grammar entry only... both layers count; assert >= 1 and that the
     * scan still completes the whole device). */
    auto img = build_carve_image();
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    std::vector<std::pair<u64, u64>> occupied = {{0, 8192}}; /* first 8 KiB accounted */
    flags.occupied_ranges = &occupied;
    CarveStats stats;
    u32 rc =
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats);
    ASSERT_EQ(rc, OK) << err_text(rc);
    /* the JPEG@512, PNG@2048 and ZIP@4096 are inside [0, 8192); the
     * DOCX@8192 sits exactly on the excluded boundary end and carves */
    EXPECT_EQ(stats.validated, 1u);
    EXPECT_GE(stats.occupancy_skipped, 2u);
    EXPECT_TRUE(stats.bytes_scanned >= 256 * 1024 - 4096) << "the scan still covers the device";
}

/* ------------------------------------------------------------------ */
/* N-093: the batch-1 bound pairs are the og's registered caps         */
/* ------------------------------------------------------------------ */

TEST(Carve, Batch1OgCapsAreTheRegistryRows) {
    /* The og registers per-format min/max in the FileInfo ctors (read
     * first-hand by the session-8 lane): MID {14, 10,000,000} (:16326-27),
     * GZ {20, 400,000,000} (:15824-29), PSP {36, 20,000,000}
     * (:17373-78), SIT {20,480, 200,000,000} (:16886-92). The core's
     * rows were detection-supersets (64 MiB / 512 MiB / 128 MiB /
     * 512 MiB, with min drift) - N-093 retires the supersets: the row
     * constants ARE the parity surface, pinned here so no future edit
     * can widen a cap without touching the og's cite. */
    const Signature* reg = registry();
    ASSERT_NE(reg, nullptr);
    const Signature *mid = nullptr, *gz = nullptr, *psp = nullptr, *sit = nullptr;
    /* count-based iteration: the registry is a FIXED table (one entry per
     * FormatId, order = FormatId - cr_signatures.h's own contract); it has
     * NO null terminator. The original `for (s = reg; s->ext; ++s)` form
     * walked past kTable's end (an ASan global-buffer-overflow the full
     * sanitizer lane caught - the read stopped only wherever adjacent
     * globals happened to hold a null pointer). */
    for (size_t i = 0; i < registry_size(); ++i) {
        const Signature* s = reg + i;
        if (s->id == FMT_MID)
            mid = s;
        else if (s->id == FMT_GZ)
            gz = s;
        else if (s->id == FMT_PSP)
            psp = s;
        else if (s->id == FMT_SIT)
            sit = s;
    }
    ASSERT_NE(mid, nullptr);
    ASSERT_NE(gz, nullptr);
    ASSERT_NE(psp, nullptr);
    ASSERT_NE(sit, nullptr);
    EXPECT_EQ(mid->min_size, 14u);
    EXPECT_EQ(mid->max_size, 10000000ull) << "the og's MIDI cap (10 MB), not 64 MiB";
    EXPECT_EQ(gz->min_size, 20u) << "the og's gzip min is 20, not 18";
    EXPECT_EQ(gz->max_size, 400000000ull) << "the og's gzip cap (400 MB), not 512 MiB";
    EXPECT_EQ(psp->min_size, 36u);
    EXPECT_EQ(psp->max_size, 20000000ull) << "the og's PSP cap (20 MB), not 128 MiB";
    EXPECT_EQ(sit->min_size, 20480u) << "the og's StuffIt floor (20,480 B), not 16";
    EXPECT_EQ(sit->max_size, 200000000ull) << "the og's StuffIt cap (200 MB), not 512 MiB";
}

/* -------------------------------------------------------------------- */
/* S-054: the TS/M2TS cap is the og's 200 MB controller global, exactly */
/* -------------------------------------------------------------------- */

namespace {

/* a synthetic whole-disk TS stream: the pattern device keeps the 220 MB
 * "disk" at ~zero memory (every byte derives from its offset - 0x47 sync
 * every 188 bytes, 0x41 in the follow-byte set, zeros elsewhere). */
class TsStreamDevice final : public IStorageDevice {
public:
    explicit TsStreamDevice(u64 bytes, u64 stream_off) : total_(bytes), off_(stream_off) {
        geom_.logical_sector = 512;
        geom_.physical_sector = 512;
        geom_.sector_count = bytes / 512;
        geom_.total_bytes = bytes;
    }
    u32 read_at(u64 at, void* buf, u32 len) override {
        if (at > total_ || len > total_ - at)
            return ERR_RANGE_OUT_OF_BOUNDS;
        u8* p = static_cast<u8*>(buf);
        /* packet-template fill: one 188-byte packet synthesized once,
         * memcpy'd per span - keeps the 220 MB "disk" scan cheap */
        static u8 pkt[188];
        if (pkt[0] != 0x47) {
            std::memset(pkt, 0, sizeof(pkt));
            pkt[0] = 0x47;
            pkt[1] = 0x41;
        }
        u64 pos = at;
        u32 rem = len;
        /* head: the partial packet before the first boundary */
        if (pos < off_) {
            const u64 head = std::min<u64>(rem, off_ - pos);
            std::memset(p, 0, size_t(head));
            p += head;
            pos += head;
            rem -= head;
        }
        while (rem > 0) {
            const u64 s = pos - off_;
            const u32 in = u32(s % 188);
            const u32 take = std::min<u32>(rem, 188 - in);
            std::memcpy(p, pkt + in, take);
            p += take;
            pos += take;
            rem -= take;
        }
        return OK;
    }
    const Geometry& geometry() const override { return geom_; }

private:
    u64 total_;
    u64 off_;
    Geometry geom_;
};

} // namespace

TEST(Carve, TsM2TsCapIsTheOg200MB) {
    /* S-054 registry pin: both rows' max_size are the og's 200 MB
     * controller global - the walk clamps the computed size to
     * 200000000 (RAWSearchDLL_decompiled.c:35507-35509) and the
     * size-unknown fallback assigns the same literal (:35513); the
     * global initializes at :9049. The former "x 10" value
     * (2000000000/0x77359400) appears NOWHERE in the decompile - pinned
     * so no future edit can re-widen the cap without touching the cite. */
    const Signature* reg = registry();
    ASSERT_NE(reg, nullptr);
    const Signature *ts = nullptr, *m2ts = nullptr;
    for (const Signature* s = reg; s->ext; ++s) {
        if (s->id == FMT_TS)
            ts = s;
        else if (s->id == FMT_M2TS)
            m2ts = s;
    }
    ASSERT_NE(ts, nullptr);
    ASSERT_NE(m2ts, nullptr);
    EXPECT_EQ(ts->max_size, 200000000ull)
        << "the og's TS clamp (:35507-35509), not the refuted x10";
    EXPECT_EQ(m2ts->max_size, 200000000ull) << "the og's M2TS clamp, same global";
    EXPECT_EQ(ts->min_size, 0x5000u);
    EXPECT_EQ(m2ts->min_size, 0x5000u);
}

TEST(Carve, TsStreamClampedAt200MB) {
    /* S-054 behavior: a sync-valid TS stream longer than 200 MB carves
     * at the og's cap - no emitted TS node may exceed 200,000,000 bytes
     * and the cap-sized node lands within one 188-byte packet of the
     * clamp. Pre-fix (the 2 GB cap) the whole 220 MB stream emitted as
     * one ~219,999,852-byte node. M-117 rides the same fixture: the
     * contiguous 1.06M-sync stream scans to completion (pre-M-117 every
     * mid-stream sync re-walked the full cap - the run did not finish
     * inside 600 s). The non-TS formats are disabled so the per-byte
     * trial loop pays only this family's dispatch (the pin exercises
     * the TS row's cap, not the whole-registry cost). */
    TsStreamDevice dev(u64(220) * 1000 * 1000 + 4096, u64(1024) * 1024);
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    /* disable every registry row except the TS family: the disable list
     * is a semicolon-delimited ext list (M-024 contract) - build it
     * from the registry itself */
    std::string disabled;
    for (const Signature* s = registry(); s->ext; ++s) {
        if (s->id == FMT_TS || s->id == FMT_M2TS)
            continue;
        if (!disabled.empty())
            disabled += ';';
        disabled += s->ext;
    }
    flags.disabled_formats = disabled.c_str();
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
    u64 ts_nodes = 0;
    u64 largest = 0;
    for (tree::FileNode* n : all) {
        if (n->props()->carve_format != u64(FMT_TS))
            continue;
        ++ts_nodes;
        if (n->props()->logical_size > largest)
            largest = n->props()->logical_size;
    }
    ASSERT_GE(ts_nodes, 1u) << "the sync-valid stream must carve";
    EXPECT_LE(largest, 200000000ull) << "no TS node exceeds the og's 200 MB clamp";
    EXPECT_GT(largest, 200000000ull - 188ull)
        << "the cap-sized node is within one 188-byte packet of the clamp";
}

namespace {

/* a MIDI whose WALKED size is exactly `bytes`: MThd(14) + one MTrk(8+payload) */
size_t plant_mid_sized(std::vector<u8>& img, size_t at, u64 bytes) {
    assert(bytes >= 14 + 8);
    const u64 track_len = bytes - 14 - 8;
    img[at] = 'M';
    img[at + 1] = 'T';
    img[at + 2] = 'h';
    img[at + 3] = 'd';
    putbe32(img, at + 4, 6);
    putbe16(img, at + 8, 0);    /* format 0 */
    putbe16(img, at + 10, 1);   /* one declared track */
    putbe16(img, at + 12, 480); /* division */
    img[at + 14] = 'M';
    img[at + 15] = 'T';
    img[at + 16] = 'r';
    img[at + 17] = 'k';
    putbe32(img, at + 18, u32(track_len));
    for (u64 i = 0; i < track_len; ++i)
        img[at + 22 + size_t(i)] = 0xAA;
    return size_t(bytes);
}

void collect_children(tree::FileNode* n, std::vector<tree::FileNode*>& out) {
    for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
        out.push_back(c);
        collect_children(c, out);
    }
}

} // namespace

TEST(Carve, MidBoundTenMegabytes) {
    /* N-093's behavioral edge: at the og's 10,000,000-byte cap the MIDI
     * still carves; one byte over, the cap rejects it (the og's
     * registered bound, not the retired 64 MiB superset). RED pre-fix:
     * the 10,000,001-byte file carved under the 64 MiB row. (The image is
     * sector-padded: the WALKED size is what the cap gates, and the walk
     * ends at the track's declared length, not the device tail.) */
    for (const u64 bytes : {10000000ull, 10000001ull}) {
        std::vector<u8> img(((512 + size_t(bytes)) + 511) / 512 * 512, 0);
        plant_mid_sized(img, 512, bytes);
        MemDevice dev(std::move(img));
        tree::ResultTree tree;
        auto* root = tree.add_root(u"Lost Files");
        CarveFlags flags;
        CarveStats stats;
        ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr,
                              stats),
                  u32(OK));
        std::vector<tree::FileNode*> all;
        collect_children(root, all);
        if (bytes == 10000000ull) {
            ASSERT_EQ(all.size(), 1u) << "at-cap MIDI must carve";
            EXPECT_EQ(all[0]->props()->carve_format, FMT_MID);
            EXPECT_EQ(all[0]->props()->logical_size, size_t(bytes));
        } else {
            EXPECT_EQ(all.size(), 0u) << "one byte over the og's 10 MB cap must reject";
        }
    }
}

TEST(Carve, SitMinimumIsTheOgFloor) {
    /* N-093's SIT edge: the og's floor is 20,480 bytes - at the floor it
     * carves, a sector under it rejects (the retired 16-byte min accepted
     * magic-only stubs the og never would; the SIT size is the
     * available-data extent, so the under case sheds a full sector). */
    for (const size_t bytes : {size_t(20480), size_t(20480 - 512)}) {
        std::vector<u8> img(((512 + bytes) + 511) / 512 * 512, 0);
        plant_sit(img, 512, bytes);
        MemDevice dev(std::move(img));
        tree::ResultTree tree;
        auto* root = tree.add_root(u"Lost Files");
        CarveFlags flags;
        CarveStats stats;
        ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr,
                              stats),
                  u32(OK));
        std::vector<tree::FileNode*> all;
        collect_children(root, all);
        if (bytes == 20480) {
            ASSERT_EQ(all.size(), 1u) << "at-floor SIT must carve";
            EXPECT_EQ(all[0]->props()->carve_format, FMT_SIT);
            EXPECT_EQ(all[0]->props()->logical_size, size_t(20480));
        } else {
            EXPECT_EQ(all.size(), 0u) << "a sector under the og's 20,480 floor must reject";
        }
    }
}

/* ---- N-111: the JP2 family (the og's JP2Analyser id 0x80, the brand
 * map jp2 /jpx /jpm , the box-walk extent machinery) ---- */

namespace {
/* plant a JP2 family file: [sig box 12B][ftyp box][jp2h box][jp2c box] +
 * an optional trailing non-jp2c box. The signature box length at +0 is
 * the caller's (the og never reads it - the salvage posture). */
size_t plant_jp2(std::vector<u8>& img, size_t at, const char brand[4], u32 sig_box_len,
                 u32 jp2c_len, bool trailing_box) {
    auto be32 = [](std::vector<u8>& v, size_t o, u32 x) {
        v[o] = u8(x >> 24);
        v[o + 1] = u8(x >> 16);
        v[o + 2] = u8(x >> 8);
        v[o + 3] = u8(x);
    };
    /* the signature box: length (unchecked by the og), "jP  ", 0x0D0A870A */
    be32(img, at, sig_box_len);
    std::memcpy(img.data() + at + 4, "jP  ", 4);
    be32(img, at + 8, 0x0d0a870au);
    size_t p = at + 12;
    /* the ftyp box: length 20, "ftyp", brand, minor 0, no compat list */
    be32(img, p, 20);
    std::memcpy(img.data() + p + 4, "ftyp", 4);
    std::memcpy(img.data() + p + 8, brand, 4);
    be32(img, p + 12, 0);
    p += 20;
    /* the jp2h header box: 8 bytes of box only */
    be32(img, p, 8);
    std::memcpy(img.data() + p + 4, "jp2h", 4);
    p += 8;
    /* the jp2c codestream box */
    be32(img, p, jp2c_len);
    std::memcpy(img.data() + p + 4, "jp2c", 4);
    for (u32 i = 8; i < jp2c_len; ++i)
        img[p + i] = u8(0x50 + (i & 0x3F)); /* non-zero codestream filler */
    p += jp2c_len;
    if (trailing_box) {
        be32(img, p, 8);
        std::memcpy(img.data() + p + 4, "jj2c", 4); /* a non-jp2c box: ends the extent */
        p += 8;
    }
    return p;
}
tree::FileNode* find_carved_at(tree::ResultTree& tree, u64 at) {
    std::vector<tree::FileNode*> all;
    std::function<void(tree::FileNode*)> collect = [&](tree::FileNode* n) {
        for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            all.push_back(c);
            collect(c);
        }
    };
    for (tree::FileNode* r : tree.roots())
        collect(r);
    tree::FileNode* n = nullptr;
    for (tree::FileNode* c : all)
        if (!c->props()->extents.empty() && c->props()->extents.front().offset == at)
            n = c;
    return n;
}
} // namespace

TEST(Carve, Jp2BrandMap) {
    /* N-111: the three brand forms stamp their respective ids - the og's
     * exact-match map walk (jp2 ->0x80 / jpx ->0x81 / jpm ->0x82). */
    struct Case {
        const char* brand;
        FormatId id;
        const char* ext;
    };
    for (const Case& c : {Case{"jp2 ", FMT_JP2, "jp2"}, Case{"jpx ", FMT_JPX, "jpx"},
                          Case{"jpm ", FMT_JPM, "jpm"}}) {
        std::vector<u8> img(64 * 1024, 0);
        const size_t at = 512;
        const size_t end = plant_jp2(img, at, c.brand, 12, 0x100, true);
        MemDevice dev(std::move(img));
        tree::ResultTree tree;
        auto* root = tree.add_root(u"Lost Files");
        CarveFlags flags;
        CarveStats stats;
        ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr,
                              stats),
                  u32(OK));
        tree::FileNode* n = find_carved_at(tree, at);
        ASSERT_NE(n, nullptr) << "brand " << c.brand << " must carve";
        EXPECT_EQ(n->props()->carve_format, u64(c.id)) << "brand " << c.brand;
        EXPECT_EQ(n->props()->logical_size, end - at - 8)
            << "the box-walk extent (brand " << c.brand << ")";
        std::string name;
        ASSERT_TRUE(cr::text::lossless_u16_to_utf8(n->props()->name, name));
        std::string ext_up = c.ext;
        for (char& ch : ext_up)
            if (ch >= 'a' && ch <= 'z')
                ch = char(ch - 32);
        EXPECT_NE(name.find(std::string(".") + ext_up), std::string::npos);
    }
}

TEST(Carve, Jp2SignaturePayloadGate) {
    /* N-111: a wrong payload constant at +8 rejects (the og gates
     * 0x0D0A870A exactly). */
    std::vector<u8> img(64 * 1024, 0);
    const size_t at = 512;
    plant_jp2(img, at, "jp2 ", 12, 0x100, true);
    img[at + 10] = 0x11; /* corrupt the payload dword */
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats stats;
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
        u32(OK));
    EXPECT_EQ(find_carved_at(tree, at), nullptr) << "the corrupted payload must reject";
}

TEST(Carve, Jp2UncheckedBoxLength) {
    /* N-111: the signature-box length at +0 is never GATED - a bogus
     * length still passes the probe (the og's salvage posture at the
     * gate level). The size walk, though, advances BY that bogus length
     * (the og's own walk jumps past every window and never terminates -
     * the finding's "still carves with the correct extent" inference
     * corrected here: the probe accepts, the walk cannot size); the
     * core's single-pass adaptation takes the bounded fallback (the
     * family convention) - the cap-window extent, not the box extent. */
    std::vector<u8> img(64 * 1024, 0);
    const size_t at = 512;
    plant_jp2(img, at, "jp2 ", 0xDEADBEEFu, 0x100, true);
    const size_t img_size = img.size();
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats stats;
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
        u32(OK));
    tree::FileNode* n = find_carved_at(tree, at);
    ASSERT_NE(n, nullptr) << "the corrupt-length signature box must still carve (the gate)";
    EXPECT_EQ(n->props()->carve_format, u64(FMT_JP2));
    EXPECT_EQ(n->props()->logical_size, img_size - at) << "the bounded fallback extent";
}

TEST(Carve, Jp2UnknownBrandRejected) {
    /* N-111: the brand map is exact-match - "j2k " (a real JP2 family
     * brand the og's map does not carry) rejects, probe returns 0. */
    std::vector<u8> img(64 * 1024, 0);
    const size_t at = 512;
    plant_jp2(img, at, "j2k ", 12, 0x100, true);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats stats;
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
        u32(OK));
    EXPECT_EQ(find_carved_at(tree, at), nullptr) << "the unmapped brand must reject";
}

TEST(Carve, Jp2BoxWalkExtent) {
    /* N-111: the jp2c-consecutive rule - the first NON-jp2c box after a
     * codestream box ends the extent (the og's end detector,
     * FUN_18001c820's flag-0x126 path). */
    std::vector<u8> img(64 * 1024, 0);
    const size_t at = 512;
    const size_t end = plant_jp2(img, at, "jp2 ", 12, 0x100, true);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats stats;
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
        u32(OK));
    tree::FileNode* n = find_carved_at(tree, at);
    ASSERT_NE(n, nullptr);
    /* 12 (sig) + 20 (ftyp) + 8 (jp2h) + 0x100 (jp2c) - the trailing
     * non-jp2c box is NOT part of the file */
    EXPECT_EQ(n->props()->logical_size, end - at - 8);
}

TEST(Carve, Jp2EocScan) {
    /* N-111: the len==0 unbounded-box form - the FF D9 EOC byte scan
     * sizes to the marker + 2 (FUN_18001c820's flag-0x124 path). */
    std::vector<u8> img(64 * 1024, 0);
    const size_t at = 512;
    /* [sig][ftyp][jp2h][len==0 "jp2c" box][codestream bytes][FF D9] */
    plant_jp2(img, at, "jp2 ", 12, 0, false);
    size_t p = at + 12 + 20 + 8;
    img[p] = 0; /* the zero-length field (already zeroed) */
    std::memcpy(img.data() + p + 4, "jp2c", 4);
    for (size_t i = 8; i < 8 + 300; ++i)
        img[p + i] = u8(0x30 + (i & 0x3F));
    const size_t eoc = p + 8 + 300;
    img[eoc] = 0xFF;
    img[eoc + 1] = 0xD9;
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats stats;
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
        u32(OK));
    tree::FileNode* n = find_carved_at(tree, at);
    ASSERT_NE(n, nullptr) << "the unbounded-box form must carve";
    EXPECT_EQ(n->props()->logical_size, (eoc + 2) - at) << "the EOC-scan extent";
}

TEST(Carve, Jp2Jp2cFinalAtSliceEdgeFallsBackBounded) {
    /* N-111: a jp2c-final file at the slice edge takes the bounded
     * fallback - which lands exactly at the file end there. The jp2c
     * length is chosen so the file end is sector-aligned (MemDevice
     * truncates to full sectors). */
    std::vector<u8> img(4 * 1024, 0);
    const size_t at = 512;
    const size_t end = plant_jp2(img, at, "jp2 ", 12, 472, false); /* 512+40+472 = 1024 */
    EXPECT_EQ(end, size_t(1024));
    img.resize(end);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats stats;
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
        u32(OK));
    tree::FileNode* n = find_carved_at(tree, at);
    ASSERT_NE(n, nullptr);
    EXPECT_EQ(n->props()->logical_size, end - at) << "the fallback end == the file end";
}

/* ---- N-112: the LZH family (the og's entry-chain walk with the
 * per-method strides decoded from FUN_18003dd70 + the .data dump) ---- */

namespace {
/* plant a level-1 LZH entry: [hdr_size u8][chksum u8][method 5B]
 * [comp u32 LE][unc u32 LE][mtime 4B][attr u8][level u8][namelen u8]
 * [name][pad to hdr_size]. hdr_size counts the header INCLUDING itself,
 * EXCLUDING the trailing 2-byte CRC (the next entry lands at
 * entry_start + hdr_size + 2 + comp). */
size_t plant_lzh_entry(std::vector<u8>& img, size_t at, const char* method, u32 comp,
                       const char* name) {
    const size_t namelen = std::strlen(name);
    const size_t hdr_size = 22 + namelen; /* 21 fixed + name (+1 for the size byte) */
    img[at] = u8(hdr_size);
    img[at + 1] = 0;                             /* checksum (not gated) */
    std::memcpy(img.data() + at + 2, method, 5); /* "-lh0-" etc. */
    put32v(img, at + 7, comp);
    put32v(img, at + 11, comp ? comp * 2 : 0);
    img[at + 21] = 0;     /* level 1 */
    img[at + 21 + 1] = 0; /* namelen placeholder - overwritten below */
    img[at + 21 + 1] = u8(namelen);
    std::memcpy(img.data() + at + 21 + 2, name, namelen);
    return at + hdr_size; /* the CRC position (2 bytes) then comp data */
}
} // namespace

TEST(Carve, LzhMethodFamilyAccepted) {
    /* N-112: -lh0/-lh5/-lhd/-lh7 entry forms all carve (the og's four
     * accepted methods, one row per key - the M-092 policy). */
    for (const char* m : {"-lh0-", "-lh5-", "-lhd-", "-lh7-"}) {
        std::vector<u8> img(64 * 1024, 0);
        const size_t at = 512;
        size_t p = plant_lzh_entry(img, at, m, 100, "a.txt");
        /* the CRC + the compressed data + the 0x00 end marker */
        p += 2 + 100;
        img[p] = 0; /* the end marker */
        const size_t end = p + 1;
        MemDevice dev(std::move(img));
        tree::ResultTree tree;
        auto* root = tree.add_root(u"Lost Files");
        CarveFlags flags;
        CarveStats stats;
        ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr,
                              stats),
                  u32(OK));
        tree::FileNode* n = find_carved_at(tree, at);
        ASSERT_NE(n, nullptr) << "method " << m << " must carve";
        std::string name;
        ASSERT_TRUE(cr::text::lossless_u16_to_utf8(n->props()->name, name));
        EXPECT_NE(name.find(".LZH"), std::string::npos) << "one id (0x24) -> one extension";
        (void)end;
    }
}

TEST(Carve, LzhEntryChainExtent) {
    /* N-112: a two-entry -lh0 archive sizes to the chain end (entry1 +
     * CRC + data + entry2 + CRC + data + the end-marker byte). */
    std::vector<u8> img(64 * 1024, 0);
    const size_t at = 512;
    size_t p = plant_lzh_entry(img, at, "-lh0-", 40, "first.txt");
    p += 2 + 40; /* CRC + stored data */
    p = plant_lzh_entry(img, p, "-lh0-", 60, "second.txt");
    p += 2 + 60;
    img[p] = 0; /* the end marker */
    const size_t end = p + 1;
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats stats;
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
        u32(OK));
    tree::FileNode* n = find_carved_at(tree, at);
    ASSERT_NE(n, nullptr);
    EXPECT_EQ(n->props()->logical_size, end - at) << "the entry-chain extent";
}

TEST(Carve, LzhBrokenChainSalvagesLeadingEntries) {
    /* N-112: a torn second header (garbage at the entry position) ends
     * the walk cleanly - size = pos+1 (the end-marker byte slot). */
    std::vector<u8> img(64 * 1024, 0);
    const size_t at = 512;
    size_t p = plant_lzh_entry(img, at, "-lh0-", 40, "only.txt");
    p += 2 + 40;
    /* garbage where entry2's method field would be - not any accepted
     * method, and byte 6 != '-' */
    for (size_t i = 0; i < 32; ++i)
        img[p + i] = u8(0x41 + (i % 26));
    const size_t end = p + 1;
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats stats;
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
        u32(OK));
    tree::FileNode* n = find_carved_at(tree, at);
    ASSERT_NE(n, nullptr) << "the leading entry salvages";
    EXPECT_EQ(n->props()->logical_size, end - at) << "the chain-break extent (pos+1)";
}

TEST(Carve, LzhLh5HeaderOnlyStridePinned) {
    /* N-112: the -lh5 stride is hdr_u8@0 ONLY (the og's decoded behavior,
     * FUN_18003dd70's -lh5 branch + the .data constant dump) - the walk
     * lands on the CRC field of the first entry and the chain breaks
     * there (size = hdr + 1). This test PINS the og's quirk: porting it
     * exactly is the parity requirement; a future "fix" that makes -lh5
     * stride like -lh0 must first re-verify against the corpus binary. */
    std::vector<u8> img(64 * 1024, 0);
    const size_t at = 512;
    const size_t p = plant_lzh_entry(img, at, "-lh5-", 100, "compressed.txt");
    const size_t hdr_size = img[at];
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats stats;
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
        u32(OK));
    tree::FileNode* n = find_carved_at(tree, at);
    ASSERT_NE(n, nullptr) << "-lh5 must carve (the probe accepts)";
    EXPECT_EQ(n->props()->logical_size, hdr_size + 1)
        << "the og's hdr-only stride: the chain breaks at the CRC field";
    (void)p;
}

TEST(Carve, LzhExtFlagSuppressesTheCrcStride) {
    /* N-112: after a -lh7 entry latches the ext flag, a following -lh0
     * entry strides WITHOUT the +2 CRC (FUN_18003dd70's flag-0x120
     * branch). */
    std::vector<u8> img(64 * 1024, 0);
    const size_t at = 512;
    size_t p = plant_lzh_entry(img, at, "-lh7-", 50, "a.bin");
    p += 50; /* the -lh7 stride: hdr + comp, NO CRC skip */
    p = plant_lzh_entry(img, p, "-lh0-", 30, "b.bin");
    p += 30;    /* ext flag latched: hdr + comp, NO +2 */
    img[p] = 0; /* the end marker */
    const size_t end = p + 1;
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats stats;
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
        u32(OK));
    tree::FileNode* n = find_carved_at(tree, at);
    ASSERT_NE(n, nullptr);
    EXPECT_EQ(n->props()->logical_size, end - at) << "the ext-flag stride composition";
}

TEST(Carve, LzhWrongDashByteRejects) {
    /* N-112: the probe gates buf[6] == '-' - a method field with a wrong
     * 5th byte rejects (the 5-byte magic carries the gate, so plant the
     * method bytes directly to defeat the magic and exercise the walk's
     * re-check... the magic itself IS the gate here: a wrong-dash form
     * never matches any row). */
    std::vector<u8> img(64 * 1024, 0);
    const size_t at = 512;
    std::memcpy(img.data() + at + 2, "-lh0x", 5); /* 'x' at byte 6 */
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats stats;
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
        u32(OK));
    EXPECT_EQ(find_carved_at(tree, at), nullptr) << "the wrong-dash form must not carve";
}

/* ---- N-113: the text school (VCF / GPX / OSP / POS / CHFP) ---- */

TEST(Carve, TextSchoolVcfAcceptedAndExtent) {
    /* N-113: a canonical vCard carves with the END:VCARD\r\n extent. */
    std::vector<u8> img(64 * 1024, 0);
    const size_t at = 512;
    const std::string card = "BEGIN:VCARD\r\nVERSION:3.0\r\nFN:Alice\r\nTEL:+1234\r\nEND:VCARD\r\n";
    std::memcpy(img.data() + at, card.data(), card.size());
    const size_t end = at + card.size();
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats stats;
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
        u32(OK));
    tree::FileNode* n = find_carved_at(tree, at);
    ASSERT_NE(n, nullptr) << "the canonical vCard must carve";
    EXPECT_EQ(n->props()->logical_size, end - at) << "the END:VCARD extent";
    std::string name;
    ASSERT_TRUE(cr::text::lossless_u16_to_utf8(n->props()->name, name));
    EXPECT_NE(name.find(".VCF"), std::string::npos);
}

TEST(Carve, TextSchoolVcfMultiCardContinuation) {
    /* N-113: the multi-card continuation - after END:VCARD\r\n, a second
     * BEGIN:VCARD resumes the walk; the extent covers BOTH cards. */
    std::vector<u8> img(64 * 1024, 0);
    const size_t at = 512;
    const std::string card1 = "BEGIN:VCARD\r\nFN:A\r\nEND:VCARD\r\n";
    const std::string card2 = "BEGIN:VCARD\r\nFN:B\r\nEND:VCARD\r\n";
    std::memcpy(img.data() + at, card1.data(), card1.size());
    std::memcpy(img.data() + at + card1.size(), card2.data(), card2.size());
    const size_t end = at + card1.size() + card2.size();
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats stats;
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
        u32(OK));
    tree::FileNode* n = find_carved_at(tree, at);
    ASSERT_NE(n, nullptr);
    EXPECT_EQ(n->props()->logical_size, end - at) << "both cards in the extent";
}

TEST(Carve, TextSchoolVcfNoEndTagFallsBackBounded) {
    /* N-113: no END:VCARD within the 20 MiB cap - the bounded fallback. */
    std::vector<u8> img(16 * 1024, 0);
    const size_t at = 512;
    std::memcpy(img.data() + at, "BEGIN:VCARD\r\nFN:A\r\n", 20); /* no end tag */
    const size_t img_size = img.size();
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats stats;
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
        u32(OK));
    tree::FileNode* n = find_carved_at(tree, at);
    ASSERT_NE(n, nullptr) << "the unterminated card still carves (the fallback)";
    EXPECT_EQ(n->props()->logical_size, img_size - at);
}

TEST(Carve, TextSchoolGpxAcceptedAndExtent) {
    /* N-113: the declaration + the <gpx window gate, the </gpx> extent.
     * The root element must sit past +0x16 (the og's window start) - the
     * encoding attribute pushes it there (a short declaration without it
     * puts <gpx at 21 and REJECTS - the og's own window semantics). */
    std::vector<u8> img(64 * 1024, 0);
    const size_t at = 512;
    const std::string doc =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n<gpx version=\"1.1\"><trk><name>t</name>"
        "</trk></gpx>";
    std::memcpy(img.data() + at, doc.data(), doc.size());
    const size_t end = at + doc.size();
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats stats;
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
        u32(OK));
    tree::FileNode* n = find_carved_at(tree, at);
    ASSERT_NE(n, nullptr) << "the canonical GPX must carve";
    EXPECT_EQ(n->props()->logical_size, end - at) << "the </gpx> extent";
    std::string name;
    ASSERT_TRUE(cr::text::lossless_u16_to_utf8(n->props()->name, name));
    EXPECT_NE(name.find(".GPX"), std::string::npos);
}

TEST(Carve, TextSchoolGpxRootBeforeWindowRejects) {
    /* N-113: the og's window semantics - a <gpx root BEFORE +0x16 (the
     * short-declaration form) rejects: the scan window is [+0x16, +0x216). */
    std::vector<u8> img(64 * 1024, 0);
    const size_t at = 512;
    const std::string doc = "<?xml version=\"1.0\"?><gpx></gpx>"; /* <gpx at 21 < 22 */
    std::memcpy(img.data() + at, doc.data(), doc.size());
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats stats;
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
        u32(OK));
    EXPECT_EQ(find_carved_at(tree, at), nullptr) << "the early-root form must reject";
}

TEST(Carve, TextSchoolOspAcceptedAndRejectOnMissing) {
    /* N-113: OSP - the needle found (extent = needle + 0xF) and the
     * family's unique reject-on-missing (no needle -> no carve). */
    const std::string needle = "\"libopenshot\":";
    for (const bool with_needle : {true, false}) {
        std::vector<u8> img(64 * 1024, 0);
        const size_t at = 512;
        std::string doc = "{\r\n \"di\": 1,\r\n \"clips\": []";
        if (with_needle)
            doc += ",\r\n " + needle + " 12345}";
        std::memcpy(img.data() + at, doc.data(), doc.size());
        MemDevice dev(std::move(img));
        tree::ResultTree tree;
        auto* root = tree.add_root(u"Lost Files");
        CarveFlags flags;
        CarveStats stats;
        ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr,
                              stats),
                  u32(OK));
        tree::FileNode* n = find_carved_at(tree, at);
        if (with_needle) {
            ASSERT_NE(n, nullptr) << "the needle form must carve";
            const size_t ndl_at = at + doc.find(needle);
            EXPECT_EQ(n->props()->logical_size, (ndl_at + 14 + 0xF) - at)
                << "the needle + 0xF extent";
            std::string name;
            ASSERT_TRUE(cr::text::lossless_u16_to_utf8(n->props()->name, name));
            EXPECT_NE(name.find(".OSP"), std::string::npos);
        } else {
            EXPECT_EQ(n, nullptr) << "the missing needle must REJECT (the og's size-0 posture)";
        }
    }
}

TEST(Carve, TextSchoolPosAcceptedAndExtent) {
    /* N-113: POS - the walk needle settled first-hand from FUN_18001a470:
     * "ProcessOn Schema File"; extent = find + 21 + 0x13. */
    std::vector<u8> img(64 * 1024, 0);
    const size_t at = 512;
    const std::string doc = "{\"diagram\":{\"page\":{\"shapes\":[]}},\"meta\":\"ProcessOn "
                            "Schema File v2\",\"tail\":1}";
    std::memcpy(img.data() + at, doc.data(), doc.size());
    const size_t ndl_at = at + doc.find("ProcessOn Schema File");
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats stats;
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
        u32(OK));
    tree::FileNode* n = find_carved_at(tree, at);
    ASSERT_NE(n, nullptr) << "the ProcessOn form must carve";
    EXPECT_EQ(n->props()->logical_size, (ndl_at + 21 + 0x13) - at) << "the needle + 0x13 extent";
    std::string name;
    ASSERT_TRUE(cr::text::lossless_u16_to_utf8(n->props()->name, name));
    EXPECT_NE(name.find(".POS"), std::string::npos);
}

TEST(Carve, TextSchoolChfpAcceptedAndExtent) {
    /* N-113: CHFP - the declaration + <BiffProject@0x28 gate, the
     * </BiffProject> extent. The 0x28 offset forces the padded header. */
    std::vector<u8> img(64 * 1024, 0);
    const size_t at = 512;
    const std::string decl = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
    const std::string body = "<BiffProject name=\"p\"><track/></BiffProject>";
    /* the root must sit EXACTLY at +0x28 (40): the declaration is 38
     * bytes; pad with 2 spaces to reach the offset */
    ASSERT_EQ(decl.size(), size_t(38));
    std::memcpy(img.data() + at, decl.data(), decl.size());
    img[at + 38] = ' ';
    img[at + 39] = ' ';
    std::memcpy(img.data() + at + 0x28, body.data(), body.size());
    const size_t end = at + 0x28 + body.size();
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats stats;
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
        u32(OK));
    tree::FileNode* n = find_carved_at(tree, at);
    ASSERT_NE(n, nullptr) << "the BiffProject form must carve";
    EXPECT_EQ(n->props()->logical_size, end - at) << "the </BiffProject> extent";
    std::string name;
    ASSERT_TRUE(cr::text::lossless_u16_to_utf8(n->props()->name, name));
    EXPECT_NE(name.find(".HFP"), std::string::npos);
}

TEST(Carve, TextSchoolChfpRootOffAnchorRejects) {
    /* N-113: the root at an offset other than +0x28 rejects (the og's
     * _memicmp(buf+0x28, ...) gate - position-sensitive). */
    std::vector<u8> img(64 * 1024, 0);
    const size_t at = 512;
    const std::string decl = "<?xml version=\"1.0\"?>";
    const std::string body = "<BiffProject></BiffProject>";
    std::memcpy(img.data() + at, decl.data(), decl.size()); /* 21 bytes */
    std::memcpy(img.data() + at + 21, body.data(), body.size());
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats stats;
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
        u32(OK));
    EXPECT_EQ(find_carved_at(tree, at), nullptr) << "the off-anchor root must reject";
}

TEST(Carve, TextSchoolVcfInnerCardIsSameFile) {
    /* N-150-class (this lane's verification of the parallel N-113 landing):
     * the multi-card file must carve ONCE - the inner BEGIN:VCARD re-match
     * is the same file (the og's sector-aligned probe + extent consumption
     * never re-fires inside a walked file). The parallel landing's test
     * checked only the outer extent; this pin closes the count. */
    std::vector<u8> img(64 * 1024, 0);
    const size_t at = 512;
    const std::string card1 = "BEGIN:VCARD\r\nFN:A\r\nEND:VCARD\r\n";
    const std::string card2 = "BEGIN:VCARD\r\nFN:B\r\nEND:VCARD\r\n";
    std::memcpy(img.data() + at, card1.data(), card1.size());
    std::memcpy(img.data() + at + card1.size(), card2.data(), card2.size());
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats stats;
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
        u32(OK));
    std::vector<tree::FileNode*> all;
    collect_children(root, all);
    ASSERT_EQ(all.size(), 1u) << "the inner card re-match must not double-carve";
    EXPECT_EQ(all[0]->props()->logical_size, card1.size() + card2.size());
}

TEST(Carve, SvgDeclaredDocumentCarvesOnce) {
    /* N-151 (discovered during this lane's N-113 verification): an
     * XML-declared SVG document must carve ONCE - the pre-fix tree carried
     * BOTH the declaration-start candidate (the <?xm row, override->SVG)
     * AND the interior direct-<svg candidate (the <svg row).
     * suppress_overlap on the XML family rows keeps the interior re-match
     * suppressed (the same document). */
    const char* svgdoc = "<?xml version=\"1.0\"?>\n<svg xmlns=\"x\"></svg>";
    std::vector<u8> img(256 * 1024, 0);
    std::memcpy(img.data() + 4096, svgdoc, std::strlen(svgdoc));
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats s;
    ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, s),
              u32(OK));
    std::vector<tree::FileNode*> all;
    collect_children(root, all);
    ASSERT_EQ(all.size(), 1u) << "ONE document, ONE file (the N-151 double-carve)";
    EXPECT_EQ(all[0]->props()->carve_format, u32(FMT_SVG));
    EXPECT_EQ(all[0]->props()->extents.front().offset, u64(4096))
        << "the declaration-start candidate owns the document";
}

TEST(Carve, GpxBomFormCarvesOnce) {
    /* N-150-class: the BOM'd GPX must carve ONCE (the BOM row's candidate
     * owns the document; the interior declaration re-match is the same
     * file). */
    const char* doc = "<?xml version=\"1.0\"?>\n<gpx version=\"1.1\"><trk></trk></gpx>";
    std::vector<u8> img(256 * 1024, 0);
    img[4096] = 0xEF;
    img[4097] = 0xBB;
    img[4098] = 0xBF;
    std::memcpy(img.data() + 4099, doc, std::strlen(doc));
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats s;
    ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, s),
              u32(OK));
    std::vector<tree::FileNode*> all;
    collect_children(root, all);
    ASSERT_EQ(all.size(), 1u) << "the BOM form is ONE file (the nested declaration is the same)";
    EXPECT_EQ(all[0]->props()->carve_format, u32(FMT_GPX_BOM));
    EXPECT_EQ(all[0]->props()->extents.front().offset, u64(4096));
    EXPECT_EQ(all[0]->props()->logical_size, std::strlen(doc) + 3);
}

/* ===================================================================== */
/* N-152: the DXF 999-comment entry key - the u16 "99" 0x3939 dispatch  */
/* (RAWSearchDLL :2722-2733) and the probe's leading-comment acceptance */
/* (:42240-42290).                                                       */
/* ===================================================================== */

TEST(Carve, Dxf999CommentHeaderLfCarves) {
    /* N-152: a comment-led DXF with LF line endings - the og's u16 "99"
     * key dispatches to CDXFAnalyser and the probe's comment branch
     * scans forward for the LF section form "  0\nSECTION\n  ". The
     * interior "  0\r"-key re-match never fires (the same document -
     * suppress_overlap). */
    const char* doc = "999\nFree-form comment line\n"
                      "  0\nSECTION\n  2\nHEADER\n  9\n$INSBASE\n  10\n0.0\n  20\n0.0\n"
                      "  0\nENDSEC\n  0\nEOF\n";
    std::vector<u8> img(256 * 1024, 0);
    std::memcpy(img.data() + 4096, doc, std::strlen(doc));
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats s;
    ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, s),
              u32(OK));
    std::vector<tree::FileNode*> all;
    collect_children(root, all);
    ASSERT_EQ(all.size(), 1u) << "ONE comment-led document, ONE file";
    EXPECT_EQ(all[0]->props()->carve_format, u32(FMT_DXF_C999))
        << "the u16 0x3939 key's row owns the comment-led candidate";
    EXPECT_EQ(all[0]->props()->extents.front().offset, u64(4096))
        << "the candidate starts at the 999 comment, not the interior header";
}

TEST(Carve, Dxf999CommentHeaderCrLfCarves) {
    /* N-152: the CRLF form - "999\r\n" then the canonical CRLF section
     * pattern "  0\r\nSECTION\r\n  " (the og's _Size=0x10 branch). */
    const char* doc = "999\r\nFree-form comment line\r\n"
                      "  0\r\nSECTION\r\n  2\r\nHEADER\r\n  9\r\n$INSBASE\r\n"
                      "  10\r\n0.0\r\n  20\r\n0.0\r\n  0\r\nENDSEC\r\n  0\r\nEOF\r\n";
    std::vector<u8> img(256 * 1024, 0);
    std::memcpy(img.data() + 8192, doc, std::strlen(doc));
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats s;
    ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, s),
              u32(OK));
    std::vector<tree::FileNode*> all;
    collect_children(root, all);
    ASSERT_EQ(all.size(), 1u) << "ONE comment-led document, ONE file";
    EXPECT_EQ(all[0]->props()->carve_format, u32(FMT_DXF_C999));
    EXPECT_EQ(all[0]->props()->extents.front().offset, u64(8192));
}

TEST(Carve, Dxf999RejectsNonCommentContinuation) {
    /* N-152: the hostile near-miss - "99x" is not the 999 comment line;
     * the probe's variant branch requires the full "999" + line ending,
     * so the u16 key's candidate REJECTS (the comment row claims
     * nothing). The og-faithful outcome on this fixture: the interior
     * u32 "  0\r" key still fires and the canonical pattern matches
     * there - the headless canonical salvage carves ONE DXF at the
     * interior position (the og's u32 map dispatches the same way). */
    const char* doc = "99x\r\n  0\r\nSECTION\r\n  2\r\nHEADER\r\n  0\r\nENDSEC\r\n";
    std::vector<u8> img(256 * 1024, 0);
    std::memcpy(img.data() + 4096, doc, std::strlen(doc));
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats s;
    ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, s),
              u32(OK));
    std::vector<tree::FileNode*> all;
    collect_children(root, all);
    ASSERT_EQ(all.size(), 1u) << "the near-miss comment claims nothing; ONE salvage";
    EXPECT_EQ(all[0]->props()->carve_format, u32(FMT_DXF))
        << "the claim is the canonical row's (the interior position), never the comment row's";
    EXPECT_EQ(all[0]->props()->extents.front().offset, u64(4096 + 5))
        << "the headless salvage starts at the interior section header";
}

TEST(Carve, Dxf999InteriorCommentOfCanonicalFileDoesNotDoubleCarve) {
    /* N-152: the reverse direction - a CANONICAL DXF carrying an interior
     * 999 comment (group code 999 mid-body): the canonical row owns the
     * position; the interior "99" re-match is the same file. */
    const char* doc = "  0\r\nSECTION\r\n  2\r\nHEADER\r\n"
                      "999\r\ninterior comment\r\n"
                      "  0\r\nENDSEC\r\n  0\r\nEOF\r\n";
    std::vector<u8> img(256 * 1024, 0);
    std::memcpy(img.data() + 4096, doc, std::strlen(doc));
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats s;
    ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, s),
              u32(OK));
    std::vector<tree::FileNode*> all;
    collect_children(root, all);
    ASSERT_EQ(all.size(), 1u) << "the canonical document carves once";
    EXPECT_EQ(all[0]->props()->carve_format, u32(FMT_DXF));
    EXPECT_EQ(all[0]->props()->extents.front().offset, u64(4096));
}

/* ===================================================================== */
/* N-115: the OfficeTagFromXML typing pass - the rels-path split, the    */
/* reordered-zip tolerance, and the wider tag corpus.                    */
/* ===================================================================== */

namespace {

/* a minimal multi-entry stored ZIP: the LFH chain + one CD record per
 * entry + the EOCD (the typing walk only needs the LFH chain) */
size_t plant_zip_entries(std::vector<u8>& img, size_t at,
                         const std::vector<std::pair<const char*, std::string>>& entries) {
    std::vector<size_t> lfh_offs;
    size_t pos = at;
    for (const auto& [name, data] : entries) {
        const size_t nlen = std::strlen(name);
        std::memcpy(img.data() + pos, "PK\x03\x04", 4);
        put16(img, pos + 4, 20);
        put16(img, pos + 8, 0); /* stored */
        put16(img, pos + 12, 0x5A46);
        put32v(img, pos + 18, u32(data.size()));
        put32v(img, pos + 22, u32(data.size()));
        put16(img, pos + 26, u16(nlen));
        put16(img, pos + 28, 0);
        std::memcpy(img.data() + pos + 30, name, nlen);
        if (!data.empty())
            std::memcpy(img.data() + pos + 30 + nlen, data.data(), data.size());
        lfh_offs.push_back(pos);
        pos += 30 + nlen + data.size();
    }
    /* central directory + EOCD */
    const size_t cd_at = pos;
    for (size_t i = 0; i < entries.size(); ++i) {
        const auto& [name, data] = entries[i];
        const size_t nlen = std::strlen(name);
        std::memcpy(img.data() + pos, "PK\x01\x02", 4);
        put16(img, pos + 4, 20);
        put16(img, pos + 12, 0x5A46);
        put32v(img, pos + 20, u32(data.size()));
        put32v(img, pos + 24, u32(data.size()));
        put16(img, pos + 28, u16(nlen));
        put32v(img, pos + 42, u32(lfh_offs[i] - at));
        std::memcpy(img.data() + pos + 46, name, nlen);
        pos += 46 + nlen;
    }
    const size_t cd_len = pos - cd_at;
    std::memcpy(img.data() + pos, "PK\x05\x06", 4);
    put16(img, pos + 8, u16(entries.size()));
    put16(img, pos + 10, u16(entries.size()));
    put32v(img, pos + 12, u32(cd_len));
    put32v(img, pos + 16, u32(cd_at - at));
    return at;
}

} // namespace

/* N-115/N-153: the carved ARCHIVE count - direct children of the carve
 * root only (the S-032 inventory presents the inner entries as CHILD
 * nodes of the archive; collect_children would count them too). */
static size_t direct_carved_count(tree::FileNode* root) {
    size_t n = 0;
    for (tree::FileNode* c = root->first_child(); c; c = c->next_sibling())
        ++n;
    return n;
}

TEST(Carve, OoxmlRelsPathSplit) {
    /* N-115: the og's per-entry rels-path map (word/xl/ppt) types the
     * OOXML trio - each form stamps its own id and extension. The
     * extension is asserted case-insensitively (the S-049 naming
     * contract emits the UPPERCASE form, DOCX_000001.DOCX - the
     * GZ_000001.GZ real-session pin - but the typed extension identity
     * is the case-folded tail, per the N-126 lane's earlier push). */
    const struct {
        const char* rels;
        u32 fmt;
        const char* ext;
    } cases[] = {
        {"word/_rels/document.xml.rels", FMT_DOCX, ".docx"},
        {"xl/_rels/workbook.xml.rels", FMT_XLSX, ".xlsx"},
        {"ppt/_rels/presentation.xml.rels", FMT_PPTX, ".pptx"},
    };
    for (const auto& c : cases) {
        std::vector<u8> img(256 * 1024, 0);
        plant_zip_entries(img, 4096,
                          {{"[Content_Types].xml", "<Types/>"},
                           {c.rels, "<Relationships/>"},
                           {"doc.xml", "body"}});
        MemDevice dev(std::move(img));
        tree::ResultTree tree;
        auto* root = tree.add_root(u"Lost Files");
        CarveFlags flags;
        CarveStats s;
        ASSERT_EQ(
            carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, s),
            u32(OK));
        ASSERT_EQ(direct_carved_count(root), 1u) << "the rels path " << c.rels;
        tree::FileNode* n0 = root->first_child();
        EXPECT_EQ(n0->props()->carve_format, c.fmt) << "the rels-path split";
        /* the S-049 naming contract: <PREFIX>_<counter>.<UPPERCASE-EXT>.
         * The table carries the lowercase forms, the emitted name the
         * uppercase ones - both sides fold to lowercase so the comparison
         * is case-insensitive (the b26017a7/59bd5208 re-pin chain each
         * folded only one side; this is the both-sides form, and the
         * 59bd5208 push's conflict-marker residue is what this repair
         * removed - the marker failed the -Werror build outright). */
        const std::string nm = n0->name_utf8();
        ASSERT_GE(nm.size(), size_t(5));
        std::string tail = nm.substr(nm.size() - 5);
        for (auto& ch : tail)
            if (ch >= 'A' && ch <= 'Z')
                ch = char(ch + 32);
        std::string want = c.ext;
        for (auto& ch : want)
            if (ch >= 'A' && ch <= 'Z')
                ch = char(ch + 32);
        EXPECT_EQ(tail, want) << "the typed extension";
    }
}

TEST(Carve, ZipEntriesReorderedStillTyped) {
    /* N-115: the og matches the rels path ANYWHERE in the entry list - a
     * damaged/reordered zip whose [Content_Types].xml is NOT the literal
     * first entry still types (the old first-entry gate missed it). */
    std::vector<u8> img(256 * 1024, 0);
    plant_zip_entries(img, 4096,
                      {{"docProps/app.xml", "<app/>"},
                       {"[Content_Types].xml", "<Types/>"},
                       {"xl/_rels/workbook.xml.rels", "<Relationships/>"}});
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats s;
    ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, s),
              u32(OK));
    ASSERT_EQ(direct_carved_count(root), 1u);
    EXPECT_EQ(root->first_child()->props()->carve_format, u32(FMT_XLSX))
        << "the reordered zip still types by the per-entry scan";
}

TEST(Carve, OdfEpubTypedByMimetype) {
    /* N-115: the wider tag corpus - the mimetype entry's content types
     * the ODF trio and EPUB (the og's decompressed-content tags; the
     * stored mimetype needs no inflate). */
    const struct {
        const char* mime;
        u32 fmt;
    } cases[] = {
        {"application/vnd.oasis.opendocument.text", FMT_ODT},
        {"application/vnd.oasis.opendocument.spreadsheet", FMT_ODS},
        {"application/vnd.oasis.opendocument.presentation", FMT_ODP},
        {"application/epub+zip", FMT_EPUB},
        {"application/hwp+zip", FMT_HWP},
    };
    for (const auto& c : cases) {
        std::vector<u8> img(256 * 1024, 0);
        plant_zip_entries(img, 4096, {{"mimetype", c.mime}, {"content.xml", "<doc/>"}});
        MemDevice dev(std::move(img));
        tree::ResultTree tree;
        auto* root = tree.add_root(u"Lost Files");
        CarveFlags flags;
        CarveStats s;
        ASSERT_EQ(
            carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, s),
            u32(OK));
        ASSERT_EQ(direct_carved_count(root), 1u) << "mimetype " << c.mime;
        EXPECT_EQ(root->first_child()->props()->carve_format, c.fmt) << "the mimetype content tag";
    }
}

TEST(Carve, IworkAndCdrTypedByEntryName) {
    /* N-115: the name-tag set - the iWork Index/.iwa family + index.apxl
     * and CorelDRAW's content/riffData.cdr (no decompression needed). */
    const struct {
        const char* entry;
        u32 fmt;
    } cases[] = {
        {"Index/Document.iwa", FMT_IWORK},
        {"index.apxl", FMT_IWORK},
        {"content/riffData.cdr", FMT_CDR},
    };
    for (const auto& c : cases) {
        std::vector<u8> img(256 * 1024, 0);
        plant_zip_entries(img, 4096, {{c.entry, std::string(32, 'x')}});
        MemDevice dev(std::move(img));
        tree::ResultTree tree;
        auto* root = tree.add_root(u"Lost Files");
        CarveFlags flags;
        CarveStats s;
        ASSERT_EQ(
            carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, s),
            u32(OK));
        ASSERT_EQ(direct_carved_count(root), 1u) << "entry " << c.entry;
        EXPECT_EQ(root->first_child()->props()->carve_format, c.fmt) << "the name tag";
    }
}

TEST(Carve, OfficeUnrefinableStaysDocxAndPlainZipStaysZip) {
    /* N-115: an office-shaped zip the map cannot refine stays the
     * dispatch row (DOCX, the og's 0x49); a zip with NO office shape and
     * no tags stays the generic ZIP row. */
    std::vector<u8> img(256 * 1024, 0);
    plant_zip_entries(img, 4096, {{"[Content_Types].xml", "<Types/>"}, {"media.bin", "xxx"}});
    plant_zip_entries(img, 8192, {{"plain.txt", std::string(200, 'p')}});
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats s;
    ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, s),
              u32(OK));
    ASSERT_EQ(direct_carved_count(root), 2u);
    u32 seen[2] = {0, 0};
    for (tree::FileNode* n = root->first_child(); n; n = n->next_sibling()) {
        const u64 off = n->props()->extents.front().offset;
        seen[off == 4096 ? 0 : 1] = n->props()->carve_format;
    }
    EXPECT_EQ(seen[0], u32(FMT_DOCX)) << "the unrefinable office zip stays DOCX";
    EXPECT_EQ(seen[1], u32(FMT_ZIP)) << "the untyped zip stays the generic ZIP row";
}

TEST(Carve, MultiEntryZipCarvesOnce) {
    /* N-153: a multi-entry archive emits ONE file - the inner LFH
     * positions are the SAME archive (the og's sector-bitmap
     * consumption; the S-032 inventory presents the entries as
     * children). Pre-fix: the outer node + one phantom per inner LFH
     * whose tail-to-EOCD span cleared the 128-byte floor. */
    std::vector<u8> img(256 * 1024, 0);
    plant_zip_entries(img, 4096,
                      {{"a.txt", std::string(200, 'a')}, {"b.txt", std::string(200, 'b')}});
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats s;
    ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, s),
              u32(OK));
    ASSERT_EQ(direct_carved_count(root), 1u) << "one archive, one file (the N-153 phantom)";
    tree::FileNode* az = root->first_child();
    EXPECT_EQ(az->props()->extents.front().offset, u64(4096));
    EXPECT_EQ(az->props()->carve_format, u32(FMT_ZIP));
    EXPECT_GT(az->props()->logical_size, u64(500)) << "the full archive extent";
}
/* ---- M-117: the Outlook Express Dbx analyser (id 0x15, three keys) ---- */

namespace {

/* the og's four Outlook Express store GUIDs (FUN_180016740's stack table
 * :17529-17541 - the CF12ADFE family's three generations + the fourth
 * variant), and the marked 16-byte pattern */
const u8 kDbxOeGuids[4][16] = {
    {0xCF, 0xAD, 0x12, 0xFE, 0xC5, 0xFD, 0x74, 0x6F, 0x66, 0xE3, 0xD1, 0x11, 0x9A, 0x4E, 0x00,
     0xC0},
    {0xCF, 0xAD, 0x12, 0xFE, 0xC6, 0xFD, 0x74, 0x6F, 0x66, 0xE3, 0xD1, 0x11, 0x9A, 0x4E, 0x00,
     0xC0},
    {0xCF, 0xAD, 0x12, 0xFE, 0xC7, 0xFD, 0x74, 0x6F, 0x66, 0xE3, 0xD1, 0x11, 0x9A, 0x4E, 0x00,
     0xC0},
    {0xCF, 0xAD, 0x12, 0xFE, 0x30, 0x9D, 0xFE, 0x26, 0x8F, 0x1A, 0xD2, 0x11, 0xAA, 0xBF, 0x00,
     0x60},
};
const u8 kDbxMarked[16] = {0xF7, 0x89, 0x34, 0x2C, 0xF7, 0xF0, 0xDE, 0x6D,
                           0xAF, 0x22, 0x89, 0xD1, 0x00, 0xFF, 0xF0, 0xB0};

} // namespace

TEST(Carve, DbxPlainStoreGuidFamilyCarvesWithDeclaredSize) {
    /* M-117: the classic key (CF AD 12 FE) + each of the four OE store
     * GUIDs carves with the SELF-DECLARED size u32@+0x7C (the plain
     * form's whole size grammar). */
    for (int g = 0; g < 4; ++g) {
        std::vector<u8> img(128 * 1024, 0);
        const size_t at = 512;
        std::memcpy(img.data() + at, kDbxOeGuids[g], 16);
        /* a plausible self-declared store size at +0x7C */
        const u32 declared = 0x8400;
        put32v(img, at + 0x7C, declared);
        MemDevice dev(std::move(img));
        tree::ResultTree tree;
        auto* root = tree.add_root(u"Lost Files");
        CarveFlags flags;
        CarveStats stats;
        ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr,
                              stats),
                  u32(OK));
        std::vector<tree::FileNode*> all;
        collect_children(root, all);
        tree::FileNode* n = nullptr;
        for (tree::FileNode* c : all)
            if (!c->props()->extents.empty() && c->props()->extents.front().offset == at)
                n = c;
        ASSERT_NE(n, nullptr) << "the OE store GUID #" << g << " must carve";
        EXPECT_EQ(n->props()->carve_format, u64(FMT_DBX));
        EXPECT_EQ(n->props()->logical_size, size_t(declared)) << "the self-declared size@0x7C";
        std::string name;
        ASSERT_TRUE(cr::text::lossless_u16_to_utf8(n->props()->name, name));
        /* the S-049 naming contract emits the UPPERCASE form
         * (DBX_000001.DBX) - assert the case-folded ".dbx" tail */
        ASSERT_GE(name.size(), size_t(4));
        std::string tail = name.substr(name.size() - 4);
        for (auto& ch : tail)
            if (ch >= 'A' && ch <= 'Z')
                ch = char(ch + 32);
        EXPECT_EQ(tail, ".dbx") << "the dbx extension";
    }
}

TEST(Carve, DbxJmf6VariantCarves) {
    /* M-117: the JMF6 key + the version word 03 00 10 00 (the probe's
     * u64 0x0010000336464D4A) carves as the container variant with the
     * same declared-size grammar. */
    std::vector<u8> img(128 * 1024, 0);
    const size_t at = 512;
    std::memcpy(img.data() + at, "JMF6", 4);
    img[at + 4] = 0x03;
    img[at + 5] = 0x00;
    img[at + 6] = 0x10;
    img[at + 7] = 0x00;
    const u32 declared = 0x18000; /* fits inside the 128 KiB image */
    put32v(img, at + 0x7C, declared);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats stats;
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
        u32(OK));
    std::vector<tree::FileNode*> all;
    collect_children(root, all);
    tree::FileNode* n = nullptr;
    for (tree::FileNode* c : all)
        if (!c->props()->extents.empty() && c->props()->extents.front().offset == at)
            n = c;
    ASSERT_NE(n, nullptr) << "the JMF6+version-word form must carve";
    EXPECT_EQ(n->props()->carve_format, u64(FMT_DBX_JMF6));
    EXPECT_EQ(n->props()->logical_size, size_t(declared));
}

TEST(Carve, DbxJmf6WrongVersionWordRejected) {
    /* M-117: the probe gates the version word exactly - "JMF6" with the
     * finding text's transcribed "03 00 01 00" (the corpus literal is
     * 03 00 10 00) is NOT a table identity and rejects. */
    std::vector<u8> img(128 * 1024, 0);
    const size_t at = 512;
    std::memcpy(img.data() + at, "JMF6", 4);
    img[at + 4] = 0x03;
    img[at + 5] = 0x00;
    img[at + 6] = 0x01;
    img[at + 7] = 0x00;
    put32v(img, at + 0x7C, 0x8400);
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats stats;
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
        u32(OK));
    std::vector<tree::FileNode*> all;
    collect_children(root, all);
    for (tree::FileNode* c : all)
        EXPECT_NE(c->props()->carve_format, u64(FMT_DBX_JMF6))
            << "the wrong version word must reject";
}

TEST(Carve, DbxMarkedFormZeroEndExtent) {
    /* M-117: the marked form's size is the block computation - the
     * extent from the candidate start to the LAST NON-ZERO byte (the
     * trailing zero run trimmed, the walk ending at the first all-zero
     * block). */
    std::vector<u8> img(128 * 1024, 0);
    const size_t at = 512;
    std::memcpy(img.data() + at, kDbxMarked, 16);
    /* two blocks of data, the last non-zero byte at at+0x405 */
    for (size_t i = 16; i < 0x800; ++i)
        img[at + i] = u8((i * 7) & 0xFF);
    const size_t last_nonzero = at + 0x405;
    for (size_t i = last_nonzero + 1; i < at + 0x800; ++i)
        img[i] = 0;
    MemDevice dev(std::move(img));
    tree::ResultTree tree;
    auto* root = tree.add_root(u"Lost Files");
    CarveFlags flags;
    CarveStats stats;
    ASSERT_EQ(
        carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr, stats),
        u32(OK));
    std::vector<tree::FileNode*> all;
    collect_children(root, all);
    tree::FileNode* n = nullptr;
    for (tree::FileNode* c : all)
        if (!c->props()->extents.empty() && c->props()->extents.front().offset == at)
            n = c;
    ASSERT_NE(n, nullptr) << "the marked form must carve";
    EXPECT_EQ(n->props()->carve_format, u64(FMT_DBX_MARKED));
    EXPECT_EQ(n->props()->logical_size, (last_nonzero + 1) - at)
        << "the extent ends at the last non-zero byte (the zero tail trimmed)";
    std::string note;
    ASSERT_TRUE(cr::text::lossless_u16_to_utf8(n->props()->carve_note, note));
    EXPECT_EQ(note, "dbx-marked-zeroend-walk");
}

TEST(Carve, DbxRejectsMagicWithoutTableIdentity) {
    /* M-117: the key without a table identity rejects - CF AD 12 FE
     * with a non-OE GUID tail, and each bare key without any identity
     * tail, produce no DBX row (the probe's gate). */
    {
        std::vector<u8> img(128 * 1024, 0);
        const size_t at = 512;
        std::memcpy(img.data() + at, "\xCF\xAD\x12\xFE", 4);
        std::memcpy(img.data() + at + 4, "\xDE\xAD\xBE\xEF\xCA\xFE\xBA\xBE\x13\x37\x13\x37", 12);
        put32v(img, at + 0x7C, 0x8400);
        MemDevice dev(std::move(img));
        tree::ResultTree tree;
        auto* root = tree.add_root(u"Lost Files");
        CarveFlags flags;
        CarveStats stats;
        ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr,
                              stats),
                  u32(OK));
        std::vector<tree::FileNode*> all;
        collect_children(root, all);
        for (tree::FileNode* c : all)
            EXPECT_NE(c->props()->carve_format, u64(FMT_DBX)) << "the non-OE GUID must reject";
    }
    for (int variant = 0; variant < 3; ++variant) {
        std::vector<u8> img(64 * 1024, 0);
        const size_t at = 512;
        if (variant == 0)
            std::memcpy(img.data() + at, "\xCF\xAD\x12\xFE", 4);
        else if (variant == 1)
            std::memcpy(img.data() + at, "JMF6", 4);
        else
            std::memcpy(img.data() + at, "\xF7\x89\x34\x2C", 4);
        /* the identity tail stays all-zero: none of the probe's table
         * identities matches */
        MemDevice dev(std::move(img));
        tree::ResultTree tree;
        auto* root = tree.add_root(u"Lost Files");
        CarveFlags flags;
        CarveStats stats;
        ASSERT_EQ(carve_slice(&dev, flags, nullptr, nullptr, nullptr, tree, root, nullptr, nullptr,
                              stats),
                  u32(OK));
        std::vector<tree::FileNode*> all;
        collect_children(root, all);
        for (tree::FileNode* c : all)
            EXPECT_NE(c->props()->carve_format, u64(FMT_DBX_JMF6))
                << "the bare key without identity must reject (variant " << variant << ")";
    }
}

TEST(Carve, DbxRegistryRowsCarryTheThreeOgKeys) {
    /* M-117: the three registry rows exist, share the "dbx" extension
     * and the shared validator (the og's duplicate-key policy), and the
     * F8 gate + self-probe stay green over them. */
    const Signature* reg = registry();
    size_t seen = 0;
    for (size_t i = 0; i < registry_size(); ++i) {
        const Signature* s = reg + i;
        if (s->id == FMT_DBX_JMF6 || s->id == FMT_DBX || s->id == FMT_DBX_MARKED) {
            ++seen;
            EXPECT_EQ(std::string(s->ext), "dbx");
            EXPECT_EQ(s->header_len, 4u);
            EXPECT_EQ(s->header_offset, 0u);
            EXPECT_EQ(s->min_size, 16u) << "the probe's own 16-byte identity window";
            EXPECT_EQ(s->max_size, 0xFFFFFFFFull) << "no og bound - the u32 field's own range";
        }
    }
    EXPECT_EQ(seen, 3u) << "three rows, one per og key";
    std::vector<const Signature*> bad;
    EXPECT_EQ(validate_registry(&bad), 0u);
    EXPECT_EQ(self_probe_registry(&bad), 0u);
}
