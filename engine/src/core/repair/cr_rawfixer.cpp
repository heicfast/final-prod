/* cr_rawfixer.cpp - transactional filesystem repair (corpus RawFixer.dll).
 *
 * Family fixers reuse the scan engines' wire structs and checksums:
 * exFAT geometry comes from exfat::BootSectorMain (spec-exact offsets),
 * the exFAT boot checksum from exfat::boot_region_checksum, FAT boot
 * validation from fat::validate_boot, and NTFS DBR checks mirror
 * ntfs::validate_boot_sector's gates. The fixer never accepts a boot
 * sector the scanner would reject.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/repair/cr_rawfixer.h"
#include "cr/cr_error.h"
#include "cr/exfat/cr_exfat_engine.h"
#include "cr/exfat/cr_exfat_wire.h"
#include "cr/fat/cr_fat_engine.h"
#include "cr/table/cr_table_wire.h"
#include <cstring>

namespace cr {
namespace repair {

namespace {

u16 le16(const u8* p) {
    return u16(p[0]) | (u16(p[1]) << 8);
}
u32 le32(const u8* p) {
    return u32(p[0]) | (u32(p[1]) << 8) | (u32(p[2]) << 16) | (u32(p[3]) << 24);
}
u64 le64(const u8* p) {
    return u64(le32(p)) | (u64(le32(p + 4)) << 32);
}

/* Validate one FAT boot sector shape (BPB sanity; corpus FATFixer::IsValidDBR).
 * The scanner's fat::validate_boot needs a device; for candidate boot
 * sectors held in memory this mirror checks the same first-line gates. */
bool fat_boot_sane(const u8* b, u64 volume_bytes) {
    if (le16(b + 0x0B) == 0 || (le16(b + 0x0B) & (le16(b + 0x0B) - 1)) != 0)
        return false;
    const u16 bps = le16(b + 0x0B);
    if (bps < 512 || bps > 4096)
        return false;
    const u8 spc = b[0x0D];
    if (spc == 0 || (spc & (spc - 1)) != 0)
        return false;
    if (b[510] != 0x55 || b[511] != 0xAA)
        return false;
    /* FAT12/16: root entry count nonzero; FAT32: root cluster nonzero */
    if (le16(b + 0x11) == 0 && le32(b + 0x2C) == 0)
        return false;
    const u64 total_sec = le16(b + 0x13) != 0 ? u64(le16(b + 0x13)) : le32(b + 0x20);
    if (total_sec == 0)
        return false;
    if (volume_bytes != 0 && total_sec * bps > volume_bytes * 2)
        return false; /* wildly over-sized */
    return true;
}

/* Validate an NTFS boot sector (corpus NTFSFixer::CheckDBR family; mirrors
 * the gates of ntfs::validate_boot_sector for an in-memory candidate). */
bool ntfs_boot_sane(const u8* b, u64 volume_bytes) {
    if (std::memcmp(b + 3, "NTFS    ", 8) != 0)
        return false;
    const u16 bps = le16(b + 0x0B);
    if (bps == 0 || (bps & (bps - 1)) != 0 || bps < 512 || bps > 4096)
        return false;
    const u8 spc = b[0x0D];
    if (spc == 0 || (spc & (spc - 1)) != 0)
        return false;
    if (le64(b + 0x28) == 0)
        return false; /* total sectors */
    if (le64(b + 0x30) == 0)
        return false; /* $MFT LCN */
    /* clusters-per-record: positive count or negative log2 encoding
     * (corpus NTFSFixer::CheckMFT record-size math) */
    const i8 cpr = i8(b[0x40]);
    if (cpr > 0 && (cpr & (cpr - 1)) != 0)
        return false;
    if (volume_bytes != 0 && le64(b + 0x28) * u64(bps) > volume_bytes * 2)
        return false; /* wildly over-sized */
    return true;
}

/* exFAT boot sector sanity for an in-memory candidate (mirrors the gates
 * of exfat::validate_boot_region's main-boot checks). */
