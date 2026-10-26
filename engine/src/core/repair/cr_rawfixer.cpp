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
#include "cr/ntfs/cr_attributes.h"
#include "cr/ntfs/cr_fixup.h"
#include "cr/ntfs/cr_ntfs_wire.h"
#include "cr/table/cr_table_wire.h"
#include <algorithm>
#include <cstring>
#include <string>

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

/* ---- S-008: corpus RawFixer method extensions --------------------
 * Ground truth: re-workspace/artifacts/PartitionRecovery/
 * rawfixer-v2--method-inventory.md (33-method table; CheckMFT mechanics
 * §3; CheckFatTable/CheckRootDir specifics §5) + rawfixer-fs-repair.md
 * (strategy trees; CheckDBR -> CheckMFT ordering; FixByScanSector deep
 * fallback). Re-implemented from understanding of the decompile
 * (ghidra-partition/RawFixer_decompiled.c): FUN_18005de40 = 'FILE' magic
 * + record-name check, FUN_18005df00 = $MFTMirr name check, FATFixer::
 * CheckFatTable reads BOTH copies (wFatTableStartSector +
 * backupFatStartSector = reserved + sectors_per_fat) and flags damage.
 * No decompiled text is reproduced; no vendor strings are copied. */

constexpr u64 DEEP_SCAN_WINDOW_BYTES = 64ull << 20; /* FixByScanSector bound */
constexpr u32 SHAPE_NTFS = 1, SHAPE_FAT = 2, SHAPE_EXFAT = 4;

/* NTFSFixer::CheckMFT record validation (corpus FUN_18005de40): 'FILE'
 * magic, USA fixup (the engine's own validator - single source of truth),
 * then the record's $FILE_NAME must equal the expected metafile name. */
bool ntfs_record_named(const u8* rec, u32 recsize, u32 bps, const std::u16string& want) {
    if (recsize < 0x400)
        return false;
    ntfs::RecordHeader rh;
    std::memcpy(&rh, rec, sizeof(rh));
    if (rh.magic != ntfs::RECORD_MAGIC_FILE)
        return false;
    if (ntfs::fixup_validate(rec, recsize, rh.update_sequence_offset, rh.update_sequence_size,
                             bps) != OK)
        return false;
    std::vector<u8> fixed(rec, rec + recsize);
    if (ntfs::fixup_apply(fixed.data(), recsize, rh.update_sequence_offset, rh.update_sequence_size,
                          bps) != OK)
        return false;
    std::vector<ntfs::AttrView> attrs;
    if (ntfs::enum_attributes(fixed.data(), recsize, attrs) != OK)
        return false;
    for (const auto& a : attrs) {
        if (a.type != ntfs::ATTR_TYPE_FILE_NAME || a.nonresident)
            continue;
        ntfs::FileNameInfo fi;
        if (ntfs::parse_file_name(a, fi) == OK && fi.name == want)
            return true;
    }
    return false;
}

/* NTFSFixer::CheckMFT (+ IsValidRootDir as a report-only advisory) run
 * against the boot-sector image the scanner will see once any planned
 * boot action applies (primary bytes, or backup/candidate bytes when a
 * restore is planned first - the corpus CheckDBR->UpdateDBR->verify
 * order). Record-size math per rawfixer-v2 §3: cpr<0 -> 1<<(-cpr), else
 * clusters * cluster size. */
