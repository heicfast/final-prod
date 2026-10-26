/*
 * cr_fscheck.cpp - the damage-audit check phase (M-041, diff-to-fix).
 *
 * Re-implemented from understanding of ghidra-mo/FileSystemCheck_decompiled.c
 * (corpus 03ad46c; every gate cites its decompile line in the header). No
 * decompiled code is reproduced; the error taxonomy, phase order, thresholds
 * and termination semantics are the parity surface.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/check/cr_fscheck.h"

#include "cr/ntfs/cr_attributes.h"
#include "cr/ntfs/cr_bitmap.h"
#include "cr/ntfs/cr_bootstrap.h"
#include "cr/ntfs/cr_fixup.h"
#include "cr/ntfs/cr_ntfs_wire.h"
#include "cr/ntfs/cr_runlist.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace cr {
namespace check {

namespace {

/* ---------- small helpers ---------- */

inline u16 rd16(const u8* p) {
    return u16(p[0]) | (u16(p[1]) << 8);
}
inline u32 rd32(const u8* p) {
    return u32(p[0]) | (u32(p[1]) << 8) | (u32(p[2]) << 16) | (u32(p[3]) << 24);
}
inline u64 rd64(const u8* p) {
    return u64(rd32(p)) | (u64(rd32(p + 4)) << 32);
}

void emit(CheckIssue issue, const CheckCallbacks& cb, FsCheckReport& out, bool& stop) {
    out.errors.push_back(std::move(issue));
    if (cb.on_error && !cb.on_error(cb.user, out.errors.back()))
        stop = true; /* the og's stop-decision (event 6) */
}

void status(const CheckCallbacks& cb, const char* text) {
    if (cb.status)
        cb.status(cb.user, text);
}

/* The og's FAT flavor reserved floors (FUN_180005fd0/FUN_180005e90
 * local_50 tables, :4353-4355/:4398-4400): the dead-zone upper bound is
 * the value ONE BELOW the BAD sentinel. */
constexpr u32 FAT_RESERVED_FLOOR[3] = {0xFF6, 0xFFF6, 0xFFFFFF6};
/* The orphan sweep's BAD sentinels (:874-876). */
constexpr u32 FAT_BAD[3] = {0xFF7, 0xFFF7, 0xFFFFFF7};

/* Raw (uncanonicalized) FAT entry decode at flavor width. */
u32 fat_raw_entry(const std::vector<u8>& fat, u32 flavor, u32 cluster) {
    switch (flavor) {
    case 0: { /* FAT12 */
        const size_t off = size_t(cluster) + size_t(cluster) / 2;
        if (off + 2 > fat.size())
            return 0;
        u32 v = rd16(fat.data() + off);
        return (cluster & 1) ? (v >> 4) : (v & 0xFFF);
    }
    case 1: { /* FAT16 */
        const size_t off = size_t(cluster) * 2;
        if (off + 2 > fat.size())
            return 0;
        return rd16(fat.data() + off);
    }
    default: { /* FAT32 */
        const size_t off = size_t(cluster) * 4;
        if (off + 4 > fat.size())
            return 0;
        return rd32(fat.data() + off) & 0x0FFFFFFF;
    }
    }
}

/* Canonicalized entry (the chain-walk view: BAD/EOC sentinels folded to
 * the fat:: constants) — used for chain walking, never for the raw
 * pairwise copy compare. */
u32 fat_canon_entry(const std::vector<u8>& fat, u32 flavor, u32 cluster) {
    const u32 v = fat_raw_entry(fat, flavor, cluster);
    if (v == FAT_BAD[flavor])
        return fat::CHAIN_BAD;
    if (v >= (FAT_BAD[flavor] + 1))
        return fat::CHAIN_EOC_MIN;
    return v;
}

/* ---------- the FAT pass ---------- */

struct FatCheckCtx {
    IStorageDevice* slice = nullptr;
    CheckCallbacks cb;
    FsCheckReport* rep = nullptr;
    bool stop = false;

    u32 flavor = 1; /* 0/1/2 = FAT12/16/32 */
    u16 bps = 512;
    u32 spc = 1;
    u32 fat_count = 2;
    u32 spf = 1;          /* sectors per FAT (flavor's own field) */
    u64 fat_start = 0;    /* byte offset of FAT1 */
    u32 root_sectors = 0; /* FAT12/16 */
    u64 root_dir_off = 0; /* FAT12/16 byte offset */
    u32 root_cluster = 2; /* FAT32 */
    u64 total_sectors = 0;
    u64 cluster_count = 0; /* incl. the 2 reserved (clusters 2..cc+1) */
    u64 cluster_bytes = 0;
    u64 first_data_byte = 0;
    std::vector<u8> fat1, fat2; /* raw copies (fat2 empty when single) */
    std::vector<u8> visited;    /* byte per cluster (2..cc+1) */
};

bool read_bytes(IStorageDevice* dev, u64 off, void* buf, u32 len) {
    return dev->read_at(off, buf, len) == OK;
}

/* DBR validators — FUN_180002a60 (FAT12) / FUN_180002c20 (FAT16) /
 * FUN_180002dd0 (FAT32), cited per gate. Returns false -> 0x100d. */
