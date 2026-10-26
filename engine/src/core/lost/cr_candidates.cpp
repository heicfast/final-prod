/* cr_candidates.cpp - lost partition discovery + scoring (R3.1).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/lost/cr_lostpart.h"
#include "cr/ntfs/cr_bootstrap.h"
#include "cr/ntfs/cr_fixup.h"
#include "cr/ntfs/cr_ntfs_wire.h"
#include "cr/storage/cr_slice.h"
#include "cr/fat/cr_fat_engine.h"
#include "cr/exfat/cr_exfat_engine.h"
#include "cr/exfat/cr_exfat_wire.h"
#include "cr/ext4/cr_ext4_engine.h"
#include "cr/hfsplus/cr_hfsplus_engine.h"
#include "cr/btrfs/cr_btrfs_engine.h"
#include "cr/refs/cr_refs_detect.h"
#include <algorithm>
#include <cstring>

namespace cr {
namespace lost {

namespace {

/* Buffered sequential signature scan window (64 KiB). */
class WindowScanner {
public:
    explicit WindowScanner(IStorageDevice* dev) : dev_(dev) {
        buf_.resize(64 * 1024);
        geom_ = dev->geometry();
    }

    /* Fill the window containing byte offset; returns pointer to window
     * base and sets win_base/win_len. false on read failure. */
    bool fetch(u64 byte_off) {
        const u64 win = buf_.size();
        win_base_ = byte_off / win * win;
        u64 avail = dev_->size() - win_base_;
        u64 take = avail < win ? avail : win;
        if (dev_->read_at(win_base_, buf_.data(), u32(take)) != OK)
            return false;
        win_len_ = take;
        return true;
    }

    const u8* at(u64 byte_off) const {
        if (byte_off < win_base_ || byte_off >= win_base_ + win_len_)
            return nullptr;
        return buf_.data() + (byte_off - win_base_);
    }
    u64 window_len() const { return win_len_; }

private:
    IStorageDevice* dev_;
    std::vector<u8> buf_;
    u64 win_base_ = 0;
    u64 win_len_ = 0;
    Geometry geom_;
};

/* --- format probes: each returns evidence points and fills the candidate ---
 */

int probe_ntfs(IStorageDevice* disk, u64 boot_lba, Candidate& c) {
    int pts = 0;
    u8 sec[512];
    if (disk->read_at(boot_lba * disk->geometry().logical_sector, sec, 512) != OK)
        return -100;
    const ntfs::BootSector* bs = reinterpret_cast<const ntfs::BootSector*>(sec);
    if (!ntfs::oem_is_ntfs(*bs))
        return -100;
    pts += 15; /* OEM + valid-looking DBR */

    ntfs::VolumeParams vp;
    if (ntfs::validate_boot_sector(*bs, disk->geometry(), vp) == OK) {
        pts += 25;
        /* $MFT probe (volume-relative -> absolute on the disk) */
        std::vector<u8> rec;
        rec.resize(size_t(vp.record_size));
        u64 off = boot_lba * disk->geometry().logical_sector + vp.mft_lcn * vp.cluster_bytes;
        if (disk->read_at(off, rec.data(), u32(rec.size())) == OK) {
            ntfs::RecordHeader rh;
            std::memcpy(&rh, rec.data(), sizeof(rh));
            if (rh.magic == ntfs::RECORD_MAGIC_FILE &&
                ntfs::fixup_apply(rec.data(), u32(rec.size()), rh.update_sequence_offset,
                                  rh.update_sequence_size, vp.sector_bytes) == OK) {
                pts += 30; /* $MFT alive: very strong */
                c.evidence.push_back({"mft-probe", 30});
            }
        }
        /* length from DBR, bounded by the device */
        u64 sectors = vp.total_clusters * vp.cluster_bytes / vp.sector_bytes;
        c.sector_count = sectors;
        c.evidence.push_back({"bpb-valid", 25});
    } else {
        /* no usable BPB: derive from the device end (audit row 31b - the
         * old fabricated 2048 hid real geometry); the materialize path
         * clamps to the device anyway. GAP-S6-6: mark the fallback - a
         * tail-bounded extent is NOT containment evidence (it spans to
         * disk end by construction and would swallow every later peer
         * in the nested-same-fs dedup; bench 40073093997: ours surfaced
         * ONE of TWO deleted NTFS partitions). */
        const u64 end = disk->geometry().sector_count;
        c.sector_count = end > boot_lba ? end - boot_lba : 1;
        c.size_from_device_end = true;
        c.evidence.push_back({"size-from-device-end", 1});
    }
    c.evidence.push_back({"boot-sig", 15});
    return pts;
}

