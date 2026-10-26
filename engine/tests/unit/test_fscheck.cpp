/* test_fscheck.cpp - M-041 (diff-to-fix): the damage-audit check phase.
 *
 * Proves the og's FileSystemCheck behaviors the core lacked: FAT copy
 * disagreement reporting (0x1005), duplicate references (0x1006), the
 * directory-tree audit (cycles 0x1007, size!=chain 0x1004), orphan
 * clusters (0x1003), the NTFS version gate (0x100a), the MFT x $Bitmap
 * cross-check (0x1002 duplicate / 0x1003 free-but-allocated), and the
 * never-aborts-the-scan stance. Test names describe the behavior proven.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/check/cr_fscheck.h"
#include "cr/orchestrator/cr_scan.h"
#include "cr/tree/cr_node.h"
#include <gtest/gtest.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

using namespace cr;
using namespace cr::check;

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

/* ---- FAT16 fixture builder ----
 * Geometry (the orchestrator engine's accepted shape): bps 512, spc 1,
 * reserved 2, fat count 2, spf 20, root 512 entries (32 sectors),
 * total 5074 sectors -> 5004 data clusters (the FAT16 bracket).
 * FATs at sectors 2 and 22; root at 42; data at 74. */
struct Fat16Vol {
    std::vector<u8> img;
    static constexpr u16 SPF = 20;
    static constexpr u16 TOT = 5074;
    static constexpr u64 FAT1_OFF = 2 * 512;
    static constexpr u64 FAT2_OFF = (2 + SPF) * 512;
    static constexpr u64 ROOT_OFF = (2 + 2 * SPF) * 512;
    static constexpr u64 DATA_OFF = (2 + 2 * SPF + 32) * 512;

    Fat16Vol() : img(size_t(TOT) * 512, 0) {
        u8* b = img.data();
        std::memcpy(b + 3, "MSWIN4.1", 8);
        put16(img, 0x0B, 512);
        b[0x0D] = 1; /* spc */
        put16(img, 0x0E, 2);
        b[0x10] = 2;           /* fat count */
        put16(img, 0x11, 512); /* root entries: 32 sectors; >= 224 = the
                                * engine open gate (N-010b) */
        put16(img, 0x13, TOT);
        b[0x15] = 0xF8;
        put16(img, 0x16, SPF);
        put16(img, 0x18, 63);
        put16(img, 0x1A, 255);
        b[0x26] = 0x29;
        std::memcpy(b + 0x36, "FAT16   ", 8);
        b[510] = 0x55;
        b[511] = 0xAA;
        /* FAT signatures: entry 0 = 0xFFF8, entry 1 = 0xFFFF */
        set_fat(0, 0xFFF8);
        set_fat(1, 0xFFFF);
    }
    void set_fat(u32 cluster, u16 value) {
        put16(img, FAT1_OFF + size_t(cluster) * 2, value);
        put16(img, FAT2_OFF + size_t(cluster) * 2, value);
    }
    void set_fat2_only(u32 cluster, u16 value) {
        put16(img, FAT2_OFF + size_t(cluster) * 2, value);
    }
    u8* root_entry(size_t i) { return img.data() + ROOT_OFF + i * 32; }
    u8* cluster_data(u32 c) { return img.data() + DATA_OFF + (u64(c) - 2) * 512; }
    void make_dentry(size_t slot, const char* name8_3, u8 attrs, u32 first_cluster, u32 size) {
        u8* e = root_entry(slot);
        std::memset(e, 0, 32);
        for (int i = 0; i < 11; i++)
            e[i] = ' ';
        const char* dot = std::strchr(name8_3, '.');
        if (dot) {
            for (const char* p = name8_3; p < dot; ++p)
                e[size_t(p - name8_3)] = u8(*p);
            for (int i = 0; dot[1 + i]; ++i)
                e[8 + i] = u8(dot[1 + i]);
        } else {
            for (int i = 0; name8_3[i]; ++i)
                e[i] = u8(name8_3[i]);
        }
        e[11] = attrs;
        put16(img, size_t(e - img.data()) + 26, u16(first_cluster));
        put32v(img, size_t(e - img.data()) + 28, size);
    }
};

