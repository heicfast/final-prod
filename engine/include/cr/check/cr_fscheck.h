/*
 * cr_fscheck.h - the damage-audit check phase (M-041, diff-to-fix).
 *
 * The og's FileSystemCheck.mo is a chkdsk-grade READ-ONLY audit that runs
 * before/alongside scanning: CCheckFAT validates the DBR per-type, scans
 * both FAT copies for entry holes and duplicate references, compares the
 * copies entry-by-entry, walks the whole directory tree (cycles,
 * cross-links, `.`/`..`, size-vs-chain) and sweeps for orphan clusters;
 * CCheckNTFSVolume proves $MFT/$MFTMirr, gates the NTFS version
 * {1.2, 3.0, 3.1}, validates $Bitmap structurally, and cross-checks
 * every allocated MFT record's datarun LCNs against the bitmap
 * (free-but-allocated and double-claim). Failures feed the og's
 * escalation ladder (structured -> RawFixer -> WinChkdsk -> RAW
 * carving) and are user-visible as per-error reports.
 *
 * This module reproduces the audit as a pure read pass with the og's
 * ERROR TAXONOMY VERBATIM (numeric parity, 0x1001-0x100e). It NEVER
 * repairs and NEVER aborts a scan - the report is data.
 *
 * Corpus ground truth (verified first-hand this session):
 *   - FAT12/16/32 DBR parsers + cluster-count ceilings:
 *     FileSystemCheck_decompiled.c :1315-1471 (FAT12 spf*341.3333
 *     :1358-1360, FAT16 spf*0x100 :1415, FAT32 "FAT3"+'2'@+0x52 + spf
 *     gates + rootCluster range + (bps*spf32)>>2 :1440-1471), error
 *     0x100d.
 *   - FAT entry-hole check :4386-4411 (entry < clusterCount or in the
 *     reserved/BAD family), duplicate-reference scan :4337-4381 -> 0x1006.
 *   - Pairwise FAT-copy compare :689-781, mismatch -> 0x1005 (:749).
 *   - Dir audit :1772-2097: `.`/`..` validation :1893-1907 -> 0x1007;
 *     invalid first-cluster :1941-1948 -> 0x1007; size-vs-chain
 *     :2050-2076 -> 0x1004 (ceil(size/cluster) != chain clusters);
 *     directory cycle :2079-2097 -> 0x1007; cross-link vs the global
 *     visited array :1157-1167 -> 0x1002.
 *   - Orphan sweep :874-949: unvisited clusters with FAT != 0 and not
 *     BAD {0xff7,0xfff7,0xffffff7} -> 0x1003.
 *   - NTFS: volume-size gate :2832-2835 -> 0x1001; $MFT record-0
 *     $FILE_NAME resident + "$MFT" name :2900-2916 -> 0x1008; $MFTMirr
 *     same proof :2921-2971 -> 0x1009; version gate {0x102, 3, 0x301}
 *     :2994-3003 -> 0x100a; $Bitmap structural :3788-3942 -> 0x100b;
 *     MFT x $Bitmap cross-check :3196-3224 (free-but-allocated ->
 *     0x1003 "Bitmap mark it as free!!!", double-claim -> 0x1002
 *     "Please use 'Chkdsk' first!!!"); exception -> 0x100e :9295-9301.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_FSCHECK_H_
#define CR_FSCHECK_H_

#include "../cr_types.h"
#include "../storage/cr_device.h"
#include <string>
#include <vector>

namespace cr {
namespace fscheck {

/* The og's error taxonomy (numeric parity with the decompile). */
enum CheckCode : u32 {
    CHECK_SIZE_MISMATCH = 0x1001,   /* NTFS: the volume-size gate */
    CHECK_CROSSLINK = 0x1002,       /* FAT: two files claim one cluster;
                                      NTFS: a fresh-bitmap double-claim */
    CHECK_ORPHAN = 0x1003,          /* FAT: unvisited non-free cluster;
                                      NTFS: bitmap says free but allocated */
    CHECK_SIZE_NE_CHAIN = 0x1004,   /* FAT: file size != chain clusters */
    CHECK_FAT_COPIES_DIFFER = 0x1005, /* FAT1[i] != FAT2[i] */
    CHECK_CHAIN_INVALID = 0x1006,   /* FAT: entry hole / duplicate ref */
    CHECK_DENTRY_INVALID = 0x1007,  /* FAT: bad first cluster / `.`/`..` /
                                       directory cycle */
    CHECK_MFT_BAD = 0x1008,         /* NTFS: $MFT record-0 proof failed */
    CHECK_MFTMIRR_BAD = 0x1009,     /* NTFS: $MFTMirr proof failed */
    CHECK_VERSION_BAD = 0x100a,     /* NTFS: version not in {1.2,3.0,3.1} */
    CHECK_BITMAP_BAD = 0x100b,      /* NTFS: $Bitmap structural */
    CHECK_DBR_BAD = 0x100d,         /* FAT: DBR parse / gate failure */
    CHECK_EXCEPTION = 0x100e,       /* unexpected internal error */
};

inline const char* check_code_text(u32 code) {
    switch (code) {
    case CHECK_SIZE_MISMATCH: return "volume size gate";
    case CHECK_CROSSLINK: return "cross-link (double allocation)";
    case CHECK_ORPHAN: return "orphan cluster / bitmap-free-but-allocated";
    case CHECK_SIZE_NE_CHAIN: return "file size does not match chain";
    case CHECK_FAT_COPIES_DIFFER: return "FAT copies differ";
    case CHECK_CHAIN_INVALID: return "chain/FAT invalid (hole or duplicate)";
    case CHECK_DENTRY_INVALID: return "directory entry invalid";
    case CHECK_MFT_BAD: return "$MFT check failed";
    case CHECK_MFTMIRR_BAD: return "$MFTMirr check failed";
    case CHECK_VERSION_BAD: return "unsupported NTFS version";
    case CHECK_BITMAP_BAD: return "$Bitmap check failed";
    case CHECK_DBR_BAD: return "boot sector (DBR) check failed";
    case CHECK_EXCEPTION: return "internal check error";
    default: return "unknown";
    }
}

struct CheckError {
    u32 code = 0;
    u64 param = 0;            /* cluster / record / LCN context */
    std::u16string path_hint; /* best-effort path (dir audit; may be empty) */
};

struct FsCheckReport {
    std::vector<CheckError> errors;
    u64 clusters_visited = 0; /* FAT: clusters consumed by live chains */
    u64 entries_scanned = 0;  /* FAT: dentries examined */
    u64 records_scanned = 0;  /* NTFS: allocated MFT records examined */
    u64 fat_entries_scanned = 0;

    u32 first_code() const { return errors.empty() ? 0 : errors.front().code; }
    bool ok() const { return errors.empty(); }
};

/* Audit a FAT-family volume (flavor from the boot sector). Read-only:
 * returns OK with whatever the report found, or an ERR_* transport code
 * when the device cannot be read at all (a torn read inside a phase is
 * reported as CHECK_EXCEPTION, never as a hard error). */
u32 check_fat_volume(IStorageDevice* slice, FsCheckReport& report,
                     CancelToken* cancel = nullptr);

/* Audit an NTFS volume. Same contract. */
u32 check_ntfs_volume(IStorageDevice* slice, FsCheckReport& report,
                      CancelToken* cancel = nullptr);

} // namespace fscheck
} // namespace cr

#endif // CR_FSCHECK_H_
