/* cr_fscheck.cpp - the damage-audit check phase (M-041, diff-to-fix).
 *
 * Read-only audit pass reproducing the og's FileSystemCheck.mo behavior
 * with its error taxonomy (see cr_fscheck.h for the full corpus map).
 * NEVER repairs, NEVER aborts a scan - the report is data.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/check/cr_fscheck.h"
#include "cr/fat/cr_fat_engine.h"
#include "cr/fat/cr_fat_wire.h"
#include "cr/ntfs/cr_attributes.h"
#include "cr/ntfs/cr_bitmap.h"
#include "cr/ntfs/cr_bootstrap.h"
#include "cr/ntfs/cr_fixup.h"
#include "cr/ntfs/cr_ntfs_wire.h"
#include "cr/ntfs/cr_runlist.h"
#include <algorithm>
#include <cstring>
#include <memory>
#include <set>
#include <vector>

namespace cr {
namespace fscheck {

namespace {

void push_error(FsCheckReport& r, u32 code, u64 param, std::u16string hint = {}) {
    CheckError e;
    e.code = code;
    e.param = param;
    e.path_hint = std::move(hint);
    r.errors.push_back(std::move(e));
}

/* ------------------------------- FAT pass ------------------------------ */

struct FatAudit {
    IStorageDevice* dev;
    fat::FatParams p;
    FsCheckReport* r;
    CancelToken* cancel;
    /* entry values per copy, decoded (fat1 authoritative) */
    std::vector<u32> fat1, fat2;
    u32 entry_count = 0;      /* table entries (incl. 2 virtual) */
    u32 data_cluster_count = 0; /* clusters after the 2 virtual ones */
    /* per-flavor reserved/BAD/EOC families */
    u32 bad_val = 0;
    u32 reserved_lo = 0;

    u32 reserved_bad() const {
        return bad_val;
    }
    bool is_eoc_family(u32 v) const { return v >= reserved_lo; }
    bool is_data_link(u32 v) const {
        return v >= 2 && v < 2 + data_cluster_count;
    }

    std::vector<u8> visited;    /* per data cluster: consumed by a chain */
    std::vector<u8> dir_on_path; /* recursion-local cycle guard */
    int depth = 0;
    static constexpr int MAX_DEPTH = 64;

    std::u16string cur_path; /* best-effort path prefix */
};

bool poll_cancel(FatAudit& a) {
    return a.cancel != nullptr && a.cancel->checkpoint();
}

u32 fat_entry_at(const std::vector<u8>& raw, fat::Flavor f, u32 i) {
    switch (f) {
    case fat::Flavor::Fat12: {
        const size_t off = (size_t(i) * 3) / 2;
        if (off + 1 >= raw.size())
            return 0;
        if (i & 1)
            return u32(raw[off] >> 4) | (u32(raw[off + 1]) << 4);
        return u32(raw[off]) | (u32(raw[off + 1] & 0x0F) << 8);
    }
    case fat::Flavor::Fat16: {
        const size_t off = size_t(i) * 2;
        if (off + 1 >= raw.size())
            return 0;
        return u32(raw[off]) | (u32(raw[off + 1]) << 8);
    }
    default: {
        const size_t off = size_t(i) * 4;
        if (off + 3 >= raw.size())
            return 0;
        return u32(raw[off]) | (u32(raw[off + 1]) << 8) | (u32(raw[off + 2]) << 16) |
               (u32(raw[off + 3]) << 24);
    }
    }
}

/* The og's DBR phase: structural gates reject (0x100d upstream); the
 * per-flavor ceilings CLAMP the audited cluster count (decompile
 * :1355-1360/:1413-1417/:1469-1473 - `if (ceiling < computed) computed =
 * ceiling`, return 1). Returns false on a structural failure. */
