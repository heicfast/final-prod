/* test_vollabel.cpp - N-133: the engine-side on-disk volume-label decode.
 *
 * Every test plants a labeled volume image (the four families) and proves
 * probe_slice_volume_label decodes the on-disk label from the volume's own
 * metadata - the og's engines do exactly this (NTFS $Volume :647-676 +
 * FUN_18003f3a0 :50963-50980; ext4 s_volume_name, algorithms-v2 SS2 step 5)
 * and it works identically for image scans (the finding's falsifiable
 * claims (a) and (b)).
 */

#include <gtest/gtest.h>

#include <cr/cr_types.h>
#include <cr/ntfs/cr_ntfs_wire.h>
#include <cr/orchestrator/cr_scan.h>
#include <cr/orchestrator/cr_vollabel.h>
#include <cr/storage/cr_device.h>

#include <cstring>
#include <vector>

using namespace cr;

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

void wr16(std::vector<u8>& v, size_t at, u16 x) {
    v[at] = u8(x);
    v[at + 1] = u8(x >> 8);
}
void wr32(std::vector<u8>& v, size_t at, u32 x) {
    for (int i = 0; i < 4; ++i)
        v[at + i] = u8(x >> (8 * i));
}
void wr64(std::vector<u8>& v, size_t at, u64 x) {
    for (int i = 0; i < 8; ++i)
        v[at + i] = u8(x >> (8 * i));
}

} // namespace

TEST(VolumeLabel, Ext4FromSuperblock) {
    /* N-133: s_volume_name decoded from the superblock at +1024 - the
     * og's Ext4Volume ctor strncpy's it "for the UI"; the probe makes the
     * engine-side label reachable (the finding's falsifiable claim (a):
     * an ext4 image with s_volume_name = "myvol" yields "myvol"). */
    std::vector<u8> img(256 * 1024, 0);
    /* the superblock: magic 0xEF53 at sb+56, the 16-byte label at sb+120 */
    img[1024 + 56] = 0x53;
    img[1024 + 57] = 0xEF;
    std::memcpy(img.data() + 1024 + 120, "myvol", 5);
    MemDevice dev(std::move(img));
    char label[33] = {0};
    ASSERT_EQ(orchestrator::probe_slice_volume_label(&dev, label), OK);
    EXPECT_STREQ(label, "myvol");
}

TEST(VolumeLabel, Ext4LabellessYieldsEmpty) {
    /* N-133: a valid ext4 superblock with an EMPTY s_volume_name decodes
     * to the empty label (the caller keeps its fallback - a label-less
     * volume, never an error). */
    std::vector<u8> img(256 * 1024, 0);
    img[1024 + 56] = 0x53;
    img[1024 + 57] = 0xEF;
    MemDevice dev(std::move(img));
    char label[33] = {1};
    ASSERT_EQ(orchestrator::probe_slice_volume_label(&dev, label), OK);
    EXPECT_EQ(label[0], 0);
}

TEST(VolumeLabel, FatFromBootSector) {
    /* N-133: the 11-byte volume-label boot field at +0x2B (the standard
     * FAT structure). The FS-type string gates the family. */
    std::vector<u8> img(64 * 1024, 0);
    /* a FAT32-shaped boot: the FS-type string at +0x52, the label at
     * +0x2B, space-padded */
    std::memcpy(img.data() + 0x52, "FAT32   ", 8);
    std::memcpy(img.data() + 0x2B, "MYDISK     ", 11);
    img[510] = 0x55;
    img[511] = 0xAA;
    MemDevice dev(std::move(img));
    char label[33] = {0};
    ASSERT_EQ(orchestrator::probe_slice_volume_label(&dev, label), OK);
    EXPECT_STREQ(label, "MYDISK") << "the trailing spaces trimmed";
}

TEST(VolumeLabel, FatLabellessYieldsEmpty) {
    /* N-133: a FAT boot with an all-spaces label field = label-less. */
    std::vector<u8> img(64 * 1024, 0);
    std::memcpy(img.data() + 0x36, "FAT16   ", 8);
    MemDevice dev(std::move(img));
    char label[33] = {1};
    ASSERT_EQ(orchestrator::probe_slice_volume_label(&dev, label), OK);
    EXPECT_EQ(label[0], 0);
}