void ntfs_check_mft(IStorageDevice* slice, RepairPlan& plan, const u8* boot, u64 total) {
    const u16 bps = le16(boot + 0x0B);
    const u8 spc = boot[0x0D];
    const u64 mft_lcn = le64(boot + 0x30);
    const u64 mirr_lcn = le64(boot + 0x38);
    const i8 cpr = i8(boot[0x40]);
    if (bps < 512 || bps > 4096 || (bps & (bps - 1)) != 0 || spc == 0)
        return; /* unparseable geometry: the boot-repair path owns it */
    const u64 cluster_bytes = u64(spc) * bps;
    u64 recsize = 0;
    if (cpr < 0) {
        const u32 shift = u32(-i32(cpr)) & 0x1Fu;
        if (shift > 16)
            return; /* 64 KiB records are the ceiling; larger is corruption */
        recsize = 1ull << shift;
    } else if (cpr > 0) {
        recsize = u64(cpr) * cluster_bytes;
    } else {
        return; /* cpr == 0: undefined */
    }
    if (recsize < 0x400 || recsize > 65536)
        return;
    const u64 mft_off = mft_lcn * cluster_bytes;
    if (mft_off + recsize > total)
        return; /* BPB points outside the volume - not CheckMFT's call */

    /* IsValidRootDir (report-only v1): root = MFT record 5. */
    {
        const u64 root_off = mft_off + 5ull * recsize;
        if (root_off + recsize <= total) {
            std::vector<u8> root(recsize);
            if (slice->read_at(root_off, root.data(), u32(recsize)) == OK) {
                ntfs::RecordHeader rh;
                std::memcpy(&rh, root.data(), sizeof(rh));
                const bool ok =
                    rh.magic == ntfs::RECORD_MAGIC_FILE &&
                    ntfs::fixup_validate(root.data(), recsize, rh.update_sequence_offset,
                                         rh.update_sequence_size, bps) == OK;
                if (!ok)
                    plan.advisories.push_back(
                        "root record (MFT #5) invalid (IsValidRootDir; directory-content repair "
                        "out of scope)");
            } else {
                plan.advisories.push_back("root record (MFT #5) unreadable (IsValidRootDir)");
            }
        }
    }

    std::vector<u8> rec(recsize);
    if (slice->read_at(mft_off, rec.data(), u32(recsize)) != OK) {
        plan.advisories.push_back("$MFT record 0 unreadable (CheckMFT)");
        return;
    }
    if (ntfs_record_named(rec.data(), u32(recsize), bps, std::u16string(u"$MFT")))
        return; /* clean: never touched (S2) */

    /* primary invalid -> the mirror-backed restore (corpus: CheckMFT is
     * mirror-backed; FUN_18005df00 validates the $MFTMirr identity). The
     * mirror's slot 0 is a byte copy of $MFT record 0, so it must itself
     * be a valid '$MFT'-named record before we trust it as the source. */
    const u64 mirr_off = mirr_lcn * cluster_bytes;
    if (mirr_off + recsize <= total && mirr_off != mft_off) {
        std::vector<u8> mirr(recsize);
        if (slice->read_at(mirr_off, mirr.data(), u32(recsize)) == OK &&
            ntfs_record_named(mirr.data(), u32(recsize), bps, std::u16string(u"$MFT"))) {
            RepairAction a;
            a.kind = ActionKind::RestoreMftRecordFromMirror;
            a.offset = mft_off;
            a.length = u32(recsize);
            a.after = mirr;
            a.reason = "$MFT record 0 invalid; restore from $MFTMirr slot 0 (CheckMFT)";
            plan.actions.push_back(std::move(a));
            return;
        }
    }
    plan.advisories.push_back(
        "$MFT record 0 invalid and $MFTMirr slot 0 unusable - no mirror-backed repair (S3)");
}

/* FAT copy entry-signature check (FATFixer::CheckFatTable side): entry
 * 0 = 0xFF..|media, entry 1 = end-of-chain, at the width's packing. */
bool fat_copy_signature_ok(const u8* first, u32 width, u8 media) {
    if (first[0] != media)
        return false;
    if (width == 12)
        return first[1] == 0xFF && first[2] == 0xFF;
    if (width == 16)
        return first[1] == 0xFF && le16(first + 2) == 0xFFFF;
    return first[1] == 0xFF && first[2] == 0xFF && first[3] == 0xFF &&
           (le32(first + 4) & 0x0FFFFFF8u) == 0x0FFFFFF8u;
}

/* FATFixer::CheckFatTable (mirror compare + torn-copy restore) and
 * CheckRootDir (report-only advisory), run against the boot-sector image
 * the scanner will see. Sector-granular: only differing sectors of the
 * torn copy are rewritten, undo-logged by execute(). */