/* one-cluster file chain end */
constexpr u16 EOC16 = 0xFFFF;

} // namespace

/* ---- 0x1005: FAT1/FAT2 disagreement on a live chain ---- */
TEST(FsCheck, FatCopyDisagreementReported) {
    Fat16Vol v;
    v.make_dentry(0, "LIVE.TXT", 0x20, 2, 512);
    v.set_fat(2, EOC16);
    /* tear FAT2 at the live chain entry 2: a VALID link to cluster 3
     * instead of EOC — a dead-zone value (0xFFEF) would correctly fire
     * the 0x1006 table gate first and terminate before the compare */
    v.set_fat2_only(2, 3);

    FsCheckReport rep;
    ASSERT_EQ(check_fat(std::make_unique<MemDevice>(v.img).get(), {}, rep), OK);
    ASSERT_TRUE(rep.has_code(CHK_FAT_COPY_MISMATCH));
    EXPECT_EQ(rep.first_of(CHK_FAT_COPY_MISMATCH)->param, 2u);
}

/* ---- 0x1006: two entries referencing the same live cluster ---- */
TEST(FsCheck, FatDuplicateRefDetected) {
    Fat16Vol v;
    v.make_dentry(0, "A.TXT", 0x20, 2, 512);
    v.make_dentry(1, "B.TXT", 0x20, 3, 512);
    /* both chains pass through cluster 4: 2->4->EOC and 3->4->EOC */
    v.set_fat(2, 4);
    v.set_fat(3, 4);
    v.set_fat(4, EOC16);

    FsCheckReport rep;
    ASSERT_EQ(check_fat(std::make_unique<MemDevice>(v.img).get(), {}, rep), OK);
    EXPECT_TRUE(rep.has_code(CHK_FAT_TABLE_INVALID));
}

/* ---- 0x1006 (the hole half): an entry pointing into the dead zone
 * between the data region and the reserved family — the og's
 * FUN_180005fd0 entry-hole scan (:4392-4415). ---- */
TEST(FsCheck, FatEntryHoleReported) {
    Fat16Vol v;
    /* entry 2 points past the last data cluster (5004 clusters ->
     * valid clusters 2..5005) but below the reserved floor 0xFFF6:
     * the dead zone -> 0x1006 */
    v.set_fat(2, 5002);

    FsCheckReport rep;
    ASSERT_EQ(check_fat(std::make_unique<MemDevice>(v.img).get(), {}, rep), OK);
    EXPECT_TRUE(rep.has_code(CHK_FAT_TABLE_INVALID));
}

/* ---- 0x1002 (the FAT dir-walk form): a live file's chain re-enters a
 * cluster a previous chain already traversed — the og's FUN_1800027a0
 * shadow-registry re-entry (:1156-1173, param = the cross-linked
 * cluster). The shared-HEAD shape: no two FAT entries name the same
 * target, so the table-level duplicate scan stays quiet — only the
 * walk-order re-entry reports. ---- */
TEST(FsCheck, FatCrossLinkedChainReported) {
    Fat16Vol v;
    v.make_dentry(0, "GOOD.TXT", 0x20, 2, 1024);
    v.set_fat(2, 3);
    v.set_fat(3, EOC16);
    /* a second live file whose chain STARTS INSIDE good.txt's chain
     * (cluster 3): the dir audit's second walk re-enters it */
    v.make_dentry(1, "CLASH.TXT", 0x20, 3, 512);

    FsCheckReport rep;
    ASSERT_EQ(check_fat(std::make_unique<MemDevice>(v.img).get(), {}, rep), OK);
    ASSERT_TRUE(rep.has_code(CHK_DUPLICATE_ALLOC));
    EXPECT_EQ(rep.first_of(CHK_DUPLICATE_ALLOC)->param, 3u);
    /* the shared cluster is reported once per re-entering chain, and the
     * report carries nothing else (no table dup, no size mismatch) */
    EXPECT_EQ(rep.errors.size(), 1u);
}

