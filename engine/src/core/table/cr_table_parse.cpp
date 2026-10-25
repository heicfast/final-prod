/* cr_table_parse.cpp - MBR / EBR / GPT parsing with validation (R1.3, R1.4).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/table/cr_table_parse.h"
#include <cstring>

namespace cr {
namespace table {

namespace {
/* GPT read budget: entries region typically 32 sectors (128 x 128B). We read
 * entry_count * entry_size rounded up to sectors, capped at 1 MiB. */
constexpr u32 GPT_ENTRIES_READ_CAP = 1u << 20;

bool read_sectors(IStorageDevice* dev, u64 lba, u32 count, std::vector<u8>& buf) {
    const Geometry& g = dev->geometry();
    if (g.logical_sector == 0)
        return false;
    u64 off = lba * g.logical_sector;
    u64 len = u64(count) * g.logical_sector;
    if (off + len > dev->size())
        return false;
    buf.resize(len);
    return dev->read_at(off, buf.data(), u32(len)) == OK;
}

u32 load_gpt_side(IStorageDevice* dev, u64 header_lba, GptHeader& header_out,
                  std::vector<GptEntry>& entries_out) {
    std::vector<u8> sec;
    if (!read_sectors(dev, header_lba, 1, sec))
        return ERR_READ_FAILED;
    GptHeader h;
    std::memcpy(&h, sec.data(), sizeof(h));
    if (std::memcmp(h.signature, GPT_SIGNATURE, 8) != 0)
        return ERR_BAD_MAGIC;
    if (h.entry_size != sizeof(GptEntry))
        return ERR_BAD_PARAMETER;
    if (h.entry_count == 0 || u64(h.entry_count) * h.entry_size > GPT_ENTRIES_READ_CAP)
        return ERR_BAD_PARAMETER;
    if (h.header_size < 92 || h.header_size > sec.size())
        return ERR_BAD_PARAMETER; /* P0-2: CRC length must stay inside the sector */

    /* header CRC: compute over header_size bytes with crc field zeroed */
    {
        std::vector<u8> tmp(sec);
        std::memset(tmp.data() + offsetof(GptHeader, header_crc32), 0, 4);
        u32 computed = crc32(tmp.data(), h.header_size);
        if (computed != h.header_crc32)
            return ERR_BAD_CHECKSUM;
    }

    /* entries CRC over the declared array */
    {
        u64 bytes = u64(h.entry_count) * h.entry_size;
        std::vector<u8> ebuf;
        u32 sectors =
            u32((bytes + dev->geometry().logical_sector - 1) / dev->geometry().logical_sector);
        if (!read_sectors(dev, h.entries_start_lba, sectors, ebuf))
            return ERR_READ_FAILED;
        ebuf.resize(bytes);
        if (crc32(ebuf.data(), bytes) != h.entries_crc32)
            return ERR_BAD_CHECKSUM;
        entries_out.resize(h.entry_count);
        std::memcpy(entries_out.data(), ebuf.data(), bytes);
    }
    header_out = h;
    return OK;
}

/* og two-step GPT salvage, step 1 (FUN_180006e20:4517-4560,
 * Partition_decompiled.c): the primary header failed verification, but
 * its OWN entry-array fields may still be sane - re-read the raw header,
 * gate on signature + entry geometry, read the array at the header's
 * entries_start_lba and require the entries CRC (hdr+0x58) to match. On
 * success the volume keeps its partitions even though the header itself
 * stays untrusted (gpt_primary_ok remains false). Stricter than the og
 * on one point: we require the declared entry_size == 128 (the og
 * hardcodes the 0x80 stride, which mis-sizes the array for corrupt
 * non-128 headers). */