bool fat_dbr_gates(const u8* boot, FatAudit& a) {
    using fat::Flavor;
    const u16 bps = u16(boot[11]) | u16(boot[12]) << 8;
    const u8 spc = boot[13];
    const u8 nfats = boot[16];
    if (bps == 0 || (bps & (bps - 1)) != 0)
        return false;
    if (spc == 0 || (spc & (spc - 1)) != 0)
        return false;
    if (nfats < 1 || nfats > 4)
        return false;
    const u32 spf = u32(boot[22]) | u32(boot[23]) << 8; /* FAT12/16 */
    const u32 spf32 = u32(boot[36]) | u32(boot[37]) << 8 | u32(boot[38]) << 16 |
                      u32(boot[39]) << 24;
    switch (a.p.flavor) {
    case Flavor::Fat12: {
        if (std::memcmp(boot + 54, "FAT12   ", 8) != 0 && std::memcmp(boot + 54, "FAT     ", 8) != 0)
            return false;
        const long long ceiling = (long long)((double)spf * 341.3333333);
        if ((long long)a.data_cluster_count > ceiling)
            a.data_cluster_count = u32(ceiling);
        a.bad_val = 0xff7;
        a.reserved_lo = 0xff6;
        break;
    }
    case Flavor::Fat16: {
        if (std::memcmp(boot + 54, "FAT16   ", 8) != 0 && std::memcmp(boot + 54, "FAT     ", 8) != 0)
            return false;
        const long long ceiling = (long long)spf * 0x100;
        if ((long long)a.data_cluster_count > ceiling)
            a.data_cluster_count = u32(ceiling);
        a.bad_val = 0xfff7;
        a.reserved_lo = 0xfff6;
        break;
    }
    case Flavor::Fat32: {
        if (std::memcmp(boot + 82, "FAT32   ", 8) != 0)
            return false;
        if (spf32 == 0)
            return false;
        const u32 root_cluster = u32(boot[44]) | u32(boot[45]) << 8 | u32(boot[46]) << 16 |
                                 u32(boot[47]) << 24;
        if (root_cluster < 2 || root_cluster >= 2 + a.data_cluster_count)
            return false;
        const u64 ceiling = (u64(bps) * spf32) >> 2;
        if (u64(a.data_cluster_count) > ceiling)
            a.data_cluster_count = u32(ceiling);
        a.bad_val = 0xffffff7;
        a.reserved_lo = 0xffffff6;
        break;
    }
    default:
        return false;
    }
    return true;
}

/* Follow one chain from `first`, marking the global visited array
 * (cross-link detection) and detecting path-local cycles. Returns the
 * walked cluster list. The walk is capped at data_cluster_count steps
 * (a pathological cycle cannot spin forever); a cycle in a DIRECTORY
 * chain is reported by the caller (0x1007). */
std::vector<u32> walk_chain(FatAudit& a, u32 first, bool* crosslink, bool* cycle) {
    std::vector<u32> out;
    *crosslink = false;
    *cycle = false;
    std::set<u32> local;
    u32 c = first;
    u32 guard = 0;
    while (c >= 2 && c < 2 + a.data_cluster_count) {
        if (guard++ > a.data_cluster_count + 2) {
            *cycle = true;
            break;
        }
        if (!local.insert(c).second) {
            *cycle = true; /* path-local revisit (the og's TBitmap loop
                            * detection, :1174) - a self-referencing chain
                            * is a CYCLE, not a cross-link */
            break;
        }
        if (a.visited[size_t(c - 2)]) {
            *crosslink = true; /* another chain already consumed it */
            break;
        }
        a.visited[size_t(c - 2)] = 1;
        ++a.r->clusters_visited;
        out.push_back(c);
        const u32 v = a.fat1[size_t(c)];
        if (v == 0 || a.is_eoc_family(v))
            break; /* FREE stop (should not happen live) or EOC */
        c = v;
    }
    return out;
}

std::vector<u8> read_dir_clusters(FatAudit& a, const std::vector<u32>& clusters) {
    std::vector<u8> out;
    out.reserve(clusters.size() * size_t(a.p.cluster_bytes));
    for (u32 c : clusters) {
        const u64 off = fat::FatTable::cluster_offset(a.p, c);
        const size_t before = out.size();
        out.resize(before + a.p.cluster_bytes, 0);
        if (a.dev->read_at(off, out.data() + before, a.p.cluster_bytes) != OK) {
            push_error(*a.r, CHECK_EXCEPTION, c);
            out.resize(before);
            continue;
        }
    }
    return out;
}

