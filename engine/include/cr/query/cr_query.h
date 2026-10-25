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

enum class SortKey : u8 { None = 0, Name, Size, Modified, Confidence, Path, Grade };
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
