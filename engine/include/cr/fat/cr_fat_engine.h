/*
 * cr_fat_engine.h - FAT12/16/32 engine: table model, chain walker, LFN.
 *
 * Corpus mapping (fat-exfat v1+v2):
 *   - IFATTable polymorphism -> FatTable with flavor variants; canonical
 *     BAD = 0x0FFFFFF7, EOC >= 0x0FFFFFF8 (corpus constants)
 *   - mirrored FAT (CDoubleFATTable): primary + mirror, choose-healthy
 *   - chain walker: visited-range cycle guard; whole-run batched reads;
 *     cluster->sector = (c-2)*spc + first_data_sector
 *   - LFN: VFAT slots assembled before the 8.3 entry; checksum-validated
 *   - deleted dentry marker 0xE5; '.' self-entry anchors directory identity
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_FAT_ENGINE_H_
#define CR_FAT_ENGINE_H_

#include "../storage/cr_device.h"
#include "../tree/cr_node.h"
#include "cr_fat_wire.h"
#include <memory>
#include <vector>

namespace cr {
namespace fat {

/* ---- volume parameters (from BPB) ---- */
enum class Flavor : u8 { Fat12 = 0, Fat16 = 1, Fat32 = 2 };

struct FatParams {
    Flavor flavor = Flavor::Fat32;
    u16 bytes_per_sector = 512;
    u8 sectors_per_cluster = 1;
    u32 cluster_bytes = 0;
    u16 reserved_sectors = 1;
    u8 fat_count = 2;
    u32 fat_sectors = 0;       /* per FAT */
    u64 fat_start = 0;         /* byte offset of FAT1 */
    u64 fat2_start = 0;        /* byte offset of FAT2 (0 if single) */
    u32 root_entries = 0;      /* FAT12/16: fixed root dir entries */
    u64 root_dir_offset = 0;   /* FAT12/16: byte offset of root area */
    u32 root_cluster = 2;      /* FAT32 */
    u64 first_data_sector = 0; /* sector index */
    u32 cluster_count = 0;     /* data clusters (incl. 2 reserved) */
    u64 total_bytes = 0;
};

/* Flavor discrimination. Boundary constants are the og's FATSearchDLL
 * FUN_180007e50 :5219-5231 brackets (N-010a, diff-to-fix): FAT12
 * [0, 0xFF5), FAT16 [0xFF5, 0xFFF4) upper-exclusive, FAT32 [0xFFF4, ...)
 * lower-inclusive - a 65524-data-cluster volume is FAT32 on the og and
 * was FAT16 on the core's old `< 65525` boundary. */
Flavor flavor_from_clusters(u32 cluster_count_data);

/* Parse + validate a FAT boot sector. Accepts FAT12/16/32 layouts.
 * SPC is gated to the og's engine set {1,2,4,8,16,32,64,128} (FATSearchDLL
 * FUN_18002f970 :38050-52 bitmask 0x800000008000808b + the 0x80 special
 * case; PartitionRecoveryD FUN_1800048f0/4a40/4b70 use the same set in
 * the deep-scan gates - N-010b/N-034). */
u32 validate_boot(const u8* sector512, u64 device_bytes, FatParams& out);

/* Engine-open gates (N-010b, diff-to-fix): the og's FATSearchDLL boot
 * validator FUN_18002f970 + its quick-search consumer FUN_180021000
 * (:26318 bails on flavor 0xE). Layered on top of validate_boot's
 * structural parse:
 *   - numFATs in {1,2}          (:38049 `(byte)(n-1) < 2`)
 *   - bps == device sector size  (:38055 `*(ushort*)(boot+0xb) == dev+0xc`;
 *     skipped when device_sector == 0 - unknown geometry)
 *   - FAT16: "FAT16   " at +0x36 (_strnicmp), root_entries >= 224
 *     (:38054 `0xdf < root`), root_entries*32 % bps == 0 (:38055),
 *     spf16 != 0 (:38056)
 *   - FAT32: "FAT32   " at +0x52 (_strnicmp), spf32 != 0 (:38062 block)
 *   - FAT12: NO string/geometry gate - the og's validator has no FAT12
 *     branch (its quick engine declines them); the core keeps FAT12
 *     recovery as the documented I-002-family superset (PARITY-DELTAS).
 * Returns OK or ERR_BAD_PARAMETER. */
u32 boot_passes_open_gates(const u8* sector512, u32 device_sector, const FatParams& p);

