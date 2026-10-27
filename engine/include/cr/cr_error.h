/*
 * cr_error.h - error-code family for the recovery stack.
 *
 * M-049 (diff-to-fix) - the value-space truth: the og's engine errors are
 * 12-hex-digit HRESULT-shaped codes with only TWO values corpus-documented
 * (open-fail 0x5002200000004, read-fail 0x5002c0000004 - architecture/05
 * section 4 :62); the og's full code table is NOT corpus-documented, and its
 * error-object architecture (CErrorManager/CErrorTrackManager, the MOM bus)
 * is a UI-layer surface. This core owns its error space: the FAMILY-PREFIX
 * convention matches (0x5002 open/transport .. 0x5009, HRESULT-style), and
 * the numeric values are CORE-INTERNAL (registered PARITY-DELTAS D-68) -
 * the old "reproduces the reference engine convention" claim was false as
 * written (value equality was never true for any code). Success is 0.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_ERROR_H_
#define CR_ERROR_H_

#include "cr_types.h"

namespace cr {

enum Err : u32 {
    OK = 0,

    /* --- open / transport (reference open-fail class 0x5002...) --- */
    ERR_OPEN_FAILED = 0x50020001,
    ERR_READ_FAILED = 0x50020002,
    ERR_WRITE_FAILED = 0x50020003,
    ERR_RANGE_OUT_OF_BOUNDS = 0x50020004,
    ERR_CANCELLED = 0x50020005,
    ERR_RETRY_EXHAUSTED = 0x50020006,
    /* Mandatory-driver session (2026-10-27): the RDA kernel driver is a
     * REQUIRED product component — every raw-disk read/write path rides
     * it and there is no direct-handle fallback (the user directive that
     * supersedes the reference's duality; docs/PARITY-DELTAS.md D-FALL).
     * Returned when the \\.\{service} control device cannot be opened
     * (driver not installed, not started, or blocked by signing policy). */
    ERR_DRIVER_REQUIRED = 0x50020007,

    /* --- format / parse --- */
    ERR_BAD_MAGIC = 0x50030001,
    ERR_BAD_CHECKSUM = 0x50030002,
    ERR_BAD_PARAMETER = 0x50030003,
    ERR_TRUNCATED = 0x50030004,
    ERR_FIXUP_FAILED = 0x50030005,
    ERR_CYCLE_LIMIT = 0x50030006,
    ERR_DEPTH_LIMIT = 0x50030007,
    ERR_UNSUPPORTED = 0x50030008,
    ERR_NO_MEMORY = 0x50030009,

    /* --- session --- */
    ERR_SESSION_FORMAT = 0x50040001,
    ERR_SESSION_IO = 0x50040002,
    ERR_SESSION_VERSION = 0x50040003,

    /* --- recovery writer / policy --- */
    ERR_DEST_OVERLAP = 0x50050001,
    ERR_DEST_UNWRITABLE = 0x50050002,
    ERR_HASH_MISMATCH = 0x50050003,
    /* N-015: destination full (ENOSPC mapped at the DestIo layer; the
     * writer fast-fails the remaining queue instead of attempting every
     * item, and the pre-check refuses upfront when free space is known) */
    ERR_DEST_FULL = 0x50050004,
    /* N-065: collision rename space exhausted (the og's cap-10000 loop:
     * DRWUI :357131 `if (10000 < counter) break` - the file is FAILED,
     * never a synthesized fallback name) */
    ERR_DEST_NAME_EXHAUSTED = 0x50050005,

    /* --- bitlocker policy --- */
    ERR_FVE_LOCKED = 0x50060001,

    /* --- health / repair --- */
    ERR_HEALTH_UNAVAILABLE = 0x50070001,
    ERR_REPAIR_REFUSED = 0x50080001,
    ERR_REPAIR_UNDO_FAILED = 0x50080002,
    ERR_VOLUME_LOCK_FAILED = 0x50080003, /* S-010: volume busy, quiesce refused */
    /* S-029 apply (consent-gated lost-partition table write; the convergent
     * legs' codes unified into the repair-apply family) */
    ERR_APPLY_CONSENT_DENIED = 0x50080004, /* A1: the consent callback refused */
    ERR_APPLY_STALE = 0x50080005,          /* A2: sector 0 changed since planning */
    ERR_APPLY_REFUSED = 0x50080006,        /* A3: plan-level refusal (overlap/scope/capacity) */
    ERR_APPLY_REFRESH_FAILED = 0x50080007, /* written+verified; the 3x refresh tail
                                            * failed (the og swallows this silently) */

    /* --- image containers (S-030: vmdk/vhd/vhdx scan targets) --- */
    ERR_IMAGE_CORRUPT = 0x500A0001, /* structural nonsense inside the container */

    /* --- generic engine failure (task exceptions, defensive) --- */
    ERR_INTERNAL = 0x50090001,
};

/* Human-readable text for codes used in CLI output and logs. */
const char* err_text(u32 code);

} // namespace cr

#endif // CR_ERROR_H_