void audit_dir_entries(FatAudit& a, const std::vector<u8>& buf, const std::u16string& prefix) {
    const size_t count = buf.size() / 32;
    for (size_t i = 0; i < count; ++i) {
        if (poll_cancel(a))
            return;
        const u8* raw = buf.data() + i * 32;
        const fat::DirEntry* e = reinterpret_cast<const fat::DirEntry*>(raw);
        if (e->name[0] == fat::DENTRY_END)
            break;
        if (e->name[0] == fat::DENTRY_DELETED)
            continue; /* deleted: not audited (the og skips 0xE5, :1929) */
        if (e->attributes == fat::ATTR_LFN)
            continue;
        if (e->attributes & 0x08)
            continue; /* volume label */
        ++a.r->entries_scanned;

        const bool is_dir = (e->attributes & fat::ATTR_DIRECTORY) != 0;
        const u32 fc = (u32(e->first_cluster_high) << 16) | e->first_cluster_low;

        /* `.`/`..` validation (the og checks the pair's byte form,
         * :1893-1907): when present, the first entry must be "." and the
         * second "..", both directories. */
        if (e->name[0] == '.' && is_dir) {
            const bool dot_ok = e->name[1] == ' ' && i == 0;
            bool dotdot_ok = false;
            if (i + 1 < count) {
                const fat::DirEntry* e2 =
                    reinterpret_cast<const fat::DirEntry*>(buf.data() + (i + 1) * 32);
                dotdot_ok = e2->name[0] == '.' && e2->name[1] == '.' && e2->name[2] == ' ' &&
                            (e2->attributes & fat::ATTR_DIRECTORY) != 0 &&
                            e2->name[0] != fat::DENTRY_DELETED;
            }
            if (!dot_ok || !dotdot_ok) {
                push_error(*a.r, CHECK_DENTRY_INVALID, 0, prefix);
                continue;
            }
            ++i; /* the ".." pair member is validated */
            continue;
        }

        const bool fc_ok = fc >= 2 && fc < 2 + a.data_cluster_count;
        if (!fc_ok && !( !is_dir && e->file_size == 0 && fc == 0)) {
            push_error(*a.r, CHECK_DENTRY_INVALID, fc, prefix);
            continue;
        }
        if (fc == 0)
            continue; /* legal empty file */

        bool crosslink = false, cycle = false;
        const std::vector<u32> chain = walk_chain(a, fc, &crosslink, &cycle);
        if (crosslink) {
            push_error(*a.r, CHECK_CROSSLINK, fc, prefix);
            continue;
        }
        if (!is_dir) {
            /* size vs chain (the og: expected clusters from the size vs
             * the walked sum, :2050-2076) */
            const u64 expect = (u64(e->file_size) + a.p.cluster_bytes - 1) / a.p.cluster_bytes;
            if (expect != chain.size())
                push_error(*a.r, CHECK_SIZE_NE_CHAIN, fc, prefix);
            continue;
        }
        /* directory: cycle -> 0x1007; then recurse into the content */
        if (cycle) {
            push_error(*a.r, CHECK_DENTRY_INVALID, fc, prefix);
            continue;
        }
        if (a.depth >= FatAudit::MAX_DEPTH)
            continue;
        const std::vector<u8> content = read_dir_clusters(a, chain);
        std::u16string name;
        for (int k = 0; k < 11 && e->name[k] != ' '; ++k)
            name.push_back(u16(e->name[k]));
        std::u16string child_prefix = prefix.empty() ? name : prefix + u"\\" + name;
        ++a.depth;
        audit_dir_entries(a, content, child_prefix);
        --a.depth;
    }
}

} // namespace