void fat_check_tables_and_root(IStorageDevice* slice, RepairPlan& plan, const u8* boot, u32 sector,
                               u64 total) {
    const u16 bps = le16(boot + 0x0B);
    const u8 spc = boot[0x0D];
    const u16 reserved = le16(boot + 0x0E);
    const u8 nfats = boot[0x10];
    const u16 root_entries = le16(boot + 0x11);
    const u16 fat_len16 = le16(boot + 0x16);
    const u32 fat_len32 = le32(boot + 0x24);
    const u8 media = boot[0x15];
    const u32 total_sec = le16(boot + 0x13) != 0 ? le16(boot + 0x13) : le32(boot + 0x20);
    if (bps < 512 || bps > 4096 || (bps & (bps - 1)) != 0 || spc == 0 || reserved == 0 ||
        total_sec == 0)
        return;
    const u32 fat_len = fat_len16 != 0 ? fat_len16 : fat_len32;
    if (fat_len == 0)
        return;

    const u64 root_secs = (u64(root_entries) * 32 + bps - 1) / bps;
    const u64 data_start_sec = u64(reserved) + u64(nfats) * fat_len + root_secs;
    if (data_start_sec >= total_sec)
        return; /* degenerate layout: the boot-repair path owns it */
    const u64 clusters = (total_sec - data_start_sec) / spc;
    const u32 width = clusters < 4085 ? 12 : (clusters < 65525 ? 16 : 32);

    /* ---- CheckFatTable (only when the BPB declares a mirror) ---- */
    const u64 fat_bytes = u64(fat_len) * bps;
    const u64 fat1_off = u64(reserved) * bps;
    const u64 fat2_off = fat1_off + fat_bytes;
    if (nfats >= 2 && fat2_off + fat_bytes <= total) {
        constexpr u32 CHUNK = 64u * 1024u; /* X51: 64 KiB multiples */
        std::vector<u8> a(CHUNK), b(CHUNK);
        bool a_sig = false, b_sig = false, read_fail = false;
        std::vector<u64> diff; /* differing sector offsets inside the FAT */
        for (u64 off = 0; off < fat_bytes; off += CHUNK) {
            const u32 n = u32(std::min<u64>(CHUNK, fat_bytes - off));
            if (slice->read_at(fat1_off + off, a.data(), n) != OK ||
                slice->read_at(fat2_off + off, b.data(), n) != OK) {
                read_fail = true;
                break;
            }
            if (off == 0) {
                a_sig = fat_copy_signature_ok(a.data(), width, media);
                b_sig = fat_copy_signature_ok(b.data(), width, media);
            }
            if (std::memcmp(a.data(), b.data(), n) != 0) {
                for (u64 s = 0; s + sector <= n; s += sector)
                    if (std::memcmp(a.data() + s, b.data() + s, sector) != 0)
                        diff.push_back(off + s);
            }
        }
        if (read_fail) {
            plan.advisories.push_back("FAT mirror copies unreadable (CheckFatTable)");
        } else if (diff.empty()) {
            if (!a_sig)
                plan.advisories.push_back(
                    "both FAT copies carry a damaged entry signature - no mirror source "
                    "(CheckFatTable)");
        } else if (a_sig == b_sig) {
            /* both (or neither) signature-valid: no side is provably the
             * good one - a coin-flip write is forbidden (S3) */
            plan.advisories.push_back(
                "FAT mirror copies differ with equal signature validity - ambiguous, no restore "
                "(CheckFatTable, S3)");
        } else {
            const u64 good_off = a_sig ? fat1_off : fat2_off;
            const u64 bad_off = a_sig ? fat2_off : fat1_off;
            /* coalesce contiguous differing sectors into actions */
            size_t run_start = 0;
            for (size_t k = 1; k <= diff.size(); ++k) {
                const bool flush = k == diff.size() || diff[k] != diff[k - 1] + sector;
                if (!flush)
                    continue;
                const u64 span_off = diff[run_start];
                const u32 span_len = u32(diff[k - 1] + sector - span_off);
                RepairAction act;
                act.kind = ActionKind::RestoreFatSectorFromMirror;
                act.offset = bad_off + span_off;
                act.length = span_len;
                act.after.resize(span_len);
                if (slice->read_at(good_off + span_off, act.after.data(), span_len) != OK)
                    continue; /* source unreadable: skip this run, keep the rest */
                act.reason = "FAT mirror copy torn; restore " + std::to_string(span_len / sector) +
                             " sector(s) from the signature-valid sibling (CheckFatTable)";
                plan.actions.push_back(std::move(act));
                run_start = k;
            }
        }
    }

    /* ---- CheckRootDir (report-only advisory) ---- */
    u64 root_off = 0;
    bool have_root = false;
    if (width == 32) {
        const u32 root_cluster = le32(boot + 0x2C);
        if (root_cluster >= 2) {
            root_off = (data_start_sec + (u64(root_cluster) - 2) * spc) * bps;
            have_root = true;
        }
    } else if (root_entries > 0) {
        root_off = (u64(reserved) + u64(nfats) * fat_len) * bps;
        have_root = true;
    }
    if (have_root && root_off + 512 <= total) {
        u8 rootsec[512];
        if (slice->read_at(root_off, rootsec, 512) != OK)
            plan.advisories.push_back("root directory region unreadable (CheckRootDir)");
    }
}

