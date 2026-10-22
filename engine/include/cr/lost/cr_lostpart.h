/*
 * cr_lostpart.h - lost/deleted partition discovery (R3).
 *
 * Candidate-scoring engine: strides the whole disk for boot-sector and
 * table signatures, validates each with format-specific probes, scores
 * evidence, dedups against the live table and between candidates, and
 * materializes every result as a virtual read-only PartitionSlice for the
 * normal FS engines.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_LOSTPART_H_
#define CR_LOSTPART_H_

#include "../storage/cr_device.h"
#include "../storage/cr_slice.h"
#include "../table/cr_table_parse.h"
#include <memory>
#include <vector>

namespace cr {
namespace lost {

enum class CandidateFs : u8 { Ntfs = 0, Fat, Exfat, GptTable, MbrTable, Unknown };

struct Evidence {
    const char* tag; /* "boot-sig", "bpb-valid", "mft-probe", ... */
    int points;
};

struct Candidate {
    u64 start_lba = 0;
    u64 sector_count = 0;
    CandidateFs fs = CandidateFs::Unknown;
    int score = 0;
    std::vector<Evidence> evidence;
    u32 fs_probe = 0; /* engine open rc when probed (0 = OK) */
};

/* The full result set of one discovery pass. */
struct DiscoveryResult {
    std::vector<Candidate> candidates; /* sorted by score desc */
    u64 sectors_scanned = 0;
    u64 read_errors = 0;
};

/*
 * discover - full-disk pass.
 *   known: partitions from the live table (parse_disk_table output) - used
 *   for overlap scoring only; candidates inside a live region are still
 *   reported (damaged inner volumes) but marked.
 *   stride_sectors: probe granularity (default 1).
 */
u32 discover(IStorageDevice* disk, const std::vector<table::PartitionInfo>& known,
             u32 stride_sectors, CancelToken* cancel, bool (*progress)(u64, u64, void*),
             void* progress_user, DiscoveryResult& out);

/* Overlap classification vs the live table. */
enum class Overlap : u8 { None = 0, Inside, Overlaps, Contains };

Overlap classify_overlap(u64 start, u64 count, const std::vector<table::PartitionInfo>& known);

/* Materialize a candidate as a bounded virtual volume (R3.2: read-only,
 * the user's table is never repaired). */
std::unique_ptr<PartitionSlice> materialize(IStorageDevice* disk, const Candidate& c);

} // namespace lost
} // namespace cr

#endif // CR_LOSTPART_H_
