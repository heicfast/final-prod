/*
 * cr_fscheck.h - the damage-audit check phase (M-041, diff-to-fix).
 *
 * Corpus contract (ghidra-mo/FileSystemCheck_decompiled.c, read line-by-line
 * and re-verified first-hand at corpus 03ad46c — every constant below carries
 * its decompile citation):
 *   CCheckFAT::Check FUN_1800015b0:
 *     - DBR validators per flavor FUN_180002a60/c20/dd0 (:1314-1475):
 *       FAT12 sig "FAT1"+'2'@+0x36, cluster ceiling spf*341.3333333
 *       (:1358-1360); FAT16 sig '6'@+0x3a, ceiling spf*0x100 (:1415);
 *       FAT32 sig "FAT3"+'2'@+0x52, spf16@+0x16 in {0, spf32}, root
 *       cluster in (1, tot/spc) (:1455), ceiling (bps*spf32)>>2 (:1471);
 *       SPC pow2 1..128, FAT count 1..4, tot16->tot32 fallback — failure
 *       = error 0x100d (:553/:560/:567)
 *     - volume-size floor: caller size < DBR-derived volume = 0x1001
 *       (:576-577)
 *     - per-copy table validation: dead-zone entries ([cluster_count,
 *       reserved-floor)) fail FUN_180005fd0 (:4392-4415) and duplicate
 *       references fail FUN_180005e90 (:4340-4386) = 0x1006 (:606/:616)
 *     - pairwise FAT-copy compare, entry-by-entry (:743-753) = 0x1005
 *     - FAT32 root-cluster chain walk FUN_1800027a0 fail = 0x1006
 *       (:803-824); entry-0 media signature mask 0xf8/0xfff8/0xffffff8
 *       (:667-684, :757-774)
 *     - dir-tree audit FUN_180003360 (:1772): '.'/'.. ' first-pair
 *       (:1890-1897), first-cluster == 1 or >= cluster space = 0x1007
 *       (:1937-1948), live file size != chain = 0x1004 (:2050-2076, the
 *       sector-sum vs round-up(ceil(size/bps), spc) compare), directory
 *       cycle (cluster in ancestor set) = 0x1007 (:2079-2097)
 *     - orphan sweep (:880-949): cluster unvisited && FAT != 0/self/BAD
 *       -> host decision FUN_180002770 -> report = 0x1003 (:949);
 *       BAD sentinels 0xff7/0xfff7/0xffffff7 (:874-876)
 *   CCheckNTFSVolume::Check FUN_1800045a0 (:2757):
 *     - $MFT record 0: parse + RESIDENT $FILE_NAME named "$MFT"
 *       (name_len 4 @+0x40, "$MFT" @+0x42, exact-length pin @+0x4a) =
 *       else 0x1008 (:2888-2916)
 *     - $MFTMirr slot 0: parse + resident "$MFT" $FILE_NAME = else
 *       0x1009 (:2944-2979)
 *     - NTFS version in {0x102, 3, 0x301} (1.2 / 3.0 / 3.1) = else 0x100a
 *       with the version word as param (:2994-3003)
 *     - $Bitmap load FUN_180005590: $DATA non-resident, bitmap bits >=
 *       MFT record count (:3866-3888) = else 0x100b; the og's slack term
 *       (CCheckNTFSVolume+0x30, :3869-3871) is layout-undecoded and
 *       registered in the finding's Resolution
 *     - MFT x $Bitmap cross-check FUN_180004d80 (:3061-3249): records
 *       gated by $MFT::$BITMAP (FUN_180005900 Read_MFT_Bitmap, :3138)
 *       AND record flags&1 (:3165); per non-resident attribute run, per
 *       LCN: bitmap bit clear = 0x1003 "Bitmap mark it as free"
 *       (:3196-3209); already-seen LCN = 0x1002 "Please use 'Chkdsk'
 *       first" (:3211-3224) — two records claiming one cluster
 *     - exception wrapper: 0x100e "please use Chkdsk /F" (:9295-9301)
 *   Escalation stance (rawfixer-fs-repair.md:59-65): structured scan ->
 *     RawFixer -> WinChkdsk -> RAW carving; the check phase NEVER aborts
 *     the scan (the orchestrator's pre-existing stance, cr_scan.cpp
 *     repair-stage comment).
 *
 * The host-callback contract mirrors the og's 8-event surface (SetRange
 * SetPos/OnError/stop-decision/status/path + the orphan decision
 * FUN_180002770): every hook is optional (null = silent / report /
 * continue).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_FSCHECK_H_
#define CR_FSCHECK_H_

#include "../cr_types.h"
#include "../fat/cr_fat_engine.h"
#include "../storage/cr_device.h"
#include <string>
#include <vector>

namespace cr {
namespace check {

/* ---- the og's error taxonomy (numeric parity, P23) ------------------
 * Values are FileSystemCheck.mo's own (0x1001..0x100e; 0x100c unused by
 * the og — every gap in the og's numbering is preserved). */
