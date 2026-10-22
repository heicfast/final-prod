/*
 * cr_rawfixer.h - transactional filesystem repair (corpus RawFixer.dll).
 *
 * Corpus contract (rawfixer-fs-repair.md + rawfixer-v2--method-inventory.md,
 * decompile-verified):
 *   CRawFixer::Start/CheckPartition with per-FS fixers:
 *     NTFSFixer:  CheckDBR, FixFromDBR, UpdateDBR, FixDBR, FixByScanSector,
 *                 CheckSector, Undo, CheckMFT (record-size math incl. the
 *                 negative clusters_per_record log2 encoding)
 *     FATFixer:   CheckRootDir, IsValidDBR, CheckFatTable, CheckCluster,
 *                 CheckBackupDBR (backup boot sector 6 -> primary restore)
 *     ExFATFixer: TryFixFromSectorsPerCluster (re-derive spc from cluster
 *                 count invariants, 2 candidate paths), ReadCluster,
 *                 CopySectors (undo snapshots), ChecksumDBR (rolling
 *                 checksum over primary 0 + backup 12), CheckRoot,
 *                 CheckMustBeZero
 *   Protocol: transactional - CopySectors snapshots before any write, every
 *   fixer implements Undo, RawFixerLog records each action; failures leave
 *   FS type unknown (0x19) and the volume falls through to carving.
 *
 * SAFETY STANDARD (this engine is write-capable; stricter than the rest of
 * the core combined):
 *   S1. Consent: analyze() is read-only and produces a plan; NOTHING is
 *       written until execute() is called with an explicit plan.
 *   S2. Clean-refusal: a volume whose structures validate clean is never
 *       modified (actions stay empty, refusal_reason explains).
 *   S3. Ambiguity-refusal: multiple equally-plausible repairs -> refusal,
 *       never a coin-flip write.
 *   S4. Transaction: every written sector is snapshotted into the undo log
 *       BEFORE the write; the log is durable (CRC-checked container) and
 *       undo() restores the exact original bytes. A failed action rolls
 *       back every action applied before it.
 *   S5. Verification: every fixer re-validates the structures it wrote
 *       after the write; mismatch = immediate full rollback.
 *   S6. Scope: boot sectors, checksums and derived geometry only. FAT
 *       chains, MFT records and directory content are NEVER rewritten.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_RAWFIXER_H_
#define CR_RAWFIXER_H_

#include "../cr_types.h"
#include "../storage/cr_device.h"
#include <string>
#include <vector>

namespace cr {
namespace repair {

/* Write-capable device view (implemented by the platform layer with
 * GENERIC_WRITE handles; the pure core never opens devices itself). */
class IWriteDevice {
public:
    virtual ~IWriteDevice() = default;
    virtual u32 read_at(u64 offset, void* buffer, u32 length) = 0;
    virtual u32 write_at(u64 offset, const void* buffer, u32 length) = 0;
    virtual u64 size() const = 0;
    /* Logical sector size: raw-disk writes must be sector-granular, so
     * execute() coalesces every action up to this unit (4Kn drives report
     * 4096). Implementations override with their probed geometry. */
    virtual u32 sector_bytes() const { return 512; }
};

enum class ActionKind : u32 {
    RestoreBootFromBackup = 1, /* copy backup boot sector over a damaged primary */
    RepairBootChecksum = 2,    /* exFAT: recompute the boot-region checksum fill */
    RepairGeometryField = 3,   /* derived BPB field (e.g. sectors-per-cluster shift) */
};

struct RepairAction {
    ActionKind kind = ActionKind::RestoreBootFromBackup;
    u64 offset = 0; /* byte offset written */
    u32 length = 0;
    std::vector<u8> after; /* bytes to write */
    std::string reason;    /* human explanation (RawFixerLog line) */
};

struct RepairPlan {
    std::vector<RepairAction> actions;
    bool refused = false;
    std::string refusal_reason;
    std::string fs_family; /* detected family (ntfs/fat/exfat) */
    bool detected = false; /* family identified at all */
};

/* Durable undo log: magic + version + count + records + CRC32. */
struct UndoLog {
    u64 sequence = 0;
    struct Entry {
        u64 offset;
        u32 length;
        std::vector<u8> original;
    };
    std::vector<Entry> entries;

    void record(u64 offset, u32 length, const u8* original) {
        Entry e;
        e.offset = offset;
        e.length = length;
        e.original.assign(original, original + length);
        entries.push_back(std::move(e));
    }

    /* Container serialize/deserialize (CRC32-checked, magic "CRUNDO01"). */
    void serialize(std::vector<u8>& out) const;
    static u32 deserialize(const u8* data, size_t len, UndoLog& out);
};

/* ---- Phase 1: analyze (READ-ONLY) ----
 * Detects the family, validates structures, and derives the repair plan.
 * Returns OK with an empty non-refused plan when the volume is clean.
 * Structure checks reuse the scan engines' own validators wherever one
 * exists (single source of truth: the fixer can never accept a boot
 * sector the scanner would reject, and vice versa). */
u32 analyze(IStorageDevice* slice, RepairPlan& plan);

/* ---- Phase 2: execute (TRANSACTIONAL WRITES) ----
 * Snapshots + writes + verifies + FULL rollback on any failure (every
 * action applied so far is restored, in reverse order). The undo log is
 * populated even on success (the operator may still revert later). */
u32 execute(IWriteDevice* dev, const RepairPlan& plan, UndoLog& undo);

/* ---- Phase 3: undo (RESTORE) ----
 * Replays the undo log's original bytes back, verifying each write. */
u32 undo(IWriteDevice& dev, const UndoLog& log);

} // namespace repair
} // namespace cr

#endif // CR_RAWFIXER_H_