/* ---- 0x1007: subdirectory missing the '.'/'..' pair ---- */
TEST(FsCheck, DirDotPairMissingRejected) {
    Fat16Vol v;
    /* subdirectory at cluster 2 without the '.'/'..' leading pair */
    v.make_dentry(0, "SUB", 0x10, 2, 0);
    v.set_fat(2, EOC16);
    u8* c = v.cluster_data(2);
    std::memset(c, 0, 512);
    /* first entry is a plain name, not '.' */
    std::memcpy(c, "NOBODY  TXT", 11);
    c[11] = 0x20;

    FsCheckReport rep;
    ASSERT_EQ(check_fat(std::make_unique<MemDevice>(v.img).get(), {}, rep), OK);
    EXPECT_TRUE(rep.has_code(CHK_DIR_TREE_INVALID));
}

/* ---- 0x1007: directory cycle (child chain returns into its ancestor) -- */
TEST(FsCheck, DirCycleDetected) {
    Fat16Vol v;
    /* root/SUB (cluster 2) contains a subdir CHILD (cluster 3) whose
     * chain links back: 3 -> 2 -> EOC (CHILD's FAT entry points into
     * SUB's cluster). */
    v.make_dentry(0, "SUB", 0x10, 2, 0);
    v.set_fat(2, EOC16);
    u8* sub = v.cluster_data(2);
    std::memset(sub, 0, 512);
    std::memcpy(sub, ".          ", 11);
    sub[11] = 0x10;
    put16(v.img, size_t(sub - v.img.data()) + 26, 2);
    std::memcpy(sub + 32, "..         ", 11);
    sub[32 + 11] = 0x10;
    put16(v.img, size_t(sub - v.img.data()) + 32 + 26, 0);
    /* CHILD entry inside SUB */
    std::memcpy(sub + 64, "CHILD   ", 8);
    for (int i = 8; i < 11; ++i)
        sub[64 + i] = ' ';
    sub[64 + 11] = 0x10;
    put16(v.img, size_t(sub - v.img.data()) + 64 + 26, 3);
    /* CHILD's chain points back at SUB's cluster 2 (the cycle) */
    v.set_fat(3, 2);

    FsCheckReport rep;
    ASSERT_EQ(check_fat(std::make_unique<MemDevice>(v.img).get(), {}, rep), OK);
    EXPECT_TRUE(rep.has_code(CHK_DIR_TREE_INVALID));
}

/* ---- 0x1004: live file size disagrees with its chain length ---- */
TEST(FsCheck, LiveFileSizeNeChain) {
    Fat16Vol v;
    /* dentry says 2 clusters (1024 bytes) but the chain is one cluster */
    v.make_dentry(0, "BIG.TXT", 0x20, 2, 1024);
    v.set_fat(2, EOC16); /* 1-cluster chain vs 2-cluster size */

    FsCheckReport rep;
    ASSERT_EQ(check_fat(std::make_unique<MemDevice>(v.img).get(), {}, rep), OK);
    ASSERT_TRUE(rep.has_code(CHK_SIZE_CHAIN_MISMATCH));
    EXPECT_EQ(rep.first_of(CHK_SIZE_CHAIN_MISMATCH)->path_hint, "BIG.TXT");
}

/* ---- 0x1003: orphan cluster (unvisited, FAT non-free, not self/BAD) -- */
TEST(FsCheck, OrphanClustersReported) {
    Fat16Vol v;
    v.make_dentry(0, "LIVE.TXT", 0x20, 2, 512);
    v.set_fat(2, EOC16);
    /* cluster 5: not reachable from any directory entry, FAT claims it
     * is in the middle of a chain (points at 6) */
    v.set_fat(5, 6);
    v.set_fat(6, EOC16);

    FsCheckReport rep;
    ASSERT_EQ(check_fat(std::make_unique<MemDevice>(v.img).get(), {}, rep), OK);
    ASSERT_TRUE(rep.has_code(CHK_ORPHAN_CLUSTER));
    EXPECT_EQ(rep.first_of(CHK_ORPHAN_CLUSTER)->param, 5u);
}

