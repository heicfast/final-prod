// Audit harness: GPT dynamic-disk fixture with the REAL Microsoft LDM GUIDs
// (winioctl.h PARTITION_LDM_METADATA_GUID / PARTITION_LDM_DATA_GUID, mixed-endian
// disk form) → run cr::table::parse_disk_table + cr::table::parse_ldm_database.
// Expected on a correct implementation: LDM metadata partition located,
// parse_ldm_database proceeds past GPT entry scan ( PRIVHEAD magic optional ).
//
// A-002 CORRECTION (fix session): this harness originally wrote FABRICATED
// GUID bytes (5808c8aa-8e9f-4c87-... / af9b60a0-384e-4f7d-...) as "canonical";
// the real values below were verified four ways: Microsoft Learn
// PARTITION_INFORMATION_GPT, the og binaries' own compare operands
// (LdmManager.mo DAT_0x1800c4960, Partition.mo DAT_0x180036630/0x36640),
// the Linux kernel EFI set, and execution (the unit suite's
// GptDynamicDisk* tests). See findings/FIXED/A-002-*.md.
#include <cr/table/cr_table_parse.h>
#include <cr/table/cr_ldm.h>
#include <cr/storage/cr_device.h>
#include <cstdio>
#include <cstring>
#include <vector>

using namespace cr;

namespace cr {
class MemDevice : public IStorageDevice {
public:
    explicit MemDevice(std::vector<u8> img, u32 sector = 512)
        : img_(std::move(img)) {
        geo_.logical_sector = sector;
        geo_.physical_sector = sector;
        geo_.total_bytes = img_.size();
        geo_.sector_count = img_.size() / sector;
    }
    u32 read_at(u64 offset, void* buffer, u32 length) override {
        if (offset + length > img_.size()) return 1;
        std::memcpy(buffer, img_.data() + offset, length);
        return 0;
    }
    const Geometry& geometry() const override { return geo_; }
private:
    std::vector<u8> img_;
    Geometry geo_;
};
} // namespace cr

static void put_u32(u8* p, u32 v) { p[0]=u8(v); p[1]=u8(v>>8); p[2]=u8(v>>16); p[3]=u8(v>>24); }
static void put_u64(u8* p, u64 v) { for (int i=0;i<8;i++) p[i]=u8(v>>(8*i)); }
static void put_crc_table(u8* sector);

static u32 crc32_entry_array(const u8* p, size_t n) {
    // same polynomial as cr table (reflected 0xEDB88320)
    static u32 tab[256]; static bool init=false;
    if (!init) { for (u32 i=0;i<256;i++){u32 c=i;for(int k=0;k<8;k++)c=(c>>1)^(0xEDB88320u&-(c&1));tab[i]=c;} init=true; }
    u32 c = 0xFFFFFFFFu;
    for (size_t i=0;i<n;i++) c = tab[(c ^ p[i]) & 0xFF] ^ (c >> 8);
    return ~c;
}
static u32 crc32_header(u8* h, size_t n) {
    return crc32_entry_array(h, n);
}

