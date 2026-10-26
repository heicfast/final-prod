/* M-035 tests — the lost-candidate content-consistency scorer
 * (the og's engine-side FileFilter validation semantics, exFAT-only),
 * the tree detach_subtree mechanism it needs, and the end-to-end wiring
 * through the orchestrator's candidate scan. */
#include <gtest/gtest.h>
#include "cr/lost/cr_consistency.h"
#include "cr/lost/cr_lostpart.h"
#include "cr/orchestrator/cr_scan.h"
#include "cr/storage/cr_device.h"
#include "cr/tree/cr_node.h"
#include <cstring>
#include <deque>
#include <functional>
#include <vector>

using namespace cr;
using namespace cr::lost;
using namespace cr::orchestrator;
using namespace cr::tree;

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

/* ---- the scorer's unit surface (pure) --------------------------------- */

TEST(Consistency, ExtensionIsLastDotUppercased) {
    EXPECT_EQ(consistency_extension(u"photo.jpg"), "JPG");
    EXPECT_EQ(consistency_extension(u"ARCHIVE.Tar.GZ"), "GZ");
    EXPECT_EQ(consistency_extension(u"nodot"), "");
    EXPECT_EQ(consistency_extension(u"trailing."), "");
    EXPECT_EQ(consistency_extension(u"lower.png"), "PNG");
}

TEST(Consistency, CheckableVocabularyIsSignatureFamiliesPlusSpecials) {
    /* the carve registry's families */
    EXPECT_TRUE(consistency_extension_checkable("JPG"));
    EXPECT_TRUE(consistency_extension_checkable("PNG"));
    EXPECT_TRUE(consistency_extension_checkable("TIF"));
    EXPECT_TRUE(consistency_extension_checkable("PDF"));
    EXPECT_TRUE(consistency_extension_checkable("MP4"));
    EXPECT_TRUE(consistency_extension_checkable("MOV"));
    /* the og's two filter-set specials (FUN_180025d60 :31659-31680) */
    EXPECT_TRUE(consistency_extension_checkable("TIFF"));
    EXPECT_TRUE(consistency_extension_checkable("JPEG"));
    /* the OLE .DOC family IS a core signature (D0 CF 11 E0) */
    EXPECT_TRUE(consistency_extension_checkable("DOC"));
    /* families without a core signature stay uncheckable (bounded
     * vocabulary - camera RAWs and web formats the og's Graphic set
     * carried; registered with the finding) */
    EXPECT_FALSE(consistency_extension_checkable("HTML"));
    EXPECT_FALSE(consistency_extension_checkable("NEF"));
    EXPECT_FALSE(consistency_extension_checkable(""));
}

TEST(Consistency, ClassifierIsSubstringPlusVideoSpecials) {
    /* substring: the family ext appears anywhere in the name */
    EXPECT_TRUE(consistency_name_matches_family(u"photo.jpg", "JPG", "JPG"));
    EXPECT_TRUE(consistency_name_matches_family(u"myjpgfile.txt", "", "JPG"));
    EXPECT_FALSE(consistency_name_matches_family(u"photo.png", "PNG", "JPG"));
    /* the MOV/MP4/JPG family specials: a .JPG/.JPEG name matches those
     * three families regardless of the substring check */
    EXPECT_TRUE(consistency_name_matches_family(u"clip.jpeg", "JPEG", "MOV"));
    EXPECT_TRUE(consistency_name_matches_family(u"clip.jpg", "JPG", "MP4"));
    EXPECT_TRUE(consistency_name_matches_family(u"clip.jpg", "JPG", "JPG"));
    /* ... and ONLY those three families (a .JPEG name does not match PNG) */
    EXPECT_FALSE(consistency_name_matches_family(u"clip.jpeg", "JPEG", "PNG"));
    EXPECT_FALSE(consistency_name_matches_family(u"song.mp3", "MP3", "MOV"));
}