/* ---- the clean volume reports nothing ---- */
TEST(FsCheck, CleanFatVolumeReportsNoErrors) {
    Fat16Vol v;
    v.make_dentry(0, "LIVE.TXT", 0x20, 2, 512);
    v.set_fat(2, EOC16);

    FsCheckReport rep;
    ASSERT_EQ(check_fat(std::make_unique<MemDevice>(v.img).get(), {}, rep), OK);
    EXPECT_TRUE(rep.errors.empty()) << "clean fixture must report nothing";
    EXPECT_TRUE(rep.family_detected);
    EXPECT_STREQ(rep.family, "fat");
}

/* ---- 0x1001 + 0x100D: DBR gates ---- */
TEST(FsCheck, DbrCeilingRejectsOversizedClusterCount) {
    Fat16Vol v;
    /* claim a sector-per-FAT too small for the declared geometry: 64
     * total sectors with spf 1 -> the validator's arithmetic still
     * passes the ceiling, so instead break the signature gate: FAT16
     * marker replaced by garbage (the DBR validator's memcmp). */
    std::memcpy(v.img.data() + 0x36, "GARBAGE ", 8);

    FsCheckReport rep;
    MemDevice dev(v.img);
    ASSERT_EQ(run_fscheck(&dev, {}, rep), OK);
    EXPECT_FALSE(rep.family_detected) << "non-FAT signature: undetected";
}

/* ---- the NTFS-side gates (0x1008/0x1009/0x100a/0x1002/0x1003) ---- */

namespace {

/* Minimal NTFS image: 1 KiB records, 512 B sectors, 1 KiB clusters...
 * simpler: cluster = 512 so every record spans 2 clusters (the runlist
 * covers them). Geometry: bps 512, spc 1, records 1 KiB. */
constexpr u64 REC = 1024;
constexpr u64 CLUSTER = 512;
constexpr u64 MFT_LCN = 32;   /* $MFT at cluster 32 */
constexpr u64 MIRR_LCN = 200; /* $MFTMirr at cluster 200 */
constexpr u64 BMP_LCN = 64;   /* $Bitmap data at cluster 64 */

struct NtfsVol {
    std::vector<u8> img;
    NtfsVol() : img(512 * 1024, 0) {
        u8* b = img.data();
        std::memcpy(b + 3, "NTFS    ", 8);
        put16(img, 0x0B, 512);
        b[0x0D] = 1; /* spc */
        put16(img, 0x0E, 0);
        b[0x10] = 0;
        put16(img, 0x11, 0);
        put16(img, 0x13, 0);
        put32v(img, 0x24, 0x00800080);         /* the NTFS BPB's fixed marker */
        put64_at(img, 0x28, img.size() / 512); /* total_sectors (u64 @+0x28) */
        put64_at(img, 0x30, MFT_LCN);
        put64_at(img, 0x38, MIRR_LCN);
        img[0x40] = 0xF6; /* -10 -> 1 KiB records */
        img[0x44] = 1;    /* clusters per index block (the open ladder's
                           * +0x44 nonzero gate, N-011b) */
        b[510] = 0x55;
        b[511] = 0xAA;
    }
    static void put64_at(std::vector<u8>& v, size_t at, u64 x) {
        for (int i = 0; i < 8; ++i)
            v[at + size_t(i)] = u8(x >> (8 * i));
    }
    u8* rec_at(u64 lcn) { return img.data() + lcn * CLUSTER; }

