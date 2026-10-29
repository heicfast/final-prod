/* test_fve_policy.cpp - the M-105 option (a) denial-diagnosis battery
 * (both lanes: the pure rule rides cr::core, the M-075 cr_fsck_launch
 * pattern).
 *
 * The finding's falsifiable claim #4: "In a non-elevated core run (a
 * posture the core deliberately supports, D-60a), a BitLocker
 * auto-unlock volume the og unlocks (via its LocalSystem service or its
 * always-elevated UI) stays locked in the core, with no surfaced
 * elevation diagnosis specific to FVE (the harvest returns
 * ERR_FVE_LOCKED without distinguishing 'no keys configured' from 'keys
 * exist but unreadable by this token')."
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include <gtest/gtest.h>
#include "cr/health/cr_fve_policy.h"
#include "cr/cr_error.h"

using namespace cr;
using namespace cr::health;

/* ---- the diagnosis matrix (the rule itself) ------------------------ */

TEST(FvePolicy, AclDenialOnNonElevatedTokenIsTheElevationClaim) {
    /* the live store EXISTS (the open returned ERROR_ACCESS_DENIED, not
     * ERROR_FILE_NOT_FOUND) but the token is not elevated -> the og's
     * service existed to bridge exactly this; the core's registered
     * no-service posture surfaces the elevation claim instead. */
    EXPECT_EQ(fve_denial_diagnosis(true, false, false), u32(ERR_FVE_ELEVATION_REQUIRED));
    EXPECT_EQ(fve_denial_diagnosis(false, true, false), u32(ERR_FVE_ELEVATION_REQUIRED));
    EXPECT_EQ(fve_denial_diagnosis(true, true, false), u32(ERR_FVE_ELEVATION_REQUIRED));
}

TEST(FvePolicy, AbsentStoreStaysTheLoudRefusal) {
    /* no denial observed (FILE_NOT_FOUND or a successful read that found
     * no usable keys): the honest ERR_FVE_LOCKED - never a silent success,
     * never a false elevation claim. */
    EXPECT_EQ(fve_denial_diagnosis(false, false, false), u32(ERR_FVE_LOCKED));
    EXPECT_EQ(fve_denial_diagnosis(false, false, true), u32(ERR_FVE_LOCKED));
}

TEST(FvePolicy, ElevatedTokenMakesNoElevationClaim) {
    /* an elevated token that still cannot read the store is NOT an
     * elevation claim - the loud refusal stands (the finding's own
     * conservative reading; a false claim would send users chasing a
     * UAC dialog that changes nothing). */
    EXPECT_EQ(fve_denial_diagnosis(true, false, true), u32(ERR_FVE_LOCKED));
    EXPECT_EQ(fve_denial_diagnosis(true, true, true), u32(ERR_FVE_LOCKED));
}

/* ---- the surfacing (the code + the text) --------------------------- */

TEST(FvePolicy, ErrorCodeRegisteredInPolicyFamily) {
    /* append-only in the 0x5006 bitlocker-policy family; the error table
     * carries the short text (cr_error.cpp), the guidance carries the
     * long form (the open-volume refusal renders it). */
    EXPECT_EQ(u32(ERR_FVE_ELEVATION_REQUIRED), 0x50060002u);
    EXPECT_NE(ERR_FVE_ELEVATION_REQUIRED, ERR_FVE_LOCKED);
    EXPECT_STRNE(fve_denial_guidance(ERR_FVE_ELEVATION_REQUIRED),
                 fve_denial_guidance(ERR_FVE_LOCKED));
    EXPECT_STREQ(fve_denial_guidance(ERR_FVE_ELEVATION_REQUIRED),
                 "FVE auto-unlock keys exist but require an elevated token - "
                 "run elevated and re-try");
    EXPECT_STREQ(cr::err_text(ERR_FVE_ELEVATION_REQUIRED),
                 "FVE auto-unlock keys exist but require an elevated token");
}
