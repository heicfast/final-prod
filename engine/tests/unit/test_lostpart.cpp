/* Lost-partition discovery tests (R3). */
#include <gtest/gtest.h>
#include "cr/lost/cr_lostpart.h"
#include "cr/storage/cr_device.h"
#include "cr/table/cr_table_parse.h"
#include <cstring>
#include <vector>

using namespace cr;
using namespace cr::lost;

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

/* a plausible NTFS boot sector at `lba` + a live $MFT record at mft_lcn */
void plant_ntfs(std::vector<u8>& img, u64 lba, u64 total_sectors, u64 mft_lcn) {
    u8* b = img.data() + lba * 512;
    std::memcpy(b + 3, "NTFS    ", 8);
    put16(img, lba * 512 + 0x0B, 512);
    b[0x0D] = 1; /* spc */
    b[0x15] = 0xF8;
    put64(img, lba * 512 + 0x28, total_sectors);
    put64(img, lba * 512 + 0x30, mft_lcn);
    put64(img, lba * 512 + 0x38, mft_lcn + 8);
    b[0x40] = 2; /* clusters/record: 1024B */
    b[0x44] = 2;
    b[510] = 0x55;
    b[511] = 0xAA;
    /* fake $MFT record at lba + mft_lcn (volume-relative): 'FILE' + USA */
    const u64 rec_off = (lba + mft_lcn) * 512;
    b = img.data() + rec_off;
    put32v(img, rec_off + 0, 0x454C4946); /* 'FILE' */
    put16(img, rec_off + 4, 0x30);        /* USA offset */
    put16(img, rec_off + 6, 3);           /* USA size: 2+1 */
    put16(img, rec_off + 8, 1);           /* sequence */
    put16(img, rec_off + 0x14, 0x38);     /* attribute offset */
    put16(img, rec_off + 0x18, 1);        /* in-use */
    put32v(img, rec_off + 0x1C, 1024);
    put32v(img, rec_off + 0x20, 1024);
    put32v(img, rec_off + 0x2C, u32(0));
    /* USA slots: USN, orig0, orig1 */
    put16(img, rec_off + 0x30, 0x1234);
    put16(img, rec_off + 0x32, 0xAAAA);
    put16(img, rec_off + 0x34, 0xBBBB);
    /* tails carry the USN */
    img[rec_off + 510] = 0x34;
    img[rec_off + 511] = 0x12;
    img[rec_off + 1022] = 0x34;
    img[rec_off + 1023] = 0x12;
}

void plant_exfat_boot(std::vector<u8>& img, u64 lba, u32 volume_length, u32 cluster_count) {
    u8* b = img.data() + lba * 512;
    b[0] = 0xEB;
    b[1] = 0x76;
    b[2] = 0x90;
    std::memcpy(b + 3, "EXFAT   ", 8);
    put64(img, lba * 512 + 72, volume_length);
    put32v(img, lba * 512 + 80, 24);
    put32v(img, lba * 512 + 84, 1);
    put32v(img, lba * 512 + 88, 25);
    put32v(img, lba * 512 + 92, cluster_count);
    put32v(img, lba * 512 + 96, 2);
    b[108] = 9;
    b[510] = 0x55;
    b[511] = 0xAA;
}

std::vector<u8> build_disk() {
    std::vector<u8> img(u64(8192) * 512, 0);
    /* live MBR: one NTFS-typed partition at 2048 len 1024 (no boot sector
     * content - it is the KNOWN table, not a discovery target) */
    u8* mbr = img.data();
    mbr[446 + 4] = 0x07;
    put32v(img, 446 + 8, 2048);
    put32v(img, 446 + 12, 1024);
    mbr[510] = 0x55;
    mbr[511] = 0xAA;

    plant_ntfs(img, 1024, 512, 100);        /* mft_lcn inside the volume */
    plant_exfat_boot(img, 4096, 2048, 100); /* spec-min size, own region */
    /* noise: 55AA with garbage BPB at sector 500 */
    img[500 * 512 + 510] = 0x55;
    img[500 * 512 + 511] = 0xAA;
    img[500 * 512 + 0x0B] = 0x11; /* nonsense bps */
    return img;
}

} // namespace

