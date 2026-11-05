/*
 * cr_apply.h - consent-gated lost-partition apply (S-029, diff-to-fix).
 *
 * Corpus contract (PartitionRecovery_decompiled.c, lead-verified citations
 * from the finding; re-verified line-by-line by the convergent DFX16 leg):
 * the og's lost-partition flow is a TWO-STEP product feature - discover
 * (the decoded probe family, S-006/S-019 parity), then APPLY:
 * RecoveryModule's CreateRecoveryPartitionObject hosts the apply
 * scenario, PartitionRecovery.dll's FUN_1800566d0 "rebuild disk layout"
 * writes the recovered entry back into the user's table (the legacy chain:
 * IOCTL_DISK_DELETE_DRIVE_LAYOUT 0x7C100 :46953, raw sector-0 rewrite -
 * zeroed entries + 0x55AA :46957-46962, IOCTL_DISK_SET_DRIVE_LAYOUT 0x7C010
 * with a 4-entry DRIVE_LAYOUT_INFORMATION :46995-47002; the modern single-
 * entry path IOCTL 0x7C058 with an 0x18-byte record :47015; every write
 * followed by IOCTL_DISK_UPDATE_PROPERTIES 0x70140 with a 3x Sleep(1000)
 * retry :46873-46880). The GPT variant FUN_1800392f0 :32797-32860 (mirrored
 * at RecoveryModule :46328, CloneModule :118357, UILogic :37590) regenerates
 * the 32-BIT DISK SIGNATURE via GPT_GenerateSignature and rewrites the
 * protective MBR - CORPUS CORRECTION (S-029 Resolution): every call site
 * passes a 4-byte stack slot as ONE register argument (PR:32805 local_24
 * is a ulong immediately followed by the HANDLE local_20 - a 16-byte GUID
 * write would corrupt it), so the regenerated identity is the MBR+440 /
 * DRIVE_LAYOUT_INFORMATION.Mbr.Signature value, NOT the 128-bit GPT disk
 * GUID; the GPT headers stay untouched, exactly as the reference does.
 *
 * The core's v1 apply implements the SEMANTIC (the recovered partition
 * entry lands in the user's MBR, transactionally, undoably) through the
 * SAME seams RawFixer established (S-008/S-010): the raw sector-0 write
 * path via repair::IWriteDevice (the og's raw-write leg), the volume
 * quiesce in the platform transport's lock_volume() (the S-010 sequence),
 * and the CRUNDO01 undo container. The kernel-mediated IOCTL chain
 * (DELETE/SET_DRIVE_LAYOUT, the 0x7C058 record, UPDATE_PROPERTIES) is the
 * IApplyDevice seam below - the Windows RW transport implements it with
 * the real DeviceIoControl sequence; file/RAM devices keep the portable
 * no-op defaults (the sector write IS the commit).
 *
 * SAFETY STANDARD (mirrors cr_rawfixer.h, plus the apply-specific rules):
 *   A1. Consent: plan_apply is read-only; execute_apply calls the consent
 *       callback BEFORE the volume lock is taken (the caller deliberates
 *       without holding the volume - the union of both convergent legs'
 *       orders: revalidate first so a doomed plan never reaches consent,
 *       consent before lock so a denial never holds the disk) and returns
 *       false/zero to refuse - a denied/absent callback =
 *       ERR_APPLY_CONSENT_DENIED, ZERO writes, ZERO locks.
 *   A2. Revalidation: execute compares the live sector 0 against the
 *       plan's snapshot BEFORE consent and again AFTER the lock - a table
 *       that changed since planning = ERR_APPLY_STALE, replan (the og
 *       re-reads sector 0 raw before writing :46944).
 *   A3. Containment: the plan REFUSES candidates that overlap a live
 *       entry, extend past the disk end, sit at LBA 0, exceed the 32-bit
 *       MBR LBA range, or target a non-MBR (GPT) layout in the
 *       single-entry scope - never a coin-flip write into a table the
 *       plan does not fully understand.
 *   A4. Transaction: the original sector-0 bytes are snapshotted into the
 *       undo log BEFORE the write (sector-granular, exactly like
 *       rawfixer's execute - direct-handle writes require it); every
 *       write is read-back verified; any failure rolls back everything
 *       applied (reverse order, verified).
 *   A5. Scope (v1): the MBR entry + the 0x55AA signature restore (the
 *       og's rebuild rewrites it :46957-46962). The GPT layout REBUILD
 *       (the og's FUN_1800392f0 signature-regen variant) is implemented
 *       in the RebuildTable scope; GPT ENTRY-array writes and the
 *       boot-chain side effects (M-040: BootIni/BCD) are OUT OF SCOPE and
 *       refused cleanly - the plan carries a boot_repair_available field
 *       for the future wiring.
 *   A6. Preservation superset (PARITY-DELTAS D-72b): the og's legacy
 *       rebuild zero-wipes ALL FOUR entries; the SingleEntry scope (the
 *       default) preserves the other three byte-for-byte. The og's exact
 *       wipe semantics remain available as the explicit RebuildTable
 *       scope.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_APPLY_H_
#define CR_APPLY_H_

#include "../cr_types.h"
#include "../lost/cr_lostpart.h"
#include "../repair/cr_rawfixer.h"
#include "../table/cr_table_parse.h"
#include <string>
#include <vector>

namespace cr {
namespace apply {

/* ---- the write device ---------------------------------------------------
 * IWriteDevice carries the sector transport + the S-010 quiesce. The
 * apply-specific device controls below are the corpus PartitionRecovery
 * IOCTL chain; the defaults are portable no-ops so RAM/file-backed test
 * doubles and non-Windows platform layers succeed without overriding
 * (for an image file the sector write IS the commit), while the Windows
 * RW transport overrides with the real DeviceIoControl sequence. Error
 * convention: OK (0) = success (the decompile's vtable calls are
 * BOOL-style; the transport inverts at its boundary). */
