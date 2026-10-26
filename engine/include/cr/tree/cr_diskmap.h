/*
 * cr_diskmap.h - the visualization data model (GAP-S6-11; ABI-CONTRACT
 * section 7).
 *
 * Corpus lineage: the reference renders a sector-grid disk map during the
 * scan (AnalysisGridWidget, 54 v-methods, RTTI-verified - corpus
 * UIUX/08 section 1) fed by the engine event pipe and the engines' shared
 * IBuildVolumeBitmap interface ("which clusters are free/used" - corpus
 * searchdll-family--architecture.md section 2). The WIDGET is UI-side;
 * the DATA MODEL is what core produces: geometry + per-cell allocation
 * state + scan coverage + found-content density + confidence grading +
 * bad-sector overlay. This builder is that producer - no rendering, no
 * colors, no UI semantics.
 *
 * Truth rules (nothing invented):
 *  - allocation = USED only where ACTIVE structured files' extents lie
 *    (the only "allocated" claim a session can prove); FREE exactly where
 *    the engine allocation bitmap (S-009: NTFS $Bitmap / FAT / exFAT
 *    table truth, persisted cell-granular in the session as REC_ALLOC_MAP)
 *    marks free AND no structured extent and no error span claims the
 *    cell; everything else is UNKNOWN - deleted/carved content may sit in
 *    free or reused clusters.
 *  - coverage = ALL cells when the carve stage completed (carve reads the
 *    whole slice), else exactly the cells covered by found-file extents
 *    (the result-bearing coverage; metadata-only reads are not tracked).
 *  - found = the count of distinct files whose extents cover the cell
 *    (capped at 255); confidence = the max Confidence ordinal of covering
 *    content (0 = none, 1..4 the ordinal).
 *  - bad = the ReadErrorMap spans, absolute device bytes, capped to the
 *    caller's buffer.
 *  - Cell sizing: the cluster size when known, else 64 KiB (the og grid
 *    granularity class), growing by powers of two until the total overlay
 *    bytes stay within the 64 MiB budget; the chosen granularity is
 *    reported back in the geometry.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_DISKMAP_H_
#define CR_DISKMAP_H_

#include "cr/tree/cr_snapshot.h"
#include "cr/storage/cr_device.h"
#include "cr/storage/cr_freemap.h"
#include <memory>
#include <utility>
#include <vector>

namespace cr {
namespace viz {

struct DiskMapGeometry {
    u64 start_byte = 0;    /* absolute base of the mapped view */
    u64 total_bytes = 0;   /* mapped extent */
    u64 cell_bytes = 0;    /* the granularity ACTUALLY used */
    u64 cell_count = 0;    /* the overlay array lengths */
    u32 cluster_bytes = 0; /* 0 = no cluster model was known */
    u32 fs_family = 0;     /* orchestrator::FsFamily value */
};

/* Cell values: allocation 0 unknown / 1 used / 2 free / 3 fs-marked bad;
 * coverage 0 unscanned / 1 scanned; found 0..255 (capped count);
 * confidence 0..4 (max ordinal). */
constexpr u8 kCellAllocUnknown = 0;
constexpr u8 kCellAllocUsed = 1;
constexpr u8 kCellAllocFree = 2;
constexpr u8 kCellAllocFsBad = 3;

struct DiskMapInputs {
    u64 slice_start = 0;                  /* absolute base of the scanned view */
    u64 slice_bytes = 0;                  /* extent of the scanned view */
    u32 cluster_bytes = 0;                /* 0 = unknown -> 64 KiB default cells */
    u32 fs_family = 0;                    /* orchestrator::FsFamily value */
    bool carve_completed = false;         /* coverage truth: carve read everything */
    const ReadErrorMap* errors = nullptr; /* bad-sector overlay source */
    /* S-009: the engine allocation bitmap (IBuildVolumeBitmap truth).
     * Null/invalid = no free overlay (cells stay unknown - never a
     * guess). Cells the bitmap marks free AND no structured-active
     * content covers AND no error span marks become kCellAllocFree. */
    const storage::FreeMap* free_map = nullptr;
};

/* Build the map over a live tree (session trees are the stable case).
 * Any overlay pointer may be null (the caller takes only what it paints);
 * all-null is ERR_BAD_PARAMETER. bad_ranges receives up to bad_cap
 * [start,len] u64 pairs (absolute bytes); *bad_needed returns the true
 * count either way; bad_cap < needed is ERR_TRUNCATED after filling. */
u32 build_disk_map(const DiskMapInputs& in, const tree::ResultTree& tree, DiskMapGeometry& geom,
                   std::vector<u8>* allocation, std::vector<u8>* coverage, std::vector<u8>* found,
                   std::vector<u8>* confidence, std::vector<std::pair<u64, u64>>& bad_ranges,
                   u32 bad_cap, u32* bad_needed);

/* Snapshot variant (the mid-scan surface - value semantics, race-free). */
u32 build_disk_map(const DiskMapInputs& in, const tree::Snapshot& snap, DiskMapGeometry& geom,
                   std::vector<u8>* allocation, std::vector<u8>* coverage, std::vector<u8>* found,
                   std::vector<u8>* confidence, std::vector<std::pair<u64, u64>>& bad_ranges,
                   u32 bad_cap, u32* bad_needed);

/* S-009 persistence: downsample a raw cluster-granular free map to the
 * disk map's own cell granularity (the same choose_cell_bytes sizing,
 * bounded by the overlay budget - a 16 TiB volume's map persists in
 * <= 8 MiB, not 512 MiB). A cell's bit is set only when EVERY cluster
 * it covers is free and fully inside the map's coverage (conservative:
 * partial coverage is unknown, not free). The output map's "cluster
 * 0" is the view's first cell, so a reloaded map aligns with the
 * rebuilt grid by construction. */
u32 downsample_free_map(const storage::FreeMap& raw, u64 slice_start, u64 slice_bytes,
                        storage::FreeMap& out);

} // namespace viz
} // namespace cr

#endif /* CR_DISKMAP_H_ */
