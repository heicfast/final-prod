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

} // namespace

FsKind mbr_type_to_kind(u8 type) {
    switch (type) {
    case 0x05:
    case 0x0F:
        return FsKind::Extended;
    case 0x07:
        /* HPFS/NTFS/exFAT share 0x07 - disambiguated later by boot-sector
         * probe (matches reference PartitionRecovery analyser chain). */
        return FsKind::Ntfs; /* provisional; refine_ntfs_vs_exfat in engines */
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

u32 parse_ebr_chain(IStorageDevice* device, u64 first_ebr_lba, std::vector<PartitionInfo>& out) {
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
        if (!read_sectors(device, walk, 1, sec))
            return ERR_READ_FAILED;
        if (!mbr_signature_ok(sec.data()))
            return ERR_BAD_MAGIC;
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

    /* GPT? LBA1 signature */
    bool has_gpt = false;
    {
        std::vector<u8> lba1;
        if (device->size() >= u64(2) * g.logical_sector && read_sectors(device, 1, 1, lba1) &&
            std::memcmp(lba1.data(), GPT_SIGNATURE, 8) == 0)
            has_gpt = true;
    }

    if (has_gpt) {
        out.layout = TableLayout::Gpt;
        GptHeader prim;
        std::vector<GptEntry> pent;
        u32 rc_p = load_gpt_side(device, 1, prim, pent);
        GptHeader bak;
        std::vector<GptEntry> bent;
        u64 last_lba = 0;
        if (rc_p == OK) {
            out.gpt_primary_ok = true;
            std::memcpy(out.disk_guid, prim.disk_guid, 16);
            out.first_usable_lba = prim.first_usable_lba;
            out.last_usable_lba = prim.last_usable_lba;
            out.gpt_entry_count = prim.entry_count;
            last_lba = prim.backup_lba;
            for (const auto& e : pent) {
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
                else if (gpt_guid_equal(e.type_guid, GPT_TYPE_BIOS_BOOT) ||
                         gpt_guid_equal(e.type_guid, GPT_TYPE_MICROSOFT_RESERVED))
                    p.fs = FsKind::Reserved;
                else
                    p.fs = FsKind::Other;
                for (int i = 0; i < 36; ++i)
                    p.name[i] = e.name[i];
                p.from_gpt = true;
                p.used = true;
                if (p.sector_count != 0)
                    out.partitions.push_back(p);
            }
        }
        if (last_lba != 0) {
            u32 rc_b = load_gpt_side(device, last_lba, bak, bent);
            if (rc_b == OK) {
                out.gpt_backup_ok = true;
                if (!out.gpt_primary_ok) {
                    /* salvage from the backup copy (fix F10 spirit) */
                    std::memcpy(out.disk_guid, bak.disk_guid, 16);
                    out.first_usable_lba = bak.first_usable_lba;
                    out.last_usable_lba = bak.last_usable_lba;
                    out.gpt_entry_count = bak.entry_count;
                    for (const auto& e : bent) {
                        if (!gpt_entry_used(e))
                            continue;
                        PartitionInfo p;
                        p.start_lba = e.first_lba;
                        p.sector_count = e.last_lba - e.first_lba + 1;
                        p.from_gpt = true;
                        p.used = true;
                        p.fs = FsKind::Other;
                        for (int i = 0; i < 36; ++i)
                            p.name[i] = e.name[i];
                        if (p.sector_count != 0)
                            out.partitions.push_back(p);
                    }
                }
            }
        }
        if (!out.gpt_primary_ok && !out.gpt_backup_ok)
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
            out.layout = TableLayout::Gpt; /* protective MBR: GPT damaged form */
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
        u32 rc = parse_ebr_chain(device, ebr_base, ext);
        if (rc == OK || rc == ERR_CYCLE_LIMIT) {
            for (auto& p : ext)
                out.partitions.push_back(p);
        }
    }
    return OK;
}

} // namespace table
} // namespace cr