/* a synthetic volume: 8 KiB, content planted by offset */
std::vector<u8> scorer_volume() {
    std::vector<u8> img(8192, 0);
    /* JPEG magic at 512, PNG magic at 1024, text at 1536/2048/2560 */
    const u8 jpg[] = {0xFF, 0xD8, 0xFF, 0xE0};
    std::memcpy(img.data() + 512, jpg, sizeof(jpg));
    const u8 png[] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
    std::memcpy(img.data() + 1024, png, sizeof(png));
    std::memcpy(img.data() + 1536, "plain text bytes", 16);
    std::memcpy(img.data() + 2048, "more text bytes", 15);
    std::memcpy(img.data() + 2560, "text again here", 15);
    return img;
}

ConsistencyFile mk_file(const char16_t* name, u64 start, u64 len = 256) {
    ConsistencyFile f;
    /* deque storage: references to existing elements survive later
     * push_backs (a vector would reallocate and dangle them) */
    static std::deque<std::u16string> names;
    names.push_back(name);
    f.name = &names.back();
    f.data_start = start;
    f.length = len;
    f.has_extent = true;
    return f;
}

TEST(Lost, ContentInconsistentGhostRejected) {
    /* the finding's named test: a valid-BPB ghost whose checkable files'
     * content does not match their extensions. 6 files, all .JPG-named,
     * content is text at 5 of 6 data starts -> ratio 1/6, majority
     * filterable -> REJECTED. */
    MemDevice dev(scorer_volume());
    std::vector<ConsistencyFile> files;
    files.push_back(mk_file(u"good.jpg", 512));   /* JPEG content -> matched */
    files.push_back(mk_file(u"bad1.jpg", 1536));  /* text */
    files.push_back(mk_file(u"bad2.jpg", 2048));
    files.push_back(mk_file(u"bad3.jpg", 2560));
    files.push_back(mk_file(u"bad4.jpg", 3072));
    files.push_back(mk_file(u"bad5.jpg", 3584));
    const ConsistencyVerdict v =
        score_content_consistency(&dev, files.data(), files.size());
    EXPECT_EQ(6u, v.considered);
    EXPECT_EQ(6u, v.filterable);
    EXPECT_EQ(1u, v.matched);
    EXPECT_DOUBLE_EQ(1.0 / 6.0, v.ratio);
    EXPECT_TRUE(v.rejected);
    EXPECT_TRUE(v.consistent);
    EXPECT_FALSE(v.out_of_bounds);
}

TEST(Lost, ContentConsistentGhostAccepted) {
    /* matched content: every .JPG/.PNG name sits on its own magic ->
     * ratio 1.0 -> accepted (the over-rejection guard). */
    MemDevice dev(scorer_volume());
    std::vector<ConsistencyFile> files;
    files.push_back(mk_file(u"a.jpg", 512));
    files.push_back(mk_file(u"b.jpg", 512));
    files.push_back(mk_file(u"c.png", 1024));
    files.push_back(mk_file(u"d.jpg", 512));
    const ConsistencyVerdict v =
        score_content_consistency(&dev, files.data(), files.size());
    EXPECT_EQ(4u, v.considered);
    EXPECT_EQ(4u, v.filterable);
    EXPECT_EQ(4u, v.matched);
    EXPECT_DOUBLE_EQ(1.0, v.ratio);
    EXPECT_FALSE(v.rejected);
}

TEST(Lost, RatioBoundaryThirtyPercent) {
    /* ratio EXACTLY 0.3 (3/10 matched, majority filterable) does NOT
     * reject: the decompile's reject compare is `<= 0.3 && != 0.3` -
     * STRICT less-than (:31152-31155). This pins the decoded truth and
     * corrects the finding text's `<= 0.3`. */
    MemDevice dev(scorer_volume());
    std::vector<ConsistencyFile> files;
    files.push_back(mk_file(u"m1.jpg", 512));
    files.push_back(mk_file(u"m2.jpg", 512));
    files.push_back(mk_file(u"m3.jpg", 512));
    for (int i = 0; i < 7; ++i)
        files.push_back(mk_file(u"bad.jpg", 1536 + u64(i) * 512));
    ConsistencyVerdict v = score_content_consistency(&dev, files.data(), files.size());
    EXPECT_EQ(10u, v.filterable);
    EXPECT_EQ(3u, v.matched);
    EXPECT_DOUBLE_EQ(0.3, v.ratio);
    EXPECT_FALSE(v.rejected) << "ratio == 0.3 must NOT reject (strict <)";

    /* one fewer match (2/10 = 0.2 < 0.3, majority filterable) -> reject */
    files[2].data_start = 2048; /* was a JPEG match */
    v = score_content_consistency(&dev, files.data(), files.size());
    EXPECT_DOUBLE_EQ(0.2, v.ratio);
    EXPECT_TRUE(v.rejected);
}

