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
    u16 backup_boot_sector = 0; /* FAT32: usually 6 */
    u32 first_usable_cluster = 2;
};

/* Flavor discrimination (Microsoft whitepaper thresholds). */
Flavor flavor_from_clusters(u32 cluster_count_data);

/* Parse + validate a FAT boot sector. Accepts FAT12/16/32 layouts. */
u32 validate_boot(const u8* sector512, u64 device_bytes, FatParams& out);

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
    void set_raw(u32 cluster, u32 value); /* recovery reconstruction only */

    bool loaded() const { return !fat1_.empty(); }
    bool mirror_loaded() const { return mirror_; }
    u32 max_cluster() const { return max_cluster_; }
    /* divergence count between FAT1/FAT2 (health report) */
    u32 divergences() const { return divergences_; }

    /* cluster -> absolute byte offset of its data */
    static u64 cluster_offset(const FatParams& p, u32 cluster);

private:
    std::vector<u8> fat1_;
    std::vector<u8> fat2_;
    bool mirror_ = false;
    u32 divergences_ = 0;
    u32 max_cluster_ = 0;
    Flavor flavor_ = Flavor::Fat32;

    u32 entry_at(const std::vector<u8>& f, u32 cluster) const;
};

/* ---- chain walker ---- */
struct ChainResult {
    std::vector<u32> clusters; /* in order; empty on failure */
    bool cycle = false;
    bool broken = false;    /* chain ended prematurely (0/BAD mid-chain) */
    bool cancelled = false; /* cancel token fired mid-walk (F-A1-11) */
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

/* ---- LFN ---- */
u8 lfn_checksum(const u8 short_name[11]);
/* Assemble LFN from collected slots (in disk order); returns false if the
 * checksum does not match the short entry (corrupt LFN -> fall back 8.3). */
bool lfn_assemble(const std::vector<const LfnEntry*>& slots, u8 checksum, std::u16string& out);

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
};

struct FatScanFlags {
    bool keep_deleted = true;
    bool reconstruct_deleted_chains = true; /* complete mode */
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