u32 salvage_primary_entries(IStorageDevice* dev, GptHeader& raw_out,
                            std::vector<GptEntry>& entries_out) {
    std::vector<u8> sec;
    if (!read_sectors(dev, 1, 1, sec))
        return ERR_READ_FAILED;
    GptHeader h;
    std::memcpy(&h, sec.data(), sizeof(h));
    if (std::memcmp(h.signature, GPT_SIGNATURE, 8) != 0)
        return ERR_BAD_MAGIC;
    if (h.entry_size != sizeof(GptEntry))
        return ERR_BAD_PARAMETER;
    if (h.entry_count == 0 || u64(h.entry_count) * h.entry_size > GPT_ENTRIES_READ_CAP)
        return ERR_BAD_PARAMETER;
    const u64 bytes = u64(h.entry_count) * h.entry_size;
    const u64 ls = dev->geometry().logical_sector;
    const u32 sectors = u32((bytes + ls - 1) / ls);
    const u64 total_secs =
        dev->geometry().sector_count ? dev->geometry().sector_count : dev->size() / ls;
    if (h.entries_start_lba == 0 || h.entries_start_lba + sectors > total_secs)
        return ERR_BAD_PARAMETER; /* hostile header: array outside the disk */
    std::vector<u8> ebuf;
    if (!read_sectors(dev, h.entries_start_lba, sectors, ebuf))
        return ERR_READ_FAILED;
    ebuf.resize(bytes);
    if (crc32(ebuf.data(), bytes) != h.entries_crc32)
        return ERR_BAD_CHECKSUM;
    entries_out.resize(h.entry_count);
    std::memcpy(entries_out.data(), ebuf.data(), bytes);
    raw_out = h;
    return OK;
}

} // namespace

FsKind mbr_type_to_kind(u8 type) {
    switch (type) {
    case 0x05:
    case 0x0F:
        return FsKind::Extended;
    case 0x07:
        /* HPFS/NTFS/exFAT share 0x07 - disambiguated later by boot-sector
         * probe (matches reference PartitionRecovery analyser chain). */
        return FsKind::Ntfs; /* provisional; the engine boot validators refine */
    case 0x0B:
    case 0x0C:
        return FsKind::Fat32;
    case 0x0E:
        return FsKind::Fat16;
    case 0x01:
    case 0x04:
    case 0x06:
        return FsKind::Fat16;
    case 0xEE:
        return FsKind::Protective;
    case 0x42:
        return FsKind::Ldm;
    case 0x83:
        return FsKind::Linux;
    case 0x82:
        return FsKind::Other; /* swap */
    case 0x27:
        return FsKind::Reserved; /* MSR/recovery */
    default:
        return FsKind::Other;
    }
}

u32 parse_ebr_chain(IStorageDevice* device, u64 first_ebr_lba, std::vector<PartitionInfo>& out,
                    u64* break_lba) {
    const Geometry& g = device->geometry();
    if (g.logical_sector == 0)
        return ERR_BAD_PARAMETER;
    std::vector<u8> sec;
    u64 base = first_ebr_lba; /* extended container base */
    u64 walk = first_ebr_lba;
    u32 depth = 0;
    std::vector<u64> visited;
    while (depth < EBR_DEPTH_LIMIT) {
        ++depth;
        /* cycle guard (fix F7) */
        for (u64 v : visited)
            if (v == walk)
                return ERR_CYCLE_LIMIT;
        visited.push_back(walk);
        if (!read_sectors(device, walk, 1, sec)) {
            if (break_lba)
                *break_lba = walk; /* M-002: the honest break point */
            return ERR_READ_FAILED;
        }
        if (!mbr_signature_ok(sec.data())) {
            if (break_lba)
                *break_lba = walk; /* M-002: the honest break point */
            return ERR_BAD_MAGIC;
        }
        MbrSector mbr;
        std::memcpy(&mbr, sec.data(), sizeof(mbr));
        /* entry 0: this segment's volume; entry 1: next EBR (absolute base +
         * offset). Some layouts use entry 0 for next - handle both shapes. */
        u64 next_ebr = 0;
        bool have_next = false;
        bool have_vol = false;
        for (int i = 0; i < 4; ++i) {
            const MbrPartitionEntry& e = mbr.entries[i];
            if (!mbr_entry_used(e))
                continue;
            FsKind kind = mbr_type_to_kind(e.type);
            if (kind == FsKind::Extended) {
                if (i == 0 && !have_vol) {
                    /* unusual: first entry links next EBR (base + rel) */
                    if (e.lba_start != 0) {
                        next_ebr = base + e.lba_start;
                        have_next = true;
                    }
                } else if (!have_next) {
                    if (e.lba_start != 0) {
                        next_ebr = base + e.lba_start;
                        have_next = true;
                    }
                }
                continue;
            }
            if (i == 0 || !have_vol) {
                PartitionInfo p;
                p.start_lba = walk + e.lba_start; /* relative to THIS EBR */
                p.sector_count = e.sector_count;
                p.fs = kind;
                p.mbr_type = e.type;
                p.used = true;
                if (p.sector_count != 0)
                    out.push_back(p);
                have_vol = true;
            }
        }
        if (!have_next)
            break;
        if (next_ebr < base)
            break; /* malformed: must not point before base */
        walk = next_ebr;
    }
    return depth >= EBR_DEPTH_LIMIT ? ERR_CYCLE_LIMIT : OK;
}

