/* cr_lastscan.h - the last-scan pointer (the auto-session lifecycle).
 *
 * M-154 (diff-to-fix): the og's auto-session lifecycle is bounded and
 * pointed - before writing a new auto session it reads LastScanInfo.ini
 * (the [Info] section's SavePath/Name/Count/Time, DRWUI:443060-443086)
 * and DELETES the previous auto .rsf (the retention loop :442815-442833),
 * keeping exactly one live auto session; the pointer (plus an HKLM
 * SOFTWARE\EASEUS\DRW mirror - the Windows-host convention, deliberately
 * dropped, PARITY-DELTAS) drives the continue-last flow, validated with
 * PathFileExistsA (:443209-443214: a dead pointer never blocks a new
 * save). The core's pointer is a line-based sidecar `last_scan.info` in
 * the autosave dir carrying the og's exact four fields - the semantics
 * preserved, the location convention registered as the difference.
 *
 * Licensed under the MIT license. See LICENSE.
 */
#ifndef CR_LASTSCAN_H
#define CR_LASTSCAN_H

#include <cr/cr_types.h>
#include <string>

namespace cr {
namespace session {

struct LastScanInfo {
    std::string save_path; /* the full path of the last auto session */
    std::string name;      /* the file name (the og's Name field) */
    u64 count = 0;         /* the og's Count field (the surfaced node count) */
    u64 time = 0;          /* the save time (unix seconds) */
    bool exists = false;   /* the PathFileExistsA validation semantic */
};

/* Read the pointer from <dir>/last_scan.info. Returns false when the
 * sidecar is absent or carries no save_path (the og's empty-record
 * class: the caller proceeds without retention). `exists` reflects the
 * pointed file's presence - a dead pointer is data, not an error. */
bool last_scan_info_read(const std::string& dir, LastScanInfo& out);

/* Write the pointer atomically (tmp+rename, the session writer's own
 * convention). Returns false on I/O failure - the caller treats the
 * pointer as best-effort (the og's WritePrivateProfileStringA posture:
 * its failure never fails the save). */
bool last_scan_info_write(const std::string& dir, const LastScanInfo& in);

/* The retention half of the og's loop: delete the PREVIOUS auto session
 * named by the pointer. Never touches a non-auto_ file (manual saves
 * are never deleted - the og's DeleteFileA targets the pointer's
 * SavePath, which only ever names an auto_*.rsf the writer itself
 * recorded). A dead/absent pointer is a no-op. Returns true when a file
 * was removed. */
bool last_scan_retention_delete_previous(const std::string& dir);

} // namespace session
} // namespace cr

#endif /* CR_LASTSCAN_H */
