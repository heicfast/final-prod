/*
 * cr_apply.cpp - consent-gated lost-partition apply (S-029, diff-to-fix).
 * See cr_apply.h for the corpus contract and the A1-A5 safety standard.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/apply/cr_apply.h"
#include <cstring>

namespace cr {
namespace apply {

namespace {

constexpr size_t kSector = 512;
constexpr size_t kTableOff = 446;    /* the MBR entry table base */
constexpr size_t kEntryBytes = 16;   /* one partition entry */
constexpr u8 kMaxChs[3] = {0xFE, 0xFF, 0xFF}; /* beyond-cylinder-1023 */

/* LBA -> CHS bytes (255 heads / 63 sectors-per-track, the universal DOS
 * geometry a BIOS/firmware reports for large disks). Beyond cylinder
 *1023 the maximum CHS is written (the convention the kernel derives for
 * DRIVE_LAYOUT_INFORMATION MBR writes; nothing boot-relevant reads CHS
 * for LBA-booted partitions). */
void lba_to_chs(u64 lba, u8 out[3]) {
    constexpr u64 kHeads = 255, kSpt = 63;
    const u64 cyl = lba / (kHeads * kSpt);
    if (cyl > 1023) {
        out[0] = kMaxChs[0];
        out[1] = kMaxChs[1];
        out[2] = kMaxChs[2];
        return;
    }
    const u64 rem = lba % (kHeads * kSpt);
    const u64 head = rem / kSpt;
    const u64 sect = (rem % kSpt) + 1;
    out[0] = u8(head);
    out[1] = u8(((cyl >> 8) & 0x03) << 6) | u8(sect & 0x3F);
    out[2] = u8(cyl & 0xFF);
}

void put32le(u8* p, u32 v) {
    p[0] = u8(v);
    p[1] = u8(v >> 8);
    p[2] = u8(v >> 16);
    p[3] = u8(v >> 24);
}

u32 get32le(const u8* p) {
    return u32(p[0]) | u32(p[1]) << 8 | u32(p[2]) << 16 | u32(p[3]) << 24;
}

bool entry_is_zero(const u8* e) {
    for (size_t i = 0; i < kEntryBytes; ++i)
        if (e[i] != 0)
            return false;
    return true;
}

void refuse(ApplyPlan& p, const char* reason) {
    p.refused = true;
    p.refusal_reason = reason;
    p.actions.clear();
    p.entry_bytes.clear();
}

} // namespace

u8 mbr_type_for_family(lost::CandidateFs fs) {
    switch (fs) {
    case lost::CandidateFs::Ntfs:
    case lost::CandidateFs::Exfat:
    case lost::CandidateFs::Refs:
    case lost::CandidateFs::BitLocker:
        return 0x07; /* HPFS/NTFS/exFAT - the Microsoft data family */
    case lost::CandidateFs::Fat:
        return 0x06; /* FAT12/16 (big) */
    case lost::CandidateFs::GptTable:
        return 0xEE; /* protective (never planned by apply - refused) */
    case lost::CandidateFs::Ext2:
    case lost::CandidateFs::Btrfs:
        return 0x83; /* Linux native */
    case lost::CandidateFs::HfsPlus:
        return 0xAF; /* Apple HFS/HFS+ */
    case lost::CandidateFs::Zfs:
        return 0xBF; /* Solaris/ZFS */
    case lost::CandidateFs::Unknown:
    default:
        return 0x07; /* the generic data type - the honest default for
                      * an unclassified-but-scored candidate */
    }
}