bool fat_validate_dbr(FatCheckCtx& c, const u8* b) {
    /* signature gates: FAT12/16 "FAT1"+digit at +0x36/:3a; FAT32
     * "FAT3"+'2' at +0x52/:56 (validators' own memcmp gates). */
    if (c.flavor != 2) {
        if (rd32(b + 0x36) != 0x31544146) /* "FAT1" */
            return false;
        const char want = c.flavor == 0 ? '2' : '6';
        if (char(b[0x3A]) != want)
            return false;
    } else {
        if (rd32(b + 0x52) != 0x33544146) /* "FAT3" */
            return false;
        if (char(b[0x56]) != '2')
            return false;
    }
    /* sectors-per-cluster: the pow2 switch 1..128 (:1319-1327) */
    c.spc = b[0x0D];
    if (c.spc == 0 || (c.spc & (c.spc - 1)) != 0 || c.spc > 128)
        return false;
    /* FAT count: (byte)(n-1) < 4 -> 1..4 (:1328) */
    c.fat_count = b[0x10];
    if (c.fat_count < 1 || c.fat_count > 4)
        return false;
    c.bps = rd16(b + 0x0B);
    const u16 reserved = rd16(b + 0x0E);
    c.spf = c.flavor == 2 ? rd32(b + 0x24) : rd16(b + 0x16);
    if (c.spf == 0)
        return false;

    u64 root_entries = 0;
    if (c.flavor != 2) {
        root_entries = rd16(b + 0x11);
        /* root dir sectors: the og's `entries >> 4` (:1338/:1382 — 32 B
         * per entry over 512 B sectors; the check phase assumes 512 B
         * here, matching the decompile exactly). */
        c.root_sectors = u32(root_entries >> 4);
        c.total_sectors = rd32(b + 0x20);
        if (c.total_sectors == 0)
            c.total_sectors = rd16(b + 0x13); /* tot16 fallback (:1332) */
    } else {
        /* FAT32 extra gates (:1447-1455): spf16 == 0 or == spf32; root
         * cluster in (1, tot/spc). tot32 only (no 16-bit fallback). */
        const u16 spf16 = rd16(b + 0x16);
        if (spf16 != 0 && spf16 != c.spf)
            return false;
        c.root_cluster = rd32(b + 0x2C);
        c.total_sectors = rd32(b + 0x20);
        if (c.total_sectors == 0)
            return false;
        if (c.root_cluster <= 1 || c.root_cluster >= c.total_sectors / c.spc)
            return false;
    }
    if (c.total_sectors == 0)
        return false;

    /* cluster count: (tot - fat_count*spf - reserved - root_sectors)/spc
     * + 2  (:1349-1352/:1394-1397/:1470) */
    const u64 fat_span = u64(c.fat_count) * c.spf;
    if (c.total_sectors <= fat_span + reserved + c.root_sectors)
        return false;
    u64 cc = (c.total_sectors - fat_span - reserved - c.root_sectors) / c.spc + 2;

    /* per-flavor ceilings */
    if (c.flavor == 0) {
        const u64 cap = u64(double(c.spf) * 341.3333333); /* :1358-1360 */
        if (cap < cc)
            cc = cap;
    } else if (c.flavor == 1) {
        const u64 cap = u64(c.spf) * 0x100; /* :1415 */
        if (cap < cc)
            cc = cap;
    } else {
        const u64 cap = (u64(c.bps) * c.spf) >> 2; /* :1471 */
        if (cap < cc)
            cc = cap;
    }
    if (cc < 3)
        return false;

    c.cluster_count = cc;
    c.cluster_bytes = u64(c.spc) * c.bps;
    c.fat_start = u64(reserved) * c.bps;
    if (c.flavor != 2)
        c.root_dir_off = (u64(reserved) + fat_span) * c.bps;
    c.first_data_byte = (u64(reserved) + fat_span + c.root_sectors) * c.bps;
    return true;
}

/* Load one FAT copy (the og's per-copy read loop, :583-626). */
bool fat_load_copy(FatCheckCtx& c, u32 index, std::vector<u8>& out) {
    const u64 bytes = u64(c.spf) * c.bps;
    if (bytes == 0 || bytes > (64u << 20))
        return false;
    out.resize(size_t(bytes));
    return read_bytes(c.slice, c.fat_start + u64(index) * bytes, out.data(), u32(bytes));
}

/* Dead-zone entry check — FUN_180005fd0 (:4392-4415): entries 2..cc-1
 * whose value lands in [cc, reserved-floor) are neither valid links nor
 * the reserved family -> table invalid (0x1006). */
bool fat_entries_in_range(FatCheckCtx& c, const std::vector<u8>& fat) {
    const u32 floor = FAT_RESERVED_FLOOR[c.flavor];
    for (u32 e = 2; e + 1 < c.cluster_count; ++e) {
        const u32 v = fat_raw_entry(fat, c.flavor, e);
        if (v >= c.cluster_count && v <= floor)
            return false;
    }
    return true;
}

/* Duplicate-reference scan — FUN_180005e90 (:4340-4386): two entries
 * pointing at the same in-range cluster -> 0x1006. */
bool fat_no_duplicate_refs(FatCheckCtx& c, const std::vector<u8>& fat) {
    const u32 floor = FAT_RESERVED_FLOOR[c.flavor];
    std::vector<u8> seen(size_t(c.cluster_count) + 2, 0);
    for (u32 e = 2; e + 1 < c.cluster_count; ++e) {
        const u32 v = fat_raw_entry(fat, c.flavor, e);
        if (v == 0 || v > floor || v >= c.cluster_count)
            continue; /* free / reserved family / out of space */
        if (seen[size_t(v)])
            return false;
        seen[size_t(v)] = 1;
    }
    return true;
}

/* Chain walk over FAT1 (canonical entries). The og walks chains through
 * its (compared) first-copy table object. */
