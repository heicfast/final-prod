/*
 * cr_query.h - core-side query/filter/sort/paging (G-12, issue #8).
 *
 * A pure query layer over one ResultTree: composable predicates, stable
 * sort, count-only mode, and offset/limit paging. Serves the UI's result
 * list (via the cr_ui shim over a snapshot or a session-loaded tree) and
 * the CLI parity path - one implementation, no UI-specific logic.
 *
 * STABILITY CONTRACT: rows carry `node` pointers that are stable between
 * scan stages (cr_node.h) or for the lifetime of a snapshot/session tree.
 * Mid-scan queries must go through the snapshot surface (G-02/G-03).
 *
 * Performance contract (CI-enforced): a filter+count pass over 1M nodes
 * completes in < 200 ms on CI-class hardware (single linear pass, predicate
 * short-circuit, no per-node allocation in the count path).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_QUERY_H_
#define CR_QUERY_H_

#include "../tree/cr_node.h"
#include <string>
#include <vector>

namespace cr {
namespace query {

/* Result categories. The extension membership mirrors the corpus
 * FileFilter.xml (8 categories / 1,481 unique extensions) via the
 * generated table in cr_filefilter_data.cpp (M-023). Email and
 * Bookmarks were absent from the original 64-entry hand table. */
enum class Category : u8 {
    None = 0,
    Image = 1,
    Video = 2,
    Audio = 3,
    Document = 4,
    Archive = 5,
    Other = 6,
    Email = 7,
    Bookmarks = 8,
    /* N-126: the og's "Unsaved File" search-type category (the UI's
     * type-mask bit 6, DRWUI :367384/:368807) - the office
     * unsaved-document family. The engine-side membership decoded from
     * the corpus: the Word autosave/backup pair (.asd/.wbk - the
     * finding's named members; the og's full table - the office tmp
     * forms and the ~-prefixed locks - is not decoded anywhere in the
     * corpus: the R-27 assistant space carries only the six 5-0xB
     * categories, the UI layer owns the bit). APPEND-ONLY (serialized
     * category values stay stable). */
    Unsaved = 9,
};

/* "jpg" (no dot, any case) -> Image; unknown extensions -> Other.
 * Binary search over the generated 1,481-entry table. */
Category category_for_extension(const std::string& ext_utf8);

/* ---- N-132: the og's two display-config data layers ----
 *
 * SumaryFileType (FilterFolder.ini [Filter]): the 11 extension-unification
 * groups - the members are counted/presented as one family ("MP4 family:
 * N files" - the container-identity grouping). The core lands the groups
 * as this lookup; the ABI host's summary presentation consults the same
 * table the og's UI reads (the layering registered in PARITY-DELTAS
 * D-113). NULL = the extension stands alone.
 *
 * PriorityDisplayType.xml: the 5-category priority tier (Picture/Video/
 * Document/Audio/Archive) with the 35 ranked extensions - the Type tree
 * orders extensions WITHIN each category by the 2-digit alias key
 * (jpg=00 before png=01 before jpeg=02...). The lookup returns the
 * extension's category + rank; false = unranked (the og's Type tab lists
 * unranked extensions after the ranked set, unordered). */
enum class TypeDisplayCategory : u8 {
    Picture = 0,
    Video = 1,
    Document = 2,
    Audio = 3,
    Archive = 4,
};

const char* summary_family_for_extension(const char* ext_utf8_lower);
bool type_display_order(const char* ext_utf8_lower, TypeDisplayCategory& out_category,
                        u32& out_rank);
const char* type_display_category_name(TypeDisplayCategory c);

/* ---- M-125: the og's progress DISPLAY math (DRWUI decode) ----
 * The og's displayed scan percent is not the raw scan completion:
 * DRWUI computes raw = done*100/total from the sector-based engine
 * messages, then (for the type-1 progress messages) applies a
 * piecewise remap and a defensive stall cap. These helpers land that
 * display math as an OPTIONAL query-layer surface so hosts can render
 * og-faithful UX; the engine's own telemetry stays the TRUTHFUL
 * linear weighted permille (cr_scan's weighted_progress - the
 * deliberate engine-side truth registered in PARITY-DELTAS; the remap
 * is the og's display layer, D-118). */