TEST(Lost, MajorityGateRequiresHalfFilterable) {
    /* the considered/2 <= filterable gate: 6 files but only 2 checkable
     * (3 <= 2 is false) -> NO reject even at ratio 0. The unfilterable
     * majority shields the candidate (the og's same gate). */
    MemDevice dev(scorer_volume());
    std::vector<ConsistencyFile> files;
    files.push_back(mk_file(u"bad.jpg", 1536));
    files.push_back(mk_file(u"bad2.jpg", 2048));
    files.push_back(mk_file(u"note.txt", 512));   /* unfilterable */
    files.push_back(mk_file(u"doc.pdfx", 512));  /* unfilterable */
    files.push_back(mk_file(u"raw.dat", 512));   /* unfilterable */
    files.push_back(mk_file(u"x.abc", 512));     /* unfilterable */
    const ConsistencyVerdict v =
        score_content_consistency(&dev, files.data(), files.size());
    EXPECT_EQ(6u, v.considered);
    EXPECT_EQ(2u, v.filterable);
    EXPECT_EQ(0u, v.matched);
    EXPECT_FALSE(v.rejected) << "filterable minority must not reject";
}

TEST(Lost, DirectoriesAndExtentlessFilesMirrorOgCounters) {
    /* directories are skipped from considered (the og's 0x10 attribute);
     * extentless checkable files count as considered but neither
     * filterable nor unfilterable (the og's start_cluster<2 shape). */
    MemDevice dev(scorer_volume());
    std::vector<ConsistencyFile> files;
    ConsistencyFile dir;
    static const std::u16string dirname = u"somedir";
    dir.name = &dirname;
    dir.is_directory = true;
    dir.has_extent = true;
    dir.data_start = 512;
    files.push_back(dir);
    files.push_back(mk_file(u"bad.jpg", 1536));
    ConsistencyFile extentless;
    static const std::u16string ename = u"empty.jpg";
    extentless.name = &ename;
    extentless.has_extent = false; /* considered, not filterable */
    files.push_back(extentless);
    const ConsistencyVerdict v =
        score_content_consistency(&dev, files.data(), files.size());
    EXPECT_EQ(2u, v.considered) << "directory excluded, extentless included";
    EXPECT_EQ(1u, v.filterable);
    EXPECT_EQ(0u, v.matched);
    /* considered(2)/2 = 1 <= filterable(1) && ratio 0 < 0.3 -> reject */
    EXPECT_TRUE(v.rejected);
}

TEST(Lost, DataStartOutOfBoundsInvalidatesTheCandidate) {
    /* the og's *param_3 invalidation (:31077-31083): cluster math past the
     * volume end means structurally impossible geometry - reject outright. */
    MemDevice dev(scorer_volume()); /* 8192 bytes */
    std::vector<ConsistencyFile> files;
    files.push_back(mk_file(u"bad.jpg", 999999)); /* beyond the volume */
    const ConsistencyVerdict v =
        score_content_consistency(&dev, files.data(), files.size());
    EXPECT_TRUE(v.out_of_bounds);
    EXPECT_TRUE(v.rejected);
}