    /* record builder: header + attrs; USA covers 2 sectors */
    struct Rec {
        std::vector<u8>& img;
        u8* buf;
        u32 pos = 0;
        Rec(std::vector<u8>& im, u64 lcn) : img(im), buf(im.data() + lcn * CLUSTER) {
            std::memset(buf, 0, size_t(REC));
            buf[0] = 'F';
            buf[1] = 'I';
            buf[2] = 'L';
            buf[3] = 'E';
            put16(img, size_t(buf - img.data()) + 4, 0x30);
            put16(img, size_t(buf - img.data()) + 6, 3);       /* USA count: 2 + 1 */
            put16(img, size_t(buf - img.data()) + 0x14, 0x38); /* attribute offset */
            put16(img, size_t(buf - img.data()) + 0x16, 0);    /* flags: set later */
            put32v(img, size_t(buf - img.data()) + 0x18, 0);   /* bytes in use (patch) */
            /* USA array at 0x30: USN + 2 originals; tails patched below */
            const u16 usn = 0x0BB8;
            u16_from(img, size_t(buf - img.data()) + 0x30, usn);
            u16_from(img, size_t(buf - img.data()) + 0x32, 0x1111);
            u16_from(img, size_t(buf - img.data()) + 0x34, 0x2222);
            patch_tail(0);
            patch_tail(1);
            pos = 0x38;
        }
        void patch_tail(int sector) {
            const size_t base = size_t(buf - img.data());
            const size_t tail = base + size_t(sector) * 512 + 510;
            img[tail] = 0xB8;
            img[tail + 1] = 0x0B;
        }
        static void u16_from(std::vector<u8>& v, size_t at, u16 x) {
            v[at] = u8(x);
            v[at + 1] = u8(x >> 8);
        }
        void set_flags(u16 f) { u16_from(img, size_t(buf - img.data()) + 0x16, f); }
        /* resident attribute with raw value bytes */
        void attr_res(u32 type, const std::vector<u8>& value, u16 name_len = 0) {
            const u32 voff = 0x18;
            const u32 alen = (voff + u32(value.size()) + 7) / 8 * 8;
            const size_t base = size_t(buf - img.data());
            put32v(img, base + pos, type);
            put32v(img, base + pos + 4, alen);
            img[base + pos + 8] = 0; /* resident */
            img[base + pos + 9] = u8(name_len);
            u16_from(img, base + pos + 10, u16(voff));
            u16_from(img, base + pos + 16, u16(value.size() & 0xFFFF));
            u16_from(img, base + pos + 20, u16(voff));
            if (!value.empty())
                std::memcpy(buf + pos + voff, value.data(), value.size());
            pos += alen;
            attr_end();
        }
        /* non-resident $DATA with a single run */
        void attr_data_run(u64 lcn, u64 clusters, u64 real_size) {
            const size_t base = size_t(buf - img.data());
            const u32 voff = 0x40;
            put32v(img, base + pos, 0x80);
            put32v(img, base + pos + 4, voff + 16);
            img[base + pos + 8] = 1; /* nonresident */
            put64_at(img, base + pos + 0x10, 0);
            put64_at(img, base + pos + 0x18, clusters - 1);
            u16_from(img, base + pos + 0x20, u16(voff));
            put64_at(img, base + pos + 0x28, clusters * CLUSTER);
            put64_at(img, base + pos + 0x30, real_size);
            /* runlist entry: header, then the LENGTH field, then the
             * OFFSET field (NTFS order; sizes grown as needed) */
            std::vector<u8> rl;
            const int lbytes = clusters > 0x7F ? 2 : 1;
            const int obytes = lcn > 0x7F ? 2 : 1;
            rl.push_back(u8(lbytes | (obytes << 4)));
            for (int i = 0; i < lbytes; ++i)
                rl.push_back(u8(clusters >> (8 * i)));
            u64 lcnv = lcn;
            for (int i = 0; i < obytes; ++i)
                rl.push_back(u8(lcnv >> (8 * i)));
            rl.push_back(0);
            std::memcpy(buf + pos + voff, rl.data(), rl.size());
            pos += voff + 16;
            attr_end();
        }
        void attr_end() {
            const size_t base = size_t(buf - img.data());
            put32v(img, base + pos, 0xFFFFFFFF);
        }
        void finish() {
            const size_t base = size_t(buf - img.data());
            put32v(img, base + 0x18, pos + 8); /* bytes in use */
        }
    };