u32 parse_disk_table(IStorageDevice* device, DiskTable& out) {
    const Geometry& g = device->geometry();
    if (g.logical_sector == 0 || device->size() < u64(2) * g.logical_sector)
        return ERR_BAD_PARAMETER;

    std::vector<u8> sec;
    if (!read_sectors(device, 0, 1, sec))
        return ERR_READ_FAILED;
    if (!mbr_signature_ok(sec.data())) {
        out.layout = TableLayout::None;
        return OK; /* blank/damaged: no table - not an error */
    }
    MbrSector mbr;
    std::memcpy(&mbr, sec.data(), sizeof(mbr));

    /* GPT? LBA1 signature, OR a protective MBR whose backup header may
     * still be intact (og IsGPTDisk detection: 55AA + 0xEE @+0x1C2, then
     * the last-LBA probe when LBA1 is dead - CGPTDiskAnalyzer
     * FUN_180006e20:4208-4218). The protective-only shape is what a
     * lost-partition GPT volume looks like on disk. */
    bool has_gpt = false;
    bool lba1_gpt = false;
    {
        std::vector<u8> lba1;
        if (device->size() >= u64(2) * g.logical_sector && read_sectors(device, 1, 1, lba1) &&
            std::memcmp(lba1.data(), GPT_SIGNATURE, 8) == 0) {
            has_gpt = true;
            lba1_gpt = true;
        }
    }
    if (!has_gpt) {
        for (int i = 0; i < 4; ++i)
            if (mbr_entry_used(mbr.entries[i]) && mbr.entries[i].type == 0xEE) {
                has_gpt = true; /* protective MBR: GPT path, backup probe */
                break;
            }
    }

    if (has_gpt) {
        out.layout = TableLayout::Gpt;
        /* shared per-entry classification (verified primary, salvaged
         * primary and backup all emit through this - the og reallocs and
         * re-parses identically for the backup, Partition_decompiled.c
         * :4580-4634) */
        auto emit_gpt_entries = [&](const std::vector<GptEntry>& ents) {
            for (const auto& e : ents) {
                if (!gpt_entry_used(e))
                    continue;
                PartitionInfo p;
                p.start_lba = e.first_lba;
                p.sector_count = e.last_lba - e.first_lba + 1;
                p.gpt_attrs = e.attributes;
                if (gpt_guid_equal(e.type_guid, GPT_TYPE_BASIC_DATA) ||
                    gpt_guid_equal(e.type_guid, GPT_TYPE_LINUX_FS))
                    p.fs = gpt_guid_equal(e.type_guid, GPT_TYPE_LINUX_FS) ? FsKind::Linux
                                                                          : FsKind::Ntfs;
                else if (gpt_guid_equal(e.type_guid, GPT_TYPE_LDM_DATA))
                    p.fs = FsKind::Ldm;
                else if (gpt_guid_equal(e.type_guid, GPT_TYPE_EFI_SYSTEM))
                    p.fs = FsKind::EfiSystem; /* A-001: ESP is not MSR/Reserved -
                                               * the og lists ESP as a scannable
                                               * volume; our scan probes content,
                                               * the kind feeds reporting only */
                else if (gpt_guid_equal(e.type_guid, GPT_TYPE_BIOS_BOOT) ||
                         gpt_guid_equal(e.type_guid, GPT_TYPE_MICROSOFT_RESERVED) ||
                         gpt_guid_equal(e.type_guid, GPT_TYPE_LDM_METADATA))
                    p.fs = FsKind::Reserved; /* MSR / BIOS boot / LDM metadata */
                else
                    p.fs = FsKind::Other;
                for (int i = 0; i < 36; ++i)
                    p.name[i] = e.name[i];
                p.from_gpt = true;
                p.used = true;
                if (p.sector_count != 0)
                    out.partitions.push_back(p);
            }
        };
        auto copy_header_facts = [&](const GptHeader& h) {
            std::memcpy(out.disk_guid, h.disk_guid, 16);
            out.first_usable_lba = h.first_usable_lba;
            out.last_usable_lba = h.last_usable_lba;
            out.gpt_entry_count = h.entry_count;
        };
        const u64 dev_last =
            (g.sector_count ? g.sector_count : device->size() / g.logical_sector) - 1;

        GptHeader prim;
        std::vector<GptEntry> pent;
        const u32 rc_p = lba1_gpt ? load_gpt_side(device, 1, prim, pent) : ERR_BAD_MAGIC;
        GptHeader bak;
        std::vector<GptEntry> bent;
        bool emitted = false;
        if (rc_p == OK) {
            out.gpt_primary_ok = true;
            copy_header_facts(prim);
            emit_gpt_entries(pent);
            emitted = true;
            /* informational backup probe at the TRUSTED primary's own
             * backup LBA (healthy-disk case: both flags true) */
            if (load_gpt_side(device, prim.backup_lba, bak, bent) == OK)
                out.gpt_backup_ok = true;
        } else if (lba1_gpt) {
            /* og two-step step 1: header dead, entry array maybe intact -
             * salvage BEFORE touching the backup (FUN_180006e20:4530-4560
             * sets try_backup=0 exactly when this succeeds) */
            GptHeader raw;
            std::vector<GptEntry> sent;
            if (salvage_primary_entries(device, raw, sent) == OK) {
                /* header stays untrusted (gpt_primary_ok false); the
                 * volume keeps its partitions + the entry count fact */
                out.gpt_entry_count = raw.entry_count;
                emit_gpt_entries(sent);
                emitted = true;
            }
        }
        if (!emitted) {
            /* primary dead AND salvage failed (or protective-only entry):
             * og step 2 - the backup header at the DEVICE-derived last LBA
             * (a damaged primary's backup_lba field cannot be trusted) */
            if (load_gpt_side(device, dev_last, bak, bent) == OK) {
                out.gpt_backup_ok = true;
                copy_header_facts(bak);
                emit_gpt_entries(bent);
                emitted = true;
            }
        } else if (!out.gpt_backup_ok && lba1_gpt && rc_p != OK) {
            /* entries salvaged from a dead primary: still probe the
             * backup for the informational flag (the og skips this; our
             * gpt_backup_ok consumers expect the flag regardless) */
            if (load_gpt_side(device, dev_last, bak, bent) == OK)
                out.gpt_backup_ok = true;
        }
        if (!out.gpt_primary_ok && !out.gpt_backup_ok && !emitted)
            out.layout = TableLayout::GptDamaged;
        return OK;
    }

    /* plain MBR (possibly with EBR chain) */
    out.layout = TableLayout::Mbr;
    u64 ebr_base = 0;
    bool has_ebr = false;
    for (int i = 0; i < 4; ++i) {
        const MbrPartitionEntry& e = mbr.entries[i];
        if (!mbr_entry_used(e))
            continue;
        FsKind kind = mbr_type_to_kind(e.type);
        if (kind == FsKind::Extended) {
            ebr_base = e.lba_start;
            has_ebr = true;
            continue;
        }
        if (kind == FsKind::Protective) {
            /* protective MBR: GPT damaged form (primary/backup verified
             * separately; the pre-GptDamaged Gpt store was dead) */
            out.gpt_primary_ok = out.gpt_backup_ok = false;
            out.layout = TableLayout::GptDamaged;
            continue;
        }
        PartitionInfo p;
        p.start_lba = e.lba_start;
        p.sector_count = e.sector_count;
        p.fs = kind;
        p.mbr_type = e.type;
        p.used = true;
        if (p.sector_count != 0)
            out.partitions.push_back(p);
    }
    if (has_ebr && ebr_base != 0) {
        std::vector<PartitionInfo> ext;
        u64 break_lba = 0;
        u32 rc = parse_ebr_chain(device, ebr_base, ext, &break_lba);
        /* M-002 (diff-to-fix): salvage-over-purity — the volumes already
         * walked are NEVER discarded on a torn link. The og's lost-volume
         * discovery does not gate on EBR-chain integrity (Partition.mo's
         * chain parse feeds the same candidate pool the deep-scan block
         * parsers fill independently — corpus mo-module-map); a broken
         * link degrades to the walked prefix, it does not erase it. The
         * chain shape is recorded for honest reporting ("N volumes
         * found; chain broken at LBA X"). */
        for (auto& p : ext)
            out.partitions.push_back(p);
        if (rc == OK || rc == ERR_CYCLE_LIMIT) {
            out.ebr_status = EbrChainStatus::Ok;
        } else if (!ext.empty()) {
            out.ebr_status = EbrChainStatus::Partial;
            out.ebr_break_lba = break_lba;
        } else {
            out.ebr_status = EbrChainStatus::Failed;
            out.ebr_break_lba = break_lba;
        }
    }
    return OK;
}

} // namespace table
} // namespace cr