/* FUN_1401af820 (DRWUI :314931-314959) verbatim - the piecewise remap:
 *   raw < 11        -> raw * 2.0
 *   11 <= raw < 30  -> (raw - 11) * 0.2 + 20.0
 *   30 <= raw <= 90 -> raw * 0.8
 *   90 < raw <= 98  -> (raw - 88) * 2.7 + 72.0
 *   raw > 98        -> raw
 *   result > 100 && raw != 100 -> 99.99 (the defensive stall cap - for
 *   raw in [0,100] every segment is <= 100, so the cap is the og's
 *   dead defensive branch, reachable only for out-of-range raw; ported
 *   as decoded) */
double progress_display_remap(double raw_percent);

/* FUN_1401af950 (DRWUI :314964-314986) - the display percent for one
 * engine progress message: message_type 0 -> 0.0 (the reset), 1 ->
 * REMAP(raw) (clamped to 100.0), anything else -> raw as-is. */
double progress_display_percent(u32 message_type, u64 done, u64 total);

/* FUN_1401afa60 (DRWUI :315014-315024) - the monotonic publish guard:
 * a lower value never lowers the displayed percent (the completion
 * message bypasses the guard with its forced 100). */
double progress_display_publish(double current, double next);

/* ---- M-126: the og's ETA model (DRWUI decode) ----
 * FUN_1401af410 (DRWUI :314835-314887, re-read first-hand at the fix):
 * on every progress message the og computes a remaining-time estimate -
 * a whole-run base ETA, an EMA-smoothed rate (0.2 instant / 0.8
 * history), a type-gated time-budget variant (x0.6), and a 1 Hz local
 * countdown that ticks the label between computed refreshes
 * (:1527650-1527768; runtime anchor "Remaining time: 07m43s" at 34%,
 * dynamic-ground-truth :95). These helpers land the model as a pure
 * query-layer surface so hosts render og-faithful estimates; the
 * telemetry snapshot carries the smoothed rate so hosts need no state
 * of their own (cr_scan.h - ScanTelemetrySnapshot.eta_*). */

struct EtaModel {
    /* the og's +0x100 field: -1 = the parameter-error class (done == 0,
     * total < done, or total == 0 - FUN_1401af410's opening gate) */
    long long eta_seconds = -1;
    /* the EMA state (the og's +0x118): feed back as prev_smoothed on
     * the next call; 0 = no history (the first sample passes through) */
    unsigned long long smoothed_rate = 0;
};

/* The computation core, verbatim per the decode:
 *   eta_s  = (total - done) * elapsed_ms / done / 1000   (whole-run)
 *   rate   = (total - done) / eta_s
 *   smooth = prev != 0 ? (2*rate + 8*prev) / 10 : rate   (EMA 0.2/0.8)
 *   !time_budget_variant: eta_seconds = eta_s            (the type-0 leg)
 *   time_budget_variant:  projected = max(1, total * elapsed_s / done)
 *                         eta_seconds = (budget - budget * elapsed_s /
 *                                        projected) * 0.6
 * elapsed_s is INTEGER seconds (the og's tick math divides by 1000
 * before the budget leg). Returns {-1, 0} for the parameter-error
 * class; eta_seconds stays -1 when eta_s <= 0 (no estimate). */
EtaModel eta_compute(unsigned long long done, unsigned long long total,
                     unsigned long long elapsed_ms, unsigned long long prev_smoothed,
                     bool time_budget_variant, unsigned long long budget_seconds);

/* The label renderer: the computed refresh renders minutes and seconds
 * zero-padded to two digits each - the runtime anchor's "07m43s" shape
 * (the empty-label placeholder is the LITERAL "3m00s", :1527664-7669). */
std::string eta_format_m_s(long long seconds);

