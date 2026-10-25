/*
 * cr_report.h - the machine-readable recovery report (G-09, R7.3).
 *
 * Pure-core JSON emitter over the writer's per-item outcomes: a versioned,
 * strictly-escaped document (schema in docs/recovery-report-schema.md)
 * sufficient for the UI to present results without re-deriving them.
 * Write-only by design - the core never parses reports back; the fuzz
 * surface is the serializer's crash-freedom under hostile strings.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_REPORT_H_
#define CR_REPORT_H_

#include "../storage/cr_device.h"
#include <string>
#include <vector>

namespace cr {
namespace recovery {
namespace report {

/* Wire values (docs/recovery-report-schema.md v1) - keep in sync with the
 * serializer's spelling below. */
enum class Outcome : u8 {
    Done = 0,             /* content committed (files) or directory created */
    Refused = 1,          /* refused before any write (e.g. dest overlap) */
    Error = 2,            /* write/commit failed (reason carries the code) */
    HashMismatch = 3,     /* committed, but the stored hash did not match */
    Resumed = 4,          /* final already existed - counted + skipped */
    Pending = 5,          /* not attempted: the pass aborted before this item
                           * (truthful reporting of a partial run) */
    Salvaged = 6,         /* committed with zero-filled spans (unreadable
                           * device ranges / undecodable blocks) - ours-only
                           * truth marker (FB-R02): byte count is right but not
                           * every byte is recovered content */
    NothingToRecover = 7, /* GAP-S6-7: skipped BEFORE any write - the node
                           * carries no extents and no resident data (metadata-
                           * only reconstruction) while claiming a positive
                           * size; the og emits no content for metadata-only
                           * entries (root-causes permanent_delete). Empty
                           * files (size 0) are real content and stay Done. */
};

enum class HashStatus : u8 {
    NotComputed = 0, /* refused / error / resumed / directory item */
    Computed = 1,    /* writer streamed a hash; no stored hash to compare */
    Verified = 2,    /* stored hash existed and matched */
    Mismatch = 3,    /* stored hash existed and did not match */
};

struct Item {
    u64 item_id = 0;  /* queue item id (monotonic per run) */
    std::string path; /* UTF-8 destination path ('/'-joined) */
    std::string name; /* UTF-8 final name (collision-resolved) */
    Outcome outcome = Outcome::Done;
    u32 reason = 0;         /* cr error code; 0 when outcome is Done */
    u64 bytes = 0;          /* logical size attempted (files) */
    u64 duration_ms = 0;    /* per-item processing time */
    std::string sha256_hex; /* lowercase hex, empty when not computed */
    HashStatus hash_status = HashStatus::NotComputed;
    bool is_dir = false;
};

struct Totals {
    u64 queued = 0;
    u64 done = 0; /* files committed */
    u64 dirs = 0; /* directories materialized (G-08) */
    u64 refused = 0;
    u64 errors = 0;
    u64 hash_mismatch = 0;
    u64 resumed = 0;
    u64 salvaged = 0;           /* committed with zero-filled spans (FB-R02); a
                                 * sub-bucket of done (both are commits) */
    u64 nothing_to_recover = 0; /* GAP-S6-7: metadata-only items skipped
                                 * before any write (separate bucket, NOT a
                                 * done sub-bucket - nothing was written) */
    u64 pending = 0;            /* never attempted in this pass (partial-run truth;
                                 * queued == done + dirs + refused + errors +
                                 * resumed + nothing_to_recover + pending, done
                                 * subsumes the committed sub-buckets hash_mismatch
                                 * and salvaged) */
    u64 bytes = 0;              /* sum of committed file bytes */
    u64 elapsed_ms = 0;
    std::string source;      /* UTF-8 source identity (caller-supplied) */
    std::string destination; /* UTF-8 destination identity */
};

/* Serialize as a complete JSON document (trailing newline, UTF-8).
 * Escaping: \" \\ \b \f \n \r \t and \u00XX for the remaining C0 controls;
 * invalid UTF-8 bytes are emitted as U+FFFD so the document stays valid
 * JSON regardless of recovered names. Deterministic field order (schema). */
std::string build_report(const std::vector<Item>& items, const Totals& totals);

/* The engine version string baked in at build time (CMake-injected). */
const char* engine_version();

/* JSON string escaping for arbitrary UTF-8 (exposed for tests/fuzz). */
std::string escape_json(const std::string& in);

} // namespace report
} // namespace recovery
} // namespace cr

#endif // CR_REPORT_H_
