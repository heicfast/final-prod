/* cr_error.cpp - error text table (pure).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/cr_error.h"

namespace cr {

const char* err_text(u32 code) {
    switch (code) {
    case OK:
        return "OK";
    case ERR_OPEN_FAILED:
        return "open failed";
    case ERR_READ_FAILED:
        return "read failed";
    case ERR_WRITE_FAILED:
        return "write failed";
    case ERR_RANGE_OUT_OF_BOUNDS:
        return "range out of bounds";
    case ERR_CANCELLED:
        return "cancelled";
    case ERR_DEVICE_DISCONNECTED:
        return "target device disconnected (reattach and resume from the session checkpoint)";
    case ERR_RETRY_EXHAUSTED:
        return "retries exhausted";
    case ERR_BAD_MAGIC:
        return "bad magic";
    case ERR_BAD_CHECKSUM:
        return "bad checksum";
    case ERR_BAD_PARAMETER:
        return "bad parameter";
    case ERR_TRUNCATED:
        return "truncated data";
    case ERR_FIXUP_FAILED:
        return "USA fixup failed";
    case ERR_CYCLE_LIMIT:
        return "chain cycle limit";
    case ERR_DEPTH_LIMIT:
        return "depth limit";
    case ERR_UNSUPPORTED:
        return "unsupported";
    case ERR_NO_MEMORY:
        return "out of memory";
    case ERR_SESSION_FORMAT:
        return "session format error";
    case ERR_SESSION_IO:
        return "session I/O error";
    case ERR_SESSION_VERSION:
        return "session version unsupported";
    case ERR_DEST_OVERLAP:
        return "destination overlaps source";
    case ERR_DEST_UNWRITABLE:
        return "destination not writable";
    case ERR_HASH_MISMATCH:
        return "hash mismatch after write";
    case ERR_DEST_FULL:
        return "destination full";
    case ERR_DEST_NAME_EXHAUSTED: /* N-065 (was absent from this table) */
        return "collision rename space exhausted";
    case ERR_DEST_PATH_TOO_LONG: /* M-074: the og's numbered taxonomy */
        return "the recovery path is too long";
    case ERR_DEST_PATH_INVALID:
        return "might be an invalid path to save";
    case ERR_DEST_NETWORK:
        return "network destination (the local-only destination contract)";
    case ERR_FVE_LOCKED:
        return "volume locked (FVE)";
    case ERR_FVE_ELEVATION_REQUIRED: /* M-105 option (a) */
        return "FVE auto-unlock keys exist but require an elevated token";
    case ERR_HEALTH_UNAVAILABLE:
        return "health data unavailable";
    case ERR_REPAIR_REFUSED:
        return "repair refused by policy";
    case ERR_REPAIR_UNDO_FAILED:
        return "repair undo failed";
    case ERR_VOLUME_LOCK_FAILED:
        return "volume lock failed (volume busy - close other programs and retry)";
    case ERR_APPLY_CONSENT_DENIED: /* S-029 */
        return "apply consent denied (nothing was written)";
    case ERR_APPLY_STALE: /* S-029 */
        return "partition table changed since planning (re-run discovery and replan)";
    case ERR_APPLY_REFUSED: /* S-029 (the union leg's plan-level refusals) */
        return "apply refused by policy";
    case ERR_APPLY_REFRESH_FAILED: /* S-029 (the union leg's refresh honesty) */
        return "applied but disk properties refresh failed (undo available)";

    case ERR_IMAGE_CORRUPT:
        return "image container structurally corrupt";
    case ERR_INTERNAL:
        return "internal error";

    /* raw-disk privilege precondition (the direct-access mandate; the
     * CLI prints the full remediation text, err_text carries the class
     * name) */
    case ERR_ELEVATION_REQUIRED:
        return "raw disk access requires an elevated process (run as administrator)";
    default:
        return "unknown error";
    }
}

} // namespace cr
