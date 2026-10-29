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
    ReadError = 8,        /* M-135: the og's error-9 family - the read failed
                           * HARD (post-retry) and the item TRUNCATED at the
                           * first unreadable byte; the PARTIAL FILE IS
                           * COMMITTED (the og's runlist path breaks, keeps
                           * what was written, and lists the file failed:
                           * DRWUI :875684-875696 + the recorder :874072-
                           * 874129). cause_id carries the numbered user-
                           * facing cause; first_failed_offset the boundary. */
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
    /* N-159 (diff-to-fix): the results-tree classification the og's
     * failed-files HTML report carries per row (CSaveFailedFilesWithHtml
     * ::FUN_1403c6fc0, DRWUI :642755-642822 - the group/kind-to-label
     * mapping decoded first-hand). Six-valued, derived from the node's
     * root branch at report time (the D-107 lineage: og UI-surface
     * semantics land as core data). Wire values (snake_case, stable;
     * the og's display label for each is in the schema doc):
     *   existing_files | lost_files | lost_files_reconstructed |
     *   lost_files_tags_extracted | lost_files_deleted | <custom
     *   raw-category name>
     * Empty = unclassified (value-enqueued snapshot rows carry no node;
     * the field is then omitted - additive-within-v1 per the schema
     * doc's own version contract). */
    std::string tree_class;
    /* S-060: the metadata restoration outcome (the og's per-item
     * SetFileAttributesW/SetFileTime tail). "restored" | "failed:<code>"
     * | "unsupported" | "" (not attempted: dirs, refused items). The
     * og logs both failures as traces and continues - the core reports
     * the same non-fatal truth per row. */
    std::string metadata;
    /* ---- M-135: the og's per-file failure record (the numbered-cause
     * taxonomy + the truncation boundary). ---- */
    /* the og's numbered user-facing cause (RecoveryFailedInfo1..7, the
     * COMPILED-IN defaults at DRWUI strings :30580-30596 - not the qm
     * file's stale list): 1 insufficient storage space, 2 recovery path
     * too long, 3 garbage characters in the name, 4 the sector reading
     * location exceeds total sector number of the source partition,
     * 5 the correct storage sector location cannot be found (the
     * error-9 read-failure family), 6 exceeded the free quota (the og's
     * licensing gate - the core never produces it), 7 other unknown
     * reasons. 0 = no failure. The og's internal code->cause mapping
     * function is corpus-UNDECODED (FUN_1400ed760 is a %d formatter,
     * refuted first-hand); the core maps by the cause strings'
     * semantics - failure_cause_text() in cr_safewriter.h renders the
     * same numbering for the CLI. */
    u32 cause_id = 0;
    /* the committed byte count (the og's std::filesystem size probe at
     * :874131-874205): equal to `bytes` for full commits; the truncation
     * point for ReadError rows. Emitted in the report only when it
     * differs from `bytes` (additive-within-v1). */
    u64 bytes_written = 0;
    /* the logical offset of the first unreadable byte (ReadError rows;
     * 0 otherwise; emitted only when cause_id != 0). */
    u64 first_failed_offset = 0;
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
    /* M-135: committed-but-truncated items (the og's error-9 family:
     * the partial IS on disk AND the file IS in the failed list -
     * a separate bucket from done and errors). */
    u64 read_error = 0;
    u64 pending = 0; /* never attempted in this pass (partial-run truth;
                      * queued == done + dirs + refused + errors +
                      * resumed + nothing_to_recover + pending, done
                      * subsumes the committed sub-buckets hash_mismatch
                      * and salvaged) */
    u64 bytes = 0;   /* sum of committed file bytes */
    /* N-159: the og's failed-files counters split - CSaveFailedFiles
     * WithHtml's collection slot accumulates the failed count + failed
     * BYTES separately from the committed totals (the results singleton
     * +0x254/+0x268, DRWUI :642729-642746). The failed class here =
     * non-committed files (Error + Refused): the og's write failures
     * plus the destination-refusal family the core's safety hardening
     * adds. NothingToRecover items are excluded (nothing was
     * attempted, no bytes moved). */
    u64 failed_bytes = 0;
    u64 elapsed_ms = 0;
    std::string source;      /* UTF-8 source identity (caller-supplied) */
    std::string destination; /* UTF-8 destination identity */
};

/* S-023: the og RecoveryAssistant's scan-analysis statistics block (8
 * counters + per-category counts + the recoverability gauge). Optional:
 * when present it is emitted as the "scan_analysis" object after the
 * totals (a report built by a writer that has no scan context - resumed
 * sessions, queue-only runs - simply omits it). */
struct ScanAnalysis {
    u64 total_file_count = 0;
    u64 raw_count = 0;
    u64 lost_partition_exist_file_count = 0;
    u64 lost_partition_delete_file_count = 0;
    u64 quick_scan_exist_file_count = 0;
    u64 quick_scan_delete_file_count = 0;
    u64 deep_scan_file_count = 0;
    u64 quick_scan_delete_extend = 0;
    u64 category_counts[8] = {0}; /* Image..Bookmarks (query::Category-1) */
    u64 matched = 0;
    u64 total = 0;
    bool small_result = false;
    /* N-183 (append-only): the member-set gauge + the verdict score. */
    u64 gauge_matched = 0;
    u64 gauge_total = 0;
    u32 score = 0;
    u64 ntfs_device_index_high = 0;
};

/* Serialize as a complete JSON document (trailing newline, UTF-8).
 * Escaping: \" \\ \b \f \n \r \t and \u00XX for the remaining C0 controls;
 * invalid UTF-8 bytes are emitted as U+FFFD so the document stays valid
 * JSON regardless of recovered names. Deterministic field order (schema). */
std::string build_report(const std::vector<Item>& items, const Totals& totals,
                         const ScanAnalysis* analysis = nullptr);

/* The engine version string baked in at build time (CMake-injected). */
const char* engine_version();

/* JSON string escaping for arbitrary UTF-8 (exposed for tests/fuzz). */
std::string escape_json(const std::string& in);

} // namespace report
} // namespace recovery
} // namespace cr

#endif // CR_REPORT_H_
