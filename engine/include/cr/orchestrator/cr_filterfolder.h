/*
 * cr_filterfolder.h - the result-set junk filter (M-018, diff-to-fix).
 *
 * Corpus: the og's shipped config/FilterFolder.ini (via artifact
 * filtering-priority--config-layer.md §3, byte-exact lists re-read
 * first-hand from the file), pushed per scan-partition at scan setup
 * (raw-evidence ui-interaction-recovery-flow/applog_log_DRWUI.log
 * :22:57:57.920 "run to CDoScanPartition::PushFolderFilter:84";
 * config-data-reference.md :36-41 "hide OS junk from results").
 *
 * Three hidden sets (the og's entry order preserved, generated data -
 * scripts/gen_filterfolder.py):
 *   FolderName   - folder names: Windows upgrade/setup scaffolding
 *                  (;Config.Msi;$Windows.~BT;$Windows.~WS;$Windows.~TMP;)
 *   FileName     - file names: filesystem metadata noise
 *                  (;desktop.ini;._.DS_Store;.DS_Store;$UsnJrnl;
 *                   IndexerVolumeGuid;WPSettings.dat;
 *                   MountPointManagerRemoteDatabase;)
 *   FatFilterName- the localized "New Folder" family (10 locales) +
 *                  System Volume Information + the BitLocker
 *                  recovery-password artifacts (mixed files/folders)
 *
 * Application point: the scan pipeline runs filterfolder::apply() once
 * after every stage completes (post-walk, pre-grading, pre-session
 * capture) - flagged nodes carry NODE_FLAG_JUNK_FILTERED and the
 * presentation surfaces (query rows, CLI tree, recover-all planning)
 * skip them, so sessions/reports/recovery agree with the og's presented
 * tree. Matching: exact name equality, ASCII-case-insensitive (Windows
 * filesystem semantics; the og's comparator mode is not decoded in the
 * corpus - the choice is registered in PARITY-DELTAS).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_FILTERFOLDER_H_
#define CR_FILTERFOLDER_H_

#include "../cr_types.h"
#include "../tree/cr_node.h"
#include <string>

namespace cr {
namespace filterfolder {

/* generated data (cr_filterfolder_data.cpp, scripts/gen_filterfolder.py):
 * semicolon-delimited UTF-8 pools, the og's entry order, each pool
 * NUL-terminated (the consumer reads them as C strings). Counts are
 * pinned by the FilterFolder tests (regeneration-drift guard). */
namespace data {
extern const u8 kFolderPool[];
extern const u8 kFilePool[];
extern const u8 kFatNamePool[];
/* N-081: the pools are semicolon-delimited with a TRAILING delimiter and
 * NO NUL terminator - reading them as C strings overruns the arrays
 * (global-buffer-overflow under ASan, first caught on the linux-gcc
 * lane). The byte lengths pin the parse to the array bounds. */
extern const u32 kFolderPoolLen;
extern const u32 kFilePoolLen;
extern const u32 kFatNamePoolLen;
extern const u32 kFolderCount;
extern const u32 kFileCount;
extern const u32 kFatNameCount;
} // namespace data

/* Does this node name belong to the og's hidden sets?
 * dir=true consults FolderName + FatFilterName; dir=false consults
 * FileName + FatFilterName (the FatFilterName list mixes both kinds -
 * System Volume Information is a folder, the BitLocker artifacts are
 * files - and the og's list is name-keyed, not kind-keyed). */
bool is_junk_name(const std::u16string& name, bool is_dir);

/* Mark every matching node under every root of `tree` with
 * NODE_FLAG_JUNK_FILTERED (idempotent; hard links share props so a
 * shared name marks once). Returns the number of nodes flagged this
 * call (telemetry parity: ScanStats.junk_filtered). */
u64 apply(tree::ResultTree& tree);

} // namespace filterfolder
} // namespace cr

#endif // CR_FILTERFOLDER_H_