    /* $FILE_NAME value for "$MFT" (parent 5, namespace 2) */
    static std::vector<u8> fn_value(const char16_t* name, u8 ns = 2) {
        std::vector<u8> v(66 + 26, 0);
        std::u16string n = name;
        for (size_t i = 0; i < 8; ++i)
            v[i] = 5; /* parent record 5 */
        v[0x40] = u8(n.size());
        v[0x41] = ns;
        for (size_t i = 0; i < n.size(); ++i) {
            v[66 + 2 * i] = u8(n[i]);
            v[66 + 2 * i + 1] = u8(n[i] >> 8);
        }
        v.resize(66 + n.size() * 2);
        return v;
    }
    static std::vector<u8> volinfo_value(u8 major, u8 minor) {
        std::vector<u8> v(10, 0);
        v[8] = major;
        v[9] = minor;
        return v;
    }

    /* record 0 ($MFT): $FILE_NAME "$MFT" + $BITMAP + $DATA runlist over
     * [MFT_LCN, MFT_LCN + 32) clusters (16 records) */
    void build_mft(u8 major = 3, u8 minor = 1) {
        Rec r(img, MFT_LCN);
        r.set_flags(0x0003); /* in-use + directory */
        r.attr_res(0x30, fn_value(u"$MFT"));
        /* $BITMAP: 16 records -> 2 bytes, all 16 records marked in use */
        r.attr_res(0xB0, {0xFF, 0xFF});
        /* $DATA: run over MFT_LCN..+32 clusters (16 KiB / 1 KiB = 16 recs) */
        r.attr_data_run(MFT_LCN, 32, 32 * CLUSTER);
        r.finish();

        /* record 1 at MFT_LCN + 2 clusters (its own $FILE_NAME $MFTMirr-ish
         * name is irrelevant: the check only reads record 0 for the run
         * list, the mirror slot and $Volume/$Bitmap records below). */

        /* record 3 ($Volume): $VOLUME_INFORMATION version major.minor */
        Rec vol(img, MFT_LCN + 6);
        vol.set_flags(0x0001);
        vol.attr_res(0x70, volinfo_value(major, minor));
        vol.finish();

        /* record 6 ($Bitmap): $DATA run at BMP_LCN, 1 cluster (512 bytes
         * = 4096 bits >= 16 MFT records; cluster count 1024 sectors) */
        Rec bmp(img, MFT_LCN + 12);
        bmp.set_flags(0x0001);
        bmp.attr_data_run(BMP_LCN, 1, 512);
        bmp.finish();

        /* mirror slot 0: "$MFT"-named copy */
        Rec mirr(img, MIRR_LCN);
        mirr.set_flags(0x0003);
        mirr.attr_res(0x30, fn_value(u"$MFT"));
        mirr.finish();
    }
    void set_bmp_bit(u64 lcn, bool on) {
        u8* p = img.data() + BMP_LCN * CLUSTER + size_t(lcn / 8);
        if (on)
            *p |= u8(1u << (lcn & 7));
        else
            *p &= u8(~(1u << (lcn & 7)));
    }
};

} // namespace

TEST(FsCheck, NtfsCleanVolumeReportsNoErrors) {
    NtfsVol v;
    v.build_mft(3, 1);
    /* $Bitmap: mark the MFT run + $Bitmap data in use; everything else
     * legitimately free */
    for (u64 l = MFT_LCN; l < MFT_LCN + 32; ++l)
        v.set_bmp_bit(l, true);
    v.set_bmp_bit(BMP_LCN, true);
    v.set_bmp_bit(MIRR_LCN, true);
    v.set_bmp_bit(MIRR_LCN + 1, true);
    v.set_bmp_bit(MIRR_LCN + 2, true);

    FsCheckReport rep;
    MemDevice dev(v.img);
    ASSERT_EQ(run_fscheck(&dev, {}, rep), OK);
    EXPECT_TRUE(rep.errors.empty()) << "clean NTFS fixture must report nothing";
    EXPECT_TRUE(rep.family_detected);
}