u32 plan_apply(IStorageDevice* disk, const lost::Candidate& c, ApplyPlan& out) {
    out = ApplyPlan{};
    out.start_lba = c.start_lba;
    out.sector_count = c.sector_count;
    out.fs = c.fs;
    out.mbr_type = mbr_type_for_family(c.fs);

    if (disk == nullptr)
        return ERR_BAD_PARAMETER;
    if (c.sector_count == 0) {
        refuse(out, "candidate has no size (sector_count 0)");
        return OK;
    }

    /* read + snapshot sector 0 (the og re-reads sector 0 raw before its
     * rewrite, PR:46944 - the same truth, captured at plan time so
     * execute can detect a table that moved underneath) */
    u8 sec[kSector];
    if (disk->read_at(0, sec, sizeof(sec)) != OK) {
        refuse(out, "cannot read sector 0 (the disk's table is not readable)");
        return OK;
    }
    out.sector0_original.assign(sec, sec + kSector);

    /* GPT refusal: a protective entry means the layout rebuild is the
     * GPT variant (FUN_1800392f0, GUID regen) - v2 scope, refused
     * cleanly, never a half-mode write */
    if (sec[kTableOff + 4] == 0xEE) {
        refuse(out, "GPT layout (protective MBR) - GPT apply is not supported yet");
        return OK;
    }

    /* A3 containment: LBA 0 is the MBR itself */
    if (c.start_lba == 0) {
        refuse(out, "candidate starts at LBA 0 (the MBR sector itself)");
        return OK;
    }
    /* A3 containment: the candidate must fit the disk */
    const u64 disk_sectors =
        disk->size() / (disk->geometry().logical_sector ? disk->geometry().logical_sector : 512);
    if (disk_sectors != 0 && c.start_lba + c.sector_count > disk_sectors) {
        refuse(out, "candidate extends past the end of the disk");
        return OK;
    }
    /* A3 containment: no overlap with any live entry (the og's recovery
     * flow ADDS the recovered partition; live entries are never
     * overwritten by the single-partition apply) */
    for (u32 i = 0; i < 4; ++i) {
        const u8* e = sec + kTableOff + i * kEntryBytes;
        if (entry_is_zero(e))
            continue;
        const u32 lba = get32le(e + 8);
        const u32 cnt = get32le(e + 12);
        if (cnt != 0 && c.start_lba < u64(lba) + cnt && u64(lba) < c.start_lba + c.sector_count) {
            refuse(out, "candidate overlaps an existing partition entry "
                        "(live entries are never overwritten)");
            return OK;
        }
    }

    /* slot pick: the first all-zero entry (the og's 4-entry
     * DRIVE_LAYOUT_INFORMATION rebuild fills free slots; a full table
     * has nowhere to land) */
    out.entry_index = 0xFFFFFFFFu;
    for (u32 i = 0; i < 4; ++i) {
        if (entry_is_zero(sec + kTableOff + i * kEntryBytes)) {
            out.entry_index = i;
            break;
        }
    }
    if (out.entry_index > 3) {
        refuse(out, "the MBR table is full (no free entry slot)");
        return OK;
    }

    /* the entry: status 0x00 (never auto-bootable), CHS pair, type,
     * LBA + count. The candidate's evidence (the discovery score)
     * stands behind every field - apply transcribes, never invents. */
    u8 entry[kEntryBytes] = {};
    u8 chs0[3], chs1[3];
    lba_to_chs(c.start_lba, chs0);
    lba_to_chs(c.start_lba + c.sector_count - 1, chs1);
    entry[0] = 0x00; /* not bootable - the og's flow does not decode a
                      * boot-flag decision for the recovered entry */
    entry[1] = chs0[0];
    entry[2] = chs0[1];
    entry[3] = chs0[2];
    entry[4] = out.mbr_type;
    entry[5] = chs1[0];
    entry[6] = chs1[1];
    entry[7] = chs1[2];
    put32le(entry + 8, u32(c.start_lba));
    put32le(entry + 12, u32(c.sector_count));
    out.entry_bytes.assign(entry, entry + kEntryBytes);

    /* the signature restore (only when absent - the og's rebuild
     * rewrites 0x55AA, PR:46957-46962) */
    if (sec[510] != 0x55 || sec[511] != 0xAA) {
        out.restores_signature = true;
        ApplyAction sig;
        sig.offset = 510;
        sig.after = {0x55, 0xAA};
        sig.reason = "restore the 0x55AA MBR signature (absent on the disk)";
        out.actions.push_back(std::move(sig));
    }

    char type_hex[4];
    type_hex[0] = '0';
    type_hex[1] = 'x';
    type_hex[2] = "0123456789ABCDEF"[out.mbr_type >> 4];
    type_hex[3] = "0123456789ABCDEF"[out.mbr_type & 0xF];
    ApplyAction a;
    a.offset = kTableOff + out.entry_index * kEntryBytes;
    a.after = out.entry_bytes;
    a.reason = "write the recovered partition entry (slot " +
               std::to_string(out.entry_index) + ", LBA " + std::to_string(c.start_lba) + ", " +
               std::to_string(c.sector_count) + " sectors, type " +
               std::string(type_hex, type_hex + 4) + ")";
    out.actions.push_back(std::move(a));
    return OK;
}