class IApplyDevice : public repair::IWriteDevice {
public:
    /* corpus 0x7C100 (legacy path; called BEFORE the sector writes) */
    virtual u32 delete_drive_layout() { return OK; }
    /* corpus 0x7C010: DRIVE_LAYOUT_INFORMATION (0xA8) - signature is the
     * Mbr.Signature field the kernel stamps at MBR+440; entry_count is the
     * occupied entry count after the apply (the og stamps 4 in the rebuild
     * flow). */
    virtual u32 set_drive_layout_mbr(u32 /*signature*/, u32 /*entry_count*/) { return OK; }
    /* corpus 0x7C058 modern single record (0x18 bytes: partition number,
     * 16-byte identity payload, boot flag - the FUN_180056bc0 tail). */
    struct PartitionInfoExRecord {
        u32 partition_number = 0;
        u8 identity[16] = {0};
        u8 boot_flag = 0;
    };
    virtual u32 set_partition_info_ex(const PartitionInfoExRecord& /*rec*/) { return OK; }
    /* corpus 0x70140 properties refresh; execute_apply() retries it 3x
     * with settle_refresh() between attempts, breaking on success (the
     * corpus tail - a failure there is REPORTED, not swallowed:
     * ERR_APPLY_REFRESH_FAILED with the table written+verified, D-72c). */
    virtual u32 update_properties() { return OK; }
    virtual void settle_refresh() {} /* Sleep(1000) on the Windows transport */
    /* corpus OS gate: legacy (pre-5.1) path selection. The build's
     * envelope is Win7 SP1+, so the shipped transport always answers
     * false (the modern path); the legacy branch stays corpus-faithful
     * and is exercised through test doubles. */
    virtual bool legacy_os_path() const { return false; }
    /* corpus GPT_GenerateSignature (PARTITION.MO): a fresh 32-bit disk
     * signature for the RebuildTable regenerate policy. The default mixes
     * the current sector-0 bytes with a process-unique counter (FNV-1a);
     * transports with real entropy and deterministic tests override. */
    virtual u32 generate_disk_signature();
};

/* ---- the plan ----------------------------------------------------------- */

/* v1 scopes: SingleEntry (the default - one candidate into the first free
 * MBR slot, the other entries preserved byte-for-byte, D-72b) and
 * RebuildTable (the og's FUN_1800566d0/FUN_1800392f0 rebuild semantics:
 * zero all four entries, write the candidate at slot 0, stamp the
 * signature per policy - on GPT disks the regenerate policy is forced,
 * the corpus GPT variant always calls GPT_GenerateSignature; the GPT
 * headers themselves stay untouched). */
enum class ApplyScope : u8 {
    SingleEntry = 0,
    RebuildTable = 1,
};

/* One concrete write: byte offset (device-absolute) + the bytes + the
 * human reason (printed by the CLI consent surface, mirroring the
 * RawFixerLog line). */
struct ApplyAction {
    u64 offset = 0;
    std::vector<u8> after;
    std::string reason;
};