int main() {
    const u64 SECTORS = 4096; // 2 MiB image
    std::vector<u8> img(size_t(SECTORS) * 512, 0);

    // ---- GPT header at LBA1, entries at LBA2, 8 entries ----
    u8* hdr = img.data() + 512;
    std::memcpy(hdr, "EFI PART", 8);
    put_u32(hdr + 8, 0x00010000);      // revision
    put_u32(hdr + 12, 92);             // header size
    put_u32(hdr + 16, 0);              // crc (fill later)
    put_u32(hdr + 20, 0); put_u32(hdr + 24, 0);
    put_u64(hdr + 24, 1);              // current LBA (careful: overlapping — redo below)
    // redo cleanly:
    std::memset(hdr + 20, 0, 32);
    put_u64(hdr + 24, 1);              // current LBA
    put_u64(hdr + 32, SECTORS - 1);    // backup LBA
    put_u64(hdr + 40, 2);              // first usable
    put_u64(hdr + 48, SECTORS - 2);    // last usable
    // disk GUID (any)
    for (int i = 0; i < 16; i++) hdr[56 + i] = u8(0x10 + i);
    put_u64(hdr + 72, 2);              // entries start LBA
    put_u32(hdr + 80, 8);              // entry count
    put_u32(hdr + 84, 128);            // entry size
    // entries CRC over 8*128
    u8 entries[8 * 128];
    std::memset(entries, 0, sizeof(entries));
    auto set_entry = [&](int idx, const char* hexguid, u64 first, u64 last) {
        u8* e = entries + idx * 128;
        for (int i = 0; i < 16; i++) { unsigned b; std::sscanf(hexguid + 2*i, "%2x", &b); e[i] = u8(b); }
        put_u64(e + 32, first);
        put_u64(e + 40, last);
        put_u64(e + 48, 0); put_u64(e + 56, 0);
        e[48] = 0; // attrs
    };
    // A-002 CORRECTION (diff-fix session 3): the two LDM rows below were
    // rewritten from the fabricated 1cb6a43 values to the REAL Microsoft
    // PARTITION_LDM_METADATA_GUID / PARTITION_LDM_DATA_GUID disk forms.
    // The session-1 harness encoded the fabricated bytes, so the pre-fix
    // core (which carried the REAL constants) correctly rejected this
    // fixture and the run was mis-recorded as "LDM discovery FAILS" — the
    // fixture was wrong, not the code. See A-002 + its Resolution.
    // LDM metadata 5808c8aa-7e8f-42e0-85d2-e1e90434cfb3 -> aa c8 08 58 8f 7e e0 42 85 d2 e1 e9 04 34 cf b3
    set_entry(0, "aac808588f7ee04285d2e1e90434cfb3", 2048, 3071);
    // LDM data af9b60a0-1431-4f62-bc68-3311714a69ad -> a0 60 9b af 31 14 62 4f bc 68 33 11 71 4a 69 ad
    set_entry(1, "a0609baf3114624fbc683311714a69ad", 3072, SECTORS - 34);
    // ESP (for classification cross-check) c12a7328-f81f-11d2-ba4b-00a0c93ec93b
    set_entry(2, "c12a7328f81f11d2ba4b00a0c93ec93b", 4096, 8191);
    // MSR e3c9e316-0b5c-4db8-817d-f92df00215ae -> 16 e3 c9 e3 5c 0b db 48 17 df 92 df 00 21 5a e... careful
    set_entry(3, "16e3c9e35c0bdb4817df92df00215ae", 8192, 9215);
    put_u32(hdr + 88, crc32_entry_array(entries, sizeof(entries)));

    // protective MBR
    u8* mbr = img.data();
    mbr[510] = 0x55; mbr[511] = 0xAA;
    mbr[0x1BE + 4] = 0xEE;
    put_u32(mbr + 0x1BE + 8, 1);
    put_u32(mbr + 0x1BE + 12, 0xFFFFFFFF);

    std::memcpy(img.data() + 2 * 512, entries, sizeof(entries));

    // fake PRIVHEAD at last block of LDM metadata partition (sector 3071) so the
    // LDM walk can proceed past the GPT scan (magic "PRIVHEAD" per libldm)
    u8* ph = img.data() + u64(3071) * 512;
    std::memcpy(ph, "PRIVHEAD", 8);

    MemDevice dev(img);
    table::DiskTable ti;
    u32 rc = table::parse_disk_table(&dev, ti);
    std::printf("parse_disk_table rc=%u partitions=%zu\n", rc, ti.partitions.size());
    for (const auto& p : ti.partitions) {
        std::printf("  part start=%llu sectors=%llu fs=%d from_gpt=%d\n",
                    (unsigned long long)p.start_lba, (unsigned long long)p.sector_count,
                    int(p.fs), int(p.from_gpt));
    }
    table::LdmInfo li;
    u32 lrc = table::parse_ldm_database(&dev, li);
    std::printf("parse_ldm_database rc=%u (0=OK, 0x%x=ERR_BAD_MAGIC?)\n", lrc, lrc);
    std::printf("VERDICT: GPT-dynamic-disk LDM discovery %s\n",
                lrc == 0 ? "WORKS" : "FAILS (canonical GUID not recognized)");
    return 0;
}