/* Lost-scan boot gates (N-034, diff-to-fix): the og PartitionRecoveryD
 * deep-scan ladder FUN_180004610 :2516-2606 - bps must be a 512 multiple
 * (:2547 `(bps & 0x1ff) == 0`), cluster count via the BPB, then the
 * bracket-ordered gate walk (< 0xFF5: FAT12,FAT16,FAT32; < 0xFFF5:
 * FAT16,FAT32,FAT12; < 0xFFFFFF0: FAT32,FAT16,FAT12):
 *   - FAT12 gate FUN_1800048f0: numFATs != 0, a total-sector field
 *     nonzero, SPC in {1..128 set}, spf16 in [1,200], 0x55AA
 *   - FAT16 gate FUN_180004a40: numFATs != 0, total nonzero, SPC set,
 *     spf16 != 0, 0x55AA (no string)
 *   - FAT32 gate FUN_180004b70: memcmp "FAT32   " at +0x52 (CASE-
 *     SENSITIVE per the decompile), numFATs != 0, total32 != 0, SPC set,
 *     spf32 != 0, 0x55AA
 * Returns the winning flavor, or a negative sentinel when no gate
 * passes (candidate rejected). */
int lost_boot_flavor(const u8* sector512);

/* ---- allocation table ---- */
class FatTable {
public:
    /* Load FAT1 (+FAT2 when present) into memory. max_clusters bounds the
     * copy. Chooses-healthy mirror semantics: entries consulted from the
     * healthy copy per-entry (CDoubleFATTable behavior). */
    u32 load(IStorageDevice* dev, const FatParams& p);

    /* next(cluster) -> canonical: 0 on free/bad-param, CHAIN_BAD,
     * >= CHAIN_EOC_MIN on end, else next cluster. Out-of-range -> 0. */
    u32 next(u32 cluster) const;

    /* raw entry (canonicalized) for completeness checks */
    u32 raw(u32 cluster) const;

    /* N-022 (diff-to-fix): default OFF = og parity. The og's
     * CDoubleFATTable::GetNext (FUN_180041de0 :54216-18) consults the
     * mirror ONLY when the primary's canonicalized entry is 0 (FREE);
     * a primary BAD (0xFF7/0xFFF7/0x0FFFFFF7) is non-zero and is served
     * directly - the chain truncates there. The core previously also
     * consulted the mirror on primary-BAD; that torn-pair salvage is
     * kept as an OPT-IN recovery mode (never the default scan path). */
    void set_mirror_bad_salvage(bool on) { mirror_bad_salvage_ = on; }

    bool loaded() const { return !fat1_.empty(); }
    u32 max_cluster() const { return max_cluster_; }

    /* cluster -> absolute byte offset of its data */
    static u64 cluster_offset(const FatParams& p, u32 cluster);

private:
    std::vector<u8> fat1_;
    std::vector<u8> fat2_;
    bool mirror_ = false;
    bool mirror_bad_salvage_ = false; /* N-022: opt-in, default og parity */
    u32 max_cluster_ = 0;
    Flavor flavor_ = Flavor::Fat32;

    u32 entry_at(const std::vector<u8>& f, u32 cluster) const;
    u32 mirror_consult_(u32 primary) const;
};

/* ---- chain walker ---- */
struct ChainResult {
    std::vector<u32> clusters; /* in order; empty on failure */
    bool cycle = false;
    bool broken = false;    /* chain ended prematurely (0/BAD mid-chain) */
    bool cancelled = false; /* cancel token fired mid-walk (F-A1-11) */
    bool inferred = false;  /* GAP-S6-2: contiguous-candidate run, not FAT-linked */
    u32 length() const { return u32(clusters.size()); }
};

/* Follow a chain from `start`; visited-set cycle guard (corpus RB-tree
 * equivalent); max_len bounds. EOC terminates cleanly. The cancel poll
 * happens at every chain step (og cadence: FATSearchDLL FUN_18000fb80
 * checks the token after every Next32() before the device read). */
ChainResult follow_chain(const FatTable& t, u32 start, u32 max_len = 0x100000,
                         const CancelToken* cancel = nullptr);

/* Reconstruct a chain for a DELETED entry (dentry's first cluster known,
 * FAT may still link it): walk while entries look like a plausible chain
 * (not FREE, not BAD); stop at EOC/cycle/implausible jump. */
ChainResult reconstruct_deleted_chain(const FatTable& t, u32 start, u32 expect_clusters,
                                      const CancelToken* cancel = nullptr);