TEST(Lost, UnreadableDataStartStaysUnmatched) {
    /* a failing read at the data start is NOT an error: the og's sig map
     * has no entry at unreadable offsets -> filterable-but-unmatched. */
    struct FailingDevice final : IStorageDevice {
        u32 read_at(u64, void*, u32) override { return ERR_READ_FAILED; }
        const Geometry& geometry() const override { return geom_; }
        Geometry geom_{};
    } dev;
    dev.geom_.logical_sector = 512;
    dev.geom_.physical_sector = 512;
    dev.geom_.sector_count = 16;
    dev.geom_.total_bytes = 8192;
    std::vector<ConsistencyFile> files;
    files.push_back(mk_file(u"bad.jpg", 512));
    files.push_back(mk_file(u"bad2.jpg", 1024));
    const ConsistencyVerdict v =
        score_content_consistency(&dev, files.data(), files.size());
    EXPECT_EQ(2u, v.considered);
    EXPECT_EQ(2u, v.filterable);
    EXPECT_EQ(0u, v.matched);
    EXPECT_TRUE(v.rejected); /* 2/2 filterable, ratio 0 */
    EXPECT_FALSE(v.out_of_bounds);
}

TEST(Lost, EmptyCandidateListNeverRejects) {
    MemDevice dev(scorer_volume());
    const ConsistencyVerdict v = score_content_consistency(&dev, nullptr, 0);
    EXPECT_FALSE(v.rejected);
    EXPECT_EQ(0u, v.considered);
}

} // namespace

/* ---- the detach_subtree mechanism (M-035's tree surgery) --------------- */

namespace {

FileProperties* mk_props(const char16_t* name, u64 record, u64 ext_off, u64 size) {
    auto* p = new FileProperties();
    p->name = name;
    p->record_ref = record;
    p->logical_size = size;
    FileProperties::Extent e;
    e.offset = ext_off;
    e.length = size;
    p->extents.push_back(e);
    return p;
}

TEST(TreeDetach, RemovesNodesRootsAndIndexes) {
    ResultTree tree;
    auto* keep = tree.add_root(u"keep");
    auto* drop = tree.add_root(u"drop");
    auto* k1 = mk_props(u"a.jpg", 101, 512, 10);
    auto* d1 = mk_props(u"b.jpg", 202, 1024, 20);
    auto* d2 = mk_props(u"c.jpg", 303, 2048, 30);
    ASSERT_NE(nullptr, tree.add_file(keep, k1));
    ASSERT_NE(nullptr, tree.add_file(drop, d1));
    ASSERT_NE(nullptr, tree.add_file(drop, d2));
    k1->release();
    d1->release();
    d2->release();
    ASSERT_EQ(5u, tree.node_count()); /* 2 roots + 3 files */

    tree.detach_subtree(drop);

    EXPECT_EQ(2u, tree.node_count()) << "root + surviving file only";
    EXPECT_EQ(1u, tree.roots().size());
    EXPECT_EQ(keep, tree.roots()[0]);
    /* file_count counts every props-bearing non-directory node, roots
     * included (the tree's existing semantics) */
    EXPECT_EQ(2u, tree.file_count());
    EXPECT_EQ(nullptr, tree.find_by_record(202));
    EXPECT_EQ(nullptr, tree.find_by_record(303));
    EXPECT_NE(nullptr, tree.find_by_record(101));
    /* the dropped root must look empty to subtree walkers (links severed
     * before destruction - the orchestrator's per-candidate rebase and
     * the snapshot DFS walk safely through it) */
    EXPECT_EQ(nullptr, drop->first_child());
    /* the surviving index entry is intact */
    EXPECT_NE(nullptr, tree.find_by_extent(512, 0));
    EXPECT_EQ(nullptr, tree.find_by_extent(1024, 0));
    EXPECT_EQ(nullptr, tree.find_by_extent(2048, 0));
    /* stats refresh reflects the removal */
    tree.refresh_stats();
    EXPECT_EQ(1u, keep->stats().file_count);
}

TEST(TreeDetach, HardLinkPropsSurviveWithTheirRefs) {
    /* two nodes sharing ONE props object (hard link): one under the
     * detached root, one under a surviving root. The detach releases the
     * detached node's ref exactly once - the surviving node's props stay
     * valid (the UAF-class guard for the refcount). */
    ResultTree tree;
    auto* keep = tree.add_root(u"keep");
    auto* drop = tree.add_root(u"drop");
    FileProperties* shared = mk_props(u"link.jpg", 77, 512, 10);
    FileNode* n1 = tree.add_file(keep, shared);
    FileNode* n2 = tree.add_file(drop, shared);
    shared->release(); /* the tree holds both refs now */
    ASSERT_NE(nullptr, n1);
    ASSERT_NE(nullptr, n2);

    tree.detach_subtree(drop);

    EXPECT_EQ(1u, tree.roots().size());
    /* keep-root + the surviving link node (roots count - see above) */
    EXPECT_EQ(2u, tree.file_count());
    /* the surviving node's props are alive and correct */
    ASSERT_NE(nullptr, keep->first_child());
    EXPECT_EQ(u"link.jpg", keep->first_child()->props()->name);
    EXPECT_EQ(77u, keep->first_child()->props()->record_ref);
    /* record lookup: by_record_ is last-writer-wins at insert time - n2
     * (the detached node) shadowed n1 when it was added; the detach
     * erased the shadowed entry, so the record now resolves to nothing.
     * The SURVIVING node itself remains fully usable (props above). */
    EXPECT_EQ(nullptr, tree.find_by_record(77));
}

} // namespace