struct ApplyPlan {
    /* the candidate being applied (copied for the consent surface) */
    u64 start_lba = 0;
    u64 sector_count = 0;
    lost::CandidateFs fs = lost::CandidateFs::Unknown;
    /* the derived MBR entry truth */
    u32 entry_index = 0; /* 0..3, the slot the plan targets */
    u8 mbr_type = 0;     /* the type byte (family-mapped) */
    /* the 16 entry bytes as they will land at 446 + entry_index*16 */
    std::vector<u8> entry_bytes;
    /* the signature restore action (planned only when 0x55AA is absent -
     * the og's rebuild rewrites it) */
    bool restores_signature = false;

    std::vector<ApplyAction> actions;

    /* A2 revalidation truth: the exact sector-0 bytes planning saw.
     * execute_apply refuses (ERR_APPLY_STALE) when the live disk no
     * longer matches - the table changed, replan. */
    std::vector<u8> sector0_original;

    /* M-040 gate: the boot-chain side effects (BootIni_Init / Fixup.mo
     * BCD repair, PR:47290-47302) are v1-out-of-scope; the field exists
     * so the consent surface can state the boundary truthfully. */
    bool boot_repair_available = false;

    /* ---- the DFX16 union additions (RebuildTable + the device chain) -- */
    ApplyScope scope = ApplyScope::SingleEntry;
    /* RebuildTable signature policy: preserve the on-disk signature by
     * caller choice, EXCEPT on GPT disks where plan_apply forces
     * regeneration (FUN_1800392f0 parity). The effective value lands in
     * mbr_signature (the pre-regeneration value for the consent surface). */
    bool regenerate_signature = false;
    u32 mbr_signature = 0;
    /* the modern-path record (corpus 0x18-byte 0x7C058 shape): the
     * partition number is the 1-based slot, the identity payload carries
     * the LBA geometry (the core's deterministic encoding of the entry it
     * writes; the og copies a caller GUID there). */
    IApplyDevice::PartitionInfoExRecord record;

    bool refused = false;
    std::string refusal_reason;
};

/* Consent callback: called once per execute, after revalidation, before
 * the volume lock and the first write. Return false (or pass nullptr)
 * to refuse - nothing is written, nothing is locked. Mirrors the
 * rawfixer core-consent convention. */
using ApplyConsentFn = bool (*)(const ApplyPlan& plan, void* user);

/* ---- Phase 1: plan (READ-ONLY) ----
 * Derives the MBR entry write for a discovered lost-partition candidate:
 *   - validates sector 0 is an MBR (0x55AA present or restorable; a GPT
 *     protective entry 0xEE = clean refusal in the SingleEntry scope -
 *     GPT entry apply is v2 scope; the RebuildTable scope implements the
 *     og's GPT signature-regen variant)
 *   - picks the first all-zero entry slot (a full table = refusal)
 *   - refuses containment violations: candidate at LBA 0, past the disk
 *     end, beyond the 32-bit MBR LBA range, or overlapping any live entry
 *     (the og never overwrites live entries in the single-partition
 *     recovery flow)
 *   - maps the candidate family to the MBR type byte, computes the CHS
 *     pair (255-heads/63-sector DOS convention, clamped to the maximum
 *     CHS beyond cylinder 1023 - what a BIOS/kernel reports for large
 *     disks; the og's kernel path derives the same through
 *     DRIVE_LAYOUT_INFORMATION)
 *   - snapshots sector 0 for execute-time revalidation + undo
 * A refused plan returns OK with refused=true (the refusal is a RESULT,
 * not an error - the caller surfaces the reason).
 * known (optional): the parsed live table - used for the overlap refusal
 * when the caller has it (the cr_ui path parses once); the plan itself
 * reads the entries from the sector-0 snapshot either way.
 * scope (optional, default SingleEntry): see ApplyScope. */
u32 plan_apply(IStorageDevice* disk, const lost::Candidate& c, ApplyPlan& out,
               const std::vector<table::PartitionInfo>& known = {},
               ApplyScope scope = ApplyScope::SingleEntry);

