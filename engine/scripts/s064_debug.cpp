/* S-064 debug: trace where the fixtures fail. */
#include "cr/fat/cr_fat_engine.h"
#include "cr/fat/cr_fat_wire.h"
#include <cstdio>
#include <cstring>
#include <vector>

using namespace cr;

static void put16(std::vector<u8>& img, size_t off, u16 v) {
    img[off] = u8(v & 0xFF);
    img[off + 1] = u8(v >> 8);
}
static void put32v(std::vector<u8>& img, size_t off, u32 v) {
    img[off] = u8(v & 0xFF);
    img[off + 1] = u8((v >> 8) & 0xFF);
    img[off + 2] = u8((v >> 16) & 0xFF);
    img[off + 3] = u8((v >> 24) & 0xFF);
}

int main() {
    /* the FAT32-under-bracket fixture */
    std::vector<u8> img(u64(20000) * 512, 0);
    u8* b = img.data();
    std::memcpy(b + 3, "MSWIN4.1", 8);
    put16(img, 0x0B, 512);
    b[0x0D] = 1;
    put16(img, 0x0E, 2);
    b[0x10] = 2;
    put16(img, 0x11, 0);
    put16(img, 0x13, 0);
    b[0x15] = 0xF8;
    put16(img, 0x16, 0);
    put32v(img, 0x20, 20000);
    put32v(img, 0x24, 20);
    put32v(img, 0x2C, 2);
    std::memcpy(b + 0x52, "FAT32   ", 8);
    b[510] = 0x55;
    b[511] = 0xAA;

    fat::FatParams p;
    const u32 rc = fat::validate_boot(img.data(), img.size(), p);
    std::printf("validate_boot rc=0x%x flavor=%u cluster_count=%u first_data=%llu\n", rc,
                unsigned(p.flavor), p.cluster_count, (unsigned long long)p.first_data_sector);
    if (rc == OK) {
        const u32 rc2 = fat::boot_passes_open_gates(img.data(), 512, p);
        std::printf("boot_passes_open_gates rc=0x%x\n", rc2);
    }
    return 0;
}
