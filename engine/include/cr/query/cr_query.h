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
};

/* "jpg" (no dot, any case) -> Image; unknown extensions -> Other.
 * Binary search over the generated 1,481-entry table. */
Category category_for_extension(const std::string& ext_utf8);

/* The generated table's entry count (regeneration-drift guard:
 * Filter.TableCount pins it to the corpus unique count). */
size_t filefilter_entry_count();

/* The predicate set. All conditions AND together; unset fields are
 * pass-through. Sizes are logical bytes; dates are the node's `modified`
 * time in 100-ns units (FileTimes wire unit). */
struct Filter {
    u32 category_mask = 0x1FF; /* Category bits (None..Bookmarks) */
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
    bool deleted_only = false;
    u32 origin_mask = 0xF; /* tree::Origin bits */
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
    DateBucket_None = 0,        /* leave the filter untouched */
    DateBucket_Today = 0x100,
    DateBucket_Yesterday = 0x101,
    DateBucket_Last7Days = 0x102,
    DateBucket_Last1Month = 0x103,
    /* 0x104 skipped by the og's menu */
    DateBucket_Custom = 0x105,
};

enum SizeBucket : u16 {
    SizeBucket_None = 0,        /* leave the filter untouched */
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
bool apply_date_bucket(Filter& f, u16 bucket, u64 now_ft,
                       u64 custom_from_ft = 0, u64 custom_to_ft = 0);

/* Translate a Size bucket ID into the filter's size range (exact og
 * boundaries; the Custom bucket is inclusive on both ends). Returns false
 * (filter untouched) for unknown or skipped IDs. */
bool apply_size_bucket(Filter& f, u16 bucket,
                       u64 custom_min = 0, u64 custom_max = ~u64(0));

enum class SortKey : u8 { None = 0, Name, Size, Modified, Confidence, Path, Grade,
                      /* M-029: the og's Type column (results-view column
                       * 3, corpus UIUX results-view captures). Ordering:
                       * category (the FileFilter.xml iconIndex display order:
                       * Graphic/Document/Email/Audio/Video/Other/Archive/
                       * Bookmarks, iconIndex 0-7), then extension, then name.
                       * None-category rows sort last. */
                      Type };
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
};

struct Result {
    u64 total_matched = 0; /* count BEFORE paging */
    std::vector<Row> rows;
};

/* Run one query. Single pass; the sort materializes an index vector only
 * when a sort key is requested. Rows are built only for the returned
 * page (paths are never computed in the count path). */
Result run(const tree::ResultTree& tree, const Query& q);

} // namespace query
} // namespace cr

#endif // CR_QUERY_H_