bool fat_chain(FatCheckCtx& c, u32 start, std::vector<u32>& chain) {
    chain.clear();
    if (start < 2 || start >= c.cluster_count + 2)
        return false;
    u32 cur = start;
    const u32 cap = u32(c.cluster_count) + 2;
    std::vector<u8> seen(size_t(c.cluster_count) + 2, 0);
    while (true) {
        /* cross-link (FUN_1800027a0 :1156-1173): entering a cluster the
         * context's shadow registry (+0x60) already holds — a previous
         * chain's traversal — reports 0x1002 with the cluster as param.
         * The walk continues (the og loops on; only the host callback's
         * stop-decision ends it, and our `visited` array is exactly the
         * registry's "already claimed" signal). */
        if (c.visited.size() == cap && c.visited[size_t(cur)] && !c.stop) {
            CheckIssue is;
            is.code = CHK_DUPLICATE_ALLOC; /* 0x1002 (the FAT dir-walk form) */
            is.param = cur;
            emit(std::move(is), c.cb, *c.rep, c.stop);
            if (c.stop)
                return false; /* the og's callback-stop ends the walk */
        }
        if (seen[size_t(cur)])
            return false; /* cycle */
        seen[size_t(cur)] = 1;
        chain.push_back(cur);
        const u32 v = fat_canon_entry(c.fat1, c.flavor, cur);
        if (v == fat::CHAIN_BAD)
            return false; /* broken mid-chain */
        if (v >= fat::CHAIN_EOC_MIN)
            return true; /* clean EOC */
        if (v < 2 || v >= cap)
            return false;
        cur = v;
        if (chain.size() > cap)
            return false;
    }
}

void fat_mark_visited(FatCheckCtx& c, const std::vector<u32>& chain) {
    for (u32 cl : chain)
        if (cl >= 2 && size_t(cl) < c.visited.size())
            c.visited[size_t(cl)] = 1;
}

/* Directory audit — FUN_180003360. `ancestors` carries the recursion
 * path's clusters (the og's CSimpleArray<uint> param_5; 0xffffffff at
 * the root). Returns false when the walk must stop (host stop-decision
 * or an og-terminal error). */
bool fat_audit_dir(FatCheckCtx& c, const std::vector<u32>& chain_or_root, bool root_fixed_area,
                   bool is_root, std::vector<u32> ancestors, const std::string& path) {
    if (c.stop)
        return false;
    if (c.cb.path)
        c.cb.path(c.cb.user, path.c_str());

    /* entries per cluster window (the og's local_130 = cluster_bytes/32
     * per cluster of the chain) */
    const size_t per_cluster = c.cluster_bytes / 32;
    std::vector<u32> chain;

    auto read_entry = [&](size_t idx, u8* out32) -> bool {
        if (root_fixed_area) {
            /* FAT12/16 fixed root area: contiguous bytes */
            const u64 off = c.root_dir_off + u64(idx) * 32;
            return read_bytes(c.slice, off, out32, 32);
        }
        const u32 cluster = chain_or_root[idx / per_cluster];
        const u64 off = c.first_data_byte + (u64(cluster) - 2) * c.cluster_bytes +
                        (u64(idx % per_cluster) * 32);
        return read_bytes(c.slice, off, out32, 32);
    };
    auto entry_count = [&]() -> size_t {
        if (root_fixed_area)
            return size_t(c.root_sectors) * (c.bps / 32);
        return chain_or_root.size() * per_cluster;
    };

    /* subdirectories must open with '.' and '..' (:1890-1897) and the
     * walk skips those two entries (:1907 local_130 -= 2). */
    size_t start_idx = 0;
    if (!is_root) {
        u8 e0[32], e1[32];
        if (!read_entry(0, e0) || !read_entry(1, e1)) {
            CheckIssue is;
            is.code = CHK_DIR_TREE_INVALID;
            is.path_hint = path;
            emit(std::move(is), c.cb, *c.rep, c.stop);
            return !c.stop;
        }
        const bool dot_ok = e0[0] == '.' && e0[1] == ' ';
        const bool dotdot_ok = e1[0] == '.' && e1[1] == '.' && e1[2] == ' ';
        if (!dot_ok || !dotdot_ok) {
            CheckIssue is;
            is.code = CHK_DIR_TREE_INVALID;
            is.path_hint = path;
            emit(std::move(is), c.cb, *c.rep, c.stop);
        }
        start_idx = 2;
    }

    for (size_t idx = start_idx; idx < entry_count(); ++idx) {
        if (c.stop)
            return false;
        u8 e[32];
        if (!read_entry(idx, e))
            break; /* unreadable window: og's read-fail path ends the dir */
        if (e[0] == fat::DENTRY_END)
            break; /* 0x00 end marker (:1920-1924) */
        if (e[0] == fat::DENTRY_DELETED)
            continue; /* deleted: not audited (live-tree audit) */
        if (e[11] == fat::ATTR_LFN)
            continue; /* LFN slot */
        if (e[0] == 0x20 && e[1] == 0 && e[11] == 0)
            break; /* the og's 0x20/zero-attr end form (:1922-1924) */

        const u32 first_cluster =
            c.flavor == 2 ? (u32(rd16(e + 20)) << 16) | rd16(e + 26) : rd16(e + 26);
        const u32 size = rd32(e + 28);
        const bool is_dir = (e[11] & fat::ATTR_DIRECTORY) != 0;

        /* name for path context (8.3 display form) */
        char nm[13];
        size_t nlen = 0;
        for (size_t i = 0; i < 11 && e[i] != ' '; ++i)
            nm[nlen++] = char(e[i]);
        if (e[8] != ' ' && e[9] != ' ') { /* has extension */
            nm[nlen++] = '.';
            for (size_t i = 8; i < 11 && e[i] != ' '; ++i)
                nm[nlen++] = char(e[i]);
        } else if (nlen > 8) {
            nlen = 8; /* base-name only, no space-run tail */
        }
        nm[nlen] = 0;
        std::string child = path.empty() ? nm : path + "\\" + nm;

        /* invalid first cluster: == 1 or >= cluster space (:1937-1948) */
        if (first_cluster != 0 && (first_cluster == 1 || first_cluster >= c.cluster_count + 2)) {
            CheckIssue is;
            is.code = CHK_DIR_TREE_INVALID;
            is.path_hint = child;
            emit(std::move(is), c.cb, *c.rep, c.stop);
            continue; /* per-entry: the walk continues */
        }

        if (first_cluster == 0)
            continue; /* empty file/dir: nothing to walk */

        if (is_dir) {
            /* cycle: the cluster already in the ancestor path (:2079-2097) */
            if (std::find(ancestors.begin(), ancestors.end(), first_cluster) != ancestors.end()) {
                CheckIssue is;
                is.code = CHK_DIR_TREE_INVALID;
                is.path_hint = child;
                emit(std::move(is), c.cb, *c.rep, c.stop);
                continue;
            }
            if (!fat_chain(c, first_cluster, chain)) {
                /* chain build failure -> host decision -> 0x1006
                 * (:2020-2045: FUN_1800027a0 fail + FUN_180002770) */
                const bool report = !c.cb.orphan_decision || c.cb.orphan_decision(c.cb.user);
                if (report) {
                    CheckIssue is;
                    is.code = CHK_FAT_TABLE_INVALID;
                    is.path_hint = child;
                    emit(std::move(is), c.cb, *c.rep, c.stop);
                }
                continue;
            }
            fat_mark_visited(c, chain);
            ancestors.push_back(first_cluster);
            if (!fat_audit_dir(c, chain, /*root_fixed_area=*/false, /*is_root=*/false, ancestors,
                               child))
                return false;
            ancestors.pop_back();
        } else {
            /* live file: size != chain length (:2050-2076). The og
             * compares sector sums: round_up(ceil(size/bps), spc) vs the
             * chain's sector total — cluster-granular equivalent:
             * ceil(size/cluster_bytes) vs chain length. */
            if (!fat_chain(c, first_cluster, chain)) {
                const bool report = !c.cb.orphan_decision || c.cb.orphan_decision(c.cb.user);
                if (report) {
                    CheckIssue is;
                    is.code = CHK_FAT_TABLE_INVALID;
                    is.path_hint = child;
                    emit(std::move(is), c.cb, *c.rep, c.stop);
                }
                continue;
            }
            fat_mark_visited(c, chain);
            const u64 expected = (size + c.cluster_bytes - 1) / c.cluster_bytes;
            if (expected != chain.size()) {
                CheckIssue is;
                is.code = CHK_SIZE_CHAIN_MISMATCH;
                is.path_hint = child;
                emit(std::move(is), c.cb, *c.rep, c.stop);
            }
        }
    }
    return !c.stop;
}

} // namespace

