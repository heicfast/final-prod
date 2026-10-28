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
    /* M-017: the og's content-driven score, permille form (0..1000 =
     * 0.0..1.0): ln(count)/ln(max_count) where count is the per-FS
     * weighted file/folder count + the per-FS bonus + the +500
     * table-match, and max_count is the best count across THIS scan
     * (DRWUI FUN_14050fda0 :842164-278, verified to 6 decimals against
     * all 187 CalcPartition.log rows). Replaces the absolute structural
     * points as the PRIMARY ranking key. */
    int score = 0;
    int structural_points = 0; /* the legacy absolute evidence points -
                                * the secondary (tie-break) key */
    u64 content_count = 0;     /* the og's weighted count incl. bonus and
                                * the +500 table-match */
    bool table_match = false;  /* the candidate's start matches a live
                                * table entry ("Found once partition:") */
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
    /* M-137 (diff-to-fix): the og's found-partition row context - the
     * candidate presentation metadata the UIUX/08 §2 rows carry
     * (Disk / Basic / Dynamic / MBR / GPT / File System). The label is
     * the boot-sector volume label where the family carries one in the
     * boot sector (the FAT12/16 +0x2B 11-byte old-style label - the only
     * on-disk label the discovery pass reads without running a full
     * engine probe; NTFS/exFAT labels live in metadata the discovery
     * never touches, and the register-channel names are M-118's open
     * scope). Empty = unnamed (the naming template's counter path).
     * The disk_* trio is the scan-target context stamped by the
     * orchestrator at discovery time (the table parse + LDM probe that
     * already ran in the engine pass) and persisted per-record (v3
     * tail). */
    char volume_label[12] = {0}; /* NUL-padded FAT12/16 boot label */
    u32 disk_layout = 0;         /* table::TableLayout: 0 none, 1 MBR,
                                  * 2 EBR, 3 GPT, 4 GPT-damaged */
    u8 disk_dynamic = 0;         /* 1 = LDM database detected on target */
    u8 disk_table_ok = 0;        /* 1 = the target's table parsed */
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

/* M-017: the og's scorer (DRWUI FUN_14050fda0 :842164-278), exposed for
 * the CalcPartition.log parity tests. num = count > 1 ? ln(count) : 1.0
 * (the count==1 identity quirk: a 1-file candidate scores ABOVE every
 * 2..6-file candidate when max > e^2); denom = max_count > 1 ?
 * ln(max_count) : 1.0. Returns the ratio in [0, 1]. */
double score_ratio(u64 count, u64 max_count);

/* M-017: the og's weighted count (FUN_14050fda0 :842176-842200): NTFS
 * family -> files + 50*folders; FS code 8 (HFS+) -> files + 10*folders;
 * FS 1/2 (FAT) -> (files + folders) * 50. fs_code is the og's DRWUI FS
 * code, NOT CandidateFs. */
u64 weighted_count(int og_fs_code, u64 files, u64 folders);

/* Materialize a candidate as a bounded virtual volume: the READ-ONLY
 * recovery path (scanning a lost volume's content without touching the
 * user's table). S-029 supersedes the old "R3.2: the user's table is
 * never repaired" blanket read-only contract: the WRITE path exists as a
 * separate consent-gated module (cr/apply/cr_apply.h - plan_apply /
 * execute_apply / undo_apply + the cr_ui consent ABI); discovery and
 * materialization stay read-only by design, and applying a candidate is
 * never a side effect of scanning. */
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