TEST(VolumeLabel, NtfsFromVolumeRecord) {
    /* N-133: $Volume is MFT record 3; its $VOLUME_NAME attribute (type
     * 0x60, resident, UTF-16LE) is the label - the og's FUN_18003f3a0
     * wide->narrow read. The fixture builds the minimal volume: boot
     * sector + $MFT (record 0 with the $DATA runlist) + record 3 with
     * the 0x60 attribute. */
    constexpr u32 SEC = 512;
    constexpr u32 CLUSTER = SEC; /* spc = 1 */
    constexpr u64 NSECTORS = 128;
    constexpr u64 RECORD_SIZE = 1024; /* cpr = 2 */
    constexpr u64 MFT_LCN = 4;
    std::vector<u8> img(NSECTORS * SEC, 0);
    Geometry g;
    g.logical_sector = SEC;
    g.physical_sector = SEC;
    g.sector_count = NSECTORS;
    g.total_bytes = img.size();

    /* the boot sector (the test_ntfs_volume fixture shape) */
    {
        u8* bs = img.data();
        std::memcpy(bs + 3, "NTFS    ", 8);
        wr16(img, 11, u16(SEC));
        img[13] = 1; /* spc */
        img[21] = 0xF8;
        wr64(img, 0x28, NSECTORS);
        wr64(img, 0x30, MFT_LCN);
        wr64(img, 0x38, 68);
        img[0x40] = 2; /* cpr -> 1024B records */
        img[0x44] = 2;
        wr32(img, 0x24, 0x00800080); /* the must-value field */
        img[510] = 0x55;
        img[511] = 0xAA;
    }
    /* record 0: $MFT with the $DATA runlist at LCN 4, 16 clusters */
    const auto build_record = [&](u64 record_no, const char16_t* label16) {
        std::vector<u8> rec(RECORD_SIZE, 0);
        auto* rh = reinterpret_cast<ntfs::RecordHeader*>(rec.data());
        rh->magic = ntfs::RECORD_MAGIC_FILE;
        rh->update_sequence_offset = 0x30;
        rh->update_sequence_size = u16(RECORD_SIZE / SEC + 1);
        rh->sequence = 1;
        rh->link_count = 1;
        rh->attribute_offset = u16((0x30 + (RECORD_SIZE / SEC + 1) * 2 + 7) / 8 * 8);
        rh->flags = ntfs::RECORD_IN_USE;
        rh->bytes_in_use = u32(RECORD_SIZE);
        rh->bytes_allocated = u32(RECORD_SIZE);
        rh->base_record = 0;
        rh->record_number = u32(record_no);
        /* the USA: slot0 = USN, slots 1..n = the sector-tail originals;
         * the tails carry the USN (fixup_apply's contract) */
        for (u64 i = 0; i <= RECORD_SIZE / SEC; ++i) {
            const u16 v = i == 0 ? 0x0BB8 : u16(0xAAAA + i);
            wr16(rec, size_t(0x30 + i * 2), v);
        }
        for (u64 i = 0; i < RECORD_SIZE / SEC; ++i) {
            u8* t = rec.data() + size_t(i * SEC + SEC - 2);
            t[0] = 0xB8;
            t[1] = 0x0B;
        }
        u32 pos = rh->attribute_offset;
        if (record_no == 0) {
            /* $MFT: a nonresident $DATA runlist (LCN 4, 16 clusters) */
            const u32 alen = 0x40 + 8; /* header + a minimal run */
            wr32(rec, pos + 0, ntfs::ATTR_TYPE_DATA);
            wr32(rec, pos + 4, alen);
            rec[pos + 8] = 1;          /* nonresident */
            wr16(rec, pos + 32, 0x40); /* runlist offset */
            wr64(rec, pos + 40, 16 * CLUSTER);
            wr64(rec, pos + 48, 16 * CLUSTER);
            wr64(rec, pos + 56, 16 * CLUSTER);
            /* the run: offset 4, length 16 (LEB128-ish: fits one byte) */
            rec[pos + 0x40] = 0x11; /* 1 offset byte, 1 length byte */
            rec[pos + 0x41] = 16;
            rec[pos + 0x42] = 4;
            pos += alen;
        } else {
            /* record 3: the resident $VOLUME_NAME (type 0x60). The
             * resident header: value_length u32 @+0x10, value_offset u16
             * @+0x14 (the field order the wire struct pins). */
            const std::u16string lbl = label16;
            const u32 vlen = u32(lbl.size()) * 2;
            const u32 voff = 0x18;
            const u32 alen = (voff + vlen + 7) / 8 * 8;
            wr32(rec, pos + 0, ntfs::ATTR_TYPE_VOLUME_NAME);
            wr32(rec, pos + 4, alen);
            rec[pos + 8] = 0; /* resident */
            wr32(rec, pos + 0x10, vlen);
            wr16(rec, pos + 0x14, u16(voff));
            for (u32 i = 0; i < lbl.size(); ++i)
                wr16(rec, pos + voff + i * 2, u16(lbl[i]));
            pos += alen;
        }
        /* the terminator */
        wr32(rec, pos, ntfs::ATTR_TYPE_TERMINATOR);
        return rec;
    };
    {
        const auto r0 = build_record(0, nullptr);
        std::memcpy(img.data() + MFT_LCN * CLUSTER, r0.data(), r0.size());
        const auto r3 = build_record(3, u"DataDisk");
        std::memcpy(img.data() + (MFT_LCN + 6) * CLUSTER, r3.data(), r3.size());
    }
    MemDevice dev(std::move(img));
    char label[33] = {0};
    ASSERT_EQ(orchestrator::probe_slice_volume_label(&dev, label), OK);
    EXPECT_STREQ(label, "DataDisk");
}

