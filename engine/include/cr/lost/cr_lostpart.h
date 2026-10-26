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

/* value 4 is RESERVED: it was the never-constructed MbrTable name (W4,
 * AUDIT-PIPELINE register row 6); the numeric positions are session-
 * serialized (PartitionContext fs_kind) and must not renumber.
 * S-006 appended Ext2..Refs (values 6..10): Ext2 = the og's CExtXBlock-
 * Parser lost-scan family (parity); HfsPlus/Btrfs/Zfs/Refs are documented
 * supersets (the og's PartitionRecoveryD deep scan has no parsers for
 * these - see docs/PARITY-DELTAS.md D-50; the core's engines exist and
 * ReFS is first-class per the audit mandate). BitLocker (S-019) is the
 * og's own lost-scan family (type 0xB classifier + BitlockerParser,
 * PartitionRecoveryD_decompiled.c:2778-2785/:2604-2606). Values > 5
 * deserialize only from sessions written after S-006; older sessions
 * carry <= 5. */
enum class CandidateFs : u8 {
    Ntfs = 0,
    Fat,
    Exfat,
    GptTable,
    Unknown = 5,
    Ext2,
    HfsPlus,
    Btrfs,
    Zfs,
    Refs,
    BitLocker
};

struct Evidence {
    const char* tag; /* "boot-sig", "bpb-valid", "mft-probe", ... */
    int points;
};

struct Candidate {
    u64 start_lba = 0;
    u64 sector_count = 0; /* whole-volume extent (boot sector -> volume
                           * end; materialize/slicing need the metadata
                           * region too) */
    /* N-035 (diff-to-fix): the og's CSearchedPartitionInfo carries BOTH
     * StartSector and DataAreaStart and anchors its extent math at the
     * DATA AREA (EndSec = DataAreaStart + totalSec, DRWUI_decompiled.c
     * :842476-842479; CalcPartition.log ground truth: FAT32 candidates
     * show StartSec->DataAreaStart gaps of 472011..2724233 sectors -
     * the reserved+two-FATs region; NTFS candidates show no gap). The
     * containment/dedup/overlap classification runs on the data-area
     * extent below; 0 = unset (normalized to the whole-volume extent,
     * i.e. DataAreaStart == StartSec - the NTFS/exFAT shape). */
    u64 data_start_lba = 0;
    u64 data_sector_count = 0;
    CandidateFs fs = CandidateFs::Unknown;
    int score = 0;
    std::vector<Evidence> evidence;
    bool size_from_device_end = false; /* GAP-S6-6: the size is the
                                        * device-tail FALLBACK (probe could
                                        * not read a real volume size) -
                                        * never containment evidence */
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

/* Materialize a candidate as a bounded virtual volume: the READ-ONLY
 * recovery path (scanning a lost volume's content without touching the
 * user's table). S-029 adds the WRITE path as a separate consent-gated
 * module (cr/apply/cr_apply.h: plan_apply/execute_apply - MBR entry,
 * transactional, undoable); this materializer stays read-only by design. */
std::unique_ptr<PartitionSlice> materialize(IStorageDevice* disk, const Candidate& c);

} // namespace lost
} // namespace cr

#endif // CR_LOSTPART_H_