int probe_fat(IStorageDevice* disk, u64 boot_lba, Candidate& c) {
    u8 sec[512];
    if (disk->read_at(boot_lba * disk->geometry().logical_sector, sec, 512) != OK)
        return -100;
    fat::FatParams p;
    if (fat::validate_boot(sec, disk->size(), p) != OK)
        return -100;
    /* N-034 (diff-to-fix): the og's deep-scan ladder gates
     * (PartitionRecoveryD FUN_180004610 + FUN_1800048f0/4a40/4b70):
     * the flavor is picked by the bracket-ordered gate walk - FAT12
     * (spf <= 200 probe), FAT16 (no string), FAT32 (case-sensitive
     * "FAT32   " memcmp + total32 + spf32). A boot sector every og
     * gate rejects is not a FAT candidate on the og's deep scan; the
     * old core accepted any structurally-valid BPB (noise the og never
     * surfaces). */
    const int flavor = fat::lost_boot_flavor(sec);
    if (flavor == 0)
        return -100;
    int pts = 20; /* valid FAT BPB (og gate passed) */
    c.sector_count = p.total_bytes / p.bytes_per_sector;
    /* N-035: FAT is the metadata-gap family - the data area begins at
     * first_data_sector (reserved + two FATs [+ root dir on FAT12/16]
     * ahead of it; the og's DataAreaStart). totalSec is data-area-sized
     * (EndSec = DataAreaStart + totalSec in the og's dump). */
    c.data_start_lba = boot_lba + p.first_data_sector;
    c.data_sector_count =
        c.sector_count > p.first_data_sector ? c.sector_count - p.first_data_sector
                                             : c.sector_count;
    /* FAT area probe: entries 2..4 readable and first data cluster sane */
    u8 fat_probe[16];
    if (disk->read_at(p.fat_start + 8, fat_probe, 16) == OK)
        pts += 10;
    c.evidence.push_back({"fat-bpb", 20});
    c.evidence.push_back(
        {flavor == 3 ? "fat32-gate" : (flavor == 2 ? "fat16-gate" : "fat12-gate"), 5});
    if (pts > 20)
        c.evidence.push_back({"fat-probe", 10});
    return pts;
}

int probe_exfat(IStorageDevice* disk, u64 boot_lba, Candidate& c) {
    u8 sec[512];
    if (disk->read_at(boot_lba * disk->geometry().logical_sector, sec, 512) != OK)
        return -100;
    const exfat::BootSectorMain* bs = reinterpret_cast<const exfat::BootSectorMain*>(sec);
    if (!exfat::oem_is_exfat(*bs))
        return -100;
    int pts = 15;
    if (bs->volume_length >= 2048 && bs->cluster_count > 0 && bs->cluster_count <= 0xFFFFFFF5) {
        pts += 20;
        c.sector_count = u32(bs->volume_length);
    }
    c.evidence.push_back({"exfat-bpb", pts});
    return pts;
}

/* --- S-019: BitLocker ("-FVE-FS-") lost-partition probe --------------------
 *
 * The og's boot-sector classifier pre-check FUN_180004ce0 builds the 8-byte
 * literal "-FVE-FS-" byte-by-byte on the stack
 * (PartitionRecoveryD_decompiled.c:2778-2785: 2d 46 56 45 2d 46 53 2d) and
 * memcmp's it against the boot sector's OEM area (+3) - on match
 * FUN_180004610 classifies the candidate as type 0xB and skips FAT
 * classification entirely (:2604-2606). PartitionRecoveryD.mo carries the
 * dedicated BitlockerParser deep-scan block parser (:721; wired into the
 * CompleteSearch/RegSearch/Fast1M/Fast generator sets :6463/:6558/:8664/
 * :8720). Pre-S-019 the FVE boot sector fell into the FAT catch-all and
 * died in fat::validate_boot - the lost BitLocker partition was invisible.
 *
 * Scoring: boot-sig +15 (the OEM gate the caller matched + 55AA, the og's
 * classifier); fve-shape +25 when the OEM-version region right after the
 * 8-byte label carries printable version digits/dots (real BitLocker VBRs
 * carry e.g. "-FVE-FS-2013.0.0" / "-FVE-FS-2.0.1.0" style OEM labels; the
 * og's classifier needs only the 8 label bytes, so the shape check only
 * grades confidence - it never rejects).
 *
 * Size: the FVE VBR volume-geometry decode is corpus-UNDECODEED (the
 * finding's open question - BitLockerLib's BitlockerParser field reads);
 * device-tail fallback with the GAP-S6-6 marker, same honest policy as
 * probe_refs.
 */