enum CheckError : u32 {
    CHK_VOLUME_TOO_SMALL = 0x1001, /* FAT: volume < DBR-derived size */
    CHK_DUPLICATE_ALLOC = 0x1002,  /* NTFS: two records claim one LCN */
    CHK_ORPHAN_CLUSTER = 0x1003,   /* FAT: orphan cluster; NTFS: bitmap-free
                                    * but record-allocated LCN */
    CHK_SIZE_CHAIN_MISMATCH = 0x1004, /* FAT: live file size != chain */
    CHK_FAT_COPY_MISMATCH = 0x1005,   /* FAT1 != FAT2 at an entry */
    CHK_FAT_TABLE_INVALID = 0x1006,   /* FAT dead-zone entry / duplicate
                                       * reference / root chain broken */
    CHK_DIR_TREE_INVALID = 0x1007,    /* '.'/'..' pair, first-cluster,
                                       * directory cycle */
    CHK_MFT_RECORD_INVALID = 0x1008,  /* $MFT record 0 not "$MFT"-named */
    CHK_MFTMIRR_INVALID = 0x1009,     /* $MFTMirr slot 0 unusable */
    CHK_NTFS_VERSION = 0x100A,        /* version not 1.2/3.0/3.1 */
    CHK_BITMAP_STRUCTURAL = 0x100B,   /* $Bitmap size contradiction */
    CHK_DBR_INVALID = 0x100D,         /* per-flavor DBR validator failed */
    CHK_CHECK_EXCEPTION = 0x100E,     /* catastrophic: "use Chkdsk /F" */
};

/* One reported issue. `param` carries the og's context word: cluster /
 * LCN / version — 0 when the og passes none. `path_hint` carries the
 * dir-audit path context (the og's "Name: %s\%s" log line). */
struct CheckIssue {
    u32 code = 0;
    u64 param = 0;
    std::string path_hint;
    u32 record_no = 0xFFFFFFFFu; /* NTFS cross-check: the claiming record */
};

struct FsCheckReport {
    std::vector<CheckIssue> errors; /* discovery order (og order) */
    /* summary counters (the og's *param_3 error count per phase) */
    u32 fat_copies_compared = 0;
    u32 fat_entries_checked = 0;
    u32 clusters_visited = 0;
    u32 mft_records_examined = 0;
    bool family_detected = false;
    char family[8] = {0}; /* "fat"/"ntfs"/"" */

    bool has_code(u32 code) const {
        for (const auto& e : errors)
            if (e.code == code)
                return true;
        return false;
    }
    const CheckIssue* first_of(u32 code) const {
        for (const auto& e : errors)
            if (e.code == code)
                return &e;
        return nullptr;
    }
};

/* ---- the og's host-callback surface (8 events) -----------------------
 * All optional. on_error returning false = the og's stop-decision path
 * (event 6, FUN_180004d80 :3152-3160): the check STOPS at the current
 * phase; whatever was reported so far stands. orphan_decision: the og's
 * FUN_180002770 host prompt — return true to report orphan clusters
 * (0x1003), false to skip them. Null = report (audit-honest default). */
struct CheckCallbacks {
    void (*set_range)(void* user, u64 lo, u64 hi) = nullptr;
    void (*set_pos)(void* user, u64 pos) = nullptr;
    bool (*on_error)(void* user, const CheckIssue& issue) = nullptr; /* false = stop */
    void (*status)(void* user, const char* text) = nullptr;
    void (*path)(void* user, const char* path) = nullptr;
    bool (*orphan_decision)(void* user) = nullptr; /* true = report */
    void* user = nullptr;
};

/* ---- the FAT pass ----------------------------------------------------
 * Self-contained (reads the boot sector itself; no engine state shared).
 * Phases in the og's order: DBR ceilings -> size floor -> per-copy table
 * validation (dead-zone + duplicate refs) -> pairwise copy compare ->
 * root chain -> dir-tree audit -> orphan sweep. Returns OK when the
 * audit completed (errors — if any — are in the report; the phase NEVER
 * fails the caller's scan), or an ERR_* transport code when the device
 * itself cannot be read at all (the og's 0x100e class; also reported as
 * CHK_CHECK_EXCEPTION when the boot sector is unreadable). */
u32 check_fat(IStorageDevice* slice, const CheckCallbacks& cb, FsCheckReport& out);

/* ---- the NTFS pass ----------------------------------------------------
 * $MFT proof -> $MFTMirr proof -> version gate -> $Bitmap structural ->
 * MFT x $Bitmap cross-check (gated by $MFT::$BITMAP + record in-use).
 * Same never-fail-the-scan contract as check_fat. */
u32 check_ntfs(IStorageDevice* slice, const CheckCallbacks& cb, FsCheckReport& out);

/* ---- dispatch ---------------------------------------------------------
 * Probes the family from the boot sector (OEM + BPB shape, the
 * orchestrator's probe rules narrowed to the check families) and runs
 * the matching pass. Unknown family: OK with an empty, undetected
 * report (the og's FileSystemCheck only knows FAT and NTFS volumes). */
u32 run_fscheck(IStorageDevice* slice, const CheckCallbacks& cb, FsCheckReport& out);

/* ---- session persistence (REC_FS_CHECK) --------------------------------
 * Wire: u32 count + per-issue {u32 code, u64 param, u32 record_no,
 * u16 path_len, path bytes utf-8} + the summary tail (u32 x4 + char[8] +
 * u32 detected). Round-trips through the CRSF container like every
 * other record. */
u32 serialize_fs_check(const FsCheckReport& rep, std::vector<u8>& out);
u32 deserialize_fs_check(const u8* data, size_t len, FsCheckReport& rep);

} // namespace check
} // namespace cr

#endif // CR_FSCHECK_H_
