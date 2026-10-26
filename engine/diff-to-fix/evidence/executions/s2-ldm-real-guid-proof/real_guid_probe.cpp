// s2-probe: GPT dynamic-disk LDM discovery with REAL canonical Microsoft GUIDs
// (PARTITION_LDM_METADATA_GUID 5808c8aa-7e8f-42e0-85d2-e1e90434cfb3 per Microsoft Learn,
//  Linux kernel efi.h, and the og binaries themselves).
// Builds a minimal GPT+LDM image like test_ldm.cpp's GptDynamicDiskCanonicalGuidsParse,
// but with the REAL GUID bytes, and runs the CURRENT core's parse_ldm_database.
#include "cr/table/cr_ldm.h"
#include "cr/table/cr_table_wire.h"

#include "cr/storage/cr_slice.h"
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <vector>
#include <memory>

using namespace cr;
using namespace cr::table;

typedef std::uint8_t u8; typedef std::uint32_t u32; typedef std::uint64_t u64;

// MemDevice modeled on tests/unit/test_ldm.cpp:31-50 (read-only in-memory IStorageDevice).
class MemDevice : public IStorageDevice {
public:
    explicit MemDevice(std::vector<u8> d) : d_(std::move(d)) {
        g_.logical_sector = 512;
        g_.physical_sector = 512;
        g_.total_bytes = d_.size();
        g_.sector_count = d_.size() / 512;
    }
    u32 read_at(u64 off, void* buf, u32 len) override {
        if (off > d_.size() || len > d_.size() - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        memcpy(buf, d_.data() + off, len);
        return OK;
    }
    const Geometry& geometry() const override { return g_; }
private:
    Geometry g_{};
    std::vector<u8> d_;
};

static void put32le(u8* p, u32 v) { p[0]=u8(v); p[1]=u8(v>>8); p[2]=u8(v>>16); p[3]=u8(v>>24); }
static void put64le(u8* p, u64 v) { for (int i=0;i<8;i++) p[i]=u8(v>>(8*i)); }
static void p32be(u8* p, u32 v) { p[0]=u8(v>>24); p[1]=u8(v>>16); p[2]=u8(v>>8); p[3]=u8(v); }

// Minimal PRIVHEAD (512 bytes) with the fields cr_ldm's parse_privhead validates.
// Modeled on the field offsets validated by the og (LdmManager FUN_180072f40) and libldm:
//   magic 'PRIVHEAD' at +0, ver 2.11 at +0xC/+0xE, disk-id GUID at +0x30,
//   db-start at +0x11B, db-size at +0x123, config-size at +0x133 (0x800),
//   logical-disk-start +0x153, size +0x15B, host-config +0x143.
static void build_privhead(u8* ph, u64 db_start, u64 db_size, u64 cfg_start, u64 disk_size) {
    std::memset(ph, 0, 512);
    std::memcpy(ph, "PRIVHEAD", 8);
    put32le(ph + 0xC, 2); put32le(ph + 0xE, 0xB);   // version 2.11 (og checks ==2, ==0xB)
    std::memcpy(ph + 0x30, "\xaa\xaa\xaa\xaa\xbb\xbb\xbb\xbb\xcc\xcc\xcc\xcc\xdd\xdd\xdd\xdd", 16);
    put64le(ph + 0x11B, db_start);   // logical disk start (db start)
    put64le(ph + 0x123, db_size);    // logical disk size
    put64le(ph + 0x12B, disk_size);  // disk size field (og size checks skipped in gpt mode)
    put64le(ph + 0x133, 0x800);      // config size
    put64le(ph + 0x153, cfg_start);  // config start
    put64le(ph + 0x15B, 0x800);      // toc start? (head1)
}

int main() {
    const u64 SECT = 65536;
    std::vector<u8> disk(SECT * 512, 0);

    // ---- real canonical GPT type GUIDs (mixed-endian disk bytes) ----
    const u8 REAL_LDM_META[16] = {0xAA,0xC8,0x08,0x58,0x8F,0x7E,0xE0,0x42,0x85,0xD2,0xE1,0xE9,0x04,0x34,0xCF,0xB3};
    const u8 REAL_LDM_DATA[16] = {0xA0,0x60,0x9B,0xAF,0x31,0x14,0x62,0x4F,0xBC,0x68,0x33,0x11,0x71,0x4A,0x69,0xAD};
    // what the CURRENT tree holds (post-1cb6a43):
    const u8* CORE_META = GPT_TYPE_LDM_METADATA;
    const u8* CORE_DATA = GPT_TYPE_LDM_DATA;

    printf("core GPT_TYPE_LDM_METADATA bytes: ");
    for (int i=0;i<16;i++) printf("%02x", CORE_META[i]);
    printf("\nreal canonical metadata bytes:    ");
    for (int i=0;i<16;i++) printf("%02x", REAL_LDM_META[i]);
    printf("\ncore metadata == real canonical? %s\n",
           std::memcmp(CORE_META, REAL_LDM_META, 16)==0 ? "YES" : "NO  <-- REAL GPT DYNAMIC DISKS FAIL");
    printf("core GPT_TYPE_LDM_DATA bytes:     ");
    for (int i=0;i<16;i++) printf("%02x", CORE_DATA[i]);
    printf("\nreal canonical data bytes:        ");
    for (int i=0;i<16;i++) printf("%02x", REAL_LDM_DATA[i]);
    printf("\ncore data == real canonical? %s\n",
           std::memcmp(CORE_DATA, REAL_LDM_DATA, 16)==0 ? "YES" : "NO  <-- LDM data partitions misclassify");

    // ---- protective MBR ----
    u8* mbr = disk.data();
    mbr[510]=0x55; mbr[511]=0xAA;
    mbr[0x1BE + 4]=0xEE; p32be(mbr+0x1BE + 8, 1); p32be(mbr+0x1BE + 12, u32(SECT-1));

    // ---- GPT header LBA1 ----
    u8* h = disk.data() + 512;
    std::memcpy(h, "EFI PART", 8);
    put32le(h+8, 0x00010000); put32le(h+12, 92);
    put64le(h+24, 1); put64le(h+32, SECT-1);
    put64le(h+40, 2); put64le(h+48, SECT-34);
    put64le(h+72, 2); put32le(h+80, 2); put32le(h+84, 128);

    // ---- 2 entries: metadata [63488,65532], data [2048,4096] ----
    const u64 META_START = 63488, META_LAST = 65532;
    std::vector<u8> entries(2*128, 0);
    std::memcpy(entries.data()+0, REAL_LDM_META, 16);
    put64le(entries.data()+32, META_START); put64le(entries.data()+40, META_LAST);
    std::memcpy(entries.data()+ 128, REAL_LDM_DATA, 16);
    put64le(entries.data()+ 128 + 32, 2048); put64le(entries.data()+ 128 + 40, 4096);
    put32le(h+88, crc32(entries.data(), entries.size()));
    std::memcpy(disk.data()+2*512, entries.data(), entries.size());
    put32le(h+16, crc32(h, 92));
    // backup entries + header
    std::memcpy(disk.data()+(SECT-2)*512, entries.data(), entries.size());
    u8* bh = disk.data()+(SECT-1)*512;
    std::memcpy(bh, h, 512); put32le(bh+16,0); put64le(bh+24,SECT-1); put64le(bh+32,1);
    put64le(bh+72, SECT-2); put32le(bh+16, crc32(bh, 92));

    // ---- PRIVHEAD at the metadata partition's LAST sector (og reads entry+0x28 = last LBA) ----
    u8* ph = disk.data() + META_LAST*512;
    build_privhead(ph, META_START, 2048, 63488, SECT-34);

    MemDevice dev(std::move(disk));
    LdmInfo info;
    const int rc = parse_ldm_database(&dev, info);
    printf("parse_ldm_database with REAL canonical GUIDs: rc=%d (%s)\n", rc,
           rc==OK ? "OK — GPT LDM reachable" : "FAILS — real GPT dynamic disks get zero LDM recovery");
    return rc==OK ? 0 : 1;
}