int probe_bitlocker(IStorageDevice* disk, u64 lba, Candidate& c) {
    u8 sec[512];
    if (disk->read_at(lba * disk->geometry().logical_sector, sec, 512) != OK)
        return -100;
    if (std::memcmp(sec + 3, "-FVE-FS-", 8) != 0)
        return -100; /* the og's exact gate: memcmp(OEM@+3, "-FVE-FS-", 8) */
    int pts = 15;
    c.evidence.push_back({"boot-sig", 15});
    /* version-region shape: printable digits/dots in +0x0B..+0x13 (9 bytes,
     * at least 3 version-ish chars) - grades confidence, never rejects */
    int versionish = 0;
    for (u32 i = 0x0B; i <= 0x13; ++i) {
        /* unsigned char: the 0xFF padding compare (PROTOCOL signed-char
         * rule - a signed char can never equal 0xFF) */
        const unsigned char ch = sec[i];
        if ((ch >= '0' && ch <= '9') || ch == '.')
            ++versionish;
        else if (ch == 0x00 || ch == 0xFF || (ch >= 'A' && ch <= 'Z') || ch == '-' ||
                 (ch >= 'a' && ch <= 'z'))
            continue; /* padding/label-ish: not version chars, not noise */
        else {
            versionish = -1; /* binary garbage: not a real FVE VBR shape */
            break;
        }
    }
    if (versionish >= 3) {
        pts += 25;
        c.evidence.push_back({"fve-shape", 25});
    }
    /* FVE volume geometry: corpus-undecoded - marked device-tail fallback
     * (the finding's own open question; BitLockerLib decode pending) */
    const u64 end = disk->geometry().sector_count;
    c.sector_count = end > lba ? end - lba : 1;
    c.size_from_device_end = true;
    return pts;
}

int probe_gpt_header(IStorageDevice* disk, u64 lba, Candidate& c) {
    u8 sec[512];
    if (disk->read_at(lba * disk->geometry().logical_sector, sec, 512) != OK)
        return -100;
    if (std::memcmp(sec, "EFI PART", 8) != 0)
        return -100;
    int pts = 10;
    /* backup-header shape: current_lba points at the primary position */
    const table::GptHeader* h = reinterpret_cast<const table::GptHeader*>(sec);
    if (h->entry_size == 128 && h->entry_count <= 1024 && h->entry_count > 0)
        pts += 20;
    c.sector_count = 34; /* the table region itself */
    c.evidence.push_back({"gpt-header", pts});
    return pts;
}

/* --- S-006 probe families -------------------------------------------------
 *
 * ext2/3/4 is the og's lost-scan family (PartitionRecoveryD CExtXBlockParser,
 * the sole sub-parser of CBlockParserLinux - ctor wiring decoded first-hand:
 * FUN_180001b40 -> FUN_180002b00). Its accept criteria are decoded from the
 * decompile and reproduced EXACTLY here:
 *   - FUN_180002880 (validator): magic 0xEF53 @SB+0x38; s_inodes_count,
 *     s_blocks_count, s_blocks_per_group, s_inodes_per_group all nonzero;
 *     s_first_ino >= 3; group count from blocks
 *     ((blocks_count - first_data_block - 1)/blocks_per_group + 1) == group
 *     count from inodes ((inodes_count - 1)/inodes_per_group + 1);
 *     first_data_block == (block_size == 1024) (the kernel geometry rule).
 *   - FUN_180002b90 (parse): additionally s_block_group_nr == 0 - only
 *     block-group-0 superblocks anchor a partition start; the SB copies at
 *     later block-group boundaries must not surface candidates.
 * The core's ext4::validate_superblock (the ext4SearchDLL FUN_180004710
 * gate family) supplies the shared gates; the two og lost-scan-only gates
 * are applied here.
 *
 * HFS+/Btrfs/ZFS/ReFS are SUPERSET probes: the og's PartitionRecoveryD deep
 * scan carries no parsers for these families (CBlockParseWin32 wires only
 * CFatxBlockParser/CNTFSBlockParser/CExFatBlockParser/BitlockerParser -
 * FUN_180001c50; CBlockParserLinux wires only CExtXBlockParser). The
 * core's engines exist and materialize->probe_family already dispatches
 * them; docs/PARITY-DELTAS.md D-50 records the deliberate superset.
 */