/* The 1 Hz countdown pair (:1527550-1527768): parse the label's
 * "<minutes>m<seconds>s" (the og sections on 'm' then 's' - any
 * non-digit padding like the leading zero is tolerated), subtract one
 * second. Returns the ticked seconds, or -1 when the label does not
 * parse or the countdown reached zero (the og's stop condition:
 * minutes*60 - 1 + seconds < 0 -> the FUN_1409cb4e0 stop). */
long long eta_countdown_tick(const std::string& label);

/* ---- N-194: the og's two byte-size display formatters (DRWUI decode) ----
 *
 * display_size_log - FUN_140163580 (wide, DRWUI :264525-264550) and
 * FUN_140163650 (ascii, :264552-264583), verbatim tier structure:
 *   bytes < 1            -> nothing written (the og returns 0 with the
 *                           caller's buffer untouched; the empty string
 *                           is the C++ surface's "nothing written")
 *   bytes < 0x100001     -> "%.2f KB", value = bytes / 1024.0
 *   bytes < 0x40000001   -> "%.2f MB", value = bytes / 1048576.0
 *   else                 -> "%.2f GB", value = bytes / 1073741824.0
 * Binary (1024-tier) math under decimal KB/MB/GB labels, NO bytes tier,
 * NO TB tier, and the +1 flip boundaries: exactly 1 MiB (0x100000)
 * renders "1024.00 KB", exactly 1 GiB renders "1024.00 MB", 2 TiB
 * renders "2048.00 GB". Runtime anchor: "Files Found: 1139157
 * (143.74 GB)" - GiB under a GB label (dynamic-ground-truth :95). */
std::string display_size_log(u64 bytes);

/* display_size_ui - the tr-family formatter (DRWUI :1556295-1556400):
 *   bytes < 0x401            -> "%1 B"   (value as-is)
 *   bytes < 0x100001         -> "%1 KB", value = bytes / 1024.0
 *   bytes < 0x40000001       -> "%1 MB", value = bytes / 1048576.0
 *   bytes < 0x10000000001    -> "%1 GB", value = bytes / 1073741824.0
 *   else                     -> "%1 TB", value = bytes / 1099511627776.0
 * 'f' format with the CALLER's precision; bytes == 0 forces precision 0
 * (the og's local_100 = 0 quirk - "0 B"); an all-zero fractional part
 * that spans the FULL precision (".00" for precision 2) is stripped
 * with its point ("5 KB", not "5.00 KB" - the og's replace(".%1 zeros",
 * "") tail). Negative precision clamps to 0. */
std::string display_size_ui(u64 bytes, int precision);

/* The generated table's entry count (regeneration-drift guard:
 * Filter.TableCount pins it to the corpus unique count). */
size_t filefilter_entry_count();

/* The predicate set. All conditions AND together; unset fields are
 * pass-through. Sizes are logical bytes; dates are the node's `modified`
 * time in 100-ns units (FileTimes wire unit). */
