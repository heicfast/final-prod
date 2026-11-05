/* cr_fve_policy.cpp - the F61 denial-diagnosis rule (M-105 option (a)).
 *
 * Pure policy, no OS surface: the win layer's harvest gathers the
 * evidence (the live-hive open's LSTATUS, the offline hive-file open's
 * error, the cr_elevate probe) and calls the rule here; keeping it in
 * the portable core makes the rule testable on both lanes (the M-075
 * cr_fsck_launch pattern).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/health/cr_fve_policy.h"

namespace cr {
namespace health {

u32 fve_denial_diagnosis(bool live_store_denied, bool offline_hive_denied, bool token_elevated) {
    if (token_elevated)
        return ERR_FVE_LOCKED; /* an elevated token that still cannot read
                                * the store is not an elevation claim - the
                                * loud refusal stands (never silent) */
    if (live_store_denied || offline_hive_denied)
        return ERR_FVE_ELEVATION_REQUIRED; /* the store exists (the open
                                            * denied, not FILE_NOT_FOUND) -
                                            * the og's service existed to
                                            * bridge exactly this ACL */
    return ERR_FVE_LOCKED;                 /* no denial observed: the honest default */
}

const char* fve_denial_guidance(u32 err) {
    if (err == ERR_FVE_ELEVATION_REQUIRED)
        return "FVE auto-unlock keys exist but require an elevated token - "
               "run elevated and re-try";
    return "unlock (Unlock-BitLocker -MountPoint %c: -RecoveryPassword <key>) "
           "and re-try";
}

} // namespace health
} // namespace cr