u32 check_fat_volume(IStorageDevice* slice, FsCheckReport& report, CancelToken* cancel) {
    if (slice == nullptr)
        return ERR_BAD_PARAMETER;
    report = FsCheckReport();
    u8 boot[512];
    if (slice->read_at(0, boot, sizeof(boot)) != OK)
        return ERR_READ_FAILED;

    FatAudit a;
    a.dev = slice;
    a.r = &report;
    a.cancel = cancel;
    /* the engine's own boot validation gives the geometry; the check's
     * DBR gates (og 0x100d family) run on top */
    if (fat::validate_boot(boot, slice->size(), a.p) != OK) {
        push_error(report, CHECK_DBR_BAD, 0);
        return OK;
    }
    a.data_cluster_count = a.p.cluster_count >= 2 ? a.p.cluster_count - 2 : 0;
    if (!fat_dbr_gates(boot, a)) {
        push_error(report, CHECK_DBR_BAD, 0);
        return OK;
    }
    if (a.data_cluster_count == 0 || a.p.fat_sectors == 0) {
        push_error(report, CHECK_DBR_BAD, 0);
        return OK;
    }

    /* ---- load both FAT copies (raw bytes; the check reads them itself,
     * exactly like the og's CCheckFAT) ---- */
    const u64 fat_bytes = u64(a.p.fat_sectors) * a.p.bytes_per_sector;
    std::vector<u8> raw1;
    raw1.resize(size_t(fat_bytes));
    std::vector<u8> raw2;
    if (slice->read_at(a.p.fat_start, raw1.data(), u32(fat_bytes)) != OK)
        return ERR_READ_FAILED;
    const bool two_copies = a.p.fat_count >= 2 && a.p.fat2_start != 0;
    if (two_copies) {
        raw2.resize(size_t(fat_bytes));
        if (slice->read_at(a.p.fat2_start, raw2.data(), u32(fat_bytes)) != OK)
            return ERR_READ_FAILED;
    }

    /* entry count: all entries the table can address (the og scans the
     * whole table, bounded by the clamped cluster count + 2 virtual) */
    a.entry_count = 2 + a.data_cluster_count;
    a.fat1.resize(a.entry_count, 0);
    for (u32 i = 0; i < a.entry_count; ++i)
        a.fat1[i] = fat_entry_at(raw1, a.p.flavor, i);
    if (two_copies) {
        a.fat2.resize(a.entry_count, 0);
        for (u32 i = 0; i < a.entry_count; ++i)
            a.fat2[i] = fat_entry_at(raw2, a.p.flavor, i);
    }
    report.fat_entries_scanned = a.entry_count;

    /* ---- phase 2: entry holes + duplicate references (0x1006) ---- */
    {
        std::vector<u8> claimed(size_t(a.data_cluster_count), 0);
        for (u32 c = 2; c < a.entry_count; ++c) {
            if (poll_cancel(a))
                return OK;
            const u32 v = a.fat1[size_t(c)];
            if (v == 0)
                continue; /* free */
            if (a.is_data_link(v)) {
                /* hole check half: a data link is always well-formed;
                 * duplicate check: two entries naming the same target */
                const u32 target = v - 2;
                if (++claimed[size_t(target)] > 1)
                    push_error(report, CHECK_CHAIN_INVALID, v);
            } else if (!a.is_eoc_family(v)) {
                /* neither free-target, data link, nor EOC/BAD family:
                 * an entry pointing into the hole between the data
                 * region and the reserved family (FUN_180005fd0) */
                push_error(report, CHECK_CHAIN_INVALID, v);
            }
        }
    }

    /* ---- phase 3: pairwise copy compare (0x1005) ---- */
    if (two_copies) {
        for (u32 i = 0; i < a.entry_count; ++i) {
            if (a.fat1[i] != a.fat2[i]) {
                push_error(report, CHECK_FAT_COPIES_DIFFER, i);
                break; /* the og reports the first mismatch and stops */
            }
        }
    }

    /* ---- phase 4: cluster-chain / directory audit ---- */
    a.visited.assign(size_t(a.data_cluster_count), 0);
    if (a.p.flavor == fat::Flavor::Fat32) {
        if (a.p.root_cluster >= 2 && a.p.root_cluster < 2 + a.data_cluster_count) {
            bool crosslink = false, cycle = false;
            const std::vector<u32> chain = walk_chain(a, a.p.root_cluster, &crosslink, &cycle);
            if (cycle)
                push_error(report, CHECK_DENTRY_INVALID, a.p.root_cluster);
            const std::vector<u8> content = read_dir_clusters(a, chain);
            ++a.depth;
            audit_dir_entries(a, content, std::u16string());
            --a.depth;
        } else {
            push_error(report, CHECK_DBR_BAD, a.p.root_cluster);
        }
    } else {
        /* fixed root area */
        const u64 len = u64(a.p.root_entries) * 32;
        if (len > 0) {
            std::vector<u8> root(size_t(len), 0);
            if (slice->read_at(a.p.root_dir_offset, root.data(), u32(len)) != OK)
                push_error(report, CHECK_EXCEPTION, 0);
            else {
                ++a.depth;
                audit_dir_entries(a, root, std::u16string());
                --a.depth;
            }
        }
    }

    /* ---- phase 5: orphan sweep (0x1003) ---- */
    for (u32 c = 2; c < a.entry_count; ++c) {
        if (poll_cancel(a))
            return OK;
        if (a.visited[size_t(c - 2)])
            continue;
        const u32 v = a.fat1[size_t(c)];
        if (v == 0)
            continue; /* free */
        if (v == a.bad_val)
            continue; /* BAD (the og's sentinel set) */
        /* unvisited, non-free, non-BAD: orphan (host decision in the og;
         * recorded here) */
        push_error(report, CHECK_ORPHAN, c);
    }
    return OK;
}

