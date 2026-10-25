/*
 * cr_diskmap.cpp - the visualization data model builder (GAP-S6-11).
 * See cr_diskmap.h for the truth rules and corpus lineage.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/tree/cr_diskmap.h"
#include "cr/tree/cr_node.h"

#include <algorithm>
#include <cmath>

namespace cr {
namespace viz {

namespace {

constexpr u64 kDefaultCellBytes = 64 * 1024;  /* the og grid granularity class */
constexpr u64 kOverlayBudgetBytes = 64ULL * 1024 * 1024; /* total overlay cap */

u64 choose_cell_bytes(u64 total, u32 cluster_bytes) {
    u64 cell = cluster_bytes ? u64(cluster_bytes) : kDefaultCellBytes;
    if (cell < 512)
        cell = 512;
    while (total / cell > kOverlayBudgetBytes)
        cell *= 2; /* pow-2 growth honors the memory budget */
    return cell;
}

struct Overlays {
    std::vector<u8>* alloc;
    std::vector<u8>* cov;
    std::vector<u8>* fnd;
    std::vector<u8>* conf;
    u64 cells = 0;
    u64 cell_bytes = 0;
    u64 base = 0;
    u64 total = 0;
};

/* Mark one node's extents into the overlays. The extent basis must match
 * the map's absolute base (sessions are disk-absolute since F-1). */
void mark_extents(const tree::FileProperties::Extent* exts, size_t ext_count, u64 logical,
                  u8 origin, u8 deleted, u8 confidence, const Overlays& ov) {
    if (exts == nullptr || ext_count == 0)
        return;
    for (size_t i = 0; i < ext_count; ++i) {
        const u64 ext_end = exts[i].offset + exts[i].length;
        u64 cell = exts[i].offset < ov.base ? 0 : (exts[i].offset - ov.base) / ov.cell_bytes;
        if (ext_end <= ov.base)
            continue; /* outside the mapped view */
        const u64 last_cell = (ext_end - ov.base - 1) / ov.cell_bytes;
        for (; cell <= last_cell && cell < ov.cells; ++cell) {
            if (ov.fnd && (*ov.fnd)[size_t(cell)] < 255)
                ++(*ov.fnd)[size_t(cell)];
            if (ov.conf && confidence > (*ov.conf)[size_t(cell)])
                (*ov.conf)[size_t(cell)] = confidence > 4 ? 4 : confidence;
            if (ov.cov)
                (*ov.cov)[size_t(cell)] = 1;
            /* allocation: USED only for ACTIVE structured files (the one
             * claim a session can prove); deleted/carved content never
             * marks used (its clusters may be free or reused) */
            if (ov.alloc && origin == u8(tree::Origin::Structured) &&
                deleted == u8(tree::DeletedState::Active))
                (*ov.alloc)[size_t(cell)] = kCellAllocUsed;
        }
    }
    (void)logical;
}

u64 cell_of(u64 abs_byte, u64 base, u64 cell_bytes) {
    return abs_byte < base ? 0 : (abs_byte - base) / cell_bytes;
}

} // namespace

u32 build_disk_map(const DiskMapInputs& in, const tree::Snapshot& snap, DiskMapGeometry& geom,
                   std::vector<u8>* allocation, std::vector<u8>* coverage,
                   std::vector<u8>* found, std::vector<u8>* confidence,
                   std::vector<std::pair<u64, u64>>& bad_ranges, u32 bad_cap, u32* bad_needed) {
    if ((!allocation && !coverage && !found && !confidence) || in.slice_bytes == 0)
        return ERR_BAD_PARAMETER;
    geom = DiskMapGeometry{};
    geom.start_byte = in.slice_start;
    geom.total_bytes = in.slice_bytes;
    geom.cluster_bytes = in.cluster_bytes;
    geom.fs_family = in.fs_family;
    geom.cell_bytes = choose_cell_bytes(in.slice_bytes, in.cluster_bytes);
    geom.cell_count = (in.slice_bytes + geom.cell_bytes - 1) / geom.cell_bytes;

    Overlays ov;
    ov.alloc = allocation;
    ov.cov = coverage;
    ov.fnd = found;
    ov.conf = confidence;
    ov.cells = geom.cell_count;
    ov.cell_bytes = geom.cell_bytes;
    ov.base = in.slice_start;
    ov.total = in.slice_bytes;
    if (allocation)
        allocation->assign(size_t(geom.cell_count), kCellAllocUnknown);
    if (coverage)
        coverage->assign(size_t(geom.cell_count), 0);
    if (found)
        found->assign(size_t(geom.cell_count), 0);
    if (confidence)
        confidence->assign(size_t(geom.cell_count), 0);

    const u32 n = snap.node_count();
    for (u32 i = 0; i < n; ++i) {
        const tree::SnapNode& sn = snap.node(i);
        if (sn.full || sn.extents.empty())
            continue; /* roots/virtuals carry no content */
        mark_extents(sn.extents.data(), sn.extents.size(), sn.logical_size, sn.origin, sn.deleted,
                     sn.confidence, ov);
    }

    /* coverage: carve completion read the whole slice - the honest
     * all-cells mark; without it, coverage is exactly the extent-derived
     * cells already marked above */
    if (coverage && in.carve_completed)
        std::fill(coverage->begin(), coverage->end(), u8(1));

    /* bad-sector overlay: the error-map spans (absolute), clamped to the
     * mapped view; the caller's buffer gets up to bad_cap pairs */
    if (bad_needed)
        *bad_needed = 0;
    if (in.errors) {
        const auto& ranges = in.errors->ranges();
        u32 count = 0;
        for (const auto& r : ranges) {
            const u64 end = r.offset + r.length;
            if (end <= in.slice_start || r.offset >= in.slice_start + in.slice_bytes)
                continue; /* outside the mapped view */
            ++count;
            if (count <= bad_cap)
                bad_ranges.emplace_back(r.offset,
                                        end > in.slice_start + in.slice_bytes
                                            ? in.slice_start + in.slice_bytes - r.offset
                                            : r.length);
            /* mark the cells */
            if (coverage || allocation) {
                const u64 c0 = cell_of(r.offset, in.slice_start, geom.cell_bytes);
                const u64 c1 = cell_of(end - 1, in.slice_start, geom.cell_bytes);
                for (u64 c = c0; c <= c1 && c < geom.cell_count; ++c) {
                    if (coverage)
                        (*coverage)[size_t(c)] = 1; /* the scan PROBED these bytes */
                    if (allocation)
                        (*allocation)[size_t(c)] = kCellAllocFsBad;
                }
            }
        }
        if (bad_needed)
            *bad_needed = count;
        if (count > bad_cap)
            return ERR_TRUNCATED; /* filled what fits; the count tells the rest */
    }
    return OK;
}

u32 build_disk_map(const DiskMapInputs& in, const tree::ResultTree& tree, DiskMapGeometry& geom,
                   std::vector<u8>* allocation, std::vector<u8>* coverage,
                   std::vector<u8>* found, std::vector<u8>* confidence,
                   std::vector<std::pair<u64, u64>>& bad_ranges, u32 bad_cap, u32* bad_needed) {
    /* the snapshot path is the value-semantic, race-free shape; capture
     * and delegate (same emission order - byte parity is a tree contract
     * and the map is order-insensitive by construction) */
    std::unique_ptr<tree::Snapshot> snap = tree::Snapshot::capture(tree, false);
    if (!snap)
        return ERR_NO_MEMORY;
    return build_disk_map(in, *snap, geom, allocation, coverage, found, confidence, bad_ranges,
                          bad_cap, bad_needed);
}

} // namespace viz
} // namespace cr