/* GAP-S6-2 (bench run 40079065019: 46/10143 hash-true - the "classic
 * first-cluster-only" failure): a deleted file whose FAT links were zeroed
 * at delete time reconstructs as a ONE-cluster chain while the dentry still
 * carries the full size -> the writer emits plausible-size, wrong-byte
 * files. The contiguous-candidate heuristic: freed chains are commonly
 * physically contiguous (sequential writes), so the candidate run is
 * [start, start+expect) validated cluster-by-cluster against the FAT
 * free-state - every cluster in the run must be FREE (a reused cluster
 * truncates the run at its first occurrence; the prefix survives, honestly
 * partial). The result is flagged `inferred` - never graded Excellent
 * (the honesty cap in emit_entry enforces that). Corpus lineage:
 * CFatCompleteSearch "deleted-entry chain reconstruction" (v1 artifact
 * §5); the exact unlinked-chain heuristic is corpus-UNVERIFIED - this is
 * our reconstruction policy, registered in PARITY-DELTAS. */
ChainResult reconstruct_contiguous_chain(const FatTable& t, u32 start, u32 expect_clusters,
                                         const CancelToken* cancel = nullptr);

/* ---- LFN ---- */
u8 lfn_checksum(const u8 short_name[11]);
/* Assemble LFN from collected slots (in disk order); returns false if the
 * checksum does not match the short entry (corrupt LFN -> fall back 8.3). */
bool lfn_assemble(const std::vector<const LfnEntry*>& slots, u8 checksum, std::u16string& out);

/* GAP-S6-1 (bench run 40079065019, probe-verified): a deleted dentry's 8.3
 * first byte is 0xE5, so lfn_checksum over the raw bytes NEVER matches the
 * checksum the LFN slots stored at write time (computed over the ORIGINAL
 * first byte). The stored checksum is a deterministic oracle for that byte:
 * the checksum's byte-0 contribution is a bijection (ROR^10 of the byte),
 * so exactly one candidate in 0..255 can match when the slot set is intact.
 * Recover the original first byte + the matching checksum; false when the
 * slots disagree among themselves or no candidate matches (torn set).
 * Probe evidence: deletion 0xE5's byte 0 of every 32-byte entry - for LFN
 * slots that is the SEQUENCE byte; the 13-char name chunks are intact. */
bool lfn_deleted_checksum_recover(const std::vector<const LfnEntry*>& slots,
                                  const u8 short_name_raw[11], u8& out_first_byte,
                                  u8& out_checksum);

/* 8.3 name -> display string (strip pad spaces, split dot). */
std::u16string short_name_to_u16(const u8 name[11]);

/* ---- scan engine ---- */
/* progress-compatible callback type (same shape as ntfs WalkProgress) */
using WalkProgressLike = bool (*)(u64 done, u64 total, void* user);
struct FatScanStats {
    u64 dirs = 0;
    u64 files = 0;
    u64 deleted_entries = 0;
    u64 lfn_recovered = 0;
    u64 chains_broken = 0;
    u64 cycles_cut = 0;
    u64 date_rejected = 0; /* N-010c: dentries dropped by date sanity */
};

struct FatScanFlags {
    bool reconstruct_deleted_chains = true; /* complete mode */
    /* N-010c (diff-to-fix): the og's dentry validator FUN_1800365a0
     * :44298-44315 rejects a dentry when any of the three date fields
     * (create +0x10 / write +0x18 / access +0x12) is nonzero with
     * month > 12 or day == 0. The gate is config-gated in the og
     * (singleton field +0x40, FUN_18002d1b0 :44305); whole-zero date
     * fields PASS the gate and still reach the timestamp conversion
     * (the N-021 zero-FileTime path). Default OFF preserves the
     * N-021-pinned default flow; ON mirrors the og's strict mode. */
    bool strict_date_sanity = false;
    /* N-022: opt-in torn-pair salvage (mirror consult on primary BAD);
     * default OFF = og parity. */
    bool mirror_bad_salvage = false;
};

/* Quick + complete in one entry: flags toggle complete behaviors.
 * Emits under root_active (live) / root_deleted / root_lost (orphans). */
u32 scan_fat(IStorageDevice* slice, const FatScanFlags& flags, CancelToken* cancel,
             WalkProgressLike progress, void* progress_user, tree::ResultTree& tree,
             tree::FullNode* root_active, tree::FullNode* root_deleted, tree::FullNode* root_lost,
             FatScanStats& stats);

} // namespace fat
} // namespace cr

#endif // CR_FAT_ENGINE_H_