/* ---- Phase 2: execute (CONSENT-GATED TRANSACTIONAL WRITE) ----
 * Sequence: revalidate sector 0 (A2 - a stale plan never reaches consent)
 * -> consent (A1 - denial never takes the lock) -> lock_volume() (the
 * S-010 quiesce; failure = ERR_VOLUME_LOCK_FAILED, zero writes) ->
 * revalidate again (the lock-window race) -> [legacy path only:
 * delete_drive_layout()] -> the A4 transaction (sector-granular snapshot
 * -> write -> read-back verify; any failure rolls back everything applied
 * in reverse order) -> the path's device-control sequence (legacy:
 * UPDATE_PROPERTIES then SET_DRIVE_LAYOUT, the corpus order; modern: the
 * 0x7C058 record) -> the shared 3x settle+UPDATE_PROPERTIES refresh tail
 * (a failing tail = ERR_APPLY_REFRESH_FAILED, the table stays written and
 * verified, the undo log stays valid - D-72c). The undo log is populated
 * on success too (replay with undo_apply / repair::undo - the same
 * CRUNDO01 container 'fix --undo-apply' consumes). */
u32 execute_apply(IApplyDevice* dev, const ApplyPlan& plan, ApplyConsentFn consent,
                  void* consent_user, repair::UndoLog& undo);

/* ---- Phase 3: undo (RESTORE) ----
 * Replays the undo log's original bytes (byte-exact, verified) and the
 * properties refresh (the corpus chains UPDATE_PROPERTIES after every
 * layout mutation, including MainModule's restore path MM:83206-83212).
 * Equivalent to repair::undo for the bytes; adds the refresh replay for
 * apply devices. */
u32 undo_apply(IApplyDevice& dev, const repair::UndoLog& undo);

/* MBR type byte for a candidate family (the family map; Microsoft
 * families use the historical type bytes, the Linux/other families the
 * conventional Linux/Solaris ones - the og's own map is not decoded in
 * the corpus for this path, registered in PARITY-DELTAS as convention). */
u8 mbr_type_for_family(lost::CandidateFs fs);

/* ---- N-145: the cross-run in-flight journal ---------------------------- */

/* The og marks the target disk in ../res/rwConfig.ini's [QuickPartition]
 * pendingMarkedDisk list while a whole-disk repartition op is in flight
 * (the engine-side event 0x102d lifecycle: mark at begin, remove at
 * completion - RecoveryModule_decompiled.c:48164-48315, the shared
 * epmpublic utility static-linked into six modules; the trigger at
 * MainModule:123130-123144, the CDiskRepartitionPendingOperation cast).
 * The next product start can read the marker and detect an interrupted
 * apply. The core's CRUNDO01 transaction is per-operation (stronger
 * while the process lives); this journal is the missing cross-run trace
 * when the process dies mid-apply.
 *
 * REGISTERED DECLINE (the og's own defect, per the finding's fix plan):
 * the og's ini sits CWD-relative next to the process binary - two
 * instances or a service/CLI pair with different working directories
 * maintain different marker lists. The core's journal is a single file
 * at a host-supplied state-directory path (the CLI derives it beside
 * the session/undo artifacts); the library itself stays pure - the
 * journal helpers are explicit calls the hosts bracket their
 * execute_apply / undo_apply windows with.
 *
 * File format: one line per in-flight disk,
 *   <identity>\t<begin-unix-seconds>\n
 * identity is the host's stable disk identity string (the CLI uses
 * "\\\\.\\PhysicalDriveN"; a UI host may use its own - the journal
 * only requires the same string at begin and end). All three
 * operations are idempotent: begin refreshes the timestamp of an
 * existing identity line; end removes the line if present (a missing
 * file or identity is OK - the no-op); probe on a missing file
 * reports zero in-flight disks. */
struct InFlightDisk {
    char identity[96]; /* NUL-terminated host disk identity */
    u64 begin_unix;    /* when the journaled apply began */
};

/* Marks the disk in flight (append or timestamp-refresh). Returns
 * ERR_WRITE_FAILED on an unwritable path; the caller decides whether a
 * journal failure is fatal (the CLI warns and proceeds - the journal is
 * a crash trace, not a correctness gate). */
u32 inflight_begin(const char* journal_path, const char* disk_identity);

/* Removes the in-flight mark (the op completed or cleanly failed -
 * either way it is no longer in flight; only process death leaves the
 * mark behind, which is exactly the detectable class). */
u32 inflight_end(const char* journal_path, const char* disk_identity);

/* Reads the surviving marks (the next-start probe). out may be nullptr
 * with max_records == 0 to count only. Returns OK with *found set;
 * ERR_TRUNCATED when more records exist than max_records (found
 * still carries the true count). A missing journal is OK / found == 0. */
u32 inflight_probe(const char* journal_path, InFlightDisk* out, u32 max_records, u32* found);

} // namespace apply
} // namespace cr

#endif // CR_APPLY_H_