/* ------------------------------ NTFS pass ------------------------------ */

namespace {

/* non-owning device adapter: NtfsVolume::open adopts a unique_ptr; the
 * check borrows the caller's slice (the wrapper dies, the slice lives) */
struct BorrowedDevice final : public IStorageDevice {
    explicit BorrowedDevice(IStorageDevice* d) : d_(d) {
        geom_ = d->geometry();
    }
    u32 read_at(u64 at, void* out, u32 len) override { return d_->read_at(at, out, len); }
    u64 size() const override { return d_->size(); }
    const Geometry& geometry() const override { return geom_; }

private:
    IStorageDevice* d_;
    Geometry geom_;
};

struct NtfsRecHeader {
    u32 magic;
    u16 update_sequence_offset;
    u16 update_sequence_size;
    u64 base_record;
    u16 flags;
};

bool parse_rec_header(const u8* rec, NtfsRecHeader& h) {
    std::memcpy(&h.magic, rec, 4);
    if (h.magic != cr::ntfs::RECORD_MAGIC_FILE)
        return false;
    std::memcpy(&h.update_sequence_offset, rec + 4, 2);
    std::memcpy(&h.update_sequence_size, rec + 6, 2);
    std::memcpy(&h.base_record, rec + 0x20, 8);
    std::memcpy(&h.flags, rec + 0x16, 2);
    return true;
}

/* the og's $MFT proof: record must carry a RESIDENT $FILE_NAME whose
 * FILE NAME VALUE is exactly "$MFT" (:2900-2916 - the name inside the
 * attribute payload, not the attribute's own name) */
bool mft_name_proof(const u8* rec, u32 record_size, u32 sector_bytes) {
    NtfsRecHeader h;
    if (!parse_rec_header(rec, h))
        return false;
    if (cr::ntfs::fixup_apply(const_cast<u8*>(rec), record_size, h.update_sequence_offset,
                          h.update_sequence_size, sector_bytes) != OK)
        return false;
    std::vector<cr::ntfs::AttrView> attrs;
    if (cr::ntfs::enum_attributes(rec, record_size, attrs) != OK)
        return false;
    for (const cr::ntfs::AttrView& a : attrs) {
        if (a.type != cr::ntfs::ATTR_TYPE_FILE_NAME || a.nonresident)
            continue;
        cr::ntfs::FileNameInfo fi;
        if (cr::ntfs::parse_file_name(a, fi) == OK && fi.name == u"$MFT")
            return true;
    }
    return false;
}

} // namespace