u32 check_fat(IStorageDevice* slice, const CheckCallbacks& cb, FsCheckReport& out) {
    if (slice == nullptr)
        return ERR_BAD_PARAMETER;
    FatCheckCtx c;
    c.slice = slice;
    c.cb = cb;
    c.rep = &out;
    std::snprintf(out.family, sizeof(out.family), "fat");
    out.family_detected = true;

    status(cb, "  Checking DBR");
    u8 boot[512];
    if (!read_bytes(slice, 0, boot, sizeof(boot))) {
        CheckIssue is;
        is.code = CHK_CHECK_EXCEPTION;
        emit(std::move(is), cb, out, c.stop);
        return OK; /* never fails the caller's scan (0x100e class) */
    }
    /* flavor from the boot signature (the DBR validators' own gates) */
    if (rd32(boot + 0x36) == 0x31544146 && char(boot[0x3A]) == '2')
        c.flavor = 0;
    else if (rd32(boot + 0x36) == 0x31544146 && char(boot[0x3A]) == '6')
        c.flavor = 1;
    else if (rd32(boot + 0x52) == 0x33544146 && char(boot[0x56]) == '2')
        c.flavor = 2;
    else {
        out.family_detected = false;
        out.family[0] = 0;
        return OK; /* not a check-FAT volume */
    }

    if (!fat_validate_dbr(c, boot)) {
        CheckIssue is;
        is.code = CHK_DBR_INVALID;
        emit(std::move(is), cb, out, c.stop); /* terminal in the og (:553-567) */
        return OK;
    }
    /* volume-size floor: device < DBR-derived volume -> 0x1001, NON-terminal
     * (:576-577 — the check continues). */
    if (slice->size() < c.total_sectors * c.bps) {
        CheckIssue is;
        is.code = CHK_VOLUME_TOO_SMALL;
        emit(std::move(is), cb, out, c.stop);
    }
    if (c.stop)
        return OK;

    /* ---- per-copy load + table validation (0x1006, terminal) ---- */
    status(cb, "  Checking FAT");
    if (cb.set_range)
        cb.set_range(cb.user, 0, c.fat_count * 100);
    for (u32 i = 0; i < c.fat_count && !c.stop; ++i) {
        std::vector<u8> copy;
        if (!fat_load_copy(c, i, copy) || !fat_entries_in_range(c, copy) ||
            !fat_no_duplicate_refs(c, copy)) {
            CheckIssue is;
            is.code = CHK_FAT_TABLE_INVALID;
            emit(std::move(is), cb, out, c.stop); /* terminal (:606/:616) */
            return OK;
        }
        if (i == 0)
            c.fat1 = std::move(copy);
        else if (i == 1)
            c.fat2 = std::move(copy);
        if (cb.set_pos)
            cb.set_pos(cb.user, i * 100 + 50);
    }
    if (c.stop)
        return OK;

    /* ---- pairwise copy compare (0x1005, terminal) ---- */
    if (c.fat_count >= 2 && !c.fat2.empty()) {
        status(cb, "  Comparing FAT");
        out.fat_copies_compared = 2;
        const u32 floor = FAT_RESERVED_FLOOR[c.flavor];
        for (u32 e = 0; e < c.cluster_count + 2; ++e) {
            const u32 a = fat_raw_entry(c.fat1, c.flavor, e);
            const u32 b = fat_raw_entry(c.fat2, c.flavor, e);
            out.fat_entries_checked = e + 1;
            if (a != b) {
                CheckIssue is;
                is.code = CHK_FAT_COPY_MISMATCH;
                is.param = e;
                emit(std::move(is), cb, out, c.stop); /* terminal (:748-752) */
                return OK;
            }
            if (e == 0) {
                /* entry-0 media signature (:667-684/:754-775): mismatch
                 * ends the check with NO error code (og parity — the
                 * table object is unusable, the check bails silently). */
                const bool sig_ok = c.flavor == 0   ? (a & 0xF8) == 0xF8
                                    : c.flavor == 1 ? (a & 0xFFF8) == 0xFFF8
                                                    : (a & 0xFFFFFF8) == 0xFFFFFF8;
                if (!sig_ok)
                    return OK;
            }
        }
        (void)floor;
    }

    /* ---- root chain (FAT32) / root area (FAT12/16) ---- */
    status(cb, "  Checking Cluster Chain");
    c.visited.assign(size_t(c.cluster_count) + 2, 0);
    std::vector<u32> root_chain;
    bool root_ok = true;
    if (c.flavor == 2) {
        root_ok = fat_chain(c, c.root_cluster, root_chain);
        if (root_ok)
            fat_mark_visited(c, root_chain);
        else {
            CheckIssue is;
            is.code = CHK_FAT_TABLE_INVALID; /* terminal (:803-824) */
            emit(std::move(is), cb, out, c.stop);
            return OK;
        }
    }

    /* ---- directory-tree audit (per-entry errors, non-terminal) ---- */
    status(cb, "  Checking Directory Tree");
    if (cb.set_range)
        cb.set_range(cb.user, 0, c.cluster_count);
    /* the volume ROOT (fixed area or the FAT32 root chain) never takes
     * the '.'/'..' pair check (the og's 0xffffffff root sentinel) */
    fat_audit_dir(c, root_chain, /*root_fixed_area=*/c.flavor != 2, /*is_root=*/true, {}, "");
    if (c.stop)
        return OK;

    /* ---- orphan sweep (:880-949): first unvisited non-free non-self
     * non-BAD cluster -> host decision -> single 0x1003, terminal. ---- */
    status(cb, "  Checking Orphan Clusters");
    for (u32 cl = 2; cl < c.cluster_count + 2; ++cl) {
        if (c.visited[cl])
            continue;
        const u32 v = fat_canon_entry(c.fat1, c.flavor, cl);
        if (v == 0 || v == cl || v == fat::CHAIN_BAD)
            continue;
        const bool report = !c.cb.orphan_decision || c.cb.orphan_decision(cb.user);
        if (report) {
            CheckIssue is;
            is.code = CHK_ORPHAN_CLUSTER;
            is.param = cl;
            emit(std::move(is), cb, out, c.stop); /* terminal (:949) */
        }
        return OK;
    }
    out.clusters_visited = 0;
    for (u8 v : c.visited)
        out.clusters_visited += v;
    return OK;
}

