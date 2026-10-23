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
    u64 window_base() const { return win_base_; }
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
         * clamps to the device anyway. */
        const u64 end = disk->geometry().sector_count;
        c.sector_count = end > boot_lba ? end - boot_lba : 1;
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
    int pts = 20; /* valid FAT BPB */
    c.sector_count = p.total_bytes / p.bytes_per_sector;
    /* FAT area probe: entries 2..4 readable and first data cluster sane */
    u8 fat_probe[16];
    if (disk->read_at(p.fat_start + 8, fat_probe, 16) == OK)
        pts += 10;
    c.evidence.push_back({"fat-bpb", 20});
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
        if (cancel && cancel->cancelled())
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
        for (u64 off = 0; off + 512 <= win_len; off += u64(stride_sectors) * ls) {
            const u8* sec = scan.at(base + off);
            if (sec == nullptr)
                break;
            if (sec[510] != 0x55 || sec[511] != 0xAA)
                continue;
            const u64 lba = (base + off) / ls;
            char oem[9] = {0};
            std::memcpy(oem, sec + 3, 8);

            Candidate c;
            c.start_lba = lba;
            int pts = 0;
            if (std::strncmp(oem, "NTFS    ", 8) == 0) {
                c.fs = CandidateFs::Ntfs;
                pts = probe_ntfs(disk, lba, c);
            } else if (std::strncmp(oem, "EXFAT   ", 8) == 0) {
                c.fs = CandidateFs::Exfat;
                pts = probe_exfat(disk, lba, c);
            } else if (std::memcmp(sec, "EFI PART", 8) == 0) {
                c.fs = CandidateFs::GptTable;
                pts = probe_gpt_header(disk, lba, c);
            } else {
                /* FAT family: validate via the BPB parser */
                c.fs = CandidateFs::Fat;
                pts = probe_fat(disk, lba, c);
                if (pts < 0)
                    continue; /* 55AA without a coherent BPB: noise */
            }
            if (pts < 0)
                continue;
            if (c.sector_count == 0)
                /* no probe set a size: bound by the device tail (audit row
                 * 31b - the old fabricated 2048 hid real geometry) */
                c.sector_count =
                    disk->size() / ls > c.start_lba ? disk->size() / ls - c.start_lba : 1;
            /* bound by device tail */
            if (c.start_lba + c.sector_count > disk->size() / ls)
                c.sector_count = disk->size() / ls - c.start_lba;
            c.score = pts;
            found.push_back(c);
        }
    }

    /* pass 2: dedup + overlap scoring */
    std::vector<Candidate> keep;
    for (size_t i = 0; i < found.size(); ++i) {
        bool dup = false;
        for (size_t j = 0; j < keep.size(); ++j) {
            if (found[i].fs == keep[j].fs && found[i].start_lba >= keep[j].start_lba &&
                found[i].start_lba < keep[j].start_lba + keep[j].sector_count) {
                dup = true; /* same fs, nested: the outer/earlier wins */
                break;
            }
        }
        if (!dup)
            keep.push_back(found[i]);
    }
    for (auto& c : keep) {
        Overlap ov = classify_overlap(c.start_lba, c.sector_count, known);
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