u32 check_ntfs_volume(IStorageDevice* slice, FsCheckReport& report, CancelToken* cancel) {
    if (slice == nullptr)
        return ERR_BAD_PARAMETER;
    report = FsCheckReport();
    if (cancel && cancel->cancelled())
        return ERR_CANCELLED;

    /* ---- the volume-size gate (0x1001): the DBR's total size must not
     * exceed the device (the og compares the caller's size against the
     * volume's, :2832-2835) ---- */
    {
        u8 boot[512];
        if (slice->read_at(0, boot, sizeof(boot)) != OK)
            return ERR_READ_FAILED;
        const u16 bps = u16(boot[11]) | u16(boot[12]) << 8;
        u64 total_sectors = u32(boot[40]) | u32(boot[41]) << 8 | u32(boot[42]) << 16 |
                            u32(boot[43]) << 24;
        if (total_sectors == 0)
            std::memcpy(&total_sectors, boot + 0x28, 8);
        if (bps != 0 && total_sectors != 0 &&
            u64(total_sectors) * bps > slice->size() + 512) {
            push_error(report, CHECK_SIZE_MISMATCH, u64(total_sectors) * bps);
        }
    }

    u32 err = 0;
    auto vol = cr::ntfs::NtfsVolume::open(std::unique_ptr<IStorageDevice>(new BorrowedDevice(slice)),
                                      &err);
    if (vol == nullptr)
        return err != OK ? err : ERR_UNSUPPORTED;
    const cr::ntfs::VolumeParams& vp = vol->params();
    if (cancel && cancel->cancelled())
        return ERR_CANCELLED;

    std::vector<u8> rec;

    /* ---- $MFT record-0 proof (0x1008) ---- */
    if (vol->read_record(0, rec) != OK || rec.size() < vp.record_size ||
        !mft_name_proof(rec.data(), u32(vp.record_size), vp.sector_bytes))
        push_error(report, CHECK_MFT_BAD, 0);
    if (cancel && cancel->cancelled())
        return ERR_CANCELLED;

    /* ---- $MFTMirr proof (0x1009): the mirror's record 0 carries the
     * same "$MFT" $FILE_NAME (:2921-2971) ---- */
    {
        std::vector<u8> mir;
        if (vol->read_record_mirror(0, mir) != OK || mir.size() < vp.record_size ||
            !mft_name_proof(mir.data(), u32(vp.record_size), vp.sector_bytes))
            push_error(report, CHECK_MFTMIRR_BAD, 0);
    }
    if (cancel && cancel->cancelled())
        return ERR_CANCELLED;

    /* ---- the NTFS version gate (0x100a): $Volume's $VOLUME_INFORMATION
     * must carry {1.2, 3.0, 3.1} (the og's literal set {0x102, 3, 0x301},
     * :2994-3003) ---- */
    {
        bool found = false;
        u8 major = 0, minor = 0;
        const u64 scan_cap = vol->max_record_no() < 16 ? vol->max_record_no() : 16;
        for (u64 rn = 0; rn < scan_cap && !found; ++rn) {
            if (vol->read_record(rn, rec) != OK || rec.size() < vp.record_size)
                continue;
            NtfsRecHeader h;
            if (!parse_rec_header(rec.data(), h))
                continue;
            if (cr::ntfs::fixup_apply(rec.data(), u32(vp.record_size), h.update_sequence_offset,
                                  h.update_sequence_size, vp.sector_bytes) != OK)
                continue;
            std::vector<cr::ntfs::AttrView> attrs;
            if (cr::ntfs::enum_attributes(rec.data(), u32(vp.record_size), attrs) != OK)
                continue;
            bool is_volume = false;
            for (const cr::ntfs::AttrView& a : attrs) {
                if (a.type != cr::ntfs::ATTR_TYPE_FILE_NAME || a.nonresident)
                    continue;
                cr::ntfs::FileNameInfo fi;
                if (cr::ntfs::parse_file_name(a, fi) == OK && fi.name == u"$Volume")
                    is_volume = true;
            }
            if (!is_volume)
                continue;
            for (const cr::ntfs::AttrView& a : attrs) {
                if (a.type != cr::ntfs::ATTR_TYPE_VOLUME_INFORMATION || a.nonresident)
                    continue;
                const u8* d = nullptr;
                u32 dlen = 0;
                if (cr::ntfs::resident_span(a, d, dlen) != OK || dlen < 0x10)
                    continue;
                major = d[8];
                minor = d[9];
                found = true;
                break;
            }
        }
        if (found) {
            const bool ok_ver = (major == 1 && minor == 2) || (major == 3 && minor == 0) ||
                                (major == 3 && minor == 1);
            if (!ok_ver)
                push_error(report, CHECK_VERSION_BAD, (u64(major) << 8) | minor);
        }
        /* not found: the og's GetNTFSVersion would have failed through
         * its own volume object; our open already validated the boot, so
         * a missing $Volume is not a version-gate failure - no error */
    }
    if (cancel && cancel->cancelled())
        return ERR_CANCELLED;

    /* ---- $Bitmap structural (0x100b) + the MFT x $Bitmap cross-check
     * (0x1003 free-but-allocated / 0x1002 double-claim) ---- */
    std::unique_ptr<cr::ntfs::VolumeBitmap> bitmap;
    const u32 bm_rc = cr::ntfs::VolumeBitmap::load(vol.get(), bitmap);
    if (bm_rc != OK || bitmap == nullptr) {
        push_error(report, CHECK_BITMAP_BAD, bm_rc);
        return OK;
    }
    {
        /* structural: the $Bitmap record must not carry $ATTRIBUTE_LIST
         * and its $DATA must be non-resident (the og :3788-3942); size
         * sanity against the cluster count (0x100b) */
        if (vol->read_record(6, rec) != OK || rec.size() < vp.record_size) {
            push_error(report, CHECK_BITMAP_BAD, 6);
            return OK;
        }
        NtfsRecHeader h;
        if (parse_rec_header(rec.data(), h) &&
            cr::ntfs::fixup_apply(rec.data(), u32(vp.record_size), h.update_sequence_offset,
                              h.update_sequence_size, vp.sector_bytes) == OK) {
            std::vector<cr::ntfs::AttrView> attrs;
            if (cr::ntfs::enum_attributes(rec.data(), u32(vp.record_size), attrs) == OK) {
                bool has_attrlist = false, data_resident = true;
                u64 data_size = 0;
                for (const cr::ntfs::AttrView& a : attrs) {
                    if (a.type == cr::ntfs::ATTR_TYPE_ATTRIBUTE_LIST)
                        has_attrlist = true;
                    if (a.type == cr::ntfs::ATTR_TYPE_DATA && a.name_length == 0) {
                        data_resident = !a.nonresident;
                        /* non-resident span in BYTES (the run clusters x
                         * cluster size); resident = the value length */
                        data_size = a.nonresident
                                        ? (a.last_vcn - a.start_vcn + 1) * vp.cluster_bytes
                                        : a.value_length;
                    }
                }
                if (has_attrlist || data_resident) {
                    push_error(report, CHECK_BITMAP_BAD, 6);
                    return OK;
                }
                /* size sanity: the stream must COVER the volume's clusters
                 * (a short $Bitmap cannot describe the volume) without an
                 * absurd over-allocation (1 MiB of slop - real volumes
                 * round up to one cluster) */
                const u64 expect = (vp.total_clusters + 7) / 8;
                if (data_size < expect || data_size > expect + (1ull << 20)) {
                    push_error(report, CHECK_BITMAP_BAD, data_size);
                    return OK;
                }
            }
        }
    }
    if (cancel && cancel->cancelled())
        return ERR_CANCELLED;

    /* the cross-check: every allocated record's non-resident dataruns
     * against (a) the on-disk bitmap (free-but-allocated -> 0x1003) and
     * (b) a fresh claimed map (double-claim -> 0x1002). Sparse runs are
     * holes, not claims. */
    {
        const u64 max_rec = vol->max_record_no();
        std::vector<u8> claimed; /* fresh bitmap: LCNs claimed this pass */
        const u64 total = vp.total_clusters;
        claimed.assign(size_t((total + 7) / 8), 0);
        for (u64 rn = 0; rn < max_rec; ++rn) {
            if ((rn & 0xFF) == 0 && cancel && cancel->checkpoint())
                return ERR_CANCELLED;
            if (vol->read_record(rn, rec) != OK || rec.size() < vp.record_size)
                continue;
            NtfsRecHeader h;
            if (!parse_rec_header(rec.data(), h))
                continue;
            if ((h.flags & cr::ntfs::RECORD_IN_USE) == 0)
                continue; /* the og walks ALLOCATED records */
            if (h.base_record != 0)
                continue; /* extension record: its base carries the runs */
            if (cr::ntfs::fixup_apply(rec.data(), u32(vp.record_size), h.update_sequence_offset,
                                  h.update_sequence_size, vp.sector_bytes) != OK)
                continue;
            std::vector<cr::ntfs::AttrView> attrs;
            if (cr::ntfs::enum_attributes(rec.data(), u32(vp.record_size), attrs) != OK)
                continue;
            ++report.records_scanned;
            for (const cr::ntfs::AttrView& a : attrs) {
                if (!a.nonresident)
                    continue;
                cr::ntfs::RunList rl;
                if (cr::ntfs::parse_data_runs(a, vp.total_clusters, rl) != OK)
                    continue;
                for (const cr::ntfs::DataRun& dr : rl.runs()) {
                    if (dr.sparse || dr.lcn == 0)
                        continue;
                    for (u64 lcn = dr.lcn; lcn < dr.lcn + dr.length; ++lcn) {
                        if (lcn >= vp.total_clusters)
                            break;
                        if (!bitmap->test(lcn)) {
                            /* free-but-allocated ("Bitmap mark it as
                             * free!!!", :3196-3209) */
                            push_error(report, CHECK_ORPHAN, lcn);
                        }
                        const size_t byte = size_t(lcn >> 3);
                        const u8 bit = u8(1u << (lcn & 7));
                        if (claimed[byte] & bit) {
                            /* double-claim ("Please use 'Chkdsk'
                             * first!!!", :3211-3224) */
                            push_error(report, CHECK_CROSSLINK, lcn);
                        } else {
                            claimed[byte] |= bit;
                        }
                    }
                }
            }
        }
    }
    return OK;
}

} // namespace fscheck
} // namespace cr