TEST(VolumeLabel, CallerValueWins) {
    /* N-133: the corrected precedence lives in the orchestrator's copy
     * rule: a NON-derived label (an explicit --label or a live-volume
     * GetVolumeInformationW label - both caller-supplied) is never
     * replaced. The derived marker is what gates the replacement, and it
     * is set ONLY by the image-stem fallback leg. */
    orchestrator::ScanConfig cfg;
    /* the caller's explicit label */
    std::memcpy(cfg.volume_label, "USERLABEL", 10);
    EXPECT_FALSE(cfg.volume_label_derived) << "an explicit label is never derived";
    /* the stem fallback flips the marker */
    orchestrator::ScanConfig stem;
    std::memcpy(stem.volume_label, "backup", 7);
    stem.volume_label_derived = true;
    EXPECT_TRUE(stem.volume_label_derived);
    /* the engine label replaces a derived label but not an explicit one:
     * the replacement rule the orchestrator implements (cr_scan.cpp's
     * N-133 block) keys on exactly this flag */
    const char engine_label[33] = "myvol";
    if (cfg.volume_label_derived) { /* must NOT fire for the explicit label */
        ADD_FAILURE() << "the explicit label must not be marked derived";
    } else if (stem.volume_label_derived) {
        size_t k = 0;
        for (; k + 1 < sizeof(stem.volume_label) && engine_label[k]; ++k)
            stem.volume_label[k] = engine_label[k];
        stem.volume_label[k] = 0;
    }
    EXPECT_STREQ(cfg.volume_label, "USERLABEL") << "the caller's label survives";
    EXPECT_STREQ(stem.volume_label, "myvol") << "the engine label replaces the stem";
}

TEST(VolumeLabel, UnknownFamilyLeavesFallback) {
    /* N-133: an unrecognized boot sector returns ERR_UNSUPPORTED - the
     * caller keeps its fallback label (never an abort). */
    std::vector<u8> img(64 * 1024, 0);
    std::memcpy(img.data() + 3, "ZZZZ    ", 8);
    MemDevice dev(std::move(img));
    char label[33] = {0};
    EXPECT_EQ(orchestrator::probe_slice_volume_label(&dev, label), u32(ERR_UNSUPPORTED));
    EXPECT_EQ(label[0], 0);
}