/* ================= the NTFS pass ================= */

namespace {

/* A minimal MFT record reader over record 0's $DATA runlist (what the
 * og's CNTFSLib gives its check classes). */
struct NtfsCheckCtx {
    IStorageDevice* slice = nullptr;
    CheckCallbacks cb;
    FsCheckReport* rep = nullptr;
    bool stop = false;

    ntfs::VolumeParams p;
    ntfs::RunList mft_runs;
    u64 mft_records = 0;    /* allocated $MFT size / record size */
    u64 mft_bytes = 0;      /* allocated $MFT size */
    std::vector<u8> bitmap; /* $Bitmap $DATA bytes */
    u64 bitmap_bits = 0;
    std::vector<u8> mft_bmp; /* $MFT::$BITMAP resident value */
    std::vector<u8> seen;    /* cross-check LCN dedup bitmap */

    bool record_offset(u64 rec, u64& off) const {
        if (p.record_size == 0)
            return false;
        const u64 vcn = rec * p.record_size / p.cluster_bytes;
        for (const ntfs::DataRun& r : mft_runs.runs()) {
            if (r.sparse)
                continue;
            if (vcn >= r.vcn && vcn < r.vcn + r.length) {
                off = (r.lcn + (vcn - r.vcn)) * p.cluster_bytes +
                      (rec * p.record_size) % p.cluster_bytes;
                return true;
            }
        }
        return false;
    }
    bool read_record(u64 rec, std::vector<u8>& buf) {
        u64 off = 0;
        if (!record_offset(rec, off))
            return false;
        buf.resize(size_t(p.record_size));
        if (!read_bytes(slice, off, buf.data(), u32(p.record_size)))
            return false;
        ntfs::RecordHeader rh;
        std::memcpy(&rh, buf.data(), sizeof(rh));
        if (rh.magic != ntfs::RECORD_MAGIC_FILE)
            return false;
        if (ntfs::fixup_validate(buf.data(), u32(p.record_size), rh.update_sequence_offset,
                                 rh.update_sequence_size, u16(p.sector_bytes)) != OK)
            return false;
        return ntfs::fixup_apply(buf.data(), u32(p.record_size), rh.update_sequence_offset,
                                 rh.update_sequence_size, u16(p.sector_bytes)) == OK;
    }
    bool mft_in_use(u64 rec) const {
        if (mft_bmp.empty())
            return false;
        const size_t byte = size_t(rec / 8);
        if (byte >= mft_bmp.size())
            return false;
        return (mft_bmp[byte] & (1u << (rec & 7))) != 0;
    }
    bool cluster_in_use(u64 lcn) const {
        if (lcn / 8 >= bitmap.size())
            return false;
        return (bitmap[size_t(lcn / 8)] & (1u << (lcn & 7))) != 0;
    }
    void seen_set(u64 lcn) {
        if (lcn / 8 >= seen.size())
            return;
        seen[size_t(lcn / 8)] |= u8(1u << (lcn & 7));
    }
    bool seen_test(u64 lcn) const {
        if (lcn / 8 >= seen.size())
            return false;
        return (seen[size_t(lcn / 8)] & (1u << (lcn & 7))) != 0;
    }
};

/* record 0's resident $FILE_NAME must be "$MFT" (0x1008) / the mirror's
 * slot-0 record the same (0x1009). */
bool record_named_mft(std::vector<u8>& buf, u32 recsize) {
    std::vector<ntfs::AttrView> attrs;
    if (ntfs::enum_attributes(buf.data(), recsize, attrs) != OK)
        return false;
    for (const auto& a : attrs) {
        if (a.type != ntfs::ATTR_TYPE_FILE_NAME)
            continue;
        if (a.nonresident)
            return false; /* the og: non-resident $FILE_NAME fails */
        ntfs::FileNameInfo fi;
        if (ntfs::parse_file_name(a, fi) != OK)
            return false;
        return fi.name == u"$MFT"; /* first $FILE_NAME decides */
    }
    return false;
}

} // namespace