TEST(LostDiscovery, FindsScoresAndRejectsNoise) {
    auto img = build_disk();
    MemDevice dev(std::move(img));

    /* live table for overlap classification */
    table::DiskTable t;
    ASSERT_EQ(table::parse_disk_table(&dev, t), OK);
    ASSERT_EQ(t.partitions.size(), 1u);
    EXPECT_EQ(t.partitions[0].start_lba, 2048u);

    DiscoveryResult r;
    u32 rc = discover(&dev, t.partitions, 1, nullptr, nullptr, nullptr, r);
    ASSERT_EQ(rc, OK) << err_text(rc);
    EXPECT_GE(r.sectors_scanned, 4096u);

    /* locate candidates */
    const Candidate *ntfs_c = nullptr, *exfat_c = nullptr, *noise = nullptr;
    for (const auto& c : r.candidates) {
        if (c.start_lba == 1024)
            ntfs_c = &c;
        else if (c.start_lba == 4096)
            exfat_c = &c;
        else if (c.start_lba == 500)
            noise = &c;
    }
    ASSERT_NE(ntfs_c, nullptr);
    ASSERT_NE(exfat_c, nullptr);
    EXPECT_EQ(noise, nullptr); /* incoherent BPB rejected */

    EXPECT_EQ(ntfs_c->fs, CandidateFs::Ntfs);
    EXPECT_GE(ntfs_c->score, 70); /* 15 boot + 25 bpb + 30 mft probe */
    EXPECT_EQ(ntfs_c->sector_count, 512u);
    EXPECT_EQ(ntfs_c->start_lba + ntfs_c->sector_count, 1536u); /* no tail overlap
        with the live partition at 2048 -> no Overlaps penalty in this case */

    EXPECT_EQ(exfat_c->fs, CandidateFs::Exfat);
    EXPECT_GE(exfat_c->score, 35); /* 15 + 20 */
    EXPECT_EQ(exfat_c->sector_count, 2048u);

    /* sorted desc */
    ASSERT_GE(r.candidates.size(), 2u);
    EXPECT_GE(r.candidates[0].score, r.candidates[1].score);

    /* materialize + read through the virtual slice */
    auto slice = materialize(&dev, *ntfs_c);
    ASSERT_NE(slice, nullptr);
    u8 buf[9];
    EXPECT_EQ(slice->read_at(3, buf, 8), OK);
    buf[8] = 0;
    EXPECT_EQ(std::memcmp(buf, "NTFS    ", 8), 0);
}

TEST(LostDiscovery, OverlapClassification) {
    table::PartitionInfo live;
    live.start_lba = 1000;
    live.sector_count = 500;
    live.used = true;
    std::vector<table::PartitionInfo> known{live};
    EXPECT_EQ(classify_overlap(0, 100, known), Overlap::None);
    EXPECT_EQ(classify_overlap(1100, 50, known), Overlap::Inside);
    EXPECT_EQ(classify_overlap(900, 200, known), Overlap::Overlaps);
    EXPECT_EQ(classify_overlap(900, 800, known), Overlap::Contains);
}

/* ---- GAP-S6-6: TWO deleted NTFS partitions must BOTH surface ----
 *
 * Bench run 40073093997 delete_partition: the deep tree carried only
 * 'Candidate @ 32768' (partition A) while the og found BOTH deleted
 * partitions. Registered suspect: the nested-same-fs dedup when the
 * outer candidate's size came from the DEVICE-TAIL FALLBACK (a probe
 * failure path) - a fallback-sized extent spans to disk end and
 * swallows every later peer. */

TEST(LostDiscovery, TwoHealthyDeletedPartitionsBothSurface) {
    /* the bench shape: two ~equal NTFS partitions at unequal offsets,
     * both boot sectors intact (GPT entries removed - data untouched). */
    std::vector<u8> img(u64(16384) * 512, 0); /* 8 MiB disk */
    plant_ntfs(img, 32768 / 8, 4096, 64);     /* A: LBA 4096, 2 MiB */
    plant_ntfs(img, 12288, 4096, 64);         /* B: LBA 12288, 2 MiB */
    MemDevice dev(std::move(img));
    DiscoveryResult r;
    ASSERT_EQ(discover(&dev, {}, 1, nullptr, nullptr, nullptr, r), OK);
    size_t ntfs = 0;
    for (const auto& c : r.candidates)
        if (c.fs == CandidateFs::Ntfs)
            ++ntfs;
    EXPECT_EQ(ntfs, 2u) << "both healthy deleted NTFS partitions must surface";
}

TEST(LostDiscovery, DamagedBpbFallbackMustNotSwallowPeer) {
    /* A's BPB is damaged (fallback size = device tail - spans to the end);
     * B (healthy) starts inside that envelope. The nested-same-fs dedup
     * must NOT treat a fallback-sized extent as containment evidence. */
    std::vector<u8> img(u64(16384) * 512, 0);
    plant_ntfs(img, 4096, 4096, 64);  /* A: healthy shape first */
    plant_ntfs(img, 12288, 4096, 64); /* B: healthy, later offset */
    /* damage A's BPB AFTER planting (total-sectors field destroyed -> the
     * probe falls back to the device-end size) */
    put64(img, 4096 * 512 + 0x28, 0);
    MemDevice dev(std::move(img));
    DiscoveryResult r;
    ASSERT_EQ(discover(&dev, {}, 1, nullptr, nullptr, nullptr, r), OK);
    size_t ntfs = 0;
    u64 b_found = 0;
    for (const auto& c : r.candidates)
        if (c.fs == CandidateFs::Ntfs) {
            ++ntfs;
            if (c.start_lba == 12288)
                b_found = 1;
        }
    EXPECT_EQ(ntfs, 2u) << "the fallback-sized A must not dedup away B";
    EXPECT_EQ(b_found, 1u) << "B (healthy probe, inside A's fallback envelope) must surface";
}