struct Filter {
    u32 category_mask = 0x3FF; /* Category bits (None..Unsaved - N-126 widened from 0x1FF) */
    u64 size_min = 0;
    u64 size_max = ~u64(0);
    u64 modified_from = 0;
    u64 modified_to = ~u64(0);
    u32 confidence_min = 0;              /* tree::Confidence ordinal floor (0..4) */
    std::vector<std::string> extensions; /* lowercased, WITH the dot */
    std::string name_substring;          /* UTF-8, ASCII-case-insensitive */
    /* og AdvanceFilterCondition path group (corpus 31-filterbar-family-decode.md
     * §3-§6): the row's FULL path (root role + ancestors + own name,
     * '/'-joined - the same string Row::path_utf8 produces) must start with
     * this prefix, ASCII-case-insensitive. Empty = no filter (zero-init
     * safe); the walk prunes subtrees whose accumulated path is
     * prefix-incompatible with the filter. */
    std::string path_prefix;
    /* M-072: the path group's substring form (the og's filter-bar path
     * box matches on substrings; the prefix form above is the
     * corpus-decoded M-029 half) - ASCII-case-insensitive over the
     * materialized full path, evaluated only when active (the
     * no-string-conversion fast lane keeps the 1M-node budget). */
    std::string path_substring;
    /* M-072: the state dimension as a MASK (the og's 7-group filter
     * carries state as a sub-condition group; the boolean below is its
     * one-decoded "Lost files only" control). Bits map onto the core's
     * deleted-state axis: bit0 Active, bit1 Deleted, bit2 Partial,
     * bit3 Damaged (tree::DeletedState ordinals). 0 = all states.
     * deleted_only stays as sugar for the Deleted bit (back-compat). */
    u32 state_mask = 0xF;
    bool deleted_only = false;
    u32 origin_mask = 0xF; /* tree::Origin bits */
    /* M-101: the Versions-branch axis (composes with every other
     * condition): nonzero = only rows whose namespace-identity hash
     * matches (the branch's per-group membership; enumerate the groups
     * first via version_groups()). 0 = no filter. */
    u64 version_group = 0;
    /* M-101: the Tags-Extracted branch - only rows whose props carry the
     * runtime tags_extracted flag (the host's preview/metadata pass
     * stamps it; the branch is runtime grouping, the flag is not
     * persisted - a reloaded session re-derives by re-running preview). */
    bool tags_only = false;

    /* effective state mask: deleted_only sugar ORs in the Deleted bit */
    u32 effective_state_mask() const {
        u32 m = state_mask == 0 ? 0xFu : state_mask;
        if (deleted_only)
            m &= ~1u; /* drop Active (the Lost-files-only semantics) */
        return m;
    }
};

/* ---- M-029: the og filter-bar's Date/Size bucket IDs ----------------
 * The og's AdvanceFilterCondition carries its Date/Size groups as stable
 * numeric bucket IDs, not ranges (DRWUI CTimeFilterBtn menu FUN_140705c50:
 * 0x100 Today / 0x101 Yesterday / 0x102 "Last 7 days" / 0x103 "Last 1
 * month" / 0x104 SKIPPED / 0x105 Custom; CSizeFilterBtn menu FUN_140706060:
 * 0x1000 "< 512KB" / 0x1001 "512KB~1MB" / 0x1002 "1MB~512MB" / 0x1003
 * "512MB~1GB" / 0x1004 "> 1GB" / 0x1005 SKIPPED / 0x1006 Custom).
 *
 * The evaluation semantics (both decoded first-hand against the corpus
 * decompile, DRWUI FUN_140254670 + the tag setters FUN_1402556f0-family /
 * FUN_140255820 + the group matchers FUN_1402566d0/FUN_140256b20):
 * - Date buckets compare the node's modified time as a DAY-TRUNCATED
 *   LOCAL value against bounds derived from "today" (day-truncated local
 *   now, cached once per scan): Today == today; Yesterday == today-1d;
 *   Last-7-days = (today-7d, today]; Last-1-month = (today-30d, today]
 *   (COleDateTimeSpan 1/7/30 wall-clock day arithmetic). Custom is
 *   inclusive on day-truncated local [from, to].
 * - Size buckets are exact byte ranges: <= 512 KiB, (512 KiB, 1 MiB],
 *   (1 MiB, 512 MiB], (512 MiB, 1 GiB], > 1 GiB; Custom inclusive.
 * The skipped IDs (0x104, 0x1005) have NO og semantics - the translation
 * rejects them rather than inventing constants. */
enum DateBucket : u16 {
    DateBucket_None = 0, /* leave the filter untouched */
    DateBucket_Today = 0x100,
    DateBucket_Yesterday = 0x101,
    DateBucket_Last7Days = 0x102,
    DateBucket_Last1Month = 0x103,
    /* 0x104 skipped by the og's menu */
    DateBucket_Custom = 0x105,
};

