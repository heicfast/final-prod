/*
 * cr_fve_policy.h - the F61 BitLocker-policy denial diagnosis (M-105
 * option (a), the no-service posture).
 *
 * The finding (M-105, the DRWService install lifecycle): the og ships a
 * transient LocalSystem SCM service ("Read FVE Server") so a
 * non-elevated host can harvest the FVE auto-unlock keys from the
 * SYSTEM-ACL'd live store; the core keeps the in-process harvest (the
 * registered DIVERGENCE-INFORMATIONAL posture, PARITY-DELTAS D-16x) but
 * must distinguish "no keys configured" from "the store exists but this
 * token cannot read it" - the exact diagnosis the og's service exists
 * to provide. This header is the pure rule (the M-075 cr_fsck_launch
 * pattern: the policy rides cr::core on BOTH lanes; the win-layer
 * harvest feeds it, the UI surfaces it).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_FVE_POLICY_H_
#define CR_FVE_POLICY_H_

#include "cr/cr_types.h"
#include "cr/cr_error.h"

namespace cr {
namespace health {

/* The FVE denial diagnosis (M-105 option (a)).
 *
 * Inputs (the harvest's own evidence, gathered by the win layer):
 *  - live_store_denied: the live-hive open of
 *    SYSTEM\CurrentControlSet\Control\FVEAutoUnlock failed with
 *    ERROR_ACCESS_DENIED (the store EXISTS - RegOpenKeyExW returns
 *    ERROR_FILE_NOT_FOUND for an absent key - but this token cannot
 *    read it; the key material is SYSTEM/admin-ACL'd).
 *  - offline_hive_denied: an offline probe volume's SYSTEM hive FILE
 *    was present but the open failed with ERROR_ACCESS_DENIED (the same
 *    ACL class on the file).
 *  - token_elevated: the cr_elevate seam's TokenElevation probe (an
 *    elevated token can read the store; a non-elevated one cannot - the
 *    standard-user population the og's service existed to serve).
 *
 * Output: ERR_FVE_ELEVATION_REQUIRED ("FVE auto-unlock keys exist but
 * require an elevated token") when a denial was observed and the token
 * is not elevated; ERR_FVE_LOCKED otherwise (the existing loud refusal -
 * a store that genuinely does not exist, or a denial an elevated token
 * still could not clear, is NOT an elevation claim). */
u32 fve_denial_diagnosis(bool live_store_denied, bool offline_hive_denied, bool token_elevated);

/* The UI-facing text for the diagnosis (cr_error.cpp's table carries the
 * short form; this is the long form the open-volume refusal renders). */
const char* fve_denial_guidance(u32 err);

} // namespace health
} // namespace cr

#endif // CR_FVE_POLICY_H_