u32 check_ntfs(IStorageDevice* slice, const CheckCallbacks& cb, FsCheckReport& out) {
    if (slice == nullptr)
        return ERR_BAD_PARAMETER;
    NtfsCheckCtx c;
    c.slice = slice;
    c.cb = cb;
    c.rep = &out;
    std::snprintf(out.family, sizeof(out.family), "ntfs");
    out.family_detected = true;

    /* boot + geometry (the og's CheckDBR precondition) */
    ntfs::BootSector bs;
    std::memset(&bs, 0, sizeof(bs));
    if (!read_bytes(slice, 0, &bs, sizeof(bs)) ||
        ntfs::validate_boot_sector(bs, slice->geometry(), c.p) != OK) {
        CheckIssue is;
        is.code = CHK_CHECK_EXCEPTION; /* 0x100e: unusable volume shape */
        emit(std::move(is), cb, out, c.stop);
        return OK;
    }

    /* ---- $MFT record 0 proof (0x1008, terminal :2888-2916) ---- */
    status(cb, "  Checking $MFT");
    std::vector<u8> rec0;
    {
        const u64 off = c.p.mft_lcn * c.p.cluster_bytes;
        rec0.resize(size_t(c.p.record_size));
        if (!read_bytes(slice, off, rec0.data(), u32(c.p.record_size))) {
            CheckIssue is;
            is.code = CHK_MFT_RECORD_INVALID;
            emit(std::move(is), cb, out, c.stop);
            return OK;
        }
        ntfs::RecordHeader rh;
        std::memcpy(&rh, rec0.data(), sizeof(rh));
        if (rh.magic != ntfs::RECORD_MAGIC_FILE ||
            ntfs::fixup_validate(rec0.data(), u32(c.p.record_size), rh.update_sequence_offset,
                                 rh.update_sequence_size, u16(c.p.sector_bytes)) != OK ||
            ntfs::fixup_apply(rec0.data(), u32(c.p.record_size), rh.update_sequence_offset,
                              rh.update_sequence_size, u16(c.p.sector_bytes)) != OK ||
            !record_named_mft(rec0, u32(c.p.record_size))) {
            CheckIssue is;
            is.code = CHK_MFT_RECORD_INVALID;
            emit(std::move(is), cb, out, c.stop);
            return OK;
        }
    }

    /* the $MFT stream: record 0's non-resident $DATA runlist (the og's
     * m_ntfsMft.Init — the check's own reader, no engine dependency). */
    {
        std::vector<ntfs::AttrView> attrs;
        bool have_runs = false;
        if (ntfs::enum_attributes(rec0.data(), u32(c.p.record_size), attrs) == OK) {
            for (const auto& a : attrs) {
                if (a.type == ntfs::ATTR_TYPE_BITMAP && !a.nonresident) {
                    const u8* d = nullptr;
                    u32 len = 0;
                    if (ntfs::resident_span(a, d, len) == OK && d && len)
                        c.mft_bmp.assign(d, d + len);
                }
                if (a.type == ntfs::ATTR_TYPE_DATA && a.nonresident && a.name_length == 0) {
                    if (ntfs::parse_data_runs(a, c.p.total_clusters, c.mft_runs) == OK) {
                        have_runs = true;
                        c.mft_bytes = a.allocated;
                    }
                }
            }
        }
        if (!have_runs || c.mft_bytes == 0) {
            /* resident or absent $MFT $DATA: the og's MFT init failure —
             * the check cannot proceed past the proofs. */
            CheckIssue is;
            is.code = CHK_MFT_RECORD_INVALID;
            emit(std::move(is), cb, out, c.stop);
            return OK;
        }
        c.mft_records = c.mft_bytes / c.p.record_size;
    }

    /* ---- $MFTMirr slot-0 proof (0x1009, NON-terminal :2944-2979) ---- */
    status(cb, "  Checking $MFTMirr");
    {
        std::vector<u8> mirr;
        const u64 off = c.p.mftmirr_lcn * c.p.cluster_bytes;
        mirr.resize(size_t(c.p.record_size));
        bool ok = read_bytes(slice, off, mirr.data(), u32(c.p.record_size));
        if (ok) {
            ntfs::RecordHeader rh;
            std::memcpy(&rh, mirr.data(), sizeof(rh));
            ok = rh.magic == ntfs::RECORD_MAGIC_FILE &&
                 ntfs::fixup_validate(mirr.data(), u32(c.p.record_size), rh.update_sequence_offset,
                                      rh.update_sequence_size, u16(c.p.sector_bytes)) == OK &&
                 ntfs::fixup_apply(mirr.data(), u32(c.p.record_size), rh.update_sequence_offset,
                                   rh.update_sequence_size, u16(c.p.sector_bytes)) == OK &&
                 record_named_mft(mirr, u32(c.p.record_size));
        }
        if (!ok) {
            CheckIssue is;
            is.code = CHK_MFTMIRR_INVALID;
            emit(std::move(is), cb, out, c.stop);
            if (c.stop)
                return OK;
        }
    }

    /* ---- NTFS version gate (0x100A, NON-terminal :2994-3003) ---- */
    status(cb, "  Checking NTFS Version");
    {
        /* $Volume (record 3) $VOLUME_INFORMATION (0x70): major/minor at
         * value +8/+9. Valid words: 0x102 (1.2), 3 (3.0), 0x301 (3.1). */
        u32 word = 0;
        bool have = false;
        std::vector<u8> vol;
        if (c.read_record(3, vol)) {
            std::vector<ntfs::AttrView> attrs;
            if (ntfs::enum_attributes(vol.data(), u32(c.p.record_size), attrs) == OK) {
                for (const auto& a : attrs) {
                    if (a.type != ntfs::ATTR_TYPE_VOLUME_INFORMATION || a.nonresident)
                        continue;
                    const u8* d = nullptr;
                    u32 len = 0;
                    if (ntfs::resident_span(a, d, len) == OK && d && len >= 10) {
                        word = (u32(d[8]) << 8) | u32(d[9]);
                        have = true;
                    }
                    break;
                }
            }
        }
        if (!have || (word != 0x102 && word != 3 && word != 0x301)) {
            CheckIssue is;
            is.code = CHK_NTFS_VERSION;
            is.param = word; /* the og passes the version word (:3003) */
            emit(std::move(is), cb, out, c.stop);
            if (c.stop)
                return OK;
        }
    }

    /* ---- $Bitmap structural (0x100B) + load ---- */
    status(cb, "  Checking $Bitmap");
    {
        std::vector<u8> bmp_rec;
        bool loaded = false;
        if (c.read_record(6, bmp_rec)) {
            std::vector<ntfs::AttrView> attrs;
            if (ntfs::enum_attributes(bmp_rec.data(), u32(c.p.record_size), attrs) == OK) {
                for (const auto& a : attrs) {
                    if (a.type != ntfs::ATTR_TYPE_DATA || a.name_length != 0)
                        continue;
                    if (!a.nonresident) {
                        /* the og: "The $Bitmap::$DATA stream is resident!
                         * Currently, we do not support it!" -> load fails
                         * -> 0x100b, cross-check skipped. */
                        break;
                    }
                    ntfs::RunList rl;
                    if (ntfs::parse_data_runs(a, c.p.total_clusters, rl) != OK)
                        break;
                    c.bitmap_bits = a.allocated * 8;
                    /* structural floor (:3866-3888): bits >= MFT record
                     * count. (The og's slack term at :3869-3871 —
                     * CCheckNTFSVolume+0x30 — is layout-undecoded; on
                     * every well-formed volume the bit count equals the
                     * cluster count and no fixed slack constant can fire
                     * without flagging healthy volumes; registered in the
                     * M-041 Resolution, not guessed here.) */
                    if (c.bitmap_bits < c.mft_records) {
                        CheckIssue is;
                        is.code = CHK_BITMAP_STRUCTURAL;
                        emit(std::move(is), cb, out, c.stop);
                        if (c.stop)
                            return OK;
                    }
                    c.bitmap.resize(size_t(a.allocated));
                    u64 got = 0;
                    bool ok = true;
                    for (const ntfs::DataRun& r : rl.runs()) {
                        if (r.sparse)
                            continue;
                        const u64 bytes = r.length * c.p.cluster_bytes;
                        if (got + bytes > c.bitmap.size()) {
                            ok = false; /* runs overshoot the declared size */
                            break;
                        }
                        if (!read_bytes(slice, r.lcn * c.p.cluster_bytes,
                                        c.bitmap.data() + size_t(got), u32(bytes))) {
                            ok = false;
                            break;
                        }
                        got += bytes;
                    }
                    /* a partial load must NOT pass as loaded: the
                     * zero-filled tail would read as "every cluster
                     * free" and fire false 0x1003 verdicts. */
                    if (ok && got == a.allocated)
                        loaded = !c.bitmap.empty();
                    break;
                }
            }
        }
        if (!loaded) {
            /* unusable $Bitmap: the og logs LoadVolumeBitmap failed and
             * the check ends here (no cross-check without the bitmap). */
            if (!out.has_code(CHK_BITMAP_STRUCTURAL)) {
                CheckIssue is;
                is.code = CHK_BITMAP_STRUCTURAL;
                emit(std::move(is), cb, out, c.stop);
            }
            return OK;
        }
    }

    /* ---- MFT x $Bitmap cross-check (FUN_180004d80 :3061-3249) ---- */
    status(cb, "  Cross-checking MFT against $Bitmap");
    if (c.mft_bmp.empty()) {
        /* Read_MFT_Bitmap failed -> the og skips the cross-check
         * (:3130-3136, the else-branch). */
        return OK;
    }
    c.seen.assign(size_t((c.p.total_clusters + 7) / 8), 0);
    if (cb.set_range)
        cb.set_range(cb.user, 0, c.mft_records);
    for (u64 r = 0; r < c.mft_records && !c.stop; ++r) {
        if (cb.set_pos && (r & 0xFF) == 0)
            cb.set_pos(cb.user, r);
        if (!c.mft_in_use(r))
            continue; /* the $MFT::$BITMAP gate (:3138) */
        std::vector<u8> rec;
        if (!c.read_record(r, rec)) {
            out.mft_records_examined++;
            continue; /* unreadable record: the og logs and skips */
        }
        ntfs::RecordHeader rh;
        std::memcpy(&rh, rec.data(), sizeof(rh));
        if ((rh.flags & 0x01) == 0)
            continue; /* the record-flags in-use gate (:3165) */
        out.mft_records_examined++;
        std::vector<ntfs::AttrView> attrs;
        if (ntfs::enum_attributes(rec.data(), u32(c.p.record_size), attrs) != OK)
            continue;
        for (const auto& a : attrs) {
            if (!a.nonresident)
                continue; /* every non-resident attribute's runs (:3167) */
            ntfs::RunList rl;
            if (ntfs::parse_data_runs(a, c.p.total_clusters, rl) != OK)
                continue;
            for (const ntfs::DataRun& run : rl.runs()) {
                if (run.sparse)
                    continue; /* the og's -1 LCN skip (:3190) */
                for (u64 i = 0; i < run.length; ++i) {
                    const u64 lcn = run.lcn + i;
                    if (!c.cluster_in_use(lcn)) {
                        /* "Bitmap mark it as free!!!" (:3196-3209) */
                        CheckIssue is;
                        is.code = CHK_ORPHAN_CLUSTER;
                        is.param = lcn;
                        is.record_no = u32(r);
                        emit(std::move(is), cb, out, c.stop);
                        if (c.stop)
                            return OK;
                    } else if (c.seen_test(lcn)) {
                        /* "Please use 'Chkdsk' first!!!" (:3211-3224) */
                        CheckIssue is;
                        is.code = CHK_DUPLICATE_ALLOC;
                        is.param = lcn;
                        is.record_no = u32(r);
                        emit(std::move(is), cb, out, c.stop);
                        if (c.stop)
                            return OK;
                    }
                    c.seen_set(lcn);
                }
            }
        }
    }
    return OK;
}