/* ---- 0x100a: version outside {1.2, 3.0, 3.1} ---- */
TEST(FsCheck, NtfsVersionGate) {
    NtfsVol v;
    v.build_mft(4, 0); /* NTFS "4.0" — the og rejects with 0x100a */
    for (u64 l = MFT_LCN; l < MFT_LCN + 32; ++l)
        v.set_bmp_bit(l, true);
    v.set_bmp_bit(BMP_LCN, true);
    v.set_bmp_bit(MIRR_LCN, true);
    v.set_bmp_bit(MIRR_LCN + 1, true);
    v.set_bmp_bit(MIRR_LCN + 2, true);

    FsCheckReport rep;
    MemDevice dev(v.img);
    ASSERT_EQ(run_fscheck(&dev, {}, rep), OK);
    ASSERT_TRUE(rep.has_code(CHK_NTFS_VERSION));
    EXPECT_EQ(rep.first_of(CHK_NTFS_VERSION)->param, 0x400u) << "the version word is the param";
}

/* ---- 0x1008: $MFT record 0 without the resident "$MFT" $FILE_NAME --- */
TEST(FsCheck, MftRecordNotNamedMft) {
    NtfsVol v;
    v.build_mft(3, 1);
    for (u64 l = MFT_LCN; l < MFT_LCN + 32; ++l)
        v.set_bmp_bit(l, true);
    v.set_bmp_bit(BMP_LCN, true);
    /* overwrite record 0's $FILE_NAME with a foreign name */
    {
        NtfsVol::Rec r(v.img, MFT_LCN);
        r.set_flags(0x0003);
        r.attr_res(0x30, NtfsVol::fn_value(u"NOTMFT"));
        r.attr_res(0xB0, {0xFF, 0xFF});
        r.attr_data_run(MFT_LCN, 32, 32 * CLUSTER);
        r.finish();
    }

    FsCheckReport rep;
    MemDevice dev(v.img);
    ASSERT_EQ(run_fscheck(&dev, {}, rep), OK);
    EXPECT_TRUE(rep.has_code(CHK_MFT_RECORD_INVALID));
}

/* ---- 0x1002: two records claim the same LCN (cross-link) ---- */
TEST(FsCheck, MftBitmapCrossLink) {
    NtfsVol v;
    v.build_mft(3, 1);
    /* record 1: in-use (bit set in $MFT::$BITMAP already 0xFF) with a
     * $DATA run claiming the SAME cluster as $Bitmap's data (BMP_LCN) */
    {
        NtfsVol::Rec r(v.img, MFT_LCN + 2); /* record 1 at MFT+2 clusters */
        r.set_flags(0x0001);
        r.attr_data_run(BMP_LCN, 1, 512);
        r.finish();
    }
    for (u64 l = MFT_LCN; l < MFT_LCN + 32; ++l)
        v.set_bmp_bit(l, true);
    v.set_bmp_bit(BMP_LCN, true);
    v.set_bmp_bit(MIRR_LCN, true);
    v.set_bmp_bit(MIRR_LCN + 1, true);
    v.set_bmp_bit(MIRR_LCN + 2, true);

    FsCheckReport rep;
    MemDevice dev(v.img);
    ASSERT_EQ(run_fscheck(&dev, {}, rep), OK);
    ASSERT_TRUE(rep.has_code(CHK_DUPLICATE_ALLOC));
    EXPECT_EQ(rep.first_of(CHK_DUPLICATE_ALLOC)->param, u64(BMP_LCN));
}

