/* cr_candidates.cpp - lost partition discovery + scoring (R3.1).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/lost/cr_lostpart.h"
#include "cr/cr_log.h"
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
#include "cr/refs/cr_refs_engine.h"
#include "cr/zfs/cr_zfs_engine.h"
#include "cr/tree/cr_node.h"
#include "cr/ntfs/cr_mftwalk.h"
#include <algorithm>
#include <cmath>
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
        /* N-206: the addressable bound is the CEILED sector_count*ss (the
         * og's driver-padded final partial sector) - clipping to the raw
         * byte size shrank the tail window below one probeable sector,
         * so a signature anchored in the partial tail was never probed. */
        const u64 bound = geom_.sector_count * geom_.logical_sector;
        u64 avail = bound > win_base_ ? bound - win_base_ : 0;
        u64 take = avail < win ? avail : win;
        if (take == 0)
            return false;
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
    /* M-042: per-record geometry evidence (the og's CSearchedPartitionInfo
     * bps/SPC/hidden payload fields) */
    c.bytes_per_sector = u32(bs->bytes_per_sector);
    c.sectors_per_cluster = u32(bs->sectors_per_cluster);
    c.hidden_sectors = bs->hidden_sectors;
    c.have_geometry = 1;

    ntfs::VolumeParams vp;
    if (ntfs::validate_boot_sector(*bs, disk->geometry(), vp) == OK) {
        pts += 25;
        /* N-200 (diff-to-fix): the og's lost-scan NTFS acceptance requires
         * POSITIVE MFT evidence — the classifier FUN_180011150
         * (PartitionRecoveryD :9889-9906) rejects into the 0xfffffff1 slot
         * when FUN_180011560's record read validates NEITHER at the
         * primary $MFT LCN NOR at the $MFTMirr retry (:10023-10075: the
         * primary 4-record read, FUN_180011440 validation, then the
         * identical read re-issued at FUN_180011b30's +0x38 mirror LCN).
         * The record-0 probe is now a soft requirement: primary first,
         * mirror retry, reject only when BOTH fail. The mirror acceptance
         * is evidence-labeled (mft-mirror-probe) and scores identically —
         * the og's FUN_180011560 return does not distinguish the leg. */
        bool mft_evidence = false;
        bool from_mirror = false;
        std::vector<u8> rec;
        rec.resize(size_t(vp.record_size));
        const u64 vol_base = boot_lba * disk->geometry().logical_sector;
        /* primary $MFT probe (volume-relative -> absolute on the disk) */
        u64 off = vol_base + vp.mft_lcn * vp.cluster_bytes;
        if (disk->read_at(off, rec.data(), u32(rec.size())) == OK) {
            ntfs::RecordHeader rh;
            std::memcpy(&rh, rec.data(), sizeof(rh));
            if (rh.magic == ntfs::RECORD_MAGIC_FILE &&
                ntfs::fixup_apply(rec.data(), u32(rec.size()), rh.update_sequence_offset,
                                  rh.update_sequence_size, vp.sector_bytes) == OK) {
                mft_evidence = true;
            }
        }
        if (!mft_evidence) {
            /* the og's mirror retry: the same record-0 shape at the
             * $MFTMirr LCN (the og bounds neither LCN — a failed read
             * simply fails; read_at enforces the device bound) */
            const u64 mirr_off = vol_base + vp.mftmirr_lcn * vp.cluster_bytes;
            if (disk->read_at(mirr_off, rec.data(), u32(rec.size())) == OK) {
                ntfs::RecordHeader rh;
                std::memcpy(&rh, rec.data(), sizeof(rh));
                if (rh.magic == ntfs::RECORD_MAGIC_FILE &&
                    ntfs::fixup_apply(rec.data(), u32(rec.size()), rh.update_sequence_offset,
                                      rh.update_sequence_size, vp.sector_bytes) == OK) {
                    mft_evidence = true;
                    from_mirror = true;
                }
            }
        }
        if (!mft_evidence)
            return -100; /* the og's reject slot: no positive MFT evidence */
        pts += 30;       /* $MFT alive (primary or mirror): very strong */
        c.evidence.push_back({from_mirror ? "mft-mirror-probe" : "mft-probe", 30});
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
    /* M-042: per-record geometry evidence (BPB fields, the og's payload).
     * hidden_sectors is not exposed in FatParams - read the BPB position
     * directly (u32 LE at +0x1C, the DOS 3.31 BPB layout). */
    c.bytes_per_sector = p.bytes_per_sector;
    c.sectors_per_cluster = p.sectors_per_cluster;
    c.hidden_sectors =
        u32(sec[0x1C]) | (u32(sec[0x1D]) << 8) | (u32(sec[0x1E]) << 16) | (u32(sec[0x1F]) << 24);
    c.have_geometry = 1;
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
    /* M-137: the FAT12/16 boot-sector volume label (+0x2B, 11 bytes,
     * space-padded - the old-style label only the 12/16 flavors carry;
     * FAT32 zeroes the slot). Captured here so the naming template's
     * volume-label-keep rule ("%s (%s)" with the label, the ":)"-bearing
     * labels kept verbatim - DRWUI:634817-838) has its on-disk source;
     * validation: at least one printable non-space byte. */
    if (flavor == 1 || flavor == 2) {
        bool printable = false;
        for (int k = 0; k < 11; ++k) {
            const u8 ch = sec[0x2B + k];
            if (ch == 0)
                break;
            if (ch >= 0x20 && ch < 0x7F) {
                if (ch != 0x20)
                    printable = true;
            } else {
                printable = false;
                break;
            }
        }
        if (printable) {
            int k = 0;
            for (; k < 11 && sec[0x2B + k]; ++k)
                c.volume_label[k] = char(sec[0x2B + k]);
            /* right-trim spaces (the padding) */
            while (k > 0 && c.volume_label[k - 1] == ' ')
                --k;
            c.volume_label[k] = 0;
        }
    }
    c.sector_count = p.total_bytes / p.bytes_per_sector;
    /* N-035: FAT is the metadata-gap family - the data area begins at
     * first_data_sector (reserved + two FATs [+ root dir on FAT12/16]
     * ahead of it; the og's DataAreaStart). totalSec is data-area-sized
     * (EndSec = DataAreaStart + totalSec in the og's dump). */
    c.data_start_lba = boot_lba + p.first_data_sector;
    c.data_sector_count = c.sector_count > p.first_data_sector
                              ? c.sector_count - p.first_data_sector
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
    /* M-042: per-record geometry evidence (the exFAT BPB carries bps/SPC
     * in the same BPB positions as FAT) */
    c.bytes_per_sector = 1u << (bs->bytes_per_sector_shift & 0x1F);
    c.sectors_per_cluster = 1u << (bs->sectors_per_cluster_shift & 0x1F);
    c.have_geometry = 1;
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

bool revalidate_candidate(IStorageDevice* disk, Candidate& c) {
    /* M-042: the og's resume re-validation (PRD:20748-20842) — at search
     * start, every persisted lostpart record is re-read at its start LBA
     * through the device and re-validated through the scan-loop classifier
     * (FUN_180021080); records whose {start, end, type} no longer match
     * the CURRENT disk are dropped before they reach the UI. Our identity:
     * the record's start still classifies as the persisted family, plus
     * the persisted BPB geometry when the record carries it (the count
     * check rides the device size — a shrunken/re-grown disk re-derives
     * sizes from the live probes). Unknown-family records cannot be
     * re-probed (no classifier to contradict them): kept, matching the
     * og's applied-flag skip for unprobeable records.
     *
     * N-203: the candidate is MUTABLE — on a successful probe of a record
     * that carried no data-area extent (a v1..v3 session, or a probe
     * family that never sets one), the scratch's re-derived data-area
     * fields copy back into the candidate. The og's record replays with
     * the full extent identity by construction (the 0x448 image is
     * flattened and re-applied whole); the copy-back is the core's
     * equivalent repair for records saved before the v4 tail existed —
     * once enriched, a re-save persists the extent (idempotent). */
    if (disk == nullptr || disk->geometry().logical_sector == 0)
        return true; /* no arbiter: never drop on our own absence */
    const u64 ls = disk->geometry().logical_sector;
    if (c.start_lba * ls >= disk->size())
        return false; /* the record starts past the disk: stale by
                         construction (the og's start-LBA match) */
    Candidate scratch;
    scratch.start_lba = c.start_lba;
    int pts = -1;
    switch (c.fs) {
    case CandidateFs::Ntfs:
        pts = probe_ntfs(disk, c.start_lba, scratch);
        break;
    case CandidateFs::Fat:
        pts = probe_fat(disk, c.start_lba, scratch);
        break;
    case CandidateFs::Exfat:
        pts = probe_exfat(disk, c.start_lba, scratch);
        break;
    case CandidateFs::GptTable:
        pts = probe_gpt_header(disk, c.start_lba, scratch);
        break;
    case CandidateFs::Refs:
        pts = probe_refs(disk, c.start_lba, scratch);
        break;
    case CandidateFs::BitLocker:
        pts = probe_bitlocker(disk, c.start_lba, scratch);
        break;
    case CandidateFs::Btrfs:
        pts = probe_btrfs(disk, c.start_lba, scratch);
        break;
    case CandidateFs::Zfs:
        pts = probe_zfs(disk, c.start_lba, scratch);
        break;
    case CandidateFs::Ext2: {
        WindowScanner scan(disk);
        pts = probe_ext(disk, c.start_lba, scan, scratch);
        break;
    }
    case CandidateFs::HfsPlus: {
        WindowScanner scan(disk);
        pts = probe_hfsplus(disk, c.start_lba, scan, scratch);
        break;
    }
    case CandidateFs::Unknown:
        return true; /* no probe family to contradict the record */
    }
    if (pts < 0)
        return false; /* the current disk no longer classifies the start
                         sector as the persisted family: stale, drop */
    if (c.have_geometry && scratch.have_geometry && c.bytes_per_sector != scratch.bytes_per_sector)
        return false; /* geometry identity mismatch (the og's per-record
                         payload check — a re-formatted region with the
                         same OEM at the same LBA is still a different
                         volume) */
    if (c.data_sector_count == 0 && scratch.data_sector_count != 0) {
        /* N-203: the record predates the v4 tail (or its probe family
         * never anchored a data area) — the live probe re-derived one;
         * adopt it so the dedup/containment/leader consumers see the
         * same window a fresh run would have produced. */
        c.data_start_lba = scratch.data_start_lba;
        c.data_sector_count = scratch.data_sector_count;
    }
    return true;
}
/* ---------------------------------------------------------------- */
/* M-017: the og's content-driven scoring model                      */
/* ---------------------------------------------------------------- */

/* DRWUI FS codes (FUN_14050fda0's switch input). The core's CandidateFs
 * maps: Fat -> 1, HfsPlus -> 8, everything else -> the NTFS-family
 * default branch (files + 50*folders). */
int og_fs_code(CandidateFs fs) {
    switch (fs) {
    case CandidateFs::Fat:
        return 1;
    case CandidateFs::HfsPlus:
        return 8;
    default:
        return 3; /* NTFS family (any non-FAT non-HFS code) */
    }
}

u64 weighted_count(int og_fs_code, u64 files, u64 folders) {
    /* FUN_14050fda0 :842176-842200: NTFS family -> files + 50*folders;
     * FS 8 (HFS+) -> files + 10*folders; FS 1/2 (FAT) -> (f+d)*50 */
    if (og_fs_code == 8)
        return files + 10 * folders;
    if (og_fs_code == 1 || og_fs_code == 2)
        return (files + folders) * 50;
    return files + 50 * folders;
}

double score_ratio(u64 count, u64 max_count) {
    /* FUN_14050fda0 :842226-274: num = count > 1 ? ln(count) : 1.0
     * (the count<=1 identity quirk - a 1-file candidate outranks every
     * 2..6-file candidate when max > e^2); denom = max > 1 ? ln(max)
     * : 1.0; score = num/denom */
    const double num = count > 1 ? std::log(double(count)) : 1.0;
    const double den = max_count > 1 ? std::log(double(max_count)) : 1.0;
    if (den <= 0.0)
        return 1.0;
    return num / den;
}

namespace {

/* count surfaced nodes (files vs folders) under a root */
void count_tree(tree::FileNode* n, u64& files, u64& folders) {
    for (tree::FileNode* c = n->first_child(); c; c = c->next_sibling()) {
        if (!c->props())
            continue;
        if (c->props()->attributes & 0x10)
            ++folders;
        else
            ++files;
        count_tree(c, files, folders);
    }
}

/* bounded content probe: materialize the candidate and run the family's
 * engine scan over a scratch tree; the counts feed the og's weighted
 * count. A progress-call budget bounds hostile/pathological volumes
 * (each engine calls progress per file/block; past the budget the walk
 * stops and the partial counts stand - the og's own walk is unbounded,
 * this is the registered safety valve). */
constexpr u64 CONTENT_PROBE_BUDGET = 200000;

void content_counts(IStorageDevice* disk, const Candidate& c, CancelToken* cancel, u64& files,
                    u64& folders) {
    files = 0;
    folders = 0;
    if (c.fs == CandidateFs::BitLocker || c.fs == CandidateFs::GptTable ||
        c.fs == CandidateFs::Unknown)
        return; /* encrypted/table families: no walkable content model */
    auto slice = materialize(disk, c);
    if (!slice)
        return;
    tree::ResultTree tree;
    auto* ra = tree.add_root(u"probe");
    if (!ra)
        return;
    auto* rd = tree.add_root(u"d");
    u64 calls = 0;
    auto budget = [](u64, u64, void* user) {
        auto* n = static_cast<u64*>(user);
        return ++*n < CONTENT_PROBE_BUDGET;
    };
    u32 rc = OK;
    switch (c.fs) {
    case CandidateFs::Ntfs: {
        u32 open_rc = OK;
        auto vol = ntfs::NtfsVolume::open(
            std::unique_ptr<IStorageDevice>(SliceShim::shim(slice.get())), &open_rc);
        if (vol) {
            ntfs::WalkerFlags wf;
            ntfs::WalkStats ws;
            rc = ntfs::walk_mft(vol.get(), nullptr, wf, cancel, budget, &calls, tree, ra, rd,
                                nullptr, ws);
        }
        break;
    }
    case CandidateFs::Fat: {
        fat::FatScanFlags fl;
        fat::FatScanStats st;
        rc = fat::scan_fat(slice.get(), fl, cancel, budget, &calls, tree, ra, rd, nullptr, st,
                           nullptr);
        break;
    }
    case CandidateFs::Exfat: {
        exfat::ExfatScanFlags fl;
        exfat::ExfatScanStats st;
        rc = exfat::scan_exfat(slice.get(), fl, cancel, budget, &calls, tree, ra, rd, nullptr, st,
                               nullptr);
        break;
    }
    case CandidateFs::Ext2: {
        ext4::Ext4ScanFlags fl;
        ext4::Ext4ScanStats st;
        rc = ext4::scan_ext4(slice.get(), fl, cancel, budget, &calls, tree, ra, rd, nullptr, st);
        break;
    }
    case CandidateFs::HfsPlus: {
        hfsplus::HfsScanFlags fl;
        hfsplus::HfsScanStats st;
        rc = hfsplus::scan_hfsplus(slice.get(), fl, cancel, budget, &calls, tree, ra, rd, nullptr,
                                   st);
        break;
    }
    case CandidateFs::Btrfs: {
        btrfs::BtrfsScanStats st;
        rc = btrfs::scan_btrfs(slice.get(), cancel, budget, &calls, tree, ra, rd, nullptr, st);
        break;
    }
    case CandidateFs::Zfs: {
        zfs::ZfsScanStats st;
        rc = zfs::scan_zfs(slice.get(), cancel, budget, &calls, tree, ra, rd, nullptr, st);
        break;
    }
    case CandidateFs::Refs: {
        refs::RefsScanFlags fl;
        refs::RefsScanStats st;
        rc = refs::scan_refs(slice.get(), fl, cancel, budget, &calls, tree, ra, rd, nullptr, st);
        break;
    }
    default:
        return;
    }
    (void)rc; /* partial walks (budget/cancel) still count */
    count_tree(ra, files, folders);
    count_tree(rd, files, folders);
}

/* the per-FS bonus the SearchDLLs supply at info+0x68 (observed values:
 * NTFS 60/30, FAT32 20 - OQ-S2M-2 tracks the setter decode; the NTFS
 * accumulator family is census-decoded: +80 system files, +50 boot-SPC
 * validated, -10 read failure, +30). The core's analog: the healthy-
 * probe bonus for the family's strong evidence, the degraded value when
 * the structural probe was partial. */
u64 fs_bonus(const Candidate& c) {
    switch (c.fs) {
    case CandidateFs::Ntfs:
        /* observed 60 (all probes strong) / 30 (partial). N-200: the
         * mirror-validated leg counts strong — the og's FUN_180011560
         * return does not distinguish primary from mirror acceptance. */
        for (const auto& e : c.evidence)
            if (c.evidence.size() >= 3 && (std::string_view(e.tag) == "mft-probe" ||
                                           std::string_view(e.tag) == "mft-mirror-probe"))
                return 60;
        return 30;
    case CandidateFs::Fat:
        return 20; /* observed FAT32 */
    default:
        return 0; /* unpinned (OQ-S2M-2) */
    }
}

/* "Found once partition:%I64d": the candidate's start matches a live
 * table entry (DRWUI :842260-66). The core's old model INVERTED this
 * polarity (overlap penalties -10/-20 at the same decision point). */
bool table_match(const Candidate& c, const std::vector<table::PartitionInfo>& known) {
    for (const auto& p : known) {
        if (!p.used)
            continue;
        if (p.start_lba == c.start_lba)
            return true;
    }
    return false;
}

/* M-137: the geometry-fallback device shim - wraps a device whose
 * sector-size query failed (logical_sector 0) with the 512-byte view
 * (DRWUI:904185-187: `if (sector_size == 0) sector_size = 0x200;`).
 * Every probe reads geometry() directly for its LBA math, so the
 * fallback must present the corrected geometry, not patch one local. */
class FallbackGeomDevice final : public IStorageDevice {
public:
    FallbackGeomDevice(IStorageDevice* inner) : inner_(inner) {
        geom_ = inner->geometry();
        geom_.logical_sector = 512;
    }
    u32 read_at(u64 off, void* buf, u32 len) override { return inner_->read_at(off, buf, len); }
    const Geometry& geometry() const override { return geom_; }

private:
    IStorageDevice* inner_;
    Geometry geom_;
};

} // namespace

/* M-118: the og's per-subkey "Data" blob decode (FUN_18000f5b0,
 * PartitionRecoveryD_decompiled.c:8841-8929, re-verified first-hand this
 * session against the corpus decompile AND the DRWUI-side mirror
 * FUN_1405895d0 :922335+). The blob is the Explorer shell's volume
 * mount-point cache; the og reads exactly one placement field from it:
 * the 0x20-char ASCII-hex byte offset at data+0xc6, delimited by '#'
 * markers at data[0xc4] and data[0xe6]. Gates, in the og's order:
 * REG_BINARY (type checked by the callers), size > 0xe7, the two '#'
 * delimiters, the %I64X parse (case-insensitive hex, stops at the first
 * non-hex char), byte_start % bps == 0, then the /bps conversion. */
bool decode_mountpoints2_placement(const u8* data, size_t size, u32 bytes_per_sector,
                                   u64& start_lba_out) {
    start_lba_out = 0;
    if (data == nullptr || size <= 0xe7)
        return false; /* the og's 0xe7 < size gate (231) */
    if (data[0xc4] != '#' || data[0xe6] != '#')
        return false; /* the field delimiters */
    if (bytes_per_sector == 0)
        return false; /* the og divides by the device's bps unchecked;
                       * the core refuses instead of trapping */
    /* the 0x20-char hex field at +0xc6, %I64X semantics: case-
     * insensitive, leading whitespace/hex digits consumed, stops at the
     * first non-hex character; overflow saturates like the og's
     * 64-bit accumulate (sscanf %I64X wraps at 2^64 in MSVC) */
    u64 val = 0;
    size_t consumed = 0;
    for (size_t i = 0; i < 0x20 && i + 0xc6 < size; ++i) {
        const char ch = char(data[0xc6 + i]);
        int nibble = -1;
        if (ch >= '0' && ch <= '9')
            nibble = ch - '0';
        else if (ch >= 'a' && ch <= 'f')
            nibble = ch - 'a' + 10;
        else if (ch >= 'A' && ch <= 'F')
            nibble = ch - 'A' + 10;
        if (nibble < 0)
            break; /* %X stops at the first non-hex char */
        val = (val << 4) | u64(nibble);
        ++consumed;
    }
    if (consumed == 0)
        return false; /* %I64X matched nothing -> the og's local stays 0
                       * and the bps gate below rejects it for any
                       * non-zero-aligned... a 0 parse also fails the
                       * caller's nonzero-return check; same outcome */
    if (val % bytes_per_sector != 0)
        return false; /* the og's alignment gate */
    start_lba_out = val / bytes_per_sector;
    return start_lba_out != 0; /* FUN_18000f840's caller checks the
                                * return != 0: sector 0 (the MBR area) is
                                * the og's failure sentinel, never a
                                * proposal */
}

u32 discover(IStorageDevice* disk, const std::vector<table::PartitionInfo>& known,
             u32 stride_sectors, CancelToken* cancel, bool (*progress)(u64, u64, void*),
             void* progress_user, DiscoveryResult& out, const std::vector<u64>& register_starts) {
    /* M-137 (diff-to-fix): the og's geometry fallback (DRWUI:904185-187:
     * `if (sector_size == 0) sector_size = 0x200;` - a failed device
     * sector-size query proceeds at 512 rather than refusing). The old
     * core refused the whole discovery with ERR_BAD_PARAMETER; the
     * honest shape is the Warn log + the 512-byte shim (the discovery
     * is LBA-math driven and 512 is the universal floor). */
    if (disk == nullptr)
        return ERR_BAD_PARAMETER;
    FallbackGeomDevice fallback_disk(disk);
    IStorageDevice* effective = disk;
    if (disk->geometry().logical_sector == 0) {
        if (cr::log::enabled())
            cr::log::write(cr::log::Level::Warn,
                           "lost discovery: device sector size unreadable (0) - "
                           "proceeding at the 512-byte fallback");
        effective = &fallback_disk;
    }
    if (stride_sectors == 0)
        stride_sectors = 1;
    const u32 ls = effective->geometry().logical_sector;
    const u64 total = effective->size();

    WindowScanner scan(effective);
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
                    pts = probe_ntfs(effective, lba, c);
                } else if (std::strncmp(oem, "EXFAT   ", 8) == 0) {
                    c.fs = CandidateFs::Exfat;
                    pts = probe_exfat(effective, lba, c);
                } else if (std::memcmp(sec, "EFI PART", 8) == 0) {
                    c.fs = CandidateFs::GptTable;
                    pts = probe_gpt_header(effective, lba, c);
                } else if (std::memcmp(oem, "ReFS", 4) == 0) {
                    /* S-006: the ReFS DBR (the classifier's "ReFS" OEM;
                     * probe_family now carries the og's second signature
                     * "FSRS"@+0x10 - the lost-side probe's own content proof
                     * is the page-signature window, deeper than both) fell
                     * into the FAT catch-all and was dropped as noise
                     * before. Detected via the page-signature window probe. */
                    c.fs = CandidateFs::Refs;
                    pts = probe_refs(effective, lba, c);
                } else if (std::memcmp(oem, "-FVE-FS-", 8) == 0) {
                    /* S-019: BitLocker VBR (the og's type-0xB classifier
                     * gate, PartitionRecoveryD_decompiled.c:2778-2785) -
                     * must run BEFORE the FAT catch-all or the FVE BPB-less
                     * boot sector dies in fat::validate_boot. */
                    c.fs = CandidateFs::BitLocker;
                    pts = probe_bitlocker(effective, lba, c);
                } else {
                    /* FAT family: validate via the BPB parser (the catch-all
                     * stays LAST - it is the noisiest family) */
                    c.fs = CandidateFs::Fat;
                    pts = probe_fat(effective, lba, c);
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
                pts = probe_ext(effective, lba, scan, c);
            }
            if (pts < 0) {
                c.fs = CandidateFs::HfsPlus;
                pts = probe_hfsplus(effective, lba, scan, c);
            }
            if (pts < 0 && lba % mib_lbas == 0) {
                c.fs = CandidateFs::Btrfs;
                pts = probe_btrfs(effective, lba, c);
            }
            if (pts < 0 && lba % mib_lbas == 0) {
                c.fs = CandidateFs::Zfs;
                pts = probe_zfs(effective, lba, c);
            }
            if (pts < 0)
                continue; /* no family accepted: noise */
            if (c.sector_count == 0) {
                /* no probe set a size: bound by the device tail (audit row
                 * 31b - the old fabricated 2048 hid real geometry).
                 * GAP-S6-6: tail fallback - marked, never containment
                 * evidence. N-206: the tail bound rides the CEILED sector
                 * count (the og's geometry-EX form) so a candidate anchored
                 * in the final partial sector keeps its size. */
                const u64 dev_sectors = disk->geometry().sector_count
                                            ? disk->geometry().sector_count
                                            : disk->size() / ls;
                c.sector_count = dev_sectors > c.start_lba ? u32(dev_sectors - c.start_lba) : 1;
                c.size_from_device_end = true;
            }
            /* bound by device tail (N-206: the ceiled count) */
            const u64 dev_tail =
                disk->geometry().sector_count ? disk->geometry().sector_count : disk->size() / ls;
            if (c.start_lba + c.sector_count > dev_tail)
                c.sector_count = u32(dev_tail > c.start_lba ? dev_tail - c.start_lba : 0);
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
            c.structural_points = pts; /* M-017: the primary score is the
                                        * content model; points tie-break */
            found.push_back(c);
        }
    }

    /* pass 1r (M-118): the register channel - the og's third candidate
     * source. Each remembered placement (Explorer MountPoints2\\CPC\\Volume
     * history, host-fed as sector LBAs) is probed with the SAME format
     * chain as the sweep: the og's DRWUI-side FUN_140589e80 reads 0x800
     * sectors at the remembered start and runs its registered parser
     * chain over the buffer (FUN_14058a070 - any parser accepting the
     * content admits the placement); the core's probe dispatch is that
     * same chain. Placements whose probe accepts merge into `found`
     * upstream of the pass-2 dedup - the og's real-run shape (mbr=1 +
     * filesystem=1 + register=3 -> ONE partition scanned once, the
     * three-source overlap collapsing in the container insertion
     * FUN_1800108f0 exactly like our dedup). A placement the sweep
     * already found at the same start is the same-fs/same-start dup and
     * drops here; a placement the sweep MISSED (stride-skipped anchor,
     * destroyed table entry with intact boot sector) survives as the
     * register-provenance candidate - the channel's unique value. */
    if (!register_starts.empty()) {
        /* dedupe the history itself first: the real log shows THREE
         * register records for ONE placement (multiple mount records per
         * volume GUID) - the og's container dedup collapses them; the
         * core collapses before probing (one probe per placement) */
        std::vector<u64> unique_starts;
        unique_starts.reserve(register_starts.size());
        for (u64 lba : register_starts) {
            if (std::find(unique_starts.begin(), unique_starts.end(), lba) == unique_starts.end())
                unique_starts.push_back(lba);
        }
        for (u64 lba : unique_starts) {
            if (cancel && cancel->checkpoint())
                return ERR_CANCELLED;
            const u64 dev_sectors = effective->geometry().sector_count
                                        ? effective->geometry().sector_count
                                        : effective->size() / ls;
            if (lba == 0 || lba + 1 > dev_sectors)
                continue; /* the og's bounds: FUN_140589e80 drops a
                           * placement outside the current disk's extent
                           * (its start+len vs placement LBA compare);
                           * sector 0 is the MBR area, never a proposal */
            /* the probe chain, sweep-shaped: 55AA families first, then
             * the superblock families */
            u8 sec[512];
            if (effective->read_at(lba * ls, sec, 512) != OK)
                continue; /* unreadable start: the og's 0x800-sector read
                           * failing skips the record the same way */
            Candidate c;
            c.start_lba = lba;
            c.provenance = u8(Provenance::Register);
            int pts = -1;
            if (sec[510] == 0x55 && sec[511] == 0xAA) {
                char oem[9] = {0};
                std::memcpy(oem, sec + 3, 8);
                if (std::strncmp(oem, "NTFS    ", 8) == 0) {
                    c.fs = CandidateFs::Ntfs;
                    pts = probe_ntfs(effective, lba, c);
                } else if (std::strncmp(oem, "EXFAT   ", 8) == 0) {
                    c.fs = CandidateFs::Exfat;
                    pts = probe_exfat(effective, lba, c);
                } else if (std::memcmp(sec, "EFI PART", 8) == 0) {
                    c.fs = CandidateFs::GptTable;
                    pts = probe_gpt_header(effective, lba, c);
                } else if (std::memcmp(oem, "ReFS", 4) == 0) {
                    c.fs = CandidateFs::Refs;
                    pts = probe_refs(effective, lba, c);
                } else if (std::memcmp(oem, "-FVE-FS-", 8) == 0) {
                    c.fs = CandidateFs::BitLocker;
                    pts = probe_bitlocker(effective, lba, c);
                } else {
                    c.fs = CandidateFs::Fat;
                    pts = probe_fat(effective, lba, c);
                }
            }
            if (pts < 0) {
                /* the in-window superblock anchors: one shared window
                 * fetch at the placement (the probes only read in-window
                 * via scan.at() and fall back to direct reads - a fresh
                 * scanner never disturbs the sweep's own window); a
                 * failed fetch falls through to the out-of-window
                 * families below */
                WindowScanner rscan(effective);
                const bool win_ok = rscan.fetch(lba * ls);
                if (win_ok) {
                    c.fs = CandidateFs::Ext2;
                    pts = probe_ext(effective, lba, rscan, c);
                }
                if (pts < 0 && win_ok) {
                    c.fs = CandidateFs::HfsPlus;
                    pts = probe_hfsplus(effective, lba, rscan, c);
                }
            }
            if (pts < 0) {
                c.fs = CandidateFs::Btrfs;
                pts = probe_btrfs(effective, lba, c);
            }
            if (pts < 0) {
                c.fs = CandidateFs::Zfs;
                pts = probe_zfs(effective, lba, c);
            }
            if (pts < 0)
                continue; /* no parser admits the placement's content:
                           * FUN_14058a070's all-parsers-reject drop */
            /* the same normalization the sweep applies: tail-bound the
             * size, anchor the data area */
            if (c.sector_count == 0) {
                const u64 dev_tail = effective->geometry().sector_count
                                         ? effective->geometry().sector_count
                                         : effective->size() / ls;
                c.sector_count = dev_tail > c.start_lba ? u32(dev_tail - c.start_lba) : 1;
                c.size_from_device_end = true;
            }
            const u64 dev_tail = effective->geometry().sector_count
                                     ? effective->geometry().sector_count
                                     : effective->size() / ls;
            if (c.start_lba + c.sector_count > dev_tail)
                c.sector_count = u32(dev_tail > c.start_lba ? dev_tail - c.start_lba : 0);
            if (c.data_sector_count == 0) {
                c.data_start_lba = c.start_lba;
                c.data_sector_count = c.sector_count;
            }
            if (c.data_start_lba + c.data_sector_count > c.start_lba + c.sector_count) {
                c.data_start_lba = c.start_lba;
                c.data_sector_count = c.sector_count;
            }
            c.structural_points = pts;
            c.evidence.push_back({"register-placement", 10});
            found.push_back(c);
        }
    }

    /* pass 2: dedup (same-fs nested: the outer/earlier wins) */
    std::vector<Candidate> keep;
    for (size_t i = 0; i < found.size(); ++i) {
        bool dup = false;
        for (size_t j = 0; j < keep.size(); ++j) {
            /* N-035: containment runs on the DATA-AREA extent (the og's
             * EndSec = DataAreaStart + totalSec math) - a peer whose boot
             * sector sits inside the outer's reserved+FATs region is NOT
             * contained: the metadata gap spans it, the data area does
             * not. */
            if (found[i].fs == keep[j].fs && found[i].start_lba >= keep[j].data_start_lba &&
                found[i].start_lba < keep[j].data_start_lba + keep[j].data_sector_count) {
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

    /* pass 3 (M-017): the og's content-driven scoring.
     * (a) containment-in-known drop: a candidate FULLY inside a live
     *     (used, non-container) partition is dropped (AdjustPartition-
     *     InfoOrder rule (a), FUN_140510410 :842495+). The old core
     *     kept such candidates with a -20 penalty - the polarity
     *     inversion the census leg called the sharpest slice of this
     *     finding. Containers (extended/protective) do not count: a
     *     candidate inside the extended container's span but outside
     *     every real volume is genuinely lost (the og's own lost scan
     *     walks the container's unallocated interior). */
    std::vector<Candidate> adjusted;
    adjusted.reserve(keep.size());
    for (auto& c : keep) {
        /* M-017 (a): the containment-in-known drop — a candidate's DATA
         * AREA (N-035: the og's EndSec = DataAreaStart + totalSec math)
         * fully inside a live (used, non-container) partition is dropped
         * (AdjustPartitionInfoOrder rule (a)). This REPLACES the old
         * -20/-10 overlap penalties, which were the polarity inversion
         * of the og's +500 table-match. Containers (extended/protective)
         * do not count; a table-MATCHED candidate (same start) is exempt
         * — it takes the +500 instead. */
        bool contained = false;
        for (const auto& p : known) {
            if (!p.used || p.fs == table::FsKind::Extended || p.fs == table::FsKind::Protective)
                continue;
            if (c.start_lba == p.start_lba)
                continue; /* the table-MATCH case ("Found once partition"):
                           * +500 in the count, never a containment drop */
            if (c.data_start_lba >= p.start_lba &&
                c.data_start_lba + c.data_sector_count <= p.start_lba + p.sector_count) {
                contained = true;
                break;
            }
        }
        if (!contained)
            adjusted.push_back(std::move(c));
    }

    /* (b) the content probe + the og's weighted count: per-FS weights,
     *     the per-FS bonus (info+0x68, observed values), and the +500
     *     table-match bonus ("Found once partition:%I64d") */
    u64 max_count = 1;
    for (auto& c : adjusted) {
        u64 files = 0, folders = 0;
        content_counts(effective, c, cancel, files, folders);
        u64 count = weighted_count(og_fs_code(c.fs), files, folders);
        count += fs_bonus(c);
        c.table_match = table_match(c, known);
        if (c.table_match)
            count += 500;
        c.content_count = count;
        if (count > max_count)
            max_count = count;
    }
    /* (c) score = ln(count)/ln(max) in permille, the og's exact edge
     *     cases; structural points stay as the tie-break key */
    for (auto& c : adjusted)
        c.score = int(score_ratio(c.content_count, max_count) * 1000.0);

    /* (d) stable descending sort by score (order stability for equal
     *     scores - the og's merge sort; std::sort is NOT stable),
     *     tie-broken by the structural points */
    std::stable_sort(adjusted.begin(), adjusted.end(), [](const Candidate& a, const Candidate& b) {
        if (a.score != b.score)
            return a.score > b.score;
        return a.structural_points > b.structural_points;
    });

    /* (e) the ~16 MiB size-match promotion (AdjustPartitionInfoOrder
     *     :842851, log-proven in CalcPartition.log round 3): a candidate
     *     whose SIZE matches the leader's within the 16 MiB tolerance
     *     gets the leader's score copied and is spliced to result #2. */
    if (adjusted.size() >= 2) {
        /* FUN_140510410 :842776-793: |totalSector - ref| * 0x200 <
         * 0x1000001 (the ~16 MiB tolerance); the FIRST size-matching
         * candidate is promoted ONLY when at least one NON-matching
         * candidate precedes it in list order (the iVar16 counter) -
         * a matcher adjacent to the leader breaks the scan instead
         * (nothing to splice). One promotion per pass. */
        constexpr u64 TOL = 16ull * 1024 * 1024 / 512; /* 16 MiB in sectors */
        const Candidate& leader = adjusted[0];
        u64 non_matches_before = 0;
        for (size_t i = 1; i < adjusted.size(); ++i) {
            /* N-035: the og's 16-MiB size-match consumes the data-area
             * totalSec (the QA log's field) */
            const u64 a = adjusted[i].data_sector_count;
            const u64 b = leader.data_sector_count;
            const u64 diff = a > b ? a - b : b - a;
            if (diff <= TOL) {
                if (non_matches_before == 0)
                    break; /* adjacent match: the og's break path */
                Candidate promoted = adjusted[i];
                promoted.score = leader.score;
                adjusted.erase(adjusted.begin() + long(i));
                adjusted.insert(adjusted.begin() + 1, std::move(promoted));
                break;
            }
            ++non_matches_before;
        }
    }

    out.candidates = std::move(adjusted);
    out.sectors_scanned = sectors_scanned;
    out.read_errors = read_errors;
    if (progress)
        (void)progress(total, total, progress_user);
    return OK;
}

} // namespace lost
} // namespace cr
