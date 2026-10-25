/*
 * cr_table_wire.h - on-disk partition-table structures: MBR, EBR, GPT.
 *
 * Format constants are the public specification values; each one that the RE
 * corpus also confirms carries its evidence note. All structures are
 * packed, offset-asserted, and pure (no OS headers).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_TABLE_WIRE_H_
#define CR_TABLE_WIRE_H_

#include "../cr_types.h"
#include <cstring>

namespace cr {
namespace table {

#pragma pack(push, 1)

/* ---- Master Boot Record / Extended Boot Record ------------------------ */

constexpr u16 MBR_SIGNATURE = 0xAA55; /* at +0x1FE, little-endian 55 AA */
constexpr u8 MBR_TYPE_GPT_PROTECTIVE = 0xEE;
constexpr u8 MBR_TYPE_EXTENDED_LBA = 0x05; /* CHS variant 0x0F also links */
constexpr u8 MBR_TYPE_EXTENDED_CHS = 0x0F;
constexpr u8 MBR_TYPE_LDM = 0x42; /* dynamic disk (recognized, R3.3) */

struct MbrPartitionEntry {
    u8 boot_flag; /* 0x00 / 0x80 */
    u8 chs_start[3];
    u8 type;
    u8 chs_end[3];
    u32 lba_start;
    u32 sector_count;
};
static_assert(sizeof(MbrPartitionEntry) == 16);

struct MbrSector {
    u8 code[446];
    MbrPartitionEntry entries[4];
    u8 signature[2]; /* 55 AA */
};
static_assert(sizeof(MbrSector) == 512);

inline bool mbr_signature_ok(const u8* sector) {
    return sector[510] == 0x55 && sector[511] == 0xAA;
}
inline bool mbr_entry_used(const MbrPartitionEntry& e) {
    return e.type != 0;
}

/* ---- GPT -------------------------------------------------------------- */

constexpr char GPT_SIGNATURE[8] = {'E', 'F', 'I', ' ', 'P', 'A', 'R', 'T'};
constexpr u32 GPT_REVISION_V1 = 0x00010000;
/* GPT type GUIDs in MIXED-ENDIAN DISK-BYTE order (EFI GUID layout:
 * Data1/Data2/Data3 little-endian, Data4 raw) - the byte order real
 * GPT entries carry on disk. Two-source provenance per constant:
 *    Microsoft winioctl.h text form (PARTITION_INFORMATION_GPT on
 *    Microsoft Learn) and the og binaries' own compare constants:
 *    EFI System  {c12a7328-f81f-11d2-ba4b-00a0c93ec93b}  (Partition.mo)
 *    MSR         {e3c9e316-0b5c-4db8-817d-f92df00215ae}  (Partition.mo)
 *    BIOS boot   {21686148-6449-6e6f-744e-656564454649} = "Hah!IdontNeedEFI"
 *    Linux FS    {0fc63daf-8483-4772-8e79-3d69d8477de4}  (kernel efi.h agreement)
 *    LDM meta    {5808c8aa-7e8f-42e0-85d2-e1e90434cfb3}  (LdmManager.mo
 *                 DAT_1800c4960 = Partition.mo DAT_180036630; the GPT-LDM
 *                 gate memcmp target in FUN_1800724f0)
 *    LDM data    {af9b60a0-1431-4f62-bc68-3311714a69ad}  (Partition.mo
 *                 DAT_180036640)
 * A-002 provenance note: the 1cb6a43 values for LDM meta/data
 * ({5808c8aa-8e9f-4c87-...} / {af9b60a0-384e-4f7d-...}) were fabricated -
 * present in no og binary (1,470-file corpus scan), no authoritative
 * source, and no real disk - and regressed every real GPT dynamic disk
 * to ERR_BAD_MAGIC. The values below are Microsoft's actual
 * PARTITION_LDM_METADATA_GUID / PARTITION_LDM_DATA_GUID (winioctl.h;
 * Linux kernel efi.h and Wikipedia's GPT table agree), re-verified by
 * PE-section extraction from the og binaries.
 */