bool exfat_boot_sane(const u8* b, u64 volume_bytes, u32 sector_bytes) {
    const auto* m = reinterpret_cast<const exfat::BootSectorMain*>(b);
    if (std::memcmp(m->fs_name, "EXFAT   ", 8) != 0)
        return false;
    if (m->signature[0] != 0x55 || m->signature[1] != 0xAA)
        return false;
    if (m->bytes_per_sector_shift < 9 || m->bytes_per_sector_shift > 12)
        return false;
    if (sector_bytes != 0 && (1u << m->bytes_per_sector_shift) != sector_bytes)
        return false;
    if (m->sectors_per_cluster_shift > 25 - m->bytes_per_sector_shift)
        return false;
    if (m->volume_length < 2048)
        return false;
    if (m->cluster_count == 0 || m->cluster_count > 0xFFFFFFF5)
        return false;
    if (volume_bytes != 0 &&
        u64(m->volume_length) * (1ull << m->bytes_per_sector_shift) > volume_bytes * 2)
        return false;
    return true;
}

void push32(std::vector<u8>& out, u32 v) {
    out.push_back(u8(v));
    out.push_back(u8(v >> 8));
    out.push_back(u8(v >> 16));
    out.push_back(u8(v >> 24));
}
void push64(std::vector<u8>& out, u64 v) {
    push32(out, u32(v));
    push32(out, u32(v >> 32));
}

} // namespace

/* ---- UndoLog container ---- */

void UndoLog::serialize(std::vector<u8>& out) const {
    out.clear();
    const char magic[8] = {'C', 'R', 'U', 'N', 'D', 'O', '0', '1'};
    out.insert(out.end(), magic, magic + 8);
    push32(out, 1); /* version */
    push32(out, u32(entries.size()));
    for (const auto& e : entries) {
        push64(out, e.offset);
        push32(out, e.length);
        out.insert(out.end(), e.original.begin(), e.original.end());
    }
    /* CRC over everything after the magic */
    const u32 crc = table::crc32(out.data() + 8, u32(out.size() - 8));
    push32(out, crc);
}

u32 UndoLog::deserialize(const u8* data, size_t len, UndoLog& out) {
    if (len < 20 || std::memcmp(data, "CRUNDO01", 8) != 0)
        return ERR_BAD_MAGIC;
    const u32 crc = le32(data + len - 4);
    if (table::crc32(data + 8, u32(len - 8 - 4)) != crc)
        return ERR_BAD_CHECKSUM;
    const u32 version = le32(data + 8);
    if (version != 1)
        return ERR_UNSUPPORTED;
    const u32 count = le32(data + 12);
    size_t pos = 16; /* after magic(8) + version(4) + count(4) */
    out.entries.clear();
    for (u32 i = 0; i < count; ++i) {
        if (pos + 12 > len - 4)
            return ERR_TRUNCATED;
        Entry e;
        e.offset = u64(le32(data + pos)) | (u64(le32(data + pos + 4)) << 32);
        e.length = le32(data + pos + 8);
        pos += 12;
        if (e.length > len - 4 - pos)
            return ERR_TRUNCATED;
        e.original.assign(data + pos, data + pos + e.length);
        pos += e.length;
        out.entries.push_back(std::move(e));
    }
    return OK;
}

/* ---- Phase 1: analyze ---- */