namespace {

inline u16 sb_le16(const u8* p) {
    return u16(u16(p[0]) | (u16(p[1]) << 8));
}
inline u32 sb_le32(const u8* p) {
    return u32(p[0]) | (u32(p[1]) << 8) | (u32(p[2]) << 16) | (u32(p[3]) << 24);
}

} // namespace

int probe_ext(IStorageDevice* disk, u64 lba, WindowScanner& scan, Candidate& c) {
    const u32 ls = disk->geometry().logical_sector;
    /* cheap in-window magic gate first (2 bytes); full SB via window when it
     * fits, direct read at window tails */
    const u8* magic_p = scan.at(lba * u64(ls) + 1024 + 0x38);
    u8 sb_buf[1024];
    const u8* sb = nullptr;
    if (magic_p != nullptr) {
        if (sb_le16(magic_p) != 0xEF53)
            return -100;
        if (scan.at(lba * u64(ls) + 2047) != nullptr) {
            sb = scan.at(lba * u64(ls) + 1024);
        } else if (disk->read_at(lba * u64(ls) + 1024, sb_buf, 1024) == OK) {
            sb = sb_buf;
        }
    } else if (disk->read_at(lba * u64(ls) + 1024, sb_buf, 1024) == OK) {
        if (sb_le16(sb_buf + 0x38) == 0xEF53)
            sb = sb_buf;
    }
    if (sb == nullptr)
        return -100;

    ext4::SuperBlock v;
    if (!ext4::validate_superblock(sb, v) || !v.valid)
        return -100;

    /* og lost-scan gates beyond the engine validator (CExtXBlockParser):
     * s_block_group_nr == 0 (partition-start anchor) */
    if (sb_le16(sb + 0x5A) != 0)
        return -100;
    /* s_first_ino >= 3 */
    if (sb_le32(sb + 0x54) < 3)
        return -100;
    /* the og's two-formula group-count cross-check */
    {
        const u64 gb = v.blocks_count > v.first_data_block
                           ? (v.blocks_count - v.first_data_block - 1) / v.blocks_per_group + 1
                           : 0;
        const u64 gi = v.inodes_count > 0 ? (u64(v.inodes_count) - 1) / v.inodes_per_group + 1 : 0;
        if (gb == 0 || gb != gi)
            return -100;
    }

    int pts = 40; /* magic + full og validation */
    c.evidence.push_back({"ext-sb-magic", 15});
    c.evidence.push_back({"ext-sb-valid", 25});

    /* group-descriptor liveness: the first GDT entry's inode-table pointer
     * (GDT lives at block first_data_block+1 in both block-size layouts) */
    u8 gd[32] = {};
    const u64 gdt_off = lba * u64(ls) + u64(v.first_data_block + 1) * v.block_size;
    if (disk->read_at(gdt_off, gd, 32) == OK && sb_le32(gd + 8) != 0) {
        pts += 30;
        c.evidence.push_back({"ext-gdt-probe", 30});
    }

    c.sector_count = v.blocks_count * v.block_size / ls; /* real volume size */
    return pts;
}

int probe_hfsplus(IStorageDevice* disk, u64 lba, WindowScanner& scan, Candidate& c) {
    const u32 ls = disk->geometry().logical_sector;
    const u8* vh_p = scan.at(lba * u64(ls) + 1024);
    u8 vh_buf[512];
    const u8* vh = nullptr;
    if (vh_p != nullptr && scan.at(lba * u64(ls) + 1535) != nullptr) {
        vh = vh_p;
    } else if (disk->read_at(lba * u64(ls) + 1024, vh_buf, 512) == OK) {
        vh = vh_buf;
    }
    if (vh == nullptr)
        return -100;
    /* "H+" (0x48 0x2B) / "HX" (0x48 0x58) signature */
    if (vh[0] != 0x48 || (vh[1] != 0x2B && vh[1] != 0x58))
        return -100;
    hfsplus::VolumeHeader v;
    if (!hfsplus::validate_volume_header(vh, v) || !v.valid)
        return -100;
    int pts = 35;
    c.evidence.push_back({"hfsplus-vh", 35});
    if (v.total_blocks > 0) {
        pts += 5;
        c.evidence.push_back({"hfsplus-geometry", 5});
        c.sector_count = u64(v.total_blocks) * v.block_size / ls;
    }
    return pts;
}