/* ---- the end-to-end wiring (orchestrator candidate scan) --------------- */

namespace {

/* A compact but WALKABLE lost exFAT volume: boot + checksum + FAT +
 * bitmap + upcase + root directory with `jpg_count` .JPG files whose
 * data clusters carry either JPEG magic (matched) or plain text
 * (mismatched). Planted at `plant_lba` inside a larger disk with no
 * table entry covering it (the lost-partition discovery target). */
struct ExfatGhostVolume {
    std::vector<u8> img; /* the standalone 2048-sector volume */
    u64 jpg_count = 0;
    u32 upcase_checksum = 0;
};

ExfatGhostVolume build_exfat_ghost(u32 jpg_count, bool matched_content) {
    ExfatGhostVolume g;
    g.jpg_count = jpg_count;
    std::vector<u8>& img = g.img;
    img.assign(u64(2048) * 512, 0);

    /* boot sector */
    u8* b = img.data();
    b[0] = 0xEB;
    b[1] = 0x76;
    b[2] = 0x90;
    std::memcpy(b + 3, "EXFAT   ", 8);
    put64(img, 64, 0);    /* partition offset */
    put64(img, 72, 2048); /* volume length */
    put32v(img, 80, 24);  /* fat offset */
    put32v(img, 84, 1);   /* fat length */
    put32v(img, 88, 25);  /* cluster heap offset */
    put32v(img, 92, 100); /* cluster count */
    put32v(img, 96, 2);   /* root first cluster */
    put32v(img, 100, 0x12345678);
    put16(img, 104, 0x0100);
    put16(img, 106, 0);
    b[108] = 9; /* 512B sectors */
    b[109] = 0; /* 1 sector per cluster */
    b[110] = 1; /* 1 FAT */
    b[111] = 0x80;
    b[112] = 0xFF;
    b[510] = 0x55;
    b[511] = 0xAA;
    std::memcpy(img.data() + 12 * 512, b, 512);
    for (u32 s = 1; s <= 8; ++s) {
        img[s * 512 + 510] = 0x55;
        img[s * 512 + 511] = 0xAA;
        img[(12 + s) * 512 + 510] = 0x55;
        img[(12 + s) * 512 + 511] = 0xAA;
    }
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

    /* FAT: root chain 2 -> 3, bitmap 50, upcase 60, file clusters */
    const size_t FAT = 24 * 512;
    put32v(img, FAT + 2 * 4, 3);
    put32v(img, FAT + 3 * 4, 0xFFFFFFFF);
    put32v(img, FAT + 50 * 4, 0xFFFFFFFF);
    put32v(img, FAT + 60 * 4, 0xFFFFFFFF);

    /* allocation bitmap at cluster 50 (sector 75) */
    const size_t BM = 25 * 512 + (50 - 2) * 512;
    auto setbit = [&](u32 c) { img[BM + (c - 2) / 8] |= u8(1u << ((c - 2) % 8)); };
    setbit(2);
    setbit(3);
    setbit(50);
    setbit(60);

    /* upcase table at cluster 60 (sector 83): 64-entry a-z fold */
    {
        u32 uc = 0;
        std::vector<u8> tbl(128, 0);
        for (u32 i = 0; i < 64; ++i) {
            const u32 ch = (i >= 'a' && i <= 'z') ? (i - 0x20) : i;
            put16(img, 83 * 512 + i * 2, char16_t(ch));
            tbl[i * 2] = u8(ch);
            tbl[i * 2 + 1] = u8(ch >> 8);
        }
        uc = 0;
        for (size_t i = 0; i < tbl.size(); ++i)
            uc = ((uc & 1) ? 0x80000000u : 0u) + (uc >> 1) + tbl[i];
        /* root dentry below stamps this checksum */
        g.upcase_checksum = uc;
    }

    /* root dir at cluster 2 (sector 25): bitmap + upcase + N file sets */
    const size_t ROOT = 25 * 512;
    size_t at = 0;
    {
        u8* e = img.data() + ROOT;
        e[0] = 0x81;
        put32v(img, ROOT + 20, 50);
        put64(img, ROOT + 24, 13);
        at += 32;
    }
    {
        u8* e = img.data() + ROOT + at;
        e[0] = 0x82;
        put32v(img, ROOT + at + 4, g.upcase_checksum);
        put32v(img, ROOT + at + 20, 60);
        put64(img, ROOT + at + 24, 128);
        at += 32;
    }

    const u32 TS = (0x5A46u << 16) | 0x6000u; /* 2025-02-06 12:00 (M-014) */
    char namebuf[32];
    for (u32 f = 0; f < jpg_count; ++f) {
        const u32 cluster = 10 + f; /* files at clusters 10..10+N-1 */
        snprintf(namebuf, sizeof(namebuf), "f%02u.jpg", f);
        const size_t set_at = ROOT + at;
        u8* fe = img.data() + set_at;
        fe[0] = 0x85;
        fe[1] = 2; /* stream + 1 name */
        put16(img, set_at + 4, 0x20); /* archive attribute */
        put32v(img, set_at + 8, TS);
        put32v(img, set_at + 12, TS);
        put32v(img, set_at + 16, TS);
        u8* se = img.data() + set_at + 32;
        se[0] = 0xC0;
        se[1] = 0x03; /* AllocPossible | NoFatChain */
        se[3] = 7;    /* name length */
        put64(img, set_at + 32 + 8, 256);  /* valid data */
        put32v(img, set_at + 32 + 20, cluster);
        put64(img, set_at + 32 + 24, 256); /* data length */
        u8* ne = img.data() + set_at + 64;
        ne[0] = 0xC1;
        for (size_t i = 0; i < 7; ++i)
            put16(img, set_at + 64 + 2 + i * 2, char16_t(namebuf[i]));
        /* SetCheckSum over the set (bytes 2..3 excluded) */
        u16 sum = 0;
        for (size_t byte = 0; byte < 96; ++byte) {
            if (byte >= 2 && byte < 4)
                continue;
            sum = u16(((sum & 1) ? 0x8000u : 0u) + (sum >> 1) + img[set_at + byte]);
        }
        put16(img, set_at + 2, sum);
        at += 96;

        setbit(cluster);
        /* data content at the cluster (sector 25 + (cluster-2)) */
        const size_t data = 25 * 512 + u64(cluster - 2) * 512;
        if (matched_content) {
            const u8 jpg[] = {0xFF, 0xD8, 0xFF, 0xE0, 0x00, 0x10, 'J', 'F', 'I', 'F'};
            std::memcpy(img.data() + data, jpg, sizeof(jpg));
        } else {
            std::memcpy(img.data() + data, "plain text, no signature", 24);
        }
    }
    return g;
}

TEST(Lost, WholeDiskScanDropsContentInconsistentExfatGhost) {
    /* the finding's end-to-end fixture: a lost exFAT volume with a valid
     * BPB whose .JPG files carry non-JPEG content. The og's engine-side
     * scorer rejects the candidate's files; ours must surface the
     * candidate (per-hit persistence, M-042) with NO files under it and
     * lost_content_rejected == 1. */
    auto ghost = build_exfat_ghost(6, false /* mismatched content */);
    std::vector<u8> disk(u64(8192) * 512, 0);
    /* live MBR pointing at an unrelated region (the ghost stays lost) */
    u8* mbr = disk.data();
    mbr[446 + 4] = 0x07;
    put32v(disk, 446 + 8, 2048);
    put32v(disk, 446 + 12, 1024);
    mbr[510] = 0x55;
    mbr[511] = 0xAA;
    std::memcpy(disk.data() + u64(4096) * 512, ghost.img.data(), ghost.img.size());

    MemDevice dev(std::move(disk));
    ResultTree tree;
    ScanConfig cfg;
    cfg.run_quick = true;
    cfg.run_deep = true;
    cfg.run_carve = false;
    cfg.whole_disk = true;
    cfg.threads = 1;
    ScanController ctrl(tree, cfg);
    ScanStats stats;
    ASSERT_EQ(ctrl.run(&dev, nullptr, nullptr, nullptr, stats), OK);

    EXPECT_EQ(1u, stats.lost_content_rejected) << "the ghost candidate must be content-rejected";
    /* the candidate stays listed (per-hit persistence) */
    ASSERT_EQ(1u, stats.lost_candidates.size());
    EXPECT_EQ(CandidateFs::Exfat, stats.lost_candidates[0].fs);
    /* ... but NONE of its files surface (the subtree was detached) */
    EXPECT_EQ(0u, stats.files_found) << "the rejected candidate's files must not count";
    EXPECT_EQ(0u, stats.deleted_found);
    size_t files_under_candidates = 0;
    std::function<void(FileNode*)> count = [&](FileNode* n) {
        for (FileNode* c = n->first_child(); c; c = c->next_sibling()) {
            if (c->props() && !(c->props()->attributes & 0x10))
                ++files_under_candidates;
            count(c);
        }
    };
    for (FileNode* r : tree.roots())
        count(r);
    EXPECT_EQ(0u, files_under_candidates) << "no file rows anywhere in the tree";
}

TEST(Lost, WholeDiskScanKeepsContentConsistentExfatGhost) {
    /* the over-rejection guard: same volume, JPEG magic under every
     * .JPG name -> ratio 1.0 -> the candidate's files SURFACE. */
    auto ghost = build_exfat_ghost(6, true /* matched content */);
    std::vector<u8> disk(u64(8192) * 512, 0);
    u8* mbr = disk.data();
    mbr[446 + 4] = 0x07;
    put32v(disk, 446 + 8, 2048);
    put32v(disk, 446 + 12, 1024);
    mbr[510] = 0x55;
    mbr[511] = 0xAA;
    std::memcpy(disk.data() + u64(4096) * 512, ghost.img.data(), ghost.img.size());

    MemDevice dev(std::move(disk));
    ResultTree tree;
    ScanConfig cfg;
    cfg.run_quick = true;
    cfg.run_deep = true;
    cfg.run_carve = false;
    cfg.whole_disk = true;
    cfg.threads = 1;
    ScanController ctrl(tree, cfg);
    ScanStats stats;
    ASSERT_EQ(ctrl.run(&dev, nullptr, nullptr, nullptr, stats), OK);
    EXPECT_EQ(0u, stats.lost_content_rejected);
    ASSERT_EQ(1u, stats.lost_candidates.size());
    EXPECT_EQ(6u, stats.files_found) << "all six .JPG files surface";
    EXPECT_EQ(0u, stats.deleted_found);
}

} // namespace