u32 analyze(IStorageDevice* slice, RepairPlan& plan) {
    plan.actions.clear();
    plan.refused = false;
    plan.refusal_reason.clear();
    plan.detected = false;
    plan.fs_family.clear();
    if (slice == nullptr)
        return ERR_BAD_PARAMETER;

    const u64 total = slice->size();
    if (total < 24 * 512)
        return ERR_BAD_PARAMETER;
    const u32 sector = 512;

    u8 primary[512];
    if (slice->read_at(0, primary, sector) != OK)
        return ERR_READ_FAILED;

    /* ---- family detection (corpus CheckPartition) ----
     * A destroyed primary OEM still identifies the family through the
     * backup boot sector (exFAT sector 12 / FAT sector 6 / NTFS volume
     * end) - those probes run inside the family branches below. */
    const bool is_ntfs = std::memcmp(primary + 3, "NTFS    ", 8) == 0;
    const bool is_exfat = std::memcmp(primary + 3, "EXFAT   ", 8) == 0;
    const bool fat_sig = primary[510] == 0x55 && primary[511] == 0xAA;
    const bool is_fat = !is_ntfs && !is_exfat && fat_sig && fat_boot_sane(primary, total);
    /* exFAT with a destroyed primary: the backup boot sector at 12 keeps
     * the signature (corpus FixFromDBR primary/backup pair). */
    bool exfat_via_backup = false;
    if (!is_ntfs && !is_exfat && !is_fat && total >= 24 * 512) {
        u8 bak[512];
        if (slice->read_at(12 * 512, bak, 512) == OK && exfat_boot_sane(bak, total, 512))
            exfat_via_backup = true;
    }

    /* ---- NTFS: primary boot damaged -> volume-end backup (FixFromDBR) ---- */
    if (is_ntfs) {
        plan.detected = true;
        plan.fs_family = "ntfs";
        if (ntfs_boot_sane(primary, total))
            return OK; /* S2: clean volume, nothing to do */
        u8 backup[512];
        if (slice->read_at(total - sector, backup, sector) == OK && ntfs_boot_sane(backup, total)) {
            RepairAction a;
            a.kind = ActionKind::RestoreBootFromBackup;
            a.offset = 0;
            a.length = sector;
            a.after.assign(backup, backup + sector);
            a.reason = "primary NTFS boot sector damaged; restore from volume-end backup";
            plan.actions.push_back(std::move(a));
            return OK;
        }
        plan.refused = true;
        plan.refusal_reason = "NTFS primary damaged and no valid backup boot sector (S3)";
        return OK;
    }

    /* ---- exFAT: checksum repair + geometry rederivation + backup restore ---- */
    if (is_exfat || exfat_via_backup) {
        plan.detected = true;
        plan.fs_family = "exfat";
        if (exfat_via_backup) {
            /* the primary is too damaged to trust any field in it:
             * restore the boot sector from the backup copy, then plan the
             * checksum recompute (the restored sector changes the sum). */
            u8 backup[512];
            if (slice->read_at(12 * 512, backup, 512) != OK)
                return ERR_READ_FAILED;
            RepairAction a;
            a.kind = ActionKind::RestoreBootFromBackup;
            a.offset = 0;
            a.length = 512;
            a.after.assign(backup, backup + 512);
            a.reason = "primary exFAT boot sector destroyed; restore from backup sector 12";
            plan.actions.push_back(std::move(a));
            /* recompute the checksum over the restored primary */
            std::vector<u8> region(12 * 512, 0);
            if (slice->read_at(0, region.data(), u32(region.size())) != OK)
                return ERR_READ_FAILED;
            std::memcpy(region.data(), backup, 512);
            const u32 new_sum = exfat::boot_region_checksum(region.data(), 512);
            RepairAction c;
            c.kind = ActionKind::RepairBootChecksum;
            c.offset = 11 * 512;
            c.length = 512;
            std::vector<u8> fixed(512, 0);
            for (u32 i = 0; i < 512; i += 4) {
                fixed[i] = u8(new_sum);
                fixed[i + 1] = u8(new_sum >> 8);
                fixed[i + 2] = u8(new_sum >> 16);
                fixed[i + 3] = u8(new_sum >> 24);
            }
            c.after = fixed;
            c.reason = "recompute boot-region checksum after primary restore";
            plan.actions.push_back(std::move(c));
            return OK;
        }
        const auto* m = reinterpret_cast<const exfat::BootSectorMain*>(primary);
        const u32 bps_shift = m->bytes_per_sector_shift >= 9 && m->bytes_per_sector_shift <= 12
                                  ? m->bytes_per_sector_shift
                                  : 9;
        const u32 bps = 1u << bps_shift;
        const u32 spc_shift = m->sectors_per_cluster_shift;
        const u64 fat_off = m->fat_offset;           /* sectors, +0x50 */
        const u32 fat_len = m->fat_length;           /* sectors, +0x54 */
        const u64 heap_off = m->cluster_heap_offset; /* sectors, +0x58 */
        const u32 cluster_count = m->cluster_count;  /* +0x5C */
        const u64 vol_len = m->volume_length;        /* sectors, +0x48 */

        const bool boot_shape_ok = exfat_boot_sane(primary, total, bps);

        /* Geometry invariants (corpus TryFixFromSectorsPerCluster): the
         * heap must start at/after fat_offset + fat_length, and the
         * declared cluster count must fit the heap within the volume. */
        bool geometry_ok = boot_shape_ok;
        if (geometry_ok) {
            if (heap_off < fat_off + fat_len)
                geometry_ok = false;
            if (cluster_count != 0 &&
                heap_off + u64(cluster_count) * (1ull << spc_shift) > vol_len + (1ull << spc_shift))
                geometry_ok = false; /* clusters extend past the volume */
        }

        /* Boot-region checksum (sectors 0..10, fill-repeated @11) */
        std::vector<u8> region(12 * bps);
        if (slice->read_at(0, region.data(), u32(region.size())) != OK)
            return ERR_READ_FAILED;
        const u32 computed = exfat::boot_region_checksum(region.data(), bps);
        const u32 stored = le32(region.data() + 11 * bps);
        bool checksum_ok = stored == computed;
        if (checksum_ok) {
            /* the fill must be consistent across the whole sector 11 */
            for (u32 k = 1; k < bps / 4; ++k)
                if (le32(region.data() + 11 * bps + k * 4) != stored)
                    checksum_ok = false;
        }

        if (checksum_ok && geometry_ok)
            return OK; /* S2: clean */

        /* Primary boot shape damaged beyond the OEM? Restore the boot
         * sector from the backup copy at sector 12 (corpus FixFromDBR +
         * ChecksumDBR over primary 0 / backup 12). */
        if (!boot_shape_ok) {
            u8 backup[512];
            if (slice->read_at(12 * bps, backup, 512) == OK &&
                exfat_boot_sane(backup, total, bps)) {
                RepairAction a;
                a.kind = ActionKind::RestoreBootFromBackup;
                a.offset = 0;
                a.length = 512;
                a.after.assign(backup, backup + 512);
                a.reason = "primary exFAT boot sector damaged; restore from backup sector 12";
                plan.actions.push_back(std::move(a));
                /* after a boot-sector restore the checksum must be
                 * recomputed too (the restored sector changes the sum) */
                checksum_ok = false;
            } else {
                plan.refused = true;
                plan.refusal_reason = "exFAT primary damaged and no valid backup boot sector (S3)";
                return OK;
            }
        }

        if (!checksum_ok) {
            RepairAction a;
            a.kind = ActionKind::RepairBootChecksum;
            a.offset = 11 * u64(bps);
            a.length = bps;
            /* recompute over the POST-restore boot sector when a restore is
             * planned: the action list is applied in order, so mirror the
             * restored primary into the region before checksumming */
            std::vector<u8> after_region(region.begin(), region.end());
            for (const auto& act : plan.actions)
                if (act.kind == ActionKind::RestoreBootFromBackup && act.offset == 0)
                    std::memcpy(after_region.data(), act.after.data(), 512);
            const u32 new_sum = exfat::boot_region_checksum(after_region.data(), bps);
            std::vector<u8> fixed(bps, 0);
            for (u32 i = 0; i < bps; i += 4) {
                fixed[i] = u8(new_sum);
                fixed[i + 1] = u8(new_sum >> 8);
                fixed[i + 2] = u8(new_sum >> 16);
                fixed[i + 3] = u8(new_sum >> 24);
            }
            a.after = fixed;
            a.reason = "exFAT boot-region checksum mismatch; recompute over sectors 0..10";
            plan.actions.push_back(std::move(a));
        }

        if (!geometry_ok) {
            /* Rederive sectors-per-cluster-shift from cluster-count
             * invariants (corpus TryFixFromSectorsPerCluster): exactly one
             * shift must make the declared cluster count fit the heap. */
            const u64 heap_bytes = total > heap_off * bps ? total - heap_off * bps : 0;
            i32 found_shift = -1;
            u32 candidates = 0;
            for (u32 try_shift = 0; try_shift <= 25 - bps_shift; ++try_shift) {
                const u64 cluster_bytes = 1ull << try_shift;
                if (heap_bytes == 0 || cluster_bytes == 0)
                    continue;
                const u64 fit = heap_bytes / cluster_bytes;
                if (fit >= 2 && fit <= 0xFFFFFFF5 && cluster_count != 0 &&
                    (fit == cluster_count || fit == cluster_count + 1)) {
                    ++candidates;
                    if (found_shift == -1)
                        found_shift = i32(try_shift);
                }
            }
            if (candidates == 1 && found_shift >= 0) {
                RepairAction a;
                a.kind = ActionKind::RepairGeometryField;
                a.offset = 0x6D; /* SectorsPerClusterShift */
                a.length = 1;
                a.after = {u8(found_shift)};
                a.reason = "sectors-per-cluster shift inconsistent with cluster count; rederived";
                plan.actions.push_back(std::move(a));
            } else {
                plan.refused = true;
                plan.refusal_reason = "ambiguous or no sectors-per-cluster candidates (S3)";
                return OK;
            }
        }
        return OK;
    }

    /* ---- FAT family ---- */
    if (is_fat) {
        plan.detected = true;
        plan.fs_family = "fat";
        return OK; /* S2: clean primary */
    }

    /* FAT primary damaged (or FAT32 with corrupt DBR): probe the backup
     * boot sector at sector 6 (FAT32 only per spec; FAT12/16 have none,
     * but a sane backup there is evidence enough - the corpus FATFixer
     * checks CheckBackupDBR the same way). */
    {
        u8 backup[512];
        if (total >= 7 * sector && slice->read_at(6 * sector, backup, sector) == OK) {
            if (fat_boot_sane(backup, total)) {
                plan.detected = true;
                plan.fs_family = "fat";
                RepairAction a;
                a.kind = ActionKind::RestoreBootFromBackup;
                a.offset = 0;
                a.length = sector;
                a.after.assign(backup, backup + sector);
                a.reason = "primary FAT boot sector damaged; restore from backup sector 6";
                plan.actions.push_back(std::move(a));
                return OK;
            }
        }
    }

    /* exFAT with a destroyed primary OEM is handled above via the backup
     * boot sector probe (exfat_via_backup); nothing further here. */

    plan.refused = true;
    plan.refusal_reason = "no recognizable filesystem family (leaves carving path, corpus 0x19)";
    return OK;
}

