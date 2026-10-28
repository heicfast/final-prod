/* cr_vollabel.cpp - engine-side on-disk volume-label decode (N-133).
 *
 * Per-family decodes (the corpus-verified forms):
 *   NTFS  : $Volume is MFT record 3; its $VOLUME_NAME attribute (type
 *           0x60, always resident) holds the UTF-16LE label - the og's
 *           FUN_18003f3a0 takes the value at +0x18 of the attribute and
 *           converts wide->narrow. The core reads the record through the
 *           existing bootstrap (NtfsVolume::open + read_record(3)), the
 *           same path the walk uses.
 *   ext4  : the superblock's s_volume_name (16 bytes at sb+120), already
 *           decoded into SuperBlock::volume_name by the engine - this
 *           probe re-reads the superblock directly (the engine-side
 *           consumer of the existing decode rides scan_ext4's stats
 *           surface; this is the image/pre-scan half).
 *   FAT   : the 11-byte volume-label boot field at +0x2B (the standard
 *           FAT structure - the format's own on-disk label; the corpus
 *           deep-dive documents the FS-type strings at +0x36/+0x52 which
 *           the detection here also uses).
 *   exFAT : the Volume Label dentry (type 0x83, exfat-engine--deep-dive.md
 *           SS3) in the root directory's first cluster: byte 3 = the label
 *           length (chars), bytes 4..25 = the UTF-16LE label. (The
 *           finding's "exFAT +0x80" boot-offset reading is corrected by
 *           the corpus: the exFAT label lives in the root-dir dentry, not
 *           the boot sector.)
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/orchestrator/cr_vollabel.h"

#include "cr/exfat/cr_exfat_engine.h"
#include "cr/ntfs/cr_attributes.h"
#include "cr/ntfs/cr_bootstrap.h"
#include "cr/ntfs/cr_fixup.h"
#include "cr/ntfs/cr_ntfs_wire.h"
#include "cr/storage/cr_device.h"
#include "cr/storage/cr_slice.h"

#include <cstring>

namespace cr {
namespace orchestrator {
namespace {

/* wide->narrow with the U+FFFD substitution rule (N-016b's precedent: a
 * label is display text - lossy narrowing, never a hard failure) */
void narrow_utf16_to_label(const u8* w, u32 chars, char out[33]) {
    u32 k = 0;
    for (u32 i = 0; i < chars && k + 1 < 33; ++i) {
        const u32 cp = u32(w[2 * i]) | (u32(w[2 * i + 1]) << 8);
        if (cp == 0)
            break;
        out[k++] = cp < 0x80 ? char(cp) : '?'; /* ASCII labels; else the
                                                * documented substitution */
    }
    out[k] = 0;
}

bool boot_is_fat(const u8* boot) {
    /* the FS-type strings the family detection keys on (fat-engine--
     * deep-dive.md SS4: _strnicmp at +0x36 (FAT12/16) / +0x52 (FAT32)) */
    for (const u32 off : {u32(0x36), u32(0x52)}) {
        if (boot[off] == 'F' && boot[off + 1] == 'A' && boot[off + 2] == 'T')
            return true;
    }
    return false;
}

void fat_label_from_boot(const u8* boot, char out[33]) {
    /* the 11-byte volume-label boot field (+0x2B), space-padded; trim the
     * pad + trailing NULs; empty = a label-less FAT volume */
    u32 end = 11;
    while (end > 0 && (boot[0x2B + end - 1] == ' ' || boot[0x2B + end - 1] == 0))
        --end;
    u32 k = 0;
    for (; k < end && k + 1 < 33; ++k)
        out[k] = boot[0x2B + k] >= 0x20 && boot[0x2B + k] < 0x7F ? char(boot[0x2B + k]) : '?';
    out[k] = 0;
}

u32 exfat_label_from_root(IStorageDevice* slice, const exfat::ExfatParams& p, char out[33]) {
    out[0] = 0;
    if (p.first_cluster_of_root == 0 || p.cluster_bytes == 0)
        return OK; /* no root placement - a label-less volume */
    /* the root's first cluster: the Volume Label dentry (exFAT spec 7.2:
     * EntryType 0x83 @0, CharacterCount @1, Reserved @2-3, the UTF-16LE
     * VolumeLabel @4 - the same layout test_fixtures.hpp's M-079
     * spec-repair pins). Scan the whole first cluster (order-tolerant);
     * the deleted 0x03 form is NOT the volume label anymore. */
    const u64 heap = u64(p.cluster_heap_offset) * p.bytes_per_sector;
    const u64 cluster_off = heap + u64(p.first_cluster_of_root - 2) * p.cluster_bytes;
    if (cluster_off + p.cluster_bytes > slice->size())
        return OK; /* out of range: label-less, never an error */
    std::vector<u8> buf(p.cluster_bytes);
    if (slice->read_at(cluster_off, buf.data(), u32(buf.size())) != OK)
        return OK; /* unreadable root: label-less (the informational posture) */
    for (u32 off = 0; off + 32 <= buf.size(); ++off) {
        if (buf[off] == 0x83 && (buf[off + 1] != 0) && buf[off + 1] <= 11) {
            narrow_utf16_to_label(buf.data() + off + 4, buf[off + 1], out);
            return OK;
        }
        if (buf[off] == 0x00)
            break; /* end-of-directory: no label dentry present */
    }
    return OK;
}