u32 run_fscheck(IStorageDevice* slice, const CheckCallbacks& cb, FsCheckReport& out) {
    if (slice == nullptr)
        return ERR_BAD_PARAMETER;
    u8 boot[512];
    if (!read_bytes(slice, 0, boot, sizeof(boot)))
        return OK; /* nothing checkable */
    /* family probe: the check module's own dispatch (the og's
     * FileSystemCheck constructs CCheckFAT/CCheckNTFSVolume by volume
     * type; the DBR validators themselves gate on these signatures). */
    if (std::memcmp(boot + 3, "NTFS    ", 8) == 0)
        return check_ntfs(slice, cb, out);
    const bool fat12_16 =
        rd32(boot + 0x36) == 0x31544146 && (char(boot[0x3A]) == '2' || char(boot[0x3A]) == '6');
    const bool fat32 = rd32(boot + 0x52) == 0x33544146 && char(boot[0x56]) == '2';
    if (fat12_16 || fat32)
        return check_fat(slice, cb, out);
    return OK; /* unknown family: undetected, empty report */
}

/* ---- session persistence (REC_FS_CHECK) ---- */

u32 serialize_fs_check(const FsCheckReport& rep, std::vector<u8>& out) {
    out.clear();
    out.reserve(16 + rep.errors.size() * 24);
    auto put32 = [&](u32 v) {
        out.push_back(u8(v));
        out.push_back(u8(v >> 8));
        out.push_back(u8(v >> 16));
        out.push_back(u8(v >> 24));
    };
    auto put64 = [&](u64 v) {
        put32(u32(v));
        put32(u32(v >> 32));
    };
    put32(u32(rep.errors.size()));
    for (const auto& e : rep.errors) {
        put32(e.code);
        put64(e.param);
        put32(e.record_no);
        put32(u32(e.path_hint.size()));
        for (char ch : e.path_hint)
            out.push_back(u8(ch));
    }
    put32(rep.fat_copies_compared);
    put32(rep.fat_entries_checked);
    put32(rep.clusters_visited);
    put32(rep.mft_records_examined);
    put32(rep.family_detected ? 1 : 0);
    for (char ch : rep.family) {
        out.push_back(u8(ch));
        if (ch == 0)
            break;
    }
    while (out.size() % 8)
        out.push_back(0);
    return OK;
}