/* ExFATFixer::CheckRoot (report-only advisory): the root cluster's first
 * sector inside the cluster heap. */
void exfat_check_root_advise(IStorageDevice* slice, RepairPlan& plan, const u8* boot, u64 total) {
    const u32 bps_shift = boot[0x6C];
    const u8 spc_shift = boot[0x6D];
    if (bps_shift < 9 || bps_shift > 12)
        return;
    const u64 bps = 1ull << bps_shift;
    const u64 heap_off = le32(boot + 0x58);
    const u32 root_cluster = le32(boot + 0x60);
    if (root_cluster < 2)
        return;
    const u64 root_off = (heap_off + ((u64(root_cluster) - 2) << spc_shift)) * bps;
    if (root_off + 512 > total)
        return;
    u8 rootsec[512];
    if (slice->read_at(root_off, rootsec, 512) != OK)
        plan.advisories.push_back("root cluster unreadable (CheckRoot)");
}

/* Plan the exFAT boot-region checksum recompute for a primary restore
 * whose new bytes are `restore` (the action list order guarantees the
 * restore applies first). Shared by the backup restore and the
 * FixByScanSector candidate paths. */
void plan_exfat_checksum_after(IStorageDevice* slice, RepairPlan& plan, u32 bps, const u8* restore,
                               u32 restore_len) {
    std::vector<u8> region(size_t(12) * bps, 0);
    if (slice->read_at(0, region.data(), u32(region.size())) != OK)
        return; /* analyze already failed reads upstream in practice */
    /* A-005: mirror the FULL restored sector (the pre-fix 512-byte copy
     * checksummed a half-stale region whenever bps > 512) */
    if (restore_len > region.size())
        restore_len = u32(region.size());
    std::memcpy(region.data(), restore, restore_len);
    const u32 new_sum = exfat::boot_region_checksum(region.data(), bps);
    RepairAction c;
    c.kind = ActionKind::RepairBootChecksum;
    c.offset = 11 * u64(bps);
    c.length = bps;
    std::vector<u8> fixed(bps, 0);
    for (u32 i = 0; i < bps; i += 4) {
        fixed[i] = u8(new_sum);
        fixed[i + 1] = u8(new_sum >> 8);
        fixed[i + 2] = u8(new_sum >> 16);
        fixed[i + 3] = u8(new_sum >> 24);
    }
    c.after = fixed;
    c.reason = "recompute boot-region checksum after primary restore";
    plan.actions.push_back(std::move(c));
}

/* FixByScanSector (shared corpus method): bounded windowed probe of the
 * first 64 MiB for boot-shaped sectors, family-masked. Reads 64 KiB
 * blocks and probes each logical-sector slot; the damaged primary itself
 * (offset 0) is skipped. Candidates are content-deduped via CRC32 -
 * byte-identical copies are one repair, not an ambiguity. */
struct ScanCandidate {
    u64 offset = 0;
    std::vector<u8> bytes;
};