u32 ntfs_label_from_volume(IStorageDevice* slice, char out[33]) {
    out[0] = 0;
    u32 rc = 0;
    auto vol =
        ntfs::NtfsVolume::open(std::unique_ptr<IStorageDevice>(cr::SliceShim::shim(slice)), &rc);
    if (!vol)
        return rc != OK ? rc : u32(ERR_OPEN_FAILED);
    std::vector<u8> rec;
    if ((rc = vol->read_record(3, rec)) != OK) /* $Volume */
        return rc;
    ntfs::RecordHeader rh;
    if (rec.size() < sizeof(rh))
        return OK; /* short record: label-less */
    std::memcpy(&rh, rec.data(), sizeof(rh));
    if (rh.magic != ntfs::RECORD_MAGIC_FILE)
        return OK;
    /* F4: fixup mandatory before any parse (the walk's own rule) */
    if (ntfs::fixup_apply(rec.data(), u32(rec.size()), rh.update_sequence_offset,
                          rh.update_sequence_size, vol->params().sector_bytes) != OK)
        return OK; /* damaged $Volume: label-less, never an error */
    std::vector<ntfs::AttrView> attrs;
    if (ntfs::enum_attributes(rec.data(), u32(rec.size()), attrs) != OK)
        return OK;
    for (const ntfs::AttrView& a : attrs) {
        if (a.type != ntfs::ATTR_TYPE_VOLUME_NAME || a.nonresident)
            continue;
        /* the resident value: the UTF-16LE label (the og's +0x18 read of
         * the attribute, wide->narrow) */
        if (a.value_offset >= a.buf_size || u64(a.value_offset) + a.value_length > a.buf_size)
            continue;
        narrow_utf16_to_label(a.at + a.value_offset, (a.value_length + 1) / 2, out);
        break;
    }
    return OK;
}

bool ext4_label_from_superblock(IStorageDevice* slice, char out[33]) {
    out[0] = 0;
    /* the superblock at byte offset 1024; s_magic (LE u16) at sb+56 */
    u8 sb[1024];
    if (slice->size() < 1024 + sizeof(sb))
        return false;
    if (slice->read_at(1024, sb, sizeof(sb)) != OK)
        return false;
    const u16 magic = u16(sb[56]) | (u16(sb[57]) << 8);
    if (magic != 0xEF53)
        return false;
    /* s_volume_name: 16 bytes at sb+120, NUL-padded */
    u32 k = 0;
    for (; k < 16 && k + 1 < 33 && sb[120 + k] != 0; ++k)
        out[k] = sb[120 + k] >= 0x20 && sb[120 + k] < 0x7F ? char(sb[120 + k]) : '?';
    out[k] = 0;
    return true;
}

} // namespace

u32 probe_slice_volume_label(IStorageDevice* slice, char out[33]) {
    if (slice == nullptr || out == nullptr)
        return ERR_BAD_PARAMETER;
    out[0] = 0;
    u8 boot[512];
    if (slice->read_at(0, boot, sizeof(boot)) != OK)
        return ERR_READ_FAILED;
    /* exFAT: the FS name at +0x03 ("EXFAT   ") */
    if (std::memcmp(boot + 3, "EXFAT", 5) == 0) {
        exfat::ExfatParams p;
        if (exfat::validate_boot_region(slice, p) == OK)
            return exfat_label_from_root(slice, p, out);
        return OK; /* backup-boot fallback etc.: the engine's own path */
    }
    /* NTFS: the FS name at +0x03 ("NTFS    ") */
    if (std::memcmp(boot + 3, "NTFS", 4) == 0)
        return ntfs_label_from_volume(slice, out);
    /* FAT: the FS-type strings at +0x36/+0x52 */
    if (boot_is_fat(boot)) {
        fat_label_from_boot(boot, out);
        return OK;
    }
    /* ext4: the superblock probe (LBA0 is not the superblock) */
    if (ext4_label_from_superblock(slice, out))
        return OK;
    return ERR_UNSUPPORTED; /* no known family: the caller's fallback */
}

} // namespace orchestrator
} // namespace cr