u32 deserialize_fs_check(const u8* data, size_t len, FsCheckReport& rep) {
    rep = FsCheckReport{};
    if (data == nullptr || len < 4)
        return ERR_BAD_PARAMETER;
    size_t pos = 0;
    auto get32 = [&](u32& v) -> bool {
        if (pos + 4 > len)
            return false;
        v = rd32(data + pos);
        pos += 4;
        return true;
    };
    auto get64 = [&](u64& v) -> bool {
        u32 lo = 0, hi = 0;
        if (!get32(lo) || !get32(hi))
            return false;
        v = u64(lo) | (u64(hi) << 32);
        return true;
    };
    u32 count = 0;
    if (!get32(count))
        return ERR_BAD_PARAMETER;
    rep.errors.reserve(std::min<u32>(count, 1u << 16));
    for (u32 i = 0; i < count; ++i) {
        CheckIssue e;
        u32 plen = 0;
        if (!get32(e.code) || !get64(e.param) || !get32(e.record_no) || !get32(plen) ||
            pos + plen > len)
            return ERR_BAD_PARAMETER;
        e.path_hint.assign(reinterpret_cast<const char*>(data + pos), plen);
        pos += plen;
        rep.errors.push_back(std::move(e));
    }
    u32 detected = 0;
    if (!get32(rep.fat_copies_compared) || !get32(rep.fat_entries_checked) ||
        !get32(rep.clusters_visited) || !get32(rep.mft_records_examined) || !get32(detected))
        return ERR_BAD_PARAMETER;
    rep.family_detected = detected != 0;
    size_t f = 0;
    while (pos < len && f + 1 < sizeof(rep.family) && data[pos] != 0)
        rep.family[f++] = char(data[pos++]);
    rep.family[f] = 0;
    return OK;
}

} // namespace check
} // namespace cr