int probe_btrfs(IStorageDevice* disk, u64 lba, Candidate& c) {
    const u32 ls = disk->geometry().logical_sector;
    u8 sb[4096];
    if (disk->read_at(lba * u64(ls) + 0x10000, sb, 4096) != OK)
        return -100;
    btrfs::SuperBlock v;
    if (!btrfs::validate_superblock(sb, v) || !v.valid)
        return -100;
    /* size: the core's SB model does not decode total_bytes (detection
     * anchor only); the flagged device-tail fallback keeps containment
     * honest, exactly like ZFS/ReFS. The engine derives the true size
     * from the chunk tree when the candidate is scanned. */
    int pts = 35;
    c.evidence.push_back({"btrfs-sb", 35});
    return pts;
}

int probe_zfs(IStorageDevice* disk, u64 lba, Candidate& c) {
    const u32 ls = disk->geometry().logical_sector;
    /* label-0 uberblock ring head (label+128 KiB, probe_family's anchor) */
    u8 ub[64];
    if (disk->read_at(lba * u64(ls) + 0x20000, ub, 64) != OK)
        return -100;
    if (ub[0] != 0x0C || ub[1] != 0xB1 || ub[2] != 0xBA || ub[3] != 0x00)
        return -100;
    int pts = 25;
    c.evidence.push_back({"zfs-uberblock-magic", 25});
    /* liveness: the first ring slot's txg (uberblock layout: magic u64,
     * spa_version u64, txg u64 - a written uberblock always carries txg>=1;
     * an unwritten slot is all-zero and has no magic). Ring slots are
     * 1 KiB apart (ashift-10), so one 64-byte header read is the honest
     * liveness window here. */
    const u64 txg = u64(sb_le32(ub + 16)) | (u64(sb_le32(ub + 20)) << 32);
    if (txg != 0) {
        pts += 10;
        c.evidence.push_back({"zfs-txg-live", 10});
    }
    /* size: labels carry geometry only in the config nvlist (engine-side
     * work); the flagged device-tail fallback keeps containment honest */
    const u64 end = disk->geometry().sector_count;
    c.sector_count = end > lba ? end - lba : 1;
    c.size_from_device_end = true;
    return pts;
}

int probe_refs(IStorageDevice* disk, u64 lba, Candidate& c) {
    const u32 ls = disk->geometry().logical_sector;
    /* the caller verified the DBR OEM "ReFS" + 55AA. Bounded page-signature
     * window (8 MiB, the finding's plan) through the existing diagnostic
     * probe wired onto the discovery path. */
    u64 win = u64(8) * 1024 * 1024;
    const u64 max = disk->size() - lba * u64(ls);
    if (win > max)
        win = max;
    u32 err = 0;
    auto slice = PartitionSlice::create(disk, lba * u64(ls), win, &err);
    if (!slice)
        return -100;
    refs::RefsInfo info;
    if (refs::detect_refs(slice.get(), info) != OK || !info.found)
        return -100;
    int pts = 35;
    c.evidence.push_back({"refs-msb-pages", 35});
    if (info.checkpointed) {
        pts += 5;
        c.evidence.push_back({"refs-checkpointed", 5});
    }
    /* the ReFS volume size lives in the MSB chain (engine-side); the DBR
     * carries no reliable length - flagged device-tail fallback */
    const u64 end = disk->geometry().sector_count;
    c.sector_count = end > lba ? end - lba : 1;
    c.size_from_device_end = true;
    return pts;
}

} // namespace

Overlap classify_overlap(u64 start, u64 count, const std::vector<table::PartitionInfo>& known) {
    Overlap worst = Overlap::None;
    const u64 end = start + count;
    for (const auto& p : known) {
        const u64 pe = p.start_lba + p.sector_count;
        if (end <= p.start_lba || start >= pe)
            continue;
        const bool inside = start >= p.start_lba && end <= pe;
        const bool contains = start <= p.start_lba && end >= pe;
        if (inside)
            return Overlap::Inside;
        if (contains)
            worst = Overlap::Contains;
        else if (worst == Overlap::None)
            worst = Overlap::Overlaps;
    }
    return worst;
}

