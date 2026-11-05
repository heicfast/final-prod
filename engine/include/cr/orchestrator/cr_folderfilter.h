/*
 * cr_folderfilter.h - the scan-time folder filter (M-073, diff-to-fix).
 *
 * Corpus: the og pushes the active AdvanceFilterCondition into the scan
 * controller at scan start (UIUX/31-filterbar-family-decode.md:142,
 * runtime-verified: "FilterPath=" + CDoScanPartition::PushFolderFilter:84 +
 * "AdvanceFilter current time is :..." at scan START - the log sequence in
 * qa-logs--session-forensics.md:26), so a pre-set filter constrains
 * incremental tree insertion, not just the post-hoc results view. The og's
 * effect: a sub-tree scan (e.g. \Users\alice\Documents) never pays
 * tree/merge cost for out-of-scope folders.
 *
 * Core posture (the plan-of-record): the engines still read metadata (the
 * og's filter is a results filter riding the scan); the prune applies at
 * the orchestrator's stage boundaries BEFORE the checkpoint emit - the
 * observable contract is the og's (sessions, snapshots, queries and
 * recovery never see out-of-prefix nodes; checkpoints never persist them).
 *
 * Matching semantics (cr_query.cpp's own prefix family, M-072/M-029):
 * ASCII case-insensitive; '/'-separated accumulated paths (volume-root
 * relative - the result roots themselves are not part of the path, the
 * Row::path_utf8 shape); a FILE matches when its full path starts with a
 * prefix; a FOLDER stays while any descendant could still match
 * (prefix-compatible in either direction - the same path_compatible rule
 * the post-hoc walk uses), and is pruned with its whole subtree when no
 * descendant ever could.
 *
 * Exemptions (the scoping decision, registered with the finding): the
 * carve root and the "File Path Lost" root serve PATHLESS content by
 * definition - a folder-prefix filter has no folder to match there, and
 * pruning pathless content as a side effect of a folder filter would be
 * the silent-data-loss class the mandate forbids. The og's own RAW family
 * is "File Path Lost" - pathless is its contract.
 *
 * Ordering interplay with M-018 (FilterFolder): the folder prune runs at
 * stage boundaries (pre-checkpoint); the junk hidden-set pass runs
 * post-walk, pre-grading as before. A junk-named folder inside an
 * in-prefix path stays flagged-hidden (never pruned by this pass); an
 * out-of-prefix folder is pruned before the junk pass ever sees it - the
 * two filters compose without double-counting (folder_filtered counts
 * pruned files; junk_filtered counts flagged nodes).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_FOLDERFILTER_H_
#define CR_FOLDERFILTER_H_

#include "../cr_types.h"
#include "../tree/cr_node.h"
#include <string>
#include <string_view>
#include <vector>

namespace cr {
namespace folderfilter {

/* What a prune removed (per root; the orchestrator sums and decrements
 * ScanStats truthfully - the presented tree is the truth the counters
 * describe). */
struct PruneStats {
    u64 files = 0;   /* file nodes pruned */
    u64 deleted = 0; /* of those, the deleted-state ones */
};

/* Normalize one caller-supplied prefix: '\' separators become '/', a
 * leading/trailing separator is dropped, ASCII upper folds to lower.
 * Returns false when the result is empty (the prefix is a no-op - the
 * "off" case; callers skip it, never treat it as match-everything). */
bool normalize_prefix(std::string_view in, std::string& out);

/* True when at least one prefix normalizes non-empty (the filter is ON). */
bool any_active(const std::vector<std::string>& prefixes);

/* A FILE's full path matches a prefix when the path starts with it
 * (raw string prefix, no separator boundary - the og filter-bar's own
 * semantics, cr_query.cpp's starts_with_ci family). Both sides are
 * already ASCII-lowercase (normalize_prefix / node_path_key). */
bool file_path_matches(const std::string& path_lower, const std::string& prefix_lower);

/* A FOLDER's accumulated path is kept while any descendant could still
 * match: either the path starts with the prefix (the folder is inside
 * the scope) or the prefix starts with the path (the folder is an
 * ancestor of the scope) - cr_query.cpp's path_compatible, verbatim. */
bool folder_path_compatible(const std::string& dir_lower, const std::string& prefix_lower);

/* The node's name as an ASCII-lowercase UTF-8 key (folder roles and file
 * names share props->name; lossless conversion keeps raw-byte names
 * comparable). */
std::string node_path_key(const tree::FileNode* n);

/* Prune one root's subtree down to the filter prefixes. Returns the
 * removal counts. The root itself always stays (a result root is
 * presentation, not content). Call on the scan thread between stages
 * (single-writer handoff, the detach_subtree discipline). */
PruneStats prune_root(tree::ResultTree& tree, tree::FileNode* root,
                      const std::vector<std::string>& prefixes);

} // namespace folderfilter
} // namespace cr

#endif // CR_FOLDERFILTER_H_
