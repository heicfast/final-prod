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
    u64 sector_count = 0;
    CandidateFs fs = CandidateFs::Unknown;
    int score = 0;
    std::vector<Evidence> evidence;
    bool size_from_device_end = false; /* GAP-S6-6: the size is the
                                        * device-tail FALLBACK (probe could
                                        * not read a real volume size) -
                                        * never containment evidence */
    /* M-042 (the og's per-record geometry payload — CSearchedPartitionInfo
     * bps/SPC/hidden, PartitionRecoveryD lostpart.dat +0x38 bps / +0x444
     * SPC): captured by the BPB-family probes at discovery so the
     * persisted record carries geometry evidence. have_geometry = 0 for
     * non-BPB formats (GPT/Btrfs/ZFS/HFS+/ext/ReFS/BitLocker) — the og
     * leaves those fields zero for them too (only the BPB families write
     * real values). */
    u32 bytes_per_sector = 0;
    u32 sectors_per_cluster = 0;
    u32 hidden_sectors = 0;
    u8 have_geometry = 0;
    /* M-017 (the og's content-driven dSuggested inputs): files/folders
     * recovered from the candidate during the scan fan-out (the raw-count
     * weights live in cr_scoring.h), and the og-shaped score carried on
     * the candidate (permille 0..1000, rounded; the og's dSuggested is a
     * double printed %f - the permille keeps the int score field's
     * persisted shape while ordering by the exact double). */
    u64 content_files = 0;
    u64 content_folders = 0;
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

/* M-042: resume re-validation (the og's per-record re-read-and-drop,
 * PartitionRecoveryD PRD:20748-20842 — at search start every persisted
 * lostpart record is re-read at its start LBA through the device and
 * re-validated; records whose identity no longer matches the CURRENT disk
 * are dropped before they reach the UI). Our identity = {start_lba,
 * fs classification} (the og's is {start, end, type}; our count check
 * rides the device size) plus the persisted BPB geometry when the record
 * carries it. still_valid = false when the disk no longer classifies the
 * record's start as the persisted family — the caller drops the record.
 * The CURRENT disk is the arbiter, exactly like the og: a session resumed
 * against any other disk gets that disk's verdict. */
bool revalidate_candidate(IStorageDevice* disk, const Candidate& c);

} // namespace lost
} // namespace cr

#endif // CR_LOSTPART_H_
