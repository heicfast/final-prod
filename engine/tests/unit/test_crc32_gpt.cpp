/* CRC32 + GPT/MBR parser tests. */
#include <gtest/gtest.h>
#include "cr/table/cr_table_parse.h"
#include "cr/storage/cr_slice.h"
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

using namespace cr;
using namespace cr::table;

namespace {
/* In-memory device for parser tests. */
class MemDevice final : public IStorageDevice {
public:
    explicit MemDevice(std::vector<u8> bytes, u32 sector = 512) : data_(std::move(bytes)) {
        geom_.logical_sector = sector;
        geom_.physical_sector = sector;
        geom_.sector_count = data_.size() / sector;
        geom_.total_bytes = geom_.sector_count * sector;
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

void put32(std::vector<u8>& v, size_t at, u32 x) {
    v[at] = u8(x);
    v[at + 1] = u8(x >> 8);
    v[at + 2] = u8(x >> 16);
    v[at + 3] = u8(x >> 24);
}
void put64(std::vector<u8>& v, size_t at, u64 x) {
    for (int i = 0; i < 8; ++i)
        v[at + i] = u8(x >> (8 * i));
}
} // namespace

TEST(Crc32, KnownVectors) {
    /* IEEE crc32: "" -> 0, "123456789" -> 0xCBF43926 */
    EXPECT_EQ(crc32("", 0), 0u);
    EXPECT_EQ(crc32("123456789", 9), 0xCBF43926u);
    EXPECT_EQ(crc32("The quick brown fox jumps over the lazy dog", 43), 0x414FA339u);
}

TEST(Gpt, ValidTable) {
    /* disk: 2048 sectors * 512 */
    std::vector<u8> disk(u64(2048) * 512, 0);
    /* protective MBR */
    disk[510] = 0x55;
    disk[511] = 0xAA;
    disk[446 + 4] = 0xEE;
    put32(disk, 446 + 8, 1);
    put32(disk, 446 + 12, 2047);

    /* GPT header at LBA1 (offsets: current 24, backup 32, first_usable 40,
     * last_usable 48, entries_start 72, count 80, size 84, ecrc 88, hcrc 16) */
    std::memcpy(disk.data() + 512, "EFI PART", 8);
    put32(disk, 512 + 8, 0x00010000); /* revision */
    put32(disk, 512 + 12, 92);        /* header size */
    put64(disk, 512 + 24, 1);         /* current LBA */
    put64(disk, 512 + 32, 2047);      /* backup header at last sector */
    put64(disk, 512 + 40, 2);         /* first usable */
    put64(disk, 512 + 48, 2046);      /* last usable */
    put64(disk, 512 + 72, 2);         /* entries at LBA2 */
    put32(disk, 512 + 80, 4);         /* 4 entries */
    put32(disk, 512 + 84, 128);       /* entry size */

    /* two entries: one used (type=basic data), one empty */
    std::vector<u8> entries(4 * 128, 0);
    const u8 basic[16] = {0xA2, 0xA0, 0xD0, 0xEB, 0xE5, 0xB9, 0x33, 0x44,
                          0x87, 0xC0, 0x68, 0xB6, 0xB7, 0x26, 0x99, 0xC7};
    std::memcpy(entries.data() + 0, basic, 16); /* type @0 */
    put64(entries, 32, 100);                    /* first_lba @32 */
    put64(entries, 40, 200);                    /* last_lba @40 */
    put64(entries, 48, 0);                      /* attributes @48 */
    const char16_t nm[] = u"Data";
    std::memcpy(entries.data() + 56, nm, sizeof(char16_t) * 4); /* name @56 */

    /* entries CRC */
    u32 ecrc = crc32(entries.data(), entries.size());
    put32(disk, 512 + 88, ecrc);
    /* entries live at LBA2..LBA33 (4*128 = 512 bytes = 1 sector) */
    std::memcpy(disk.data() + 2 * 512, entries.data(), entries.size());

    /* header CRC (field zeroed while computing) */
    u32 hcrc = crc32(disk.data() + 512, 92);
    put32(disk, 512 + 16, hcrc);

    /* backup GPT: entries at LBA 2046-... header at 2047 */
    std::vector<u8> bentries = entries;
    std::memcpy(disk.data() + 2046 * 512, bentries.data(), bentries.size());
    std::vector<u8> bh(512);
    std::memcpy(bh.data(), disk.data() + 512, 512);
    put32(bh, 16, 0);    /* zero CRC before computing */
    put64(bh, 24, 2047); /* current lba = 2047 */
    put64(bh, 32, 1);    /* backup lba -> primary position */
    put64(bh, 72, 2046); /* entries at 2046 */
    u32 bcrc = crc32(bh.data(), 92);
    put32(bh, 16, bcrc);
    std::memcpy(disk.data() + 2047 * 512, bh.data(), 512);

    MemDevice dev(std::move(disk));
    DiskTable t;
    ASSERT_EQ(parse_disk_table(&dev, t), OK);
    EXPECT_EQ(t.layout, TableLayout::Gpt);
    EXPECT_TRUE(t.gpt_primary_ok);
    EXPECT_TRUE(t.gpt_backup_ok);
    ASSERT_EQ(t.partitions.size(), 1u);
    EXPECT_EQ(t.partitions[0].start_lba, 100u);
    EXPECT_EQ(t.partitions[0].sector_count, 101u);
}

TEST(Gpt, BadHeaderCrcDetected) {
    std::vector<u8> disk(u64(2048) * 512, 0);
    disk[510] = 0x55;
    disk[511] = 0xAA;
    disk[446 + 4] = 0xEE;
    std::memcpy(disk.data() + 512, "EFI PART", 8);
    put32(disk, 512 + 8, 0x00010000);
    put32(disk, 512 + 12, 92);
    put64(disk, 512 + 72, 2);
    put32(disk, 512 + 80, 4);
    put32(disk, 512 + 84, 128);
    put32(disk, 512 + 88, 0xDEADBEEF); /* wrong entries crc */
    u32 hcrc = crc32(disk.data() + 512, 92);
    put32(disk, 512 + 16, hcrc);
    std::vector<u8> entries(512, 0);
    std::memcpy(disk.data() + 2 * 512, entries.data(), 512);

    MemDevice dev(std::move(disk));
    DiskTable t;
    ASSERT_EQ(parse_disk_table(&dev, t), OK);
    EXPECT_EQ(t.layout, TableLayout::GptDamaged);
    EXPECT_FALSE(t.gpt_primary_ok);
    EXPECT_FALSE(t.gpt_backup_ok);
}

TEST(Mbr, PlainWithExtended) {
    /* 4096-sector disk: MBR with 1 primary + EBR chain of 2 */
    std::vector<u8> disk(u64(4096) * 512, 0);
    disk[510] = 0x55;
    disk[511] = 0xAA;
    /* primary: NTFS type 7 at LBA 128 len 512 */
    disk[446 + 4] = 0x07;
    put32(disk, 446 + 8, 128);
    put32(disk, 446 + 12, 512);
    /* extended container at LBA 2048 len 2040 */
    disk[446 + 16 + 4] = 0x05;
    put32(disk, 446 + 16 + 8, 2048);
    put32(disk, 446 + 16 + 12, 2040);

    /* EBR1 at 2048: entry0 = volume at +63 len 500 ; entry1 = next EBR at +1024 */
    u8* ebr1 = disk.data() + 2048 * 512;
    ebr1[510] = 0x55;
    ebr1[511] = 0xAA;
    ebr1[446 + 4] = 0x07;
    put32(disk, 2048 * 512 + 446 + 8, 63);
    put32(disk, 2048 * 512 + 446 + 12, 500);
    ebr1[446 + 16 + 4] = 0x05;
    put32(disk, 2048 * 512 + 446 + 16 + 8, 1024);
    put32(disk, 2048 * 512 + 446 + 16 + 12, 900);

    /* EBR2 at 2048+1024=3072: entry0 = volume at +63 len 700; no next */
    u8* ebr2 = disk.data() + 3072 * 512;
    ebr2[510] = 0x55;
    ebr2[511] = 0xAA;
    ebr2[446 + 4] = 0x0B; /* FAT32 */
    put32(disk, 3072 * 512 + 446 + 8, 63);
    put32(disk, 3072 * 512 + 446 + 12, 700);

    MemDevice dev(std::move(disk));
    DiskTable t;
    ASSERT_EQ(parse_disk_table(&dev, t), OK);
    /* N-057: the extended chain makes the EBR layout the table's
     * classification (a first-class og model; ebr_status carries the
     * chain's integrity separately) - the pre-fix enum value was dead */
    EXPECT_EQ(t.layout, TableLayout::Ebr);
    ASSERT_EQ(t.partitions.size(), 3u);
    EXPECT_EQ(t.partitions[0].start_lba, 128u);
    EXPECT_EQ(t.partitions[1].start_lba, 2048u + 63u);
    EXPECT_EQ(t.partitions[1].fs, FsKind::Ntfs);
    EXPECT_EQ(t.partitions[2].start_lba, 3072u + 63u);
    EXPECT_EQ(t.partitions[2].fs, FsKind::Fat32);
}

TEST(Mbr, EbrLayoutAssignedAndPlainMbrStaysMbr) {
    /* N-057: TableLayout::Ebr was a dead enum value (grep-verified at
     * audit time: zero assignment sites). The extended chain now sets
     * it; a 4-primary MBR with NO extended partition keeps Mbr. */
    {
        /* plain 4-primary MBR: no extended container -> stays Mbr */
        std::vector<u8> disk(u64(512) * 512, 0);
        disk[510] = 0x55;
        disk[511] = 0xAA;
        for (int i = 0; i < 4; ++i) {
            disk[446 + i * 16 + 4] = 0x07;
            put32(disk, 446 + i * 16 + 8, 32 + i * 64);
            put32(disk, 446 + i * 16 + 12, 32);
        }
        MemDevice dev(std::move(disk));
        DiskTable t;
        ASSERT_EQ(parse_disk_table(&dev, t), OK);
        EXPECT_EQ(t.layout, TableLayout::Mbr) << "no extended chain: stays Mbr";
        ASSERT_EQ(t.partitions.size(), 4u);
    }
    {
        /* single extended container with one EBR -> Ebr */
        std::vector<u8> disk(u64(1024) * 512, 0);
        disk[510] = 0x55;
        disk[511] = 0xAA;
        disk[446 + 4] = 0x05; /* extended container at LBA 64 */
        put32(disk, 446 + 8, 64);
        put32(disk, 446 + 12, 900);
        u8* ebr = disk.data() + 64 * 512;
        ebr[510] = 0x55;
        ebr[511] = 0xAA;
        ebr[446 + 4] = 0x07;
        put32(disk, 64 * 512 + 446 + 8, 63);
        put32(disk, 64 * 512 + 446 + 12, 500);
        MemDevice dev(std::move(disk));
        DiskTable t;
        ASSERT_EQ(parse_disk_table(&dev, t), OK);
        EXPECT_EQ(t.layout, TableLayout::Ebr) << "extended chain must classify as Ebr";
        ASSERT_EQ(t.partitions.size(), 1u);
        EXPECT_EQ(t.partitions[0].start_lba, 64u + 63u);
    }
}

TEST(Mbr, EbrCycleGuard) {
    /* EBR1 -> EBR2 -> EBR1 : infinite loop must be cut (fix F7) */
    std::vector<u8> disk(u64(4096) * 512, 0);
    disk[510] = 0x55;
    disk[511] = 0xAA;
    disk[446 + 16 + 4] = 0x05;
    put32(disk, 446 + 16 + 8, 2048);
    put32(disk, 446 + 16 + 12, 2040);
    u8* ebr1 = disk.data() + 2048 * 512;
    ebr1[510] = 0x55;
    ebr1[511] = 0xAA;
    ebr1[446 + 4] = 0x07;
    put32(disk, 2048 * 512 + 446 + 8, 63);
    put32(disk, 2048 * 512 + 446 + 12, 500);
    ebr1[446 + 16 + 4] = 0x05;
    put32(disk, 2048 * 512 + 446 + 16 + 8, 1024); /* -> EBR at 3072 */
    u8* ebr2 = disk.data() + 3072 * 512;
    ebr2[510] = 0x55;
    ebr2[511] = 0xAA;
    ebr2[446 + 4] = 0x07;
    put32(disk, 3072 * 512 + 446 + 8, 63);
    put32(disk, 3072 * 512 + 446 + 12, 500);
    ebr2[446 + 16 + 4] = 0x05;
    put32(disk, 3072 * 512 + 446 + 16 + 8, 1024 - 1024); /* -> back to EBR1 base+0 */

    MemDevice dev(std::move(disk));
    DiskTable t;
    u32 rc = parse_disk_table(&dev, t);
    /* chain detected the loop: OK with salvaged entries or CYCLE - both fine;
     * the call must TERMINATE (the test would hang otherwise) */
    EXPECT_TRUE(rc == OK || rc == ERR_CYCLE_LIMIT);
}

/* ===================================================================== */
/* M-124 (diff-to-fix): the og's EBR chain walker treats link type 0x85  */
/* (Linux extended, the fdisk/dual-boot convention) IDENTICALLY to      */
/* 0x05/0x0F - Partition_decompiled.c:989-1010: the -0x7b (== 0x85)     */
/* comparison jumps to LAB_180002348, the same link path. The core's    */
/* mbr_type_to_kind mapped only 0x05/0x0F, so a 0x85-stamped chain      */
/* emitted a bogus Other-volume at the link target and lost every later */
/* logical volume.                                                      */
/* ===================================================================== */

TEST(Ebr, LinkType085WalksChain) {
    /* MBR extended container stamped 0x85 + a 3-EBR chain whose links are
     * all 0x85: all three logical volumes must surface with correct
     * EBR-relative bases. Fail-before: only the first volume surfaced plus
     * one bogus Other volume; volumes 2 and 3 vanished. */
    std::vector<u8> disk(u64(8192) * 512, 0);
    disk[510] = 0x55;
    disk[511] = 0xAA;
    /* primary NTFS + Linux-extended container at LBA 1024 (type 0x85) */
    disk[446 + 4] = 0x07;
    put32(disk, 446 + 8, 128);
    put32(disk, 446 + 12, 512);
    disk[446 + 16 + 4] = 0x85;
    put32(disk, 446 + 16 + 8, 1024);
    put32(disk, 446 + 16 + 12, 6000);

    /* EBR1 at 1024: vol at +63 len 500; next EBR at base+1024=2048 (0x85) */
    put32(disk, 1024 * 512 + 446 + 0 * 16 + 4, 0x07);
    put32(disk, 1024 * 512 + 446 + 0 * 16 + 8, 63);
    put32(disk, 1024 * 512 + 446 + 0 * 16 + 12, 500);
    put32(disk, 1024 * 512 + 446 + 1 * 16 + 4, 0x85);
    put32(disk, 1024 * 512 + 446 + 1 * 16 + 8, 1024);
    disk[1024 * 512 + 510] = 0x55;
    disk[1024 * 512 + 511] = 0xAA;

    /* EBR2 at 2048: vol at +63 len 700; next EBR at base+2048=3072 (0x85) */
    put32(disk, 2048 * 512 + 446 + 0 * 16 + 4, 0x0B);
    put32(disk, 2048 * 512 + 446 + 0 * 16 + 8, 63);
    put32(disk, 2048 * 512 + 446 + 0 * 16 + 12, 700);
    put32(disk, 2048 * 512 + 446 + 1 * 16 + 4, 0x85);
    put32(disk, 2048 * 512 + 446 + 1 * 16 + 8, 2048);
    disk[2048 * 512 + 510] = 0x55;
    disk[2048 * 512 + 511] = 0xAA;

    /* EBR3 at 3072: vol at +63 len 800; chain end */
    put32(disk, 3072 * 512 + 446 + 0 * 16 + 4, 0x83);
    put32(disk, 3072 * 512 + 446 + 0 * 16 + 8, 63);
    put32(disk, 3072 * 512 + 446 + 0 * 16 + 12, 800);
    disk[3072 * 512 + 510] = 0x55;
    disk[3072 * 512 + 511] = 0xAA;

    MemDevice dev(std::move(disk));
    DiskTable t;
    ASSERT_EQ(parse_disk_table(&dev, t), OK);
    EXPECT_EQ(t.layout, TableLayout::Ebr) << "0x85 container is an extended chain";
    ASSERT_EQ(t.partitions.size(), 4u) << "primary + all three logical volumes";
    EXPECT_EQ(t.partitions[0].start_lba, 128u);
    EXPECT_EQ(t.partitions[1].start_lba, 1024u + 63u);
    EXPECT_EQ(t.partitions[1].fs, FsKind::Ntfs);
    EXPECT_EQ(t.partitions[2].start_lba, 2048u + 63u);
    EXPECT_EQ(t.partitions[2].fs, FsKind::Fat32);
    EXPECT_EQ(t.partitions[3].start_lba, 3072u + 63u);
    EXPECT_EQ(t.partitions[3].fs, FsKind::Linux);
}

TEST(Ebr, LinkType085SameBasisAs05) {
    /* The same physical chain stamped 0x05 vs 0x85 must yield IDENTICAL
     * volume bases - the og feeds both through the same link path, so the
     * basis arithmetic (links base-relative, volumes EBR-relative) cannot
     * differ between the two stamps. */
    auto build = [](u8 link_type) {
        std::vector<u8> disk(u64(4096) * 512, 0);
        disk[510] = 0x55;
        disk[511] = 0xAA;
        disk[446 + 16 + 4] = link_type;
        put32(disk, 446 + 16 + 8, 2048);
        put32(disk, 446 + 16 + 12, 2040);
        put32(disk, 2048 * 512 + 446 + 0 * 16 + 4, 0x07);
        put32(disk, 2048 * 512 + 446 + 0 * 16 + 8, 63);
        put32(disk, 2048 * 512 + 446 + 0 * 16 + 12, 500);
        put32(disk, 2048 * 512 + 446 + 1 * 16 + 4, link_type);
        put32(disk, 2048 * 512 + 446 + 1 * 16 + 8, 1024);
        disk[2048 * 512 + 510] = 0x55;
        disk[2048 * 512 + 511] = 0xAA;
        put32(disk, 3072 * 512 + 446 + 0 * 16 + 4, 0x0B);
        put32(disk, 3072 * 512 + 446 + 0 * 16 + 8, 63);
        put32(disk, 3072 * 512 + 446 + 0 * 16 + 12, 700);
        disk[3072 * 512 + 510] = 0x55;
        disk[3072 * 512 + 511] = 0xAA;
        return disk;
    };
    DiskTable t5, t85;
    {
        MemDevice dev(build(0x05));
        ASSERT_EQ(parse_disk_table(&dev, t5), OK);
    }
    {
        MemDevice dev(build(0x85));
        ASSERT_EQ(parse_disk_table(&dev, t85), OK);
    }
    ASSERT_EQ(t5.partitions.size(), 2u);
    ASSERT_EQ(t85.partitions.size(), 2u) << "0x85 chain must surface the same volumes";
    for (size_t i = 0; i < 2; ++i) {
        EXPECT_EQ(t85.partitions[i].start_lba, t5.partitions[i].start_lba)
            << "volume " << i << " basis differs between 0x85 and 0x05 stamps";
        EXPECT_EQ(t85.partitions[i].sector_count, t5.partitions[i].sector_count);
        EXPECT_EQ(t85.partitions[i].fs, t5.partitions[i].fs);
    }
}

TEST(Mbr, Extended085NotEmittedAsVolume) {
    /* A 0x85 container in the MBR must NOT surface as one opaque Other-
     * volume spanning the container - it is a container, and its content
     * comes from the walked logical volumes. Fail-before: the unmapped
     * 0x85 fell to FsKind::Other and was emitted as a bogus volume at the
     * container LBA. */
    std::vector<u8> disk(u64(4096) * 512, 0);
    disk[510] = 0x55;
    disk[511] = 0xAA;
    disk[446 + 16 + 4] = 0x85;
    put32(disk, 446 + 16 + 8, 2048);
    put32(disk, 446 + 16 + 12, 2040);
    /* one EBR with one volume, no link */
    put32(disk, 2048 * 512 + 446 + 0 * 16 + 4, 0x07);
    put32(disk, 2048 * 512 + 446 + 0 * 16 + 8, 63);
    put32(disk, 2048 * 512 + 446 + 0 * 16 + 12, 500);
    disk[2048 * 512 + 510] = 0x55;
    disk[2048 * 512 + 511] = 0xAA;

    MemDevice dev(std::move(disk));
    DiskTable t;
    ASSERT_EQ(parse_disk_table(&dev, t), OK);
    ASSERT_EQ(t.partitions.size(), 1u)
        << "exactly the logical volume - no bogus container-volume row";
    EXPECT_EQ(t.partitions[0].start_lba, 2048u + 63u);
    EXPECT_NE(t.partitions[0].start_lba, 2048u)
        << "the container LBA itself must not surface as a volume";
}

/* ===================================================================== */
/* M-002 (diff-to-fix): a torn EBR link must NOT discard the volumes the */
/* chain already walked (salvage-over-purity; the og's deep-scan block    */
/* parsers re-find volumes independently of the EBR chain — corpus       */
/* mo-module-map). The old gate kept ext[] only on OK/CYCLE_LIMIT.       */
/* ===================================================================== */

TEST(Mbr, TornLinkKeepsPriorVolumes) {
    /* 3-link chain; the 3rd EBR sector is torn (no 0x55AA): volumes 1-2
     * must survive, status Partial, break LBA reported. Fail-before: the
     * caller's rc gate dropped BOTH walked volumes -> 0 logical volumes. */
    std::vector<u8> disk(u64(6144) * 512, 0);
    disk[510] = 0x55;
    disk[511] = 0xAA;
    /* extended container at LBA 1024 */
    disk[446 + 16 + 4] = 0x05;
    put32(disk, 446 + 16 + 8, 1024);
    put32(disk, 446 + 16 + 12, 4000);

    /* EBR1 at 1024: volume at +63 len 500; next EBR at +1024 */
    put32(disk, 1024 * 512 + 446 + 0 * 16 + 4, 0x07);
    put32(disk, 1024 * 512 + 446 + 0 * 16 + 8, 63);
    put32(disk, 1024 * 512 + 446 + 0 * 16 + 12, 500);
    put32(disk, 1024 * 512 + 446 + 1 * 16 + 4, 0x05);
    put32(disk, 1024 * 512 + 446 + 1 * 16 + 8, 1024);
    disk[1024 * 512 + 510] = 0x55;
    disk[1024 * 512 + 511] = 0xAA;

    /* EBR2 at 2048: volume at +63 len 700; next EBR at base+2048=3072 */
    put32(disk, 2048 * 512 + 446 + 0 * 16 + 4, 0x0B);
    put32(disk, 2048 * 512 + 446 + 0 * 16 + 8, 63);
    put32(disk, 2048 * 512 + 446 + 0 * 16 + 12, 700);
    put32(disk, 2048 * 512 + 446 + 1 * 16 + 4, 0x05);
    put32(disk, 2048 * 512 + 446 + 1 * 16 + 8, 2048); /* base-relative */
    disk[2048 * 512 + 510] = 0x55;
    disk[2048 * 512 + 511] = 0xAA;

    /* EBR3 at 3072: TORN SECTOR (no 0x55AA signature) */
    put32(disk, 3072 * 512 + 446 + 0 * 16 + 4, 0x07);
    put32(disk, 3072 * 512 + 446 + 0 * 16 + 8, 63);
    put32(disk, 3072 * 512 + 446 + 0 * 16 + 12, 500);

    MemDevice dev(std::move(disk));
    DiskTable t;
    ASSERT_EQ(parse_disk_table(&dev, t), OK);
    /* the two WALKED logical volumes survive the torn 3rd link */
    ASSERT_EQ(t.partitions.size(), 2u);
    EXPECT_EQ(t.partitions[0].start_lba, 1024u + 63u);
    EXPECT_EQ(t.partitions[0].fs, FsKind::Ntfs);
    EXPECT_EQ(t.partitions[1].start_lba, 2048u + 63u);
    EXPECT_EQ(t.partitions[1].fs, FsKind::Fat32);
    /* chain shape honesty */
    EXPECT_EQ(t.ebr_status, EbrChainStatus::Partial);
    EXPECT_EQ(t.ebr_break_lba, 3072u);
}

TEST(Mbr, TornLinkStatusReported) {
    /* The status vocabulary: whole chain -> Ok/break 0; first link dead
     * and nothing walked -> Failed with the break LBA; no extended
     * container -> None. */
    std::vector<u8> disk(u64(4096) * 512, 0);
    disk[510] = 0x55;
    disk[511] = 0xAA;
    disk[446 + 16 + 4] = 0x05;
    put32(disk, 446 + 16 + 8, 2048);
    put32(disk, 446 + 16 + 12, 2040);
    /* EBR1 at 2048 is itself torn: no signature anywhere in the chain */
    MemDevice dev(std::move(disk));
    DiskTable t;
    ASSERT_EQ(parse_disk_table(&dev, t), OK);
    EXPECT_EQ(t.ebr_status, EbrChainStatus::Failed);
    EXPECT_EQ(t.ebr_break_lba, 2048u);
    EXPECT_EQ(t.partitions.size(), 0u);

    /* whole chain -> Ok, break 0 (the PlainWithExtended shape) */
    std::vector<u8> disk2(u64(4096) * 512, 0);
    disk2[510] = 0x55;
    disk2[511] = 0xAA;
    disk2[446 + 16 + 4] = 0x05;
    put32(disk2, 446 + 16 + 8, 2048);
    put32(disk2, 446 + 16 + 12, 2040);
    put32(disk2, 2048 * 512 + 446 + 0 * 16 + 4, 0x07);
    put32(disk2, 2048 * 512 + 446 + 0 * 16 + 8, 63);
    put32(disk2, 2048 * 512 + 446 + 0 * 16 + 12, 500);
    disk2[2048 * 512 + 510] = 0x55;
    disk2[2048 * 512 + 511] = 0xAA;
    MemDevice dev2(std::move(disk2));
    DiskTable t2;
    ASSERT_EQ(parse_disk_table(&dev2, t2), OK);
    EXPECT_EQ(t2.ebr_status, EbrChainStatus::Ok);
    EXPECT_EQ(t2.ebr_break_lba, 0u);
    EXPECT_EQ(t2.partitions.size(), 1u);
}

/* ---- session-5 continuation: the og two-step GPT salvage (F-A3-2/CARVE-1) ----
 * Ground truth: FUN_180006e20 (Partition_decompiled.c:4517-4634) - on
 * primary-header verify failure it first retries the primary's OWN entry
 * array (entries CRC only), and only then falls to the backup at the
 * device-derived last LBA. The pre-fix code could never reach either
 * path (last_lba came from the verified primary only), so a dead LBA1
 * always meant GptDamaged with zero partitions. */

namespace {
/* shared builder: healthy-disk GPT pieces the salvage tests corrupt */
struct GptDisk {
    std::vector<u8> disk;
    std::vector<u8> entries;
    std::vector<u8> entry_copies() const { return entries; }
};

GptDisk make_gpt_disk() {
    GptDisk gd;
    gd.disk.assign(u64(2048) * 512, 0);
    gd.disk[510] = 0x55;
    gd.disk[511] = 0xAA;
    gd.disk[446 + 4] = 0xEE;
    put32(gd.disk, 446 + 8, 1);
    put32(gd.disk, 446 + 12, 2047);

    std::memcpy(gd.disk.data() + 512, "EFI PART", 8);
    put32(gd.disk, 512 + 8, 0x00010000);
    put32(gd.disk, 512 + 12, 92);
    put64(gd.disk, 512 + 24, 1);
    put64(gd.disk, 512 + 32, 2047);
    put64(gd.disk, 512 + 40, 2);
    put64(gd.disk, 512 + 48, 2046);
    put64(gd.disk, 512 + 72, 2);
    put32(gd.disk, 512 + 80, 4);
    put32(gd.disk, 512 + 84, 128);

    gd.entries.assign(4 * 128, 0);
    const u8 basic[16] = {0xA2, 0xA0, 0xD0, 0xEB, 0xE5, 0xB9, 0x33, 0x44,
                          0x87, 0xC0, 0x68, 0xB6, 0xB7, 0x26, 0x99, 0xC7};
    std::memcpy(gd.entries.data(), basic, 16);
    put64(gd.entries, 32, 100);
    put64(gd.entries, 40, 200);
    std::memcpy(gd.disk.data() + 2 * 512, gd.entries.data(), gd.entries.size());
    put32(gd.disk, 512 + 88, crc32(gd.entries.data(), gd.entries.size()));
    put32(gd.disk, 512 + 16, crc32(gd.disk.data() + 512, 92));

    /* backup: entries at 2046, header at 2047 (mirrored LBAs) */
    std::memcpy(gd.disk.data() + 2046 * 512, gd.entries.data(), gd.entries.size());
    std::vector<u8> bh(gd.disk.begin() + 512, gd.disk.begin() + 1024);
    put32(bh, 16, 0);
    put64(bh, 24, 2047);
    put64(bh, 32, 1);
    put64(bh, 72, 2046);
    put32(bh, 16, crc32(bh.data(), 92));
    std::memcpy(gd.disk.data() + 2047 * 512, bh.data(), 512);
    return gd;
}
} // namespace

TEST(Gpt, DeadPrimaryHeaderSalvagesOwnEntries) {
    /* step 1: header CRC destroyed, entry array + entries CRC intact ->
     * partitions survive from the primary's own array; the header itself
     * stays untrusted (gpt_primary_ok false), layout stays Gpt */
    GptDisk gd = make_gpt_disk();
    put32(gd.disk, 512 + 16, 0xDEADBEEF); /* header CRC wrong */

    MemDevice dev(std::move(gd.disk));
    DiskTable t;
    ASSERT_EQ(parse_disk_table(&dev, t), OK);
    EXPECT_EQ(t.layout, TableLayout::Gpt);
    EXPECT_FALSE(t.gpt_primary_ok);
    EXPECT_TRUE(t.gpt_backup_ok); /* informational probe still runs */
    ASSERT_EQ(t.partitions.size(), 1u);
    EXPECT_EQ(t.partitions[0].start_lba, 100u);
    EXPECT_EQ(t.partitions[0].sector_count, 101u);
    EXPECT_EQ(t.partitions[0].fs, FsKind::Ntfs); /* full classification */
}

TEST(Gpt, ProtectiveOnlyVolumeRecoversFromBackup) {
    /* lost-partition shape: LBA1 dead entirely (no "EFI PART"), the
     * protective MBR is the only hint, the backup header at the last LBA
     * is intact -> og step 2 finds it (the pre-fix code never probed) */
    GptDisk gd = make_gpt_disk();
    std::memset(gd.disk.data() + 512, 0, 512); /* wipe the primary header */

    MemDevice dev(std::move(gd.disk));
    DiskTable t;
    ASSERT_EQ(parse_disk_table(&dev, t), OK);
    EXPECT_EQ(t.layout, TableLayout::Gpt);
    EXPECT_FALSE(t.gpt_primary_ok);
    EXPECT_TRUE(t.gpt_backup_ok);
    ASSERT_EQ(t.partitions.size(), 1u);
    EXPECT_EQ(t.partitions[0].start_lba, 100u);
    EXPECT_EQ(t.partitions[0].sector_count, 101u);
    EXPECT_EQ(t.partitions[0].fs, FsKind::Ntfs);
}

TEST(Gpt, HostileSalvageHeaderEntriesOutsideDiskRejected) {
    /* a corrupt header pointing the entry array beyond the device must
     * not be trusted by the salvage path (bounds gate) */
    GptDisk gd = make_gpt_disk();
    put32(gd.disk, 512 + 16, 0xDEADBEEF);             /* header CRC wrong */
    put64(gd.disk, 512 + 72, 999999);                 /* entries_start_lba off-disk */
    std::memset(gd.disk.data() + 2047 * 512, 0, 512); /* no backup either */

    MemDevice dev(std::move(gd.disk));
    DiskTable t;
    ASSERT_EQ(parse_disk_table(&dev, t), OK);
    EXPECT_EQ(t.layout, TableLayout::GptDamaged);
    EXPECT_TRUE(t.partitions.empty());
}

/* ---- A-001: canonical GPT type GUIDs (disk-byte order) ---------------- */

namespace {

/* Mixed-endian (EFI GUID) disk bytes derived independently from the
 * canonical Microsoft text forms - the same derivation every real GPT
 * formatter performs (Data1/Data2/Data3 little-endian, Data4 raw).
 * This helper does NOT read the constants under test. */
std::vector<u8> guid_disk_bytes(const char* text) {
    std::string t;
    for (const char* p = text; *p; ++p)
        if (*p != '-' && *p != '{' && *p != '}')
            t += *p;
    auto hx = [&](size_t i) { return u8(strtoul(t.substr(i, 2).c_str(), nullptr, 16)); };
    std::vector<u8> b(16);
    b[0] = hx(6);
    b[1] = hx(4);
    b[2] = hx(2);
    b[3] = hx(0); /* Data1 LE */
    b[4] = hx(10);
    b[5] = hx(8); /* Data2 LE */
    b[6] = hx(14);
    b[7] = hx(12); /* Data3 LE */
    for (int i = 0; i < 8; i++)
        b[8 + i] = hx(16 + 2 * size_t(i)); /* Data4 raw */
    return b;
}

void expect_guid(const u8* constant, const char* canonical_text, const char* name) {
    const std::vector<u8> want = guid_disk_bytes(canonical_text);
    EXPECT_EQ(0, std::memcmp(constant, want.data(), 16))
        << name << " != canonical " << canonical_text;
}

} // namespace

TEST(Gpt, CanonicalTypeGuidConstantsMatchRealDiskBytes) {
    /* A-001/A-002 regression guard: every GPT type-GUID constant must
     * equal the canonical mixed-endian disk bytes derived independently
     * from the Microsoft text forms. A-001 fixed BIOS boot/MSR/Linux/ESP
     * (previously mangled, duplicated, fabricated); A-002 then restored
     * the two LDM GUIDs, which the A-001 fix had itself corrupted with
     * fabricated values (in no og binary, no authoritative source) -
     * every real GPT dynamic disk failed LDM discovery until A-002. */
    expect_guid(GPT_TYPE_EFI_SYSTEM, "c12a7328-f81f-11d2-ba4b-00a0c93ec93b", "ESP");
    expect_guid(GPT_TYPE_BIOS_BOOT, "21686148-6449-6e6f-744e-656564454649", "BIOS boot");
    expect_guid(GPT_TYPE_MICROSOFT_RESERVED, "e3c9e316-0b5c-4db8-817d-f92df00215ae", "MSR");
    expect_guid(GPT_TYPE_BASIC_DATA, "ebd0a0a2-b9e5-4433-87c0-68b6b72699c7", "basic data");
    expect_guid(GPT_TYPE_LINUX_FS, "0fc63daf-8483-4772-8e79-3d69d8477de4", "Linux fs");
    /* A-002: the canonical text forms are the REAL Microsoft winioctl.h
     * values (PARTITION_LDM_METADATA_GUID / PARTITION_LDM_DATA_GUID,
     * Microsoft Learn PARTITION_INFORMATION_GPT; og binaries carry the
     * same bytes: LdmManager.mo DAT_1800c4960, Partition.mo
     * DAT_180036630/DAT_180036640). The 1cb6a43 values (8e9f/384e rows)
     * were fabricated - present in no og binary and no authoritative
     * source; they regressed every real GPT dynamic disk to
     * ERR_BAD_MAGIC (diff-to-fix A-002). */
    expect_guid(GPT_TYPE_LDM_METADATA, "5808c8aa-7e8f-42e0-85d2-e1e90434cfb3", "LDM metadata");
    expect_guid(GPT_TYPE_LDM_DATA, "af9b60a0-1431-4f62-bc68-3311714a69ad", "LDM data");
    /* sanity of the derivation itself: the BIOS-boot GUID's disk bytes are
     * the ASCII string "Hah!IdontNeedEFI" (the canonical curiosity that
     * makes byte-order bugs visible) */
    const u8 hahi[16] = {'H', 'a', 'h', '!', 'I', 'd', 'o', 'n',
                         't', 'N', 'e', 'e', 'd', 'E', 'F', 'I'};
    EXPECT_EQ(0, std::memcmp(GPT_TYPE_BIOS_BOOT, hahi, 16));
}

namespace {

/* A-002 guard: format the mixed-endian constants back to GUID TEXT and
 * compare against the winioctl.h string literals. Text comparison makes
 * a regressed constant self-evident in the failure message (the printed
 * text is directly diffable against Microsoft Learn / winioctl.h) and
 * catches transposed-hex edits that a byte-array eyeball diff misses. */
std::string guid_text(const u8* disk_bytes) {
    char buf[40];
    std::snprintf(buf, sizeof(buf), "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                  u32(disk_bytes[0]) | (u32(disk_bytes[1]) << 8) | (u32(disk_bytes[2]) << 16) |
                      (u32(disk_bytes[3]) << 24),
                  u16(disk_bytes[4]) | (u16(disk_bytes[5]) << 8),
                  u16(disk_bytes[6]) | (u16(disk_bytes[7]) << 8), disk_bytes[8], disk_bytes[9],
                  disk_bytes[10], disk_bytes[11], disk_bytes[12], disk_bytes[13], disk_bytes[14],
                  disk_bytes[15]);
    return std::string(buf);
}

} // namespace

TEST(Gpt, LdmGuidConstantsMatchMicrosoftWinioctlText) {
    /* A-002: the two LDM constants must be exactly Microsoft's documented
     * PARTITION_LDM_METADATA_GUID / PARTITION_LDM_DATA_GUID (winioctl.h,
     * PARTITION_INFORMATION_GPT on Microsoft Learn), which is also what
     * the og binaries compare against (LdmManager.mo DAT_1800c4960;
     * Partition.mo DAT_180036630/DAT_180036640) and what dmio writes on
     * every real GPT dynamic disk. */
    EXPECT_EQ(guid_text(GPT_TYPE_LDM_METADATA), "5808c8aa-7e8f-42e0-85d2-e1e90434cfb3");
    EXPECT_EQ(guid_text(GPT_TYPE_LDM_DATA), "af9b60a0-1431-4f62-bc68-3311714a69ad");
}

TEST(Gpt, EspMsrLinuxBiosbootLdmClassification) {
    /* A-001: canonical-GUID entries classify to their own kinds. Before
     * the fix, ESP matched the (duplicated, wrong) MSR constant and
     * classified Reserved; MSR/Linux/LDM-metadata fell to Other. */
    std::vector<u8> disk(u64(2048) * 512, 0);
    disk[510] = 0x55;
    disk[511] = 0xAA;
    disk[446 + 4] = 0xEE;
    put32(disk, 446 + 8, 1);
    put32(disk, 446 + 12, 2047);

    std::memcpy(disk.data() + 512, "EFI PART", 8);
    put32(disk, 512 + 8, 0x00010000);
    put32(disk, 512 + 12, 92);
    put64(disk, 512 + 24, 1);
    put64(disk, 512 + 32, 2047);
    put64(disk, 512 + 40, 2);
    put64(disk, 512 + 48, 2046);
    put64(disk, 512 + 72, 2);
    put32(disk, 512 + 80, 6);
    put32(disk, 512 + 84, 128);

    std::vector<u8> entries(6 * 128, 0);
    struct E {
        const u8* guid;
        u64 first, last;
    } es[] = {
        {GPT_TYPE_EFI_SYSTEM, 200, 399},     {GPT_TYPE_MICROSOFT_RESERVED, 400, 431},
        {GPT_TYPE_LINUX_FS, 500, 900},       {GPT_TYPE_BIOS_BOOT, 950, 1050},
        {GPT_TYPE_LDM_METADATA, 1200, 1300}, {GPT_TYPE_LDM_DATA, 1400, 1600},
    };
    for (size_t i = 0; i < 6; i++) {
        std::memcpy(entries.data() + i * 128, es[i].guid, 16);
        put64(entries, i * 128 + 32, es[i].first);
        put64(entries, i * 128 + 40, es[i].last);
    }
    put32(disk, 512 + 88, crc32(entries.data(), entries.size()));
    std::memcpy(disk.data() + 2 * 512, entries.data(), entries.size());
    put32(disk, 512 + 16, crc32(disk.data() + 512, 92));

    MemDevice dev(std::move(disk));
    DiskTable t;
    ASSERT_EQ(parse_disk_table(&dev, t), OK);
    EXPECT_EQ(t.layout, TableLayout::Gpt);
    ASSERT_EQ(t.partitions.size(), 6u);
    EXPECT_EQ(t.partitions[0].fs, FsKind::EfiSystem);
    EXPECT_EQ(t.partitions[1].fs, FsKind::Reserved); /* MSR */
    EXPECT_EQ(t.partitions[2].fs, FsKind::Linux);
    EXPECT_EQ(t.partitions[3].fs, FsKind::Reserved); /* BIOS boot */
    EXPECT_EQ(t.partitions[4].fs, FsKind::Reserved); /* LDM metadata */
    EXPECT_EQ(t.partitions[5].fs, FsKind::Ldm);      /* LDM data */
}
