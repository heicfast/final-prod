/*
 * cr_runlist.h - NTFS datarun (runlist) codec + typed run view.
 *
 * Corpus (FUN_180003440 + algorithms docs): header byte high nibble =
 * length-field width, low nibble = offset-field width; offset is signed
 * (negative deltas = backward runs); terminator 0x00. Sparse runs = zero
 * offset delta with nonzero length (VCN gaps stay unwritten).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_RUNLIST_H_
#define CR_RUNLIST_H_

#include "../cr_types.h"
#include "../cr_error.h"
#include <vector>

namespace cr {
namespace ntfs {

struct DataRun {
    u64 vcn;    /* first VCN of the run */
    u64 length; /* clusters in the run */
    i64 offset; /* signed LCN delta from previous run's LCN */
    u64 lcn;    /* absolute LCN (0 with sparse flag = hole) */
    bool sparse;
};

class RunList {
public:
    /* Decode from an on-disk runlist byte string (not including the 8-byte
     * attribute header). Returns OK/ERR_TRUNCATED/ERR_BAD_PARAMETER.
     * max_runs guards corrupt streams (fix F6 family). */
    static u32 decode(const u8* data, u32 bytes, u64 first_vcn, u64 max_clusters, RunList& out);

    /* Encode to bytes (used by session serialization of extents). */
    std::vector<u8> encode() const;

    const std::vector<DataRun>& runs() const { return runs_; }
    std::vector<DataRun>& runs() { return runs_; }

    u64 total_clusters() const;
    u64 allocated_bytes(u32 cluster_bytes) const { return total_clusters() * cluster_bytes; }
    bool empty() const { return runs_.empty(); }

    /* Map a VCN to (lcn, run index) - binary search. sparse_out tells hole. */
    bool vcn_to_lcn(u64 vcn, u64* lcn, bool* sparse_out) const;

    /* Contiguous byte ranges of valid data (for recovery IO planning). */
    struct Extent {
        u64 lcn;
        u64 clusters;
    };
    void extents(std::vector<Extent>& out) const;

private:
    std::vector<DataRun> runs_;
};

constexpr u32 RUNLIST_MAX_RUNS = 0x100000; /* 1 Mi runs - guard (fix F6) */

} // namespace ntfs
} // namespace cr

#endif // CR_RUNLIST_H_
