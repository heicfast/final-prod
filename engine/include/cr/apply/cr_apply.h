/*
 * cr_apply.h - consent-gated lost-partition apply (S-029, diff-to-fix).
 *
 * Corpus contract (PartitionRecovery_decompiled.c, lead-verified citations
 * from the finding): the og's lost-partition flow is a TWO-STEP product
 * feature - discover (the decoded probe family, S-006/S-019 parity), then
 * APPLY: RecoveryModule's CreateRecoveryPartitionObject hosts the apply
 * scenario, PartitionRecovery.dll's FUN_1800566d0 "rebuild disk layout"
 * writes the recovered entry back into the user's table (the legacy chain:
 * IOCTL_DISK_DELETE_DRIVE_LAYOUT 0x7C100 :46953, raw sector-0 rewrite -
 * zeroed entries + 0x55AA :46957-46962, IOCTL_DISK_SET_DRIVE_LAYOUT 0x7C010
 * with a 4-entry DRIVE_LAYOUT_INFORMATION :46995-47002; the modern single-
 * entry path IOCTL 0x7C058 :47015; every write followed by
 * IOCTL_DISK_UPDATE_PROPERTIES 0x70140 with a 3x Sleep(1000) retry
 * :46873-46880).
 *
 * The core's v1 apply implements the SEMANTIC (the recovered partition
 * entry lands in the user's MBR, transactionally, undoably) through the
 * SAME seams RawFixer established (S-008/S-010): the raw sector-0 write
 * path via repair::IWriteDevice (the og's raw-write leg), the volume
 * quiesce in the Windows transport's lock_volume() (the S-010 sequence),
 * and the CRUNDO01 undo container. The kernel-mediated IOCTL chain
 * (DELETE/SET_DRIVE_LAYOUT, UPDATE_PROPERTIES) is the Windows layer's
 * expression of the same rebuild; the properties refresh is the platform
 * caller's post-apply step.
 *
 * SAFETY STANDARD (mirrors cr_rawfixer.h, plus the apply-specific rules):
 *   A1. Consent: plan_apply is read-only; execute_apply calls the consent
 *       callback AFTER quiesce + revalidation and BEFORE the first write;
 *       a denied/absent callback = ERR_APPLY_CONSENT_DENIED, ZERO writes.
 *   A2. Revalidation: execute compares the live sector 0 against the
 *       plan's snapshot - a table that changed since planning = ERR_APPLY_
 *       STALE, replan (the og re-reads sector 0 raw before writing :46944).
 *   A3. Containment: the plan REFUSES candidates that overlap a live
 *       entry, extend past the disk end, sit at LBA 0, or target a
 *       non-MBR (GPT) layout - never a coin-flip write into a table the
 *       plan does not fully understand.
 *   A4. Transaction: the original sector-0 bytes are snapshotted into the
 *       undo log BEFORE the write; every write is read-back verified; any
 *       failure rolls back everything applied (reverse order, verified).
 *   A5. Scope (v1): the MBR entry + the 0x55AA signature restore (the
 *       og's rebuild rewrites it :46957-46962). GPT layouts (the og's
 *       FUN_1800392f0 GUID-regen variant) and the boot-chain side effects
 *       (M-040: BootIni/BCD) are OUT OF SCOPE and refused cleanly - the
 *       plan carries a boot_repair_available field for the future wiring.
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

    bool refused = false;
    std::string refusal_reason;
};

/* Consent callback: called once per execute, after quiesce +
 * revalidation, before the first write. Return false (or pass nullptr)
 * to refuse - nothing is written. Mirrors cr_ui_repair_consent_fn. */
using ApplyConsentFn = bool (*)(const ApplyPlan& plan, void* user);

/* ---- Phase 1: plan (READ-ONLY) ----
 * Derives the MBR entry write for a discovered lost-partition candidate:
 *   - validates sector 0 is an MBR (0x55AA present or restorable; a GPT
 *     protective entry 0xEE = clean refusal - GPT apply is v2 scope)
 *   - picks the first all-zero entry slot (a full table = refusal)
 *   - refuses containment violations: candidate at LBA 0, past the disk
 *     end, or overlapping any live entry (the og never overwrites live
 *     entries in the single-partition recovery flow)
 *   - maps the candidate family to the MBR type byte, computes the CHS
 *     pair (255-heads/63-sector DOS convention, clamped to the maximum
 *     CHS beyond cylinder 1023 - what a BIOS/kernel reports for large
 *     disks; the og's kernel path derives the same through
 *     DRIVE_LAYOUT_INFORMATION)
 *   - snapshots sector 0 for execute-time revalidation + undo
 * A refused plan returns OK with refused=true (the refusal is a RESULT,
 * not an error - the caller surfaces the reason). */
u32 plan_apply(IStorageDevice* disk, const lost::Candidate& c, ApplyPlan& out);

/* ---- Phase 2: execute (CONSENT-GATED TRANSACTIONAL WRITE) ----
 * Sequence: lock_volume() (the S-010 quiesce; failure = ERR_VOLUME_LOCK_
 * FAILED, zero writes) -> re-read + revalidate sector 0 (A2) -> consent
 * (A1) -> snapshot -> write -> read-back verify -> (on any failure) full
 * rollback in reverse order. The undo log is populated on success too
 * (replay with repair::undo - the same CRUNDO01 container 'fix
 * --undo-apply' already consumes). */
u32 execute_apply(repair::IWriteDevice* dev, const ApplyPlan& plan, ApplyConsentFn consent,
                  void* consent_user, repair::UndoLog& undo);

/* MBR type byte for a candidate family (the family map; Microsoft
 * families use the historical type bytes, the Linux/other families the
 * conventional Linux/Solaris ones - the og's own map is not decoded in
 * the corpus for this path, registered in PARITY-DELTAS as convention). */
u8 mbr_type_for_family(lost::CandidateFs fs);

} // namespace apply
} // namespace cr

#endif // CR_APPLY_H_