void deep_scan_boot_candidates(IStorageDevice* slice, u32 sector, u64 total, u32 family_mask,
                               std::vector<ScanCandidate>& out) {
    out.clear();
    const u64 window = std::min(total, DEEP_SCAN_WINDOW_BYTES);
    constexpr u32 CHUNK = 64u * 1024u; /* X51: 64 KiB multiples */
    std::vector<u8> buf(CHUNK);
    std::vector<ScanCandidate> hits;
    for (u64 off = 0; off < window;) {
        u32 n = u32(std::min<u64>(CHUNK, window - off));
        n = n / sector * sector;
        if (n == 0)
            break;
        if (slice->read_at(off, buf.data(), n) == OK) {
            for (u64 s = 0; s + 512 <= n; s += sector) {
                if (off + s == 0)
                    continue; /* the damaged primary itself */
                const u8* sec = buf.data() + s;
                bool hit = false;
                if ((family_mask & SHAPE_NTFS) != 0 && ntfs_boot_sane(sec, total))
                    hit = true;
                if (!hit && (family_mask & SHAPE_FAT) != 0 && fat_boot_sane(sec, total))
                    hit = true;
                if (!hit && (family_mask & SHAPE_EXFAT) != 0 && exfat_boot_sane(sec, total, sector))
                    hit = true;
                if (hit)
                    hits.push_back({off + s, std::vector<u8>(sec, sec + 512)});
            }
        }
        off += n;
    }
    /* content dedupe (identical copies = one repair) */
    for (const auto& h : hits) {
        const u32 crc = table::crc32(h.bytes.data(), u32(h.bytes.size()));
        bool dup = false;
        for (const auto& kept : out)
            if (table::crc32(kept.bytes.data(), u32(kept.bytes.size())) == crc) {
                dup = true;
                break;
            }
        if (!dup)
            out.push_back(h);
    }
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

void rebase_plan(RepairPlan& plan, u64 base_bytes) {
    for (auto& a : plan.actions)
        a.offset += base_bytes;
}

u32 analyze(IStorageDevice* slice, RepairPlan& plan) {
    plan.actions.clear();
    plan.refused = false;
    plan.refusal_reason.clear();
    plan.detected = false;
    plan.fs_family.clear();
    if (slice == nullptr)
        return ERR_BAD_PARAMETER;

    const u64 total = slice->size();
    /* FIX-3 (AUDIT-PIPELINE F-A4-25): sector size comes from the DEVICE
     * geometry (4Kn volumes read the exFAT backup at 12*4096, not
     * 12*512; execute() already used sector_bytes() - analyze() was the
     * hardcoded side). */
    const u32 sector = slice->geometry().logical_sector ? slice->geometry().logical_sector : 512;
    if (total < u64(24) * sector)
        return ERR_BAD_PARAMETER;

    /* A-005 (diff-to-fix): the four boot-sector buffers are sized by the
     * DEVICE sector (the FIX-3 read lengths were already sector-sized -
     * the 512-byte stacks overflowed by 3,584 bytes on every 4Kn read).
     * Heap vectors: the geometry guard accepts up to 64 KiB sectors. */
    std::vector<u8> primary(sector, 0);
    if (slice->read_at(0, primary.data(), sector) != OK)
        return ERR_READ_FAILED;

    /* ---- family detection (corpus CheckPartition) ----
     * A destroyed primary OEM still identifies the family through the
     * backup boot sector (exFAT sector 12 / FAT sector 6 / NTFS volume
     * end) - those probes run inside the family branches below. */
    const bool is_ntfs = std::memcmp(primary.data() + 3, "NTFS    ", 8) == 0;
    const bool is_exfat = std::memcmp(primary.data() + 3, "EXFAT   ", 8) == 0;
    const bool fat_sig = primary[510] == 0x55 && primary[511] == 0xAA;
    const bool is_fat = !is_ntfs && !is_exfat && fat_sig && fat_boot_sane(primary.data(), total);
    /* exFAT with a destroyed primary: the backup boot sector at 12 keeps
     * the signature (corpus FixFromDBR primary/backup pair). */
    bool exfat_via_backup = false;
    if (!is_ntfs && !is_exfat && !is_fat && total >= u64(24) * sector) {
        std::vector<u8> bak(sector, 0);
        if (slice->read_at(u64(12) * sector, bak.data(), sector) == OK &&
            exfat_boot_sane(bak.data(), total, sector))
            exfat_via_backup = true;
    }

    /* ---- NTFS: primary boot damaged -> volume-end backup (FixFromDBR);
     * no backup -> FixByScanSector deep probe; CheckMFT in every branch
     * where a boot sector the scanner will parse exists ---- */
    if (is_ntfs) {
        plan.detected = true;
        plan.fs_family = "ntfs";
        if (ntfs_boot_sane(primary.data(), total)) {
            ntfs_check_mft(slice, plan, primary.data(),
                           total); /* corpus: CheckDBR ok -> CheckMFT */
            return OK;             /* S2: clean boot; MFT check adds only what it proved */
        }
        std::vector<u8> backup(sector, 0);
        if (slice->read_at(total - sector, backup.data(), sector) == OK &&
            ntfs_boot_sane(backup.data(), total)) {
            RepairAction a;
            a.kind = ActionKind::RestoreBootFromBackup;
            a.offset = 0;
            a.length = sector;
            a.after.assign(backup.begin(), backup.end());
            a.reason = "primary NTFS boot sector damaged; restore from volume-end backup";
            plan.actions.push_back(std::move(a));
            /* corpus order: UpdateDBR then VERIFY (IsValidRootDir/CheckMFT)
             * - the checks run against the to-be-restored boot image */
            ntfs_check_mft(slice, plan, backup.data(), total);
            return OK;
        }
        /* FixByScanSector (deep): the only NTFS-shaped candidate in the
         * bounded 64 MiB window rebuilds the DBR; multiple distinct
         * candidates are an ambiguity (S3), never a coin flip. */
        std::vector<ScanCandidate> cands;
        deep_scan_boot_candidates(slice, sector, total, SHAPE_NTFS, cands);
        if (cands.size() == 1) {
            RepairAction a;
            a.kind = ActionKind::DeepScanBootRestore;
            a.offset = 0;
            a.length = sector;
            a.after = cands[0].bytes;
            a.reason = "primary NTFS boot damaged, no valid backup; rebuild from the only "
                       "boot-shaped sector in the 64 MiB window (FixByScanSector)";
            plan.actions.push_back(std::move(a));
            ntfs_check_mft(slice, plan, cands[0].bytes.data(), total);
            return OK;
        }
        if (cands.size() > 1)
            plan.refusal_reason = "NTFS primary damaged, no valid backup, " +
                                  std::to_string(cands.size()) +
                                  " distinct boot-shaped sectors in the deep window (S3)";
        else
            plan.refusal_reason = "NTFS primary damaged and no valid backup boot sector (S3)";
        plan.refused = true;
        return OK;
    }

    /* ---- exFAT: checksum repair + geometry rederivation + backup restore ---- */
    if (is_exfat || exfat_via_backup) {
        plan.detected = true;
        plan.fs_family = "exfat";
        if (exfat_via_backup) {
            /* the primary is too damaged to trust any field in it:
             * restore the boot sector from the backup copy, then plan the
             * checksum recompute (the restored sector changes the sum).
             * A-005: the restore plan is SECTOR-sized (a 512-of-4096
             * restore leaves the rest of the 4Kn boot sector garbage) and
             * the checksum-after mirrors the full restored sector. */
            std::vector<u8> backup(sector, 0);
            if (slice->read_at(u64(12) * sector, backup.data(), sector) != OK)
                return ERR_READ_FAILED;
            RepairAction a;
            a.kind = ActionKind::RestoreBootFromBackup;
            a.offset = 0;
            a.length = sector;
            a.after.assign(backup.begin(), backup.end());
            a.reason = "primary exFAT boot sector destroyed; restore from backup sector 12";
            plan.actions.push_back(std::move(a));
            /* recompute the checksum over the restored primary (the boot
             * region's sector size is the device sector here - the primary
             * is destroyed, so the device geometry is the only source) */
            plan_exfat_checksum_after(slice, plan, sector, backup.data(), sector);
            exfat_check_root_advise(slice, plan, backup.data(), total); /* S-008 CheckRoot */
            return OK;
        }
        const auto* m = reinterpret_cast<const exfat::BootSectorMain*>(primary.data());
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

        const bool boot_shape_ok = exfat_boot_sane(primary.data(), total, bps);

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

        if (checksum_ok && geometry_ok) {
            exfat_check_root_advise(slice, plan, primary.data(), total); /* S-008 CheckRoot */
            return OK;                                                   /* S2: clean */
        }

        /* Primary boot shape damaged beyond the OEM? Restore the boot
         * sector from the backup copy at sector 12 (corpus FixFromDBR +
         * ChecksumDBR over primary 0 / backup 12). M-046: the backup read
         * and the restore plan are SECTOR-sized (bps, boot-declared) —
         * the old 512-of-4096 read left a truncated backup image and the
         * plan wrote one-eighth of a 4Kn boot sector. */
        if (!boot_shape_ok) {
            std::vector<u8> backup(bps, 0);
            if (slice->read_at(12 * bps, backup.data(), bps) == OK &&
                exfat_boot_sane(backup.data(), total, bps)) {
                RepairAction a;
                a.kind = ActionKind::RestoreBootFromBackup;
                a.offset = 0;
                a.length = bps;
                a.after = backup;
                a.reason = "primary exFAT boot sector damaged; restore from backup sector 12";
                plan.actions.push_back(std::move(a));
                /* after a boot-sector restore the checksum must be
                 * recomputed too (the restored sector changes the sum) */
                checksum_ok = false;
            } else {
                /* FixByScanSector deep fallback (S-008): the only
                 * exFAT-shaped candidate in the bounded 64 MiB window
                 * rebuilds the primary; ambiguity is refused (S3). */
                std::vector<ScanCandidate> cands;
                deep_scan_boot_candidates(slice, sector, total, SHAPE_EXFAT, cands);
                if (cands.size() == 1) {
                    RepairAction a;
                    a.kind = ActionKind::DeepScanBootRestore;
                    a.offset = 0;
                    a.length = sector;
                    a.after = cands[0].bytes;
                    a.reason = "primary exFAT boot damaged, no valid backup; rebuild from the "
                               "only boot-shaped sector in the 64 MiB window (FixByScanSector)";
                    plan.actions.push_back(std::move(a));
                    plan_exfat_checksum_after(slice, plan, bps, cands[0].bytes.data(),
                                              u32(cands[0].bytes.size()));
                    exfat_check_root_advise(slice, plan, cands[0].bytes.data(), total);
                    return OK;
                }
                plan.refused = true;
                plan.refusal_reason =
                    cands.size() > 1 ? "exFAT primary damaged, no valid backup, " +
                                           std::to_string(cands.size()) +
                                           " distinct boot-shaped sectors in the deep window (S3)"
                                     : "exFAT primary damaged and no valid backup boot sector (S3)";
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
                if (act.kind == ActionKind::RestoreBootFromBackup && act.offset == 0) {
                    /* A-005: mirror the restore's ACTUAL length (a 4Kn
                     * restore is 4096 bytes; the fixed 512 left the rest
                     * of the region stale for the checksum) */
                    const size_t n = std::min(act.after.size(), after_region.size());
                    std::memcpy(after_region.data(), act.after.data(), n);
                }
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
        exfat_check_root_advise(slice, plan, primary.data(), total); /* S-008 CheckRoot */
        return OK;
    }

    /* ---- FAT family ---- */
    if (is_fat) {
        plan.detected = true;
        plan.fs_family = "fat";
        /* corpus FATFixer order: CheckDBR ok -> CheckBackupDBR ->
         * CheckFatTable -> CheckRootDir/CheckCluster */
        fat_check_tables_and_root(slice, plan, primary.data(), sector, total);
        return OK; /* S2: clean boot; the table check adds only what it proved */
    }

    /* FAT primary damaged (or FAT32 with corrupt DBR): probe the backup
     * boot sector at sector 6 (FAT32 only per spec; FAT12/16 have none,
     * but a sane backup there is evidence enough - the corpus FATFixer
     * checks CheckBackupDBR the same way). */
    {
        std::vector<u8> backup(sector, 0); /* A-005: sector-sized (was u8[512]) */
        if (total >= 7 * sector && slice->read_at(6 * sector, backup.data(), sector) == OK) {
            if (fat_boot_sane(backup.data(), total)) {
                plan.detected = true;
                plan.fs_family = "fat";
                RepairAction a;
                a.kind = ActionKind::RestoreBootFromBackup;
                a.offset = 0;
                a.length = sector;
                a.after.assign(backup.begin(), backup.end());
                a.reason = "primary FAT boot sector damaged; restore from backup sector 6";
                plan.actions.push_back(std::move(a));
                /* corpus order: UpdateDBR then verify - the table/root
                 * checks run against the to-be-restored boot image */
                fat_check_tables_and_root(slice, plan, backup.data(), sector, total);
                return OK;
            }
        }
    }

    /* exFAT with a destroyed primary OEM is handled above via the backup
     * boot sector probe (exfat_via_backup); nothing further here. */

    /* ---- FixByScanSector, family-agnostic terminal (S-008): every
     * family probe failed, but a single boot-shaped sector in the
     * bounded 64 MiB window rebuilds the primary DBR. Distinct multiple
     * candidates (any family) are an ambiguity (S3). An exFAT candidate
     * additionally needs the boot-region checksum recompute. */
    {
        std::vector<ScanCandidate> cands;
        deep_scan_boot_candidates(slice, sector, total, SHAPE_NTFS | SHAPE_FAT | SHAPE_EXFAT,
                                  cands);
        if (cands.size() == 1) {
            const auto& cand = cands[0];
            const bool cand_exfat = exfat_boot_sane(cand.bytes.data(), total, sector);
            plan.detected = true;
            plan.fs_family =
                cand_exfat ? "exfat" : (ntfs_boot_sane(cand.bytes.data(), total) ? "ntfs" : "fat");
            RepairAction a;
            a.kind = ActionKind::DeepScanBootRestore;
            a.offset = 0;
            a.length = sector;
            a.after = cand.bytes;
            a.reason = "no recognizable family in the primary or backups; rebuild the DBR from "
                       "the only boot-shaped sector in the 64 MiB window (FixByScanSector)";
            plan.actions.push_back(std::move(a));
            if (cand_exfat) {
                const u32 cand_bps =
                    1u << (cand.bytes[0x6C] >= 9 && cand.bytes[0x6C] <= 12 ? cand.bytes[0x6C] : 9);
                plan_exfat_checksum_after(slice, plan, cand_bps, cand.bytes.data(),
                                          u32(cand.bytes.size()));
                exfat_check_root_advise(slice, plan, cand.bytes.data(), total);
            } else if (plan.fs_family == "ntfs") {
                ntfs_check_mft(slice, plan, cand.bytes.data(), total);
            } else {
                fat_check_tables_and_root(slice, plan, cand.bytes.data(), sector, total);
            }
            return OK;
        }
        if (cands.size() > 1) {
            plan.refused = true;
            plan.refusal_reason =
                std::to_string(cands.size()) +
                " distinct boot-shaped sectors in the deep window with no primary family "
                "evidence (S3)";
            return OK;
        }
    }

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
    /* S-010 volume quiesce: acquire the device's volume lock BEFORE the
     * first write (corpus RecoveryModule sequence). A lock failure is a
     * clean refusal - zero writes performed, the concurrent user keeps
     * their access and the volume keeps its bytes (never raced). The
     * guard releases the lock on every exit path; on Windows the locks
     * additionally live until the handle closes. The device's own error
     * code propagates (the win layer's busy path is ERR_VOLUME_LOCK_FAILED). */
    if (u32 lrc = dev->lock_volume())
        return lrc;
    struct UnlockGuard {
        IWriteDevice* dev;
        ~UnlockGuard() { dev->unlock_volume(); }
    } unlock_guard{dev};
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

            /* S5: read back and verify the write landed. FIX-2
             * (AUDIT-PIPELINE F-A4-24): a CLEAN rollback (disk restored
             * to its original state) is ERR_REPAIR_REFUSED, not
             * ERR_REPAIR_UNDO_FAILED - the undo-failed code must mean
             * "verify the disk state!", and conflating the two sends a
             * false alarm to the operator. */
            std::vector<u8> check(span_len);
            if (dev->read_at(span_start, check.data(), span_len) != OK ||
                std::memcmp(check.data(), span.data(), span_len) != 0) {
                rollback_all();
                return rollback_failures ? ERR_REPAIR_UNDO_FAILED : ERR_REPAIR_REFUSED;
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
