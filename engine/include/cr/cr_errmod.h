/*
 * cr_errmod.h - error-context module attribution (M-049, diff-to-fix).
 *
 * The og's error surface is an error-object architecture: every error
 * carries module/proc context across the MOM bus (Error.md census:
 * CErrorManager/CErrorTrackManager + IErrorModule/IErrorProc + per-module
 * CMomError registrars). The manager/track-manager layer is a UI-bus
 * surface out of core scope; the ENGINE-SIDE SUBSET is what this header
 * provides: which core module was reading when a persistent read failure
 * was recorded, so the error-range event family can carry a
 * module/operation/retry-state triple (the finding's registered step 2).
 *
 * Attribution model (honest at its documented granularity):
 *   - PER-THREAD stamp: the orchestrator stamps its thread (and every
 *     pool task it owns) with the module of the sub-pass it is running.
 *     Exact for the main engine pass, the probe/table reads, the lost
 *     discovery, the parallel lost-candidate engine scans (each task
 *     stamps its candidate's family) and the carve sweep.
 *   - PER-DEVICE owner fallback: engine-INTERNAL worker pools (the NTFS
 *     remnant fan-out, the carve window fan-out) read with no thread
 *     stamp; the retry device falls back to the orchestrator-thread
 *     "owner" module published for the enclosing sub-pass. The owner is
 *     written only by the orchestrator thread between sub-passes (they
 *     are sequential; pools join before the next sub-pass starts), so
 *     the fallback is race-free.
 *   - kNone: unattributed - the pre-stamp state, or ranges reloaded
 *     from sessions written before this landing (sessions persist the
 *     geometric trio only; module attribution is live-scan metadata -
 *     the og's persisted lostpart records carry geometry too,
 *     PRD:20409).
 *
 * The numeric ids are CORE-INTERNAL (the D-68 value-space policy: the
 * og's registry keys are string class-names, not these numbers) and are
 * mirrored in the cr_ui ABI as CR_ERRMOD_* (pinned by static_asserts in
 * cr_ui.cpp - the ABI truth stays self-contained C).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_ERRMOD_H_
#define CR_ERRMOD_H_

#include "cr_types.h"

namespace cr {
namespace errmod {

/* Module ids - the engine-side subset of the og's per-module error
 * registrars. Stable numbering (ABI-mirrored); append-only. */
enum Module : u32 {
    kNone = 0,         /* unattributed (thread stamp unset / pre-M-049 sessions) */
    kOrchestrator = 1, /* probe/table/geometry reads owned by the scan layer */
    kNtfs = 2,
    kFat = 3,
    kExfat = 4,
    kExt4 = 5,
    kBtrfs = 6,
    kHfsplus = 7,
    kRefs = 8,
    kZfs = 9,
    kCarve = 10, /* the carve stage's grammar sweep */
    kLost = 11,  /* lost-partition discovery */
    kLdm = 12,   /* the dynamic-disk (LDM) database parse + volume scans */
};

/* Operations - the error-range family is read failures today. Stable
 * numbering (ABI-mirrored as CR_ERROP_*). */
constexpr u32 op_none = 0;
constexpr u32 op_read = 1;

/* This thread's module stamp (kNone when unset - the fallback signal
 * the retry device consumes). Thread-safe by construction. */
u32 current();

/* Set this thread's stamp; returns the previous value. Callers that can
 * be entered re-entrantly should prefer the Guard. */
u32 set_thread(u32 module);

/* RAII thread stamp: sets on construction, restores the previous stamp
 * on destruction (pool threads are reused - an unrestored stamp would
 * leak into the next task). */
class Guard {
public:
    explicit Guard(u32 module) : prev_(set_thread(module)) {}
    ~Guard() { set_thread(prev_); }
    Guard(const Guard&) = delete;
    Guard& operator=(const Guard&) = delete;

private:
    u32 prev_;
};

} // namespace errmod
} // namespace cr

#endif // CR_ERRMOD_H_
