/*
 * cr_elevate.h - definitive process-elevation probe (GAP-P1-5).
 *
 * Replaces the "(admin?)" guesses at every raw-device open failure with a
 * diagnosis: TokenElevation via OpenProcessToken tells whether the process
 * token is actually elevated (UAC: a non-elevated admin still gets FALSE -
 * raw \\.\PhysicalDriveN access needs the ELEVATED token, not mere group
 * membership), which is exactly the distinction the error text needs.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_ELEVATE_H_
#define CR_ELEVATE_H_

namespace cr {
namespace win {

/* True when the current process token is elevated (admin group enabled by
 * UAC). FALSE for standard users AND non-elevated admins. On any probe
 * failure returns FALSE (the caller's message stays honest: "requires
 * elevation"). */
bool is_elevated();

} // namespace win
} // namespace cr

#endif // CR_ELEVATE_H_