constexpr u8 GPT_TYPE_EFI_SYSTEM[16] = {0x28, 0x73, 0x2A, 0xC1, 0x1F, 0xF8, 0xD2, 0x11,
                                        0xBA, 0x4B, 0x00, 0xA0, 0xC9, 0x3E, 0xC9, 0x3B};
constexpr u8 GPT_TYPE_BIOS_BOOT[16] = {0x48, 0x61, 0x68, 0x21, 0x49, 0x64, 0x6F, 0x6E,
                                       0x74, 0x4E, 0x65, 0x65, 0x64, 0x45, 0x46, 0x49};
constexpr u8 GPT_TYPE_MICROSOFT_RESERVED[16] = {0x16, 0xE3, 0xC9, 0xE3, 0x5C, 0x0B, 0xB8, 0x4D,
                                                0x81, 0x7D, 0xF9, 0x2D, 0xF0, 0x02, 0x15, 0xAE};
constexpr u8 GPT_TYPE_BASIC_DATA[16] = {0xA2, 0xA0, 0xD0, 0xEB, 0xE5, 0xB9, 0x33, 0x44,
                                        0x87, 0xC0, 0x68, 0xB6, 0xB7, 0x26, 0x99, 0xC7};
constexpr u8 GPT_TYPE_LINUX_FS[16] = {0xAF, 0x3D, 0xC6, 0x0F, 0x83, 0x84, 0x72, 0x47,
                                      0x8E, 0x79, 0x3D, 0x69, 0xD8, 0x47, 0x7D, 0xE4};
constexpr u8 GPT_TYPE_LDM_METADATA[16] = {0xAA, 0xC8, 0x08, 0x58, 0x8F, 0x7E, 0xE0, 0x42,
                                          0x85, 0xD2, 0xE1, 0xE9, 0x04, 0x34, 0xCF, 0xB3};
constexpr u8 GPT_TYPE_LDM_DATA[16] = {0xA0, 0x60, 0x9B, 0xAF, 0x31, 0x14, 0x62, 0x4F,
                                      0xBC, 0x68, 0x33, 0x11, 0x71, 0x4A, 0x69, 0xAD};

struct GptHeader {
    char signature[8]; /* "EFI PART" */
    u32 revision;
    u32 header_size;  /* usually 92 */
    u32 header_crc32; /* over header with this field zeroed */
    u32 reserved;
    u64 current_lba;
    u64 backup_lba;
    u64 first_usable_lba;
    u64 last_usable_lba;
    u8 disk_guid[16];
    u64 entries_start_lba;
    u32 entry_count;
    u32 entry_size;    /* usually 128 */
    u32 entries_crc32; /* over the entry array */
    u8 padding[420];   /* to 512 */
};
static_assert(sizeof(GptHeader) == 512);

struct GptEntry {
    u8 type_guid[16];
    u8 unique_guid[16];
    u64 first_lba;
    u64 last_lba;
    u64 attributes;
    u16 name[36]; /* UTF-16LE, zero-padded; 72 bytes -> 128 total */
};
static_assert(sizeof(GptEntry) == 128);

#pragma pack(pop)

/* CRC32 (IEEE 802.3, reflected, init 0xFFFFFFFF, final xor) - GPT uses the
 * standard zlib/Linux crc32; shared with the session container format. */
u32 crc32(const void* data, size_t len);

inline bool gpt_entry_used(const GptEntry& e) {
    for (u8 b : e.type_guid)
        if (b != 0)
            return true;
    return false;
}
inline bool gpt_guid_equal(const u8* a, const u8* b) {
    return std::memcmp(a, b, 16) == 0;
}

} // namespace table
} // namespace cr

#endif // CR_TABLE_WIRE_H_
