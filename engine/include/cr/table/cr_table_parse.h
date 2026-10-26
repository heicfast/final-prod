/*
 * cr_table_parse.h - partition-table parsing with full validation.
 *
 * GPT: primary + backup headers, entry-array CRC32 (header +0x58 field),
 * protective MBR. MBR: signature, 4 entries, EBR chain walk with visited-set
 * cycle guard (fix F7). LDM (0x42) databases are fully decoded by cr_ldm
 * (session 5); spanned/striped dynamic volumes surface as virtual devices.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_TABLE_PARSE_H_
#define CR_TABLE_PARSE_H_

#include "cr_table_wire.h"
#include "cr/storage/cr_device.h"
#include <memory>
#include <vector>

namespace cr {
namespace table {

enum class FsKind : u32 {
    Unknown = 0,
    Ntfs,
    Fat12,
    Fat16,
    Fat32,
    Exfat,
    Refs,
    Ldm,        /* dynamic disk data (recognized only) */
    Extended,   /* container, not a volume */
    Protective, /* GPT protective MBR entry */
    Reserved,   /* GPT reserved (MSR/BIOS boot/LDM metadata) */
    Linux,
    Other,
    EfiSystem, /* EFI System Partition (appended: A-001 - ESP is its own kind,
                * not conflated with MSR; the scan layer probes content, so a
                * distinct kind only improves reporting/lost-partition inputs) */
};

enum class TableLayout : u32 { None = 0, Mbr, Ebr, Gpt, GptDamaged };

struct PartitionInfo {
    u64 start_lba = 0;
    u64 sector_count = 0;
    FsKind fs = FsKind::Unknown;
    u8 mbr_type = 0;
    u64 gpt_attrs = 0;
    u16 name[36] = {0}; /* GPT UTF16 name (up to 36 chars incl. NUL) */
    bool from_gpt = false;
    bool used = false; /* entry occupied */
};

/* M-002 (diff-to-fix): the extended-container chain verdict
 * (DiskTable::ebr_status). A chain that breaks mid-walk KEEPS every
 * volume already walked (salvage-over-purity — the og's deep-scan block
 * parsers re-find volumes independently of the EBR chain,
 * mo-module-map; the core's old gate discarded the walked set on the
 * FIRST bad link). */
enum class EbrChainStatus : u32 {
    None = 0, /* no extended container on the disk */
    Ok,       /* whole chain walked (or cycle-guarded) */
    Partial,  /* chain broke mid-walk; the walked prefix is in partitions */
    Failed,   /* chain broke before ANY volume was walked */
};

/* Result of reading the on-disk table of one disk. */
struct DiskTable {
    TableLayout layout = TableLayout::None;
    bool gpt_primary_ok = false;
    bool gpt_backup_ok = false;
    u32 gpt_entry_count = 0;
    u8 disk_guid[16] = {0};
    u64 first_usable_lba = 0;
    u64 last_usable_lba = 0;
    std::vector<PartitionInfo> partitions;
    /* M-002: torn-chain honesty — ebr_break_lba is the LBA of the first
     * unreadable/invalid EBR link (0 when the chain is whole). */
    EbrChainStatus ebr_status = EbrChainStatus::None;
    u64 ebr_break_lba = 0;
};

class CancelRegistry; /* fwd */

/*
 * parse_disk_table - reads sector 0 (+ LBA1 for GPT) and classifies.
 *   device: whole-disk view (must cover at least the first 34 sectors for GPT).
 * Returns via out-param; layout GptDamaged when GPT is present but CRC checks
 * fail on BOTH copies - partitions still extracted with per-entry best effort
 * (salvage-over-purity, fix F7/F10).
 */
u32 parse_disk_table(IStorageDevice* device, DiskTable& out);

/* EBR chain walk (called by parse_disk_table for extended containers).
 * depth_limit bounds the chain (typical disks: < 128; guard = fix F7).
 * break_lba (optional out): the LBA of the first unreadable/invalid
 * link — the M-002 honesty carrier for DiskTable::ebr_break_lba. */
constexpr u32 EBR_DEPTH_LIMIT = 1024;
u32 parse_ebr_chain(IStorageDevice* device, u64 first_ebr_lba, std::vector<PartitionInfo>& out,
                    u64* break_lba = nullptr);

/* Map an MBR type byte to FsKind (best effort, public id conventions). */
FsKind mbr_type_to_kind(u8 type);

} // namespace table
} // namespace cr

#endif // CR_TABLE_PARSE_H_