/* ---- Phase 2: execute ---- */

u32 execute(IWriteDevice* dev, const RepairPlan& plan, UndoLog& undo) {
    if (dev == nullptr)
        return ERR_BAD_PARAMETER;
    if (plan.refused || plan.actions.empty())
        return ERR_REPAIR_REFUSED;

    const size_t applied_before = undo.entries.size();
    undo.entries.reserve(undo.entries.size() + plan.actions.size());
    /* S4/S5 helper: undo every action applied so far, reverse order.
     * Entries that fail to restore are KEPT in the log (the operator can
     * still retry them) - erasing unrestored evidence would violate S4. */
    size_t rollback_failures = 0;
    const auto rollback_all = [&]() {
        std::vector<size_t> keep;
        for (size_t i = undo.entries.size(); i-- > applied_before;) {
            const auto& e = undo.entries[i];
            if (dev->write_at(e.offset, e.original.data(), e.length) != OK) {
                ++rollback_failures;
                keep.push_back(i);
            }
        }
        if (keep.empty()) {
            undo.entries.resize(applied_before);
        } else {
            /* retain the unrestored entries in order */
            std::vector<UndoLog::Entry> saved;
            for (size_t k = keep.size(); k-- > 0;)
                saved.push_back(std::move(undo.entries[keep[k]]));
            undo.entries.resize(applied_before);
            for (auto& e : saved)
                undo.entries.push_back(std::move(e));
        }
    };
    /* Raw-disk writes must be sector-granular: every action is coalesced
     * up to the containing sector span (a 1-byte geometry patch becomes a
     * read-modify-write of its whole sector, corpus UpdateDBR semantics).
     * The undo log therefore always holds full sectors. */
    u32 ssec = dev->sector_bytes();
    if (ssec == 0 || ssec > 65536 || (ssec & (ssec - 1)) != 0)
        ssec = 512;
    try {
        for (const auto& a : plan.actions) {
            if (a.length == 0 || a.after.size() != a.length) {
                rollback_all();
                return ERR_BAD_PARAMETER;
            }
            const u64 span_start = a.offset / ssec * ssec;
            const u64 span_end = (a.offset + a.length + ssec - 1) / ssec * ssec;
            const u32 span_len = u32(span_end - span_start);
            if (span_end > dev->size()) {
                rollback_all();
                return rollback_failures ? ERR_REPAIR_UNDO_FAILED : ERR_RANGE_OUT_OF_BOUNDS;
            }
            /* S4: snapshot the full span BEFORE the write (CopySectors). A
             * snapshot failure also rolls back: without it we cannot keep
             * the transaction guarantee. */
            std::vector<u8> span(span_len);
            if (dev->read_at(span_start, span.data(), span_len) != OK) {
                rollback_all();
                return rollback_failures ? ERR_REPAIR_UNDO_FAILED : ERR_READ_FAILED;
            }
            undo.record(span_start, span_len, span.data());

            /* patch the action bytes into the sector image */
            std::memcpy(span.data() + (a.offset - span_start), a.after.data(), a.length);

            /* write the full span */
            if (dev->write_at(span_start, span.data(), span_len) != OK) {
                /* the write never landed: this action's own snapshot is
                 * unnecessary (the sector is unchanged) - drop it, then roll
                 * back everything that DID land */
                if (!undo.entries.empty() && undo.entries.back().offset == span_start)
                    undo.entries.pop_back();
                rollback_all();
                return rollback_failures ? ERR_REPAIR_UNDO_FAILED : ERR_WRITE_FAILED;
            }

            /* S5: read back and verify the write landed */
            std::vector<u8> check(span_len);
            if (dev->read_at(span_start, check.data(), span_len) != OK ||
                std::memcmp(check.data(), span.data(), span_len) != 0) {
                rollback_all();
                return ERR_REPAIR_UNDO_FAILED;
            }
        }
    } catch (const std::bad_alloc&) {
        rollback_all();
        return ERR_NO_MEMORY;
    } catch (const std::length_error&) {
        rollback_all();
        return ERR_NO_MEMORY;
    }
    return OK;
}

/* ---- Phase 3: undo ---- */

u32 undo(IWriteDevice& dev, const UndoLog& log) {
    for (const auto& e : log.entries) {
        if (e.offset + e.length > dev.size())
            return ERR_RANGE_OUT_OF_BOUNDS;
        if (dev.write_at(e.offset, e.original.data(), e.length) != OK)
            return ERR_WRITE_FAILED;
        /* verify */
        std::vector<u8> check(e.length);
        if (dev.read_at(e.offset, check.data(), e.length) != OK ||
            std::memcmp(check.data(), e.original.data(), e.length) != 0)
            return ERR_REPAIR_UNDO_FAILED;
    }
    return OK;
}

} // namespace repair
} // namespace cr