std::unique_ptr<PartitionSlice> materialize(IStorageDevice* disk, const Candidate& c) {
    const u32 ls = disk->geometry().logical_sector;
    u64 bytes = c.sector_count * ls;
    const u64 max = disk->size() - c.start_lba * ls;
    if (bytes > max)
        bytes = max; /* clamp ragged tails to the device */
    u32 err = 0;
    return PartitionSlice::create(disk, c.start_lba * ls, bytes, &err);
}

u32 discover(IStorageDevice* disk, const std::vector<table::PartitionInfo>& known,
             u32 stride_sectors, CancelToken* cancel, bool (*progress)(u64, u64, void*),
             void* progress_user, DiscoveryResult& out) {
    if (disk == nullptr || disk->geometry().logical_sector == 0)
        return ERR_BAD_PARAMETER;
    if (stride_sectors == 0)
        stride_sectors = 1;
    const u32 ls = disk->geometry().logical_sector;
    const u64 total = disk->size();

    WindowScanner scan(disk);
    std::vector<Candidate> found;
    u64 sectors_scanned = 0;
    u64 read_errors = 0;

    /* pass 1: every stride boundary, check the last two bytes for 55 AA and
     * the 8-byte OEM region - one window fetch covers 128 sectors */
    u64 win_bytes = 64 * 1024;
    for (u64 base = 0; base < total; base += win_bytes) {
        if (cancel && cancel->checkpoint())
            return ERR_CANCELLED;
        if (progress && !progress(base, total, progress_user))
            return ERR_CANCELLED;
        if (!scan.fetch(base)) {
            ++read_errors;
            continue;
        }
        const u64 win_len = scan.window_len();
        sectors_scanned += win_len / ls;
        /* probe each sector within the window */
        const u64 mib_lbas = (1024 * 1024) / ls > 0 ? (1024 * 1024) / ls : 1;
        for (u64 off = 0; off + 512 <= win_len; off += u64(stride_sectors) * ls) {
            const u8* sec = scan.at(base + off);
            if (sec == nullptr)
                break;
            const u64 lba = (base + off) / ls;

            Candidate c;
            c.start_lba = lba;
            int pts = -1;

            /* pass 1a: boot-sector families (55AA-gated) */
            if (sec[510] == 0x55 && sec[511] == 0xAA) {
                char oem[9] = {0};
                std::memcpy(oem, sec + 3, 8);
                if (std::strncmp(oem, "NTFS    ", 8) == 0) {
                    c.fs = CandidateFs::Ntfs;
                    pts = probe_ntfs(disk, lba, c);
                } else if (std::strncmp(oem, "EXFAT   ", 8) == 0) {
                    c.fs = CandidateFs::Exfat;
                    pts = probe_exfat(disk, lba, c);
                } else if (std::memcmp(sec, "EFI PART", 8) == 0) {
                    c.fs = CandidateFs::GptTable;
                    pts = probe_gpt_header(disk, lba, c);
                } else if (std::memcmp(oem, "ReFS", 4) == 0) {
                    /* S-006: the ReFS DBR (OEM "ReFS", probe_family's gate)
                     * fell into the FAT catch-all and was dropped as noise
                     * before. Detected via the page-signature window probe. */
                    c.fs = CandidateFs::Refs;
                    pts = probe_refs(disk, lba, c);
                } else if (std::memcmp(oem, "-FVE-FS-", 8) == 0) {
                    /* S-019: BitLocker VBR (the og's type-0xB classifier
                     * gate, PartitionRecoveryD_decompiled.c:2778-2785) -
                     * must run BEFORE the FAT catch-all or the FVE BPB-less
                     * boot sector dies in fat::validate_boot. */
                    c.fs = CandidateFs::BitLocker;
                    pts = probe_bitlocker(disk, lba, c);
                } else {
                    /* FAT family: validate via the BPB parser (the catch-all
                     * stays LAST - it is the noisiest family) */
                    c.fs = CandidateFs::Fat;
                    pts = probe_fat(disk, lba, c);
                }
            }

            /* pass 1b: superblock/volume-header families (no 55AA on a
             * plain Linux/Mac data partition - and a bootable ext4 carries
             * 55AA but no FAT BPB, so these run whenever the boot-sector
             * families did not claim the sector). ext/HFS+ anchors are
             * in-window (+1024); btrfs (+64 KiB) and ZFS (+128 KiB) anchors
             * need out-of-window reads and are gated on 1 MiB grid alignment
             * (the standard partition-start alignment) to bound scan cost -
             * documented in PARITY-DELTAS D-50. */
            if (pts < 0) {
                c.fs = CandidateFs::Ext2;
                pts = probe_ext(disk, lba, scan, c);
            }
            if (pts < 0) {
                c.fs = CandidateFs::HfsPlus;
                pts = probe_hfsplus(disk, lba, scan, c);
            }
            if (pts < 0 && lba % mib_lbas == 0) {
                c.fs = CandidateFs::Btrfs;
                pts = probe_btrfs(disk, lba, c);
            }
            if (pts < 0 && lba % mib_lbas == 0) {
                c.fs = CandidateFs::Zfs;
                pts = probe_zfs(disk, lba, c);
            }
            if (pts < 0)
                continue; /* no family accepted: noise */
            if (c.sector_count == 0) {
                /* no probe set a size: bound by the device tail (audit row
                 * 31b - the old fabricated 2048 hid real geometry).
                 * GAP-S6-6: tail fallback - marked, never containment
                 * evidence */
                c.sector_count =
                    disk->size() / ls > c.start_lba ? disk->size() / ls - c.start_lba : 1;
                c.size_from_device_end = true;
            }
            /* bound by device tail */
            if (c.start_lba + c.sector_count > disk->size() / ls)
                c.sector_count = disk->size() / ls - c.start_lba;
            /* N-035: probes that do not set a data-area extent are the
             * DataAreaStart == StartSec shape (NTFS/exFAT/others per the
             * corpus log); the tail fallback inherits the same
             * normalization (its containment is already excluded by
             * GAP-S6-6's flag). */
            if (c.data_sector_count == 0) {
                c.data_start_lba = c.start_lba;
                c.data_sector_count = c.sector_count;
            }
            if (c.data_start_lba + c.data_sector_count > c.start_lba + c.sector_count) {
                /* the data extent can never exceed the volume extent */
                c.data_start_lba = c.start_lba;
                c.data_sector_count = c.sector_count;
            }
            c.score = pts;
            found.push_back(c);
        }
    }

    /* pass 2: dedup + overlap scoring */
    std::vector<Candidate> keep;
    for (size_t i = 0; i < found.size(); ++i) {
        bool dup = false;
        for (size_t j = 0; j < keep.size(); ++j) {
            /* N-035: containment runs on the DATA-AREA extent (the og's
             * EndSec = DataAreaStart + totalSec math) - a peer whose boot
             * sector sits inside the outer's reserved+FATs region is NOT
             * contained: the metadata gap spans it, the data area does
             * not. */
            if (found[i].fs == keep[j].fs &&
                found[i].start_lba >= keep[j].data_start_lba &&
                found[i].start_lba <
                    keep[j].data_start_lba + keep[j].data_sector_count) {
                /* GAP-S6-6: a fallback-sized outer candidate spans to disk
                 * end BY CONSTRUCTION - its "containment" of a later peer
                 * is an artifact, not evidence of nesting. Only probe-
                 * derived sizes may claim containment. */
                if (keep[j].size_from_device_end)
                    continue;
                dup = true; /* same fs, nested: the outer/earlier wins */
                break;
            }
        }
        if (!dup)
            keep.push_back(found[i]);
    }
    for (auto& c : keep) {
        /* N-035: the og's containment/overlap rules operate on the
         * data-area extent (its 16-MiB size-match and containment-drop
         * both; the QA log's totalSec is the data-area size) */
        Overlap ov = classify_overlap(c.data_start_lba, c.data_sector_count, known);
        switch (ov) {
        case Overlap::Inside:
            c.score -= 20; /* a filesystem inside a live partition: damaged
                              inner volume - still reportable, lower score */
            break;
        case Overlap::Overlaps:
        case Overlap::Contains:
            c.score -= 10;
            break;
        case Overlap::None:
        default:
            break;
        }
    }
    std::sort(keep.begin(), keep.end(),
              [](const Candidate& a, const Candidate& b) { return a.score > b.score; });
    out.candidates = std::move(keep);
    out.sectors_scanned = sectors_scanned;
    out.read_errors = read_errors;
    if (progress)
        (void)progress(total, total, progress_user);
    return OK;
}

} // namespace lost
} // namespace cr