u32 execute_apply(repair::IWriteDevice* dev, const ApplyPlan& plan, ApplyConsentFn consent,
                  void* consent_user, repair::UndoLog& undo) {
    if (dev == nullptr || plan.refused || plan.actions.empty())
        return ERR_BAD_PARAMETER;

    /* A4 order: quiesce FIRST (the S-010 sequence in the platform
     * writer) - a busy volume is a clean refusal before any write */
    if (u32 rc = dev->lock_volume())
        return rc;

    /* A2 revalidation: the live sector 0 must still match the plan's
     * snapshot. A third-party table change between plan and execute is
     * never overwritten - replan. */
    const u32 sbytes = dev->sector_bytes() ? dev->sector_bytes() : 512;
    if (sbytes < kSector)
        return ERR_BAD_PARAMETER; /* a sub-512 "sector" device cannot hold
                                     * an MBR - refuse, never a torn write */
    std::vector<u8> now(size_t(sbytes), 0);
    if (dev->read_at(0, now.data(), kSector) != OK)
        return ERR_READ_FAILED;
    if (plan.sector0_original.size() != kSector ||
        std::memcmp(now.data(), plan.sector0_original.data(), kSector) != 0)
        return ERR_APPLY_STALE;

    /* A1 consent: the last gate before the first write */
    if (consent == nullptr || !consent(plan, consent_user))
        return ERR_APPLY_CONSENT_DENIED;

    /* A4 transaction: snapshot, write, verify; any failure rolls back
     * everything applied (reverse order, verified) */
    const size_t applied_start = undo.entries.size();
    for (const ApplyAction& a : plan.actions) {
        if (a.after.empty())
            continue;
        /* the action's original bytes (the undo truth) - re-read per
         * action so co-located writes snapshot correctly */
        std::vector<u8> orig(a.after.size(), 0);
        if (dev->read_at(a.offset, orig.data(), u32(a.after.size())) != OK) {
            /* rollback what was applied, then fail */
            for (size_t k = undo.entries.size(); k-- > applied_start;) {
                (void)dev->write_at(undo.entries[k].offset, undo.entries[k].original.data(),
                                    undo.entries[k].length);
            }
            undo.entries.resize(applied_start);
            return ERR_READ_FAILED;
        }
        undo.record(a.offset, u32(a.after.size()), orig.data());
        if (dev->write_at(a.offset, a.after.data(), u32(a.after.size())) != OK) {
            for (size_t k = undo.entries.size(); k-- > applied_start;) {
                (void)dev->write_at(undo.entries[k].offset, undo.entries[k].original.data(),
                                    undo.entries[k].length);
            }
            undo.entries.resize(applied_start);
            return ERR_WRITE_FAILED;
        }
        /* read-back verify */
        std::vector<u8> check(a.after.size(), 0);
        if (dev->read_at(a.offset, check.data(), u32(a.after.size())) != OK ||
            std::memcmp(check.data(), a.after.data(), a.after.size()) != 0) {
            for (size_t k = undo.entries.size(); k-- > applied_start;) {
                (void)dev->write_at(undo.entries[k].offset, undo.entries[k].original.data(),
                                    undo.entries[k].length);
            }
            undo.entries.resize(applied_start);
            return ERR_WRITE_FAILED;
        }
    }
    return OK;
}

} // namespace apply
} // namespace cr