enum SizeBucket : u16 {
    SizeBucket_None = 0, /* leave the filter untouched */
    SizeBucket_To512KiB = 0x1000,
    SizeBucket_To1MiB = 0x1001,
    SizeBucket_To512MiB = 0x1002,
    SizeBucket_To1GiB = 0x1003,
    SizeBucket_Over1GiB = 0x1004,
    /* 0x1005 skipped by the og's menu */
    SizeBucket_Custom = 0x1006,
};

/* Translate a Date bucket ID into the filter's modified range. now_ft is
 * the current time in FILETIME units (the local-day window is derived
 * from it exactly as the og derives its cached bounds); the Custom bucket
 * uses custom_from_ft/custom_to_ft (raw FILETIMEs, day-truncated locally,
 * both ends inclusive). Returns false (filter untouched) for unknown or
 * skipped IDs. */
bool apply_date_bucket(Filter& f, u16 bucket, u64 now_ft, u64 custom_from_ft = 0,
                       u64 custom_to_ft = 0);

/* Translate a Size bucket ID into the filter's size range (exact og
 * boundaries; the Custom bucket is inclusive on both ends). Returns false
 * (filter untouched) for unknown or skipped IDs. */
bool apply_size_bucket(Filter& f, u16 bucket, u64 custom_min = 0, u64 custom_max = ~u64(0));

enum class SortKey : u8 {
    None = 0,
    Name,
    Size,
    Modified,
    Confidence,
    Path,
    Grade,
    /* M-029: the og's Type column (results-view column
     * 3, corpus UIUX results-view captures). Ordering:
     * category (the FileFilter.xml iconIndex display order:
     * Graphic/Document/Email/Audio/Video/Other/Archive/
     * Bookmarks, iconIndex 0-7), then extension, then name.
     * None-category rows sort last. */
    Type
};
enum class SortDir : u8 { Asc = 0, Desc };

struct Query {
    Filter filter;
    SortKey sort = SortKey::None; /* None = emission order (stable) */
    SortDir dir = SortDir::Asc;
    u64 offset = 0;
    u32 limit = 100;         /* rows to return after offset */
    bool count_only = false; /* total_matched only; rows empty */
};

/* Numeric wire values of the node's enums (UI agents consume these
 * verbatim; they match cr_node.h's declared order). */
struct Row {
    const tree::FileNode* node = nullptr;
    std::string name_utf8;
    std::string path_utf8; /* root role + ancestor names, '/'-joined */
    u64 size = 0;
    u64 modified = 0;
    u32 origin = 0;     /* tree::Origin */
    u32 deleted = 0;    /* tree::DeletedState */
    u32 confidence = 0; /* tree::Confidence ordinal (0..4) */
    u32 category = 0;   /* Category */
    /* M-101: the Versions-branch axis (nonzero = the record's
     * namespace-identity hash; rows sharing a nonzero id are versions
     * of one pathname - the og's Versions node membership) and the
     * Tags-Extracted flag (the runtime branch membership). */
    u64 version_group = 0;
    u32 tags_extracted = 0;
};

struct Result {
    u64 total_matched = 0; /* count BEFORE paging */
    std::vector<Row> rows;
};

/* ---- M-101: the Versions-branch enumeration -------------------------
 * The multi-member namespace groups (the og's Versions node population:
 * "another version of the file in the same directory", UIUX/09:31).
 * One pass over the tree collecting the live nodes' version_group ids;
 * single-member ids (every ordinary file) stay out - the branch holds
 * only the torn-overwrite / record-reuse siblings. The group's display
 * name is the member name (all members share it by construction: the
 * id derives from (parent, name)). */
struct VersionGroup {
    u64 id = 0;          /* the namespace-identity hash (Filter::version_group) */
    u32 count = 0;       /* live members */
    std::u16string name; /* the shared display name */
};
std::vector<VersionGroup> version_groups(const tree::ResultTree& tree);

/* Run one query. Single pass; the sort materializes an index vector only
 * when a sort key is requested. Rows are built only for the returned
 * page (paths are never computed in the count path). */
Result run(const tree::ResultTree& tree, const Query& q);

} // namespace query
} // namespace cr

#endif // CR_QUERY_H_