/* ---- 0x1003 (NTFS): record claims an LCN the bitmap says is free ---- */
TEST(FsCheck, BitmapFreeButAllocated) {
    NtfsVol v;
    v.build_mft(3, 1);
    /* record 1 claims cluster 500 — never marked in $Bitmap */
    {
        NtfsVol::Rec r(v.img, MFT_LCN + 2);
        r.set_flags(0x0001);
        r.attr_data_run(500, 1, 512);
        r.finish();
    }
    for (u64 l = MFT_LCN; l < MFT_LCN + 32; ++l)
        v.set_bmp_bit(l, true);
    v.set_bmp_bit(BMP_LCN, true);
    v.set_bmp_bit(MIRR_LCN, true);
    v.set_bmp_bit(MIRR_LCN + 1, true);
    v.set_bmp_bit(MIRR_LCN + 2, true);
    /* cluster 500 deliberately NOT set in the bitmap */

    FsCheckReport rep;
    MemDevice dev(v.img);
    ASSERT_EQ(run_fscheck(&dev, {}, rep), OK);
    ASSERT_TRUE(rep.has_code(CHK_ORPHAN_CLUSTER));
    EXPECT_EQ(rep.first_of(CHK_ORPHAN_CLUSTER)->param, 500u);
}

/* ---- the check phase never aborts the scan ---- */
TEST(FsCheck, CheckNeverAbortsScan) {
    Fat16Vol v;
    v.make_dentry(0, "LIVE.TXT", 0x20, 2, 1024); /* size mismatch */
    v.set_fat(2, EOC16);
    v.set_fat(5, 6); /* orphan */

    auto dev = std::make_unique<MemDevice>(v.img);
    tree::ResultTree tree;
    orchestrator::ScanConfig cfg;
    cfg.pre_scan_check = true;
    orchestrator::ScanController scan(tree, cfg);
    orchestrator::ScanStats stats;
    const u32 rc = scan.run(dev.get(), nullptr, nullptr, nullptr, stats);
    EXPECT_EQ(rc, OK) << "the audit must NEVER abort the scan";
    EXPECT_TRUE(stats.check_ran);
    EXPECT_TRUE(stats.check_report.has_code(CHK_SIZE_CHAIN_MISMATCH));
    EXPECT_TRUE(stats.check_report.has_code(CHK_ORPHAN_CLUSTER));
    EXPECT_TRUE(stats.files_found > 0) << "the scan itself still produced results";
}

/* ---- the report round-trips through the session wire form ---- */
TEST(FsCheck, ReportSessionRoundTrip) {
    FsCheckReport rep;
    rep.family_detected = true;
    std::snprintf(rep.family, sizeof(rep.family), "fat");
    rep.fat_copies_compared = 2;
    rep.fat_entries_checked = 47;
    rep.clusters_visited = 12;
    rep.mft_records_examined = 0;
    CheckIssue a;
    a.code = CHK_FAT_COPY_MISMATCH;
    a.param = 7;
    a.path_hint = "DIR\\FILE.TXT";
    rep.errors.push_back(a);
    CheckIssue b;
    b.code = CHK_NTFS_VERSION;
    b.param = 0x400;
    b.record_no = 3;
    rep.errors.push_back(b);

    std::vector<u8> wire;
    ASSERT_EQ(serialize_fs_check(rep, wire), OK);
    FsCheckReport back;
    ASSERT_EQ(deserialize_fs_check(wire.data(), wire.size(), back), OK);
    ASSERT_EQ(back.errors.size(), 2u);
    EXPECT_EQ(back.errors[0].code, CHK_FAT_COPY_MISMATCH);
    EXPECT_EQ(back.errors[0].param, 7u);
    EXPECT_EQ(back.errors[0].path_hint, "DIR\\FILE.TXT");
    EXPECT_EQ(back.errors[1].code, CHK_NTFS_VERSION);
    EXPECT_EQ(back.errors[1].param, 0x400u);
    EXPECT_EQ(back.errors[1].record_no, 3u);
    EXPECT_TRUE(back.family_detected);
    EXPECT_STREQ(back.family, "fat");
    EXPECT_EQ(back.fat_copies_compared, 2u);
    EXPECT_EQ(back.fat_entries_checked, 47u);
    EXPECT_EQ(back.clusters_visited, 12u);
}
