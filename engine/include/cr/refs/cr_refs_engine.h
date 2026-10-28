/*
 * cr_refs_engine.h - full ReFS scan engine (MSB-chain reconstruction).
 *
 * On-disk model (Prade 2019 "Forensic Analysis of ReFS" + ReadFS working
 * parser + og decompile gates):
 *   VBR (LBA 0): "ReFS" OEM @+3, "FSRS" @+0x10, sectorCount u64 @+0x18,
 *     bytesPerSector u32 @+0x20, sectorsPerCluster u32 @+0x24, major u8
 *     @+0x28, minor u8 @+0x29, serial @+0x38, bytesPerContainer @+0x40.
 *   Superblock page (cluster 30; backups in the 2nd/3rd-last clusters):
 *     page header 0x50, GUID 4xu32 @+0x50 (volume signature = XOR),
 *     version u64 @+0x68, checkpointRefsOffset u32 @+0x70 -> two u64
 *     checkpoint cluster numbers, self-descriptor @+0x78/+0x7c.
 *   Checkpoint page: page header 0x50, major/minor @+0x54, self-descriptor
 *     @+0x58, chkpVirtualClock u64 @+0x60, pointerCount u32 @+0x90,
 *     pointer list u32[13] @+0x94 (0=ObjectID, 1=MediumAllocator,
 *     4=ParentChild, ...) - each pointer is an offset to a page reference
 *     (LCN quadruple + checksum info).
 *   Minstore B+ nodes (pages, 16 KiB at 4 KiB clusters / 1 cluster at
 *     64 KiB): page header 0x50, index root @+0x50 (fixed 0x28 + variable),
 *     index header (data area bounds, height, flags 1=inner/2=root/4=
 *     stream, key-index bounds), key index u32[&0xFFFF] -> entries
 *     {len, keyOff, keyLen, flags (2=rightmost, 4=deleted, 0x40=stream),
 *     valOff, valLen}.
 *   Directory tables (root directory = fixed table ID 0x600): rows keyed
 *     {u32 type, name}: 0x10 directory descriptor, 0x00010030 file (value =
 *     embedded file-table root node: variable index root = MACE times,
 *     flags, size, alloc size; rows = attributes), 0x00020030 directory
 *     link (value: child directory table ID @+0x08 + metadata),
 *     0x80000020 ID2 (moved/deleted name evidence).
 *   Attributes (key header: type u16 @+8): 0x80 $DATA (value = embedded
 *     data-run table; rows: startLCN u64, flags u16 @+8 (0x10=data,
 *     0x80/0x100=checksummed), relVCN u64 @+0xC, numClusters u32 @+0x14),
 *     0xb0 $NAMED_DATA (resident ADS), 0xc0 $REPARSE_POINT.
 *   Allocator tables: rows {firstCluster u64, numClusters u64, free u16,
 *     flags u16 @+0x12 (2=sparse), bitmap @+0x18} -> deleted-extent
 *     viability grading (NTFS $Bitmap parity).
 *
 * The og corpus engine (ReFSSearchDLL, 20,270 lines) gates on
 * major==3 && minor>3 - ReFS 3.4+ (FUN_180012390:13529-13531,
 * re-validated on the checkpoint body by FUN_180012650:13744-50) - plus a
 * separate major==1 path; this engine validates the boot shape and accepts
 * any 1.x-3.x with self-validating structure discovery (strictness at the
 * page level: page signatures + volume signature checks everywhere).
 * REFS is a FIRST-CLASS engine: full MSB-chain reconstruction (see
 * scan_refs), not a detection report.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_REFS_ENGINE_H_
#define CR_REFS_ENGINE_H_

#include "../cr_types.h"
#include "../storage/cr_device.h"
#include "../tree/cr_node.h"

namespace cr {
namespace refs {

using WalkProgressLike = bool (*)(u64 done, u64 total, void* user);

struct RefsScanStats {
    u64 tables = 0;         /* ObjectID rows walked */
    u64 dirs = 0;           /* directory tables emitted */
    u64 files = 0;          /* files emitted */
    u64 data_runs = 0;      /* non-sparse data runs resolved */
    u64 deleted_rows = 0;   /* deleted-flagged + ID2 orphans harvested */
    u64 orphan_tables = 0;  /* tables unreachable from the root (lost) */
    u64 damaged_nodes = 0;  /* nodes failing validation */
    u64 allocator_rows = 0; /* allocator bitmap rows loaded (0 = none) */
    /* S-020/M-022 (og attribute-layer parity) */
    u64 resident_files = 0;       /* inline files materialized as resident extents */
    u64 dollar_i_rows = 0;        /* $Recycle.Bin "$I" resident records read */
    u64 paged_chases = 0;         /* indirect/paged run-table pages followed */
    u64 granularity_discards = 0; /* runlists discarded by the og granularity
                                   * gate (garbage-run false-positive guard) */
    /* M-026/M-027 (stale-page sweep + object identity) */
    u64 stale_pages = 0;     /* stale MSB+ pages harvested (not live-reachable) */
    u64 identity_dedups = 0; /* duplicate object rows collapsed (keep-one) */
    u64 recycle_routed = 0;  /* "$R…"/"S-…"/SVI rows routed to the recycle flush */
    u64 dollar_i_pairs = 0;  /* $R nodes remapped from their $I evidence */
    /* N-141: rows dropped by the og's zero-size AND no-dot noise filter
     * (FUN_18000ba90 :8317-37 — the FileValues +0x30 size gate local_e8 < 1
     * combined with the memchr(name, 0x2e) dot-scan; a zero-size row WITH
     * a '.' survives, a nonzero row without one survives). Counted so the
     * suppression is observable, never silent. */
    u64 zero_size_nodot_filtered = 0;
};

struct RefsScanFlags {
    bool harvest_deleted = true;   /* deleted-flagged rows + ID2 evidence */
    bool allocator_grading = true; /* medium-allocator bitmap viability */
    bool stale_sweep = true;       /* M-026: the og's stale-MSB-page sweep
                                    * (FUN_180005330 - runs unconditionally
                                    * og-side; the flag keeps an opt-out for
                                    * bounded product policies) */
};

/* Boot-sector validation + geometry decode. */
struct RefsBoot {
    bool valid = false;
    u16 major = 0, minor = 0;
    u32 bytes_per_sector = 0;
    u32 sectors_per_cluster = 0;
    u64 cluster_bytes = 0;
    u64 volume_clusters = 0;
    u64 volume_bytes = 0;
    u64 serial = 0;
    u64 container_bytes = 0;    /* DBR +0x40: bounds the MSB/OID area (M-026) */
    u32 container_clusters = 0; /* DBR +0x34: container span in clusters -
                                 * N-036: the og's container-translation unit
                                 * (volume+0x148 -> parser+0xa8), gates the
                                 * container-record acceptance and keys the
                                 * map; 0 = no container layout decode */
};
bool validate_boot(const u8 sector[512], RefsBoot& out);

/* Full scan: MSB-chain reconstruction, hierarchy walk, data runs,
 * deleted/ID2 harvest, allocator grading. Emits into the standard
 * three-root model. */
u32 scan_refs(IStorageDevice* slice, const RefsScanFlags& flags, CancelToken* cancel,
              WalkProgressLike progress, void* progress_user, tree::ResultTree& tree,
              tree::FullNode* root_active, tree::FullNode* root_deleted, tree::FullNode* root_lost,
              RefsScanStats& stats);

/* corpus ReFS engine FS code (engine-matrix artifact) */
constexpr u32 REFS_FS_KIND = 10;

} // namespace refs
} // namespace cr

#endif // CR_REFS_ENGINE_H_
